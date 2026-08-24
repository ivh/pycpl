/*
 * This file is part of the ESO Common Pipeline Library
 * Copyright (C) 2001-2022 European Southern Observatory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <cpl_tools.h>
#include <cpl_test.h>
#include <cpl_memory.h>
#include <cpl_math_const.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#ifdef _OPENMP
#include <omp.h>
#endif

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

/* The maximum number of data elements for which all possible permutations will
 * be tested. */
#define MIN_ARRAY_ALL_PERMUTATIONS 1
#define MAX_ARRAY_ALL_PERMUTATIONS 11

/* The maximum number of data elements for which all possible permutations will
 * be tested - with 3 variants of input data. */
#define MAX_ARRAY_ALL_PERMUTATIONS_3 9

/* Do not run in parallel below this array length (reduces messaging) */
#define MIN_PARALLEL_ALL_PERMUTATIONS 8

/* The minimum number of data elements for which a random sampling of
 * permutations will be tested - first sequence */
#define MIN_ARRAY_RANDOM_PERMUTATIONS 12
/* The maximum number of data elements for which a random sampling of
 * permutations will be tested - first sequence */
#define MAX_ARRAY_RANDOM_PERMUTATIONS 28

/* The number of random permutations in one block. */
#define RND_BLOCK 500000

/* The maximum number of random blocks */
#define MAX_BLOCKS 3

/* The maximum number of random permutations to test - first sequence */
#define MAX_PERMUTATIONS MAX_BLOCKS *(cpl_size)RND_BLOCK

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

static int cpl_test_permutations_4739(cpl_size n,
                                      cpl_size nn1fix,
                                      cpl_size nn2fix,
                                      cpl_size *pcount,
                                      cpl_size nrand) CPL_ATTR_NONNULL;

static void
cpl_print_failed_permutation(const double *data, cpl_size n) CPL_ATTR_NONNULL;
static void cpl_print_failed_permutation_float(const float *data,
                                               cpl_size n) CPL_ATTR_NONNULL;
static void
cpl_print_failed_permutation_int(const int *data, cpl_size n) CPL_ATTR_NONNULL;

inline static int cpl_check_median(double *data,
                                   cpl_size n,
                                   double expected,
                                   cpl_boolean check_perm) CPL_ATTR_NONNULL;
inline static int cpl_check_median_float(float *data,
                                         cpl_size n,
                                         float expected) CPL_ATTR_NONNULL;
inline static int
cpl_check_median_int(int *data, cpl_size n, int expected) CPL_ATTR_NONNULL;
static int
cpl_check_median_mid(const cpl_size *perm, cpl_size n) CPL_ATTR_NONNULL;
static int
cpl_check_median_floor(const cpl_size *perm, cpl_size n) CPL_ATTR_NONNULL;
static int
cpl_check_median_ceil(const cpl_size *perm, cpl_size n) CPL_ATTR_NONNULL;

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/

int
main(void)
{
    const cpl_size nthreads = 1
#ifdef _OPENMP
                              * (cpl_size)omp_get_max_threads()
#endif
        ;
    const cpl_size max_perm = MAX_BLOCKS * nthreads;
    double nfact = 1.0;
    double msum = 0.;
    cpl_size nfull = 0;
    cpl_size nrand = 0;
    cpl_size irand = 0;
    int did_fail = 0; /* Assume no failure */

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Deliberately test differents parts of the very large permutation space */
    srand((int)time(NULL));

    /* Assume no failure */
    for (cpl_size i = 1; i <= MAX_ARRAY_RANDOM_PERMUTATIONS; i++) {
        nfact *= (double)i;
        if (i >= MIN_ARRAY_RANDOM_PERMUTATIONS) {
            msum += nfact;
        }
    }

#if MIN_ARRAY_ALL_PERMUTATIONS <= MAX_ARRAY_ALL_PERMUTATIONS
    cpl_msg_info(cpl_func, "Testing all %d- to %d-medians",
                 MIN_ARRAY_ALL_PERMUTATIONS, MAX_ARRAY_ALL_PERMUTATIONS);
#endif

    /* Here we perform 3 tests on all possible permutations for data arrays of
     * size 2 to MAX_ARRAY_ALL_PERMUTATIONS. For each test the data array is
     * prepared with elements from one of 3 corresponding sets. These are given
     * by:
     *   (1)  {0, 1, ... n-1}.
     *   (2)  {floor(n/2), 1, 2, ... n-1}
     *   (3)  {floor(n/2)+1, 1, 2, ... n-1}  (only if n is odd)
     * The expected median for each of the data sets is given by:
     *   (1)  (n-1)/2
     *   (2)  floor(n/2)
     *   (3)  ceil(n/2)   (only valid if n is odd)
     */

    for (cpl_size n = MIN_ARRAY_ALL_PERMUTATIONS;
         n <=
         CPL_MIN(MIN_PARALLEL_ALL_PERMUTATIONS - 1, MAX_ARRAY_ALL_PERMUTATIONS);
         ++n) {
        cpl_size ifull = 0;

        if (cpl_test_permutations_4739(n, n, n, &ifull, 0)) {
            ++did_fail;
            break;
        }
        cpl_msg_info(cpl_func, "Tested %d-median: %d", (int)n, (int)ifull);
        nfull += ifull;
    }

    for (cpl_size n =
             CPL_MAX(MIN_PARALLEL_ALL_PERMUTATIONS, MIN_ARRAY_ALL_PERMUTATIONS);
         n <= MAX_ARRAY_ALL_PERMUTATIONS; ++n) {
        cpl_size ifull = 0;

#ifdef _OPENMP
#pragma omp parallel for collapse(2) reduction(+ : ifull)
#endif
        for (cpl_size n1 = 0; n1 < n; n1++) {
            for (cpl_size n2a = 0; n2a < n - 1; n2a++) {
                const cpl_size n2 = n2a == n1 ? n - 1 : n2a;

                if (did_fail)
                    continue;

                if (cpl_test_permutations_4739(n, n1, n2, &ifull, 0)) {
                    ++did_fail;
                }
                else {
                    cpl_msg_info(cpl_func, "Tested %d-median: %d <=> %d",
                                 (int)n, (int)n1, (int)n2);
                }
            }
        }

        if (did_fail)
            break;

        nfull += ifull;

        cpl_msg_info(cpl_func, "Tested all %d-median(s): %" CPL_SIZE_FORMAT,
                     (int)n, ifull);
    }

#if MIN_ARRAY_RANDOM_PERMUTATIONS <= MAX_ARRAY_RANDOM_PERMUTATIONS
    /* Run the tests as before except starting at random
           points in the (typically much larger) permutation space
           - and perform only a fixed number of tests */

    cpl_msg_info(cpl_func,
                 "Random %d-threaded fractional testing %d- to "
                 "%d-medians: (%d - %d) * %d * %" CPL_SIZE_FORMAT " / %g = %g",
                 (int)nthreads, MIN_ARRAY_RANDOM_PERMUTATIONS,
                 MAX_ARRAY_RANDOM_PERMUTATIONS,
                 1 + MAX_ARRAY_RANDOM_PERMUTATIONS,
                 MIN_ARRAY_RANDOM_PERMUTATIONS, (int)nthreads, MAX_PERMUTATIONS,
                 msum,
                 (1 + MAX_ARRAY_RANDOM_PERMUTATIONS -
                  MIN_ARRAY_RANDOM_PERMUTATIONS) *
                     nthreads * MAX_PERMUTATIONS / msum);
#endif

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) reduction(+ : irand)
#endif
    for (cpl_size mrand = 0; mrand < max_perm; mrand++) {
        cpl_size n;

        if (did_fail)
            continue;

        for (n = MIN_ARRAY_RANDOM_PERMUTATIONS;
             n <= MAX_ARRAY_RANDOM_PERMUTATIONS; ++n) {
            if (cpl_test_permutations_4739(n, n, n, &irand, RND_BLOCK)) {
                break;
            }
        }

        if (n <= MAX_ARRAY_RANDOM_PERMUTATIONS) {
#ifdef _OPENMP
#pragma omp atomic
#endif
            ++did_fail;
        }

#if MIN_ARRAY_RANDOM_PERMUTATIONS <= MAX_ARRAY_RANDOM_PERMUTATIONS
        if (mrand % nthreads == 0)
            cpl_msg_info(cpl_func,
                         "Random tested %d- to %d-median(s): block "
                         "%" CPL_SIZE_FORMAT "/%" CPL_SIZE_FORMAT
                         " @ %d = %" CPL_SIZE_FORMAT,
                         MIN_ARRAY_RANDOM_PERMUTATIONS,
                         MAX_ARRAY_RANDOM_PERMUTATIONS, mrand, max_perm,
                         RND_BLOCK, irand);
#endif
    }

    nrand += irand;

#if MIN_ARRAY_RANDOM_PERMUTATIONS <= MAX_ARRAY_RANDOM_PERMUTATIONS
    assert(msum > 0);
    cpl_msg_info(cpl_func,
                 "Random tested %d- to %d-medians: %" CPL_SIZE_FORMAT
                 " / %g = %g",
                 MIN_ARRAY_RANDOM_PERMUTATIONS, MAX_ARRAY_RANDOM_PERMUTATIONS,
                 nrand, msum, nrand / msum);
#endif

    cpl_test_zero(did_fail);

    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Prints out the data for a failed permutation as error messages.
 */
/*----------------------------------------------------------------------------*/
static void
cpl_print_failed_permutation(const double *data, cpl_size n)
{
    cpl_msg_error(cpl_func,
                  "Failed test on permutation with %" CPL_SIZE_FORMAT
                  " double elements:",
                  n);
    cpl_msg_error(cpl_func, "Item\tValue");
    for (cpl_size i = 0; i < n; ++i) {
        cpl_msg_error(cpl_func, "%" CPL_SIZE_FORMAT "\t%f", i + 1, data[i]);
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Prints out the data for a failed permutation as error messages.
 */
/*----------------------------------------------------------------------------*/
static void
cpl_print_failed_permutation_float(const float *data, cpl_size n)
{
    cpl_msg_error(cpl_func,
                  "Failed test on permutation with %" CPL_SIZE_FORMAT
                  " float elements:",
                  n);
    cpl_msg_error(cpl_func, "Item\tValue");
    for (cpl_size i = 0; i < n; ++i) {
        cpl_msg_error(cpl_func, "%" CPL_SIZE_FORMAT "\t%f", i + 1, data[i]);
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Prints out the data for a failed permutation as error messages.
 */
/*----------------------------------------------------------------------------*/
static void
cpl_print_failed_permutation_int(const int *data, cpl_size n)
{
    cpl_msg_error(cpl_func,
                  "Failed test on permutation with %" CPL_SIZE_FORMAT
                  " int elements:",
                  n);
    cpl_msg_error(cpl_func, "Item\tValue");
    for (cpl_size i = 0; i < n; ++i) {
        cpl_msg_error(cpl_func, "%" CPL_SIZE_FORMAT "\t%d", i + 1, data[i]);
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Checks if median is computed correctly, per the provided value.
  @param data       The array of elements to permute
  @param n          The number of elements
  @param expected   The expected median value
  @param check_perm Verify lower + upper permunations of the median data
  @return Zero (or non-zero on failure)
  
 */
/*----------------------------------------------------------------------------*/
inline static int
cpl_check_median(double *data,
                 cpl_size n,
                 double expected,
                 cpl_boolean check_perm)
{
    /* This CPL function can set no error code, so don't check it */
    const double computed = cpl_tools_get_median_double(data, n);

    /* Log only on failure to reduce massive amount of logging */
    if (fabs(computed - expected) > DBL_EPSILON) {
        cpl_test_abs(computed, expected, DBL_EPSILON);
        cpl_print_failed_permutation(data, n);
        return 1;
    }

    if (check_perm) {
        const cpl_size nfailed = cpl_test_get_failed();
        cpl_size j;

        if (n & 1) {
            for (j = 0; j < n / 2; j++) {
                if (!(data[j] <= data[n / 2]))
                    break;
            }
            /* Log only on failure to reduce massive amount of logging */
            if (j < n / 2)
                cpl_test_lt(data[j], data[n / 2]);
        }
        else {
            for (j = 0; j < n / 2 - 1; j++) {
                if (!(data[j] <= data[n / 2 - 1]))
                    break;
            }
            if (j < n / 2 - 1)
                cpl_test_lt(data[j], data[n / 2 - 1]);
        }

        for (j = n - 1; j > n / 2; j--) {
            if (!(data[n / 2] <= data[j]))
                break;
        }

        /* Log only on failure to reduce massive amount of logging */
        if (j > n / 2)
            cpl_test_lt(data[n / 2], data[j]);

        if (cpl_test_get_failed() != nfailed) {
            cpl_print_failed_permutation(data, n);
            return 1;
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Checks if median is computed correctly, per the provided value.
  @param data       The array of elements to permute
  @param n          The number of elements
  @param expected   The expected median value
  @param check_perm Verify lower + upper permunations of the median data
  @return Zero (or non-zero on failure)
  
 */
/*----------------------------------------------------------------------------*/
inline static int
cpl_check_median_float(float *data, cpl_size n, float expected)
{
    const cpl_size nfailed = cpl_test_get_failed();
    /* This CPL function can set no error code, so don't check it */
    const float computed = cpl_tools_get_median_float(data, n);
    cpl_size j;

    /* Log only on failure to reduce massive amount of logging */
    if (fabsf(computed - expected) > FLT_EPSILON) {
        cpl_test_abs(computed, expected, FLT_EPSILON);
        cpl_print_failed_permutation_float(data, n);
        return 1;
    }

    if (n & 1) {
        for (j = 0; j < n / 2; j++) {
            if (!(data[j] <= data[n / 2]))
                break;
        }
        /* Log only on failure to reduce massive amount of logging */
        if (j < n / 2)
            cpl_test_lt(data[j], data[n / 2]);
    }
    else {
        for (j = 0; j < n / 2 - 1; j++) {
            if (!(data[j] <= data[n / 2 - 1]))
                break;
        }
        if (j < n / 2 - 1)
            cpl_test_lt(data[j], data[n / 2 - 1]);
    }

    for (j = n - 1; j > n / 2; j--) {
        if (!(data[n / 2] <= data[j]))
            break;
    }

    /* Log only on failure to reduce massive amount of logging */
    if (j > n / 2)
        cpl_test_lt(data[n / 2], data[j]);

    if (cpl_test_get_failed() != nfailed) {
        cpl_print_failed_permutation_float(data, n);
        return 1;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Checks if median is computed correctly, per the provided value.
  @param data       The array of elements to permute
  @param n          The number of elements
  @param expected   The expected median value
  @param check_perm Verify lower + upper permunations of the median data
  @return Zero (or non-zero on failure)
  
 */
/*----------------------------------------------------------------------------*/
inline static int
cpl_check_median_int(int *data, cpl_size n, int expected)
{
    const cpl_size nfailed = cpl_test_get_failed();
    /* This CPL function can set no error code, so don't check it */
    const int computed = cpl_tools_get_median_int(data, n);
    cpl_size j;

    /* Log only on failure to reduce massive amount of logging */
    if (computed != expected) {
        cpl_test_eq(computed, expected);
        cpl_print_failed_permutation_int(data, n);
        return 1;
    }


    if (n & 1) {
        for (j = 0; j < n / 2; j++) {
            if (!(data[j] <= data[n / 2]))
                break;
        }
        /* Log only on failure to reduce massive amount of logging */
        if (j < n / 2)
            cpl_test_lt(data[j], data[n / 2]);
    }
    else {
        for (j = 0; j < n / 2 - 1; j++) {
            if (!(data[j] <= data[n / 2 - 1]))
                break;
        }
        if (j < n / 2 - 1)
            cpl_test_lt(data[j], data[n / 2 - 1]);
    }

    for (j = n - 1; j > n / 2; j--) {
        if (!(data[n / 2] <= data[j]))
            break;
    }

    /* Log only on failure to reduce massive amount of logging */
    if (j > n / 2)
        cpl_test_lt(data[n / 2], data[j]);

    if (cpl_test_get_failed() != nfailed) {
        cpl_print_failed_permutation_int(data, n);
        return 1;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Checks if median is computed correctly as (n-1)/2.
  @see cpl_check_median()
  @note Also verify correct lower/upper partitioning
 */
/*----------------------------------------------------------------------------*/
static int
cpl_check_median_mid(const cpl_size *perm, cpl_size n)
{
    double data[n];

    for (cpl_size i = 0; i < n; ++i) {
        data[perm[i]] = i;
    }

    return cpl_check_median(data, n, 0.5 * (n - 1), CPL_TRUE);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Checks if median is computed correctly as floor(n/2).
  @see cpl_check_median()
 */
/*----------------------------------------------------------------------------*/
static int
cpl_check_median_floor(const cpl_size *perm, cpl_size n)
{
    double data[n];
    float fdata[n];
    int idata[n];

    /* NB: Samples are not guaranteed to be unique */
    data[perm[0]] = n / 2;
    fdata[perm[0]] = n / 2;
    idata[perm[0]] = n / 2;
    for (cpl_size i = 1; i < n; ++i) {
        data[perm[i]] = i;
        fdata[perm[i]] = i;
        idata[perm[i]] = i;
    }

    return cpl_check_median(data, n, floor(0.5 * n), CPL_FALSE) ||
           cpl_check_median_float(fdata, n, floor(0.5 * n)) ||
           cpl_check_median_int(idata, n, n / 2);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Checks if median is computed correctly as ceil(n/2).
  @see cpl_check_median()
 */
/*----------------------------------------------------------------------------*/
static int
cpl_check_median_ceil(const cpl_size *perm, cpl_size n)
{
    double data[n];

    data[perm[0]] = n / 2 + 1; /* NB: Samples are not guaranteed to be unique */
    for (cpl_size i = 1; i < n; ++i) {
        data[perm[i]] = i;
    }

    return cpl_check_median(data, n, ceil(0.5 * n), CPL_FALSE);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Generates and tests all permutations of a data array.

  @param   n        The number of elements in the data array to test
  @param   nn1fix   The fixed value in the last data array element, or n
  @param   nn2fix   The fixed value in the 2nd last data array element, or n
  @param   pcount   Increment by the number of succesfull median tests
  @param   nrand    Number of random samples, zero for exhaustive
  @return Zero iff all permutations were processed OK.
  @see PIPE-4739.

  This function generates all possible permutations of a data array and tests
  various median functions on each one.

  Because of the potentially large number of permutations, a failure causes the
  testing to stop.

  This function is using the iterative implementation [1] of Heap's permutation
  generation algorithm [2], to generate all permutations of the data array.

  In order to facilitate parallelism, the traversal of the two last elements
  can optionally be peeled off, by providing the values of these two elements
  directly. In that case only 1/n/n-1 of the tests are performed, using those
  two values (so the function may be called n*(n-1) times - potentially in
  parallel, with the n*(n-1) different values: 0, 1, ..., n - 1. This is 
  supported for n greater than 2.

  [1] http://permute.tchs.info/ScalablePermutations.html
  [2] Heap, B. R. (1963). "Permutations by Interchanges". The Computer Journal
      6 (3): 293-4 - see also
     https://en.wikipedia.org/wiki/Heap%27s_algorithm
 */
/*----------------------------------------------------------------------------*/
static int
cpl_test_permutations_4739(cpl_size n,
                           cpl_size nn1fix,
                           cpl_size nn2fix,
                           cpl_size *pcount,
                           cpl_size nrand)
{
    cpl_size p1[n + 1]; /* Short for p[i-1], p1[0] never used */
    cpl_size *p = p1 + 1;
    cpl_size perm[n];
    const cpl_size n1 = (nn1fix < n && n > 1) ? n - 1 : n;
    const cpl_size n2 = (n1 < n && n > 2) ? n - 2 : n1;
    const cpl_size nn12min = CPL_MIN(nn1fix, nn2fix);
    const cpl_size nn12max = CPL_MAX(nn1fix, nn2fix);
    cpl_size count = 0;
    cpl_size count2 = 1;

    for (cpl_size i = 0; i < n; ++i) {
        p[i] = i + 1; /* Number of iterations on ith element */

        /* Initial permutation */
        if (n1 == n) {
            perm[i] = i;
        }
        else if (i < n2) {
            if (i < nn12min) {
                perm[i] = i;
            }
            else if (i + 1 < nn12max) {
                perm[i] = i + 1;
            }
            else {
                perm[i] = i + 2;
            }
        }
        else if (i > n2) {
            perm[i] = nn2fix;
        }
        else {
            perm[i] = nn1fix;
        }

        if (i < n2) {
            count2 *= i + 1;
            if (nrand > 0 && count2 > nrand) {
                /* Handle that count2 may wrap around */
                count2 = nrand;
            }
        }
    }

    if (nrand) { /* Start from a random permutation */
        assert(n == nn1fix);
        assert(n == nn2fix);
        for (cpl_size i = 0; i < n; ++i) {
            const cpl_size k = rand() % n;
            const cpl_size tperm = perm[i];

            perm[i] = perm[k];
            perm[k] = tperm;
        }
    }

    /* Iterate through the n-factorial permutations */
    for (cpl_size i = 0; i < n2; p1[i]--, count++) {
        /* odd  i: swap position p1[i] and i. */
        /* even i: swap position 0 and i. */
        const cpl_size j = (i & 1) ? p1[i] : 0;
        const cpl_size tmp = perm[i];

        perm[i] = perm[j];
        perm[j] = tmp;

        for (i = 1; p1[i] == 0; ++i) {
            p1[i] = i;
        }

        if (nrand > 0 && count == nrand)
            break;

#if 0
        /* FIXME: Verify also w. n1 < n */
        if (!nrand && n1 == n && count > 0) {
            /* Check on permutation correctness - cannot redo initial */
            cpl_size k;
            for (k = 0; k < n; k++) {
                if (perm[k] != k)
                    break;
            }
            if (k == n) { /* Log only on error */
                cpl_test_lt(k, n);
                break;
            }
        }

        cpl_msg_warning(cpl_func, "P(%d<%d): %d", (int)i, (int)n, (int)count);
        for (cpl_size k = 0; k < n; k++) {
            cpl_msg_warning(cpl_func, "%d: %d", (int)k, (int)perm[k]);
        }
#endif

        if (cpl_check_median_mid(perm, n))
            return 1;

        if (n > MAX_ARRAY_ALL_PERMUTATIONS_3)
            continue;

        if (cpl_check_median_floor(perm, n))
            return 1;
        if ((n & 1) && cpl_check_median_ceil(perm, n))
            return 1;
    }

    /* Check on permutation correctness - log only on error */
    if (!(count2 == count)) {
        cpl_msg_warning(cpl_func, "%lld <=> %lld (%lld)", count2, count, nrand);
        cpl_test_eq(count2, count);
    }

    *pcount += count;

    return 0;
}
