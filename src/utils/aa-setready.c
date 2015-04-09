/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * aa-setready.c
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

#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <s6/ftrigw.h>
#include <s6/s6-supervise.h>
#include <anopa/common.h>

extern char const *PROG;

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION] SERVICEDIR",
            " -U, --ready                   Mark service ready; This is the default.\n"
            " -D, --unready                 Mark service not ready\n"
            "\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-setready";
    int ready = 1;
    int r = 0;

    for (;;)
    {
        struct option longopts[] = {
            { "unready",            no_argument,        NULL,   'D' },
            { "help",               no_argument,        NULL,   'h' },
            { "ready",              no_argument,        NULL,   'U' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "DhUV", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'D':
                ready = 0;
                break;

            case 'h':
                dieusage (0);

            case 'U':
                ready = 1;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1)
        dieusage (1);

    {
        int l = strlen (argv[0]);
        char fifodir[l + 1 + sizeof (S6_SUPERVISE_EVENTDIR)];
        char readyfile[l + 1 + sizeof (S6_SUPERVISE_READY_FILENAME)];

        byte_copy (fifodir, l, argv[0]);
        fifodir[l] = '/';
        byte_copy (fifodir + l + 1, sizeof (S6_SUPERVISE_EVENTDIR), S6_SUPERVISE_EVENTDIR);

        byte_copy (readyfile, l, argv[0]);
        readyfile[l] = '/';
        byte_copy (readyfile + l + 1, sizeof (S6_SUPERVISE_READY_FILENAME), S6_SUPERVISE_READY_FILENAME);

        if (ready)
        {
            char data[TAIN_PACK];

            if (!tain_now_g())
                strerr_diefu1sys (2, "tain_now");
            tain_pack (data, &STAMP);

            if (!openwritenclose_suffix (readyfile, data, TAIN_PACK, ".new"))
            {
                r = 3;
                strerr_warnwu2sys ("create ", readyfile);
            }
        }
        else
            if (unlink (readyfile) < 0 && errno != ENOENT)
            {
                r = 4;
                strerr_warnwu2sys ("remove ", readyfile);
            }

        if (ftrigw_notify (fifodir, (ready) ? 'U' : 'D') < 0)
        {
            r += 10;
            strerr_warnwu4sys ("send event ", (ready) ? "U": "D" , " via ", fifodir);
        }
    }

    return r;
}
