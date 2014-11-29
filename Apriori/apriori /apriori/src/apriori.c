/*----------------------------------------------------------------------
  File    : apriori.c
  Contents: apriori algorithm for finding association rules
  Author  : Christian Borgelt
  History : 1996.02.14 file created
            1996.07.26 output precision reduced
            1996.11.22 options -b, -f, and -r added
            1996.11.24 option -e added (add. evaluation measures)
            1997.08.18 normalized chi^2 measure and option -m added
            1997.10.13 quiet version (no output to stdout or stderr)
            1998.01.27 adapted to changed ist_create() function
            1998.08.08 optional input file (item appearances) added
            1998.09.07 hyperedge mode added (option -h)
            1998.12.08 output of absolute support added (option -a)
            1998.12.09 conversion of names to a scanable form added
            1999.02.09 input from stdin, output to stdout added
            1999.08.09 bug in check of support parameter fixed (<= 0)
            1999.11.05 rule evaluation measure IST_LIFT_DIFF added
            1999.11.08 output of add. rule eval. measure value added
            2000.03.16 optional use of original rule support definition
            2001.04.01 option -h replaced by option -t (target type)
            2001.05.26 extended support output added
            2001.08.15 module scan used for item name formatting
            2001.11.18 item and transaction functions made a module
            2001.11.19 options -C, -l changed, option -y removed
            2001.12.28 adapted to module tract, some improvements
            2002.01.11 evaluation measures codes changed to letters
            2002.02.10 option -q extended by a direction parameter
            2002.02.11 memory usage minimization option added
            2002.06.09 arbitrary supp./conf. formats made possible
            2003.01.09 option -k added (user-defined item separator)
            2003.01.14 check for empty transaction set added
            2003.07.17 item filtering w.r.t. usage added (option -u)
            2003.07.17 sorting w.r.t. transaction size sum added
            2003.07.18 maximal item set filter added
            2003.08.11 closed  item set filter added
            2003.08.15 item filtering for transaction tree added
            2003.08.16 parameter for transaction filtering added
            2003.08.18 dynamic filtering decision based on times added
            2004.03.25 option -S added (maximal support of a set/rule)
            2004.05.09 additional selection measures for sets added
            2004.10.28 two unnecessary assignments removed
            2004.11.23 absolute/relative support output changed
            2005.01.25 bug in output of absolute/relative support fixed
            2005.01.31 another bug in this output fixed
            2005.06.20 use of flag for "no item sorting" corrected
            2007.02.13 adapted to modified module tabread
            2008.08.12 option -l removed (do not load transactions)
            2008.08.14 adapted to extension of function tbg_filter()
            2008.08.18 more flexible output format control (numbers)
            2008.09.01 option -I added (implication sign for rules)
            2008.09.07 optional a-posteriori pruning added
            2008.09.09 more flexible information output control
            2008.09.10 item set extraction and evaluation redesigned
            2008.10.30 adapted to changes in item set reporting
            2008.11.13 adapted to changes in transaction management
            2008.11.19 adapted to modified transaction tree interface
            2008.12.06 perfect extension pruning added (optional)
            2009.05.28 adapted to modified function tbg_filter()
            2010.03.02 bug concerning size-sorted output fixed
            2010.03.04 several additional evaluation measures added
            2010.06.18 filtering for increase of evaluation added (-i)
            2010.07.14 output file made optional (for benchmarking)
            2010.08.22 adapted to modified modules tract and tabread
            2010.08.30 Fisher's exact test added as evaluation measure
            2010.10.15 adapted to modified interface of module report
            2010.10.22 chi^2 measure with Yates correction added
            2010.11.24 adapted to modified error reporting (tract)
            2010.12.11 adapted to a generic error reporting function
            2011.03.20 optional integer transaction weights added
            2011.07.08 adapted to modified function tbg_recode()
            2011.07.21 bug in function apriori fixed (closed/maximal)
            2011.07.25 threshold inverted for measures yielding p-values
            2011.08.04 weak forward pruning with evaluation added
            2011.08.16 new target type 'generators' added (option -tg)
            2011.08.28 output of item set counters per size added
            2011.10.18 all processing shifted to apriori function
            2012.01.23 rounding the mininum support for mining fixed
            2012.04.30 bug in apriori() fixed ("rule(s)" output)
            2012.06.13 bug in apriori() fixed (IST_INVBXS in eval)
            2012.11.29 bug in apriori() fixed (perf. ext. and ISR_RULE)
            2013.03.30 adapted to type changes in module tract
            2013.06.03 real-valued support made possible (SUPP==double)
            2013.10.18 optional pattern spectrum collection added
            2013.11.12 item selection file changed to option -R#
------------------------------------------------------------------------
  Reference for the Apriori algorithm:
    R. Agrawal and R. Srikant.
    Fast Algorithms for Mining Association Rules.
    Proc. 20th Int. Conf. on Very Large Databases
    (VLDB 1994, Santiago de Chile), 487-499.
    Morgan Kaufmann, San Mateo, CA, USA 1994
----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#ifndef ISR_PATSPEC
#define ISR_PATSPEC
#endif
#ifdef  APR_MAIN
#ifndef PSP_REPORT
#define PSP_REPORT
#endif
#ifndef TA_READ
#define TA_READ
#endif
#endif
#include "apriori.h"
#ifdef APR_MAIN
#include "error.h"
#endif
#ifdef STORAGE
#include "storage.h"
#endif

#ifndef INFINITY
#define INFINITY    (DBL_MAX+DBL_MAX)
#endif                          /* MSC still does not support C99 */

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define PRGNAME     "apriori"
#define DESCRIPTION "find frequent item sets with the apriori algorithm"
#define VERSION     "version 6.6 (2014.01.08)         " \
                    "(c) 1996-2014   Christian Borgelt"

/* --- error codes --- */
/* error codes   0 to  -4 defined in tract.h */
#define E_STDIN      (-5)       /* double assignment of stdin */
#define E_OPTION     (-6)       /* unknown option */
#define E_OPTARG     (-7)       /* missing option argument */
#define E_ARGCNT     (-8)       /* too few/many arguments */
#define E_TARGET     (-9)       /* invalid target type */
#define E_SIZE      (-10)       /* invalid set/rule size */
#define E_SUPPORT   (-11)       /* invalid support */
#define E_CONF      (-12)       /* invalid confidence */
#define E_MEASURE   (-13)       /* invalid evaluation measure */
#define E_AGGMODE   (-14)       /* invalid aggregation mode */
#define E_STAT      (-16)       /* invalid test statistic */
#define E_SIGLVL    (-17)       /* invalid significance level */
/* error codes -15 to -25 defined in tract.h */

#ifndef QUIET                   /* if not quiet version, */
#define MSG         fprintf     /* print messages */
#define XMSG        if (mode & APR_VERBOSE) fprintf
#else                           /* if quiet version, */
#define MSG(...)                /* suppress messages */
#define XMSG(...)
#endif

#define SEC_SINCE(t)  ((double)(clock()-(t)) /(double)CLOCKS_PER_SEC)

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
typedef struct {                /* --- apriori execution data --- */
  int    mode;                  /* processing mode */
  TATREE *tatree;               /* transaction tree */
  ISTREE *istree;               /* item set tree (for counting) */
  ITEM   *map;                  /* identifier map for filtering */
} APRIORI;                      /* (apriori execution data) */

/*----------------------------------------------------------------------
  Constants
----------------------------------------------------------------------*/
#if !defined QUIET && defined APR_MAIN
/* --- error messages --- */
static const char *errmsgs[] = {
  /* E_NONE      0 */  "no error",
  /* E_NOMEM    -1 */  "not enough memory",
  /* E_FOPEN    -2 */  "cannot open file %s",
  /* E_FREAD    -3 */  "read error on file %s",
  /* E_FWRITE   -4 */  "write error on file %s",
  /* E_STDIN    -5 */  "double assignment of standard input",
  /* E_OPTION   -6 */  "unknown option -%c",
  /* E_OPTARG   -7 */  "missing option argument",
  /* E_ARGCNT   -8 */  "wrong number of arguments",
  /* E_TARGET   -9 */  "invalid target type '%c'",
  /* E_SIZE    -10 */  "invalid item set or rule size %"ITEM_FMT,
  /* E_SUPPORT -11 */  "invalid minimum support %g",
  /* E_CONF    -12 */  "invalid minimum confidence %g",
  /* E_MEASURE -13 */  "invalid evaluation measure '%c'",
  /* E_AGGMODE -14 */  "invalid aggregation mode '%c'",
  /* E_NOITEMS -15 */  "no (frequent) items found",
  /* E_STAT    -16 */  "invalid test statistic '%c'",
  /* E_SIGLVL  -17 */  "invalid significance level/p-value %g",
  /*           -18 */  "unknown error"
};
#endif

/*----------------------------------------------------------------------
  Global Variables
----------------------------------------------------------------------*/
#ifdef APR_MAIN
#ifndef QUIET
static CCHAR    *prgname;       /* program name for error messages */
#endif
static TABREAD  *tread  = NULL; /* table/transaction reader */
static ITEMBASE *ibase  = NULL; /* item base */
static TABAG    *tabag  = NULL; /* transaction bag/multiset */
static ISREPORT *report = NULL; /* item set reporter */
static TABWRITE *twrite = NULL; /* table writer for pattern spectrum */
#endif

/*----------------------------------------------------------------------
  Apriori Algorithm (with plain transactions)
----------------------------------------------------------------------*/

static int cleanup (APRIORI *data)
{                               /* --- clean up on error */
  if (!(data->mode & APR_NOCLEAN)) {
    if (data->map)    free(data->map);
    if (data->istree) ist_delete(data->istree);
    if (data->tatree) tat_delete(data->tatree, 0);
  }                             /* free all allocated memory */
  return -1;                    /* return an error indicator */
}  /* cleanup() */

/*--------------------------------------------------------------------*/

int apriori (TABAG *tabag, int target, int mode, SUPP supp, SUPP smax,
             double conf, int eval, int agg, double thresh,
             double minimp, ITEM prune, double filter, int dir,
             ISREPORT *report)
{                               /* --- apriori algorithm */
  int     r = 0;                /* error status */
  ITEM    i, k, n;              /* loop variables, buffers */
  ITEM    size, max;            /* current/maximal item set size */
  SUPP    frq, body, head;      /* frequency of an item set */
  clock_t t, tt, tc, x;         /* timers for measurements */
  APRIORI a = { 0, NULL, NULL, NULL };  /* apriori execution data */

  assert(tabag && report);      /* check the function arguments */
  a.mode = mode;                /* note the processing mode */

  /* --- create transaction tree --- */
  tt = 0;                       /* init. the tree construction time */
  if (mode & APR_TATREE) {      /* if to use a transaction tree */
    t = clock();                /* start the timer for construction */
    XMSG(stderr, "building transaction tree ... ");
    a.tatree = tat_create(tabag);  /* create a transaction tree */
    if (!a.tatree) return cleanup(&a);
    XMSG(stderr, "[%"SIZE_FMT" node(s)]", tat_size(a.tatree));
    XMSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));
    tt = clock() -t;            /* note the time for the construction */
  }                             /* of the transaction tree */

  /* --- create item set tree --- */
  if ((target & (ISR_CLOSED|ISR_MAXIMAL|ISR_RULE))
  ||  (((k = eval & ~IST_INVBXS) > RE_NONE) && (k < IST_LDRATIO))
  ||  dir)                      /* if individual counters needed, */
    mode &= ~IST_PERFECT;       /* remove perfect extension pruning */
  t = clock(); tc = 0;          /* start the timer for the search */
  a.istree = ist_create(tbg_base(tabag), mode, supp, smax, conf);
  if (!a.istree) return cleanup(&a);
  max = isr_max(report);        /* create an apriori item set tree */
  if ((k = tbg_max(tabag)) < max) max = k;
  ist_setsize(a.istree, isr_min(report), max, dir);
  if ((eval & ~IST_INVBXS) <= RE_NONE) prune = ITEM_MIN;
  ist_seteval(a.istree, eval, agg, thresh, minimp, prune);
  eval &= ~IST_INVBXS;          /* configure apriori item set tree */

  /* --- check item subsets --- */
  XMSG(stderr, "checking subsets of size 1");
  n = tbg_itemcnt(tabag);       /* create an item filter map */
  a.map = (ITEM*)malloc((size_t)n *sizeof(ITEM));
  if (!a.map) return cleanup(&a);
  for (i = n; 1; ) {            /* traverse the item set sizes */
    size = ist_height(a.istree);/* get the current item set size */
    if (size >= max) break;     /* abort if maximal size is reached */
    if ((filter != 0)           /* if to filter w.r.t. item usage */
    && ((i = ist_check(a.istree, (int*)a.map)) <= size))
      break;                    /* check which items are still used */
    if (mode & APR_POST)        /* if a-posteriori pruning requested, */
      ist_prune(a.istree);      /* prune infrequent item sets */
    k = ist_addlvl(a.istree);   /* add a level to the item set tree */
    if (k < 0) return cleanup(&a);
    if (k > 0) break;           /* if no level was added, abort */
    if (((filter < 0)           /* if to filter w.r.t. item usage and */
    &&   ((double)i < -filter *(double)n))    /* enough items removed */
    ||  ((filter > 0) && (i < n)/* or counting time is long enough */
    && ((double)i*(double)tt < (double)filter*(double)n*(double)tc))) {
      n = i;                    /* note the new number of items */
      x = clock();              /* start the timer for filtering */
      if (a.tatree) {           /* if a transaction tree was created */
        if (tat_filter(a.tatree, size+1, (int*)a.map, 0) != 0)
          return cleanup(&a); } /* filter the transaction tree */
      else {                    /* if there is only a transaction bag */
        tbg_filter(tabag, size+1, (int*)a.map, 0);
        tbg_sort  (tabag, 0,0); /* remove unnecessary items and */
        tbg_reduce(tabag, 0);   /* transactions and reduce */
      }                         /* transactions to unique ones */
      tt = clock() -x;          /* note the filter/rebuild time */
    }
    ++size;                     /* increment the item set size */
    XMSG(stderr, " %"ITEM_FMT, size);          /* and print it */
    x = clock();                /* start the timer for counting */
    if (a.tatree) ist_countx(a.istree, a.tatree);
    else          ist_countb(a.istree, tabag);
    ist_commit(a.istree);       /* count the transaction tree/bag */
    tc = clock() -x;            /* compute the new counting time */
  }
  free(a.map); a.map = NULL;    /* delete filter map and trans. tree */
  if (!(mode & APR_NOCLEAN) && a.tatree) {
    tat_delete(a.tatree, 0); a.tatree = NULL; }
  XMSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));

  /* --- filter found item sets --- */
  if ((prune >  ITEM_MIN)       /* if to filter with evaluation */
  &&  (prune <= 0)) {           /* (backward and weak forward) */
    t = clock();                /* start the timer for filtering */
    XMSG(stderr, "filtering with evaluation ... ");
    ist_filter(a.istree,prune); /* mark non-qualifying item sets */
    XMSG(stderr, "done [%.2fs].\n", SEC_SINCE(t));
  }                             /* filter with evaluation */
  if (target & (ISR_CLOSED|ISR_MAXIMAL|ISR_GENERA)) {
    t = clock();                /* start the timer for filtering */
    XMSG(stderr, "filtering for %s item sets ... ",
         (target == ISR_GENERA) ? "generator" :
         (target == ISR_CLOSED) ? "closed" : "maximal");
    ist_clomax(a.istree, target | ((prune > ITEM_MIN) ? IST_SAFE : 0));
    XMSG(stderr, "done [%.2fs].\n", SEC_SINCE(t));
  }                             /* filter closed/maximal/generators */

  /* --- report item sets/rules --- */
  t = clock();                  /* start the output timer */
  XMSG(stderr, "writing %s ... ", isr_name(report));
  ist_init(a.istree);           /* initialize the extraction */
  if (target == ISR_RULE) {     /* if to find association rules */
    a.map = (ITEM*)malloc((size_t)(size+1) *sizeof(ITEM));
    if (!a.map) return cleanup(&a);  /* create an item set buffer */
    while (1) {                 /* extract assoc. rules from tree */
      k = ist_rule(a.istree, a.map, &frq, &body, &head, &thresh);
      if (k < 0) break;         /* get the next association rule */
      r = isr_rule(report, a.map, k, frq, body, head, thresh);
      if (r < 0) break;         /* report the extracted ass. rule */
    }
    free(a.map); a.map = NULL;} /* delete the item set buffer */
  else if (dir) {               /* if to find frequent item sets */
    a.map = (ITEM*)malloc((size_t)(size+1) *sizeof(ITEM));
    if (!a.map) return cleanup(&a);   /* create an item set buffer */
    while (1) {                 /* extract item sets from the tree */
      k = ist_set(a.istree, a.map, &frq, &thresh);
      if (k < 0) break;         /* get the next frequent item set */
      r = isr_direct(report, a.map, k, frq, thresh, thresh);
      if (r < 0) break;         /* report the extracted item set */
    }
    free(a.map); a.map = NULL;} /* delete the item set buffer */
  else {                        /* if not to sort item sets by size */
    if     ((eval == IST_LDRATIO)  /* if to compute add. evaluation */
    &&     (minimp <= -INFINITY))  /* but no min. improvement req. */
      isr_seteval(report, isr_logrto,  NULL,   +1,           thresh);
    else if (eval >  IST_NONE)  /* set the add. evaluation function */
      isr_seteval(report, ist_evalx, a.istree, re_dir(eval), thresh);
    if (ist_report(a.istree, report) < 0)
      r = -1;                   /* recursively report item sets */
  }  /* if (target == ISR_RULE) .. else if (dir) .. else .. */
  XMSG(stderr, "[%"SIZE_FMT" %s(s)]", isr_repcnt(report),
               (target == ISR_RULE) ? "rule" : "set");
  XMSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));
  #ifdef BENCH                  /* if benchmark version, */
  ist_stats(a.istree);          /* show the search statistics */
  #endif                        /* (especially memory usage) */
  if (!(mode & APR_NOCLEAN)) {  /* delete the apriori item set tree */
    ist_delete(a.istree); a.istree = NULL; }
  return r;                     /* return the error status */
}  /* apriori() */

/*----------------------------------------------------------------------
  Main Functions
----------------------------------------------------------------------*/
#ifdef APR_MAIN
#ifdef APRIACC

static void help (void)
{                               /* --- print add. option information */
  #ifndef QUIET
  fprintf(stderr, "\n");        /* terminate startup message */
  printf("test statistics for p-value computation (option -t#)\n");
  printf("  x      no statistic / zero\n");
  printf("  c/p/n  chi^2 measure (default)\n");
  printf("  y/t    chi^2 measure with Yates' correction\n");
  printf("  i/g    mutual information / G statistic\n");
  printf("  f      Fisher's exact test (table probability)\n");
  printf("  h      Fisher's exact test (chi^2 measure)\n");
  printf("  m      Fisher's exact test (mutual information)\n");
  printf("  s      Fisher's exact test (support)\n");
  printf("\n");
  printf("information output format characters (option -v#)\n");
  printf("  %%%%    a percent sign\n");
  printf("  %%i    number of items (item set size)\n");
  printf("  %%a    absolute item set support\n");
  printf("  %%s    relative item set support as a fraction\n");
  printf("  %%S    relative item set support as a percentage\n");
  printf("  %%p    p-value of item set test as a fraction\n");
  printf("  %%P    p-value of item set test as a percentage\n");
  printf("All format characters can be preceded by the number\n");
  printf("of significant digits to be printed (at most 32 digits),\n");
  printf("even though this value is ignored for integer numbers.\n");
  #endif                        /* print help information */
  exit(0);                      /* abort the program */
}  /* help() */

/*--------------------------------------------------------------------*/
#else

static void help (void)
{                               /* --- print add. option information */
  #ifndef QUIET
  fprintf(stderr, "\n");        /* terminate startup message */
  printf("additional evaluation measures (option -e#)\n");
  printf("frequent item sets:\n");
  printf("  x   no measure (default)\n");
  printf("  b   binary logarithm of support quotient            (+)\n");
  printf("association rules:\n");
  printf("  x   no measure (default)\n");
  printf("  o   rule support (original def.: body & head)       (+)\n");
  printf("  c   rule confidence                                 (+)\n");
  printf("  d   absolute confidence difference to prior         (+)\n");
  printf("  l   lift value (confidence divided by prior)        (+)\n");
  printf("  a   absolute difference of lift value to 1          (+)\n");
  printf("  q   difference of lift quotient to 1                (+)\n");
  printf("  v   conviction (inverse lift for negated head)      (+)\n");
  printf("  e   absolute difference of conviction to 1          (+)\n");
  printf("  r   difference of conviction quotient to 1          (+)\n");
  printf("  z   certainty factor (relative confidence change)   (+)\n");
  printf("  n   normalized chi^2 measure                        (+)\n");
  printf("  p   p-value from (unnormalized) chi^2 measure       (-)\n");
  printf("  y   normalized chi^2 measure with Yates' correction (+)\n");
  printf("  t   p-value from Yates-corrected chi^2 measure      (-)\n");
  printf("  i   information difference to prior                 (+)\n");
  printf("  g   p-value from G statistic/information difference (-)\n");
  printf("  f   Fisher's exact test (table probability)         (-)\n");
  printf("  h   Fisher's exact test (chi^2 measure)             (-)\n");
  printf("  m   Fisher's exact test (information gain)          (-)\n");
  printf("  s   Fisher's exact test (support)                   (-)\n");
  printf("All measures for association rules are also applicable\n");
  printf("to item sets and are then aggregated over all possible\n");
  printf("association rules with a single item in the consequent.\n");
  printf("The aggregation mode can be set with the option -a#.\n");
  printf("Measures marked with (+) must meet or exceed the threshold,\n");
  printf("measures marked with (-) must not exceed the threshold\n");
  printf("in order for the rule or item set to be reported.\n");
  printf("\n");
  printf("evaluation measure aggregation modes (option -a#)\n");
  printf("  x   no aggregation (use first value)\n");
  printf("  m   minimum of individual measure values\n");
  printf("  n   maximum of individual measure values\n");
  printf("  a   average of individual measure values\n");
  printf("  s   split item set into equal size subsets\n");
  printf("\n");
  printf("information output format characters (option -v#)\n");
  printf("  %%%%  a percent sign\n");
  printf("  %%i  number of items (item set size)\n");
  printf("  %%a  absolute item set  support\n");
  printf("  %%s  relative item set  support as a fraction\n");
  printf("  %%S  relative item set  support as a percentage\n");
  printf("  %%b  absolute body set  support\n");
  printf("  %%x  relative body set  support as a fraction\n");
  printf("  %%X  relative body set  support as a percentage\n");
  printf("  %%h  absolute head item support\n");
  printf("  %%y  relative head item support as a fraction\n");
  printf("  %%Y  relative head item support as a percentage\n");
  printf("  %%c  rule confidence as a fraction\n");
  printf("  %%C  rule confidence as a percentage\n");
  printf("  %%l  lift value of a rule (confidence/prior)\n");
  printf("  %%L  lift value of a rule as a percentage\n");
  printf("  %%e  additional evaluation measure\n");
  printf("  %%E  additional evaluation measure as a percentage\n");
  printf("s,S,x,X,y,Y,c,C,l,L,e,E can be preceded by the number\n");
  printf("of decimal places to be printed (at most 32 places).\n");
  #endif                        /* print help information */
  exit(0);                      /* abort the program */
}  /* help() */

#endif
/*--------------------------------------------------------------------*/

#ifndef NDEBUG                  /* if debug version */
  #undef  CLEANUP               /* clean up memory and close files */
  #define CLEANUP \
  if (twrite) twr_delete(twrite, 1); \
  if (report) isr_delete(report, 0); \
  if (tabag)  tbg_delete(tabag,  0); \
  if (tread)  trd_delete(tread,  1); \
  if (ibase)  ib_delete (ibase);
#endif

GENERROR(error, exit)           /* generic error reporting function */

/*--------------------------------------------------------------------*/
#ifdef APRIACC

int main (int argc, char *argv[])
{                               /* --- main function */
  int     i, k = 0;             /* loop variables, counters */
  char    *s;                   /* to traverse the options */
  CCHAR   **optarg = NULL;      /* option argument */
  CCHAR   *fn_inp  = NULL;      /* name of input  file */
  CCHAR   *fn_out  = NULL;      /* name of output file */
  CCHAR   *fn_sel  = NULL;      /* name of item selection file */
  CCHAR   *fn_psp  = NULL;      /* name of pattern spectrum file */
  CCHAR   *recseps = NULL;      /* record  separators */
  CCHAR   *fldseps = NULL;      /* field   separators */
  CCHAR   *blanks  = NULL;      /* blank   characters */
  CCHAR   *comment = NULL;      /* comment characters */
  CCHAR   *hdr     = "";        /* record header  for output */
  CCHAR   *sep     = " ";       /* item separator for output */
  CCHAR   *dflt    = " (%a,%4P)";    /* default format for check */
  CCHAR   *format  = dflt;      /* format for information output */
  ITEM    min      =  2;        /* minimum size of an item set */
  ITEM    max      = ITEM_MAX;  /* maximum size of an item set */
  double  supp     = -2;        /* minimum support (percent/absolute) */
  int     stat     = 'p';       /* test statistic to use */
  double  siglvl   =  1;        /* significance level (in percent) */
  int     invbxs   =  0;        /* invalidate eval. below expectation */
  ITEM    prune    =  0;        /* (min. size for) eval. filtering */
  int     sort     =  2;        /* flag for item sorting and recoding */
  int     mode     = APP_BOTH|IST_PERFECT|APR_TATREE;  /* search mode */
  int     mtar     =  0;        /* mode for transaction weights */
  int     mrep     =  0;        /* mode for item set reporting */
  int     stats    =  0;        /* flag for item set statistics */
  PATSPEC *psp;                 /* collected pattern spectrum */
  ITEM    m;                    /* number of items */
  TID     n;                    /* number of transactions */
  SUPP    w;                    /* total transaction weight */
  clock_t t;                    /* timer for measurements */

  #ifndef QUIET                 /* if not quiet version */
  prgname = argv[0];            /* get program name for error msgs. */

  /* --- print usage message --- */
  if (argc > 1) {               /* if arguments are given */
    fprintf(stderr, "%s - %s\n", argv[0], DESCRIPTION);
    fprintf(stderr, VERSION); } /* print a startup message */
  else {                        /* if no arguments given */
    printf("usage: %s [options] infile [outfile]\n", argv[0]);
    printf("%s\n", DESCRIPTION);
    printf("%s\n", VERSION);
    printf("-m#      minimum number of items per item set     "
                    "(default: %"ITEM_FMT")\n", min);
    printf("-n#      maximum number of items per item set     "
                    "(default: no limit)\n");
    printf("-s#      minimum support of an item set           "
                    "(default: %g)\n", supp);
    printf("         (positive: percentage, "
                     "negative: absolute number)\n");
    printf("-e#      test statistic for item set evaluation   "
                    "(default: '%c')\n", stat);
    printf("-d#      significance level (maximum p-value)     "
                    "(default: %g%%)\n", siglvl);
    printf("-z       invalidate stat. below expected support  "
                    "(default: evaluate all)\n");
    printf("-p#      minimum set size for subset filtering    "
                    "(default: 0)\n");
    printf("         (0: backward filtering       "
                        "(no subset checks),\n");
    printf("         <0: weak   forward filtering "
                        "(one subset  must qualify),\n");
    printf("         >0: strong forward filtering "
                        "(all subsets must qualify))\n");
    printf("-q#      sort items w.r.t. their frequency        "
                    "(default: %d)\n", sort);
    printf("         (1: ascending, -1: descending, 0: do not sort,\n"
           "          2: ascending, -2: descending w.r.t. "
                    "transaction size sum)\n");
    printf("-R#      read an item selection from a file\n");
    printf("-P#      write a pattern spectrum to a file\n");
    printf("-Z       print item set statistics "
                    "(number of item sets per size)\n");
    printf("-g       write output in scanable form "
                    "(quote certain characters)\n");
    printf("-h#      record header  for output                "
                    "(default: \"%s\")\n", hdr);
    printf("-k#      item separator for output                "
                    "(default: \"%s\")\n", sep);
    printf("-v#      output format for item set information   "
                    "(default: \"%s\")\n", format);
    printf("-w       integer transaction weight in last field "
                    "(default: only items)\n");
    printf("-r#      record/transaction separators            "
                    "(default: \"\\n\")\n");
    printf("-f#      field /item        separators            "
                    "(default: \" \\t,\")\n");
    printf("-b#      blank   characters                       "
                    "(default: \" \\t\\r\")\n");
    printf("-C#      comment characters                       "
                    "(default: \"#\")\n");
    printf("-!       print additional option information\n");
    printf("infile   file to read transactions from           "
                    "[required]\n");
    printf("outfile  file to write found item sets to         "
                    "[optional]\n");
    return 0;                   /* print a usage message */
  }                             /* and abort the program */
  #endif  /* #ifndef QUIET */
  /* free option characters: acijloptuxy [A-Z]\[CPRZ] */

  /* --- evaluate arguments --- */
  for (i = 1; i < argc; i++) {  /* traverse arguments */
    s = argv[i];                /* get option argument */
    if (optarg) { *optarg = s; optarg = NULL; continue; }
    if ((*s == '-') && *++s) {  /* -- if argument is an option */
      while (*s) {              /* traverse options */
        switch (*s++) {         /* evaluate switches */
          case '!': help();                          break;
          case 'm': min    = (ITEM)strtol(s, &s, 0); break;
          case 'n': max    = (ITEM)strtol(s, &s, 0); break;
          case 's': supp   =      strtod(s, &s);     break;
          case 'e': stat   = (*s) ? *s++ : 'x';      break;
          case 'd': siglvl =      strtod(s, &s);     break;
          case 'z': invbxs = IST_INVBXS;             break;
          case 'p': prune  = (ITEM)strtol(s, &s, 0); break;
          case 'q': sort   = (int) strtol(s, &s, 0); break;
          case 'R': optarg = &fn_sel;                break;
          case 'P': optarg = &fn_psp;                break;
          case 'Z': stats  = 1;                      break;
          case 'g': mrep   = ISR_SCAN;               break;
          case 'h': optarg = &hdr;                   break;
          case 'k': optarg = &sep;                   break;
          case 'v': optarg = &format;                break;
          case 'w': mtar  |= TA_WEIGHT;              break;
          case 'r': optarg = &recseps;               break;
          case 'f': optarg = &fldseps;               break;
          case 'b': optarg = &blanks;                break;
          case 'C': optarg = &comment;               break;
          default : error(E_OPTION, *--s);           break;
        }                       /* set option variables */
        if (optarg && *s) { *optarg = s; optarg = NULL; break; }
      } }                       /* get option argument */
    else {                      /* -- if argument is no option */
      switch (k++) {            /* evaluate non-options */
        case  0: fn_inp = s;      break;
        case  1: fn_out = s;      break;
        default: error(E_ARGCNT); break;
      }                         /* note filenames */
    }
  }
  if (optarg)       error(E_OPTARG);    /* check (option) arguments */
  if (k      < 1)   error(E_ARGCNT);    /* and number of arguments */
  if (min    < 0)   error(E_SIZE, min); /* check the size limits */
  if (max    < 0)   error(E_SIZE, max); /* and the minimum support */
  if (supp   > 100) error(E_SUPPORT, supp);
  if (siglvl > 100) error(E_SIGLVL, siglvl);
  if ((!fn_inp || !*fn_inp) && (fn_sel && !*fn_sel))
    error(E_STDIN);             /* stdin must not be used twice */
  switch (stat) {               /* check and translate evaluation */
    case 'x': stat = RE_NONE;            break;
    case 'c': stat = RE_CHI2PVAL;        break;
    case 'p': stat = RE_CHI2PVAL;        break;
    case 'n': stat = RE_CHI2PVAL;        break;
    case 'y': stat = RE_YATESPVAL;       break;
    case 't': stat = RE_YATESPVAL;       break;
    case 'i': stat = RE_INFOPVAL;        break;
    case 'g': stat = RE_INFOPVAL;        break;
    case 'f': stat = RE_FETPROB;         break;
    case 'h': stat = RE_FETCHI2;         break;
    case 'm': stat = RE_FETINFO;         break;
    case 's': stat = RE_FETSUPP;         break;
    default : error(E_STAT, (char)stat); break;
  }                             /* (get target type code) */
  if ((format == dflt) && (supp >= 0))
    format = "  (%3S,%4P)";     /* adapt the default info. format */
  MSG(stderr, "\n");            /* terminate the startup message */

  /* --- read item selection --- */
  ibase = ib_create(0, 0);      /* create an item base */
  if (!ibase) error(E_NOMEM);   /* to manage the items */
  tread = trd_create();         /* create a transaction reader */
  if (!tread) error(E_NOMEM);   /* and configure the characters */
  trd_allchs(tread, recseps, fldseps, blanks, "", comment);
  if (fn_sel) {                 /* if item appearances are given */
    t = clock();                /* start timer, open input file */
    if (trd_open(tread, NULL, fn_sel) != 0)
      error(E_FOPEN, trd_name(tread));
    MSG(stderr, "reading %s ... ", trd_name(tread));
    m = ib_readsel(ibase,tread);/* read the given item selection */
    if (m < 0) error((int)-m, ib_errmsg(ibase, NULL, 0));
    trd_close(tread);           /* close the input file */
    MSG(stderr, "[%"ITEM_FMT" item(s)]", m);
    MSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));
  }                             /* print a log message */

  /* --- read transaction database --- */
  tabag = tbg_create(ibase);    /* create a transaction bag */
  if (!tabag) error(E_NOMEM);   /* to store the transactions */
  t = clock();                  /* start timer, open input file */
  if (trd_open(tread, NULL, fn_inp) != 0)
    error(E_FOPEN, trd_name(tread));
  MSG(stderr, "reading %s ... ", trd_name(tread));
  k = tbg_read(tabag, tread, mtar);
  if (k < 0)                    /* read the transaction database */
    error(-k, tbg_errmsg(tabag, NULL, 0));
  trd_delete(tread, 1);         /* close the input file and */
  tread = NULL;                 /* delete the table reader */
  m = ib_cnt(ibase);            /* get the number of items, */
  n = tbg_cnt(tabag);           /* the number of transactions, */
  w = tbg_wgt(tabag);           /* the total transaction weight */
  MSG(stderr, "[%"ITEM_FMT" item(s), %"TID_FMT, m, n);
  if (w != (SUPP)n) MSG(stderr, "/%"SUPP_FMT, w);
  MSG(stderr, " transaction(s)] done [%.2fs].", SEC_SINCE(t));
  if ((m <= 0) || (n <= 0))     /* check for at least one item */
    error(E_NOITEMS);           /* and at least one transaction */
  MSG(stderr, "\n");            /* compute absolute support value */
  supp = ceilsupp((supp >= 0) ? 0.01 *supp *(double)w : -supp);
  siglvl *= 0.01;               /* turn sig. level into fraction */

  /* --- sort and recode items --- */
  t = clock();                  /* start timer, print log message */
  MSG(stderr, "filtering, sorting and recoding items ... ");
  m = tbg_recode(tabag, (SUPP)supp, -1, -1, sort);
  if (m <  0) error(E_NOMEM);   /* recode items and transactions */
  if (m <= 0) error(E_NOITEMS); /* and check the number of items */
  MSG(stderr, "[%"ITEM_FMT" item(s)]", m);
  MSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));

  /* --- sort and reduce transactions --- */
  t = clock();                  /* start timer, print log message */
  MSG(stderr, "sorting and reducing transactions ... ");
  tbg_itsort(tabag, +1, 0);     /* sort items in transactions and */
  tbg_sort  (tabag, +1, 0);     /* sort the trans. lexicographically */
  n = tbg_reduce(tabag, 0);     /* reduce transactions to unique ones */
  MSG(stderr, "[%"TID_FMT, n);  /* print number of transactions */
  if (w != (SUPP)n) MSG(stderr, "/%"SUPP_FMT, w);
  MSG(stderr, " transaction(s)] done [%.2fs].\n", SEC_SINCE(t));

  /* --- find frequent item sets --- */
  t = clock();                  /* start the timer */
  report = isr_create(ibase, mrep, -1, hdr, sep, NULL);
  if (!report) error(E_NOMEM);  /* create an item set reporter */
  isr_setfmt (report, format);  /* and configure it: set flags, */
  isr_setsize(report, min, max);/* info. format and size range, */
  if (fn_psp && (isr_addpsp(report, NULL) < 0))
    error(E_NOMEM);             /* add a pattern spectrum if req. */
  if (isr_open(report, NULL, fn_out) != 0)
    error(E_FOPEN, isr_name(report));   /* open the output file */
  MSG(stderr, "writing %s ... ", isr_name(report));
  k = apriori(tabag, ISR_MAXIMAL, mode|APR_NOCLEAN,
              (SUPP)supp, (SUPP)w, 100.0, stat|invbxs, IST_MAX,
              siglvl, -INFINITY, prune, 0, 0, report);
  if (k < 0) error(E_NOMEM);    /* search for frequent item sets */
  if (stats)                    /* print item set statistics */
    isr_prstats(report, stdout, 0);
  if (isr_close(report) != 0)   /* close the output file */
    error(E_FWRITE, isr_name(report));
  MSG(stderr, "[%"SIZE_FMT" set(s)]", isr_repcnt(report));
  MSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));

  /* --- write pattern spectrum --- */
  if (fn_psp) {                 /* if to write a pattern spectrum */
    psp    = isr_getpsp(report);/* get the pattern spectrum */
    twrite = twr_create();      /* create a table writer and */
    if (!twrite) error(E_NOMEM);/* open the output file */
    if (twr_open(twrite, NULL, fn_psp) != 0)
      error(E_FOPEN,  twr_name(twrite));
    if (psp_report(psp, twrite) != 0)
      error(E_FWRITE, twr_name(twrite));
    twr_delete(twrite, 1);      /* write the pattern spectrum, */
    twrite = NULL;              /* delete the table writer, and */
  }                             /* clear the writer variable */

  /* --- clean up --- */
  CLEANUP;                      /* clean up memory and close files */
  SHOWMEM;                      /* show (final) memory usage */
  return 0;                     /* return 'ok' */
}  /* main() */

/*--------------------------------------------------------------------*/
#else

int main (int argc, char *argv[])
{                               /* --- main function */
  int     i, k = 0;             /* loop variables, counters */
  char    *s;                   /* to traverse the options */
  CCHAR   **optarg = NULL;      /* option argument */
  CCHAR   *fn_inp  = NULL;      /* name of input  file */
  CCHAR   *fn_out  = NULL;      /* name of output file */
  CCHAR   *fn_app  = NULL;      /* name of item appearances file */
  CCHAR   *fn_psp  = NULL;      /* name of pattern spectrum file */
  CCHAR   *recseps = NULL;      /* record  separators */
  CCHAR   *fldseps = NULL;      /* field   separators */
  CCHAR   *blanks  = NULL;      /* blank   characters */
  CCHAR   *comment = NULL;      /* comment characters */
  CCHAR   *hdr     = "";        /* record header  for output */
  CCHAR   *sep     = " ";       /* item separator for output */
  CCHAR   *imp     = " <- ";    /* implication sign for ass. rules */
  CCHAR   *dflt    = " (%S)";   /* default format for check */
  CCHAR   *format  = dflt;      /* format for information output */
  int     target   = 's';       /* target type (sets/rules/h.edges) */
  ITEM    min      = 1;         /* minimum rule/item set size */
  ITEM    max      = ITEM_MAX;  /* maximum rule/item set size */
  double  supp     = 10;        /* minimum support    (in percent) */
  double  smax     = 100;       /* maximum support    (in percent) */
  double  conf     = 80;        /* minimum confidence (in percent) */
  int     eval     = 'x';       /* additional evaluation measure */
  int     agg      = 'x';       /* aggregation mode for eval. measure */
  double  thresh   = 10;        /* minimum evaluation measure value */
  double  minimp   = -INFINITY; /* minimum increase of measure value */
  int     invbxs   = 0;         /* invalidate eval. below expectation */
  ITEM    prune    = ITEM_MIN;  /* (min. size for) evaluation pruning */
  int     sort     = 2;         /* flag for item sorting and recoding */
  double  filter   = 0.01;      /* item usage filtering parameter */
  int     mode     = APP_BODY|IST_PERFECT|APR_TATREE;  /* search mode */
  int     dir      = 0;         /* direction for size sorting */
  int     mtar     = 0;         /* mode for transaction reading */
  int     mrep     = 0;         /* mode for item set reporting */
  int     stats    = 0;         /* flag for item set statistics */
  PATSPEC *psp;                 /* collected pattern spectrum */
  ITEM    m;                    /* number of items */
  TID     n;                    /* number of transactions */
  SUPP    w, r;                 /* total transaction weight */
  clock_t t;                    /* timers for measurements */

  #ifndef QUIET                 /* if not quiet version */
  prgname = argv[0];            /* get program name for error msgs. */

  /* --- print usage message --- */
  if (argc > 1) {               /* if arguments are given */
    fprintf(stderr, "%s - %s\n", argv[0], DESCRIPTION);
    fprintf(stderr, VERSION); } /* print a startup message */
  else {                        /* if no arguments are given */
    printf("usage: %s [options] infile [outfile]\n", argv[0]);
    printf("%s\n", DESCRIPTION);
    printf("%s\n", VERSION);
    printf("-t#      target type                              "
                    "(default: %c)\n", target);
    printf("         (s: frequent, c: closed, m: maximal item sets,\n");
    printf("          g: generators, r: association rules)\n");
    printf("-m#      minimum number of items per set/rule     "
                    "(default: %"ITEM_FMT")\n", min);
    printf("-n#      maximum number of items per set/rule     "
                    "(default: no limit)\n");
    printf("-s#      minimum support    of a     set/rule     "
                    "(default: %g%%)\n", supp);
    printf("-S#      maximum support    of a     set/rule     "
                    "(default: %g%%)\n", smax);
    printf("         (positive: percentage, "
                     "negative: absolute number)\n");
    printf("-o       use original rule support definition     "
                    "(body & head)\n");
    printf("-c#      minimum confidence of a     rule         "
                    "(default: %g%%)\n", conf);
    printf("-e#      additional evaluation measure            "
                    "(default: none)\n");
    printf("-a#      aggregation mode for evaluation measure  "
                    "(default: none)\n");
    printf("-d#      threshold for add. evaluation measure    "
                    "(default: %g%%)\n", thresh);
    printf("-i#      least improvement of evaluation measure  "
                    "(default: no limit)\n");
    printf("         (not applicable with evaluation averaging, "
                    "i.e. option -aa)\n");
    printf("-z       invalidate eval. below expected support  "
                    "(default: evaluate all)\n");
    printf("-p#      (min. size for) pruning with evaluation  "
                    "(default: no pruning)\n");
    printf("         (< 0: weak forward, > 0 strong forward, "
                     "= 0: backward pruning)\n");
    printf("-q#      sort items w.r.t. their frequency        "
                    "(default: %d)\n", sort);
    printf("         (1: ascending, -1: descending, 0: do not sort,\n"
           "          2: ascending, -2: descending w.r.t. "
                    "transaction size sum)\n");
    printf("-u#      filter unused items from transactions    "
                    "(default: %g)\n", filter);
    printf("         (0: do not filter items w.r.t. usage in sets,\n"
           "         <0: fraction of removed items for filtering,\n"
           "         >0: take execution times ratio into account)\n");
    printf("-x       do not prune with perfect extensions     "
                    "(default: prune)\n");
    printf("-y       a-posteriori pruning of infrequent item sets\n");
    printf("-T       do not organize transactions as a prefix tree\n");
    printf("-R#      read item selection/appearance indicators\n");
    printf("-P#      write pattern spectrum to a file\n");
    printf("-Z       print item set statistics "
                    "(number of item sets per size)\n");
    printf("-g       write item names in scanable form "
                    "(quote certain characters)\n");
    printf("-h#      record header  for output                "
                    "(default: \"%s\")\n", hdr);
    printf("-k#      item separator for output                "
                    "(default: \"%s\")\n", sep);
    printf("-I#      implication sign for association rules   "
                    "(default: \"%s\")\n", imp);
    printf("-v#      output format for set/rule information   "
                    "(default: \"%s\")\n", format);
    printf("-l#      sort item sets in output by their size   "
                    "(default: no sorting)\n");
    printf("         (< 0: descending, > 0: ascending order)\n");
    printf("-w       integer transaction weight in last field "
                    "(default: only items)\n");
    printf("-r#      record/transaction separators            "
                    "(default: \"\\n\")\n");
    printf("-f#      field /item        separators            "
                    "(default: \" \\t,\")\n");
    printf("-b#      blank   characters                       "
                    "(default: \" \\t\\r\")\n");
    printf("-C#      comment characters                       "
                    "(default: \"#\")\n");
    printf("-!       print additional option information\n");
    printf("infile   file to read transactions from           "
                    "[required]\n");
    printf("outfile  file to write item sets/assoc. rules to  "
                    "[optional]\n");
    return 0;                   /* print a usage message */
  }                             /* and abort the program */
  #endif  /* #ifndef QUIET */
  /* free option characters: j[A-Z]\[CIPST] */

  /* --- evaluate arguments --- */
  for (i = 1; i < argc; i++) {  /* traverse the arguments */
    s = argv[i];                /* get an option argument */
    if (optarg) { *optarg = s; optarg = NULL; continue; }
    if ((*s == '-') && *++s) {  /* -- if argument is an option */
      while (*s) {              /* traverse the options */
        switch (*s++) {         /* evaluate the options */
          case '!': help();                          break;
          case 't': target = (*s) ? *s++ : 's';      break;
          case 'm': min    = (ITEM)strtol(s, &s, 0); break;
          case 'n': max    = (ITEM)strtol(s, &s, 0); break;
          case 's': supp   =       strtod(s, &s);    break;
          case 'S': smax   =       strtod(s, &s);    break;
          case 'o': mode  |= APP_BOTH;               break;
          case 'c': conf   =       strtod(s, &s);    break;
          case 'e': eval   = (*s) ? *s++ : 0;        break;
          case 'a': agg    = (*s) ? *s++ : 0;        break;
          case 'd': thresh =       strtod(s, &s);    break;
          case 'i': minimp =       strtod(s, &s);    break;
          case 'z': invbxs = IST_INVBXS;             break;
          case 'p': prune  = (ITEM)strtol(s, &s, 0); break;
          case 'q': sort   = (int) strtol(s, &s, 0); break;
          case 'u': filter =       strtod(s, &s);    break;
          case 'x': mode  &= ~IST_PERFECT;           break;
          case 'y': mode  |=  APR_POST;              break;
          case 'T': mode  &= ~APR_TATREE;            break;
          case 'R': optarg = &fn_app;                break;
          case 'P': optarg = &fn_psp;                break;
          case 'Z': stats  = 1;                      break;
          case 'g': mrep   = ISR_SCAN;               break;
          case 'h': optarg = &hdr;                   break;
          case 'k': optarg = &sep;                   break;
          case 'I': optarg = &imp;                   break;
          case 'v': optarg = &format;                break;
          case 'l': dir    = (int) strtol(s, &s, 0); break;
          case 'w': mtar  |= TA_WEIGHT;              break;
          case 'r': optarg = &recseps;               break;
          case 'f': optarg = &fldseps;               break;
          case 'b': optarg = &blanks;                break;
          case 'C': optarg = &comment;               break;
          default : error(E_OPTION, *--s);           break;
        }                       /* set the option variables */
        if (optarg && *s) { *optarg = s; optarg = NULL; break; }
      } }                       /* get an option argument */
    else {                      /* -- if argument is no option */
      switch (k++) {            /* evaluate non-options */
        case  0: fn_inp = s;      break;
        case  1: fn_out = s;      break;
        default: error(E_ARGCNT); break;
      }                         /* note filenames */
    }
  }
  if (optarg)     error(E_OPTARG);    /* check (option) arguments */
  if (k    < 1)   error(E_ARGCNT);    /* and number of arguments */
  if (min  < 0)   error(E_SIZE, min); /* check the size limits */
  if (max  < 0)   error(E_SIZE, max); /* and the minimum support */
  if (supp > 100) error(E_SUPPORT, supp);
  if ((conf < 0) || (conf > 100))
    error(E_CONF, conf);        /* check the minimum confidence */
  if ((!fn_inp || !*fn_inp) && (fn_app && !*fn_app))
    error(E_STDIN);             /* stdin must not be used twice */
  switch (target) {             /* check and translate target type */
    case 's': target = ISR_ALL;              break;
    case 'c': target = ISR_CLOSED;           break;
    case 'm': target = ISR_MAXIMAL;          break;
    case 'g': target = ISR_GENERA;           break;
    case 'r': target = ISR_RULE;             break;
    default : error(E_TARGET, (char)target); break;
  }
  switch (eval) {               /* check and translate measure */
    case 'x': eval = RE_NONE;                break;
    case 'o': eval = RE_SUPP;                break;
    case 'c': eval = RE_CONF;                break;
    case 'd': eval = RE_CONFDIFF;            break;
    case 'l': eval = RE_LIFT;                break;
    case 'a': eval = RE_LIFTDIFF;            break;
    case 'q': eval = RE_LIFTQUOT;            break;
    case 'v': eval = RE_CVCT;                break;
    case 'e': eval = RE_CVCTDIFF;            break;
    case 'r': eval = RE_CVCTQUOT;            break;
    case 'z': eval = RE_CERT;                break;
    case 'n': eval = RE_CHI2;                break;
    case 'p': eval = RE_CHI2PVAL;            break;
    case 'y': eval = RE_YATES;               break;
    case 't': eval = RE_YATESPVAL;           break;
    case 'i': eval = RE_INFO;                break;
    case 'g': eval = RE_INFOPVAL;            break;
    case 'f': eval = RE_FETPROB;             break;
    case 'h': eval = RE_FETCHI2;             break;
    case 'm': eval = RE_FETINFO;             break;
    case 's': eval = RE_FETSUPP;             break;
    case 'b': eval = IST_LDRATIO;            break;
    default : error(E_MEASURE, (char)eval);  break;
  }  /* free: j k u w x */
  switch (agg) {                /* check and translate agg. mode */
    case 'x': agg = IST_NONE;                break;
    case 'm': agg = IST_MIN;                 break;
    case 'n': agg = IST_MAX;                 break;
    case 'a': agg = IST_AVG;                 break;
    case 's': agg = IST_EQS;                 break;
    default : error(E_AGGMODE, (char)agg);   break;
  }
  if (eval <= RE_NONE) prune = ITEM_MIN;
  if (target < ISR_RULE) {      /* remove rule specific settings */
    mode |= APP_BOTH; conf = 100; }
  else fn_psp = NULL;           /* no pattern spectrum for rules */
  if ((filter <= -1) || (filter >= 1))
    filter = 0;                 /* check and adapt the filter option */
  if (format == dflt) {         /* if default info. format is used, */
    if (target != ISR_RULE)     /* set default according to target */
         format = (supp < 0) ? " (%a)"     : " (%S)";
    else format = (supp < 0) ? " (%b, %C)" : " (%X, %C)";
  }                             /* select absolute/relative support */
  MSG(stderr, "\n");            /* terminate the startup message */

  /* --- read item appearance indicators --- */
  ibase = ib_create(0, 0);      /* create an item base */
  if (!ibase) error(E_NOMEM);   /* to manage the items */
  tread = trd_create();         /* create a transaction reader */
  if (!tread) error(E_NOMEM);   /* and configure the characters */
  trd_allchs(tread, recseps, fldseps, blanks, "", comment);
  if (fn_app) {                 /* if item appearances are given */
    t = clock();                /* start timer, open input file */
    if (trd_open(tread, NULL, fn_app) != 0)
      error(E_FOPEN, trd_name(tread));
    MSG(stderr, "reading %s ... ", trd_name(tread));
    m = (target == ISR_RULE)    /* depending on the target type */
      ? ib_readapp(ibase,tread) /* read the item appearances */
      : ib_readsel(ibase,tread);/* or a simple item selection */
    if (m < 0) error((int)-m, ib_errmsg(ibase, NULL, 0));
    trd_close(tread);           /* close the input file */
    MSG(stderr, "[%"ITEM_FMT" item(s)]", m);
    MSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));
  }                             /* print a log message */

  /* --- read transaction database --- */
  tabag = tbg_create(ibase);    /* create a transaction bag */
  if (!tabag) error(E_NOMEM);   /* to store the transactions */
  t = clock();                  /* start timer, open input file */
  if (trd_open(tread, NULL, fn_inp) != 0)
    error(E_FOPEN, trd_name(tread));
  MSG(stderr, "reading %s ... ", trd_name(tread));
  k = tbg_read(tabag, tread, mtar);
  if (k < 0)                    /* read the transaction database */
    error(-k, tbg_errmsg(tabag, NULL, 0));
  trd_delete(tread, 1);         /* close the input file and */
  tread = NULL;                 /* delete the table reader */
  m = ib_cnt(ibase);            /* get the number of items, */
  n = tbg_cnt(tabag);           /* the number of transactions, */
  w = tbg_wgt(tabag);           /* the total transaction weight */
  MSG(stderr, "[%"ITEM_FMT" item(s), %"TID_FMT, m, n);
  if (w != (SUPP)n) MSG(stderr, "/%"SUPP_FMT, w);
  MSG(stderr, " transaction(s)] done [%.2fs].", SEC_SINCE(t));
  if ((m <= 0) || (n <= 0))     /* check for at least one item */
    error(E_NOITEMS);           /* and at least one transaction */
  MSG(stderr, "\n");            /* terminate the log message */
  supp    =       (supp >= 0) ? 0.01 *supp *(double)w : -supp;
  smax    = floor((smax >= 0) ? 0.01 *smax *(double)w : -smax);
  conf   *= 0.01;               /* transform support and confidence */
  thresh *= 0.01;               /* and the eval. measure parameters */
  if (minimp > -INFINITY) minimp *= 0.01;

  /* --- sort and recode items --- */
  t = clock();                  /* start timer, print log message */
  MSG(stderr, "filtering, sorting and recoding items ... ");
  r = (SUPP)ceilsupp((mode & APP_HEAD) ? supp : supp *conf);
  m = tbg_recode(tabag, r, -1, -1, sort);
  if (m <  0) error(E_NOMEM);   /* recode items and transactions */
  if (m <= 0) error(E_NOITEMS); /* and check the number of items */
  MSG(stderr, "[%"ITEM_FMT" item(s)]", m);
  MSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));

  /* --- sort and reduce transactions --- */
  t = clock();                  /* start timer, print log message */
  MSG(stderr, "sorting and reducing transactions ... ");
  if ((eval == IST_NONE)        /* remove items of short transactions */
  ||  (eval == IST_LDRATIO))    /* if it does not affect evaluation */
    tbg_filter(tabag, min, NULL, 0);
  else filter = 0;              /* suppress later filtering if nec. */
  tbg_itsort(tabag, +1, 0);     /* sort items in transactions and */
  tbg_sort  (tabag, +1, 0);     /* sort the trans. lexicographically */
  n = tbg_reduce(tabag, 0);     /* reduce transactions to unique ones */
  MSG(stderr, "[%"TID_FMT, n);  /* print number of transactions */
  if (w != (SUPP)n) MSG(stderr, "/%"SUPP_FMT, w);
  MSG(stderr, " transaction(s)] done [%.2fs].\n", SEC_SINCE(t));

  /* --- execute apriori algorithm --- */
  if (eval == IST_LDRATIO) mrep |= ISR_LOGS;
  report = isr_create(ibase, mrep, -1, hdr, sep, imp);
  if (!report) error(E_NOMEM);  /* create an item set reporter */
  isr_setfmt (report, format);  /* and configure it: set mode, */
  isr_setsize(report, min, max);/* info. format and size range */
  if (fn_psp && (isr_addpsp(report, NULL) < 0))
    error(E_NOMEM);             /* add a pattern spectrum if req. */
  if (isr_open(report, NULL, fn_out) != 0)
    error(E_FOPEN, isr_name(report)); /* open the output file */
  MSG(stderr, "writing %s ... ", isr_name(report));
  k = apriori(tabag, target, mode|APR_NOCLEAN,
              (SUPP)ceilsupp(supp), (SUPP)smax, conf, eval|invbxs,
              agg, thresh, minimp, prune, filter, dir, report);
  if (k) error(E_NOMEM);        /* execute the apriori algorithm */
  if (stats)                    /* print item set statistics */
    isr_prstats(report, stdout, 0);
  if (isr_close(report) != 0)   /* close the output file */
    error(E_FWRITE, isr_name(report));
  MSG(stderr, "[%"SIZE_FMT" %s(s)]", isr_repcnt(report),
              (target == ISR_RULE) ? "rule" : "set");
  MSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));

  /* --- write pattern spectrum --- */
  if (fn_psp) {                 /* if to write a pattern spectrum */
    psp    = isr_getpsp(report);/* get the pattern spectrum */
    twrite = twr_create();      /* create a table writer and */
    if (!twrite) error(E_NOMEM);/* open the output file */
    if (twr_open(twrite, NULL, fn_psp) != 0)
      error(E_FOPEN,  twr_name(twrite));
    if (psp_report(psp, twrite) != 0)
      error(E_FWRITE, twr_name(twrite));
    twr_delete(twrite, 1);      /* write the pattern spectrum, */
    twrite = NULL;              /* delete the table writer, and */
  }                             /* clear the writer variable */

  /* --- clean up --- */
  CLEANUP;                      /* clean up memory and close files */
  SHOWMEM;                      /* show (final) memory usage */
  return 0;                     /* return 'ok' */
}  /* main() */

#endif  /* #ifdef APRIACC ... #else ... */
#endif  /* #ifdef APR_MAIN ... */
