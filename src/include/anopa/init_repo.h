
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
