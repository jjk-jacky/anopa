/*
 * anopa - Copyright (C) 2015-2016 Olivier Brunel
 *
 * copy_file.h
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

#ifndef AA_COPY_FILE_H
#define AA_COPY_FILE_H

typedef enum
{
    AA_CP_CREATE = 0,
    AA_CP_OVERWRITE,
    AA_CP_APPEND,
    _AA_CP_NB
} aa_cp;

int aa_copy_file (const char *src, const char *dst, mode_t mode, aa_cp cp);

#endif /* AA_COPY_FILE_H */
