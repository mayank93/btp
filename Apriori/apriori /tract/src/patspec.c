/*----------------------------------------------------------------------
  File    : patspec.c
  Contents: pattern spectrum management
  Author  : Christian Borgelt
  History : 2012.10.12 file created
            2013.09.28 adapted to definitions of ITEM and SUPP
            2013.09.30 sums per size and total sum of frequencies added
            2013.10.15 functions psp_error() and psp_clear() added
            2013.10.16 made compatible with double support type
            2014.01.08 bug in function resize() fixed (size > n)
----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#if defined PSP_MAIN && !defined PSP_REPORT
#define PSP_REPORT
#endif
#include "patspec.h"
#ifdef PSP_MAIN
#include "error.h"
#endif
#ifdef STORAGE
#include "storage.h"
#endif

#ifdef _MSC_VER
#ifndef snprintf
#define snprintf _snprintf
#endif
#endif                          /* MSC still does not support C99 */

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define int         1           /* to check definition */
#define long        2           /* of support type */
#define ptrdiff_t   3
#define double      4

#if SUPP==int || SUPP==long || SUPP==ptrdiff_t
#define INTSUPP     1           /* set   integer support flag */
#elif SUPP==double
#define INTSUPP     0           /* clear integer support flag */
#else
#error "SUPP must be either 'int', 'long', 'ptrdiff_t' or 'double'"
#endif

#undef int                      /* remove preprocessor definitions */
#undef long                     /* needed for the type checking */
#undef ptrdiff_t
#undef double

/*--------------------------------------------------------------------*/
#define BLKSIZE      32         /* block size for enlarging arrays */

/*----------------------------------------------------------------------
  Global Variables
----------------------------------------------------------------------*/
static PSPROW empty = { SUPP_MAX, SUPP_MIN, 0, NULL };
/* an empty row entry for initializing new rows */

#ifdef PSP_MAIN
static CCHAR *errmsgs[] = {     /* error messages */
  /* E_NONE      0 */  "no error",
  /* E_NOMEM    -1 */  "not enough memory",
  /* E_FOPEN    -2 */  "cannot open file %s",
  /* E_FREAD    -3 */  "read error on file %s",
  /* E_FWRITE   -4 */  "write error on file %s",
  /* E_STDIN    -5 */  "double assignment of standard input",
  /*            -6 */  "unknown error"
};
#endif

/*----------------------------------------------------------------------
  Global Variables
----------------------------------------------------------------------*/
#ifdef PSP_MAIN
static CCHAR    *prgname;       /* program name for error messages */
static PATSPEC  *pspec  = NULL; /* pattern spectrum */
static TABWRITE *twrite = NULL; /* table writer */
#endif

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/

PATSPEC* psp_create (ITEM minsize, ITEM maxsize,
                     SUPP minsupp, SUPP maxsupp)
{                               /* --- create pattern spectrum */
  PATSPEC *psp;                 /* created pattern spectrum */

  assert((minsize >= 0) && (maxsize >= minsize)   /* check the */
  &&     (minsupp >= 0) && (maxsupp >= minsupp)); /* arguments */
  psp = (PATSPEC*)malloc(sizeof(PATSPEC));
  if (!psp) return NULL;        /* create the base structure */
  psp->minsize = minsize;       /* and initialize the fields */
  psp->maxsize = ((maxsize < 0) || (maxsize >= ITEM_MAX))
               ? ITEM_MAX-1 : maxsize;
  psp->minsupp = minsupp;
  psp->maxsupp = ((maxsupp < 0) || (maxsupp >= SUPP_MAX))
               ? SUPP_MAX-1 : maxsupp;
  psp->total   = psp->sigcnt = 0;
  psp->max     = minsize-1;
  psp->err     = 0;
  psp->rows    = NULL;
  return psp;                   /* return created pattern spectrum */
}  /* psp_create() */

/*--------------------------------------------------------------------*/

void psp_delete (PATSPEC *psp)
{                               /* --- delete pattern spectrum */
  ITEM size;                    /* loop variable */

  assert(psp);                  /* check the function argument */
  if (psp->rows) {              /* if there are pattern spectrum rows */
    #if INTSUPP                 /* if integer support type */
    for (size = psp->minsize; size < psp->max; size++)
      if (psp->rows[size].frqs) free(psp->rows[size].frqs);
    #endif                      /* delete the counter arrays */
    free(psp->rows);            /* delete the row array */
  }
  free(psp);                    /* delete the base structure */
}  /* psp_delete() */

/*--------------------------------------------------------------------*/

void psp_clear (PATSPEC *psp)
{                               /* --- clear pattern spectrum */
  ITEM size;                    /* loop variable */

  assert(psp);                  /* check the function argument */
  if (psp->rows) {              /* if there are pattern spectrum rows */
    for (size = psp->minsize; size < psp->max; size++) {
      #if INTSUPP               /* if integer support type */
      if (psp->rows[size].frqs) free(psp->rows[size].frqs);
      #endif                    /* delete the counter arrays */
      psp->rows[size] = empty;  /* reinitialize the rows */
    }                           /* (also needed for SUPP==double) */
  }
  psp->total = psp->sigcnt = 0; /* clear total and signature counter */
  psp->max   = psp->minsize-1;  /* clear the maximum size */
  psp->err   = 0;               /* and the error status */
}  /* psp_clear() */

/*--------------------------------------------------------------------*/

size_t psp_getfrq (PATSPEC *psp, ITEM size, SUPP supp)
{                               /* --- get a counter value */
  PSPROW *row;                  /* to access the table row */

  assert(psp);                  /* check the function arguments */
  if ((size < psp->minsize) || (size > psp->max))
    return 0;                   /* if row does not exist, abort */
  row = psp->rows +size;        /* get the indexed row */
  if ((supp < row->min)     || (supp > row->max))
    return 0;                   /* if counter does not exist, abort */
  #if INTSUPP                   /* if integer support type */
  return row->frqs[supp -row->min];     /* return the counter value */
  #else                         /* if double support type */
  return 1;                     /* return whether in supprt range */
  #endif
}  /* psp_getfrq() */

/*--------------------------------------------------------------------*/

static int resize (PATSPEC *psp, ITEM size, SUPP supp)
{                               /* --- resize the row array (sizes) */
  size_t n, i;                  /* new maximum size, loop variable */
  SUPP   min, max;              /* new minimum and maximum support */
  PSPROW *row;                  /* to access/reallocate the rows */
  size_t *p;                    /* to reallocate the counter array */

  assert(psp                    /* check the function arguments */
  &&    (size >= psp->minsize) && (size <= psp->maxsize)
  &&    (supp >= psp->minsupp) && (supp <= psp->maxsupp));
  if (size > psp->max) {        /* if outside of size range */
    n  = (size_t)psp->max;      /* compute the new array size */
    n += (n > BLKSIZE) ? n >> 1 : BLKSIZE;
    if (n < (size_t)size)         n = (size_t)size;
    if (n > (size_t)psp->maxsize) n = (size_t)psp->maxsize;
    row = (PSPROW*)realloc(psp->rows, (n+1) *sizeof(PSPROW));
    if (!row) return psp->err = -1;   /* enlarge the row array */
    for (i = (size_t)psp->max; ++i <= n; )
      row[i] = empty;           /* initialize the new elements */
    psp->rows = row;            /* set the new array */
    psp->max  = (ITEM)n;        /* and its size */
  }
  row = psp->rows +size;        /* get the indexed row */
  #if INTSUPP                   /* if integer support type */
  if ((supp >= row->min) && (supp <= row->max))
    return 0;                   /* if support is in range, abort */
  if      (!row->frqs)               min = supp     -BLKSIZE;
  else if (supp > row->min)          min = row->min;
  else if (supp > row->min -BLKSIZE) min = row->min -BLKSIZE;
  else                               min = supp;
  if (min < psp->minsupp +BLKSIZE)   min = psp->minsupp;
  if      (!row->frqs)               max = supp     +BLKSIZE;
  else if (supp < row->max)          max = row->max;
  else if (supp < row->max +BLKSIZE) max = row->max +BLKSIZE;
  else                               max = supp;
  if (max > psp->maxsupp)            max = psp->maxsupp;
  if (size <= 0) min = max = supp; /* only one counter for size = 0 */
  n = (size_t)max -(size_t)min +1; /* compute the new array size */
  p = (size_t*)realloc(row->frqs, n *sizeof(size_t));
  if (!p) return psp->err = -1; /* enlarge the counter array */
  if      (!row->frqs)          /* if new array created */
    memset(p, 0, n *sizeof(size_t));
  else if (supp > row->max) {   /* if enlarged at the end */
    n = (size_t)row->max -(size_t)row->min +1;
    memset(p +n, 0, (size_t)(max -row->max) *sizeof(size_t)); }
  else if (supp < row->min) {   /* if enlarged at the front */
    n = (size_t)(row->max -row->min +1);
    memmove(p +(row->min -min), p, n *sizeof(size_t));
    memset (p, 0, (size_t)(row->min -min) *sizeof(size_t));
  }                             /* move the existing counters, */
  row->frqs = p;                /* initialize the new elements */
  row->min  = min;              /* and set the new array */
  row->max  = max;              /* and its range */
  #else                         /* if double support type */
  if (supp < row->min) row->min = supp;
  if (supp > row->max) row->max = supp;
  #endif                        /* adapt the support range */
  return 0;                     /* return 'ok' */
}  /* resize() */

/*--------------------------------------------------------------------*/

int psp_setfrq (PATSPEC *psp, ITEM size, SUPP supp, size_t frq)
{                               /* --- set a counter value */
  PSPROW *row;                  /* to access the table row */

  assert(psp);                  /* check the function arguments */
  if ((size < psp->minsize) || (size > psp->maxsize)
  ||  (supp < psp->minsupp) || (supp > psp->maxsupp))
    return 0;                   /* ignore values outside range */
  if (resize(psp, size, supp) < 0)
    return psp->err = -1;       /* enlarge table if necessary */
  row   = psp->rows +size;      /* get the indexed row and */
  #if INTSUPP                   /* if integer support type */
  supp -= row->min;             /* remove the support offset */
  if (frq > 0) { if (row->frqs[supp] <= 0) psp->sigcnt++; }
  else         { if (row->frqs[supp] >  0) psp->sigcnt--; }
  frq -= row->frqs[supp];       /* compute the frequency change */
  row->frqs[supp] += frq;       /* update the signature frequency */
  #endif
  row->sum        += frq;       /* and the sum for the size */
  psp->total      += frq;       /* as well as the total frequency */
  return 0;                     /* return 'ok' */
}  /* psp_setfrq() */

/*--------------------------------------------------------------------*/

int psp_incfrq (PATSPEC *psp, ITEM size, SUPP supp, size_t frq)
{                               /* --- increase a counter value */
  PSPROW *row;                  /* to access the table row */

  assert(psp);                  /* check the function arguments */
  if ((size < psp->minsize) || (size > psp->maxsize)
  ||  (supp < psp->minsupp) || (supp > psp->maxsupp))
    return 0;                   /* ignore values outside range */
  if (resize(psp, size, supp) < 0)
    return psp->err = -1;       /* enlarge table if necessary */
  row   = psp->rows +size;      /* get the indexed row and */
  #if INTSUPP                   /* if integer support type */
  supp -= row->min;             /* remove the support offset */
  if ((row->frqs[supp] <= 0) && (row->frqs[supp] +frq > 0))
    psp->sigcnt++;              /* count a new signature */
  row->frqs[supp] += frq;       /* update the signature frequency */
  #else                         /* if double support type */
  psp->sigcnt++;                /* count a new signature */
  #endif
  row->sum        += frq;       /* update the sum for the size */
  psp->total      += frq;       /* as well as the total frequency */
  return 0;                     /* return 'ok' */
}  /* psp_incfrq() */

/*--------------------------------------------------------------------*/

int psp_addpsp (PATSPEC *dst, PATSPEC *src)
{                               /* --- add a spectrum to another */
  PSPROW *row;                  /* to traverse the rows (sizes) */
  ITEM   size;                  /* loop variable for sizes */
  #if INTSUPP
  SUPP   supp;                  /* loop variable for supports */
  size_t frq;                   /* (size,supp) signature frequency */
  #endif

  assert(dst && src);           /* check the function arguments */
  for (size = src->minsize; size <= src->max; size++) {
    row = src->rows +size;      /* traverse the rows (sizes) */
    #if INTSUPP                 /* if integer support type */
    if (!row->frqs) continue;   /* if no counters exist, skip row */
    for (supp = row->min; supp <= row->max; supp++)
      if ((frq = row->frqs[supp-row->min]) > 0)
        psp_incfrq(dst, size, supp, frq);
    #else                       /* if double support type */
    if (row->max < row->min) continue;
    if (resize(dst, size, row->min) < 0)
      return dst->err = -1;     /* enlarge table if necessary */
    psp_incfrq(dst, size, row->max, row->sum);
    #endif                      /* update the pattern spectrum */
  }                             /* with the source values */
  return dst->err;              /* return the error status */
}  /* psp_addpsp() */

/*--------------------------------------------------------------------*/
#ifdef PSP_REPORT

int psp_report (PATSPEC *psp, TABWRITE *twr)
{                               /* --- report pattern spectrum */
  PSPROW *row;                  /* to traverse the rows (sizes) */
  ITEM   size;                  /* loop variable for sizes */
  SUPP   supp;                  /* loop variable for supports */
  size_t frq;                   /* (size,supp) signature frequency */
  char   buf[64];               /* output buffer for pattern size */

  assert(psp && twr);           /* check the function arguments */
  for (size = psp->minsize; size <= psp->max; size++) {
    row = psp->rows +size;      /* traverse the rows (sizes) */
    #if INTSUPP                 /* if integer support type */
    if (!row->frqs) continue;   /* if no counters exist, skip row */
    snprintf(buf, sizeof(buf), "%"ITEM_FMT, size);
    for (supp = row->min; supp <= row->max; supp++) {
      if ((frq = row->frqs[supp-row->min]) <= 0)
        continue;               /* traverse the columns (support) */
      twr_puts(twr, buf);                 twr_fldsep(twr);
      twr_printf(twr, "%"SUPP_FMT, supp); twr_fldsep(twr);
      twr_printf(twr, "%"SIZE_FMT, frq);  twr_recsep(twr);
    }                           /* print (size,support,counter) */
    #else                       /* if double support type */
    if (row->max < row->min) continue;
    twr_printf(twr, "%"ITEM_FMT, size);     twr_fldsep(twr);
    twr_printf(twr, "%"SUPP_FMT, row->min); twr_fldsep(twr);
    twr_printf(twr, "%"SUPP_FMT, row->max); twr_recsep(twr);
    #endif                      /* print (size,minsupp,maxsupp) */
  }
  return twr_error(twr);        /* return a write error indicator */
}  /* psp_report() */

#endif
/*--------------------------------------------------------------------*/
#ifndef NDEBUG

void psp_show (PATSPEC *psp)
{                               /* --- show a pattern spectrum */
  ITEM   size;                  /* loop variable for sizes */
  SUPP   supp;                  /* loop variable for support values */
  size_t frq;                   /* (size,supp) signature frequency */
  PSPROW *row;                  /* to traverse the rows (sizes) */

  assert(psp);                  /* check the function argument */
  printf("signcnt: %"SIZE_FMT"\n", psp->sigcnt);
  if (!psp->rows) return;       /* if there is no row array, abort */
  for (size = psp->minsize; size <= psp->maxsize; size++) {
    row = psp->rows +size;      /* traverse the rows (sizes) */
    #if INTSUPP                 /* if integer support type */
    if (!row->frqs) continue;   /* if no counters exist, skip row */
    printf("%3"ITEM_FMT":", size);
    for (supp = row->min; supp <= row->max; supp++)
      if ((frq = row->frqs[supp-row->min]) > 0)
        printf(" %"SUPP_FMT":%"SIZE_FMT, supp, frq);
    printf("\n");               /* print support:frequency pairs */
    #else                       /* if double support type */
    printf("%3"ITEM_FMT":", size);
    printf("%"SUPP_FMT":%"SUPP_FMT"\n", row->min, row->max)
    #endif                      /* print (size,minsupp,maxsupp) */
  }
  printf("sigcnt: %"SIZE_FMT"\n", psp->sigcnt);
  printf("total : %"SIZE_FMT"\n", psp->total);
}  /* psp_show() */

#endif
/*----------------------------------------------------------------------
  Main Function
----------------------------------------------------------------------*/
#ifdef PSP_MAIN

#ifndef NDEBUG                  /* if debug version */
  #undef  CLEANUP               /* clean up memory and close files */
  #define CLEANUP \
  if (twrite) twr_delete(twrite, 1); \
  if (pspec)  psp_delete(pspec);
#endif

GENERROR(error, exit)           /* generic error reporting function */

/*--------------------------------------------------------------------*/

int main (int argc, char *argv[])
{                               /* --- main function for testing */
  int      i;                   /* loop variable */
  ITEM     size;                /* size    of a signature */
  SUPP     supp;                /* support of a signature */

  prgname = argv[0];            /* get program name for error msgs. */
  pspec = psp_create(2, 12, 2, 12);
  if (!pspec) error(E_NOMEM);   /* create a pattern spectrum */
  for (i = 0; i < 10000; i++) { /* create some random signatures */
    size = (ITEM)(16 *(double)rand()/((double)RAND_MAX +1));
    supp = (SUPP)(16 *(double)rand()/((double)RAND_MAX +1));
    #if 0
    printf("%d: (%"ITEM_FMT",%"SUPP_FMT")\n", i, size, supp);
    #endif
    psp_incfrq(pspec, size, supp, 1);
  }                             /* register each signature */
  twrite = twr_create();        /* create a table writer and */
  if (!twrite) error(E_NOMEM);  /* configure the characters */
  twr_xchars(twrite, "\n", " ", " ", "?");
  if (twr_open(twrite, NULL, "") != 0)
    error(E_FOPEN, twr_name(twrite));
  psp_report(pspec, twrite);    /* report the pattern spectrum */
  twr_delete(twrite, 1);        /* delete the table writer */
  twrite = NULL;                /* clear the writer variable */
  printf("sigcnt: %"SIZE_FMT"\n", psp_sigcnt(pspec));
  printf("total:  %"SIZE_FMT"\n", psp_total(pspec));
  return 0;                     /* return 'ok' */
}  /* main() */

#endif
