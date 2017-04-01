/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * exec_longrun.c
 * Copyright (C) 2015-2017 Olivier Brunel <jjk@jjacky.com>
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
#include <strings.h>
#include <errno.h>
#include <skalibs/djbunix.h>
#include <skalibs/bytestr.h>
#include <skalibs/tai.h>
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
    size_t l_sn = strlen (aa_service_name (s));
    char fifodir[l_sn + 1 + sizeof (S6_SUPERVISE_EVENTDIR)];
    tain_t deadline;
    int is_start = (mode & AA_MODE_START) ? 1 : 0;
    const char *event = (is_start) ? ((s->gets_ready) ? "[udU]" : "u") : "D";
    const char *cmd = (is_start) ? "u" : (mode & AA_MODE_STOP_ALL) ? "dx" : "d";
    int already = 0;

    byte_copy (fifodir, l_sn, aa_service_name (s));
    fifodir[l_sn] = '/';
    byte_copy (fifodir + l_sn + 1, sizeof (S6_SUPERVISE_EVENTDIR), S6_SUPERVISE_EVENTDIR);

    tain_addsec_g (&deadline, 1);
    s->ft_id = ftrigr_subscribe_g (&_aa_ft, fifodir, event,
                                   (*event == '[') ? FTRIGR_REPEAT : 0,
                                   &deadline);
    if (s->ft_id == 0)
    {
        /* this could happen e.g. if the servicedir isn't in scandir, if
         * something failed during aa-enable for example */

        const char *errmsg = "Failed to subscribe to eventdir: ";
        size_t l_msg = strlen (errmsg);
        const char *errstr = strerror (errno);
        size_t l_err = strlen (errstr);
        char msg[l_msg + l_err + 1];

        byte_copy (msg, l_msg, errmsg);
        byte_copy (msg + l_msg, l_err + 1, errstr);

        s->st.event = (is_start) ? AA_EVT_STARTING_FAILED : AA_EVT_STOPPING_FAILED;
        s->st.code = ERR_S6;
        tain_copynow (&s->st.stamp);
        aa_service_status_set_msg (&s->st, msg);
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
            /* already there: unsubcribe.
             *
             * Failure to unsubscribe w/ EINVAL means that the service got where
             * we wanted *after* we subscribed, and the event was already
             * treated/is queued up (when non-repeating, i.e. no FTRIGR_REPEAT).
             * Then, we want to process it later as usual (i.e. keep it & not
             * set already).
             */
            if (aa_unsubscribe_for (s->ft_id) == 0 || errno != EINVAL)
            {
                already = 1;
                s->ft_id = 0;
            }
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
         * happening right now. Also in STOP_ALL this sends the 'x' cmd to
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
            cmd = NULL;
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
            aa_service_status_set_msg (&s->st, strerror (errno));

            if (_exec_cb)
                _exec_cb (si, s->st.event, 0);
            return -1;
        }
    }

    if (cmd)
    {
        char dir[l_sn + 1 + sizeof (S6_SUPERVISE_CTLDIR) + 8];
        int r;

        byte_copy (dir, l_sn, aa_service_name (s));
        byte_copy (dir + l_sn, 9 + sizeof (S6_SUPERVISE_CTLDIR), "/" S6_SUPERVISE_CTLDIR "/control");

        r = s6_svc_write (dir, cmd, strlen (cmd));
        if (r <= 0 && !already)
        {
            aa_unsubscribe_for (s->ft_id);
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

static size_t idx = (size_t) -1;

int
aa_get_longrun_info (uint16_t *id, char *event)
{
    int r;

    if (idx == (size_t) -1)
    {
        r = ftrigr_update (&_aa_ft);
        if (r < 0)
            return -1;
        else if (r == 0)
            return 0;
        idx = 0;
    }

    for ( ; idx < genalloc_len (uint16_t, &_aa_ft.list); )
    {
        *id = genalloc_s (uint16_t, &_aa_ft.list)[idx];
        r = ftrigr_check (&_aa_ft, *id, event);
        ++idx;
        if (r != 0)
            return (r < 0) ? -2 : r;
    }

    idx = (size_t) -1;
    return 0;
}

int
aa_unsubscribe_for (uint16_t id)
{
    tain_t deadline;

    tain_addsec_g (&deadline, 1);
    return (ftrigr_unsubscribe_g (&_aa_ft, id, &deadline)) ? 0 : -1;
}
