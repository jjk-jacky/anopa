/*
 * anopa - Copyright (C) 2015-2016 Olivier Brunel
 *
 * ga_list.h
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

#ifndef AA_GA_LIST_H
#define AA_GA_LIST_H

#include <sys/types.h>
#include <skalibs/genalloc.h>

#define ga_get(type, ga, i)             (genalloc_s (type, ga)[i])

void ga_remove      (genalloc *ga, size_t size, int i);
int  ga_find        (genalloc *ga, size_t size, void const *val);
int  ga_add_val     (genalloc *ga, size_t size, void const *val, int check_for_dupes);
int  ga_remove_val  (genalloc *ga, size_t size, void const *val);

#endif /* AA_GA_LIST_H */
