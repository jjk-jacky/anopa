/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * die_version.c
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

#include "anopa/config.h"

#include <unistd.h>
#include <anopa/output.h>

extern char const *PROG;

void
aa_die_version (void)
{
    aa_init_output (0);
    aa_bs_noflush (AA_OUT, PROG);
    aa_bs_noflush (AA_OUT, " v" ANOPA_VERSION "\n");
    aa_bs_flush (AA_OUT,
            "Copyright (C) 2015 Olivier Brunel - http://jjacky.com/anopa\n"
            "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
            "This is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n"
            );
    _exit (0);
}
