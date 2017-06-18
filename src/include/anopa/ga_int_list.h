/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * ga_int_list.h
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

#ifndef AA_GA_INT_LIST_H
#define AA_GA_INT_LIST_H

#include <skalibs/genalloc.h>

#define ga_remove(type, ga, i)     do {         \
    int len = (ga)->len / sizeof (type);        \
    int c = len - (i) - 1;                      \
    if (c > 0)                                  \
        memmove (genalloc_s (type, (ga)) + (i), genalloc_s (type, (ga)) + (i) + 1, c * sizeof (type)); \
    genalloc_setlen (type, (ga), len - 1);    \
} while (0)

#define list_get(ga, i)         (genalloc_s (int, ga)[i])

extern int add_to_list      (genalloc *list, int si, int check_for_dupes);
extern int remove_from_list (genalloc *list, int si);
extern int is_in_list       (genalloc *list, int si);

#endif /* AA_GA_INT_LIST_H */
