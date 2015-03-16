
#ifndef AA_COPY_FILE_H
#define AA_COPY_FILE_H

typedef enum
{
    AA_CP_CREATE = 0,
    AA_CP_OVERWRITE,
    AA_CP_APPEND,
    _AA_CP_NB
} aa_cp;

int aa_copy_file (const char *src, const char *dst, mode_t mode, aa_cp cp);

#endif /* AA_COPY_FILE_H */
