/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * common.h
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

#ifndef AA_COMMON_H
#define AA_COMMON_H

#define AA_SCANDIR_DIRNAME      ".scandir"

void aa_die_usage (int rc, const char *usage, const char *details);
void aa_die_version (void);

#endif /* AA_COMMON_H */
