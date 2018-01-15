/*
 * anopa - Copyright (C) 2015-2018 Olivier Brunel
 *
 * aa-hiercopy.c
 * Copyright (C) 2018 Olivier Brunel <jjk@jjacky.com>
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
#include <sys/stat.h>
#include <unistd.h>
#include <anopa/common.h>
#include <anopa/output.h>
#include <skalibs/djbunix.h>

#ifndef NULL
#define NULL    (void *) 0
#endif

static void
dieusage (int rc)
{
    aa_die_usage (rc, "SRC DST",
            " -D, --double-output           Enable double-output mode\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-hiercopy";

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

    if (argc != 2)
        dieusage (1);

    {
        stralloc sa = STRALLOC_ZERO;
        struct stat st;
        char *sce = argv[0];

        if (lstat (sce, &st) < 0)
            aa_strerr_diefu2sys (2, "lstat ", sce);
        if (S_ISLNK (st.st_mode))
        {
            if (sarealpath (&sa, sce) < 0)
                aa_strerr_diefu2x (3, "resolve symlink ", sce);
            sce = sa.s;
        }

        if (!hiercopy(sce, argv[1]))
            aa_strerr_diefu4sys (4, "copy ", sce, " into ", argv[1]);
        if (sa.a > 0)
            stralloc_free (&sa);
    }

    return 0;
}
