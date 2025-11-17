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
#include <config.h>
#endif

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpl_error.h"
#include "cpl_math_const.h"
#include "cpl_memory.h"
#include "cpl_msg.h"
#include "cpl_ppm.h"
#include "cpl_test.h"

int
main(void)
{
    cpl_matrix *pattern = NULL;
    cpl_matrix *data = NULL;
    cpl_matrix *rdata = NULL;
    cpl_matrix *mpattern = NULL;
    cpl_matrix *mdata = NULL;
    cpl_array *matches = NULL;
    cpl_array *nullarray = NULL;
    cpl_matrix *rotate = NULL;
    cpl_matrix *twobytwo = NULL;

    // clang-format off
    const double pointdata[] = {
        96.807119, 6.673062,
        47.828109, 90.953442,
        35.169238, 93.253366,
        65.443582, 2.107025,
        51.220486, 20.201893,
        93.997703, 20.408227,
        37.882893, 79.311394,
        28.820079, 26.715673,
        35.682260, 12.837355,
        70.329776, 73.741373,
        80.252114, 91.523087,
        51.138163, 76.205738,
        45.638141, 47.106201,
        29.955025, 61.255939,
        7.338079,  49.818536,
        21.958749, 4.145198,
        56.491598, 69.786858,
        95.098640, 91.660836,
        63.040224, 60.542222,
        93.767861, 14.260710,
        80.744116, 87.765564,
        34.668937, 18.627008,
        67.076958, 63.489016,
        45.342681, 2.759218,
        76.326371, 15.672457,
        76.500591, 56.578485,
        7.195544,  27.638754,
        32.784223, 52.833685,
        74.744955, 62.739249,
        14.089624, 82.083033,
        12.557785, 36.048373,
        86.228231, 69.049383,
        5.835231,  81.326871,
        60.710220, 68.875455,
        41.869094, 54.478081,
        83.136166, 22.613209,
        42.243645, 17.805103,
        41.240218, 9.320603,
        81.294120, 86.582899,
        12.079821, 57.620490,
        2.255356,  88.580412,
        14.198976, 9.450900,
        16.219166, 46.983199,
        62.284586, 90.964121,
        9.722447,  76.374210,
        73.047154, 22.280233,
        12.422583, 59.275385,
        91.329616, 18.257814,
        40.602257, 52.039836,
        87.133270, 82.471350,
        6.517916,  70.269436,
        5.084560,  48.761561,
        88.074539, 46.324777,
        58.082164, 69.368659,
        32.907676, 70.161985,
        26.989149, 35.163032,
        58.742397, 41.188125,
        44.613932, 74.961563,
        88.171324, 6.898518,
        65.925684, 97.893771,
        83.272728, 38.972839,
        20.174004, 95.695311,
        98.248224, 11.503620,
        13.953125, 38.850481,
        63.543456, 1.086395,
        21.321831, 70.061372,
        71.355831, 26.406390,
        18.822933, 59.430370,
        72.731168, 76.905097,
        28.799029, 5.638844,
        47.067082, 55.788179,
        40.801876, 5.809480,
        96.976304, 85.415809,
        80.771043, 85.147628,
        92.314327, 46.696728,
        83.041400, 75.587054,
        85.669566, 3.215404,
        71.282365, 83.917790,
        14.719024, 85.235491,
        22.768271, 78.262480,
        86.321886, 44.090102,
        48.323852, 57.677717,
        70.496492, 67.146785,
        17.108088, 43.227660,
        44.051883, 45.907117,
        48.866504, 91.118965,
        1.695296,  89.668380,
        96.928445, 98.671600,
        75.084189, 77.699488,
        83.819228, 67.398515,
        24.396216, 66.860628,
        42.985570, 10.065782,
        70.076031, 14.267935,
        93.983572, 84.795055,
        99.503426, 16.751843,
        63.057535, 85.825312,
        60.841945, 11.381387,
        43.503029, 31.338437,
        78.528172, 60.611117,
        74.566097, 22.580055};
    // clang-format on

    const int permute[] = { 8,  2,  1,  13, 7,  3,  5,  9,  14, 4,
                            0,  6,  11, 10, 12, 23, 17, 16, 28, 22,
                            18, 20, 24, 29, 19, 15, 21, 26, 25, 27 };


    const double positions[] = { 0.1, 0.25, 1., 3., 9., 10., 11., 12.5 };

    const double min_disp = 1.1875;
    const double max_disp = 1.3125;
    const double tolerance = 0.1;

    const double peakdata[] = {
        686.772, 973.745, 1140.69, 1184.02, 1219.17, 1236.48, 1262.11, 1273.67,
        1289.98, 1332.57, 1371.65, 1402.06, 1425.62, 1464.21, 1479.52, 1562.45,
        1583.41, 1636.00, 1678.33, 1699.17, 1730.06, 1899.42, 1981.58, 2002.91,
        2094.64, 2151.58, 2306.59, 2333.43, 2346.61, 2384.20, 2390.81, 2711.13,
        2823.12, 2852.02, 2866.53, 2965.52, 2998.58, 3051.23, 3060.80, 3093.66,
        3129.96, 3155.82, 3195.60, 3217.07, 3233.23, 3268.28, 3278.29, 3284.29,
        3305.73, 3324.35, 3379.01, 3386.96, 3445.52, 3455.10, 3498.60, 3684.23,
        3727.32, 3742.57, 3807.69, 3828.57, 3997.14
    };

    const double linedata[] = {
        5400.56, 5764.42, 5820.16, 5852.49, 5872.83, 5881.90, 5901.41, 5944.83,
        5975.12, 6030.00, 6074.34, 6096.16, 6128.45, 6143.06, 6163.59, 6217.28,
        6266.50, 6304.79, 6332.77, 6382.99, 6402.25, 6506.53, 6532.88, 6598.95,
        6652.09, 6678.28, 6717.04, 6929.47, 7024.05, 7032.41, 7051.29, 7059.11,
        7173.94, 7245.17, 7438.90, 7472.44, 7488.87, 7535.77, 7544.04, 7943.18,
        8082.46, 8118.55, 8136.41, 8259.38, 8266.08, 8300.33, 8365.75, 8377.51,
        8418.43, 8463.36, 8495.36, 8544.70, 8571.35, 8591.26, 8634.65, 8647.04,
        8654.38, 8680.79, 8704.11, 8771.66, 8781.97, 8853.87, 8865.70, 8919.50,
        9148.67, 9201.76, 9220.47, 9300.85, 9326.51, 9425.38, 9534.16, 9665.42
    };

    // clang-format off
    const struct {double x; double y;} reference_matches[] = {
        {1140.69, 5975.12},
        {1184.02, 6030.00},
        {1219.17, 6074.34},
        {1236.48, 6096.16},
        {1262.11, 6128.45},
        {1273.67, 6143.06},
        {1289.98, 6163.59},
        {1332.57, 6217.28},
        {1371.65, 6266.50},
        {1402.06, 6304.79},
        {1425.62, 6332.77},
        {1464.21, 6382.99},
        {1479.52, 6402.25},
        {1562.45, 6506.53},
        {1583.41, 6532.88},
        {1636.00, 6598.95},
        {1678.33, 6652.09},
        {1699.17, 6678.28},
        {1730.06, 6717.04},
        {1899.42, 6929.47},
        {1981.58, 7032.41},
        {2002.91, 7059.11},
        {2094.64, 7173.94},
        {2151.58, 7245.17},
        {2306.59, 7438.90},
        {2333.43, 7472.44},
        {2346.61, 7488.87},
        {2384.20, 7535.77},
        {2390.81, 7544.04},
        {2711.13, 7943.18},
        {2823.12, 8082.46},
        {2852.02, 8118.55},
        {2866.53, 8136.41},
        {2965.52, 8259.38},
        {2998.58, 8300.33},
        {3051.23, 8365.75},
        {3060.80, 8377.51},
        {3093.66, 8418.43},
        {3129.96, 8463.36},
        {3155.82, 8495.36},
        {3195.60, 8544.70},
        {3217.07, 8571.35},
        {3233.23, 8591.26},
        {3268.28, 8634.65},
        {3278.29, 8647.04},
        {3284.29, 8654.38},
        {3305.73, 8680.79},
        {3324.35, 8704.11},
        {3379.01, 8771.66},
        {3386.96, 8781.97},
        {3445.52, 8853.87},
        {3455.10, 8865.70},
        {3498.60, 8919.50},
        {3684.23, 9148.67},
        {3727.32, 9201.76},
        {3742.57, 9220.47},
        {3807.69, 9300.85},
        {3828.57, 9326.51}
    };
    // clang-format on

    int i, j, k, npattern, ndata;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);


    /*
     * Create a list of 3 points (0,9), 9,0), and (9,9):
     *
     *                0 9 9
     *                9 0 9
     */

    pattern = cpl_matrix_new(2, 3);
    cpl_matrix_set(pattern, 0, 0, 0.0);
    cpl_matrix_set(pattern, 0, 1, 9.0);
    cpl_matrix_set(pattern, 0, 2, 9.0);
    cpl_matrix_set(pattern, 1, 0, 9.0);
    cpl_matrix_set(pattern, 1, 1, 0.0);
    cpl_matrix_set(pattern, 1, 2, 9.0);

    /*
     * Testing cpl_ppm_match_points()
     */

    /*
     * Create an identical data matrix
     */

    data = cpl_matrix_duplicate(pattern);

    /*
     * Now try to match points: the transformation should be an identity
     * (rotation angle 0, scaling 0, translation 0):
     */

    cpl_msg_info(cpl_func, "Trying to match 3 identical points:");

    matches = cpl_ppm_match_points(data, 3, 1, pattern, 3, 0, 0.1, 1, &mdata,
                                   &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 3);
    cpl_test_eq(cpl_array_get_size(matches), 3);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);

    cpl_msg_info(cpl_func, "Scale data points by 2:");

    cpl_matrix_multiply_scalar(data, 2.0);

    matches = cpl_ppm_match_points(data, 3, 1, pattern, 3, 0, 0.1, 1, &mdata,
                                   &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 3);
    cpl_test_eq(cpl_array_get_size(matches), 3);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);

    cpl_msg_info(cpl_func, "Rotate data points by +45 degrees:");

    rotate = cpl_matrix_new(2, 2);
    cpl_matrix_set(rotate, 0, 0, sqrt(2) / 2);
    cpl_matrix_set(rotate, 0, 1, -sqrt(2) / 2);
    cpl_matrix_set(rotate, 1, 0, sqrt(2) / 2);
    cpl_matrix_set(rotate, 1, 1, sqrt(2) / 2);

    rdata = cpl_matrix_product_create(rotate, data);

    cpl_matrix_delete(rotate);
    cpl_matrix_delete(data);

    matches = cpl_ppm_match_points(rdata, 3, 1, pattern, 3, 0, 0.1, 1, &mdata,
                                   &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 3);
    cpl_test_eq(cpl_array_get_size(matches), 3);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);

    cpl_msg_info(cpl_func, "Shift data points by some vector:");

    cpl_matrix_add_scalar(rdata, -15);

    matches = cpl_ppm_match_points(rdata, 3, 1, pattern, 3, 0, 0.1, 1, &mdata,
                                   &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 3);
    cpl_test_eq(cpl_array_get_size(matches), 3);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_matrix_delete(pattern);
    cpl_matrix_delete(rdata);

    /*
     * Now we start testing the same, with longer lists of points.
     * Matrices will still be identical (no extra points), but only
     * the first 3 points will be used for pattern matching, and the
     * rest will be recovered...
     *
     * Create a list of 8 points:
     *
     *                0 9 9 1 1 5 2 3
     *                9 0 9 0 3 4 1 7
     */

    pattern = cpl_matrix_new(2, 8);
    cpl_matrix_set(pattern, 0, 0, 0.0);
    cpl_matrix_set(pattern, 0, 1, 9.0);
    cpl_matrix_set(pattern, 0, 2, 9.0);
    cpl_matrix_set(pattern, 0, 3, 1.0);
    cpl_matrix_set(pattern, 0, 4, 1.0);
    cpl_matrix_set(pattern, 0, 5, 5.0);
    cpl_matrix_set(pattern, 0, 6, 2.0);
    cpl_matrix_set(pattern, 0, 7, 3.0);
    cpl_matrix_set(pattern, 1, 0, 9.0);
    cpl_matrix_set(pattern, 1, 1, 0.0);
    cpl_matrix_set(pattern, 1, 2, 9.0);
    cpl_matrix_set(pattern, 1, 3, 0.0);
    cpl_matrix_set(pattern, 1, 4, 3.0);
    cpl_matrix_set(pattern, 1, 5, 4.0);
    cpl_matrix_set(pattern, 1, 6, 1.0);
    cpl_matrix_set(pattern, 1, 7, 7.0);


    /*
     * Create an identical data matrix
     */

    data = cpl_matrix_duplicate(pattern);

    /*
     * Now try to match points: the transformation should be an identity
     * (rotation angle 0, scaling 0, translation 0):
     */

    cpl_msg_info(cpl_func, "Trying to match 8 identical points:");

    matches = cpl_ppm_match_points(data, 4, 1, pattern, 3, 0, 0.1, 1, &mdata,
                                   &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 8);
    cpl_test_eq(cpl_array_get_size(matches), 8);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);

    cpl_msg_info(cpl_func, "Remove a point from data:");

    rdata = cpl_matrix_duplicate(data);
    cpl_matrix_erase_columns(rdata, 6, 1);

    matches = cpl_ppm_match_points(rdata, 4, 1, pattern, 3, 0, 0.1, 1, &mdata,
                                   &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 7);
    cpl_test_eq(cpl_array_get_size(matches), 8);
    cpl_test_eq(cpl_array_count_invalid(matches), 1);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_matrix_delete(rdata);


    cpl_msg_info(cpl_func, "Rotate data points by -27 degrees:");

    rotate = cpl_matrix_new(2, 2);
    cpl_matrix_set(rotate, 0, 0, cos(-27. / 180. * CPL_MATH_PI));
    cpl_matrix_set(rotate, 0, 1, -sin(-27. / 180. * CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 0, sin(-27. / 180. * CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 1, cos(-27. / 180. * CPL_MATH_PI));

    rdata = cpl_matrix_product_create(rotate, data);

    cpl_matrix_delete(rotate);
    cpl_matrix_delete(data);

    matches = cpl_ppm_match_points(rdata, 4, 1, pattern, 3, 0, 0.1, 1, &mdata,
                                   &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 8);
    cpl_test_eq(cpl_array_get_size(matches), 8);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_matrix_delete(rdata);
    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_matrix_delete(pattern);

    /*
     * At this point we use a random distribution of 100 points.
     * Only the first 70 will be written to pattern, the other 30
     * will be "false detections". And only the first 10 pattern 
     * points and the first 20 data points will be used for direct
     * pattern matching - the remaining points will be recovered
     * in the second step. The data matrix will be rotated by 95
     * degrees and rescaled by a factor 2.35.
     */

    cpl_msg_info(cpl_func,
                 "100 random distributed data points "
                 "(70 in pattern, 30 false-detections), data rescaled "
                 "by 2.35 and rotated by 95 degrees...");

    pattern = cpl_matrix_new(2, 100);
    for (k = 0, i = 0; i < 2; i++) {
        for (j = 0; j < 100; j++, k++) {
            cpl_matrix_set(pattern, i, j, pointdata[k]);
        }
    }

    data = cpl_matrix_duplicate(pattern);
    cpl_matrix_erase_columns(pattern, 70, 30);
    cpl_matrix_multiply_scalar(data, 2.35);
    rotate = cpl_matrix_new(2, 2);
    cpl_matrix_set(rotate, 0, 0, cos(95. / 180. * CPL_MATH_PI));
    cpl_matrix_set(rotate, 0, 1, -sin(95. / 180. * CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 0, sin(95. / 180. * CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 1, cos(95. / 180. * CPL_MATH_PI));

    rdata = cpl_matrix_product_create(rotate, data);

    cpl_matrix_delete(rotate);
    cpl_matrix_delete(data);

    matches = cpl_ppm_match_points(rdata, 20, 1, pattern, 10, 0, 0.1, 0.1,
                                   &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), 70);
    cpl_test_eq(cpl_array_get_size(matches), 70);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_matrix_delete(rdata);
    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_matrix_delete(pattern);

    /*
     * Now with a random distribution of 20 points. This time
     * the whole data and the whole pattern (limited to 10 points)
     * are used. This means that a 50% contamination is present
     * on the data side already at the first pattern matching step.
     * The data matrix will be again rotated by 95 degrees and 
     * rescaled by a factor 2.35.
     */

    npattern = 10;
    ndata = 20;

    cpl_msg_info(cpl_func,
                 "%d random distributed data points "
                 "(%d in pattern, %d in data, i.e., including %d "
                 "false-detections), data rescaled "
                 "by 2.35 and rotated by 95 degrees...",
                 ndata, npattern, ndata, ndata - npattern);

    pattern = cpl_matrix_new(2, ndata);
    for (k = 0, i = 0; i < 2; i++) {
        for (j = 0; j < ndata; j++, k++) {
            cpl_matrix_set(pattern, i, j, pointdata[k]);
        }
    }

    data = cpl_matrix_duplicate(pattern);
    cpl_matrix_erase_columns(pattern, npattern, ndata - npattern);
    cpl_matrix_multiply_scalar(data, 2.35);
    rotate = cpl_matrix_new(2, 2);
    cpl_matrix_set(rotate, 0, 0, cos(95. / 180. * CPL_MATH_PI));
    cpl_matrix_set(rotate, 0, 1, -sin(95. / 180. * CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 0, sin(95. / 180. * CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 1, cos(95. / 180. * CPL_MATH_PI));

    rdata = cpl_matrix_product_create(rotate, data);

    cpl_matrix_delete(rotate);
    cpl_matrix_delete(data);

    matches = cpl_ppm_match_points(rdata, ndata, 1, pattern, npattern, 0, 0.1,
                                   0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), npattern);
    cpl_test_eq(cpl_array_get_size(matches), npattern);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_matrix_delete(rdata);
    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_matrix_delete(pattern);

    /*
     * Now with a random distribution of 10 points. This time
     * the pattern is larger than the data set by 5 points. 
     * This means that points are missing on the data side.
     * The data matrix will be again rotated by 95 degrees and 
     * rescaled by a factor 2.35.
     */

    npattern = 15;
    ndata = 10;

    cpl_msg_info(cpl_func,
                 "%d random distributed data points "
                 "(%d in pattern, %d in data, i.e., with %d "
                 "missing detections), data rescaled "
                 "by 2.35 and rotated by 95 degrees...",
                 ndata, npattern, ndata, npattern - ndata);

    pattern = cpl_matrix_new(2, npattern);
    for (k = 0, i = 0; i < 2; i++) {
        for (j = 0; j < npattern; j++, k++) {
            cpl_matrix_set(pattern, i, j, pointdata[permute[k]]);
        }
    }

    data = cpl_matrix_new(2, npattern);
    for (k = 0, i = 0; i < 2; i++) {
        for (j = 0; j < npattern; j++, k++) {
            cpl_matrix_set(data, i, j, pointdata[k]);
        }
    }

    cpl_matrix_erase_columns(data, ndata, npattern - ndata);
    cpl_matrix_multiply_scalar(data, 2.35);
    rotate = cpl_matrix_new(2, 2);
    cpl_matrix_set(rotate, 0, 0, cos(95. / 180. * CPL_MATH_PI));
    cpl_matrix_set(rotate, 0, 1, -sin(95. / 180. * CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 0, sin(95. / 180. * CPL_MATH_PI));
    cpl_matrix_set(rotate, 1, 1, cos(95. / 180. * CPL_MATH_PI));

    rdata = cpl_matrix_product_create(rotate, data);

    cpl_matrix_delete(rotate);
    cpl_matrix_delete(data);

    matches = cpl_ppm_match_points(rdata, ndata, 1, pattern, npattern, 0, 0.1,
                                   0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), ndata);
    cpl_test_eq(cpl_array_get_size(matches), npattern);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_matrix_delete(rdata);
    cpl_array_delete(matches);
    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_matrix_delete(pattern);

    /*
     * Now with a random distribution of 20 aligned points. 
     * The whole data and the whole pattern (limited to 10 points)
     * are used. This means that a 50% contamination is present
     * on the data side already at the first pattern matching step.
     * The data matrix will be rescaled by a factor 2.35.
     */

    npattern = 10;
    ndata = 20;

    cpl_msg_info(cpl_func,
                 "%d random distributed aligned data points "
                 "(%d in pattern, %d in data, i.e., including %d "
                 "false-detections), data rescaled by 2.35...",
                 ndata, npattern, ndata, ndata - npattern);

    pattern = cpl_matrix_new(2, ndata);
    for (j = 0; j < ndata; j++, k++)
        cpl_matrix_set(pattern, 0, j, pointdata[k]);
    for (j = 0; j < ndata; j++, k++)
        cpl_matrix_set(pattern, 1, j, 0.0);

    data = cpl_matrix_duplicate(pattern);
    cpl_matrix_erase_columns(pattern, npattern, ndata - npattern);
    cpl_matrix_multiply_scalar(data, 2.35);

    matches = cpl_ppm_match_points(data, ndata, 1, pattern, npattern, 0, 0.1,
                                   0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), npattern);
    cpl_test_eq(cpl_array_get_size(matches), npattern);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_array_delete(matches);

    /* Use data also as pattern */
    matches = cpl_ppm_match_points(data, ndata, 1, data, ndata, 0, 0.1, 0.1,
                                   &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matches);
    cpl_test_nonnull(mpattern);
    cpl_test_nonnull(mdata);
    cpl_test_eq(cpl_matrix_get_ncol(mpattern), ndata);
    cpl_test_eq(cpl_array_get_size(matches), ndata);

    cpl_msg_info(cpl_func, "%" CPL_SIZE_FORMAT " points matched",
                 cpl_matrix_get_ncol(mpattern));

    cpl_matrix_delete(mpattern);
    cpl_matrix_delete(mdata);
    cpl_array_delete(matches);

    cpl_msg_info(cpl_func, "Check handling of NULL input");
    nullarray = cpl_ppm_match_points(NULL, ndata, 1, pattern, npattern, 0, 0.1,
                                     0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    nullarray = cpl_ppm_match_points(data, ndata, 1, NULL, npattern, 0, 0.1,
                                     0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    nullarray = cpl_ppm_match_points(data, ndata, 1, pattern, npattern, 0, 0.1,
                                     0.1, NULL, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);

    nullarray = cpl_ppm_match_points(data, ndata, 1, pattern, npattern, 0, 0.1,
                                     0.1, &mdata, NULL, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mdata);

    cpl_msg_info(cpl_func, "Check handling of other invalid input");

    /* ndata too small */
    nullarray = cpl_ppm_match_points(data, 2, 1, pattern, npattern, 0, 0.1, 0.1,
                                     &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* ndata too large */
    nullarray = cpl_ppm_match_points(data, ndata + 1, 1, pattern, npattern, 0,
                                     0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* npattern too large */
    nullarray = cpl_ppm_match_points(data, ndata, 1, pattern, npattern + 1, 0,
                                     0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ACCESS_OUT_OF_RANGE);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* npattern too small */
    nullarray = cpl_ppm_match_points(data, ndata, 1, pattern, 2, 0, 0.1, 0.1,
                                     &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* err_data and err_pattern non-positive */
    nullarray = cpl_ppm_match_points(data, ndata, 0.0, pattern, npattern, 0.0,
                                     0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* tolerance negative */
    nullarray = cpl_ppm_match_points(data, ndata, 1.0, pattern, npattern, 0.0,
                                     -0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* radius negative */
    nullarray = cpl_ppm_match_points(data, ndata, 1.0, pattern, npattern, 0.0,
                                     0.1, -0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* Too few columns in data */
    twobytwo = cpl_matrix_new(2, 2);
    matches = cpl_ppm_match_points(twobytwo, ndata, 1, pattern, npattern, 0,
                                   0.1, 0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    /* Too few columns in pattern */
    matches = cpl_ppm_match_points(data, ndata, 1, twobytwo, npattern, 0, 0.1,
                                   0.1, &mdata, &mpattern, NULL, NULL);

    cpl_test_error(CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(nullarray);
    cpl_test_null(mpattern);
    cpl_test_null(mdata);

    cpl_matrix_delete(twobytwo);
    cpl_matrix_delete(data);
    cpl_matrix_delete(pattern);


    /*
     * Testing cpl_ppm_match_positions()
     */

    cpl_vector *peaks = NULL;
    cpl_vector *lines = NULL;
    cpl_bivector *matched_positions = NULL;

    /*
     * Test the case where peaks and lines are identical
     */

    cpl_msg_info(cpl_func, "Testing cpl_ppm_match_positions(): "
                           "identical data and pattern");

    peaks = cpl_vector_wrap(4, (double *)&positions[2]);
    lines = cpl_vector_wrap(4, (double *)&positions[2]);

    matched_positions =
        cpl_ppm_match_positions(peaks, lines, 0.99, 1.01, 1.0, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matched_positions);
    cpl_test_eq(cpl_bivector_get_size(matched_positions),
                cpl_vector_get_size(peaks) - 1);

    cpl_bivector_delete(matched_positions);
    cpl_vector_unwrap(peaks);

    /* add additional peaks at the end */
    cpl_msg_info(cpl_func, "Testing cpl_ppm_match_positions(): "
                           "extra data (not in the pattern) at the end");

    peaks = cpl_vector_wrap(6, (double *)&positions[2]);
    matched_positions =
        cpl_ppm_match_positions(peaks, lines, 0.9, 1.1, 0.9, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matched_positions);
    cpl_test_eq(cpl_bivector_get_size(matched_positions),
                cpl_vector_get_size(lines) - 1);

    cpl_bivector_delete(matched_positions);
    cpl_vector_unwrap(peaks);

    /* add additional peaks at both ends */
    cpl_msg_info(cpl_func, "Testing cpl_ppm_match_positions(): "
                           "extra data (not in the pattern) at both ends");

    peaks = cpl_vector_wrap(8, (double *)&positions[0]);
    matched_positions =
        cpl_ppm_match_positions(peaks, lines, 0.9, 1.1, 0.9, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matched_positions);
    cpl_test_eq(cpl_bivector_get_size(matched_positions),
                cpl_vector_get_size(lines) - 1);

    cpl_bivector_delete(matched_positions);
    cpl_vector_unwrap(peaks);
    cpl_vector_unwrap(lines);

    /* inverse scenario: extra lines at both ends compared to the peaks */
    cpl_msg_info(cpl_func,
                 "Testing cpl_ppm_match_positions(): "
                 "extra lines in the pattern (not in the data) at both ends");

    peaks = cpl_vector_wrap(4, (double *)&positions[2]);
    lines = cpl_vector_wrap(8, (double *)&positions[0]);

    matched_positions =
        cpl_ppm_match_positions(peaks, lines, 0.9, 1.1, 0.9, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matched_positions);
    cpl_test_eq(cpl_bivector_get_size(matched_positions),
                cpl_vector_get_size(peaks) - 1);

    cpl_bivector_delete(matched_positions);
    cpl_vector_unwrap(peaks);
    cpl_vector_unwrap(lines);

    /*
     * Test real life scenario
     */
    cpl_msg_info(cpl_func, "Testing cpl_ppm_match_positions(): "
                           "real life wavelength calibration test case");

    peaks = cpl_vector_wrap(CX_N_ELEMENTS(peakdata), (double *)&peakdata[0]);
    lines = cpl_vector_wrap(CX_N_ELEMENTS(linedata), (double *)&linedata[0]);

    matched_positions =
        cpl_ppm_match_positions(peaks, lines, min_disp, max_disp, tolerance,
                                NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(matched_positions);
    cpl_test_eq(cpl_bivector_get_size(matched_positions),
                CX_N_ELEMENTS(reference_matches));

    const double *matches_x = cpl_bivector_get_x_data_const(matched_positions);
    const double *matches_y = cpl_bivector_get_y_data_const(matched_positions);

    const double epsilon = 100. * DBL_EPSILON;

    int false_matches = 0;
    size_t imatch;
    for (imatch = 0; imatch < CX_N_ELEMENTS(reference_matches); ++imatch) {
        double dx = fabs(matches_x[imatch] - reference_matches[imatch].x);
        double dy = fabs(matches_y[imatch] - reference_matches[imatch].y);
        if ((dx > epsilon) || (dy > epsilon)) {
            ++false_matches;
        }
    }
    cpl_test_zero(false_matches);

    cpl_bivector_delete(matched_positions);
    cpl_vector_unwrap(peaks);
    cpl_vector_unwrap(lines);

    /*
     * Test error handling
     */

    /* Check for NULL return when there are less than 4 peaks in input. */
    cpl_msg_info(cpl_func, "Testing cpl_ppm_match_positions(): "
                           "error handling: insufficient data");

    peaks = cpl_vector_wrap(3, (double *)&positions[2]);
    lines = cpl_vector_wrap(3, (double *)&positions[2]);

    matched_positions =
        cpl_ppm_match_positions(peaks, lines, 0.9, 1.1, 0.9, NULL, NULL);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_null(matched_positions);

    /* Check unsupported output arrays */
    cpl_msg_info(cpl_func, "Testing cpl_ppm_match_positions(): "
                           "error handling: use of unsupported output arrays");

    cpl_array *seq_peaks = cpl_array_new(3, CPL_TYPE_DOUBLE);
    cpl_array *seq_lines = cpl_array_new(3, CPL_TYPE_DOUBLE);

    matched_positions = cpl_ppm_match_positions(peaks, lines, 1.0, 1.0, 1.0,
                                                &seq_peaks, &seq_lines);
    cpl_test_error(CPL_ERROR_UNSUPPORTED_MODE);
    cpl_test_null(matched_positions);

    cpl_array_delete(seq_peaks);
    cpl_array_delete(seq_lines);

    /* Check null input handling */
    cpl_msg_info(cpl_func, "Testing cpl_ppm_match_positions(): "
                           "error handling: NULL input");

    matched_positions =
        cpl_ppm_match_positions(NULL, lines, 0.0, 0.0, 0.0, NULL, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(matched_positions);

    matched_positions =
        cpl_ppm_match_positions(peaks, NULL, 0.0, 0.0, 0.0, NULL, NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(matched_positions);

    cpl_vector_unwrap(peaks);
    cpl_vector_unwrap(lines);

    return cpl_test_end(0);
}
