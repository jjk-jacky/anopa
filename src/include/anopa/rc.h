/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * rc.h
 * Copyright (C) 2018 Olivier Brunel <jjk@jjacky.com>
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

#ifndef AA_RC_H
#define AA_RC_H

/* we have 127 values for fatal errors, common to all binaries, listed below.
 * They'll be returned moved 1 bit to the right with the first bit always set,
 * indicating a fatal error.
 * Each binary can then have 127 values to report different "statuses". Those
 * will be returned moved 1 bit to the right, first bit not set. They can be
 * binary-specific enums, or bitwise combinations.
 */

enum
{
    RC_OK = 0,

    RC_FATAL_USAGE      = (0 << 1 | 1),
    RC_FATAL_INIT_REPO  = (1 << 1 | 1),     /* only by "commands" */
    RC_FATAL_IO         = (2 << 1 | 1),
    RC_FATAL_MEMORY     = (3 << 1 | 1),
    RC_FATAL_ALARM_S6   = (4 << 1 | 1),     /* only by aa-enable */

    RC_FATAL_EXEC       = (55 << 1 | 1),    /* so rc=111; execline style */

    RC_FATAL_INTERNAL   = (127 << 1 | 1)    /* shouldn't happen: broken state, etc */
};

#endif /* AA_RC_H */
