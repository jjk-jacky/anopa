/*
 * anopa - Copyright (C) 2015-2016 Olivier Brunel
 *
 * die_usage.c
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
#include <anopa/output.h>

extern char const *PROG;

void
aa_die_usage (int rc, const char *usage, const char *details)
{
    aa_bs_noflush (AA_OUT, "Usage: ");
    aa_bs_noflush (AA_OUT, PROG);
    aa_bs_noflush (AA_OUT, " ");
    aa_bs_noflush (AA_OUT, usage);
    aa_bs_noflush (AA_OUT, "\n\n");
    aa_bs_flush (AA_OUT, details);
    _exit (rc);
}
