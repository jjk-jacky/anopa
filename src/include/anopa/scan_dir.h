
#ifndef AA_SCAN_DIR_H
#define AA_SCAN_DIR_H

#include <skalibs/direntry.h>
#include <skalibs/stralloc.h>

typedef int (*aa_sd_it_fn) (direntry *d, void *data);

int aa_scan_dir (stralloc *sa, int files_only, aa_sd_it_fn iterator, void *data);

#endif /* AA_SCAN_DIR_H */
