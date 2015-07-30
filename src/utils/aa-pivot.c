/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * aa-pivot.c
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
#include <anopa/common.h>
#include <anopa/output.h>

#ifndef NULL
#define NULL    (void *) 0
#endif

extern int pivot_root (char const *new_root, char const *old_root);

static void
dieusage (int rc)
{
    aa_die_usage (rc, "NEWROOT OLDROOT",
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-pivot";

    for (;;)
    {
        struct option longopts[] = {
            { "help",               no_argument,        NULL,   'h' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "hV", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
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

    if (pivot_root (argv[0], argv[1]) < 0)
        aa_strerr_diefu2sys (2, "pivot into ", argv[0]);
    return 0;
}
