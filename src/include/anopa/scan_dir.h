/*
 * anopa - Copyright (C) 2015-2016 Olivier Brunel
 *
 * scan_dir.h
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

#ifndef AA_SCAN_DIR_H
#define AA_SCAN_DIR_H

#include <skalibs/direntry.h>
#include <skalibs/stralloc.h>

typedef int (*aa_sd_it_fn) (direntry *d, void *data);

int aa_scan_dir (stralloc *sa, int files_only, aa_sd_it_fn iterator, void *data);

#endif /* AA_SCAN_DIR_H */
