/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * output.h
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

#ifndef AA_OUTPUT_H
#define AA_OUTPUT_H

#include <string.h> /* strlen() */

#define ANSI_HIGHLIGHT_ON           "\x1B[1;39m"
#define ANSI_HIGHLIGHT_RED_ON       "\x1B[1;31m"
#define ANSI_HIGHLIGHT_GREEN_ON     "\x1B[1;32m"
#define ANSI_HIGHLIGHT_YELLOW_ON    "\x1B[1;33m"
#define ANSI_HIGHLIGHT_BLUE_ON      "\x1B[1;36m"
#define ANSI_HIGHLIGHT_OFF          "\x1B[0m"

#define AA_OUT      0
#define AA_ERR      1

extern void aa_init_output (int mode_both);
extern void aa_bb_noflush (int where, const char *s, int len);
extern void aa_bb_flush (int where, const char *s, int len);
#define aa_bs_noflush(w,s)  aa_bb_noflush ((w), (s), strlen (s))
#define aa_bs_flush(w,s)    aa_bb_flush ((w), (s), strlen (s))
extern void aa_ib_noflush (int where, const char *s, int len);
extern void aa_ib_flush (int where, const char *s, int len);
#define aa_is_noflush(w,s)  aa_ib_noflush ((w), (s), strlen (s))
#define aa_is_flush(w,s)    aa_ib_flush ((w), (s), strlen (s))
extern void aa_bs_end (int where);
#define aa_end_err()        aa_bs_end (AA_ERR)
extern void aa_put_err (const char *name, const char *msg, int end);
#define aa_end_warn()       aa_bs_end (AA_ERR)
extern void aa_put_warn (const char *name, const char *msg, int end);
#define aa_end_title()      aa_bs_end (AA_OUT)
extern void aa_put_title (int main, const char *name, const char *title, int end);

#endif /* AA_OUTPUT_H */
