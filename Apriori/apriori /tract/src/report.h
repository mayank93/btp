/*----------------------------------------------------------------------
  File    : report.h
  Contents: item set reporter management
  Author  : Christian Borgelt
  History : 2008.08.18 item set reporter created in tract.[ch]
            2008.08.30 handling of perfect extensions completed
            2008.09.01 handling of closed and maximal item sets added
            2008.10.15 function isr_xable() added (reporter extendable)
            2008.10.30 transaction identifier reporting added
            2008.10.31 item set reporter made a separate module
            2009.10.15 counter for reported item sets added
            2009.10.16 functions isr_wgt() and isr_wgtx() added
            2010.04.07 extended information reporting functions removed
            2010.07.02 function isr_all() added for easier access
            2010.07.22 adapted to closed/maximal item set filter
            2010.08.06 function isr_direct() for direct reporting added
            2010.08.11 function isr_directx() for extended items added
            2010.08.14 item set header for output added to isr_create()
            2010.10.15 functions isr_open(), isr_close(), isr_rule()
            2011.05.06 generalized to support type RSUPP (int/double)
            2011.06.10 function isr_wgtsupp() added (weight/support)
            2011.07.23 parameter dir added to function isr_seteval()
            2011.08.12 definition of ISR_GENERA added (for generators)
            2011.08.17 structure ISREPORT reorganized (fields moved)
            2011.09.20 flag ISR_NOFILTER added (no internal filtering)
            2011.10.05 type of item set counters changed to long int
            2012.04.30 function isr_setsent() added (item set sentinel)
            2012.05.30 function isr_addpexpk() added (packed items)
            2012.10.15 minimum and maximum support added
            2013.03.18 function isr_check() added (check for a superset)
            2013.10.08 function isr_seqrule() added (head at end)
            2013.10.15 result of isr_direct[x]() and isr_[seq]rule()
----------------------------------------------------------------------*/
#ifndef __REPORT__
#define __REPORT__
#include <limits.h>
#include <math.h>
#ifdef ISR_PATSPEC
#include "patspec.h"
#endif
#ifdef ISR_CLOMAX
#include "clomax.h"
#endif

#ifndef INFINITY
#define INFINITY    (DBL_MAX+DBL_MAX)
#endif                          /* MSC still does not support C99 */

/*--------------------------------------------------------------------*/

#ifndef RSUPP
#define RSUPP       SUPP        /* support type for reporting */
#define RSUPP_MAX   SUPP_MAX    /* maximum support value */
#define RSUPP_FMT   SUPP_FMT    /* printf format code for SUPP_T */

#else
#define int         1           /* to check definition of RSUPP_T */
#define long        2           /* for certain types */
#define ptrdiff_t   3
#define double      4

#if   RSUPP==int
#ifndef RSUPP_MAX
#define RSUPP_MAX   INT_MAX     /* maximum support value */
#endif
#ifndef RSUPP_FMT
#define RSUPP_FMT   "d"         /* printf format code for int */
#endif

#elif RSUPP==long
#ifndef RSUPP_MAX
#define RSUPP_MAX   LONG_MAX    /* maximum support value */
#endif
#ifndef RSUPP_FMT
#define RSUPP_FMT   "ld"        /* printf format code for long */
#endif

#elif RSUPP==ptrdiff_t
#ifndef RSUPP_MAX
#define RSUPP_MAX   PTRDIFF_MAX /* maximum support value */
#endif
#ifndef RSUPP_FMT
#  ifdef _MSC_VER
#  define RSUPP_FMT "Id"        /* printf format code for ptrdiff_t */
#  else
#  define RSUPP_FMT "td"        /* printf format code for ptrdiff_t */
#  endif                        /* MSC still does not support C99 */
#endif

#elif RSUPP==double
#ifndef RSUPP_MAX
#define RSUPP_MAX   INFINITY    /* maximum support value */
#endif
#ifndef RSUPP_FMT
#define RSUPP_FMT   "g"         /* printf format code for double */
#endif

#else
#error "RSUPP must be either 'int', 'long', 'ptrdiff_t' or 'double'"
#endif

#undef int                      /* remove preprocessor definitions */
#undef long                     /* needed for the type checking */
#undef ptrdiff_t
#undef double
#endif

/*--------------------------------------------------------------------*/

#ifndef CCHAR
#define CCHAR const char        /* abbreviation */
#endif

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
/* --- report modes --- */
#define ISR_SETS      0x0000    /* report all frequent item sets */
#define ISR_ALL       0x0000    /* report all frequent item sets */
#define ISR_CLOSED    0x0001    /* report only closed  item sets */
#define ISR_MAXIMAL   0x0002    /* report only maximal item sets */
#define ISR_GENERA    0x0004    /* report only generators */
#define ISR_RULE      0x0008    /* association rules */
#define ISR_TARGET    (ISR_CLOSED|ISR_MAXIMAL|ISR_GENERA|ISR_RULE)
#define ISR_NOEXPAND  0x0010    /* do not expand perfect extensions */
#define ISR_NOFILTER  0x0020    /* do not use internal filtering */
#define ISR_MAXONLY   0x0040    /* filter only with maximal sets */
#define ISR_SORT      0x0080    /* generator filtering needs sorting */
#define ISR_LOGS      0x0100    /* compute sums of logarithms */
#define ISR_WEIGHTS   0x0200    /* allow for item set weights */
#define ISR_SCAN      0x0400    /* report in scanable form */

/* --- delete modes --- */
#define ISR_DELISET   0x0001    /* delete the item set */
#define ISR_FCLOSE    0x0002    /* close the output file(s) */

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
struct isreport;                /* --- an item set eval. function --- */
typedef double ISEVALFN (struct isreport *rep, void *data);
typedef void   ISREPOFN (struct isreport *rep, void *data);

typedef struct isreport {       /* --- an item set reporter --- */
  ITEMBASE   *base;             /* underlying item base */
  FILE       *file;             /* output file to write to */
  const char *name;             /* name of item set output file */
  char       *buf;              /* write buffer for output */
  char       *next;             /* next character position to write */
  char       *end;              /* end of the write buffer */
  int        mode;              /* reporting mode (e.g. ISR_CLOSED) */
  ITEM       min;               /* minimum number of items in set */
  ITEM       max;               /* maximum number of items in set */
  ITEM       maxx;              /* maximum number for isr_xable() */
  RSUPP      smin;              /* minimum support */
  RSUPP      smax;              /* maximum support */
  ITEM       cnt;               /* current number of items in set */
  ITEM       pfx;               /* number of items in valid prefix */
  ITEM       *items;            /* current item set (array of items) */
  ITEM       *pexs;             /* perfect extension items */
  ITEM       *pxpp;             /* number of perfect exts. per prefix */
  size_t     rep;               /* number of reported item sets */
  size_t     *stats;            /* reported item sets per set size */
  #ifdef ISR_PATSPEC            /* if pattern spectrum support */
  PATSPEC    *psp;              /* an (optional) pattern spectrum */
  #else                         /* if no pattern spectrum support */
  void       *psp;              /* placeholder (for fixed offsets) */
  #endif
  RSUPP      sto;               /* max. superset support for storing */
  int        dir;               /* direction of item order in clomax */
  ITEM       *iset;             /* additional buffer for an item set */
  #ifdef ISR_CLOMAX             /* if closed/maximal sets filter */
  CLOMAX     *clomax;           /* closed/maximal item set filter */
  SYMTAB     *gentab;           /* generator      item set filter */
  #else                         /* if no closed/maximal sets filter */
  void       *clomax;           /* placeholder (for fixed offsets) */
  void       *gentab;           /* dito */
  #endif
  RSUPP      *supps;            /* (prefix) item sets support values */
  double     *wgts;             /* (prefix) item sets weights */
  double     logwgt;            /* logarithm of total trans. weight */
  double     *logs;             /* logarithms of item frequencies */
  double     *sums;             /* sums of logarithms for prefixes */
  ISEVALFN   *evalfn;           /* additional evaluation function */
  void       *evaldat;          /* additional evaluation data */
  int        evaldir;           /* direction of evaluation */
  double     evalthh;           /* threshold of evaluation */
  double     eval;              /* additional evaluation value */
  ISREPOFN   *repofn;           /* item set reporting function */
  void       *repodat;          /* item set reporting data */
  const char *hdr;              /* record header for output */
  const char *sep;              /* item separator for output */
  const char *imp;              /* implication sign for rule output */
  const char *iwfmt;            /* format for item weight output */
  const char *format;           /* format for information output */
  const char **inames;          /* (formatted) item names */
  int        fast;              /* whether fast output is possible */
  int        size;              /* size of set info. for fastout() */
  char       info[64];          /* item set info.    for fastout() */
  FILE       *tidfile;          /* output file for transaction ids */
  const char *tidname;          /* name of tid output file */
  ITEM       *occs;             /* array  of item occurrences */
  TID        *tids;             /* array  of transaction ids */
  TID        tidcnt;            /* number of transaction ids */
  TID        tracnt;            /* total number of transactions */
  ITEM       miscnt;            /* accepted number of missing items */
  char       *out;              /* output buffer for sets/rules */
  char       *pos[1];           /* append positions in output buffer */
} ISREPORT;                     /* (item set reporter) */

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/
extern ISREPORT*  isr_create   (ITEMBASE *base, int mode, int dir,
                                CCHAR *hdr, CCHAR *sep, CCHAR *imp);
extern int        isr_delete   (ISREPORT *rep, int mode);
extern ITEMBASE*  isr_base     (ISREPORT *rep);
extern int        isr_mode     (ISREPORT *rep);
extern int        isr_target   (ISREPORT *rep);
extern int        isr_open     (ISREPORT *rep, FILE *file, CCHAR *name);
extern int        isr_close    (ISREPORT *rep);
extern FILE*      isr_file     (ISREPORT *rep);
extern CCHAR*     isr_name     (ISREPORT *rep);

extern void       isr_reset    (ISREPORT *rep);
extern void       isr_setfmt   (ISREPORT *rep, const char *format);
extern void       isr_setiwf   (ISREPORT *rep, const char *format);
extern void       isr_setsize  (ISREPORT *rep, ITEM  min, ITEM  max);
extern void       isr_setsupp  (ISREPORT *rep, RSUPP min, RSUPP max);
extern void       isr_seteval  (ISREPORT *rep, ISEVALFN evalfn,
                                void *data, int dir, double thresh);
extern void       isr_setrepo  (ISREPORT *rep, ISREPOFN repofn,
                                void *data);
extern int        isr_tidopen  (ISREPORT *rep, FILE *file, CCHAR *name);
extern int        isr_tidclose (ISREPORT *rep);
extern void       isr_tidcfg   (ISREPORT *rep, TID tracnt, ITEM miscnt);
extern FILE*      isr_tidfile  (ISREPORT *rep);
extern CCHAR*     isr_tidname  (ISREPORT *rep);
extern void       isr_setsmt   (ISREPORT *rep, RSUPP supp);
extern void       isr_setwgt   (ISREPORT *rep, double wgt);
extern ITEM       isr_min      (ISREPORT *rep);
extern ITEM       isr_max      (ISREPORT *rep);

extern int        isr_add      (ISREPORT *rep, ITEM item, RSUPP supp);
extern int        isr_addnc    (ISREPORT *rep, ITEM item, RSUPP supp);
extern int        isr_addx     (ISREPORT *rep, ITEM item, RSUPP supp,
                                double wgt);
extern int        isr_addpex   (ISREPORT *rep, ITEM item);
extern void       isr_addpexpk (ISREPORT *rep, ITEM bits);
extern int        isr_uses     (ISREPORT *rep, ITEM item);
extern void       isr_remove   (ISREPORT *rep, ITEM n);
extern int        isr_xable    (ISREPORT *rep, ITEM n);

extern ITEM       isr_cnt      (ISREPORT *rep);
extern ITEM       isr_item     (ISREPORT *rep);
extern ITEM       isr_itemx    (ISREPORT *rep, ITEM index);
extern const ITEM*isr_items    (ISREPORT *rep);
extern void       isr_setsent  (ISREPORT *rep);
extern RSUPP      isr_supp     (ISREPORT *rep);
extern RSUPP      isr_suppx    (ISREPORT *rep, ITEM index);
extern double     isr_wgt      (ISREPORT *rep);
extern double     isr_wgtx     (ISREPORT *rep, ITEM index);
extern double     isr_logsum   (ISREPORT *rep);
extern double     isr_logsumx  (ISREPORT *rep, ITEM index);
extern double     isr_eval     (ISREPORT *rep);

extern ITEM       isr_pexcnt   (ISREPORT *rep);
extern ITEM       isr_pex      (ISREPORT *rep, ITEM index);
extern const int* isr_pexs     (ISREPORT *rep);
extern ITEM       isr_all      (ISREPORT *rep);
extern ITEM       isr_lack     (ISREPORT *rep);

extern CCHAR*     isr_itemname (ISREPORT *rep, ITEM item);
extern CCHAR*     isr_sep      (ISREPORT *rep);
extern CCHAR*     isr_impl     (ISREPORT *rep);

extern double     isr_log      (ISREPORT *rep, ITEM item);
extern double     isr_logwgt   (ISREPORT *rep);

extern double     isr_logrto   (ISREPORT *rep, void *data);
extern double     isr_logsize  (ISREPORT *rep, void *data);
extern double     isr_sizewgt  (ISREPORT *rep, void *data);
extern double     isr_wgtsize  (ISREPORT *rep, void *data);
extern double     isr_wgtsupp  (ISREPORT *rep, void *data);

extern ptrdiff_t  isr_report   (ISREPORT *rep);
extern ptrdiff_t  isr_reportx  (ISREPORT *rep, TID  *tids, TID n);
extern ptrdiff_t  isr_reporto  (ISREPORT *rep, ITEM *occs, TID n);
extern int        isr_direct   (ISREPORT *rep, const ITEM *items,ITEM n,
                                RSUPP supp, double wgt, double eval);
extern int        isr_directx  (ISREPORT *rep, const ITEM *items,ITEM n,
                                const double *iwgts,
                                RSUPP supp, double wgt, double eval);
extern int        isr_rule     (ISREPORT *rep, const ITEM *items,ITEM n,
                                RSUPP supp, RSUPP body, RSUPP head,
                                double eval);
extern int        isr_seqrule  (ISREPORT *rep, const ITEM *items,ITEM n,
                                RSUPP supp, RSUPP body, RSUPP head,
                                double eval);

extern size_t     isr_repcnt   (ISREPORT *rep);
extern const size_t* isr_stats (ISREPORT *rep);
extern void       isr_prstats  (ISREPORT *rep, FILE *out, ITEM min);
#ifdef ISR_PATSPEC
extern int        isr_addpsp   (ISREPORT *rep, PATSPEC *psp);
extern PATSPEC*   isr_rempsp   (ISREPORT *rep, int delpsp);
extern PATSPEC*   isr_getpsp   (ISREPORT *rep);
#endif
extern int        isr_intout   (ISREPORT *rep, diff_t num);
extern int        isr_numout   (ISREPORT *rep, double num, int digits);
extern int        isr_wgtout   (ISREPORT *rep, RSUPP supp, double wgt);
extern int        isr_sinfo    (ISREPORT *rep, RSUPP supp, double wgt,
                                double eval);
extern int        isr_rinfo    (ISREPORT *rep, RSUPP supp,
                                RSUPP body, RSUPP head, double eval);
extern void       isr_getinfo  (ISREPORT *rep, const char *sel,
                                double *vals);
#ifdef ISR_CLOMAX
extern ITEM*      isr_buf      (ISREPORT *rep);
extern int        isr_check    (ISREPORT *rep, RSUPP supp);
extern int        isr_tail     (ISREPORT *rep,
                                const ITEM *items, ITEM n);
#endif

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define isr_base(r)       ((r)->base)
#define isr_mode(r)       ((r)->mode)
#define isr_target(r)     ((r)->mode & ISR_TARGET)
#define isr_file(r)       ((r)->file)
#define isr_name(r)       ((r)->name)

#define isr_setfmt(r,f)   ((r)->format = (f))
#define isr_setiwf(r,f)   ((r)->iwfmt  = (f))
#define isr_tidfile(r)    ((r)->tidfile)
#define isr_tidname(r)    ((r)->tidname)
#define isr_setsmt(r,s)   ((r)->supps[0] = (s))
#define isr_setwgt(r,w)   ((r)->wgts [0] = (w))
#define isr_min(r)        ((r)->min)
#define isr_max(r)        ((r)->max)

#define isr_uses(r,i)     ((r)->pxpp[i] < 0)
#define isr_xable(r,n)    ((r)->cnt+(n) <= (r)->maxx)

#define isr_cnt(r)        ((r)->cnt)
#define isr_item(r)       ((r)->items[(r)->cnt -1])
#define isr_itemx(r,i)    ((r)->items[i])
#define isr_items(r)      ((const ITEM*)(r)->items)
#define isr_setsent(r)    ((r)->items[(r)->cnt] = -1)
#define isr_supp(r)       ((r)->supps[(r)->cnt])
#define isr_suppx(r,i)    ((r)->supps[i])
#define isr_wgt(r)        ((r)->wgts [(r)->cnt])
#define isr_wgtx(r,i)     ((r)->wgts [i])
#define isr_logsum(r)     ((r)->sums [(r)->cnt])
#define isr_logsumx(r,i)  ((r)->sums [i])
#define isr_eval(r)       ((r)->eval)

#define isr_pexcnt(r)     ((ITEM)((r)->items -(r)->pexs))
#define isr_pex(r,t)      ((r)->pexs [i])
#define isr_pexs(r)       ((const ITEM*)(r)->pexs)
#define isr_all(r)        ((r)->cnt +(ITEM)((r)->items -(r)->pexs))
#define isr_lack(r)       ((r)->min -isr_all(r))

#define isr_itemname(r,i) ((r)->inames[i])
#define isr_sep(r)        ((r)->isep)
#define isr_impl(r)       ((r)->impl)

#define isr_log(r,i)      ((r)->logs[i])
#define isr_logwgt(r)     ((r)->logwgt)

#define isr_repcnt(r)     ((r)->rep)
#define isr_stats(r)      ((const size_t*)(r)->stats)
#ifdef ISR_PATSPEC
#define isr_getpsp(r)     ((r)->psp)
#endif
#ifdef ISR_CLOMAX
#define isr_buf(r)        ((r)->iset)
#define isr_check(r,s)    ((r)->clomax ? cm_check((r)->clomax, s) : -1)
#define isr_tail(r,i,n)   ((r)->clomax && \
                           (cm_tail((r)->clomax, i, n) > 0) ? 1 : 0)
#endif

#endif
