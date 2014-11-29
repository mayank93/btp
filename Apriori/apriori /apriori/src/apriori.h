/*----------------------------------------------------------------------
  File    : apriori.h
  Contents: apriori algorithm for finding frequent item sets
  Author  : Christian Borgelt
  History : 2011.07.18 file created
            2011.10.18 several mode flags added
            2013.03.30 adapted to type changes in module tract
----------------------------------------------------------------------*/
#ifndef __APRIORI__
#define __APRIORI__
#include "istree.h"

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define APR_VERBOSE   INT_MIN   /* verbose message output */
#define APR_TATREE    (IST_PERFECT << 4)  /* use transaction tree */
#define APR_POST      (APR_TATREE  << 1)  /* use a-posteriori pruning */
#ifdef NDEBUG
#define APR_NOCLEAN   (APR_POST    << 1)
#else                           /* do not clean up memory */
#define APR_NOCLEAN   0         /* in function apriori() */
#endif

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/
extern int apriori (TABAG *tabag, int target, int mode, SUPP supp,
                    SUPP smax, double conf, int eval, int aggm,
                    double minval, double minimp, ITEM prune,
                    double filter, int dir, ISREPORT *rep);
#endif
