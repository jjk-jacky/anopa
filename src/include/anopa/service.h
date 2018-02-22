/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * service.h
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

#ifndef AA_SERVICE_H
#define AA_SERVICE_H

#include <stdint.h>
#include <sys/types.h>
#include <signal.h> /* pid_t */
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <skalibs/tai.h>
#include <anopa/service_status.h>

#define AA_START_FILENAME           "start"
#define AA_STOP_FILENAME            "stop"
#define AA_GETS_READY_FILENAME      "gets-ready"

extern genalloc aa_services;
extern stralloc aa_names;
extern genalloc aa_main_list;
extern genalloc aa_tmp_list;
extern genalloc aa_pid_list;
extern unsigned int aa_secs_timeout;

#define aa_service(i)               (&((aa_service *) aa_services.s)[i])
#define aa_service_name(service)    (aa_names.s + (service)->offset_name)

typedef enum
{
    AA_SERVICE_FROM_MAIN = 0,
    AA_SERVICE_FROM_TMP,
} aa_sf;

typedef enum
{
    AA_MODE_START       = (1 << 0),
    AA_MODE_STOP        = (1 << 1),
    AA_MODE_STOP_ALL    = (1 << 2),
    AA_MODE_IS_DRY      = (1 << 3),
    AA_MODE_IS_DRY_FULL = (1 << 4)
} aa_mode;

typedef enum
{
    AA_AUTOLOAD_NEEDS = 0,
    AA_AUTOLOAD_WANTS,
} aa_al;

typedef enum
{
    AA_LOAD_NOT,
    AA_LOAD_ING,
    AA_LOAD_DONE,
    AA_LOAD_DONE_CHECKED,
    AA_LOAD_FAIL
} aa_ls;

typedef struct
{
    size_t offset_name;
    int nb_mark;
    genalloc needs;
    genalloc wants;
    genalloc after;
    unsigned int secs_timeout;
    aa_ls ls;
    aa_service_status st;
    tain_t ts_exec;
    uint16_t retries;
    /* longrun */
    uint16_t ft_id;
    int gets_ready;
    /* oneshot */
    int fd_in;
    int fd_out;
    stralloc sa_out;
    int fd_progress;
    int pi;
    int timedout;
} aa_service;

typedef void (*aa_close_fd_fn) (int fd);
typedef void (*aa_autoload_cb) (int si, aa_al al, const char *name, int err);
typedef void (*aa_prepare_cb) (int si, int si_next, int is_needs, size_t first);
typedef void (*aa_scan_cb) (int si, int sni);
typedef void (*aa_exec_cb) (int si, aa_evt evt, pid_t pid);

extern void     aa_free_services (aa_close_fd_fn close_fd_fn);
extern size_t   aa_add_name (const char *name);
extern int      aa_get_service (const char *name, int *si, int new_in_main);
extern void     aa_unmark_service (int si);
extern int      aa_mark_service (aa_mode mode, int si, int in_main, int no_wants, aa_autoload_cb al_cb);
extern int      aa_preload_service (int si);
extern int      aa_ensure_service_loaded (int si, aa_mode mode, int no_wants, aa_autoload_cb al_cb);
extern int      aa_prepare_mainlist (aa_prepare_cb prepare_cb, aa_exec_cb exec_cb);
extern void     aa_scan_mainlist (aa_scan_cb scan_cb, aa_mode mode);
extern int      aa_exec_service (int si, aa_mode mode);
extern int      aa_get_longrun_info (uint16_t *id, char *event);
extern int      aa_unsubscribe_for (uint16_t id);

#endif /* AA_SERVICE_H */
