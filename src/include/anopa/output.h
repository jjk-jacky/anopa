/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * output.h
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

#ifndef AA_OUTPUT_H
#define AA_OUTPUT_H

#include <string.h>
#include <errno.h>

extern const char *PROG;

#define ANSI_HIGHLIGHT_ON           "\x1B[1;39m"
#define ANSI_HIGHLIGHT_RED_ON       "\x1B[1;31m"
#define ANSI_HIGHLIGHT_GREEN_ON     "\x1B[1;32m"
#define ANSI_HIGHLIGHT_YELLOW_ON    "\x1B[1;33m"
#define ANSI_HIGHLIGHT_BLUE_ON      "\x1B[1;36m"
#define ANSI_HIGHLIGHT_OFF          "\x1B[0m"

#define AA_OUT      0
#define AA_ERR      1

extern void aa_set_double_output (int enabled);
extern void aa_bb_noflush (int where, const char *s, size_t len);
extern void aa_bb_flush (int where, const char *s, size_t len);
#define aa_bs_noflush(w,s)  aa_bb_noflush ((w), (s), strlen (s))
#define aa_bs_flush(w,s)    aa_bb_flush ((w), (s), strlen (s))
extern void aa_ib_noflush (int where, const char *s, size_t len);
extern void aa_ib_flush (int where, const char *s, size_t len);
#define aa_is_noflush(w,s)  aa_ib_noflush ((w), (s), strlen (s))
#define aa_is_flush(w,s)    aa_ib_flush ((w), (s), strlen (s))
extern void aa_bs_end (int where);
#define aa_end_err()        aa_bs_end (AA_ERR)
extern void aa_put_err (const char *name, const char *msg, int end);
#define aa_end_warn()       aa_bs_end (AA_ERR)
extern void aa_put_warn (const char *name, const char *msg, int end);
#define aa_end_title()      aa_bs_end (AA_OUT)
extern void aa_put_title (int main, const char *name, const char *title, int end);

extern void aa_strerr_warn (const char *s1,
                            const char *s2,
                            const char *s3,
                            const char *s4,
                            const char *s5,
                            const char *s6,
                            const char *s7,
                            const char *s8,
                            const char *s9,
                            const char *s10);

#define aa_strerr_warn1x(s1) \
    aa_strerr_warn ("warning: ", s1, 0, 0, 0, 0, 0, 0, 0, 0)
#define aa_strerr_warn2x(s1,s2) \
    aa_strerr_warn ("warning: ", s1, s2, 0, 0, 0, 0, 0, 0, 0)
#define aa_strerr_warn3x(s1,s2,s3) \
    aa_strerr_warn ("warning: ", s1, s2, s3, 0, 0, 0, 0, 0, 0)
#define aa_strerr_warn4x(s1,s2,s3,s4) \
    aa_strerr_warn ("warning: ", s1, s2, s3, s4, 0, 0, 0, 0, 0)
#define aa_strerr_warn5x(s1,s2,s3,s4,s5) \
    aa_strerr_warn ("warning: ", s1, s2, s3, s4, s5, 0, 0, 0, 0)
#define aa_strerr_warn6x(s1,s2,s3,s4,s5,s6) \
    aa_strerr_warn ("warning: ", s1, s2, s3, s4, s5, s6, 0, 0, 0)

#define aa_strerr_warnu1x(s1) \
    aa_strerr_warn ("warning: ", "unable to ", s1, 0, 0, 0, 0, 0, 0, 0)
#define aa_strerr_warnu2x(s1,s2) \
    aa_strerr_warn ("warning: ", "unable to ", s1, s2, 0, 0, 0, 0, 0, 0)
#define aa_strerr_warnu3x(s1,s2,s3) \
    aa_strerr_warn ("warning: ", "unable to ", s1, s2, s3, 0, 0, 0, 0, 0)
#define aa_strerr_warnu4x(s1,s2,s3,s4) \
    aa_strerr_warn ("warning: ", "unable to ", s1, s2, s3, s4, 0, 0, 0, 0)
#define aa_strerr_warnu5x(s1,s2,s3,s4,s5) \
    aa_strerr_warn ("warning: ", "unable to ", s1, s2, s3, s4, s5, 0, 0, 0)
#define aa_strerr_warnu6x(s1,s2,s3,s4,s5,s6) \
    aa_strerr_warn ("warning: ", "unable to ", s1, s2, s3, s4, s5, s6, 0, 0)

#define aa_strerr_warnu1sys(s1) \
    aa_strerr_warn ("warning: ", "unable to ", s1, ": ", strerror (errno), 0, 0, 0, 0, 0)
#define aa_strerr_warnu2sys(s1,s2) \
    aa_strerr_warn ("warning: ", "unable to ", s1, s2, ": ", strerror (errno), 0, 0, 0, 0)
#define aa_strerr_warnu3sys(s1,s2,s3) \
    aa_strerr_warn ("warning: ", "unable to ", s1, s2, s3, ": ", strerror (errno), 0, 0, 0)
#define aa_strerr_warnu4sys(s1,s2,s3,s4) \
    aa_strerr_warn ("warning: ", "unable to ", s1, s2, s3, s4, ": ", strerror (errno), 0, 0)
#define aa_strerr_warnu5sys(s1,s2,s3,s4,s5) \
    aa_strerr_warn ("warning: ", "unable to ", s1, s2, s3, s4, s5, ": ", strerror (errno), 0)
#define aa_strerr_warnu6sys(s1,s2,s3,s4,s5,s6) \
    aa_strerr_warn ("warning: ", "unable to ", s1, s2, s3, s4, s5, s6, ": ", strerror (errno))

extern void aa_strerr_die (int rc,
                           const char *s1,
                           const char *s2,
                           const char *s3,
                           const char *s4,
                           const char *s5,
                           const char *s6,
                           const char *s7,
                           const char *s8,
                           const char *s9);

#define aa_strerr_diefu1sys(rc,s1) \
    aa_strerr_die (rc, "unable to ", s1, ": ", strerror (errno), 0, 0, 0, 0, 0)
#define aa_strerr_diefu2sys(rc,s1,s2) \
    aa_strerr_die (rc, "unable to ", s1, s2, ": ", strerror (errno), 0, 0, 0, 0)
#define aa_strerr_diefu3sys(rc,s1,s2,s3) \
    aa_strerr_die (rc, "unable to ", s1, s2, s3, ": ", strerror (errno), 0, 0, 0)
#define aa_strerr_diefu4sys(rc,s1,s2,s3,s4) \
    aa_strerr_die (rc, "unable to ", s1, s2, s3, s4, ": ", strerror (errno), 0, 0)
#define aa_strerr_diefu5sys(rc,s1,s2,s3,s4,s5) \
    aa_strerr_die (rc, "unable to ", s1, s2, s3, s4, s5, ": ", strerror (errno), 0)
#define aa_strerr_diefu6sys(rc,s1,s2,s3,s4,s5,s6) \
    aa_strerr_die (rc, "unable to ", s1, s2, s3, s4, s5, s6, ": ", strerror (errno))

#define aa_strerr_dief1x(rc,s1) \
    aa_strerr_die (rc, s1, 0, 0, 0, 0, 0, 0, 0, 0)
#define aa_strerr_dief2x(rc,s1,s2) \
    aa_strerr_die (rc, s1, s2, 0, 0, 0, 0, 0, 0, 0)
#define aa_strerr_dief3x(rc,s1,s2,s3) \
    aa_strerr_die (rc, s1, s2, s3, 0, 0, 0, 0, 0, 0)
#define aa_strerr_dief4x(rc,s1,s2,s3,s4) \
    aa_strerr_die (rc, s1, s2, s3, s4, 0, 0, 0, 0, 0)
#define aa_strerr_dief5x(rc,s1,s2,s3,s4,s5) \
    aa_strerr_die (rc, s1, s2, s3, s4, s5, 0, 0, 0, 0)
#define aa_strerr_dief6x(rc,s1,s2,s3,s4,s5,s6) \
    aa_strerr_die (rc, s1, s2, s3, s4, s5, s6, 0, 0, 0)

#define aa_strerr_diefu1x(rc,s1) \
    aa_strerr_die (rc, "unable to ", s1, 0, 0, 0, 0, 0, 0, 0)
#define aa_strerr_diefu2x(rc,s1,s2) \
    aa_strerr_die (rc, "unable to ", s1, s2, 0, 0, 0, 0, 0, 0)
#define aa_strerr_diefu3x(rc,s1,s2,s3) \
    aa_strerr_die (rc, "unable to ", s1, s2, s3, 0, 0, 0, 0, 0)
#define aa_strerr_diefu4x(rc,s1,s2,s3,s4) \
    aa_strerr_die (rc, "unable to ", s1, s2, s3, s4, 0, 0, 0, 0)
#define aa_strerr_diefu5x(rc,s1,s2,s3,s4,s5) \
    aa_strerr_die (rc, "unable to ", s1, s2, s3, s4, s5, 0, 0, 0)
#define aa_strerr_diefu6x(rc,s1,s2,s3,s4,s5,s6) \
    aa_strerr_die (rc, "unable to ", s1, s2, s3, s4, s5, s6, 0, 0)

#define aa_strerr_dieexec(rc,s) \
    aa_strerr_diefu2sys (rc, "exec ", s)

#endif /* AA_OUTPUT_H */
