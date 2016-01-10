/*
 * anopa - Copyright (C) 2015-2016 Olivier Brunel
 *
 * errmsg.c
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

#include <anopa/err.h>

const char const *errmsg[_NB_ERR] = {
    "",
    "Invalid name",
    "Unknown service",
    "Failed dependency",
    "I/O error",
    "Uable to write service status file",
    "Unable to get into service directory",
    "Unable to exec",
    "Unable to setup pipes",
    "Failed to communicate with s6",
    "Failed",
    "Timed out",
    "Failed to create repository directory",
    "Failed to create scandir directory",
    "Failed to enable/create servicedir",

    "Already up",
    "Not up"
};
