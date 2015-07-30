/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * aa-chroot.c
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

#define _BSD_SOURCE

#include <getopt.h>
#include <unistd.h>
#include <skalibs/djbunix.h>
#include <anopa/common.h>
#include <anopa/output.h>

static void
dieusage (int rc)
{
    aa_die_usage (rc, "NEWROOT COMMAND [ARG...]",
            " -D, --double-output           Enable double-output mode\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[], char * const envp[])
{
    PROG = "aa-chroot";

    for (;;)
    {
        struct option longopts[] = {
            { "double-output",      no_argument,        NULL,   'D' },
            { "help",               no_argument,        NULL,   'h' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "DhV", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'D':
                aa_set_double_output (1);
                break;

            case 'h':
                dieusage (0);

            case 'V':
                aa_die_version ();

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 2)
        dieusage (1);

    if (chdir (argv[0]) < 0)
        aa_strerr_diefu2sys (2, "chdir to ", argv[0]);
    if (chroot (".") < 0)
        aa_strerr_diefu1sys (3, "chroot");
    if (chdir ("/") < 0)
        aa_strerr_diefu1sys (3, "chdir to new root");
    pathexec_run (argv[1], (char const * const *) argv + 1, (char const * const *) envp);
    aa_strerr_dieexec (4, argv[1]);
}
