/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * aa-reset.c
 * Copyright (C) 2015-2018 Olivier Brunel <jjk@jjacky.com>
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

#define _BSD_SOURCE

#include "anopa/config.h"

#include <errno.h>
#include <strings.h>
#include <getopt.h>
#include <skalibs/tai.h>
#include <skalibs/bytestr.h>
#include <anopa/common.h>
#include <anopa/output.h>
#include <anopa/get_repodir.h>
#include <anopa/init_repo.h>
#include <anopa/service.h>
#include <anopa/service_status.h>
#include <anopa/err.h>
#include "util.h"
#include "common.h"

enum
{
    MODE_NONE = 0,
    MODE_AUTO,
    MODE_STARTED,
    MODE_STOPPED
};

static int rc = RC_OK;

static void
reset_service (const char *name, intptr_t mode)
{
    aa_service *s;
    int si;
    int r;
    aa_evt old_event;
    aa_evt event;

    r = aa_get_service (name, &si, 1);
    if (r < 0)
    {
        aa_put_err (name, errmsg[-r], 1);
        rc |= (r == -ERR_UNKNOWN) ? RC_ST_UNKNOWN : RC_ST_FAILED;
        return;
    }

    r = aa_preload_service (si);
    if (r < 0)
    {
        aa_put_err (name, errmsg[-r], 1);
        rc |= RC_ST_FAILED;
        return;
    }

    s = aa_service (si);
    if (aa_service_status_read (&s->st, aa_service_name (s)) < 0 && errno != ENOENT)
    {
        int e = errno;

        aa_put_err (name, "Failed to read service status file: ", 0);
        aa_bs (AA_ERR, strerror (e));
        aa_end_err ();

        rc |= RC_ST_FAILED;
        return;
    }

    if (s->st.type == AA_TYPE_LONGRUN)
    {
        aa_put_err (name, "Can only reset one-shot services", 1);
        rc |= RC_ST_FAILED;
        return;
    }

    /* Starting/Stopping cannot be reset */
    if (s->st.event == AA_EVT_STARTING || s->st.event == AA_EVT_STOPPING)
        return;

    if (mode == MODE_AUTO)
    {
        if (s->st.event == AA_EVT_STARTING_FAILED || s->st.event == AA_EVT_START_FAILED)
            event = AA_EVT_STARTED;
        else if (s->st.event == AA_EVT_STOPPING_FAILED || s->st.event == AA_EVT_STOP_FAILED)
            event = AA_EVT_STOPPED;
        else
            return;
    }
    else
        event = (mode == MODE_STARTED) ? AA_EVT_STARTED : AA_EVT_STOPPED;

    if (s->st.event == event)
        return;

    tain_now_g ();
    old_event = s->st.event;
    s->st.event = event;
    s->st.stamp = STAMP;
    aa_service_status_set_msg (&s->st, "");
    if (aa_service_status_write (&s->st, aa_service_name (s)) < 0)
    {
        int e = errno;

        aa_put_err (name, "Failed to write service status file: ", 0);
        aa_bs (AA_ERR, strerror (e));
        aa_end_err ();

        rc |= RC_ST_FAILED;
    }
    else
    {
        aa_put_title (1, name, "", 0);
        aa_is (AA_OUT, ANSI_HIGHLIGHT_OFF);
        aa_bs (AA_OUT, eventmsg[old_event]);
        aa_is (AA_OUT, ANSI_HIGHLIGHT_ON);
        aa_bs (AA_OUT, " -> ");
        aa_bs (AA_OUT, eventmsg[event]);
        aa_end_title ();
    }
}

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION...] [service...]",
            " -D, --double-output           Enable double-output mode\n"
            " -O, --log-file FILE|FD        Write log to FILE|FD\n"
            " -r, --repodir DIR             Use DIR as repository directory\n"
            " -A, --auto                    Automatic mode\n"
            " -a, --started                 Reset to Started\n"
            " -o, --stopped                 Reset to Stopped\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-reset";
    const char *path_repo = aa_get_repodir ();
    intptr_t mode = MODE_NONE;
    int i;
    int r;

    for (;;)
    {
        struct option longopts[] = {
            { "auto",               no_argument,        NULL,   'A' },
            { "started",            no_argument,        NULL,   'a' },
            { "double-output",      no_argument,        NULL,   'D' },
            { "help",               no_argument,        NULL,   'h' },
            { "log-file",           required_argument,  NULL,   'O' },
            { "stopped",            no_argument,        NULL,   'o' },
            { "repodir",            required_argument,  NULL,   'r' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "AaDhO:or:V", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'A':
                mode = MODE_AUTO;
                break;

            case 'a':
                mode = MODE_STARTED;
                break;

            case 'D':
                aa_set_double_output (1);
                break;

            case 'h':
                dieusage (RC_OK);

            case 'O':
                aa_set_log_file_or_die (optarg);
                break;

            case 'o':
                mode = MODE_STOPPED;
                break;

            case 'r':
                unslash (optarg);
                path_repo = optarg;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (RC_FATAL_USAGE);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 1 || mode == MODE_NONE)
        dieusage (RC_FATAL_USAGE);

    r = aa_init_repo (path_repo, AA_REPO_READ);
    if (r < 0)
        aa_strerr_diefu2sys (RC_FATAL_INIT_REPO, "init repository ", path_repo);

    for (i = 0; i < argc; ++i)
        if (str_equal (argv[i], "-"))
        {
            if (process_names_from_stdin ((names_cb) reset_service, (void *) mode) < 0)
                aa_strerr_diefu1sys (RC_FATAL_IO, "process names from stdin");
        }
        else
            reset_service (argv[i], mode);

    return rc;
}
