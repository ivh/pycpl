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

/* Type dependent macros */
#if CPL_CLASS == CPL_CLASS_DOUBLE
#define CPL_TYPE          double
#define CPL_TYPE_T        CPL_TYPE_DOUBLE
#define CPL_ADD_FLOPS_ADD cpl_tools_add_flops

#elif CPL_CLASS == CPL_CLASS_FLOAT
#define CPL_TYPE          float
#define CPL_TYPE_T        CPL_TYPE_FLOAT
#define CPL_ADD_FLOPS_ADD cpl_tools_add_flops

#elif CPL_CLASS == CPL_CLASS_INT
#define CPL_TYPE             int
#define CPL_TYPE_T           CPL_TYPE_INT
#define CPL_ADD_FLOPS_ADD(N) /* N integer ops */

#else
#undef CPL_TYPE
#undef CPL_TYPE_T
#endif

#define CPL_TYPE_ADD(a) CPL_CONCAT2X(a, CPL_TYPE)

#if CPL_OPERATION == CPL_IMAGE_STATS_ALL

case CPL_TYPE_T: {
    /* Point to first pixel in first row to read */
    const CPL_TYPE *pi =
        (const CPL_TYPE *)image->pixels + (llysz - 1) * image->nx;
    /* - ditto for bad pixel map */
    const cpl_binary *pbpm = badmap + (llysz - 1) * image->nx;
    /* Avoid in-loop casting */
    const CPL_TYPE *pmin = NULL;
    const CPL_TYPE *pmax = NULL;
    CPL_TYPE *goodbuft = (CPL_TYPE *)cpl_ifalloc_get(&goodbuf);
    double pix_sum = 0.0;
    double sqr_sum = 0.0;
    double abs_sum = 0.0;
    double pix_mean = 0.0;
    double pix_var = 0.0; /* The accumulated variance sum */
    cpl_size max_pos;
    cpl_size min_pos;
    double ipix = 0.0; /* Counter of pixels used */

    assert(goodbuft != NULL);

    for (size_t j = llysz - 1; j < urysz;
         j++, pi += image->nx, pbpm += image->nx) {
        for (size_t i = llxsz - 1; i < urxsz; i++) {
            if (!badmap || !pbpm[i]) {
                const double delta = (double)pi[i] - pix_mean;

                pix_var += ipix * delta * (delta / (ipix + 1.0));
                pix_mean += delta / (ipix + 1.0);
                ipix += 1.0;

                if (pmin == NULL)
                    pmax = pmin = pi + i;
                else if (pi[i] < *pmin)
                    pmin = pi + i;
                else if (pi[i] > *pmax)
                    pmax = pi + i;

                pix_sum += (double)pi[i];
                abs_sum += fabs((double)pi[i]);
                sqr_sum += (double)pi[i] * (double)pi[i];
            }
        }
    }
    assert(pmin >= (const CPL_TYPE *)image->pixels);
    assert(pmax >= (const CPL_TYPE *)image->pixels);

    /* Compute the median */
    self->med = CPL_TYPE_ADD(cpl_tools_get_median)(goodbufg, npix);

    self->mean = pix_mean;

    /* Compute the bias-corrected standard deviation. */
    self->stdev = npix < 2 ? 0.0 : sqrt(pix_var / (double)(npix - 1));

    min_pos = (cpl_size)(pmin - (const CPL_TYPE *)image->pixels);
    max_pos = (cpl_size)(pmax - (const CPL_TYPE *)image->pixels);

    self->min_x = 1 + min_pos % image->nx;
    self->min_y = 1 + min_pos / image->nx;
    self->max_x = 1 + max_pos % image->nx;
    self->max_y = 1 + max_pos / image->nx;

    self->min = (double)*pmin;
    self->max = (double)*pmax;

    self->flux = pix_sum;
    self->absflux = abs_sum;
    self->sqflux = sqr_sum;

    CPL_ADD_FLOPS_ADD(17 * npix);
    break;
}

#elif CPL_OPERATION == CPL_IMAGE_STATS_VARIANCE

case CPL_TYPE_T: {
    double pix_mean = 0.0;
    double pix_var = 0.0; /* The accumulated variance sum */


    if (goodbufg != NULL) {
        const CPL_TYPE *goodbuft = (const CPL_TYPE *)goodbufg;

        assert(goodbuft != NULL);

        if (mode & CPL_STATS_STDEV && npix > 1) {
            for (size_t i = 0; i < (size_t)npix; i++) {
                const double delta = (double)goodbuft[i] - pix_mean;

                pix_var += i * delta * (delta / (i + 1));
                pix_mean += delta / (i + 1);
            }
        }
        else { /* Just mean is by far the most common */
            for (size_t i = 0; i < (size_t)npix; i++) {
                pix_mean += ((double)goodbuft[i] - pix_mean) / (i + 1);
            }
        }
    }
    else if (badmap == NULL) {
        /* Point to first pixel in first row to read */
        const CPL_TYPE *pi =
            (const CPL_TYPE *)image->pixels + (llysz - 1) * image->nx;
        const size_t irow = urxsz - (llxsz - 1); /* Number of pixels per row */
        const size_t iskip = image->nx - irow;   /* Skipped pixels per row */

        pi += llxsz - 1; /* Point to first pixel to read */

        if (mode & CPL_STATS_STDEV && npix > 1) {
            for (size_t i = 0, iend = 0, j = llysz - 1; j < urysz;
                 j++, pi += iskip) { /* Adjust pi so windowed index works */
                for (iend += irow; i < iend; i++) {
                    const double delta = (double)pi[i] - pix_mean;

                    pix_var += i * delta * (delta / (i + 1));
                    pix_mean += delta / (i + 1);
                }
            }
        }
        else { /* Just mean is by far the most common */
            for (size_t i = 0, iend = 0, j = llysz - 1; j < urysz;
                 j++, pi += iskip) { /* Adjust pi so windowed index works */
                for (iend += irow; i < iend; i++) {
                    pix_mean += ((double)pi[i] - pix_mean) / (i + 1);
                }
            }
        }
    }
    else {
        /* Point to first pixel in first row to read */
        const CPL_TYPE *pi =
            (const CPL_TYPE *)image->pixels + (llysz - 1) * image->nx;
        /* - ditto for bad pixel map */
        const cpl_binary *pbpm = badmap + (llysz - 1) * image->nx;
        double ipix = 0.0; /* Counter of pixels used */

        if (mode & CPL_STATS_STDEV && npix > 1) {
            for (size_t j = llysz - 1; j < urysz;
                 j++, pi += image->nx, pbpm += image->nx) {
                for (size_t i = llxsz - 1; i < urxsz; i++) {
                    if (!pbpm[i]) {
                        const double delta = (double)pi[i] - pix_mean;

                        pix_var += ipix * delta * (delta / (ipix + 1.0));
                        pix_mean += delta / (ipix + 1.0);
                        ipix += 1.0;
                    }
                }
            }
        }
        else { /* Just mean is by far the most common */
            for (size_t j = llysz - 1; j < urysz;
                 j++, pi += image->nx, pbpm += image->nx) {
                for (size_t i = llxsz - 1; i < urxsz; i++) {
                    if (!pbpm[i]) {
                        ipix += 1.0;
                        pix_mean += ((double)pi[i] - pix_mean) / ipix;
                    }
                }
            }
        }
    }

    if (mode & CPL_STATS_MEAN)
        self->mean = pix_mean;

    if (mode & CPL_STATS_STDEV) {
        /* Compute the bias-corrected standard deviation. */
        self->stdev = npix < 2 ? 0.0 : sqrt(pix_var / (double)(npix - 1));
    }

    CPL_ADD_FLOPS_ADD(npix * ((mode & CPL_STATS_STDEV) ? 7 : 4));
    break;
}

#elif CPL_OPERATION == CPL_IMAGE_STATS_CENTROID

case CPL_TYPE_T: {
    /* Point to first pixel in first row to read */
    const CPL_TYPE *pi =
        (const CPL_TYPE *)image->pixels + (llysz - 1) * image->nx;
    /* - ditto for bad pixel map */
    const cpl_binary *pbpm = badmap + (llysz - 1) * image->nx;
    const double min_pix_tmp = CPL_MIN(self->min, 0.0);

    for (size_t j = llysz - 1; j < urysz;
         j++, pi += image->nx, pbpm += image->nx) {
        for (size_t i = llxsz - 1; i < urxsz; i++) {
            if (!badmap || !pbpm[i]) {
                sum_xz += ((double)pi[i] - min_pix_tmp) * (double)(i + 1);
                sum_yz += ((double)pi[i] - min_pix_tmp) * (double)(j + 1);
                sum_z += (double)pi[i] - min_pix_tmp;
                sum_x += (double)(i + 1);
                sum_y += (double)(j + 1);
            }
        }
    }
    if (sum_z < 0.0)
        sum_z = 0.0; /* Can only become negative due to rounding */
    if (sum_xz < 0.0)
        sum_xz = 0.0; /* Can only become negative due to rounding */
    if (sum_yz < 0.0)
        sum_yz = 0.0; /* Can only become negative due to rounding */
    CPL_ADD_FLOPS_ADD(8 * npix);
    break;
}

#elif CPL_OPERATION == CPL_IMAGE_STATS_MINMAX

case CPL_TYPE_T: {
    double max_pix = DBL_MAX; /* Avoid (false) uninit warning */
    double min_pix = DBL_MAX; /* Avoid (false) uninit warning */


    if (goodbufg != NULL && !(mode & (CPL_STATS_MINPOS | CPL_STATS_MAXPOS))) {
        /* Median has already been computed on this buffer, so min and max
           are more easily found (and their positions are not requested) */
        const CPL_TYPE *goodbuft = (const CPL_TYPE *)goodbufg;

        if (mode & CPL_STATS_MIN) {
            /* Avoid in-loop casting */
            CPL_TYPE min_tmp = goodbuft[0];
            for (size_t i = 1; i < (size_t)npix / 2; i++) {
                if (goodbuft[i] < min_tmp)
                    min_tmp = goodbuft[i];
            }
            min_pix = (double)min_tmp;
            CPL_ADD_FLOPS_ADD(npix / 2);
        }
        if (mode & CPL_STATS_MAX) {
            /* Avoid in-loop casting */
            CPL_TYPE max_tmp = goodbuft[npix - 1];
            for (size_t i = npix - 1; i > ((size_t)npix + 1) / 2; i--) {
                if (goodbuft[i - 1] > max_tmp)
                    max_tmp = goodbuft[i - 1];
            }
            max_pix = (double)max_tmp;
            CPL_ADD_FLOPS_ADD(npix / 2);
        }
    }
    else if (mode & CPL_STATS_MIN) { /* Max much more common than min */
        /* Point to first pixel in first row to read */
        const CPL_TYPE *pi =
            (const CPL_TYPE *)image->pixels + (llysz - 1) * image->nx;
        /* - ditto for bad pixel map */
        const cpl_binary *pbpm = badmap + (llysz - 1) * image->nx;
        /* Avoid in-loop casting */
        const CPL_TYPE *pmin = badmap == NULL ? pi + llxsz - 1 : NULL;
        const CPL_TYPE *pmax = badmap == NULL ? pi + llxsz - 1 : NULL;

        if (badmap == NULL) {
            for (size_t j = llysz - 1; j < urysz; j++, pi += image->nx) {
                for (size_t i = llxsz - 1; i < urxsz; i++) {
                    if (pi[i] < *pmin)
                        pmin = pi + i;
                    else if (pi[i] > *pmax)
                        pmax = pi + i;
                }
            }
        }
        else {
            for (size_t j = llysz - 1; j < urysz;
                 j++, pi += image->nx, pbpm += image->nx) {
                for (size_t i = llxsz - 1; i < urxsz; i++) {
                    if (!badmap || !pbpm[i]) {
                        if (pmin == NULL)
                            pmax = pmin = pi + i;
                        else if (pi[i] < *pmin)
                            pmin = pi + i;
                        else if (pi[i] > *pmax)
                            pmax = pi + i;
                    }
                }
            }
        }
        assert(pmin >= (const CPL_TYPE *)image->pixels);
        assert(pmax >= (const CPL_TYPE *)image->pixels);

        min_pix = (double)*pmin;
        max_pix = (double)*pmax;

        if (mode & CPL_STATS_MIN) {
            const cpl_size min_pos =
                (cpl_size)(pmin - (const CPL_TYPE *)image->pixels);
            mode |= CPL_STATS_MINPOS; /* Implied */
            self->min_x = 1 + min_pos % image->nx;
            self->min_y = 1 + min_pos / image->nx;
        }
        if (mode & CPL_STATS_MAX) {
            const cpl_size max_pos =
                (cpl_size)(pmax - (const CPL_TYPE *)image->pixels);
            mode |= CPL_STATS_MAXPOS; /* Implied */
            self->max_x = 1 + max_pos % image->nx;
            self->max_y = 1 + max_pos / image->nx;
        }
        CPL_ADD_FLOPS_ADD(2 * npix);
    }
    else {
        /* Point to first pixel in first row to read */
        const CPL_TYPE *pi =
            (const CPL_TYPE *)image->pixels + (llysz - 1) * image->nx;
        /* Avoid in-loop casting */
        const CPL_TYPE *pmax = badmap == NULL ? pi + llxsz - 1 : NULL;

        if (badmap == NULL) {
            for (size_t j = llysz - 1; j < urysz; j++, pi += image->nx) {
                for (size_t i = llxsz - 1; i < urxsz; i++) {
                    if (pi[i] > *pmax)
                        pmax = pi + i;
                }
            }
        }
        else {
            /* Point to first badflag in first row to read */
            const cpl_binary *pbpm = badmap + (llysz - 1) * image->nx;

            for (size_t j = llysz - 1; j < urysz;
                 j++, pi += image->nx, pbpm += image->nx) {
                for (size_t i = llxsz - 1; i < urxsz; i++) {
                    if (!badmap || !pbpm[i]) {
                        if (pmax == NULL)
                            pmax = pi + i;
                        else if (pi[i] > *pmax)
                            pmax = pi + i;
                    }
                }
            }
        }
        assert(pmax >= (const CPL_TYPE *)image->pixels);

        max_pix = (double)*pmax;

        if (mode & CPL_STATS_MAX) {
            const cpl_size max_pos =
                (cpl_size)(pmax - (const CPL_TYPE *)image->pixels);
            mode |= CPL_STATS_MAXPOS; /* Implied */
            self->max_x = 1 + max_pos % image->nx;
            self->max_y = 1 + max_pos / image->nx;
        }
        CPL_ADD_FLOPS_ADD(npix);
    }
    if (mode & CPL_STATS_MIN)
        self->min = min_pix;
    if (mode & CPL_STATS_MAX)
        self->max = max_pix;
    break;
}

#elif CPL_OPERATION == CPL_IMAGE_STATS_FLUX

case CPL_TYPE_T: {
    double pix_sum = 0.0;
    double sqr_sum = 0.0;
    double abs_sum = 0.0;


    if (goodbufg == NULL) {
        /* Point to first pixel in first row to read */
        const CPL_TYPE *pi =
            (const CPL_TYPE *)image->pixels + (llysz - 1) * image->nx;
        /* - ditto for bad pixel map */
        const cpl_binary *pbpm = badmap + (llysz - 1) * image->nx;

        if (mode & (CPL_STATS_ABSFLUX | CPL_STATS_SQFLUX)) {
            for (size_t j = llysz - 1; j < urysz;
                 j++, pi += image->nx, pbpm += image->nx) {
                for (size_t i = llxsz - 1; i < urxsz; i++) {
                    if (!badmap || !pbpm[i]) {
                        pix_sum += (double)pi[i];
                        abs_sum += fabs((double)pi[i]);
                        sqr_sum += (double)pi[i] * (double)pi[i];
                    }
                }
            }
        }
        else if (badmap != NULL) { /* Just sum is the most common */
            for (size_t j = llysz - 1; j < urysz;
                 j++, pi += image->nx, pbpm += image->nx) {
                for (size_t i = llxsz - 1; i < urxsz; i++) {
                    if (!pbpm[i]) {
                        pix_sum += (double)pi[i];
                    }
                }
            }
        }
        else {
            for (size_t j = llysz - 1; j < urysz;
                 j++, pi += image->nx, pbpm += image->nx) {
                for (size_t i = llxsz - 1; i < urxsz; i++) {
                    pix_sum += (double)pi[i];
                }
            }
        }
    }
    else {
        const CPL_TYPE *goodbuft = (const CPL_TYPE *)goodbufg;

        assert(goodbuft != NULL);

        for (size_t i = 0; i < (size_t)npix; i++) {
            pix_sum += (double)goodbuft[i];
            abs_sum += fabs((double)goodbuft[i]);
            sqr_sum += (double)goodbuft[i] * (double)goodbuft[i];
        }
    }

    if (mode & CPL_STATS_FLUX)
        self->flux = pix_sum;
    if (mode & CPL_STATS_ABSFLUX)
        self->absflux = abs_sum;
    if (mode & CPL_STATS_SQFLUX)
        self->sqflux = sqr_sum;

    CPL_ADD_FLOPS_ADD(5 * npix);
    break;
}

#elif CPL_OPERATION == CPL_IMAGE_STATS_MEDIAN

case CPL_TYPE_T: {
    /* For now just prepare the temporary buffer of good pixels
       - to potentially be reused in other operations */
    CPL_TYPE *goodbuft = (CPL_TYPE *)cpl_ifalloc_get(&goodbuf);
    goodbufg = (void *)goodbuft; /* For later reuse */

    if (badmap == NULL) {
        /* All pixels are good */

        /* Cannot fail here */
        (void)cpl_tools_copy_window(goodbufg, image->pixels, sizeof(CPL_TYPE),
                                    image->nx, image->ny, llxsz, llysz, urxsz,
                                    urysz);
    }
    else {
        /* Point to first pixel in first row to read */
        const CPL_TYPE *pi =
            (const CPL_TYPE *)image->pixels + (llysz - 1) * image->nx;
        /* - ditto for bad pixel map */
        const cpl_binary *pbpm = badmap + (llysz - 1) * image->nx;


        /* Count also the number of good pixels */
        npix = 0;
        for (size_t j = llysz - 1; j < urysz;
             j++, pi += image->nx, pbpm += image->nx) {
            for (size_t i = llxsz - 1; i < urxsz; i++) {
                /* Take only good pixels */
                if (!pbpm[i]) {
                    goodbuft[npix++] = pi[i];
                }
            }
        }

        if (mpix == npix) {
            badmap = NULL;
        }
        else if (npix == 0) { /* Verify that there are good pixels */
            cpl_ifalloc_free(&goodbuf);
            return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        }
    }

    break;
}

#elif CPL_OPERATION == CPL_IMAGE_STATS_MEDIAN_DEV

case CPL_TYPE_T: {
    const CPL_TYPE *goodbuft = (const CPL_TYPE *)goodbufg;
    double dev_sum = 0.0;

    assert(goodbuft != NULL);

    for (size_t i = 0; i < (size_t)npix; i++) {
        dev_sum += fabs((double)goodbuft[i] - self->med);
    }

    self->med_dev = dev_sum / (double)npix;
    cpl_tools_add_flops(3 * npix + 1);
    break;
}

#elif CPL_OPERATION == CPL_IMAGE_STATS_MAD

case CPL_TYPE_T: {
    CPL_TYPE *goodbuft = (CPL_TYPE *)goodbufg;

    assert(goodbuft != NULL);

    for (size_t i = 0; i < (size_t)npix; i++) {
        goodbuft[i] = fabs((double)goodbuft[i] - self->med);
    }

    cpl_tools_add_flops(2 * npix);

    /* Compute the median */
    self->mad = CPL_TYPE_ADD(cpl_tools_get_median)(goodbuft, npix);

    break;
}

#endif

#undef CPL_TYPE
#undef CPL_TYPE_T
#undef CPL_ADD_FLOPS_ADD

#undef CPL_TYPE_ADD
