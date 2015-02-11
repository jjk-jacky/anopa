
#ifndef AA_SERVICE_INTERNAL_H
#define AA_SERVICE_INTERNAL_H

#include <skalibs/direntry.h>
#include <anopa/service.h>

extern ftrigr_t _aa_ft;
extern aa_exec_cb _exec_cb;

struct it_data
{
    int si;
    int no_wants;
    aa_load_fail_cb lf_cb;
};

extern int _it_start_needs  (direntry *d, void *data);
extern int _it_start_wants  (direntry *d, void *data);
extern int _it_start_after  (direntry *d, void *data);
extern int _it_start_before (direntry *d, void *data);

extern int _it_stop_needs   (direntry *d, void *data);
extern int _it_stop_after   (direntry *d, void *data);
extern int _it_stop_before  (direntry *d, void *data);

extern int _exec_oneshot (int si, aa_mode mode);
extern int _exec_longrun (int si, aa_mode mode);

#endif /* AA_SERVICE_INTERNAL_H */
