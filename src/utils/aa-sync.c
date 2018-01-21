/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * aa-sync.c
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

#include <unistd.h>
#include <skalibs/bytestr.h>
#include <anopa/common.h>

const char *PROG;

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION]",
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-sync";

    if (argc == 1)
    {
        sync ();
        return RC_OK;
    }

    if (argc == 2 && (str_equal (argv[1], "-V") || str_equal (argv[1], "--version")))
        aa_die_version ();
    dieusage ((argc == 2 && (str_equal (argv[1], "-h") || str_equal (argv[1], "--help"))) ? RC_OK : RC_FATAL_USAGE);
}
