/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * aa-umount.c
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

#include <getopt.h>
#include <sys/mount.h>
#include <anopa/common.h>
#include <anopa/output.h>

#ifndef NULL
#define NULL    (void *) 0
#endif

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTIONS...] MOUNTPOINT",
            " -D, --double-output           Enable double-output mode\n"
            " -O, --log-file FILE|FD        Write log to FILE|FD\n"
            " -f, --force                   Force unmount even if busy (NFS only)\n"
            " -l, --lazy                    Perform lazy unmounting\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-umount";
    int flags = 0;

    for (;;)
    {
        struct option longopts[] = {
            { "double-output",      no_argument,        NULL,   'D' },
            { "force",              no_argument,        NULL,   'f' },
            { "help",               no_argument,        NULL,   'h' },
            { "lazy",               no_argument,        NULL,   'l' },
            { "log-file",           required_argument,  NULL,   'O' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "DfhlO:V", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'D':
                aa_set_double_output (1);
                break;

            case 'f':
                flags = MNT_FORCE;
                break;

            case 'h':
                dieusage (RC_OK);

            case 'l':
                flags = MNT_DETACH;
                break;

            case 'O':
                aa_set_log_file_or_die (optarg);
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (RC_FATAL_USAGE);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1)
        dieusage (RC_FATAL_USAGE);

    if (umount2 (argv[0], flags) < 0)
        aa_strerr_diefu2sys (RC_FATAL_IO, "unmount ", argv[0]);

    return RC_OK;
}
