
#ifndef AA_STATS_H
#define AA_STATS_H

#include <skalibs/genalloc.h>

void aa_show_stat_nb (int nb, const char *title, const char *ansi_color);
void aa_show_stat_names (const char  *names,
                         genalloc    *ga_offets,
                         const char  *title,
                         const char  *ansi_color);

#endif /* AA_STATS_H */
