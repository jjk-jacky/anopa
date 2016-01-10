/*
 * anopa - Copyright (C) 2015-2016 Olivier Brunel
 *
 * stats.c
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

#include <skalibs/uint.h>
#include <skalibs/buffer.h>
#include <skalibs/genalloc.h>
#include <anopa/output.h>
#include <anopa/ga_int_list.h>

void
aa_show_stat_nb (int nb, const char *title, const char *ansi_color)
{
    char buf[UINT_FMT];

    if (nb <= 0)
        return;

    aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_BLUE_ON);
    aa_bs_noflush (AA_OUT, "  -> ");
    aa_is_noflush (AA_OUT, ansi_color);
    aa_bs_noflush (AA_OUT, title);
    aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_ON);
    aa_bs_noflush (AA_OUT, ": ");
    buf[uint_fmt (buf, nb)] = '\0';
    aa_bs_noflush (AA_OUT, buf);
    aa_end_title ();
}

void
aa_show_stat_names (const char  *names,
                    genalloc    *ga_offets,
                    const char  *title,
                    const char  *ansi_color)
{
    int i;

    if (genalloc_len (int, ga_offets) <= 0)
        return;

    aa_put_title (0, title, "", 0);
    for (i = 0; i < genalloc_len (int, ga_offets); ++i)
    {
        if (i > 0)
            aa_bs_noflush (AA_OUT, "; ");
        aa_is_noflush (AA_OUT, ansi_color);
        aa_bs_noflush (AA_OUT, names + list_get (ga_offets, i));
        aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_ON);
    }
    aa_end_title ();
}
