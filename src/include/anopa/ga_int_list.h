
#ifndef AA_GA_INT_LIST_H
#define AA_GA_INT_LIST_H

#include <skalibs/genalloc.h>

#define ga_remove(type, ga, i)     do {         \
    int len = (ga)->len / sizeof (type);        \
    int c = len - (i) - 1;                      \
    if (c > 0)                                  \
        memmove (genalloc_s (type, (ga)) + (i), genalloc_s (type, (ga)) + (i) + 1, c * sizeof (type)); \
    genalloc_setlen (type, (ga), len - 1);    \
} while (0)

#define list_get(ga, i)         (genalloc_s (int, ga)[i])

extern int add_to_list      (genalloc *list, int si, int check_for_dupes);
extern int remove_from_list (genalloc *list, int si);
extern int is_in_list       (genalloc *list, int si);

#endif /* AA_GA_INT_LIST_H */
