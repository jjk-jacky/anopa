
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <s6/ftrigr.h>
#include <anopa/service.h>

genalloc aa_services    = GENALLOC_ZERO;
stralloc aa_names       = STRALLOC_ZERO;
genalloc aa_main_list   = GENALLOC_ZERO;
genalloc aa_tmp_list    = GENALLOC_ZERO;

ftrigr_t _aa_ft         = FTRIGR_ZERO;
aa_exec_cb _exec_cb     = NULL;
