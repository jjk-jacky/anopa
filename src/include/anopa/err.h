/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * err.h
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

#ifndef AA_ERR_H
#define AA_ERR_H

enum
{
    ERR_INVALID_NAME = 1,
    ERR_UNKNOWN,
    ERR_DEPEND,
    ERR_IO,
    ERR_WRITE_STATUS,
    ERR_CHDIR,
    ERR_EXEC,
    ERR_PIPES,
    ERR_S6,
    ERR_FAILED,
    ERR_TIMEDOUT,
    ERR_IO_REPODIR,
    ERR_IO_SCANDIR,
    ERR_FAILED_ENABLE,
    /* not actual service error, see aa_ensure_service_loaded() */
    ERR_ALREADY_UP,
    ERR_NOT_UP,
    _NB_ERR
};

extern const char const *errmsg[_NB_ERR];

#endif /* AA_ERR_H */
