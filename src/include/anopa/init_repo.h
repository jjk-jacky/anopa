/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * init_repo.h
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

#ifndef AA_INIT_REPO_H
#define AA_INIT_REPO_H

typedef enum
{
    AA_REPO_READ = 0,
    AA_REPO_WRITE,
    AA_REPO_CREATE
} aa_repo_init;

int aa_init_repo (const char *path_repo, aa_repo_init ri);

#endif /* AA_INIT_REPO_H */
