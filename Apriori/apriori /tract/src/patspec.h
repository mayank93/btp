/*----------------------------------------------------------------------
  File    : patspec.h
  Contents: pattern spectrum/statistics management
  Author  : Christian Borgelt
  History : 2012.10.12 file created (as patstat.h)
            2013.09.28 adapted to definitions of ITEM and SUPP
            2013.09.30 sums per size and total sum of frequencies added
            2013.10.15 functions psp_error() and psp_clear() added
----------------------------------------------------------------------*/
#ifndef __PATSPEC__
#define __PATSPEC__
#include "tract.h"
#ifdef PSP_REPORT
#include "tabwrite.h"
#endif

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
typedef struct {                /* --- pattern spectrum row -- */
  SUPP   min, max;              /* minimum and maximum support */
  size_t sum;                   /* sum of occurrences (for this size) */
  size_t *frqs;                 /* occurrence counters (by support) */
} PSPROW;                       /* (pattern spectrum row) */

typedef struct {                /* --- pattern spectrum --- */
  ITEM   minsize;               /* minimum pattern size (offset) */
  ITEM   maxsize;               /* maximum pattern size (limit) */
  SUPP   minsupp;               /* minimum support (offset) */
  SUPP   maxsupp;               /* maximum support (limit) */
  size_t sigcnt;                /* number of registered signatures */
  size_t total;                 /* total frequency of signatures */
  ITEM   max;                   /* number of pattern spectrum rows */
  int    err;                   /* error status */
  PSPROW *rows;                 /* pattern spectrum rows (by size) */
} PATSPEC;                      /* (pattern spectrum) */

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/
extern PATSPEC* psp_create  (ITEM minsize, ITEM maxsize,
                             SUPP minsupp, SUPP maxsupp);
extern void     psp_delete  (PATSPEC *psp);
extern void     psp_clear   (PATSPEC *psp);
extern ITEM     psp_minsize (PATSPEC *psp);
extern ITEM     psp_maxsize (PATSPEC *psp);
extern ITEM     psp_min     (PATSPEC *psp);
extern ITEM     psp_max     (PATSPEC *psp);
extern SUPP     psp_minsupp (PATSPEC *psp);
extern SUPP     psp_maxsupp (PATSPEC *psp);
extern SUPP     psp_min4sz  (PATSPEC *psp, ITEM size);
extern SUPP     psp_max4sz  (PATSPEC *psp, ITEM size);
extern int      psp_error   (PATSPEC *psp);
extern size_t   psp_sigcnt  (PATSPEC *psp);
extern size_t   psp_total   (PATSPEC *psp);
extern size_t   psp_getfrq  (PATSPEC *psp, ITEM size, SUPP supp);
extern int      psp_setfrq  (PATSPEC *psp, ITEM size, SUPP supp,
                             size_t frq);
extern int      psp_incfrq  (PATSPEC *psp, ITEM size, SUPP supp,
                             size_t frq);
extern int      psp_addpsp  (PATSPEC *dst, PATSPEC *src);
#ifdef PSP_REPORT
extern int      psp_report  (PATSPEC *psp, TABWRITE *twr);
#endif
#ifndef NDEBUG
extern void     psp_show    (PATSPEC *psp);
#endif

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define psp_minsize(p)      ((p)->minsize)
#define psp_maxsize(p)      ((p)->maxsize)
#define psp_min(p)          ((p)->minsize)
#define psp_max(p)          ((p)->max)
#define psp_minsupp(p)      ((p)->minsupp)
#define psp_maxsupp(p)      ((p)->maxsupp)
#define psp_min4sz(p,s)     ((p)->rows[s].min)
#define psp_max4sz(p,s)     ((p)->rows[s].max)
#define psp_sum4sz(p,s)     ((p)->rows[s].sum)
#define psp_error(p)        ((p)->err)
#define psp_sigcnt(p)       ((p)->sigcnt)
#define psp_total(p)        ((p)->total)

#endif
