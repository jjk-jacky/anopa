/*
 * anopa - Copyright (C) 2015 Olivier Brunel
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
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-mvlog";
    struct stat st;

    if (argc == 2)
    {
        if (str_equal (argv[1], "-V") || str_equal (argv[1], "--version"))
            aa_die_version ();
        else if (str_equal (argv[1], "-h") || str_equal (argv[1], "--help"))
            dieusage (0);
    }
    if (argc != 3)
        dieusage (1);

    if (stat (argv[1], &st) < 0)
        aa_strerr_diefu2sys (2, "stat ", argv[1]);
    else if (!S_ISREG (st.st_mode))
        aa_strerr_dief2x (2, argv[1], ": not a file");

    {
        int l = strlen (argv[2]);
        char newname[l + 27];
        char target[26];

        byte_copy (newname, l, argv[2]);
        newname[l] = '/';
        if (openreadnclose (argv[1], newname + l + 1, 25) != 25)
            aa_strerr_diefu2sys (2, "read new name from ", argv[1]);
        if (newname[l + 1] != '@'
                || byte_chr (newname + l + 1, 25, '/') < 25
                || byte_chr (newname + l + 1, 25, '\0') < 25)
            aa_strerr_dief2x (2, "invalid new name read from ", argv[1]);
        newname[l + 26] = '\0';

        if (aa_copy_file (argv[1], newname, st.st_mode, AA_CP_CREATE) < 0)
            aa_strerr_diefu4sys (2, "copy ", argv[1], " as ", newname);

        byte_copy (target, 26, newname + l + 1);
        byte_copy (newname + l + 1, 8, "current");
        unlink (newname);
        if (symlink (target, newname) < 0)
            aa_strerr_warnu2sys ("create symlink ", newname);
        if (unlink (argv[1]) < 0)
            aa_strerr_warnu2sys ("remove source file ", argv[1]);
    }

    return 0;
}
