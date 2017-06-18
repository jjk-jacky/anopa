/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * aa-mvlog.c
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
#include <sys/stat.h>
#include <unistd.h>
#include <skalibs/djbunix.h>
#include <skalibs/bytestr.h>
#include <anopa/common.h>
#include <anopa/output.h>
#include <anopa/copy_file.h>

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION] LOGFILE DESTDIR",
            " -D, --double-output           Enable double-output mode\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-mvlog";
    struct stat st;

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

    if (stat (argv[0], &st) < 0)
        aa_strerr_diefu2sys (2, "stat ", argv[0]);
    else if (!S_ISREG (st.st_mode))
        aa_strerr_dief2x (2, argv[0], ": not a file");

    {
        int l = strlen (argv[1]);
        char newname[l + 27];
        char target[26];

        byte_copy (newname, l, argv[1]);
        newname[l] = '/';
        if (openreadnclose (argv[0], newname + l + 1, 25) != 25)
            aa_strerr_diefu2sys (2, "read new name from ", argv[0]);
        if (newname[l + 1] != '@'
                || byte_chr (newname + l + 1, 25, '/') < 25
                || byte_chr (newname + l + 1, 25, '\0') < 25)
            aa_strerr_dief2x (2, "invalid new name read from ", argv[0]);
        newname[l + 26] = '\0';

        if (aa_copy_file (argv[0], newname, st.st_mode, AA_CP_CREATE) < 0)
            aa_strerr_diefu4sys (2, "copy ", argv[0], " as ", newname);

        byte_copy (target, 26, newname + l + 1);
        byte_copy (newname + l + 1, 8, "current");
        unlink (newname);
        if (symlink (target, newname) < 0)
            aa_strerr_warnu2sys ("create symlink ", newname);
        if (unlink (argv[0]) < 0)
            aa_strerr_warnu2sys ("remove source file ", argv[0]);
    }

    return 0;
}
