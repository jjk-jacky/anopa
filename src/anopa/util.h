/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * util.h
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

#ifndef AA_UTIL_H
#define AA_UTIL_H

typedef void (*names_cb) (const char *name, void *data);

int process_names_from_stdin (names_cb process_name, void *data);
void unslash (char *s);

#endif /* AA_UTIL_H */
