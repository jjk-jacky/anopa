/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * ga_int_list.h
 * Copyright (C) 2015-2017 Olivier Brunel <jjk@jjacky.com>
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

#ifndef AA_GA_INT_LIST_H
#define AA_GA_INT_LIST_H

#include <anopa/ga_list.h>

#define list_get(ga, i)                 ga_get (int, ga, i)
#define add_to_list(ga, si, chk_dupes)  ga_add_val (ga, sizeof (int), (char const *) &si, chk_dupes)
#define remove_from_list(ga, si)        ga_remove_val (ga, sizeof (int), (char const *) &si)
#define is_in_list(ga, si)              (ga_find (ga, sizeof (int), (char const *) &si) >= 0)

#endif /* AA_GA_INT_LIST_H */
