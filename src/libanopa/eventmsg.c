/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * eventmsg.c
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

#include <anopa/service_status.h>

const char const *eventmsg[_AA_NB_EVT] = {
    "Unknown status",
    "Error",
    "Starting",
    "Starting failed",
    "Start failed",
    "Started",
    "Stopping",
    "Stopping failed",
    "Stop failed",
    "Stopped"
};
