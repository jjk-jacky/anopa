/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * aa-ctty.c
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
#include <termios.h>
#include <sys/ioctl.h>
#include <skalibs/uint.h>
#include <skalibs/djbunix.h>
#include <anopa/common.h>
#include <anopa/output.h>

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION...] PROG...",
            " -f, --fd=FD                   Use FD as terminal (Default: 0)\n"
            " -s, --steal                   Steal terminal from other session if needed\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[], char const * const *envp)
{
    PROG = "aa-ctty";
    int fd = 0;
    int steal = 0;

    for (;;)
    {
        struct option longopts[] = {
            { "fd",                 required_argument,  NULL,   'f' },
            { "help",               no_argument,        NULL,   'h' },
            { "steal",              no_argument,        NULL,   's' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "+f:hsV", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'f':
                if (!uint0_scan (optarg, &fd))
                    aa_strerr_diefu1sys (1, "set fd");
                break;

            case 'h':
                dieusage (0);

            case 's':
                steal = 1;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc == 0)
        dieusage (1);

    if (ioctl (fd, TIOCSCTTY, steal) < 0)
        aa_strerr_warnu1sys ("set controlling terminal");

    pathexec_run (argv[0], (char const * const *) argv, envp);
    aa_strerr_dieexec (111, argv[0]);
}
