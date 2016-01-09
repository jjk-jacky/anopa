/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * exec_longrun.c
 * Copyright (C) 2015 Olivier Brunel <jjk@jjacky.com>
 *
 * This file is part of anopa.
 *
 * anopa is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * anopa is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * anopa. If not, see http://www.gnu.org/licenses/
 */

#include <unistd.h>
#include <errno.h>
#include <skalibs/djbunix.h>
#include <skalibs/bytestr.h>
#include <skalibs/tai.h>
#include <skalibs/error.h>
#include <s6/s6-supervise.h>
#include <s6/ftrigr.h>
#include <anopa/service.h>
#include <anopa/err.h>
#include <anopa/output.h>
#include "service_internal.h"

int
_exec_longrun (int si, aa_mode mode)
{
    aa_service *s = aa_service (si);
    s6_svstatus_t st6 = S6_SVSTATUS_ZERO;
    int l_sn = strlen (aa_service_name (s));
    char fifodir[l_sn + 1 + sizeof (S6_SUPERVISE_EVENTDIR)];
    tain_t deadline;
    int is_start = (mode & AA_MODE_START) ? 1 : 0;
    char *event = (is_start) ? "u" : "d";
    int already = 0;

    byte_copy (fifodir, l_sn, aa_service_name (s));
    fifodir[l_sn] = '/';
    byte_copy (fifodir + l_sn + 1, sizeof (S6_SUPERVISE_EVENTDIR), S6_SUPERVISE_EVENTDIR);

    tain_addsec_g (&deadline, 1);
    s->ft_id = ftrigr_subscribe_g (&_aa_ft, fifodir,
            (is_start && s->gets_ready) ? "[udU]" : event,
            (is_start && s->gets_ready) ? FTRIGR_REPEAT : 0,
            &deadline);
    if (s->ft_id == 0)
    {
        /* this could happen e.g. if the servicedir isn't in scandir, if
         * something failed during aa-enable for example */

        s->st.event = (is_start) ? AA_EVT_STARTING_FAILED : AA_EVT_STOPPING_FAILED;
        s->st.code = ERR_S6;
        tain_copynow (&s->st.stamp);
        aa_service_status_set_msg (&s->st, "Failed to subscribe to eventdir");
        if (aa_service_status_write (&s->st, aa_service_name (s)) < 0)
            aa_strerr_warnu2sys ("write service status file for ", aa_service_name (s));

        if (_exec_cb)
            _exec_cb (si, s->st.event, 0);
        return -1;
    }

    if (s6_svstatus_read (aa_service_name (s), &st6)
            && ((is_start && (st6.pid && !st6.flagfinishing))
                || (!is_start && !(st6.pid && !st6.flagfinishing))))
    {
        tain_now_g ();

        if (!is_start || !s->gets_ready || st6.flagready)
        {
            already = 1;
            /* already there; unsubcribe */
            ftrigr_unsubscribe_g (&_aa_ft, s->ft_id, &deadline);
            s->ft_id = 0;
        }

        /* make sure our status is correct, and timestamped before s6 */
        s->st.event = (is_start) ? AA_EVT_STARTING : AA_EVT_STOPPING;
        tain_addsec (&s->st.stamp, &st6.stamp, -1);
        aa_service_status_set_msg (&s->st, "");
        if (aa_service_status_write (&s->st, aa_service_name (s)) < 0)
            aa_strerr_warnu2sys ("write service status file for ", aa_service_name (s));

        /* we still process it. Because we checked the service state (from s6)
         * before adding it to the "transaction" (i.e. in
         * aa_ensure_service_loaded()) and it wasn't there already.
         * IOW this is likely e.g. that it crashed since then, but it isn't
         * really down, or something. So make sure we do send the request to
         * s6-supervise, so it isn't restarted, or indeed brought down if it's
         * happening right now. Also in STOP_ALL this sends the 'x' event to
         * s6-supervise as needed as well.
         */

        if (is_start && s->gets_ready)
            /* However, on START sending the command is possibly problematic
             * regarding (externally set) readiness.
             * E.g. if the service is ready, and readiness was set externally
             * (e.g. via aa-setready) it would result in said readiness being
             * "lost" when s6-supervise rewrites the status file.
             * Similarly, if readiness was originally set via s6-supervise
             * (notification-fd) and then unset externally, it could come back
             * if s6-supervise was to rewrite the status file.
             */
            event = NULL;
    }
    else
    {
        tain_now_g ();

        s->st.event = (is_start) ? AA_EVT_STARTING : AA_EVT_STOPPING;
        tain_copynow (&s->st.stamp);
        aa_service_status_set_msg (&s->st, "");
        if (aa_service_status_write (&s->st, aa_service_name (s)) < 0)
        {
            s->st.event = (is_start) ? AA_EVT_STARTING_FAILED : AA_EVT_STOPPING_FAILED;
            s->st.code = ERR_WRITE_STATUS;
            aa_service_status_set_msg (&s->st, error_str (errno));

            if (_exec_cb)
                _exec_cb (si, s->st.event, 0);
            return -1;
        }
    }

    if (event)
    {
        char dir[l_sn + 1 + sizeof (S6_SUPERVISE_CTLDIR) + 8];
        int r;

        byte_copy (dir, l_sn, aa_service_name (s));
        byte_copy (dir + l_sn, 9 + sizeof (S6_SUPERVISE_CTLDIR), "/" S6_SUPERVISE_CTLDIR "/control");

        r = s6_svc_write (dir, (mode & AA_MODE_STOP_ALL) ? "dx" : event,
                (mode & AA_MODE_STOP_ALL) ? 2 : 1);
        if (r <= 0 && !already)
        {
            tain_addsec_g (&deadline, 1);
            ftrigr_unsubscribe_g (&_aa_ft, s->ft_id, &deadline);
            s->ft_id = 0;

            s->st.event = (is_start) ? AA_EVT_STARTING_FAILED : AA_EVT_STOPPING_FAILED;
            s->st.code = ERR_S6;
            tain_copynow (&s->st.stamp);
            aa_service_status_set_msg (&s->st, (r < 0)
                    ? "Failed to send command"
                    : "Supervisor not listenning");
            if (aa_service_status_write (&s->st, aa_service_name (s)) < 0)
                aa_strerr_warnu2sys ("write service status file for ", aa_service_name (s));

            if (_exec_cb)
                _exec_cb (si, s->st.event, 0);
            return -1;
        }
    }

    {
        char buf[l_sn + 6];

        byte_copy (buf, l_sn, aa_service_name (s));
        byte_copy (buf + l_sn, 6, "/down");

        if (is_start)
            unlink (buf);
        else
        {
            int fd;

            fd = open_create (buf);
            if (fd < 0)
                aa_strerr_warnu2sys ("create down file for ", aa_service_name (s));
            else
                fd_close (fd);
        }
    }

    if (already)
    {
        if (_exec_cb)
            _exec_cb (si, (is_start) ? -ERR_ALREADY_UP : -ERR_NOT_UP, 0);

        /* this was not a failure, but we return -1 to trigger a
         * aa_scan_mainlist() anyways, since the service changed state */
        return -1;
    }

    if (_exec_cb)
        _exec_cb (si, s->st.event, 0);
    return 0;
}

int
aa_get_longrun_info (uint16 *id, char *event)
{
    static int i = -1;
    int r;

    if (i == -1)
    {
        r = ftrigr_update (&_aa_ft);
        if (r <= 0)
            return -1;
        i = 0;
    }

    for ( ; i < genalloc_len (uint16, &_aa_ft.list); )
    {
        *id = genalloc_s (uint16, &_aa_ft.list)[i];
        r = ftrigr_check (&_aa_ft, *id, event);
        ++i;
        if (r != 0)
            return (r < 0) ? -2 : r;
    }

    i = -1;
    return 0;
}

int
aa_unsubscribe_for (uint16 id)
{
    tain_t deadline;

    tain_addsec_g (&deadline, 1);
    return ftrigr_unsubscribe_g (&_aa_ft, id, &deadline);
}
