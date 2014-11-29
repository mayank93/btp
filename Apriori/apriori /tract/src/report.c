/*----------------------------------------------------------------------
  File    : report.c
  Contents: item set reporter management
  Author  : Christian Borgelt
  History : 2008.08.18 item set reporter created in tract.[ch]
            2008.08.30 handling of perfect extensions completed
            2008.09.01 handling of closed and maximal item sets added
            2008.09.08 functions isr_intout() and isr_numout() added
            2008.10.30 transaction identifier reporting added
            2008.10.31 item set reporter made a separate module
            2008.11.01 optional double precision support added
            2008.12.05 bug handling real-valued support fixed (_report)
            2009.10.15 counting of reported item sets added
            2010.02.11 closed/maximal item set filtering added
            2010.02.12 bugs in prefix tree handling fixed (clomax)
            2010.03.09 bug in reporting the empty item set fixed
            2010.03.11 filtering of maximal item sets improved
            2010.03.17 head union tail pruning for maximal sets added
            2010.03.18 parallel item set support and weight reporting
            2010.04.07 extended information reporting functions removed
            2010.07.01 correct output of infinite float values added
            2010.07.02 order of closed/maximal and size filtering fixed
            2010.07.04 bug in isr_report() fixed (closed set filtering)
            2010.07.12 null output file made possible (for benchmarking)
            2010.07.19 bug in function isr_report() fixed (clomax)
            2010.07.21 early closed/maximal repository pruning added
            2010.07.22 adapted to closed/maximal item set filter
            2010.08.06 function isr_direct() for direct reporting added
            2010.08.11 function isr_directx() for extended items added
            2010.08.14 item set header for output added to isr_create()
            2010.10.15 functions isr_open(), isr_close(), isr_rule()
            2010.10.27 handling of null names in isr_open() changed
            2011.05.06 generalized to support type RSUPP (int/double)
            2011.06.10 function isr_wgtsupp() added (weight/support)
            2011.07.12 adapted to optional integer item names
            2011.07.23 parameter dir added to function isr_seteval()
            2011.08.16 filtering for generators added (with hash table)
            2011.08.17 header/separator/implication sign copied
            2011.08.19 item sorting for generator filtering added
            2011.08.27 no explicit item set generation for no output
            2011.08.29 internal file write buffer added (faster output)
            2011.09.20 internal repository for filtering made optional
            2011.09.27 bug in function isr_report() fixed (item counter)
            2011.10.18 bug in function fastchk() fixed (hdr/sep check)
            2011.10.21 output of floating point numbers improved
            2012.04.10 function isr_addnc() added (no perf. ext. check)
            2012.04.17 weights and logarithms initialized to zero
            2012.05.30 function isr_addpexpk() added (packed items)
            2012.07.23 format character 'd' added (absolute support)
            2012.10.16 bug in function isr_rinfo() fixed ("L", lift)
            2012.10.26 bug in function fastout() fixed (empty set)
            2013.01.25 output order item set/perfect extensions changed
            2013.03.07 adapted to direction param. of sorting functions
            2013.03.10 adapted to modified bsearch/bisect interface
            2013.08.22 number of significant digits limited in getsd()
            2013.10.08 function isr_seqrule() added (head at end)
            2013.10.15 check of ferror() added to isr_[tid]close()
            2013.11.07 item name handling made optional (pyfim/pycoco)
----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <assert.h>
#include <math.h>
#include "report.h"
#ifndef ISR_NONAMES
#include "scanner.h"
#endif
#ifdef STORAGE
#include "storage.h"
#endif

#ifdef _MSC_VER
#ifndef isnan
#define isnan(x)      _isnan(x)
#endif                          /* check for 'not a number' */
#ifndef isinf
#define isinf(x)    (!_isnan(x) && !_finite(x))
#endif                          /* check for an infinite value */
#endif                          /* MSC still does not support C99 */

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define BS_WRITE    65536       /* size of internal write buffer */
#define BS_INT         48       /* buffer size for integer output */
#define BS_FLOAT       96       /* buffer size for float   output */
#define LN_2        0.69314718055994530942  /* ln(2) */
#define MODEMASK    (ISR_TARGET|ISR_NOEXPAND|ISR_SORT)

/*----------------------------------------------------------------------
  Constants
----------------------------------------------------------------------*/
static const double pows[] = {  /* several powers of ten */
  1e-02, 1e-01,                 /* for floating point number output */
  1e+00, 1e+01, 1e+02, 1e+03, 1e+04, 1e+05, 1e+06, 1e+07,
  1e+08, 1e+09, 1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15,
  1e+16, 1e+17, 1e+18, 1e+19, 1e+20, 1e+21, 1e+22, 1e+23,
  1e+24, 1e+25, 1e+26, 1e+27, 1e+28, 1e+29, 1e+30, 1e+31,
  1e+32, 1e+33 };

/*----------------------------------------------------------------------
  Basic Output Functions
----------------------------------------------------------------------*/

static void fastchk (ISREPORT *rep)
{                               /* --- check for fast output mode */
  if (rep->repofn               /* if there is a report function */
  ||  rep->evalfn               /* or an evaluation function */
  ||  rep->tidfile)             /* or trans ids. are to be written, */
    rep->fast =  0;             /* standard output has to be used */
  else if (!rep->file)          /* if no output (and no filtering), */
    rep->fast = -1;             /* only count the item sets */
  else {                        /* if only an output file is written */
    rep->fast = ((rep->min <= 1) && (rep->max >= ITEM_MAX)
              && ((strcmp(rep->format, " (%a)") == 0)
              ||  (strcmp(rep->format, " (%d)") == 0))
              &&  (strcmp(rep->hdr,    "")      == 0)
              &&  (strcmp(rep->sep,    " ")     == 0)) ? +1 : 0;
  }                             /* check standard reporting settings */
}  /* fastchk() */

/*--------------------------------------------------------------------*/

static int getsd (const char *s, const char **end)
{                               /* --- get number of signif. digits */
  int k = 6;                    /* number of significant digits */

  assert(s && end);             /* check the function arguments */
  if ((*s >= '0') && (*s <= '9')) {
    k = *s++ -'0';              /* get the first digit */
    if ((*s >= '0') && (*s <= '9'))
      k = 10 *k +*s++ -'0';     /* get a possible second digit and */
  }                             /* compute the number of digits */
  if (k > 32) k = 32;           /* limit   the number of digits */
  *end = s; return k;           /* return  the number of digits */
}  /* getsd() */

/*--------------------------------------------------------------------*/

static void isr_flush (ISREPORT *rep)
{                               /* --- flush the output buffer */
  assert(rep);                  /* check the function arguments */
  fwrite(rep->buf, sizeof(char),(size_t)(rep->next-rep->buf),rep->file);
  rep->next = rep->buf;         /* write the output buffer */
}  /* isr_flush() */

/*--------------------------------------------------------------------*/

static void isr_putc (ISREPORT *rep, int c)
{                               /* --- write a single character */
  assert(rep);                  /* check the function arguments */
  if (rep->next >= rep->end)    /* if the output buffer is full, */
    isr_flush(rep);             /* flush it (write it to the file) */
  *rep->next++ = (char)c;       /* store the given character */
}  /* isr_putc() */

/*--------------------------------------------------------------------*/

static void isr_puts (ISREPORT *rep, const char *s)
{                               /* --- write a character string */
  assert(rep);                  /* check the function arguments */
  while (*s) {                  /* while not at end of string */
    if (rep->next >= rep->end)  /* if the output buffer is full, */
      isr_flush(rep);           /* flush it (write it to the file) */
    *rep->next++ = *s++;        /* store the next string character */
  }
}  /* isr_puts() */

/*--------------------------------------------------------------------*/

static void isr_putsn (ISREPORT *rep, const char *s, int n)
{                               /* --- write a character string */
  int k;                        /* number of chars in buffer */

  assert(rep);                  /* check the function arguments */
  while (n > 0) {               /* while there are characters left */
    k = (int)(rep->end -rep->next); /* get free space in write buffer */
    if (k >= n){                /* if the string fits into buffer */
      memcpy(rep->next, s, (size_t)n *sizeof(char));
      rep->next += n; break;    /* simply copy the string into */
    }                           /* the write buffer and abort */
    memcpy(rep->next, s, (size_t)k *sizeof(char));
    s += k; n -= k; rep->next = rep->end;
    isr_flush(rep);             /* fill the buffer, then flush it, */
  }                             /* and reduce the remaining string */
}  /* isr_putsn() */

/*--------------------------------------------------------------------*/

int isr_intout (ISREPORT *rep, diff_t num)
{                               /* --- print an integer number */
  int  i = BS_INT, n;           /* loop variable, character counter */
  char buf[BS_INT];             /* output buffer */

  assert(rep);                  /* check the function arguments */
  if (num == 0) {               /* treat zero as a special case */
    isr_putc(rep, '0'); return 1; }
  if (num <= PTRDIFF_MIN) {     /* treat minimum as a special case */
    #if PTRDIFF_MIN < 0x80000000/* if 64 bit system */
    isr_puts(rep, "-9223372036854775808"); return 20;
    #else                       /* if 32 bit system */
    isr_puts(rep, "-2147483648"); return 11;
    #endif                      /* directly return string and size */
  }
  n = 0;                        /* default: no sign printed */
  if (num < 0) {                /* if the number is negative, */
    num = -num; isr_putc(rep, '-'); n = 1; }  /* print a sign */
  do {                          /* digit output loop */
    buf[--i] = (char)((num % 10) +'0');  /* store the next digit */
    num /= 10;                  /* and remove it from the number */
  } while (num > 0);            /* while there are more digits */
  isr_putsn(rep, buf+i, BS_INT-i);
  n += BS_INT-i;                /* print the generated digits and */
  return n;                     /* return the number of characters */
}  /* isr_intout() */

/*--------------------------------------------------------------------*/

int mantout (ISREPORT *rep, double num, int digits, int ints)
{                               /* --- format a non-negative mantissa */
  int    i, n;                  /* loop variables, sign flag */
  double x, y;                  /* integral and fractional part */
  char   *s, *e, *d;            /* pointers into the output buffer */
  char   buf[BS_FLOAT];         /* output buffer */

  assert(rep);                  /* check the function arguments */
  i = (int)dbl_bisect(num, pows, 36);
  if ((i >= 36) || (pows[i] == num)) i++;
  n = digits -(i-2);            /* compute the number of decimals */
  x = floor(num); y = num-x;    /* split into integer and fraction */
  e = d = buf +40;              /* get buffer for the decimals */
  if (n > 0) {                  /* if to print decimal digits, */
    *e++ = '.';                 /* store a decimal point */
    do { y *= 10;               /* compute the next decimal */
      *e++ = (char)((int)y+'0');/* and store it in the buffer */
      y   -= floor(y);          /* remove the printed decimal */
    } while (--n > 0);          /* while there are more decimals */
  }                             /* remove a decimal if necessary */
  if ((y > 0.5) || ((y == 0.5)  /* if number needs to be rounded */
  &&  ((e > d) ? *(e-1) & 1 : floor(x/2) >= x/2))) {
    for (s = e; --s > d; ) {    /* traverse the decimal digits */
      if (*s < '9') { (*s)++; break; }
      *s = '0';                 /* if digit can be incremented, */
    }                           /* abort, otherwise store a zero */
    if ((s <= d) && ((x += 1) >= pows[i]))
      if (--e <= d+1) e = d;    /* if all decimals have been adapted, */
  }                             /* increment the integer part and */
  if (e > d) {                  /* if there are decimal places, */
    while (*--e == '0');        /* remove all trailing zeros */
    if (e > d) e++;             /* if there are no decimals left, */
  }                             /* also remove the decimal point */
  s = d;                        /* adapt the decimals if necessary */
  do {                          /* integral part output loop */
    *--s = (char)(fmod(x, 10) +'0');
    x = floor(x/10);            /* compute and store next digit */
  } while (x > 0);              /* while there are more digits */
  if ((n = (int)(d-s)) > ints)  /* check size of integral part */
    return -n;                  /* and abort if it is too large */
  isr_putsn(rep, s, n = (int)(e-s)); /* print the formatted number */
  return n;                     /* return the number of characters */
}  /* mantout() */

/*--------------------------------------------------------------------*/

int isr_numout (ISREPORT *rep, double num, int digits)
{                               /* --- print a floating point number */
  int  k, n, e;                 /* character counters and exponent */
  char buf[BS_FLOAT];           /* output buffer */

  assert(rep);                  /* check the function arguments */
  if (isnan(num)) {             /* check for 'not a number' */
    isr_puts(rep, "nan"); return 3; }
  n = 0;                        /* default: no sign printed */
  if (num < 0) {                /* if the number is negative, */
    num = -num; isr_putc(rep, '-'); n = 1; }  /* print a sign */
  if (isinf(num)) {             /* check for an infinite value */
    isr_puts(rep, "inf"); return n+3; }
  if (num < DBL_MIN) {          /* check for a zero value */
    isr_putc(rep, '0');   return n+1; }
  if (digits > 32) digits = 32; /* limit the number of sign. digits */
  if (digits > 11) {            /* if very high precision is needed */
    k = sprintf(buf, "%.*g", digits, num);
    isr_putsn(rep, buf, k);     /* format with standard printf, */
    return n+k;                 /* print the formatted number and */
  }                             /* return the number of characters */
  e = 0;                        /* default: no exponential represent. */
  if ((num >= pows[digits+2])   /* if an exponential representation */
  ||  (num <  0.001)) {         /* is of the number is preferable */
    while (num <  1e00) { num *= 1e32; e -= 32; }
    while (num >= 1e32) { num /= 1e32; e += 32; }
    k = (int)dbl_bisect(num, pows+2, 34);
    if ((k >= 34) || (pows[k+2] != num)) k--;
    e += k;                     /* find and extract decimal exponent */
    num /= pows[k+2];           /* compute the new mantissa */
  }                             /* (one digit before decimal point) */
  k = mantout(rep, num, digits, (e == 0) ? digits : 1);
  if (k < 0) {                  /* try to output the mantissa */
    num /= pows[1-k]; e += -1-k;/* on failure adapt the mantissa */
    k = mantout(rep, num, digits, 1);
  }                             /* output the adapted number */
  n += k;                       /* compute number of printed chars. */
  if (e == 0) return n;         /* if no exponent, abort the function */
  isr_putc(rep, 'e'); n += 2;   /* print an exponent indicator */
  isr_putc(rep, (e < 0) ? '-' : '+');
  if ((e = abs(e)) < 10) { isr_putc(rep, '0'); n++; }
  k = BS_INT;                   /* get the end of the buffer */
  do {                          /* exponent digit output loop */
    buf[--k] = (char)((e % 10) +'0');    /* store the next digit */
    e /= 10;                    /* and remove it from the number */
  } while (e > 0);              /* while there are more digits */
  isr_putsn(rep, buf+k, BS_INT-k);
  return n+BS_INT-k;            /* print the generated digits and */
}  /* isr_numout() */           /* return the number of characters */

/* It is (significantly) faster to output a floating point number  */
/* with the above routines than with sprintf. However, the above   */
/* code produces slightly less accurate output for more than about */
/* 14 significant digits. For those cases sprintf is used instead. */

/*--------------------------------------------------------------------*/

int isr_occout (ISREPORT *rep, ITEM occ)
{                               /* --- print a positive integer */
  int  i = BS_INT;              /* loop variable */
  char buf[BS_INT];             /* output buffer */

  assert(rep && (occ >= 0));    /* check the function arguments */
  do {                          /* digit output loop */
    buf[--i] = (char)((occ % 10) +'0');  /* store the next digit */
    occ /= 10;                  /* and remove it from the number */
  } while (occ > 0);            /* while there are more digits */
  fwrite(buf +i, sizeof(char), (size_t)(BS_INT-i), rep->tidfile);
  return BS_INT -i;             /* print the digits and */
}  /* isr_occout() */

/*--------------------------------------------------------------------*/

int isr_wgtout (ISREPORT *rep, RSUPP supp, double wgt)
{                               /* --- print an item weight */
  int        k, n = 0;          /* number of decimals, char. counter */
  const char *s, *t;            /* to traverse the format */

  assert(rep);                  /* check the function arguments */
  if (!rep->iwfmt || !rep->file)
    return 0;                   /* check for a given format and file */
  for (s = rep->iwfmt; *s; ) {  /* traverse the output format */
    if (*s != '%') {            /* copy everything except '%' */
      isr_putc(rep, *s++); n++; continue; }
    t = s++; k = getsd(s,&s);   /* get the number of signif. digits */
    switch (*s++) {             /* evaluate the indicator character */
      case '%': isr_putc(rep, '%'); n++;                   break;
      case 'g': n += isr_numout(rep, wgt,              k); break;
      case 'w': n += isr_numout(rep, wgt,              k); break;
      case 'm': n += isr_numout(rep, wgt/(double)supp, k); break;
      case  0 : --s;            /* print the requested quantity */
      default : isr_putsn(rep, t, k = (int)(s-t)); n += k; t = s; break;
    }                           /* otherwise copy characters */
  }
  return n;                     /* return the number of characters */
}  /* isr_wgtout() */

/*--------------------------------------------------------------------*/

int isr_tidout (ISREPORT *rep, TID tid)
{                               /* --- print a positive integer */
  int  i = BS_INT;              /* loop variable */
  char buf[BS_INT];             /* output buffer */

  assert(rep && (tid >= 0));    /* check the function arguments */
  do {                          /* digit output loop */
    buf[--i] = (char)((tid % 10) +'0');  /* store the next digit */
    tid /= 10;                  /* and remove it from the number */
  } while (tid > 0);            /* while there are more digits */
  fwrite(buf +i, sizeof(char), (size_t)(BS_INT-i), rep->tidfile);
  return BS_INT -i;             /* print the digits and */
}  /* isr_tidout() */

/*----------------------------------------------------------------------
  Generator Filtering Functions
----------------------------------------------------------------------*/
#ifdef ISR_CLOMAX

static size_t is_hash (const void *set, int type)
{                               /* --- compute item set hash value */
  size_t     i;                 /* loop variable */
  size_t     h;                 /* computed hash value */
  const ITEM *p;                /* to access the items */

  assert(set);                  /* check the function argument */
  p = (const ITEM*)set;         /* type the item set pointer */
  h = (size_t)*p++;             /* get the number of items */
  i = (h >> 3) +1;              /* use Duff's device */
  switch (h & 7) {              /* to compute the hash value */
    do {    h = h *251 +(size_t)*p++;
    case 7: h = h *251 +(size_t)*p++;
    case 6: h = h *251 +(size_t)*p++;
    case 5: h = h *251 +(size_t)*p++;
    case 4: h = h *251 +(size_t)*p++;
    case 3: h = h *251 +(size_t)*p++;
    case 2: h = h *251 +(size_t)*p++;
    case 1: h = h *251 +(size_t)*p++;
    case 0: ; } while (--i > 0);
  }                             /* semicolon is necessary */
  return h;                     /* return the computed hash value */
  /* This hash function treats an item set like a string, that is, */
  /* the hash code depends on the order of the items. This is no   */
  /* drawback, though, because the comparison also requires that   */
  /* the items are in the same order in the item sets to compare.  */
  /* However, an order-independent hash code could be used to make */
  /* the function is_isgen() faster by avoiding recomputations.    */
}  /* is_hash() */

/*--------------------------------------------------------------------*/

static int is_cmp (const void *a, const void *b, void *d)
{                               /* --- compare two item sets */
  ITEM n;                       /* loop variable, number of items */
  ITEM *x, *y;                  /* to access the item sets */

  assert(a && b);               /* check the function arguments */
  x = (ITEM*)a; y = (ITEM*)b;   /* get/type the item set pointers */
  n = *x++;                     /* if the item set sizes differ, */
  if (n != *y++) return 1;      /* the item sets are not equal */
  while (--n >= 0)              /* traverse and compare the items */
    if (x[n] != y[n]) return 1; /* if an item differs, abort */
  return 0;                     /* otherwise the item sets are equal */
  /* Using memcmp() for the comparison is slower, because memcmp() */
  /* also checks the order relationship, not just equality, which, */
  /* however, is all that is needed inside the hash table.         */
}  /* is_cmp() */

/*--------------------------------------------------------------------*/

static int is_isgen (ISREPORT *rep, ITEM item, RSUPP supp)
{                               /* --- check for a generator */
  ITEM   i;                     /* loop variable */
  size_t z;                     /* key size */
  ITEM   *p;                    /* to access the hash table key */
  SUPP   *s;                    /* to access the hash table data */
  ITEM   a, b;                  /* buffers for items (hold-out) */

  assert(rep && (item >= 0));   /* check the function arguments */
  rep->iset[rep->cnt+1] = item; /* store the new item at the end */
  if (rep->cnt > 0) {           /* if the current set is not empty */
    rep->iset[0] = rep->cnt;    /* copy the item set to the buffer */
    p = (ITEM*)memcpy(rep->iset+1, rep->items,
                      (size_t)rep->cnt *sizeof(ITEM));
    if (rep->mode & ISR_SORT)   /* sort the items according to code */
      ia_qsort(p, (size_t)rep->cnt+1, rep->dir);
    a = p[i = rep->cnt];        /* note the first hold-out item */
    for (++i; --i >= 0; ) {     /* traverse the items in the set */
      b = p[i]; p[i] = a; a = b;/* get next subset (next hold-out) */
      if (a == item) continue;  /* do not exclude the new item */
      s = (SUPP*)st_lookup(rep->gentab, rep->iset, 0);
      if (!s || (*s == supp))   /* if a subset with one item less */
        break;                  /* is not in the generator repository */
    }                           /* or has the same support, abort */
    if (i >= 0) return 0;       /* if subset was found, no generator */
    memmove(p+1, p, (size_t)rep->cnt *sizeof(ITEM));
    p[0] = a;                   /* restore the full new item set */
  }                             /* (with the proper item order) */
  rep->iset[0] = rep->cnt+1;    /* store the new item set size */
  z = (size_t)(rep->cnt+2) *sizeof(ITEM);  /* compute key size */
  s = (SUPP*)st_insert(rep->gentab, rep->iset, 0, z, sizeof(SUPP));
  if (!s) return -1;            /* add the new set to the repository */
  *s = supp;                    /* and store its support as the data */
  return 1;                     /* return 'set is a generator' */
}  /* is_isgen() */

#endif
/*----------------------------------------------------------------------
  Main Functions
----------------------------------------------------------------------*/

ISREPORT* isr_create (ITEMBASE *base, int mode, int dir,
                      const char *hdr, const char *sep, const char *imp)
{                               /* --- create an item set reporter */
  ITEM       i, k, b = 0;       /* loop variables, buffers */
  ITEM       n;                 /* number of items */
  ISREPORT   *rep;              /* created item set reporter */
  size_t     len, sum;          /* length of an item name and sum */
  char       *buf;              /* buffer for formated item name */
  #ifndef ISR_NONAMES           /* if not to use item names */
  const char *name;             /* to traverse the item names */
  size_t     m;                 /* length of an item name */
  #endif

  assert(base);                 /* check the function arguments */
  if (mode &  ISR_GENERA)       /* generators *or* closed/maximal */
    mode &= ~(ISR_CLOSED|ISR_MAXIMAL|ISR_NOFILTER);
  if (mode & (ISR_CLOSED|ISR_MAXIMAL))
    mode |= ISR_NOEXPAND;       /* make reporting mode consistent */
  n   = ib_cnt(base);           /* get the number of items/trans. */
  rep = (ISREPORT*)malloc(sizeof(ISREPORT)
                        +(size_t)(n+n+1) *sizeof(char*));
  if (!rep) return NULL;        /* allocate the base structure */
  rep->base    = base;          /* store the item base */
  rep->file    = NULL;          /* clear the output file and its name */
  rep->name    = NULL;          /* and allocate a file write buffer */
  rep->buf     = (char*)malloc(BS_WRITE *sizeof(char));
  if (!rep->buf) { free(rep); return NULL; }
  rep->next    = rep->buf;
  rep->end     = rep->buf +BS_WRITE;
  rep->mode    = mode & MODEMASK;
  rep->rep     = 0;             /* init. the item set counter and */
  rep->min     = 1;             /* the range of item set sizes */
  rep->max     = n;             /* (minimum and maximum size) */
  rep->maxx    = ((mode & (ISR_CLOSED|ISR_MAXIMAL)) && (n < ITEM_MAX))
               ? n+1 : n;       /* special maximum for isr_xable() */
  rep->smin    = 0;             /* initialize the support range */
  rep->smax    = RSUPP_MAX;     /* (minimum and maximum support) */
  rep->cnt     = rep->pfx = 0;  /* init. the number of items */
  rep->evalfn  = (ISEVALFN*)0;  /* clear add. evaluation function */
  rep->evaldat = NULL;          /* and the corresponding data */
  rep->evalthh = rep->eval = 0; /* clear evaluation and its minimum */
  rep->evaldir = 1;             /* default: threshold is minimum */
  rep->repofn  = (ISREPOFN*)0;  /* clear item set report function */
  rep->repodat = NULL;          /* and the corresponding data */
  rep->tidfile = NULL;          /* clear the transaction id file */
  rep->tidname = NULL;          /* and its name */
  rep->occs    = NULL;          /* clear item occurrence array, */
  rep->tids    = NULL;          /* transaction ids array, and */
  rep->tidcnt  = 0;             /* the number of transaction ids */
  rep->tracnt  = 0;             /* set default value for the other */
  rep->miscnt  = 0;             /* transaction ids array variables */
  rep->inames  = (const char**)(rep->pos +n+1);
  memset((void*)rep->inames, 0, (size_t)n *sizeof(const char*));
  rep->out     = NULL;          /* organize the pointer arrays */
  rep->wgts    = rep->logs = rep->sums = NULL;
  #ifdef ISR_PATSPEC            /* if to manage a pattern spectrum */
  rep->psp     = NULL;          /* clear pattern spectrum variable */
  #endif
  #ifdef ISR_CLOMAX             /* if closed/maximal set filtering */
  rep->sto     = (mode & ISR_MAXONLY) ? 0 : RSUPP_MAX;
  rep->dir     = (dir < 0) ? -1 : 1;
  rep->iset    = NULL;          /* initialize all pointers */
  rep->clomax  = NULL;          /* for an easier abort on failure */
  rep->gentab  = NULL;          /* (can simply call isr_delete()) */
  if ((mode & (ISR_CLOSED|ISR_MAXIMAL|ISR_GENERA))
  && !(mode &  ISR_NOFILTER))   /* if to filter reported item sets */
    b = n+1;                    /* with an internal repository, */
  #endif                        /* get size of extra item set buffer */
  rep->stats = (size_t*)malloc((size_t)(n+1)         *sizeof(size_t)
                              +(size_t)(n+1+n+n+1+b) *sizeof(ITEM));
  rep->supps = (RSUPP*) malloc((size_t)(n+1)         *sizeof(RSUPP));
  if (!rep->stats || !rep->supps) { isr_delete(rep, 0); return NULL; }
  memset(rep->stats, 0, (size_t)(n+1) *sizeof(size_t)
                       +(size_t)(n+1) *sizeof(ITEM));
  rep->pxpp  = (ITEM*)(rep->stats +n+1);
  rep->pexs  = rep->pxpp +n+1;  /* allocate memory for the arrays */
  rep->items = rep->pexs += n;  /* and organize and initialize it */
  rep->supps[0] = base->wgt;    /* init. the empty set support */
  if (mode & ISR_WEIGHTS) {     /* if to use item set weights */
    rep->wgts = (double*)calloc((size_t)(n+1), sizeof(double));
    if (!rep->wgts) { isr_delete(rep, 0); return NULL; }
    rep->wgts[0] = (double)base->wgt;     /* create a number array */
  }                             /* and store the empty set support */
  if (mode & ISR_LOGS) {        /* if to compute logarithms of freqs. */
    rep->logs = (double*)calloc((size_t)(n+n+1), sizeof(double));
    if (!rep->logs) { isr_delete(rep, 0); return NULL; }
    rep->sums = rep->logs +n;   /* allocate the needed arrays */
    for (i = 0; i < n; i++)     /* compute logarithms of item freqs. */
      rep->logs[i] = log((double)ib_getfrq(base, i));
    rep->logwgt  = log((double)base->wgt);
    rep->sums[0] = 0;           /* store the log of the total weight */
  }                             /* and init. the sum of logarithms */
  #ifdef ISR_NONAMES            /* if not to use item names */
  for (sum = 0, i = 0; i < n; i++)
    rep->inames[i] = "";        /* clear all item names */
  #else                         /* if to use item names */
  for (sum = 0, i = 0; i < n; i++) {
    name = ib_xname(base, i);   /* traverse the items and their names */
    if (!(mode & ISR_SCAN))     /* if to use the items names directly */
      sum += strlen(name);      /* sum their string lengths */
    else {                      /* if name formatting may be needed */
      sum += m = scn_fmtlen(name, &len);
      if (m > len) {            /* if name formatting is needed */
        buf = (char*)malloc((m+1) *sizeof(char));
        if (buf) scn_format(buf, name, 0);
        name = buf;             /* format the item name */
      }                         /* (quote certain characters) */
    }                           /* and replace the original name */
    rep->inames[i] = name;      /* store the (formatted) item name */
    if (!name) { isr_delete(rep, 0); return NULL; }
  }                             /* check for proper name copying */
  #endif
  rep->inames[n] = NULL;        /* store a sentinel after the names */
  if (!hdr) hdr = "";           /* get default header      if needed */
  if (!sep) sep = " ";          /* get default separator   if needed */
  if (!imp) imp = " <- ";       /* get default implication if needed */
  i = (int)strlen(hdr); k = (int)strlen(sep);
  sum += (size_t)(i +(n-1) *k +1); /* compute the output buffer size */
  len  = strlen(imp) +(size_t)(i +k +3);
  buf  = (char*)malloc((sum+len) *sizeof(char));
  if (!buf) { isr_delete(rep, 0); return NULL; }
  rep->out    = strcpy(buf,hdr);/* allocate the output buffer */
  rep->pos[0] = buf +i;         /* and copy the header into it */
  rep->hdr    = strcpy(buf += sum, hdr);
  rep->sep    = strcpy(buf += i+1, sep);
  rep->imp    = strcpy(buf += k+1, imp);
  rep->iwfmt  = ":%w";          /* note header/separator/implication */
  rep->format = "  (%a)";       /* and formats for weights and info. */
  rep->fast   = -1;             /* default: only count the item sets */
  #ifdef ISR_CLOMAX             /* if closed/maximal filtering */
  if (b <= 0) return rep;       /* if to report all item sets, abort */
  rep->iset = rep->items +n+1;  /* set the second item set buffer */
  if (mode & ISR_GENERA) {      /* if to filter for generators, */
    n = 1024*1024-1;            /* create an item set hash table */
    rep->gentab = st_create((size_t)n, 0,
                            is_hash, is_cmp, NULL, (OBJFN*)0);
    if (!rep->gentab) { isr_delete(rep, 0); return NULL; } }
  else {                        /* if to filter for closed/maximal */
    rep->clomax = cm_create(dir, n);
    if (!rep->clomax) { isr_delete(rep, 0); return NULL; }
  }                             /* create a closed/maximal filter */
  #endif
  return rep;                   /* return created item set reporter */
}  /* isr_create() */

/*--------------------------------------------------------------------*/

int isr_delete (ISREPORT *rep, int mode)
{                               /* --- delete an item set reporter */
  #ifndef ISR_NONAMES           /* if to use item names */
  ITEM i;                       /* loop variable */
  #endif
  int  r, s;                    /* results of close operations */

  assert(rep);                  /* check the function argument */
  #ifdef ISR_PATSPEC            /* if pattern spectrum functions */
  if (rep->psp)    psp_delete(rep->psp);
  #endif                        /* delete the pattern spectrum */
  #ifdef ISR_CLOMAX             /* if closed/maximal filtering */
  if (rep->clomax) cm_delete(rep->clomax);
  if (rep->gentab) st_delete(rep->gentab);
  #endif                        /* delete the closed/maximal filter */
  if (rep->out) free(rep->out); /* delete the output buffer */
  #ifndef ISR_NONAMES           /* if to use item names */
  for (i = 0; rep->inames[i]; i++)
    if (rep->inames[i] != ib_name(rep->base, i))
      free((void*)rep->inames[i]);   /* delete all cloned names */
  #endif
  if (rep->logs)  free(rep->logs);   /* delete the arrays */
  if (rep->wgts)  free(rep->wgts);   /* (if they are present) */
  if (rep->supps) free(rep->supps);
  if (rep->stats) free(rep->stats);  /* delete the item base */
  if (mode & ISR_DELISET) ib_delete(rep->base);
  r = (mode & ISR_FCLOSE) ? isr_tidclose(rep) : 0;
  s = (mode & ISR_FCLOSE) ? isr_close(rep)    : 0;
  free(rep->buf);               /* delete the file write buffer */
  free(rep);                    /* delete the base structure */
  return (r) ? r : s;           /* return file closing result */
}  /* isr_delete() */

/*--------------------------------------------------------------------*/

int isr_open (ISREPORT *rep, FILE *file, const char *name)
{                               /* --- open an output file */
  assert(rep);                  /* check the function arguments */
  if (file)                     /* if a file is given, */
    rep->name = name;           /* store the file name */
  else if (! name) {            /* if no name is given */
    file = NULL;   rep->name = "<null>"; }
  else if (!*name) {            /* if an empty name is given */
    file = stdout; rep->name = "<stdout>"; }
  else {                        /* if a proper name is given */
    file = fopen(rep->name = name, "w");
    if (!file) return -2;       /* open file with given name */
  }                             /* and check for an error */
  rep->file = file;             /* store the new output file */
  fastchk(rep);                 /* check for fast output */
  return 0;                     /* return 'ok' */
}  /* isr_open() */

/*--------------------------------------------------------------------*/

int isr_close (ISREPORT *rep)
{                               /* --- close the output file */
  int r;                        /* result of fclose()/fflush() */

  assert(rep);                  /* check the function arguments */
  if (!rep->file) return 0;     /* check for an output file */
  isr_flush(rep);               /* flush the write buffer */
  r  = ferror(rep->file);       /* check the error indicator */
  r |= ((rep->file == stdout) || (rep->file == stderr))
     ? fflush(rep->file) : fclose(rep->file);
  rep->file = NULL;             /* close the current output file */
  fastchk(rep);                 /* check for fast output */
  return r;                     /* return the result of fclose() */
}  /* isr_close() */

/*--------------------------------------------------------------------*/

void isr_reset (ISREPORT *rep)
{                               /* --- reset the output counters */
  rep->rep = 0;                 /* reinit. number of reported sets */
  memset(rep->stats, 0,         /* clear the statistics array */
         (size_t)((size_t*)rep->pxpp-rep->stats)*sizeof(size_t));
  #ifdef ISR_PATSPEC            /* if pattern spectrum functions */
  if (rep->psp) psp_clear(rep->psp);
  #endif                        /* clear the pattern spectrum */
}  /* isr_reset() */

/*--------------------------------------------------------------------*/

void isr_setsize (ISREPORT *rep, ITEM min, ITEM max)
{                               /* --- set size range for item set */
  assert(rep                    /* check the function arguments */
  &&    (min >= 0) && (max >= min));
  rep->min  = min;              /* store the minimum and maximum */
  rep->max  = max;              /* size of an item set to report */
  rep->maxx = ((rep->mode & (ISR_CLOSED|ISR_MAXIMAL))
            && (max < ITEM_MAX)) ? max+1 : max;
  fastchk(rep);                 /* check for fast output */
}  /* isr_setsize() */

/*--------------------------------------------------------------------*/

void isr_setsupp (ISREPORT *rep, RSUPP min, RSUPP max)
{                               /* --- set support range for item set */
  assert(rep                    /* check the function arguments */
     && (min >= 0) && (max >= min));
  rep->smin = min;              /* store the minimum and maximum */
  rep->smax = max;              /* support of an item set to report */
}  /* isr_setsupp() */

/*--------------------------------------------------------------------*/

void isr_seteval (ISREPORT *rep, ISEVALFN evalfn, void *data,
                  int dir, double thresh)
{                               /* --- set evaluation function */
  assert(rep);                  /* check the function argument */
  rep->evalfn  = evalfn;        /* store the evaluation function, */
  rep->evaldat = data;          /* the corresponding user data, */
  rep->evaldir = (dir >= 0) ? +1 : -1;  /* the evaluation direction */
  rep->evalthh = rep->evaldir *thresh;  /* and the threshold value  */
  fastchk(rep);                 /* check for fast output */
}  /* isr_seteval() */

/*--------------------------------------------------------------------*/

void isr_setrepo (ISREPORT *rep, ISREPOFN repofn, void *data)
{                               /* --- set evaluation function */
  assert(rep);                  /* check the function argument */
  rep->repofn  = repofn;        /* store the reporting function and */
  rep->repodat = data;          /* the corresponding user data */
  fastchk(rep);                 /* check for fast output */
}  /* isr_setrepo() */

/*--------------------------------------------------------------------*/

int isr_tidopen (ISREPORT *rep, FILE *file, const char *name)
{                               /* --- set/open trans. id output file */
  assert(rep);                  /* check the function arguments */
  if (file) {                   /* if a file is given directly, */
    if      (name)           rep->tidname = name; /* store name */
    else if (file == stdout) rep->tidname = "<stdout>";
    else if (file == stderr) rep->tidname = "<stderr>";
    else                     rep->tidname = "<unknown>"; }
  else if (! name) {            /* if no name is given */
    file = NULL;             rep->tidname = "<null>"; }
  else if (!*name) {            /* if an empty name is given */
    file = stdout;           rep->tidname = "<stdout>"; }
  else {                        /* if a proper name is given */
    file = fopen(rep->tidname = name, "w");
    if (!file) return -2;       /* open file with given name */
  }                             /* and check for an error */
  rep->tidfile = file;          /* store the new output file */
  fastchk(rep);                 /* check for fast output */
  return 0;                     /* return 'ok' */
}  /* isr_tidopen() */

/*--------------------------------------------------------------------*/

int isr_tidclose (ISREPORT *rep)
{                               /* --- close trans. id output file */
  int r;                        /* result of fclose() */

  assert(rep);                  /* check the function arguments */
  if (!rep->tidfile) return 0;  /* check for an output file */
  r  = ferror(rep->tidfile);    /* check the error indicator */
  r |= ((rep->tidfile == stdout) || (rep->tidfile == stderr))
     ? fflush(rep->tidfile) : fclose(rep->tidfile);
  rep->tidfile = NULL;          /* close the current output file */
  fastchk(rep);                 /* check for fast output */
  return r;                     /* return the result of fclose() */
}  /* isr_tidclose() */

/*--------------------------------------------------------------------*/

void isr_tidcfg (ISREPORT *rep, TID tracnt, ITEM miscnt)
{                               /* --- configure trans. id output */
  rep->tracnt = tracnt;         /* note the number of transactions */
  rep->miscnt = miscnt;         /* and the accepted number of */
}  /* isr_tidcfg() */           /* missing items */

/*--------------------------------------------------------------------*/

int isr_add (ISREPORT *rep, ITEM item, RSUPP supp)
{                               /* --- add an item (only support) */
  assert(rep && (item >= 0)     /* check the function arguments */
  &&    (item < ib_cnt(rep->base)) && !isr_uses(rep, item));
  /* if (supp < rep->smin) return 0; */
  #ifdef ISR_CLOMAX             /* if closed/maximal filtering */
  if      (rep->clomax) {       /* if a closed/maximal filter exists */
    int r = cm_add(rep->clomax, item, supp);
    if (r <= 0) return r; }     /* add the item to the c/m filter */
  else if (rep->gentab) {       /* if a generator filter exists */
    int r = is_isgen(rep, item, supp);
    if (r <= 0) return r;       /* add item set to the gen. filter */
  }                             /* check if item needs processing */
  #endif
  rep->pxpp [item] |= ITEM_MIN; /* mark the item as used */
  rep->items[  rep->cnt] = item;/* store the item and its support */
  rep->supps[++rep->cnt] = supp;/* clear the perfect ext. counter */
  rep->pxpp [  rep->cnt] &= ITEM_MIN;
  return 1;                     /* return 'ok' */
}  /* isr_add() */

/*--------------------------------------------------------------------*/

int isr_addnc (ISREPORT *rep, ITEM item, RSUPP supp)
{                               /* --- add an item (only support) */
  assert(rep && (item >= 0)     /* check the function arguments */
  &&    (item < ib_cnt(rep->base)) && !isr_uses(rep, item));
  /* if (supp < rep->smin) return 0; */
  #ifdef ISR_CLOMAX             /* if closed/maximal filtering */
  if (rep->clomax) {            /* if a closed/maximal filter exists */
    int r = cm_addnc(rep->clomax, item, supp);
    if (r <= 0) return r;       /* add the item to the c/m filter */
  }                             /* check only for a memory error */
  #endif
  rep->pxpp [item] |= ITEM_MIN; /* mark the item as used */
  rep->items[  rep->cnt] = item;/* store the item and its support */
  rep->supps[++rep->cnt] = supp;/* clear the perfect ext. counter */
  rep->pxpp [  rep->cnt] &= ITEM_MIN;
  return 1;                     /* return 'ok' */
}  /* isr_addnc() */

/* In contrast to isr_add(), the function isr_addnc() does not check */
/* whether the extended prefix possesses a perfect extension.        */

/*--------------------------------------------------------------------*/

int isr_addx (ISREPORT *rep, ITEM item, RSUPP supp, double wgt)
{                               /* --- add an item (support & weight) */
  assert(rep && (item >= 0)     /* check the function arguments */
  &&    (item < ib_cnt(rep->base)) && !isr_uses(rep, item));
  /* if (supp < rep->smin) return 0; */
  #ifdef ISR_CLOMAX             /* if closed/maximal filtering */
  if      (rep->clomax) {       /* if a closed/maximal filter exists */
    int r = cm_add(rep->clomax, item, supp);
    if (r <= 0) return r; }     /* add the item to the c/m filter */
  else if (rep->gentab) {       /* if a generator filter exists */
    int r = is_isgen(rep, item, supp);
    if (r <= 0) return r;       /* add item set to the gen. filter */
  }                             /* check if item needs processing */
  #endif
  rep->pxpp [item] |= ITEM_MIN; /* mark the item as used */
  rep->items[  rep->cnt] = item;/* store the item and its support */
  rep->supps[++rep->cnt] = supp;/* as well as its weight */
  rep->wgts [  rep->cnt] = wgt; /* clear the perfect ext. counter */
  rep->pxpp [  rep->cnt] &= ITEM_MIN;
  return 1;                     /* return 'ok' */
}  /* isr_addx() */

/*--------------------------------------------------------------------*/

int isr_addpex (ISREPORT *rep, ITEM item)
{                               /* --- add a perfect extension */
  assert(rep && (item >= 0)     /* check the function arguments */
  &&    (item < ib_cnt(rep->base)));
  if ((rep->pxpp[item] < 0)     /* if the item is already in use */
  ||  (rep->mode & ISR_GENERA)) /* or to filter for generators, */
    return -1;                  /* perfect extensions are ignored */
  rep->pxpp[item] |= ITEM_MIN;  /* mark the item as used */
  *--rep->pexs = item;          /* store the added item and */
  rep->pxpp[rep->cnt]++;        /* count it for the current prefix */
  return 0;                     /* return 'ok' */
}  /* isr_addpex() */

/*--------------------------------------------------------------------*/

void isr_addpexpk (ISREPORT *rep, ITEM bits)
{                               /* --- add a perfect extension */
  ITEM i;                       /* loop variable/item */

  assert(rep);                  /* check the function arguments */
  bits &= ~ITEM_MIN;            /* traverse the set bits */
  for (i = 0; (UITEM)(1 << i) <= (UITEM)bits; i++) {
    if (((bits & (1 << i)) == 0)/* if the bit is not set */
    || (rep->pxpp[i] < 0)       /* or the item is already in use */
    || (rep->mode & ISR_GENERA))/* or to filter for generators, */
      continue;                 /* perfect extensions are ignored */
    rep->pxpp[i] |= ITEM_MIN;   /* mark the item as used */
    *--rep->pexs = i;           /* store the added item and */
    rep->pxpp[rep->cnt]++;      /* count it for the current prefix */
  }
}  /* isr_addpexpk() */

/*--------------------------------------------------------------------*/

void isr_remove (ISREPORT *rep, ITEM n)
{                               /* --- remove one or more items */
  ITEM i;                       /* loop variable, buffer for an item */

  assert(rep                    /* check the function arguments */
  &&    (n >= 0) && (n <= rep->cnt));
  #ifdef ISR_CLOMAX             /* if closed/maximal filtering */
  if (rep->clomax)              /* if a closed/maximal filter exists, */
    cm_remove(rep->clomax, n);  /* remove the same number of items */
  #endif                        /* from this filter */
  while (--n >= 0) {            /* traverse the items to remove */
    for (i = rep->pxpp[rep->cnt] & ~ITEM_MIN; --i >= 0; )
      rep->pxpp[*rep->pexs++] &= ~ITEM_MIN;
    i = rep->items[--rep->cnt]; /* traverse the item to remove */
    rep->pxpp[i] &= ~ITEM_MIN;  /* (current item and perfect exts.) */
  }                             /* and remove their "in use" markers */
  if (rep->cnt < rep->pfx)      /* if too few items are left, */
    rep->pfx = rep->cnt;        /* reduce the valid prefix */
}  /* isr_remove() */

/*--------------------------------------------------------------------*/

double isr_logrto (ISREPORT *rep, void *data)
{                               /* --- logarithm of support ratio */
  assert(rep);                  /* check the function arguments */
  return (log((double)rep->supps[rep->cnt])
             -(double)rep->sums [rep->cnt]
             +(double)(rep->cnt-1)*rep->logwgt) /LN_2;
}  /* isr_logrto() */

/* Evaluate an itemset by the logarithm of the quotient of the actual */
/* support of an item set and the support that is expected under full */
/* independence of the items (product of item probabilities times the */
/* total transaction weight). 'data' is needed for the interface.     */

/*--------------------------------------------------------------------*/

double isr_logsize (ISREPORT *rep, void *data)
{                               /* --- logarithm of support quotient */
  assert(rep);                  /* check the function arguments */
  return (log((double)rep->supps[rep->cnt])
             -(double)rep->sums [rep->cnt]
             +(double)(rep->cnt-1)*rep->logwgt)
       / ((double)rep->cnt *LN_2); /* divide by item set size */
}  /* isr_logsize() */

/*--------------------------------------------------------------------*/

double isr_sizewgt (ISREPORT *rep, void *data)
{                               /* --- item set size times weight */
  assert(rep);                  /* check the function arguments */
  return rep->wgts[rep->cnt] *(double)rep->cnt;
}  /* isr_sizewgt() */

/* Evaluate an item set by the product of size and weight in order to */
/* favor large item sets and thus to compensate anti-monotone weights.*/

/*--------------------------------------------------------------------*/

double isr_wgtsize (ISREPORT *rep, void *data)
{                               /* --- item set weight / size */
  assert(rep);                  /* check the function arguments */
  return (rep->cnt > 0) ? rep->wgts[rep->cnt] /(double)rep->cnt : 0;
}  /* isr_wgtsize() */

/*--------------------------------------------------------------------*/

double isr_wgtsupp (ISREPORT *rep, void *data)
{                               /* --- item set weight / size */
  double s;                     /* buffer for support */
  assert(rep);                  /* check the function arguments */
  return ((s = (double)rep->supps[rep->cnt]) > 0)
       ? (double)rep->wgts[rep->cnt] /s : 0;
}  /* isr_wgtsupp() */

/*--------------------------------------------------------------------*/

static void fastout (ISREPORT *rep, ITEM n)
{                               /* --- fast output of an item set */
  char       *s;                /* to traverse the output buffer */
  const char *name;             /* to traverse the item names */

  assert(rep);                  /* check the function argument */
  rep->stats[rep->cnt]++;       /* count the reported item set */
  rep->rep++;                   /* (for its size and overall) */
  #ifdef ISR_PATSPEC            /* if pattern spectrum functions */
  if (rep->psp)                 /* count item set in pattern spectrum */
    psp_incfrq(rep->psp, rep->cnt, (SUPP)rep->supps[rep->cnt], 1);
  #endif
  s = rep->pos[rep->pfx];       /* get the position for appending */
  while (rep->pfx < rep->cnt) { /* traverse the additional items */
    if (rep->pfx > 0)           /* if this is not the first item */
      for (name = rep->sep; *name; )
        *s++ = *name++;         /* copy the item separator */
    for (name = rep->inames[rep->items[rep->pfx]]; *name; )
      *s++ = *name++;           /* copy the item name to the buffer */
    rep->pos[++rep->pfx] = s;   /* compute and record new position */
  }                             /* for appending the next item */
  while (n > 0) {               /* traverse the perfect extensions */
    rep->items[rep->cnt++] = rep->pexs[--n];
    fastout(rep, n);            /* add the next perfect extension, */
    rep->pfx = --rep->cnt;      /* recursively report supersets, */
  }                             /* and remove the item again */
  isr_putsn(rep, rep->out, (int)(s-rep->out)); /* print item set */
  isr_putsn(rep, rep->info, rep->size);        /* and its support */
}  /* fastout() */

/*--------------------------------------------------------------------*/

static void output (ISREPORT *rep)
{                               /* --- output an item set */
  ITEM       i;                 /* loop variable, flag */
  TID        k;                 /* loop variable */
  ITEM       min;               /* minimum number of items */
  char       *s;                /* to traverse the output buffer */
  const char *name;             /* to traverse the item names */
  double     sum;               /* to compute the logarithm sums */

  assert(rep                    /* check the function arguments */
  &&    (rep->cnt >= rep->min)
  &&    (rep->cnt <= rep->max));
  if (!rep->evalfn) {           /* if no evaluation function is given */
    if (rep->wgts)              /* use the weight as evaluation */
      rep->eval = rep->wgts[rep->cnt]; }
  else {                        /* if an evaluation function is given */
    if (rep->logs) {            /* if to compute sums of logarithms */
      sum = rep->sums[rep->pfx];/* get the valid sum for a prefix */
      for (i = rep->pfx; i < rep->cnt; ) {
        sum += rep->logs[rep->items[i]];
        rep->sums[++i] = sum;   /* traverse the additional items */
      }                         /* and add the logarithms of */
    }                           /* their individual frequencies */
    rep->eval = rep->evalfn(rep, rep->evaldat);
    if (rep->evaldir *rep->eval < rep->evalthh)
      return;                   /* if the item set does not qualify, */
  }                             /* abort the output function */
  rep->stats[rep->cnt]++;       /* count the reported item set */
  rep->rep++;                   /* (for its size and overall) */
  #ifdef ISR_PATSPEC            /* if pattern spectrum functions */
  if (rep->psp)                 /* count item set in pattern spectrum */
    psp_incfrq(rep->psp, rep->cnt, (SUPP)rep->supps[rep->cnt], 1);
  #endif
  if (rep->repofn)              /* call reporting function if given */
    rep->repofn(rep, rep->repodat);
  if (!rep->file) return;       /* check for an output file */
  s = rep->pos[rep->pfx];       /* get the position for appending */
  while (rep->pfx < rep->cnt) { /* traverse the additional items */
    if (rep->pfx > 0)           /* if this is not the first item */
      for (name = rep->sep; *name; )
        *s++ = *name++;         /* copy the item separator */
    for (name = rep->inames[rep->items[rep->pfx]]; *name; )
      *s++ = *name++;           /* copy the item name to the buffer */
    rep->pos[++rep->pfx] = s;   /* compute and record new position */
  }                             /* for appending the next item */
  isr_putsn(rep, rep->out, (int)(s-rep->out));
  isr_sinfo(rep, rep->supps[rep->cnt],
            (rep->wgts) ? rep->wgts[rep->cnt] : 0, rep->eval);
  isr_putc(rep, '\n');          /* print the item set information */
  if (!rep->tidfile || !rep->tids) /* check whether to report */
    return;                        /* a list of transaction ids */
  if      (rep->tidcnt > 0) {   /* if tids are in ascending order */
    for (k = 0; k < rep->tidcnt; k++) {
      if (k > 0) fputs(rep->sep, rep->tidfile);
      isr_tidout(rep, rep->tids[k]+1);
    } }                         /* report the transaction ids */
  else if (rep->tidcnt < 0) {   /* if tids are in descending order */
    for (k = -rep->tidcnt; k > 0; ) {
      isr_tidout(rep, rep->tids[--k]+1);
      if (k > 0) fputs(rep->sep, rep->tidfile);
    } }                         /* report the transaction ids */
  else if (rep->tracnt > 0) {   /* if item occurrence counters */
    min = (ITEM)(rep->cnt-rep->miscnt); /* traverse all trans. ids */
    for (k = 0; k < rep->tracnt; k++) {
      if (rep->occs[k] < min)   /* skip all transactions that */
        continue;               /* do not contain enough items */
      if (k > 0) fputs(rep->sep, rep->tidfile);
      isr_tidout(rep, k+1);     /* print the transaction identifier */
      if (rep->miscnt <= 0) continue;
      fputc(':', rep->tidfile); /* print an item counter separator */
      isr_occout(rep, rep->occs[k]);
    }                           /* print number of contained items */
  }
  fputc('\n', rep->tidfile);    /* terminate the transaction id list */
}  /* output() */

/*--------------------------------------------------------------------*/

static void report (ISREPORT *rep, ITEM n)
{                               /* --- recursively report item sets */
  assert(rep && (n >= 0));      /* check the function arguments */
  while (n > 0) {               /* traverse the perfect extensions */
    rep->items[rep->cnt++] = rep->pexs[--n];
    if ((rep->cnt+n >= rep->min)   /* if a valid size can be reached */
    &&  (rep->cnt   <= rep->max))  /* (in the interval [min, max]), */
      report(rep, n);              /* recursively report supersets */
    if (--rep->cnt < rep->pfx)  /* remove the current item again */
      rep->pfx = rep->cnt;      /* and adapt the valid prefix */
  }
  if (rep->cnt >= rep->min)     /* if item set has minimum size, */
    output(rep);                /* report the current item set */
}  /* report() */

/*--------------------------------------------------------------------*/

ptrdiff_t isr_report (ISREPORT *rep)
{                               /* --- report the current item set */
  ITEM   n, k;                  /* number of perfect extensions */
  ITEM   z;                     /* item set size */
  size_t m, c;                  /* buffers for item set counting */
  double w;                     /* buffer for an item set weight */
  RSUPP  s;                     /* support buffer */
  #ifdef ISR_CLOMAX             /* if closed/maximal filtering */
  RSUPP  r;                     /* support buffer */
  ITEM   *items;                /* item set for prefix tree update */
  #endif

  assert(rep);                  /* check the function argument */
  n = (ITEM)(rep->items -rep->pexs); /* get number of perfect exts. */
  #ifdef ISR_CLOMAX             /* closed/maximal filtering support */
  if (rep->clomax) {            /* if a closed/maximal filter exists */
    s = rep->supps[rep->cnt];   /* get the support of the item set */
    r = cm_supp(rep->clomax);   /* and the maximal known support */
    if (r >= s)        return 0;/* check if item set is not closed */
    if (r >= rep->sto) return 0;/* check whether to store item set */
    k = rep->cnt +n;            /* compute the total number of items */
    if (n <= 0)                 /* if there are no perfect extensions */
      items = rep->items;       /* the items can be used directly */
    else {                      /* if there are perfect extensions */
      items = (ITEM*)memcpy(rep->iset, rep->pexs,
                            (size_t)k *sizeof(ITEM));
      ia_qsort(items, (size_t)k, rep->dir);
    }                           /* copy and sort the items in the set */
    if (cm_update(rep->clomax, items, k, s) < 0)
      return -1;                /* add the item set to the filter */
    if ((rep->mode & ISR_MAXIMAL) && (r >= 0))
      return  0;                /* check for a non-maximal item set */
  }                             /* (if the known support is > 0) */
  #endif
  if ((rep->cnt   > rep->max)   /* if the item set is too large or */
  ||  (rep->cnt+n < rep->min))  /* the minimum size cannot be reached */
    return 0;                   /* with prefect extensions, abort */
  if (rep->fast < 0) {          /* if just to count the item sets */
    /* if no output is produced and no item sets can be filtered out, */
    /* compute the number of item sets in the perfect ext. hypercube. */
    s = rep->supps[rep->cnt];   /* get the support of the item set */
    if (rep->mode & ISR_NOEXPAND) {
      z = rep->cnt +n;          /* if not to expand perfect exts., */
      rep->stats[z]++;          /* count only one item set */
      rep->rep++;               /* (for its size and overall) */
      #ifdef ISR_PATSPEC        /* if pattern spectrum functions */
      if (rep->psp && (psp_incfrq(rep->psp, z, (SUPP)s, 1) < 0))
        return -1;              /* if a pattern spectrum exists, */
      #endif                    /* count item set in pattern spectrum */
      return 1;                 /* return that a single item set */
    }                           /* was counted/reported */
    m = 0; z = rep->cnt;        /* and init. the item set counter */
    if (z >= rep->min) {        /* count the current item set */
      rep->stats[z]++; m++;     /* (for its size and overall) */
      #ifdef ISR_PATSPEC        /* if pattern spectrum functions */
      if (rep->psp && (psp_incfrq(rep->psp, z, (SUPP)s, 1) < 0))
        return -1;              /* if a pattern spectrum exists, */
      #endif                    /* count item set in pattern spectrum */
    }
    for (c = 1, k = 1; (k <= n) && (++z <= rep->max); k++) {
      c = (c *(size_t)(n-k+1))  /* compute n choose k */
        / (size_t)k;            /* for 1 <= k <= n */
      if (z >= rep->min) {      /* count the current item set */
        rep->stats[z] += c; m += c; /* (for its size and overall) */
        #ifdef ISR_PATSPEC      /* if pattern spectrum functions */
        if (rep->psp && (psp_incfrq(rep->psp, z, (SUPP)s, c) < 0))
          return -1;            /* if a pattern spectrum exists, */
        #endif                  /* count item set in pattern spectrum */
      }
    }                           /* (n choose k is the number of */
    rep->rep += m;              /* item sets of size rep->cnt +k) */
    return (ptrdiff_t)m;        /* return the number of item sets */
  }
  /* It is debatable whether this way of handling perfect extensions  */
  /* in case no output is produced is acceptable for fair benchmarks, */
  /* since the sets in the hypercube are not explicitly generated.    */
  if (rep->fast)                /* format support for fast output */
    rep->size = sprintf(rep->info, " (%"RSUPP_FMT")\n",
                        rep->supps[rep->cnt]);
  if (rep->mode & ISR_NOEXPAND){/* if not to expand perfect exts. */
    k = rep->cnt +n;            /* if all perfext extensions make */
    if (k > rep->max) return 0; /* the item set too large, abort */
    rep->supps[k] = rep->supps[rep->cnt];
    if (rep->wgts) rep->wgts[k] = rep->wgts[rep->cnt];
    for (k = n; --k >= 0; )     /* add all perfect extensions */
      rep->items[rep->cnt++] = rep->pexs[k];
    if (rep->fast) fastout(rep, 0); /* report the expanded set */
    else           output (rep);    /* (fast or normal output) */
    rep->cnt -= n; return 1;    /* remove the perfect extensions */
  }                             /* and abort the function */
  m = rep->rep;                 /* note the number of reported sets */
  if (rep->fast)                /* if fast output is possible, */
    fastout(rep, n);            /* report item sets recursively */
  else {                        /* if fast output is not possible */
    s = rep->supps[rep->cnt];   /* set support for pex. hypercube */
    for (k = 0; ++k <= n; ) rep->supps[rep->cnt+k] = s;
    if (rep->wgts) {            /* if there are item set weights, */
      w = rep->wgts[rep->cnt];  /* set weights for pex. hypercube */
      for (k = 0; ++k <= n; ) rep->wgts[rep->cnt+k] = w;
    }                           /* (support/weight is the same) */
    report(rep, n);             /* recursively add perfect exts. and */
  }                             /* report the resulting item sets */
  #ifdef ISR_PATSPEC            /* if pattern spectrum functions */
  if (rep->psp && psp_error(rep->psp))
    return -1;                  /* check whether updating the */
  #endif                        /* pattern spectrum failed */
  #ifndef NDEBUG                /* in debug mode */
  isr_flush(rep);               /* flush the output buffer */
  #endif                        /* after every item set */
  return (ptrdiff_t)(rep->rep -m);
}  /* isr_report() */           /* return number of rep. item sets */

/*--------------------------------------------------------------------*/

ptrdiff_t isr_reportx (ISREPORT *rep, TID *tids, TID n)
{                               /* --- report the current item set */
  ptrdiff_t r;                  /* number of reported item sets */

  assert(rep);                  /* check the function arguments */
  rep->tids   = tids;           /* store the transaction id array */
  rep->tidcnt = n;              /* and the number of transaction ids */
  r = isr_report(rep);          /* report the current item set */
  rep->tids   = NULL;           /* clear the transaction id array */
  return r;                     /* return number of rep. item sets */
}  /* isr_reportx() */

/*--------------------------------------------------------------------*/

ptrdiff_t isr_reporto (ISREPORT *rep, ITEM *occs, TID n)
{                               /* --- report the current item set */
  ptrdiff_t r;                  /* number of reported item sets */

  assert(rep);                  /* check the function arguments */
  rep->occs   = occs;           /* store the item occurrence array */
  rep->tids   = (TID*)-1;       /* and set the transaction id array */
  rep->tidcnt = n;              /* and the number of transaction ids */
  r = isr_report(rep);          /* report the current item set */
  rep->tids   = NULL;           /* clear the transaction id array */
  return r;                     /* return number of rep. item sets */
}  /* isr_reporto() */

/*--------------------------------------------------------------------*/

int isr_direct (ISREPORT *rep, const ITEM *items, ITEM n,
                RSUPP supp, double wgt, double eval)
{                               /* --- report an item set */
  ITEM c;                       /* buffer for the item counter */

  assert(rep                    /* check the function arguments */
  &&    (items || (n <= 0)) && (supp >= 0));
  if ((n < rep->min) || (n > rep->max))
    return 0;                   /* check the item set size */
  rep->stats[n]++;              /* count the reported item set */
  rep->rep++;                   /* (for its size and overall) */
  #ifdef ISR_PATSPEC            /* if pattern spectrum functions */
  if (rep->psp && (psp_incfrq(rep->psp, n, (SUPP)supp, 1) < 0))
    return -1;                  /* if a pattern spectrum exists, */
  #endif                        /* count item set in pattern spectrum */
  if (!rep->file) return 0;     /* check for an output file */
  c = rep->cnt; rep->cnt = n;   /* note the number of items */
  isr_puts(rep, rep->hdr);      /* print the record header */
  if (n > 0)                    /* print the first item */
    isr_puts(rep, rep->inames[*items++]);
  while (--n > 0) {             /* traverse the remaining items */
    isr_puts(rep, rep->sep);    /* print an item separator */
    isr_puts(rep, rep->inames[*items++]);
  }                             /* print the next item */
  isr_sinfo(rep, supp, wgt, eval);
  isr_putc(rep, '\n');          /* print the item set information */
  rep->cnt = c;                 /* restore the number of items */
  return 0;                     /* return 'ok' */
}  /* isr_direct() */

/*--------------------------------------------------------------------*/

int isr_directx (ISREPORT *rep, const ITEM *items, ITEM n,
                 const double *iwgts,
                 RSUPP supp, double wgt, double eval)
{                               /* --- report an item set */
  ITEM c;                       /* buffer for the item counter */

  assert(rep                    /* check the function arguments */
  &&    (items || (n <= 0)) && (supp >= 0));
  if ((n < rep->min) || (n > rep->max))
    return 0;                   /* check the item set size */
  rep->stats[n]++;              /* count the reported item set */
  rep->rep++;                   /* (for its size and overall) */
  #ifdef ISR_PATSPEC            /* if pattern spectrum functions */
  if (rep->psp && (psp_incfrq(rep->psp, n, (SUPP)supp, 1) < 0))
    return -1;                  /* if a pattern spectrum exists, */
  #endif                        /* count item set in pattern spectrum */
  if (!rep->file) return 0;     /* check for an output file */
  c = rep->cnt; rep->cnt = n;   /* note the number of items */
  isr_puts(rep, rep->hdr);      /* print the record header */
  if (n > 0) {                  /* if at least one item */
    isr_puts(rep, rep->inames[*items]);
    isr_wgtout(rep, supp, *iwgts);
  }                             /* print first item and item weight */
  while (--n > 0) {             /* traverse the remaining items */
    isr_puts(rep, rep->sep);    /* print an item separator */
    isr_puts(rep, rep->inames[*++items]);
    isr_wgtout(rep, supp, *++iwgts);
  }                             /* print next item and item weight */
  isr_sinfo(rep, supp, wgt, eval);
  isr_putc(rep, '\n');          /* print the item set information */
  rep->cnt = c;                 /* restore the number of items */
  return 0;                     /* return 'ok' */
}  /* isr_directx() */

/*--------------------------------------------------------------------*/

int isr_rule (ISREPORT *rep, const ITEM *items, ITEM n,
              RSUPP supp, RSUPP body, RSUPP head, double eval)
{                               /* --- report an association rule */
  ITEM c;                       /* buffer for the item counter */

  assert(rep                    /* check the function arguments */
  &&     items && (n > 0) && (supp >= 0));
  if ((n < rep->min) || (n > rep->max))
    return 0;                   /* check the item set size */
  rep->stats[n]++;              /* count the reported rule */
  rep->rep++;                   /* (for its size and overall) */
  if (!rep->file) return 0;     /* check for an output file */
  c = rep->cnt; rep->cnt = n;   /* note the number of items */
  isr_puts(rep, rep->hdr);      /* print the record header */
  isr_puts(rep, rep->inames[*items++]);
  isr_puts(rep, rep->imp);      /* print the rule head and imp. sign */
  if (--n > 0)                  /* print the first item in body */
    isr_puts(rep, rep->inames[*items++]);
  while (--n > 0) {             /* traverse the remaining items */
    isr_puts(rep, rep->sep);    /* print an item separator */
    isr_puts(rep, rep->inames[*items++]);
  }                             /* print the next item */
  isr_rinfo(rep, supp, body, head, eval);
  isr_putc(rep, '\n');          /* print the item set information */
  rep->cnt = c;                 /* restore the number of items */
  return 0;                     /* return 'ok' */
}  /* isr_rule() */

/*--------------------------------------------------------------------*/

int isr_seqrule (ISREPORT *rep, const ITEM *items, ITEM n,
                 RSUPP supp, RSUPP body, RSUPP head, double eval)
{                               /* --- report a sequence rule */
  ITEM c;                       /* buffer for the item counter */

  assert(rep                    /* check the function arguments */
  &&     items && (n > 0) && (supp >= 0));
  if ((n < rep->min) || (n > rep->max))
    return 0;                   /* check the item set size */
  rep->stats[n]++;              /* count the reported rule */
  rep->rep++;                   /* (for its size and overall) */
  if (!rep->file) return 0;     /* check for an output file */
  c = rep->cnt; rep->cnt = n;   /* note the number of items */
  isr_puts(rep, rep->hdr);      /* print the record header */
  if (--n > 0)                  /* print the first item in body */
    isr_puts(rep, rep->inames[*items++]);
  while (--n > 0) {             /* traverse the remaining items */
    isr_puts(rep, rep->sep);    /* print an item separator */
    isr_puts(rep, rep->inames[*items++]);
  }                             /* print the next item */
  isr_puts(rep, rep->imp);      /* print the imp. sign and rule head */
  isr_puts(rep, rep->inames[*items++]);
  isr_rinfo(rep, supp, body, head, eval);
  isr_putc(rep, '\n');          /* print the item set information */
  rep->cnt = c;                 /* restore the number of items */
  return 0;                     /* return 'ok' */
}  /* isr_seqrule() */

/*--------------------------------------------------------------------*/

void isr_prstats (ISREPORT *rep, FILE *out, ITEM min)
{                               /* --- print item set statistics */
  ITEM i, n;                    /* loop variables */

  assert(rep && out);           /* check the function arguments */
  fprintf(out, "all: %"SIZE_FMT"\n", rep->rep);
  for (n = (int)((size_t*)rep->pxpp -rep->stats); --n >= 0; )
    if (rep->stats[n] != 0) break;
  for (i = min; i <= n; i++)    /* print set counters per set size */
    fprintf(out, "%3"ITEM_FMT": %"SIZE_FMT"\n", i, rep->stats[i]);
}  /* isr_prstats() */

/*--------------------------------------------------------------------*/
#ifdef ISR_PATSPEC

int isr_addpsp (ISREPORT *rep, PATSPEC *psp)
{                               /* --- add a pattern spectrum */
  assert(rep);                  /* check the function arguments */
  if (rep->psp) return 1;       /* if pattern spectrum exists, abort */
  if (!psp) {                   /* if to create a pattern spectrum */
    psp = psp_create(rep->min,rep->max,(SUPP)rep->smin,(SUPP)rep->smax);
    if (!psp) return -1;        /* create a pattern spectrum */
  }                             /* with the stored limits */
  rep->psp = psp;               /* note the pattern spectrum */
  return 0;                     /* return 'ok' */
}  /* isr_addpsp() */

/*--------------------------------------------------------------------*/

PATSPEC* isr_rempsp (ISREPORT *rep, int delpsp)
{                               /* --- add a pattern spectrum */
  PATSPEC *psp;                 /* existing pattern spectrum */

  assert(rep);                  /* check the function arguments */
  psp = rep->psp;               /* get the stored pattern spectrum */
  rep->psp = NULL;              /* and clear the reporter variable */
  if (!delpsp) return psp;      /* if not to delete it, return it */
  if (psp) psp_delete(psp);     /* delete existing pattern spectrum */
  return NULL;                  /* return that there is none anymore */
}  /* isr_rempsp() */

#endif
/*--------------------------------------------------------------------*/

int isr_sinfo (ISREPORT *rep, RSUPP supp, double wgt, double eval)
{                               /* --- print item set information */
  int        k, n = 0;          /* number of decimals, char. counter */
  double     sdbl, smax, wmax;  /* (maximum) support and weight */
  const char *s, *t;            /* to traverse the format */

  assert(rep);                  /* check the function arguments */
  if (!rep->format || !rep->file)
    return 0;                   /* check for a given format and file */
  sdbl = (double)supp;          /* get support as double prec. number */
  smax = (double)rep->supps[0]; /* get maximum support and */
  if (smax <= 0) smax = 1;      /* avoid divisions by zero */
  wmax = (rep->wgts) ? rep->wgts[0] : smax;
  if (wmax <= 0) wmax = 1;      /* get maximum weight (if available) */
  for (s = rep->format; *s; ) { /* traverse the output format */
    if (*s != '%') {            /* copy everything except '%' */
      isr_putc(rep, *s++); n++; continue; }
    t = s++; k = getsd(s, &s);  /* get the number of signif. digits */
    switch (*s++) {             /* evaluate the indicator character */
      case '%': isr_putc(rep, '%'); n++;                  break;
    case 'i': n += isr_intout(rep, (diff_t)rep->cnt);     break;
      case 'n': case 'd':
      #define int    1
      #define double 2
      #if RSUPP==double
      case 'a': n += isr_numout(rep,         sdbl,    k); break;
      #else
      case 'a': n += isr_intout(rep, (diff_t)supp);       break;
      #endif
      #undef int
      #undef double
      case 's': n += isr_numout(rep,      sdbl/smax,  k); break;
      case 'S': n += isr_numout(rep, 100*(sdbl/smax), k); break;
      case 'x': n += isr_numout(rep,      sdbl/smax,  k); break;
      case 'X': n += isr_numout(rep, 100*(sdbl/smax), k); break;
      case 'w': n += isr_numout(rep,      wgt,        k); break;
      case 'W': n += isr_numout(rep, 100* wgt,        k); break;
      case 'r': n += isr_numout(rep,      wgt /wmax,  k); break;
      case 'R': n += isr_numout(rep, 100*(wgt /wmax), k); break;
      case 'z': n += isr_numout(rep,      wgt *smax,  k); break;
      case 'e': n += isr_numout(rep,      eval,       k); break;
      case 'E': n += isr_numout(rep, 100* eval,       k); break;
      case 'p': n += isr_numout(rep,      eval,       k); break;
      case 'P': n += isr_numout(rep, 100* eval,       k); break;
      case  0 : --s;            /* print the requested quantity */
    default : isr_putsn(rep, t, k = (int)(s-t)); n += k; t = s; break;
    }                           /* otherwise copy characters */
  }
  return n;                     /* return the number of characters */
}  /* isr_sinfo() */

/*--------------------------------------------------------------------*/

int isr_rinfo (ISREPORT *rep, RSUPP supp, RSUPP body, RSUPP head,
               double eval)
{                               /* --- print ass. rule information */
  int        k, n = 0;          /* number of decimals, char. counter */
  double     smax;              /* maximum support */
  double     conf, lift;        /* buffers for computations */
  const char *s, *t;            /* to traverse the format */

  assert(rep);                  /* check the function arguments */
  if (!rep->format || !rep->file)
    return 0;                   /* check for a given format and file */
  smax = (double)rep->supps[0]; /* get the total transaction weight */
  if (smax <= 0) smax = 1;      /* avoid divisions by zero */
  for (s = rep->format; *s; ) { /* traverse the output format */
    if (*s != '%') {            /* copy everything except '%' */
      isr_putc(rep, *s++); n++; continue; }
    t = s++; k = getsd(s, &s);  /* get the number of signif. digits */
    switch (*s++) {             /* evaluate the indicator character */
      case '%': isr_putc(rep, '%'); n++;               break;
      case 'n': case 'd':
      #define int    1
      #define double 2
      #if RSUPP==double
      case 'a': n += isr_numout(rep,         supp, k); break;
      case 'b': n += isr_numout(rep,         body, k); break;
      case 'h': n += isr_numout(rep,         head, k); break;
      #else
      case 'a': n += isr_intout(rep, (diff_t)supp);    break;
      case 'b': n += isr_intout(rep, (diff_t)body);    break;
      case 'h': n += isr_intout(rep, (diff_t)head);    break;
      #endif
      #undef int
      #undef double
      case 's': n += isr_numout(rep,      (double)supp/smax,  k); break;
      case 'S': n += isr_numout(rep, 100*((double)supp/smax), k); break;
      case 'x': n += isr_numout(rep,      (double)body/smax,  k); break;
      case 'X': n += isr_numout(rep, 100*((double)body/smax), k); break;
      case 'y': n += isr_numout(rep,      (double)head/smax,  k); break;
      case 'Y': n += isr_numout(rep, 100*((double)head/smax), k); break;
      case 'c': conf = (body > 0) ? (double)supp/(double)body : 0;
                n += isr_numout(rep,      conf,       k); break;
      case 'C': conf = (body > 0) ? (double)supp/(double)body : 0;
                n += isr_numout(rep, 100* conf,       k); break;
      case 'l': lift = ((body <= 0) || (head <= 0)) ? 0
                     : ((double)supp*smax) /((double)body*(double)head);
                n += isr_numout(rep,      lift,       k); break;
      case 'L': lift = ((body <= 0) || (head <= 0)) ? 0
                     : ((double)supp*smax) /((double)body*(double)head);
                n += isr_numout(rep, 100* lift,       k); break;
      case 'e': n += isr_numout(rep,      eval,       k); break;
      case 'E': n += isr_numout(rep, 100* eval,       k); break;
      case  0 : --s;            /* print the requested quantity */
      default : isr_putsn(rep, t, k = (int)(s-t)); n += k; t = s; break;
    }                           /* otherwise copy characters */
  }
  return n;                     /* return the number of characters */
}  /* isr_rinfo() */

/*--------------------------------------------------------------------*/

void isr_getinfo (ISREPORT *rep, const char *sel, double *vals)
{                               /* --- get item set information */
  double supp;                  /* support of current item set */
  double wgt;                   /* weight of current item set */
  double smax, wmax;            /* maximum support and weight */

  supp = (double)rep->supps[rep->cnt];  /* get set support and weight */
  wgt  = (rep->wgts) ? rep->wgts[rep->cnt] : 0;
  smax = (double)rep->supps[0]; /* get maximum support and */
  if (smax <= 0) smax = 1.0;    /* avoid divisions by zero */
  wmax = (rep->wgts) ? rep->wgts[0] : smax;
  if (wmax <= 0) wmax = 1.0;    /* get maximum weight (if available) */
  for (; *sel; sel++, vals++) { /* traverse the information selectors */
    switch (*sel) {             /* and evaluate them */
      case 'i': *vals = (double)rep->cnt; break;
      case 'n': *vals =      supp;        break;
      case 'd': *vals =      supp;        break;
      case 'a': *vals =      supp;        break;
      case 's': *vals =      supp/smax;   break;
      case 'S': *vals = 100*(supp/smax);  break;
      case 'x': *vals =      supp/smax;   break;
      case 'X': *vals = 100*(supp/smax);  break;
      case 'w': *vals =      wgt;         break;
      case 'W': *vals = 100* wgt;         break;
      case 'r': *vals =      wgt /wmax;   break;
      case 'R': *vals = 100*(wgt /wmax);  break;
      case 'z': *vals = 100*(wgt *smax);  break;
      case 'e': *vals =      rep->eval;   break;
      case 'E': *vals = 100* rep->eval;   break;
      case 'p': *vals =      rep->eval;   break;
      case 'P': *vals = 100* rep->eval;   break;
      default : *vals =   0;              break;
    }                           /* store the corresponding value */
  }                             /* in the output vector */
}  /* isr_getinfo() */
