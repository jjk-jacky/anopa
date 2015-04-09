/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * service_status.h
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

#ifndef AA_SERVICE_STATUS_H
#define AA_SERVICE_STATUS_H

#include <skalibs/stralloc.h>
#include <skalibs/tai.h>

typedef enum
{
    AA_EVT_NONE = 0,
    AA_EVT_ERROR,
    AA_EVT_STARTING,
    AA_EVT_STARTING_FAILED,
    AA_EVT_START_FAILED,
    AA_EVT_STARTED,
    AA_EVT_STOPPING,
    AA_EVT_STOPPING_FAILED,
    AA_EVT_STOP_FAILED,
    AA_EVT_STOPPED,
    _AA_NB_EVT
} aa_evt;

extern const char const *eventmsg[_AA_NB_EVT];

enum
{
    AA_TYPE_UNKNOWN = 0,
    AA_TYPE_ONESHOT,
    AA_TYPE_LONGRUN
};

typedef struct
{
    tain_t stamp;
    aa_evt event;
    int code;
    stralloc sa;
    /* not saved to status file */
    unsigned int type;
} aa_service_status;

#define AA_SVST_FIXED_SIZE          20
#define AA_SVST_MAX_MSG_SIZE        255
#define AA_SVST_FILENAME            "status.anopa"

extern void aa_service_status_free      (aa_service_status *svst);
extern int  aa_service_status_read      (aa_service_status *svst, const char *dir);
extern int  aa_service_status_write     (aa_service_status *svst, const char *dir);
extern int  aa_service_status_set_msg   (aa_service_status *svst, const char *msg);
extern int  aa_service_status_set_err   (aa_service_status *svst, int err, const char *msg);
#define aa_service_status_get_msg(svst) \
    (((svst)->sa.len > AA_SVST_FIXED_SIZE) ? (svst)->sa.s + AA_SVST_FIXED_SIZE : NULL)

#endif /* AA_SERVICE_STATUS_H */
