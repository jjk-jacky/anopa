
#ifndef AA_PROGRESS_H
#define AA_PROGRESS_H

#include <skalibs/stralloc.h>

typedef struct
{
    int step;
    double pctg;
    stralloc sa;
} aa_progress;

extern void aa_progress_free    (aa_progress *p);
extern int  aa_progress_update  (aa_progress *pg);
extern void aa_progress_draw    (aa_progress *pg, const char *title, int cols, int is_utf8);

#endif /* AA_PROGRESS_H */
