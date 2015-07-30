/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * aa-tty.c
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

#include <errno.h>
#include <skalibs/bytestr.h>
#include <skalibs/djbunix.h>
#include <anopa/output.h>
#include <anopa/common.h>

#define PREFIX  "/sys/class/tty/"
#define NAME    "/active"

static void
dieusage (int rc)
{
    aa_die_usage (rc, "",
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-tty";
    char file[256];
    int max = sizeof (file) - sizeof (PREFIX) - sizeof (NAME) + 1;
    char name[max];
    int r;

    if (argc == 2)
    {
        if (str_equal (argv[1], "-h") || str_equal (argv[1], "--help"))
            dieusage (0);
        else if (str_equal (argv[1], "-V") || str_equal (argv[1], "--version"))
            aa_die_version ();
        else
            dieusage (1);
    }
    else if (argc != 1)
        dieusage (1);

    byte_copy (file, sizeof (PREFIX) - 1, PREFIX);
    byte_copy (file + sizeof (PREFIX) - 1, 7, "console");
    byte_copy (file + sizeof (PREFIX) + 6, sizeof (NAME), NAME);
    r = openreadnclose (file, name, max);
    if (r <= 0)
        aa_strerr_diefu2sys (2, "read ", file);

    for (;;)
    {
        byte_copy (file + sizeof (PREFIX) - 1, r, name);
        byte_copy (file + sizeof (PREFIX) - 2 + r, sizeof (NAME), NAME);
        r = openreadnclose (file, name, max);
        if (r <= 0)
        {
            if (errno == ENOENT)
            {
                aa_bs_noflush (AA_OUT, "/dev/");
                aa_bs_flush (AA_OUT, name);
                return 0;
            }
            else
                aa_strerr_diefu2sys (2, "read ", file);
        }
    }
}
