/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * service_status.c
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

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <skalibs/allreadwrite.h>
#include <skalibs/djbunix.h>
#include <skalibs/bytestr.h>
#include <skalibs/types.h>
#include <skalibs/tai.h>
#include <anopa/service_status.h>


void
aa_service_status_free (aa_service_status *svst)
{
    stralloc_free (&svst->sa);
}

int
aa_service_status_read (aa_service_status *svst, const char *dir)
{
    size_t len = strlen (dir);
    char file[len + 1 + sizeof (AA_SVST_FILENAME)];
    uint32_t u;

    /* most cases should be w/out a message, so we'll only need FIXED_SIZE and
     * one extra byte to NUL-terminate the (empty) message */
    if (!stralloc_ready_tuned (&svst->sa, AA_SVST_FIXED_SIZE + 1, 0, 0, 1))
        return -1;

    byte_copy (file, len, dir);
    byte_copy (file + len, 1 + sizeof (AA_SVST_FILENAME), "/" AA_SVST_FILENAME);

    if (!openreadfileclose (file, &svst->sa, AA_SVST_FIXED_SIZE + AA_SVST_MAX_MSG_SIZE)
            || svst->sa.len < AA_SVST_FIXED_SIZE)
    {
        int e = errno;
        tain_now_g ();
        errno = e;
        return -1;
    }
    tain_now_g ();

    if (svst->sa.len >= svst->sa.a
            && !stralloc_ready_tuned (&svst->sa, svst->sa.len + 1, 0, 0, 1))
        return -1;
    svst->sa.s[svst->sa.len] = '\0';
    svst->sa.len++;

    tain_unpack (svst->sa.s, &svst->stamp);
    uint32_unpack (svst->sa.s + 12, &u);
    svst->event = (unsigned int) u;
    uint32_unpack (svst->sa.s + 16, &u);
    svst->code = (int) u;

    return 0;
}

int
aa_service_status_write (aa_service_status *svst, const char *dir)
{
    size_t len = strlen (dir);
    char file[len + 1 + sizeof (AA_SVST_FILENAME)];
    mode_t mask;
    int r;
    int e;

    if (!stralloc_ready_tuned (&svst->sa, AA_SVST_FIXED_SIZE, 0, 0, 1))
        return -1;

    tain_pack (svst->sa.s, &svst->stamp);
    uint32_pack (svst->sa.s + 12, (uint32_t) svst->event);
    uint32_pack (svst->sa.s + 16, (uint32_t) svst->code);
    if (svst->sa.len < AA_SVST_FIXED_SIZE)
        svst->sa.len = AA_SVST_FIXED_SIZE;

    byte_copy (file, len, dir);
    byte_copy (file + len, 1 + sizeof (AA_SVST_FILENAME), "/" AA_SVST_FILENAME);

    mask = umask (0033);
    if (!openwritenclose_suffix (file, svst->sa.s,
                svst->sa.len + ((svst->sa.len > AA_SVST_FIXED_SIZE) ? -1 : 0), ".new"))
        r = -1;
    else
        r = 0;
    e = errno;
    umask (mask);

    tain_now_g ();
    errno = e;
    return r;
}

int
aa_service_status_set_msg (aa_service_status *svst, const char *msg)
{
    size_t len;

    len = strlen (msg);
    if (len > AA_SVST_MAX_MSG_SIZE)
        len = AA_SVST_MAX_MSG_SIZE;

    if (!stralloc_ready_tuned (&svst->sa, AA_SVST_FIXED_SIZE + len + 1, 0, 0, 1))
        return -1;

    svst->sa.len = AA_SVST_FIXED_SIZE;
    stralloc_catb (&svst->sa, msg, len);
    stralloc_0 (&svst->sa);
    return 0;
}

int
aa_service_status_set_err (aa_service_status *svst, int err, const char *msg)
{
    svst->event = AA_EVT_ERROR;
    svst->code = err;
    tain_copynow (&svst->stamp);
    return aa_service_status_set_msg (svst, (msg) ? msg : "");
}
