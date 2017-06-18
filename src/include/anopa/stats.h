/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * stats.h
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

#ifndef AA_STATS_H
#define AA_STATS_H

#include <skalibs/genalloc.h>

void aa_show_stat_nb (int nb, const char *title, const char *ansi_color);
void aa_show_stat_names (const char  *names,
                         genalloc    *ga_offets,
                         const char  *title,
                         const char  *ansi_color);

#endif /* AA_STATS_H */
