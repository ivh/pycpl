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


/*----------------------------------------------------------------------------
                                   Includes
 ----------------------------------------------------------------------------*/

#include "cpl_wcs.h"

#include <cpl_memory.h>
#include <cpl_tools.h>
#include <cpl_error_impl.h>
#include <cpl_propertylist_impl.h>
#include <cpl_property_impl.h>
#include <cpl_math_const.h>
#include <cpl_errorstate.h>
#include "cpl_matrix_impl.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef CPL_WCS_INSTALLED /* If WCS is installed */

/*
 * Undefine PACKAGE_ symbols to avoid redefinition warnings from the compiler.
 * Although these symbols are meant for internal use wcslib exports them in
 * as part of a public header.
 */

#ifdef PACKAGE_NAME
#undef PACKAGE_NAME
#endif

#ifdef PACKAGE_VERSION
#undef PACKAGE_VERSION
#endif

#ifdef PACKAGE_TARNAME
#undef PACKAGE_TARNAME
#endif

#ifdef PACKAGE_STRING
#undef PACKAGE_STRING
#endif

#ifdef PACKAGE_BUGREPORT
#undef PACKAGE_BUGREPORT
#endif

#include <wcslib.h>
#endif /* End If WCS is installed */


/*---------------------------------------------------------------------------*/
/**
 * @defgroup cpl_wcs World Coordinate System
 *
 * This module provides functions to manipulate FITS World Coordinate Systems
 *
 * A @em cpl_wcs is an object containing a pointer to the WCSLIB structure
 * and the physical dimensions of the image from which the WCS was read.
 * The functionality provided includes general transformations between physical
 * and world coordinates as well as a few convenience routines for
 * x,y <=> RA,Dec transformations.
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_wcs.h"
 * @endcode
 */
/*---------------------------------------------------------------------------*/
/**@{*/

/*----------------------------------------------------------------------------
                                   Type definition
 ----------------------------------------------------------------------------*/

#ifdef CPL_WCS_INSTALLED /* If WCS is installed */
struct _cpl_wcs_
{
    struct wcsprm *wcsptr; /* WCSLIB structure */
    int istab;             /* Set if header is from a table */
    int naxis;             /* Number of dimensions of the image */
    cpl_array *imdims;     /* Dimensions of image */
    cpl_array *crval;      /* CRVALia keyvalues for each coord axis */
    cpl_array *crpix;      /* CRPIXja keyvalues for each pixel axis */
    cpl_array *ctype;      /* CTYPEja keyvalues for each pixel axis */
    cpl_array *cunit;      /* CUNITja keyvalues for each pixel axis */
    cpl_matrix *cd;        /* CDi_ja linear transformation matrix */
};

/*----------------------------------------------------------------------------
                                   Private functions
 ----------------------------------------------------------------------------*/

inline static cpl_wcs *
_cpl_wcs_init(struct wcsprm *wcs, int is_tab) CPL_ATTR_ALLOC;

inline static int _cpl_wcs_set_ctype(cpl_wcs *self);
inline static int _cpl_wcs_set_cunit(cpl_wcs *self);
inline static int _cpl_wcs_set_cd(cpl_wcs *self);

inline static int _cpl_wcsset(cpl_wcs *wcs);

static cpl_wcs *cpl_wcs_new_from_propertylist_(const cpl_propertylist *)
    CPL_ATTR_NONNULL CPL_ATTR_ALLOC;

static void cpl_wcs_platesol_4(const cpl_matrix *xy,
                               const cpl_matrix *std,
                               const cpl_array *bad,
                               cpl_array **plateconsts) CPL_ATTR_NONNULL;
static void cpl_wcs_platesol_6(const cpl_matrix *xy,
                               const cpl_matrix *std,
                               const cpl_array *bad,
                               cpl_array **plateconsts) CPL_ATTR_NONNULL;

#endif /* End If WCS is installed */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

/**
 * @brief
 *    Create a wcs structure by parsing a propertylist.
 *
 * @param plist    The input propertylist
 *
 * @return
 *   The newly created and filled cpl_wcs object or NULL if it could not be
 *   created. In the latter case an appropriate error code is set.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>plist</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_TYPE_MISMATCH</td>
 *       <td class="ecr">
 *         NAXIS information in image propertylist is not an integer
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         Error in getting NAXIS information for image propertylists
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSPECIFIED</td>
 *       <td class="ecr">
 *         An unspecified error occurred in the WCSLIB routine.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NO_WCS</td>
 *       <td class="ecr">
 *         The WCS sub library is not available.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function allocates memory for a WCS structure. A pointer to the WCSLIB
 * header information is created by parsing the FITS WCS keywords from the
 * header of a file. A few ancillary items are also filled in.
 *
 * It is allowed to pass a cpl_propertylist with a valid WCS structure and
 * NAXIS = 0, such a propertylist can be created by cpl_wcs_platesol().
 * In this case a cpl_wcs object is returned for which the dimensional
 * information (accessible via cpl_wcs_get_image_dims()) will be NULL.
 *
 * The returned property must be destroyed using the wcs destructor
 * @b cpl_wcs_delete().
 *
 * @see cpl_wcs_delete()
 */

cpl_wcs *
cpl_wcs_new_from_propertylist(const cpl_propertylist *plist)
{
    cpl_wcs *self = NULL;


    cpl_ensure(plist != NULL, CPL_ERROR_NULL_INPUT, NULL);

#ifdef CPL_WCS_INSTALLED /* If WCS is installed */

    self = cpl_wcs_new_from_propertylist_(plist);

    if (self == NULL)
        (void)cpl_error_set_where_();

#else
    (void)cpl_error_set_(CPL_ERROR_NO_WCS);
#endif /* End If WCS is installed */
    return self;
}

/**
 * @brief
 *    Destroy a WCS structure
 *
 * @param wcs  The WCS structure to destroy
 *
 * @return
 *   Nothing.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NO_WCS</td>
 *       <td class="ecr">
 *         The WCS sub library is not available.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 * The function destroys the WCS structure @em wcs and its whole
 * contents. If @em wcs is @c NULL, nothing is done and no error is set.
 */

void
cpl_wcs_delete(cpl_wcs *wcs)
{
    if (wcs == NULL)
        return;

#ifdef CPL_WCS_INSTALLED /* If WCS is installed */
    /* Free the workspace */

    if (wcs->imdims != NULL) {
        cpl_array_delete(wcs->imdims);
    }

    cpl_array_delete(wcs->cunit);
    cpl_array_delete(wcs->ctype);

    cpl_array_unwrap(wcs->crval);
    cpl_array_unwrap(wcs->crpix);

    cpl_matrix_delete(wcs->cd);

    if (wcs->wcsptr != NULL) {
        (void)wcsfree(wcs->wcsptr);
        cpl_free(wcs->wcsptr);
    }

    cpl_free(wcs);

#else
    cpl_error_set_(CPL_ERROR_NO_WCS);
#endif /* End If WCS is installed */
}

/**
 * @brief
 *    Convert between physical and world coordinates.
 *
 * @param wcs       The input cpl_wcs structure
 * @param from      The input coordinate matrix
 * @param to        The output coordinate matrix
 * @param status    The output status array
 * @param transform The transformation mode
 *
 * @return
 *   An appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>wcs</i>, <i>from</i>, <i>to</i> or <i>status</i> is a <tt>NULL</tt> pointer
 *         or <i>wcs</i> is missing some of its information.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSPECIFIED</td>
 *       <td class="ecr">
 *         No rows or columns in the input matrix, or an unspecified error
 *         has occurred in the WCSLIB routine.
 *         Alternatively, the number of columns in <i>from</i> and the
 *         NAXIS-value in <i>wcs</i> are different
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSUPPORTED_MODE</td>
 *       <td class="ecr">
 *         The input conversion mode is not supported
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NO_WCS</td>
 *       <td class="ecr">
 *         The WCS sub library is not available.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function converts between several types of coordinates. These include:
 * -- physical coordinates: The physical location on a detector (i.e.
 *                          pixel coordinates)
 * -- world coordinates: The real astronomical coordinate system for the
 *                       observations. This may be spectral, celestial,
 *                       time, etc.
 * -- standard coordinates: These are an intermediate relative coordinate
 *                          representation, defined as a distance from
 *                          a reference point in the natural units of the
 *                          world coordinate system. Any defined projection
 *                          geometry will have already been included in
 *                          the definition of standard coordinates.
 *
 * The supported conversion modes are:
 * -- CPL_WCS_PHYS2WORLD:  Converts input physical to world coordinates
 * -- CPL_WCS_WORLD2PHYS:  Converts input world to physical coordinates
 * -- CPL_WCS_WORLD2STD:   Converts input world to standard coordinates
 * -- CPL_WCS_PHYS2STD:    Converts input physical to standard coordinates
 *
 * The input cpl_matrix @b from has to be filled with coordinates. The number of
 * rows equals the number of objects and the number of columns has to be equal to
 * the value of the NAXIS keyword in the @b wcs structure. The same convention is used
 * for the output cpl_matrix @b to. For example, if an image contains NAXIS = 2 and
 * 100 stars with positions X,Y, the new matrix will be created:
 * @code
 * from = cpl_matrix_new(100, 2);
 * @endcode
 * Each element in column 0 will take a X coordinate and each element in column 1
 * will take a Y coordinate.
 *
 * The output matrix and status arrays will be allocated here, and thus will
 * need to be freed by the calling routine. The status array is used to flag
 * input coordinates where there has been some sort of failure in the
 * transformation. For historical reasons, the output matrix and status arrays
 * are allocated also in case of some (but not all) failures. If not allocated,
 * they are set to NULL. Also for historical reasons, when allocated and
 * regardless whether the call succeeds, the status array has all its elements
 * defined yet all status elements are flagged as invalid.
 *
 */

cpl_error_code
cpl_wcs_convert(const cpl_wcs *wcs,
                const cpl_matrix *from,
                cpl_matrix **to,
                cpl_array **status,
                cpl_wcs_trans_mode transform)
{
#ifdef CPL_WCS_INSTALLED /* If WCS is installed */
    cpl_ifalloc mybuf;
    int *sdata;
    double *tdata, *x1data, *x2data, *x3data;
    const double *fdata;
    const char *wcsfunc = NULL;
    const cpl_size mrows = cpl_matrix_get_nrow(from);
    const cpl_size mcols = cpl_matrix_get_ncol(from);
    const int nrows = (int)mrows;
    const int ncols = (int)mcols;
    int retval = -1; /* Nonzero, assume failure */


    /* Basic checks on the input pointers */
    cpl_ensure_code(wcs != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(from != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(to != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(status != NULL, CPL_ERROR_NULL_INPUT);
    /* FIXME: Perhaps this should be an assertion ? */
    cpl_ensure_code(wcs->wcsptr != NULL, CPL_ERROR_NULL_INPUT);

    /* Initialise output */

    *to = NULL;
    *status = NULL;

    /* Check the number of rows/columns for the input matrix */

    cpl_ensure_code((cpl_size)nrows == mrows, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code((cpl_size)ncols == mcols, CPL_ERROR_ILLEGAL_INPUT);

    if (wcs->naxis > 0 && ncols != wcs->naxis) {
        /* FIXME: Use CPL_ERROR_INCOMPATIBLE_INPUT from next major release */
        return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                      "NAXIS=%d <=> %d "
                                      "matrix column(s)",
                                      wcs->naxis, ncols);
    }

    if (nrows == 0 || ncols == 0) {
        /* Unreachable without a bug in CPL */
        return cpl_error_set_(CPL_ERROR_UNSPECIFIED);
    }

    /* Now get the pointers for the data arrays */

    fdata = cpl_matrix_get_data_const_(from);

    /* Get the output memory - WCS may not fully set this on certain errors */
    tdata = (double *)cx_calloc(mrows * mcols, sizeof(double));
    *status = cpl_array_new(mrows, CPL_TYPE_INT);
    sdata = cpl_array_get_data_int(*status);

    /* Get some (stack) memory for wcslib to use, mrows doubles etc */
    cpl_ifalloc_set(&mybuf, (size_t)(mrows * (mcols + 2)) * sizeof(*x2data));
    x2data = (double *)cpl_ifalloc_get(&mybuf);
    x3data = x2data + mrows; /* mrows doubles */
    x1data = x3data + mrows; /* mrows * mcols doubles */

    /* Right, now switch for the transform type. First physical
       to world coordinates */

    switch (transform) {
        case CPL_WCS_PHYS2WORLD:
            wcsfunc = "wcsp2s";
            retval = wcsp2s(wcs->wcsptr, nrows, wcs->naxis, fdata, x1data,
                            x2data, x3data, tdata, sdata);
            break;
        case CPL_WCS_WORLD2PHYS:
            wcsfunc = "wcss2p";
            retval = wcss2p(wcs->wcsptr, nrows, wcs->naxis, fdata, x2data,
                            x3data, x1data, tdata, sdata);
            break;
        case CPL_WCS_WORLD2STD:
            wcsfunc = "wcss2p";
            retval = wcss2p(wcs->wcsptr, nrows, wcs->naxis, fdata, x2data,
                            x3data, tdata, x1data, sdata);
            break;
        case CPL_WCS_PHYS2STD:
            wcsfunc = "wcsp2s";
            retval = wcsp2s(wcs->wcsptr, nrows, wcs->naxis, fdata, tdata,
                            x2data, x3data, x1data, sdata);
            break;
        default:
            break;
    }

    /* Ditch the intermediate coordinate results */
    cpl_ifalloc_free(&mybuf);

    if (wcsfunc == NULL) {
        cx_free(tdata);
        cpl_array_delete(*status);
        *to = NULL;
        *status = NULL;
        return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    /* NB: With a CPL error due to the WCS call failing both status and to
       are still allocated! */
    *to = cpl_matrix_wrap(mrows, mcols, tdata);

    if (retval != 0)
        return cpl_error_set_wcs(retval == 1 ? CPL_ERROR_NULL_INPUT
                                             : CPL_ERROR_UNSPECIFIED,
                                 retval, wcsfunc, "transform=%d", transform);
    return CPL_ERROR_NONE;

#else
    cpl_ensure_code(wcs != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(from != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(to != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(status != NULL, CPL_ERROR_NULL_INPUT);

    switch (transform) {
        case CPL_WCS_PHYS2WORLD:
        case CPL_WCS_WORLD2PHYS:
        case CPL_WCS_WORLD2STD:
        case CPL_WCS_PHYS2STD:
            break;
        default:
            return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    return cpl_error_set_(CPL_ERROR_NO_WCS);
#endif /* End If WCS is installed */
}

/**
 * @brief
 *    Do a 2d plate solution given physical and celestial coordinates
 *
 * @param ilist     The input property list containing the first pass WCS
 * @param cel       The celestial coordinate matrix
 * @param xy        The physical coordinate matrix
 * @param niter     The number of fitting iterations
 * @param thresh    The threshold for the fitting rejection cycle
 * @param fitmode   The fitting mode (see below)
 * @param outmode   The output mode (see below)
 * @param olist     The output property list containing the new WCS
 *
 * @return
 *   An appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>cel</i> is a <tt>NULL</tt> pointer, the parameter
 *         <i>xy</i> is a <tt>NULL</tt> pointer or <i>ilist</i> is a
 *         <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>niter</i> is non-positive.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSPECIFIED</td>
 *       <td class="ecr">
 *         Unable to parse the input propertylist into a proper FITS WCS or
 *         there are too few points in the input matrices for a fit.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_INCOMPATIBLE_INPUT</td>
 *       <td class="ecr">
 *         The matrices <i>cel</i> and <i>xy</i> have different sizes.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_UNSUPPORTED_MODE</td>
 *       <td class="ecr">
 *         Either <i>fitmode</i> or <i>outmode</i> are specified incorrectly
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         The threshold is so low that no valid points are found. If the
 *         threshold is not positive, this error is certain to occur.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NO_WCS</td>
 *       <td class="ecr">
 *         The WCS sub library is not available.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function allows for the following type of fits:
 * -- CPL_WCS_PLATESOL_4:  Fit for zero point, 1 scale and 1 rotation.
 * -- CPL_WCS_PLATESOL_6:  Fit for zero point, 2 scales, 1 rotation, 1 shear.
 *
 * This function allows the zeropoint to be defined by shifting either the
 * physical or the celestial coordinates of the reference point:
 * -- CPL_WCS_MV_CRVAL: Keeps the physical point fixed and shifts the celestial
 * -- CPL_WCS_MV_CRPIX: Keeps the celestial point fixed and shifts the physical
 *
 * The output property list contains WCS relevant information only.
 *
 * The matrices @em cel, and @em xy have to be set up in the same way as it
 * is required for @b cpl_wcs_convert(). See the documentation of
 * @b cpl_wcs_convert() for details.
 *
 * @see cpl_wcs_convert()
 */

cpl_error_code
cpl_wcs_platesol(const cpl_propertylist *ilist,
                 const cpl_matrix *cel,
                 const cpl_matrix *xy,
                 int niter,
                 float thresh,
                 cpl_wcs_platesol_fitmode fitmode,
                 cpl_wcs_platesol_outmode outmode,
                 cpl_propertylist **olist)
{
#ifdef CPL_WCS_INSTALLED /* If WCS is installed */
    const cpl_size npts = cpl_matrix_get_nrow(cel);
    int iter, n, i;
    int *isbad;
    double xifit, etafit, mederr_xi, mederr_eta;
    double crval1, crval2, phi, theta;
    double crpix1 = 0.0; /* Avoid uninit warning (See below comment on pc) */
    double crpix2 = 0.0; /* Avoid uninit warning (See below comment on pc) */
    const double *xydata = cpl_matrix_get_data_const(xy);
    const double *pc = NULL;
    char *o;
    struct wcsprm *wp;
    cpl_wcs *wcs;
    cpl_matrix *std;
    const double *stddata;
    cpl_array *status, *bad, *plateconsts;
    double *eta_work;
    double *xi_work;
    double *med_work = NULL;
    int nbad = 0;
    int nprev = -1;
    cpl_error_code code = CPL_ERROR_NONE;
    int retval;

    /* Initialise the output pointer */

    cpl_ensure_code(olist != NULL, CPL_ERROR_NULL_INPUT);

    *olist = NULL;

    /* Basic checks on the input pointers */

    cpl_ensure_code(cel != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(xy != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(ilist != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(niter > 0, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code(fitmode == CPL_WCS_PLATESOL_6 ||
                        fitmode == CPL_WCS_PLATESOL_4,
                    CPL_ERROR_UNSUPPORTED_MODE);

    /* Open the cpl_wcs structure */

    wcs = cpl_wcs_new_from_propertylist_(ilist);
    if (wcs == NULL) {
        return cpl_error_set_message_(CPL_ERROR_UNSPECIFIED,
                                      "Unable to parse header");
    }

    /* Get the number of celestial points and compare this with the size of the
       matrix with the xy coordinates. Also look at the total number of
       points available */

    if (npts != cpl_matrix_get_nrow_(xy)) {
        cpl_wcs_delete(wcs);
        return cpl_error_set_(CPL_ERROR_INCOMPATIBLE_INPUT);
    }
    if (npts < 2) {
        cpl_wcs_delete(wcs);
        return cpl_error_set_message_(
            CPL_ERROR_UNSPECIFIED,
            "Insufficient "
            "points for a fit: npoints=%" CPL_SIZE_FORMAT " < 2",
            npts);
    }

    /* Convert the celestial coordinates to standard coordinates */

    cpl_wcs_convert(wcs, cel, &std, &status, CPL_WCS_WORLD2STD);
    cpl_array_delete(status);
    stddata = cpl_matrix_get_data_const(std);

    /* Get some workspace for rejection algorithm */
    eta_work = (double *)cpl_malloc((size_t)npts * sizeof(double));
    xi_work = (double *)cpl_malloc((size_t)npts * sizeof(double));

    /* Get an array to flag bad pairs */
    isbad = (int *)cpl_calloc((size_t)npts, sizeof(int));
    bad = cpl_array_wrap_int(isbad, npts);

    /* Iterative loop */
    plateconsts = NULL;
    for (iter = 1; iter <= niter && nprev < nbad && nbad + 1 < npts; iter++) {
        double mederr;

        /* Do a plate solution */
        cpl_array_delete(plateconsts);
        (fitmode == CPL_WCS_PLATESOL_6
             ? cpl_wcs_platesol_6
             : cpl_wcs_platesol_4)(xy, std, bad, &plateconsts);

        pc = cpl_array_get_data_double_const(plateconsts);

        /* Get the fit residuals */

        n = 0;
        for (i = 0; i < npts; i++) {
            if (!isbad[i]) {
                const double xifiti =
                    xydata[2 * i] * pc[0] + xydata[2 * i + 1] * pc[1] + pc[2];
                const double etafiti =
                    xydata[2 * i] * pc[3] + xydata[2 * i + 1] * pc[4] + pc[5];

                xi_work[n] = fabs(xifiti - stddata[2 * i]);
                eta_work[n] = fabs(etafiti - stddata[2 * i + 1]);
                n++;
            }
        }

        /* assert( n > 0 ); */

        if (iter < niter) { /* No rejections in last iteration */

            if (med_work == NULL) {
                /* Need copy due to permutation in median computation */
                med_work = (double *)cpl_malloc(2 * (size_t)n * sizeof(double));
            }

            /* Get the median of the array */
            (void)memcpy(med_work, xi_work, (size_t)n * sizeof(double));
            (void)memcpy(med_work + n, eta_work, (size_t)n * sizeof(double));
            mederr = 1.48 * cpl_tools_get_median_double(med_work, 2 * n);

            /* Now reject the bad ones... */
            nprev = nbad;
            for (i = 0; i < npts; i++) {
                if (!isbad[i] && (eta_work[i] > thresh * mederr ||
                                  xi_work[i] > thresh * mederr)) {
                    isbad[i] = 1;
                    nbad++;
                }
            }
        }
    }

    /* Do some intermediate tidying */
    cpl_matrix_delete(std);
    cpl_array_delete(bad);
    cpl_free(med_work);

    if (nbad == npts) {
        cpl_array_delete(plateconsts);
        cpl_wcs_delete(wcs);
        cpl_free(eta_work);
        cpl_free(xi_work);
        return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
    }

    cx_assert(pc != NULL);

    /* Now work out the median error in each axis */

    mederr_xi = 1.48 * cpl_tools_get_median_double(xi_work, n);
    mederr_eta = 1.48 * cpl_tools_get_median_double(eta_work, n);

    /* Do some intermediate tidying */
    cpl_free(eta_work);
    cpl_free(xi_work);

    /* Define the reference point result */

    wp = wcs->wcsptr;
    switch (outmode) {
        case CPL_WCS_MV_CRPIX:
            crpix1 = (pc[4] * pc[2] - pc[1] * pc[5]) /
                     (pc[3] * pc[1] - pc[4] * pc[0]);
            crpix2 = (pc[0] * pc[5] - pc[3] * pc[2]) /
                     (pc[3] * pc[1] - pc[4] * pc[0]);
            crval1 = (wp->crval)[0];
            crval2 = (wp->crval)[1];
            break;
        case CPL_WCS_MV_CRVAL: {
            int sdata[] = { 0 };
            crpix1 = (wp->crpix)[0];
            crpix2 = (wp->crpix)[1];
            xifit = crpix1 * pc[0] + crpix2 * pc[1] + pc[2];
            etafit = crpix1 * pc[3] + crpix2 * pc[4] + pc[5];
            retval = celx2s(&(wp->cel), 1, 1, 2, 2, &xifit, &etafit, &phi,
                            &theta, &crval1, &crval2, sdata);
            if (retval) {
                cpl_wcs_delete(wcs);
                cpl_array_delete(plateconsts);
                return cpl_error_set_wcs(CPL_ERROR_UNSPECIFIED, retval,
                                         "celx2s",
                                         "niter=%d, thresh=%f, "
                                         "fitmode=%d, outmode=%d",
                                         niter, (double)thresh, (int)fitmode,
                                         (int)outmode);
            }
            break;
        }
        default:
            cpl_wcs_delete(wcs);
            cpl_array_delete(plateconsts);
            return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    /* Now update the wcs structure */

    (wp->crval)[0] = crval1;
    (wp->crval)[1] = crval2;
    (wp->crpix)[0] = crpix1;
    (wp->crpix)[1] = crpix2;
    (wp->pc)[0] = pc[0];
    (wp->pc)[1] = pc[1];
    (wp->pc)[2] = pc[3];
    (wp->pc)[3] = pc[4];
    for (i = 0; i < 4; i++)
        (wp->cd)[i] = (wp->pc)[i];
    (wp->cdelt)[0] = 1.0;
    (wp->cdelt)[1] = 1.0;
    (wp->csyer)[0] = mederr_xi;
    (wp->csyer)[1] = mederr_eta;

    _cpl_wcsset(wcs);

    /* Make a FITS string and convert it to a propertylist */

    retval = wcshdo(0, wp, &i, &o);
    if (retval) {
        code = cpl_error_set_wcs(CPL_ERROR_NULL_INPUT, retval, "wcshdo",
                                 "niter=%d", niter);
    }
    else {
        /* The FITS string is far from valid:
         its size is not a multiple of 36 * 80, and
         it is missing several mandatory cards, e.g. SIMPLE, NAXIS, END.

         Since this puts limits on how it can be written and read by CFITSIO
         we instead read the string directly by the CPL FITS card parser. */

        char cardi[FLEN_CARD];
        cpl_errorstate prestate = cpl_errorstate_get();
        const cpl_cstr *erasekeys[] = {
            CXSTR("PC1_1", 5),   CXSTR("PC1_2", 5),   CXSTR("PC2_1", 5),
            CXSTR("PC2_2", 5),   CXSTR("CDELT1", 6),  CXSTR("CDELT2", 6),
            CXSTR("RESTFRQ", 7), CXSTR("RESTWAV", 7), CXSTR("END", 3)
        };
        const int nerasekeys = (int)CX_N_ELEMENTS(erasekeys);

        cpl_property *p = NULL;

        *olist = cpl_propertylist_new();

        /* Need null-terminator for parsing a card with a numerical value */
        cardi[FLEN_CARD - 1] = '\0';

        /* Convert each FITS card to a CPL property, ensuring that the WCS
           keys that need to be floating point are indeed that (e.g. PV1_1).
        */

        /*
         * Remove keywords which are not needed for the DICB required
         * CD representation of the linear transformation matrix.
         *
         * FIXME: What about the RESTFRQ, RESTWAV stuff? Needed or not?
         *
         */

        for (int k = 0; k < i; k++) {
            const char *cardk = o + k * (FLEN_CARD - 1); /* k'th card */

            /* Skip the card if its beginning matches */
            if (cpl_fits_card_check_memcmp(CXSTR(cardk, FLEN_CARD - 1),
                                           nerasekeys, erasekeys))
                continue;

            (void)memcpy(cardi, cardk, FLEN_CARD - 1);
            if (cpl_propertylist_append_from_string(*olist, cardi, NULL,
                                                    NULL)) {
                (void)cpl_error_set_message_(cpl_error_get_code(),
                                             "Bad WCS FITS Card %d/%d", k, i);
                break;
            }
        }
        free(o);

        /* Generate the coordinate transformation matrix elements and insert
           each right after CRPIX2, so reverse order for correct end order */
        for (i = 2; i > 0; i--) {
            char cd[] = "CD0_0"; /* Single digits only */

            cd[2] = (char)('0' + i); /* Set key - 1st dim */
            for (int j = 2; j > 0; j--) {
                const double cd_val = cpl_matrix_get(wcs->cd, i - 1, j - 1);

                cd[4] = (char)('0' + j); /* Set key - 2nd dim */

                if (p == NULL) {
                    p = cpl_property_new_cx(CXSTR(cd, 5), CPL_TYPE_DOUBLE);
                    cpl_property_set_comment_cx(p, CXSTR("Coordinate "
                                                         "transformation "
                                                         "matrix element",
                                                         40));
                }
                else {
                    cpl_property_set_name_cx(p, CXSTR(cd, 5));
                }

                cpl_property_set_double(p, cd_val);

                cpl_propertylist_insert_after_property(*olist, "CRPIX2", p);
            }
        }
        cpl_property_delete(p);

        if (!cpl_errorstate_is_equal(prestate))
            code = cpl_error_set_where_();
    }

    /* Tidy and exit */

    cpl_array_delete(plateconsts);
    cpl_wcs_delete(wcs);

    return cpl_error_set_(code);
#else

    cpl_ensure_code(olist != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(cel != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(xy != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(ilist != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(niter > 0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(thresh > 0.0, CPL_ERROR_DATA_NOT_FOUND);

    switch (fitmode) {
        case CPL_WCS_PLATESOL_6:
        case CPL_WCS_PLATESOL_4:
            break;
        default:
            return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    switch (outmode) {
        case CPL_WCS_MV_CRPIX:
        case CPL_WCS_MV_CRVAL:
            break;
        default:
            return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    return cpl_error_set_(CPL_ERROR_NO_WCS);
#endif /* End If WCS is installed */
}


#ifdef CPL_WCS_IS_TABLE

/**
 * @internal
 * @brief
 *    Accessor to say whether the original header was from an image or a table
 *
 * @param wcs  The WCS structure to examine
 *
 * @return
 *   A flag where 0 -> the header was an image type or 1 -> the header was
 *   a table type. If -1 is returned, then the header didn't parse correctly
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>wcs</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NO_WCS</td>
 *       <td class="ecr">
 *         The WCS sub library is not available.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 * The function returns the flag defining the type of input header.
 * If the header was not parsed correctly then this is -1.
 */

int
cpl_wcs_is_table(const cpl_wcs *wcs)
{
    /* Check for NULL input */
    cpl_ensure(wcs != NULL, CPL_ERROR_NULL_INPUT, -1);

#ifdef CPL_WCS_INSTALLED /* If WCS is installed */

    /* Return the relevant value */
    return (wcs->istab);
#else
    (void)cpl_error_set_(CPL_ERROR_NO_WCS);
    return -1;
#endif /* End If WCS is installed */
}
#endif

/**
 * @brief
 *    Accessor to get the dimensionality of the image associated with a WCS
 *
 * @param wcs  The WCS structure to examine
 *
 * @return
 *   The image dimensionality,
 *   or zero on error
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>wcs</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NO_WCS</td>
 *       <td class="ecr">
 *         The WCS sub library is not available.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 * The function returns the dimensionality of the image associated with a WCS.
 * If no image was used to define the WCS then a value of zero is returned.
 */

int
cpl_wcs_get_image_naxis(const cpl_wcs *wcs)
{
    /* Check for NULL input */
    cpl_ensure(wcs != NULL, CPL_ERROR_NULL_INPUT, 0);

#ifdef CPL_WCS_INSTALLED /* If WCS is installed */

    /* Return the relevant value */
    return (wcs->naxis);
#else
    (void)cpl_error_set_(CPL_ERROR_NO_WCS);
    return 0;
#endif /* End If WCS is installed */
}

/**
 * @brief
 *    Accessor to get the axis lengths of the image associated with a WCS
 *
 * @param wcs  The WCS structure to examine
 *
 * @return
 *   An array with the image axis sizes, or NULL on error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>wcs</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NO_WCS</td>
 *       <td class="ecr">
 *         The WCS sub library is not available.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 * The function returns a handle to an array with the axis lengths of the image
 * associated with this WCS. If no image was used to define the WCS then
 * a NULL value will be returned.
 */

const cpl_array *
cpl_wcs_get_image_dims(const cpl_wcs *wcs)
{
    /* Check for NULL input */
    cpl_ensure(wcs != NULL, CPL_ERROR_NULL_INPUT, NULL);

#ifdef CPL_WCS_INSTALLED /* If WCS is installed */

    /* Return the array, or NULL when unavailable */
    return wcs->naxis == 0 ? NULL : wcs->imdims;

#else
    (void)cpl_error_set_(CPL_ERROR_NO_WCS);
    return NULL;
#endif /* End If WCS is installed */
}

/**
 * @brief
 *    Accessor to get the CRVAL vector for a WCS
 *
 * @param wcs  The WCS structure to examine
 *
 * @return
 *   A handle to an array with the CRVALia keyvalues for each coord axis,
 *   or NULL on error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>wcs</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NO_WCS</td>
 *       <td class="ecr">
 *         The WCS sub library is not available.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 * The function returns a handle to an array with the CRVAL vector defined for
 * this WCS.
 */

const cpl_array *
cpl_wcs_get_crval(const cpl_wcs *wcs)
{
    /* Check for NULL input */
    cpl_ensure(wcs != NULL, CPL_ERROR_NULL_INPUT, NULL);

#ifdef CPL_WCS_INSTALLED /* If WCS is installed */

    return wcs->wcsptr->naxis == 0 ? NULL : wcs->crval;

#else
    (void)cpl_error_set_(CPL_ERROR_NO_WCS);
    return NULL;
#endif /* End If WCS is installed */
}

/**
 * @brief
 *    Accessor to get the CRPIX vector for a WCS
 *
 * @param wcs  The WCS structure to examine
 *
 * @return
 *   A handle to an array with the CRPIXja keyvalues for each pixel axis,
 *   or NULL on error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>wcs</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NO_WCS</td>
 *       <td class="ecr">
 *         The WCS sub library is not available.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 * The function returns a handle to an array with the CRPIX vector defined for
 * this WCS.
 */

const cpl_array *
cpl_wcs_get_crpix(const cpl_wcs *wcs)
{
    /* Check for NULL input */
    cpl_ensure(wcs != NULL, CPL_ERROR_NULL_INPUT, NULL);

#ifdef CPL_WCS_INSTALLED /* If WCS is installed */

    return wcs->wcsptr->naxis == 0 ? NULL : wcs->crpix;

#else
    (void)cpl_error_set_(CPL_ERROR_NO_WCS);
    return NULL;
#endif /* End If WCS is installed */
}

/**
 * @brief
 *    Accessor to get the CTYPE vector for a WCS
 *
 * @param wcs  The WCS structure to examine
 *
 * @return
 *   A handle to an array with the CTYPEja keyvalues for each pixel axis,
 *   or NULL on error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>wcs</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NO_WCS</td>
 *       <td class="ecr">
 *         The WCS sub library is not available.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 * The function returns a handle to an array with the CTYPE vector defined for
 * this WCS.
 */

const cpl_array *
cpl_wcs_get_ctype(const cpl_wcs *wcs)
{
    /* Check for NULL input */
    cpl_ensure(wcs != NULL, CPL_ERROR_NULL_INPUT, NULL);

#ifdef CPL_WCS_INSTALLED /* If WCS is installed */

    return wcs->wcsptr->naxis == 0 ? NULL : wcs->ctype;

#else
    (void)cpl_error_set_(CPL_ERROR_NO_WCS);
    return NULL;
#endif /* End If WCS is installed */
}

/**
 * @brief
 *    Accessor to get the CUNIT vector for a WCS
 *
 * @param wcs  The WCS structure to examine
 *
 * @return
 *   A handle to an array with the CUNITja keyvalues for each pixel axis,
 *   or NULL on error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>wcs</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NO_WCS</td>
 *       <td class="ecr">
 *         The WCS sub library is not available.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 * The function returns a handle to an array with the CUNIT vector defined for
 * this WCS.
 */

const cpl_array *
cpl_wcs_get_cunit(const cpl_wcs *wcs)
{
    /* Check for NULL input */
    cpl_ensure(wcs != NULL, CPL_ERROR_NULL_INPUT, NULL);

#ifdef CPL_WCS_INSTALLED /* If WCS is installed */

    return wcs->wcsptr->naxis == 0 ? NULL : wcs->cunit;

#else
    (void)cpl_error_set_(CPL_ERROR_NO_WCS);
    return NULL;
#endif /* End If WCS is installed */
}

/**
 * @brief
 *    Accessor to get the CD matrix for a WCS
 *
 * @param wcs  The WCS structure to examine
 *
 * @return
 *   A handle to a matrix with the CDi_ja linear transformation matrix,
 *   or NULL on error.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>wcs</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NO_WCS</td>
 *       <td class="ecr">
 *         The WCS sub library is not available.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 * The function returns a handle to a matrix with the CD matrix defined for
 * this WCS.
 */

const cpl_matrix *
cpl_wcs_get_cd(const cpl_wcs *wcs)
{
    /* Check for NULL input */
    cpl_ensure(wcs != NULL, CPL_ERROR_NULL_INPUT, NULL);

#ifdef CPL_WCS_INSTALLED /* If WCS is installed */

    return wcs->wcsptr->naxis == 0 ? NULL : wcs->cd;
#else
    (void)cpl_error_set_(CPL_ERROR_NO_WCS);
    return NULL;
#endif /* End If WCS is installed */
}


/**@}*/

#ifdef CPL_WCS_INSTALLED /* If WCS is installed */

/**
 * @internal
 * @brief
 *    Create a wcs structure by parsing a propertylist.
 * @param plist    The input propertylist
 *
 * @return
 *   The newly created and filled cpl_wcs object or NULL if it could not be
 *   created. In the latter case an appropriate error code is set.
 *
 * @see cpl_wcs_new_from_propertylist()
 */

static cpl_wcs *
cpl_wcs_new_from_propertylist_(const cpl_propertylist *plist)
{
    cpl_errorstate prevstate = cpl_errorstate_get();
    cpl_wcs *self;
    struct wcsprm *wwcs = NULL;
    const int np = cpl_propertylist_get_size(plist);
    /* Allocate space for np + 1 FITS cards, and 1 null-byte */
    char *shdr = (char *)cpl_malloc(80 * (np + 1) + 1);
    char *shdri = shdr;
    int nstr = 0; /* Counter of properties converted to the string */
    int retval, nrej, nwcs;
    int istab = -1;

    /* Convert the propertylist into a string */
    for (int ip = 0; ip < np; ip++) {
        const cpl_property *p = cpl_propertylist_get_const(plist, ip);

        if (!cpl_fits_fill_card(shdri, p)) {
            /* See if the propertylist has some form of WCS in it */
            if (istab < 0) {
                const size_t namelen = cpl_property_get_size_name(p);
                if (namelen > 5) { /* Key must have additional digit(s) */
                    const char *name = cpl_property_get_name_(p);
                    if (!memcmp("CRVAL", name, 5)) {
                        istab = 0;
                    }
                    else if (!memcmp("TCRVL", name, 5)) {
                        istab = 1;
                    }
                }
            }
            nstr++;
            shdri += 80;
        }
    }

    if (istab < 0) {
        cpl_free(shdr);
        /* The input propertylist contains no WCS */
        /* FIXME: This error code is not according to the doxygen */
        (void)cpl_error_set_message_(CPL_ERROR_UNSPECIFIED, "np=%d", np);
        return NULL;
    }

    /* It may not be needed, but add an END card and a null-byte */
    (void)memcpy(shdri, "END", 3);
    (void)memset(shdri + 3, ' ', 77);
    shdri[80] = '\0';

    /*
     * Parse the header string to get all WCS representations which are
     * present. The number of WCS representations is stored in nwcs.
     */

    retval = istab ? wcsbth(shdr, nstr + 1, 0, 0, 0, NULL, &nrej, &nwcs, &wwcs)
                   : wcspih(shdr, nstr + 1, 0, 0, &nrej, &nwcs, &wwcs);

    cpl_free(shdr);

    if (retval != 0) {
        wcsvfree(&nwcs, &wwcs);

        if (istab) {
            (void)cpl_error_set_wcs(CPL_ERROR_UNSPECIFIED, retval, "wcsbth",
                                    "np=%d", np);
        }
        else {
            (void)cpl_error_set_wcs(CPL_ERROR_UNSPECIFIED, retval, "wcspih",
                                    "np=%d", np);
        }
        return NULL;
    }

    /* Any errors from converting the propertylist can be ignored now */
    cpl_errorstate_set(prevstate);

    /*
     * Create and initialize the cpl_wcs object.
     *
     * Only the first WCS representation which was found in the header is used
     * and all others are ignored, i.e. the first element of the WCS vector
     * wwcs is copied to the cpl_wcs object, then all extra wcsprm
     * structures present in wwcs are discarded by deallocating the
     * temporary buffers.
     */

    self = _cpl_wcs_init(wwcs, istab);

    wcsvfree(&nwcs, &wwcs);

    if (!istab && self->naxis > 0) {
        char naxiskey[18] = "ZNAXIS"; /* Extra space for any int */
        /* This is an image, see if it's a compressed image */
        const int compressed =
            cpl_propertylist_has_cx(plist, CXSTR(naxiskey, 6));

        const char *nax = naxiskey + 1 - compressed; /* Key starts here */

        /* Now copy the (Z)NAXISi over to someplace where we can get it */
        int *dims = cpl_array_get_data_int(self->imdims);
        int i = self->naxis;

        cx_assert(dims != NULL);

        /* Get the image size information */
        do {
            /* Generate each (Z)NAXISi key and get its value */
            const int nconv = sprintf(naxiskey + 6, "%d", i);
            const int inax = cpl_propertylist_get_int(plist, nax);

            cx_assert(nconv > 0);

            if (!cpl_errorstate_is_equal(prevstate))
                break; /* Property not OK, e.g. missing or not int */

            dims[--i] = inax;

        } while (i > 0);

        /*
         * If any axis dimension is missing, clear the image dimensional
         * information and reset the number of axis to 0
         */

        if (i > 0) {
            cpl_errorstate_set(prevstate);
            (void)memset(dims, 0, self->naxis * sizeof(*dims));
            /* FIXME: Is it OK that this stops being true ? */
            cx_assert(self->naxis == cpl_array_get_size(self->imdims));
            self->naxis = 0; /* crval, crpix and cd are still non-NULL */
        }
    }

    return self;
}

/*-------------------------------------------------------------------------*/

/**
 * @internal
 * @brief
 *   Create a wcs object from an existing wcsprm structure.
 *
 * @param wcs     The wcsprm structure from which the object is created.
 * @param is_tab  Flag indicating whether wcs originates from a table header.
 *
 * @return
 *   The created wcs object, or @c NULL in case the input structure @em wcs
 *   is a @C NULL pointer.
 *
 * Allocate and initialize a wcs object from the wcsprm structure @em wcs.
 * A deep copy of @em wcs is used to initialize the wcs object, and thus
 * the ownership of @em wcs is not transferred.
 *
 * The internal copy of wcs is updated to make sure it is in a consistent
 * state. Then the ancilliary members of the wcs object are filled with
 * their respective values.
 */

inline static cpl_wcs *
_cpl_wcs_init(struct wcsprm *wcs, int is_tab)
{
    cpl_wcs *self = NULL;

    if (wcs != NULL) {
        struct wcsprm *_wcs = NULL;

        self = cpl_calloc(1, sizeof *self);
        self->wcsptr = cpl_calloc(1, sizeof(struct wcsprm));

        _wcs = self->wcsptr;
        _wcs->flag = -1;

        wcscopy(1, wcs, _wcs);
        wcsset(_wcs);

        self->istab = is_tab;
        self->naxis = _wcs->naxis;

        self->crpix = NULL;
        self->crval = NULL;
        self->cunit = NULL;
        self->ctype = NULL;
        self->cd = NULL;
        self->imdims = NULL;


        if (self->naxis > 0) {
            self->crpix = cpl_array_wrap_double(_wcs->crpix, self->naxis);
            self->crval = cpl_array_wrap_double(_wcs->crval, self->naxis);

            /*
             * Provide an array interface for _wcs->cunit and _wcs->ctype.
             *
             * Due to the unfavorable way wcslib stores this information
             * the respective members cannot be wrapped in an array, but
             * they have to be copied.
             *
             * NOTE:
             *   This also means that whenever the underlying wcs structure
             *   is updated using wcsset(), these members have to be updated
             *   to.
             */

            self->ctype = cpl_array_new(self->naxis, CPL_TYPE_STRING);
            self->cunit = cpl_array_new(self->naxis, CPL_TYPE_STRING);

            _cpl_wcs_set_ctype(self);
            _cpl_wcs_set_cunit(self);


            /*
             * Provide a representation of the CD matrix.
             *
             * A local representation of the CD matrix is necessary since
             * wcslib does not always provide this information in a way that
             * wrapping a member of the underlying structure would work.
             * For instance, the member _wcs->cd is only an input parameter,
             * but is never updated with the calculated CD matrix. The
             * linear transformation matrix member of _wcs->lin on the
             * other hand is not set for certain cases, and thus may
             * not always be accessible.
             */

            self->cd = cpl_matrix_new(self->naxis, self->naxis);
            _cpl_wcs_set_cd(self);


            /*
             * If the WCS originates from an image provide the image
             * dimensions, otherwise leave it as NULL.
             */

            if (self->istab == 0) {
                self->imdims = cpl_array_new(self->naxis, CPL_TYPE_INT);
                cpl_array_fill_window_int(self->imdims, 0, self->naxis, 0);
            }
        }
    }

    return self;
}


/**
 * @internal
 * @brief
 *   Do a 6 plate constant fit
 *
 * @param xy          The matrix of physical coordinates
 * @param std         The matrix of celestial coordinates
 * @param bad         An array to flag points to be ignored from the fit
 * @param plateconsts The output array of plate constants
 *
 * @return
 *   Nothing
 *
 * This routine fits the constants a,b,c,d,e,f to the equations:
 *     xi = ax + by + c
 *    eta = dx + ey + f
 * The values of these coefficients are passed back in the <i>plateconsts</i>
 * array.
 *
 */

static void
cpl_wcs_platesol_6(const cpl_matrix *xy,
                   const cpl_matrix *std,
                   const cpl_array *bad,
                   cpl_array **plateconsts)
{
    double sx1sq, sy1sq, sx1y1, sx1x2, sy1x2, *pc;
    double sy1y2, sx1y2, xposmean, yposmean, ximean, etamean, xx1, yy1, xx2,
        yy2;
    /* Get some convenience variables */
    const double *xydata = cpl_matrix_get_data_const(xy);
    const double *stddata = cpl_matrix_get_data_const(std);
    const int *isbad = cpl_array_get_data_int_const(bad);
    const cpl_size nstds = cpl_array_get_size(bad);
    cpl_size i, ngood;

    /* Initialise all the counters and summations */

    sx1sq = 0.0;
    sy1sq = 0.0;
    sx1y1 = 0.0;
    sx1x2 = 0.0;
    sy1x2 = 0.0;
    sy1y2 = 0.0;
    sx1y2 = 0.0;
    xposmean = 0.0;
    yposmean = 0.0;
    ximean = 0.0;
    etamean = 0.0;

    /* Find means in each coordinate system */

    ngood = 0;
    for (i = 0; i < nstds; i++) {
        if (isbad[i] != 0)
            continue;
        xposmean += xydata[2 * i];
        yposmean += xydata[2 * i + 1];
        ximean += stddata[2 * i];
        etamean += stddata[2 * i + 1];
        ngood++;
    }
    xposmean /= (double)ngood;
    yposmean /= (double)ngood;
    ximean /= (double)ngood;
    etamean /= (double)ngood;

    /* Now accumulate the sums */

    for (i = 0; i < nstds; i++) {
        if (!isbad[i]) {
            xx1 = xydata[2 * i] - xposmean;
            yy1 = xydata[2 * i + 1] - yposmean;
            xx2 = stddata[2 * i] - ximean;
            yy2 = stddata[2 * i + 1] - etamean;
            sx1sq += xx1 * xx1;
            sy1sq += yy1 * yy1;
            sx1y1 += xx1 * yy1;
            sx1x2 += xx1 * xx2;
            sy1x2 += yy1 * xx2;
            sy1y2 += yy1 * yy2;
            sx1y2 += xx1 * yy2;
        }
    }

    /* Get an output array for the results */

    *plateconsts = cpl_array_new(6, CPL_TYPE_DOUBLE);
    pc = cpl_array_get_data_double(*plateconsts);

    /* Do solution for X */

    pc[0] = (sx1y1 * sy1x2 - sx1x2 * sy1sq) / (sx1y1 * sx1y1 - sx1sq * sy1sq);
    pc[1] = (sx1x2 * sx1y1 - sx1sq * sy1x2) / (sx1y1 * sx1y1 - sx1sq * sy1sq);
    pc[2] = -xposmean * pc[0] - yposmean * pc[1] + ximean;

    /* Now the solution for Y */

    pc[3] = (sy1y2 * sx1y1 - sy1sq * sx1y2) / (sx1y1 * sx1y1 - sy1sq * sx1sq);
    pc[4] = (sx1y1 * sx1y2 - sy1y2 * sx1sq) / (sx1y1 * sx1y1 - sy1sq * sx1sq);
    pc[5] = -xposmean * pc[3] - yposmean * pc[4] + etamean;
}

/**
 * @internal
 * @brief
 *   Do a 4 plate constant fit
 *
 * @param xy          The matrix of physical coordinates
 * @param std         The matrix of celestial coordinates
 * @param bad         An array to flag points to be ignored from the fit
 * @param plateconsts The output array of plate constants
 *
 * @return
 *   Nothing
 *
 * This routine fits the constants a,b,c,d,e,f to the equations:
 *     xi = ax + by + c
 *    eta = dx + ey + f
 * but where the scale and rotation implied by the coefficients a,b,d,e are
 * constrained to be the same for each axis. The 6 coefficients are passed
 * back in the <i>plateconsts</i> array.
 *
 */

static void
cpl_wcs_platesol_4(const cpl_matrix *xy,
                   const cpl_matrix *std,
                   const cpl_array *bad,
                   cpl_array **plateconsts)
{
    double sx1sq, sy1sq, sx1x2, sy1x2, sy1y2, sx1y2, xposmean, yposmean, *pc;
    double ximean, etamean, xx1, yy1, xx2, yy2, det, num, denom, theta, mag;
    double stheta, ctheta;
    /* Get some convenience variables */
    const double *xydata = cpl_matrix_get_data_const(xy);
    const double *stddata = cpl_matrix_get_data_const(std);
    const int *isbad = cpl_array_get_data_int_const(bad);
    const cpl_size nstds = cpl_array_get_size(bad);
    cpl_size i, ngood;


    /* Initialise all the counters and summations */

    sx1sq = 0.0;
    sy1sq = 0.0;
    sx1x2 = 0.0;
    sy1x2 = 0.0;
    sy1y2 = 0.0;
    sx1y2 = 0.0;
    xposmean = 0.0;
    yposmean = 0.0;
    ximean = 0.0;
    etamean = 0.0;

    /* Find means in each coordinate system */

    ngood = 0;
    for (i = 0; i < nstds; i++) {
        if (isbad[i] != 0)
            continue;
        xposmean += xydata[2 * i];
        yposmean += xydata[2 * i + 1];
        ximean += stddata[2 * i];
        etamean += stddata[2 * i + 1];
        ngood++;
    }
    xposmean /= (double)ngood;
    yposmean /= (double)ngood;
    ximean /= (double)ngood;
    etamean /= (double)ngood;

    /* Now accumulate the sums */

    for (i = 0; i < nstds; i++) {
        if (!isbad[i]) {
            xx1 = xydata[2 * i] - xposmean;
            yy1 = xydata[2 * i + 1] - yposmean;
            xx2 = stddata[2 * i] - ximean;
            yy2 = stddata[2 * i + 1] - etamean;
            sx1sq += xx1 * xx1;
            sy1sq += yy1 * yy1;
            sx1x2 += xx1 * xx2;
            sy1x2 += yy1 * xx2;
            sy1y2 += yy1 * yy2;
            sx1y2 += xx1 * yy2;
        }
    }

    /* Compute the rotation angle */

    det = sx1x2 * sy1y2 - sy1x2 * sx1y2;
    if (det < 0.0) {
        num = sy1x2 + sx1y2;
        denom = -sx1x2 + sy1y2;
    }
    else {
        num = sy1x2 - sx1y2;
        denom = sx1x2 + sy1y2;
    }
    if (num == 0.0 && denom == 0.0) {
        theta = 0.0;
    }
    else {
        theta = atan2(num, denom);
        if (theta < 0.0)
            theta += CPL_MATH_2PI;
    }

    /* Compute magnification factor */

    ctheta = cos(theta);
    stheta = sin(theta);
    num = denom * ctheta + num * stheta;
    denom = sx1sq + sy1sq;
    if (denom <= 0.0) {
        mag = 1.0;
    }
    else {
        mag = num / denom;
    }

    /* Get an output array for the results */

    *plateconsts = cpl_array_new(6, CPL_TYPE_DOUBLE);
    pc = cpl_array_get_data_double(*plateconsts);

    /* Compute coeffs */

    if (det < 0.0) {
        pc[0] = -mag * ctheta;
        pc[3] = mag * stheta;
    }
    else {
        pc[0] = mag * ctheta;
        pc[3] = -mag * stheta;
    }
    pc[1] = mag * stheta;
    pc[4] = mag * ctheta;
    pc[2] = -xposmean * pc[0] - yposmean * pc[1] + ximean;
    pc[5] = -xposmean * pc[3] - yposmean * pc[4] + etamean;
}


inline static int
_cpl_wcs_set_cd(cpl_wcs *self)
{
    register int k;


    /*
     * Calculate the elements of the CD matrix as the product of the
     * CDELT diagonal matrix and the PC matrix, i.e.
     *
     * CD = CDELT * PC
     */

    for (k = 0; k < self->naxis; ++k) {
        register int l;
        register double _cdelt = self->wcsptr->lin.cdelt[k];

        double *_pc = &self->wcsptr->lin.pc[k * self->naxis];


        for (l = 0; l < self->naxis; ++l) {
            cpl_matrix_set_(self->cd, k, l, _cdelt * _pc[l]);
        }
    }

    return 0;
}


inline static int
_cpl_wcs_set_ctype(cpl_wcs *self)
{
    register int k;

    for (k = 0; k < self->naxis; ++k) {
        cpl_array_set_string(self->ctype, k, self->wcsptr->ctype[k]);
    }

    return 0;
}


inline static int
_cpl_wcs_set_cunit(cpl_wcs *self)
{
    register int k;

    for (k = 0; k < self->naxis; ++k) {
        cpl_array_set_string(self->cunit, k, self->wcsptr->cunit[k]);
    }

    return 0;
}

/*
 * @internal
 * @brief
 *   Update a wcs object.
 *
 * @param wcs  The wcs object to update.
 *
 * @return
 *   The function returns the status of the call to wcsset(), or -1 in .
 *
 * The function first updates the wcsprm member of the the wcs object @em wcs
 * and then propagates the updated values to other members of @em where
 * necessary.
 *
 * @note
 *   If the naxis member of @em wcs differs from the naxis member of the
 *   underlying wcsprm structure, all affected data members of @em wcs get
 *   resized accordingly. Actually this should happen only once, on the first
 *   call to the function when @em is no completely set up.
 */

inline static int
_cpl_wcsset(cpl_wcs *wcs)
{
    struct wcsprm *_wcs = wcs->wcsptr;
    int status = wcsset(_wcs);

    /*
     * Update data members of the cpl_wcs object which may have
     * changed by the call to wcsset()
     */

    _cpl_wcs_set_cd(wcs);

    _cpl_wcs_set_ctype(wcs);
    _cpl_wcs_set_cunit(wcs);

    return status;
}

#endif /* End If WCS is installed */
