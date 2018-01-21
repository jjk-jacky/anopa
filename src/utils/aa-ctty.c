/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * aa-ctty.c
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
#include <termios.h>
#include <sys/ioctl.h>
#include <skalibs/types.h>
#include <skalibs/djbunix.h>
#include <anopa/common.h>
#include <anopa/output.h>

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION...] PROG...",
            " -D, --double-output           Enable double-output mode\n"
            " -O, --log-file FILE|FD        Write log to FILE|FD\n"
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
            { "double-output",      no_argument,        NULL,   'D' },
            { "fd",                 required_argument,  NULL,   'f' },
            { "help",               no_argument,        NULL,   'h' },
            { "log-file",           required_argument,  NULL,   'O' },
            { "steal",              no_argument,        NULL,   's' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "+Df:hO:sV", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'D':
                aa_set_double_output (1);
                break;

            case 'f':
                if (!uint0_scan (optarg, (unsigned int *) &fd))
                    aa_strerr_diefu1sys (RC_FATAL_USAGE, "set fd");
                break;

            case 'h':
                dieusage (RC_OK);

            case 'O':
                aa_set_log_file_or_die (optarg);
                break;

            case 's':
                steal = 1;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (RC_FATAL_USAGE);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc == 0)
        dieusage (RC_FATAL_USAGE);

    if (ioctl (fd, TIOCSCTTY, steal) < 0)
        aa_strerr_warnu1sys ("set controlling terminal");

    pathexec_run (argv[0], (char const * const *) argv, envp);
    aa_strerr_dieexec (RC_FATAL_EXEC, argv[0]);
}
