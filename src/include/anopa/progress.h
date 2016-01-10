/*
 * anopa - Copyright (C) 2015-2016 Olivier Brunel
 *
 * progress.h
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

#ifndef AA_PROGRESS_H
#define AA_PROGRESS_H

#include <skalibs/stralloc.h>

typedef struct
{
    int step;
    double pctg;
    stralloc sa;
} aa_progress;

extern void aa_progress_free    (aa_progress *p);
extern int  aa_progress_update  (aa_progress *pg);
extern void aa_progress_draw    (aa_progress *pg, const char *title, int cols, int is_utf8);

#endif /* AA_PROGRESS_H */
