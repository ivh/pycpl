/*
 * This file is part of the HDRL
 * Copyright (C) 2016 European Southern Observatory
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

/*-----------------------------------------------------------------------------
                                   Includes
-----------------------------------------------------------------------------*/

#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include "hdrl_persistence.h"

/*-----------------------------------------------------------------------------
                                   Static
 -----------------------------------------------------------------------------*/



/**
 *
 * @defgroup hdrl_persistence     Mitigate IR Persistence
 *
 * @brief This algorithm generates a map of the persistence present in an infra-
 *        red detector due to charge trapped in the depletion regions of each
 *        pixel left over from previous exposures.  Excess flux from this source
 *        may then be corrected for by subtracting off the persistence map.
 *
 *        Characterisation work performed by the ESO Detector Systems Group has
 *        resulted in a predictive model using a 5th order IIR filter, where
 *        each pole/order of the filter represents a different detrapping time
 *        constant bin.  The sum of the 5 exponential functions produces the de-
 *        trapped charge profile, or the flux due to persistence.  The algorithm
 *        in this file implements this model.
 *
 *        For more details on the algorithm/model, or the characterisation of
 *        the time constants, please refer to:
 *            SPIE Journal Paper | September 3, 2019
 *            Predictive model of persistence in H2RG detectors
 *            Simon Tulloch; Elizabeth George; ESO Detector Systems Group
 *            JATIS Vol. 5 Issue 03
 *
 * \par Generate a persistence map from previous illumination frames, a trap
 *      density map and RHO values for each detrap time constant (TAU), and
 *      source maps derived from the illumination frames.  The trap density map
 *      (TDM) and RHO values are specific to the IR detector in question.
 *
 */
/*----------------------------------------------------------------------------*/

/**@{*/

/*-----------------------------------------------------------------------------
                        Persistence parameters Definition
 -----------------------------------------------------------------------------*/

/** @cond PRIVATE */

/*-----------------------------------------------------------------------------
                             Function Prototypes
 -----------------------------------------------------------------------------*/

static void hdrl_persistence_verify_inputs(
		const double            gain,
		const double            det_turnover,
		const double            mean_trim,
		const cpl_boolean       cleanQ,
        const cpl_array        * dates_,
        const cpl_array        * exps_,
        const hdrl_imagelist   * illums_,
        const cpl_imagelist    * srcmasks_,
        const cpl_image        * maximum,
        const cpl_image        * density,
        const cpl_image        * fullwell,
        const cpl_table        * frac);

static void hdrl_persistence_threshhold_img(
        hdrl_image             * input,
        const hdrl_image       * lower,
        const hdrl_image       * upper);

static void hdrl_persistence_zero_flagged_pixels(
        hdrl_image             * hdrl_img,
        const cpl_mask         * mask,
        const cpl_binary         value);

static void hdrl_persistence_compute_qi(
        hdrl_imagelist         * Q,
        hdrl_imagelist         * Qacc,
        const double             delt,
        const hdrl_imagelist   * hirhos,
        const float            * taud,
        const double             exptime,
        const hdrl_image       * hiillum,
        const int                n_tau,
        const hdrl_imagelist   * himaxs);

static cpl_propertylist * hdrl_persistence_calc_stats(
        const hdrl_image       * Qtot,
        const double             trim_perc);

static int hdrl_persistence_get_time_const(
        const cpl_array        * nus);

static hdrl_imagelist * hdrl_persistence_calculate_rho(
        const cpl_array        * nus,
        const hdrl_image       * hiden,
        const int                n_tau);


static const hdrl_imagelist * hdrl_persistence_convert_max_traps(
        const cpl_array        * nus,
        const hdrl_image       * himax,
        const int                n_tau);

static void hdrl_persistence_sum_Qtots(
        hdrl_image             * Qtot1_n,
        hdrl_imagelist         * Qtot,
        const hdrl_imagelist   * Q,
        const int                n_tau);

static hdrl_imagelist * hdrl_persistence_create_hilist(
        const int                n,
        const cpl_size           nx,
        const cpl_size           ny);

/*----------------------------------------------------------------------------*/
/**
 * @brief
 */
/*----------------------------------------------------------------------------*/
static void hdrl_persistence_qc_dump(
        cpl_propertylist *  plist,
        const char        * qcname)
{
    cpl_error_ensure(plist && qcname, CPL_ERROR_NULL_INPUT, return, " ");

    if (cpl_propertylist_has(plist, qcname)){

    	cpl_msg_debug(cpl_func,"Statistics: %s = %g",qcname,
    			cpl_propertylist_get_double(plist,qcname));
    }

}

/*----------------------------------------------------------------------------*/
/**
 * @brief    Verify basic correctness of the non-parameter inputs (frames, etc)
 *
 * @param param_    params to control the algorithm
 * @param dates_    array of MJD-OBS vals, the 1st being from the TBC (to-be-
 *                  corrected) frame, followed by the values from the illumina-
 *                  tion frames
 * @param illums_   list of illumination frames
 * @param bpm_      the master bad pixel mask
 * @param srcmasks_ source masks indicating source regions in the illumination
 *                  frames
 *
 * @return   CPL_ERROR_NONE if everything is ok, an error code otherwise
 */
/*----------------------------------------------------------------------------*/
static void hdrl_persistence_verify_inputs(
		const double            gain,
		const double            det_turnover,
		const double            mean_trim,
		const cpl_boolean       cleanQ,
        const cpl_array        * dates_,
        const cpl_array        * exps_,
        const hdrl_imagelist   * illums_,
        const cpl_imagelist    * srcmasks_,
        const cpl_image        * maximum,
        const cpl_image        * density,
        const cpl_image        * fullwell,
        const cpl_table        * frac)
{

    cpl_error_ensure(gain > 0.0,
            CPL_ERROR_ILLEGAL_INPUT, return, "gain has to be larger than zero");

/* Dummy check in case we need to add a check in the future without changing api */
    cpl_error_ensure(det_turnover < DBL_MAX,
            CPL_ERROR_ILLEGAL_INPUT, return, "det_turnover exceeds range");

    cpl_error_ensure(mean_trim >= 0.0 && mean_trim < 100.0,
            CPL_ERROR_ILLEGAL_INPUT, return, "mean_trim < 0.0 or >= 100.0");

    cpl_error_ensure(cleanQ == CPL_TRUE || cleanQ == CPL_FALSE,
            CPL_ERROR_ILLEGAL_INPUT, return, "cleanQ isn't TRUE or FALSE");

    // dates_ checks
    cpl_error_ensure(dates_, CPL_ERROR_NULL_INPUT, return, "NULL 'dates' "
            "input");
    const cpl_size ndates = cpl_array_get_size(dates_);
    cpl_error_ensure(ndates > 0, CPL_ERROR_DATA_NOT_FOUND, return, "Empty "
            "'dates' input");
    cpl_array_delete(cpl_array_cast(dates_, CPL_TYPE_DOUBLE));
    cpl_error_ensure(!cpl_error_get_code(), cpl_error_get_code(), return,
            "'dates' input type must be numeric");

    // exps_ checks
    cpl_error_ensure(exps_, CPL_ERROR_NULL_INPUT, return, "NULL 'exps' input");
    const cpl_size nexps = cpl_array_get_size(exps_);
    cpl_error_ensure(nexps > 0, CPL_ERROR_DATA_NOT_FOUND, return, "Empty "
            "'exps' input");
    cpl_array_delete(cpl_array_cast(exps_, CPL_TYPE_DOUBLE));
    cpl_error_ensure(!cpl_error_get_code(), cpl_error_get_code(), return,
            "'exps' input type must be numeric");
    cpl_error_ensure(nexps == ndates - 1, CPL_ERROR_INCOMPATIBLE_INPUT, return,
            "'dates' should be 1 larger than 'exps'");

    // illums_ checks
    cpl_error_ensure(illums_, CPL_ERROR_NULL_INPUT, return, "NULL 'illums' "
            "input");
    const cpl_size nillums = hdrl_imagelist_get_size(illums_);
    cpl_error_ensure(nillums > 0, CPL_ERROR_DATA_NOT_FOUND, return, "Empty "
            "'illums' input");
    cpl_error_ensure(nillums == ndates - 1, CPL_ERROR_INCOMPATIBLE_INPUT,
            return, "'dates' should be 1 larger than 'illums'");
    const cpl_size illum_szx = hdrl_imagelist_get_size_x(illums_);
    const cpl_size illum_szy = hdrl_imagelist_get_size_y(illums_);

    // srcmasks_ checks
    if (srcmasks_) {
        const cpl_size nsrcmasks = cpl_imagelist_get_size(srcmasks_);
        cpl_error_ensure(nsrcmasks == nillums, CPL_ERROR_INCOMPATIBLE_INPUT,
                return, "The sizes of the 'srcmasks' & "
                "'illum' inputs differ");

        for (cpl_size i=0; i<nsrcmasks; ++i) {
            const cpl_image * srcmask = cpl_imagelist_get_const(srcmasks_, i);
            cpl_error_ensure(srcmask, CPL_ERROR_NULL_INPUT, return, "One of "
                    "the 'srcmasks' is NULL");
            const cpl_size src_szx = cpl_image_get_size_x(srcmask);
            const cpl_size src_szy = cpl_image_get_size_y(srcmask);
            cpl_error_ensure(src_szx == illum_szx && src_szy == illum_szy,
                    CPL_ERROR_INCOMPATIBLE_INPUT, return, "The dimensions of "
            "the 'srcmasks' & 'illum' inputs differ");
            const cpl_type src_typ = cpl_image_get_type(srcmask);
            cpl_error_ensure(src_typ == CPL_TYPE_INT, CPL_ERROR_INVALID_TYPE,
                    return, "One of the 'srcmasks' is not of type int");
        }
    }

    // traps_ checks
    // maximum, density, fullwell, and frac checks: they should be non-NULL but pt to NULL

    cpl_error_ensure(maximum && density && fullwell && frac, CPL_ERROR_NULL_INPUT, return, " ");

     cpl_error_ensure(cpl_image_get_size_x(maximum) == illum_szx &&
            cpl_image_get_size_y(maximum) == illum_szy,
            CPL_ERROR_INCOMPATIBLE_INPUT, return, "Dimensions of maximum "
            "trap map & 'illums' input differ");


    cpl_error_ensure(cpl_image_get_size_x(density) == illum_szx &&
            cpl_image_get_size_y(density) == illum_szy ,
            CPL_ERROR_INCOMPATIBLE_INPUT, return, "Dimensions of trap "
            "density map & 'illums' input differ");


     cpl_error_ensure(cpl_image_get_size_x(fullwell) == illum_szx &&
            cpl_image_get_size_y(fullwell) == illum_szy ,
            CPL_ERROR_INCOMPATIBLE_INPUT, return, "Dimensions of trap "
            "fullwell map & 'illums' input differ");


    cpl_error_ensure(cpl_table_has_column(frac, "TAU") &&
            cpl_table_has_column(frac, "NU"), CPL_ERROR_INCOMPATIBLE_INPUT,
            return, "Trap fraction table missing required columns");
    cpl_error_ensure(6==cpl_table_get_nrow(frac), CPL_ERROR_INCOMPATIBLE_INPUT,
            return, "Trap fraction table contains wrong number of rows");
    const cpl_type tau_t = cpl_table_get_column_type(frac, "TAU");
    const cpl_type nu_t = cpl_table_get_column_type(frac, "NU");
    cpl_error_ensure(tau_t == CPL_TYPE_FLOAT && nu_t == CPL_TYPE_FLOAT,
            CPL_ERROR_INVALID_TYPE, return, "Trap fraction table columns "
            "are of the wrong type");

}

/** @endcond */

/*----------------------------------------------------------------------------*/
/**
 * @brief generate the persistence map
 *
 * @param  gain              The gain for ADU to electron conversion [e-/ADU]
 * @param  turnover          The detector turnover value [ADU]
 * @param  mean_trim         The percentage of the pixels to discard in the
 *                           minmax based QC parameter
 * @param  cleanQ            Whether or not to filter individual Qi planes
 * @param  dateobs           The times when the data is taken, e.g. MJD-OBS
 *                           (including the MJD-OBS of the target frame)
 * @param  exptimes          The exposure sequence times of the data, e.g.
 *                           EXPTIME (excluding the EXPTIME of the target frame)[s]
 * @param  ilist_persistence Imagelist containing the images causing the persistence [ADU]
 * @param  ilist_obj         Masks with the objects of the single images (or NULL)
 * @param  maximum           A two-dimensional map describing the maximum number
 *                           of persistence traps in each pixel   [number]
 * @param  density           A two-dimensional map describing the fraction of
 *                           incident photons that get converted to traps [unitless]
 * @param  fullwell          A two-dimensional map describing the full-well
 *                           capacity of each pixel [ADU]
 * @param  frac              A table containing a six-term vector describing the
 *                           relative fraction of each time constant populated
 *                           by each trap [unitless]
 * @param  persistence       Returned: the calculated persistence [e-]
 * @param  persistence_qc    Returned: QC parameters derived from the calculated persistence
 * @return The cpl error code in case of error or CPL_ERROR_NONE
 */
/*----------------------------------------------------------------------------*/

cpl_error_code hdrl_persistence_compute(
		const double            gain,
		const double            turnover,
		const double            mean_trim,
		const cpl_boolean       cleanQ,
		const cpl_array        * dateobs,
		const cpl_array        * exptimes,
		      hdrl_imagelist   * ilist_persistence,
		const cpl_imagelist    * ilist_obj,
		const cpl_image        * maximum,
		const cpl_image        * density,
		const cpl_image        * fullwell,
		const cpl_table        * frac,
		hdrl_image             ** persistence,
		cpl_propertylist       ** persistence_qc)
{

    hdrl_persistence_verify_inputs(gain, turnover, mean_trim, cleanQ, dateobs, exptimes, ilist_persistence,
            ilist_obj, maximum, density, fullwell, frac);
    cpl_error_ensure(!cpl_error_get_code(), cpl_error_get_code(), return cpl_error_get_code(),
            " ");

    // load the MTM (Maximum Trap Map)
    hdrl_image * himax = hdrl_image_create(maximum, NULL);  // no ERR plane!
    // load the TDM (Trap Density Map)
    hdrl_image * hiden = hdrl_image_create(density, NULL);  // no ERR plane!

    /* Now transform images from ADUs into electrons. */
    cpl_msg_debug(cpl_func, "Converting frames from ADUs to electrons."
    		" For this a gain of %g is used", gain);

    hdrl_imagelist_mul_scalar(ilist_persistence, (hdrl_value){gain, 0.});
    hdrl_image_mul_scalar(himax, (hdrl_value){gain, 0.});

    const cpl_size nx_0 = hdrl_imagelist_get_size_x(ilist_persistence);
    const cpl_size ny_0 = hdrl_imagelist_get_size_y(ilist_persistence);

    
    // get pointers to the nu & tau values
    const cpl_size nrows = cpl_table_get_nrow(frac);
    const float * taud = cpl_table_get_data_float_const(frac, "TAU");
    const float * nud = cpl_table_get_data_float_const(frac, "NU");
    cpl_array * nus = cpl_array_wrap_float((float *)nud, nrows);

    // print a few params to screen/logfile
    cpl_msg_debug(cpl_func, "cleanQ: %s", cleanQ ? "true" : "false");

    // get a few size values
    const cpl_size nimgs = hdrl_imagelist_get_size(ilist_persistence);
    const int n_tau = hdrl_persistence_get_time_const(nus);

    // create list of Qi, Qtoti, and Qacci frames, and the Qtot1_n frame that
    // will be the final persistence map product
    hdrl_imagelist * Q = hdrl_persistence_create_hilist(n_tau, nx_0, ny_0);
    hdrl_imagelist * Qacc = hdrl_persistence_create_hilist(n_tau, nx_0, ny_0);
    hdrl_imagelist * Qtot = hdrl_persistence_create_hilist(n_tau, nx_0, ny_0);
    hdrl_image * Qtot1_n = hdrl_image_new(nx_0, ny_0);

    const hdrl_image * hizero = hdrl_image_new(nx_0, ny_0);
    const hdrl_imagelist * himaxs =
        hdrl_persistence_convert_max_traps(nus, himax, n_tau);
    hdrl_image_delete(himax);
    himax = NULL;
    const hdrl_imagelist * hirhos =
        hdrl_persistence_calculate_rho(nus, hiden, n_tau);
    cpl_array_unwrap(nus);
    nus = NULL;

    const double mjdobs = cpl_array_get(dateobs, 0, NULL);
    for (int index=0; index<nimgs; index++) {
        const double exptime = cpl_array_get(exptimes, index, NULL);
        const double diff_ = mjdobs - cpl_array_get(dateobs, index+1, NULL);
        const double diff = diff_ * 24.0 * 3600.0;
        cpl_msg_debug(cpl_func, "illumination frame #%d taken %f secs before "
                     "the correction frame", index+1, diff);

        hdrl_image * hiillum = hdrl_imagelist_get(ilist_persistence, index);


        // new_4.2:
        // Saturated pixels having the turn-over value defined in the persistence characterisation MEF (HIERARCH ESO SAT TURNOVER),
        // are set to their full well value given in the full well map.

        /*Working on data itself - no copy! */

        size_t npix = hdrl_image_get_size_x(hiillum);
        npix *= hdrl_image_get_size_y(hiillum);

        cpl_image * img = hdrl_image_get_image(hiillum);
        double * imgd = cpl_image_get_data_double(img);

        const double * fullwell_vals = NULL;
        if (cpl_image_get_type(fullwell) != CPL_TYPE_DOUBLE) {
        	cpl_image * fullwellnew = cpl_image_cast(fullwell, CPL_TYPE_DOUBLE);
        	fullwell_vals = cpl_image_get_data_double_const(fullwellnew);
        	for (size_t i=0; i<npix; i++) {
        		imgd[i] = (imgd[i] <= turnover ? fullwell_vals[i] : imgd[i]);
        	}
        	cpl_image_delete(fullwellnew);
        } else {
        	fullwell_vals = cpl_image_get_data_double_const(fullwell);
        	for (size_t i=0; i<npix; i++) {
        		imgd[i] = (imgd[i] <= turnover ? fullwell_vals[i] : imgd[i]);
        	}
        }


        hdrl_persistence_compute_qi(
            Q, Qacc, diff, hirhos, taud, exptime, hiillum, n_tau, himaxs);

        if (cleanQ) {
            // unlike Mark's Python prototype, we don't zero bad pixels here
            // but rather further below (outside the loop) as the last step
            for (int j=0; j<n_tau; j++) {
                hdrl_image * qj = hdrl_imagelist_get(Q, j);
                // threshholding below uses the image itself as the upper limit
                hdrl_persistence_threshhold_img(qj, hizero, qj);
            }
        }

        if (ilist_obj) {
            for (int j=0; j<n_tau; j++) {
                // note: offset i (*not* j) used below
                const cpl_image * im = cpl_imagelist_get_const(ilist_obj, index);
                cpl_mask * sm = cpl_mask_threshold_image_create(im, -0.5, 0.5);
                hdrl_image * qj = hdrl_imagelist_get(Q, j);
                hdrl_persistence_zero_flagged_pixels(qj, sm, CPL_BINARY_1);
                cpl_mask_delete(sm);
            }
        }

        hdrl_persistence_sum_Qtots(Qtot1_n, Qtot, Q, n_tau);
    }

    hdrl_imagelist_delete((hdrl_imagelist *)himaxs);
    himaxs = NULL;
    hdrl_imagelist_delete((hdrl_imagelist *)hirhos);
    hirhos = NULL;
    hdrl_imagelist_delete(Qacc);
    Qacc = NULL;
    hdrl_imagelist_delete(Q);
    Q = NULL;

    // Mark's latest script does not threshhold the final Qtot1_n frame even
    // though the cleanQtot flag is still present
    //if (p->cleanQtot) {
    //    hdrl_image * hi = hdrl_imagelist_get(Qtot1_n, n_tau - 1);
    //    hdrl_persistence_threshhold_img(hi, hizero, hiden);
    //}

    hdrl_image_delete((hdrl_image *)hizero);
    hizero = NULL;
    hdrl_image_delete(hiden);
    hiden = NULL;

    *persistence = Qtot1_n;
    *persistence_qc = hdrl_persistence_calc_stats(Qtot1_n, mean_trim);

     hdrl_persistence_qc_dump(*persistence_qc, "ESO QC PERSIST MEAN");
     hdrl_persistence_qc_dump(*persistence_qc, "ESO QC PERSIST MEANERR");
     hdrl_persistence_qc_dump(*persistence_qc, "ESO QC PERSIST MINMAX MEAN");
     hdrl_persistence_qc_dump(*persistence_qc, "ESO QC PERSIST MINMAX MEANERR");
     hdrl_persistence_qc_dump(*persistence_qc, "ESO QC PERSIST SIGCLIP MEAN");
     hdrl_persistence_qc_dump(*persistence_qc, "ESO QC PERSIST SIGCLIP MEANERR");
     hdrl_persistence_qc_dump(*persistence_qc, "ESO QC PERSIST WEIGHTED MEAN");
     hdrl_persistence_qc_dump(*persistence_qc, "ESO QC PERSIST WEIGHTED MEANERR");
     hdrl_persistence_qc_dump(*persistence_qc, "ESO QC PERSIST MEDIAN");
     hdrl_persistence_qc_dump(*persistence_qc, "ESO QC PERSIST MEDIANERR");
     hdrl_persistence_qc_dump(*persistence_qc, "ESO QC PERSIST STD");
     hdrl_persistence_qc_dump(*persistence_qc, "ESO QC PERSIST MIN");
     hdrl_persistence_qc_dump(*persistence_qc, "ESO QC PERSIST MAX");

    // Calculate statistics
    if (cpl_msg_get_level() == CPL_MSG_DEBUG) {
    	cpl_propertylist_save(NULL, "hdrl_debug_persistence_qtot.fits", CPL_IO_CREATE);
    	cpl_propertylist_save(NULL, "hdrl_debug_persistence_qtot_error.fits", CPL_IO_CREATE);

        for (int i=0; i<n_tau; i++) {
            hdrl_image * qtoti     = hdrl_imagelist_get(Qtot, i);
            cpl_image  * qtoti_ima = hdrl_image_get_image(qtoti);
            cpl_image  * qtoti_err = hdrl_image_get_error(qtoti);

            cpl_image_save(qtoti_ima,"hdrl_debug_persistence_qtot.fits",       CPL_TYPE_DOUBLE, hdrl_persistence_calc_stats(qtoti, mean_trim), CPL_IO_EXTEND );
            cpl_image_save(qtoti_err,"hdrl_debug_persistence_qtot_error.fits", CPL_TYPE_DOUBLE, hdrl_persistence_calc_stats(qtoti, mean_trim), CPL_IO_EXTEND );
        }
    }
    hdrl_imagelist_delete(Qtot);
    Qtot = NULL;
    return cpl_error_get_code();
}

/*----------------------------------------------------------------------------*/
/**
  * @brief set pixels above the trap density map or below @a lower to zero
  *
  * @param hdrl_img  the frame to be modified
  * @param lower     the (scalar) lower threshhold
  * @param upper     the trap density map image that forms the upper treshhold
  *
  * @return CPL_ERROR_NONE if everything is ok, an error code otherwise
  */
/*----------------------------------------------------------------------------*/
static void hdrl_persistence_threshhold_img(
        hdrl_image             * input,
        const hdrl_image       * lower,
        const hdrl_image       * upper)
{
    cpl_error_ensure(
        input && lower && upper, CPL_ERROR_NULL_INPUT, return," ");

    size_t npix = hdrl_image_get_size_x(input);
    npix *= hdrl_image_get_size_y(input);

    cpl_image * img = hdrl_image_get_image(input);
    double * imgd = cpl_image_get_data_double(img);

    const cpl_image * up = hdrl_image_get_image_const(upper);
    const double * upper_vals = cpl_image_get_data_double_const(up);
    const cpl_image * low = hdrl_image_get_image_const(lower);
    const double * lower_vals = cpl_image_get_data_double_const(low);
    HDRL_OMP(omp parallel for)
    for (size_t i=0; i<npix; i++) {
        imgd[i] = (imgd[i] <= upper_vals[i] ? imgd[i] : upper_vals[i]);
        imgd[i] = (imgd[i] >= lower_vals[i] ? imgd[i] : lower_vals[i]);
    }
}

/*---------------------------------------------------------------------------*/
/*
 * @param
 */
/*---------------------------------------------------------------------------*/
static void hdrl_persistence_sum_Qtots(
        hdrl_image             * Qtot1_n,
        hdrl_imagelist         * Qtot,
        const hdrl_imagelist   * Q,
        const int                n_tau)
{
    cpl_error_ensure(Qtot1_n && Qtot && Q, CPL_ERROR_NULL_INPUT, return, " ");
    cpl_error_ensure(n_tau > -1, CPL_ERROR_ILLEGAL_INPUT, return, " ");

    for (int i=0; i<n_tau; i++) {
        const hdrl_image * qi = hdrl_imagelist_get_const(Q, i);
        hdrl_image_add_image(Qtot1_n, qi);
    }

    for (int i=0; i<n_tau; i++) {
        hdrl_image * qtoti = hdrl_imagelist_get(Qtot, i);
        const hdrl_image * qi = hdrl_imagelist_get_const(Q, i);
        hdrl_image_add_image(qtoti, qi);
    }
}


/*----------------------------------------------------------------------------*/
/**
  * @brief set pixels identified by the mask and flag value to zero
  *
  * @param hdrl_img  the frame to be modified
  * @param mask      the mask to be consulted
  * @param value     the mask value that indicates which pixels to target
  *
  * @return CPL_ERROR_NONE if everything is ok, an error code otherwise
  */
/*----------------------------------------------------------------------------*/
static void hdrl_persistence_zero_flagged_pixels(
        hdrl_image             * hdrl_img,
        const cpl_mask         * mask,
        const cpl_binary         value)
{
    cpl_error_ensure(hdrl_img && mask, CPL_ERROR_NULL_INPUT, return, " ");
    cpl_error_ensure(value == CPL_BINARY_0 || value == CPL_BINARY_1,
                    CPL_ERROR_ILLEGAL_INPUT, return, " ");

    size_t npix = hdrl_image_get_size_x(hdrl_img);
    npix *= hdrl_image_get_size_y(hdrl_img);

    cpl_image * img = hdrl_image_get_image(hdrl_img);
    double * imgd = cpl_image_get_data_double(img);

    const cpl_binary * mskd = cpl_mask_get_data_const(mask);
    HDRL_OMP(omp parallel for)
    for (size_t i=0; i<npix; i++) {
        imgd[i] = (mskd[i] == value ? 0.00 : imgd[i]);
    }
}

/*----------------------------------------------------------------------------*/
/**
  * @brief calculate Qi prior to any additional filtering
  *
  * @param Q        the list to hold Qi frames
  * @param delt     the diff in secs between the illum & TBC (to-be-corrected)
  *                 frames' acquisition times
  * @param hirhos   the RHO values for all time TAUs
  * @param taud     the time TAU for this Qi frame
  * @param hiillum  the illum frame
  *
  * @return CPL_ERROR_NONE if everything is ok, an error code otherwise
  */
/*----------------------------------------------------------------------------*/
static void hdrl_persistence_compute_qi(
		hdrl_imagelist         * Q,
		hdrl_imagelist         * Qacc,
		const double             delt,
		const hdrl_imagelist   * hirhos,
		const float            * taud,
		const double             exptime,
		const hdrl_image       * hiillum,
		const int                n_tau,
		const hdrl_imagelist   * himaxs)
{
	cpl_error_ensure(Q && Qacc && hirhos && taud && hiillum && himaxs,
			CPL_ERROR_NULL_INPUT, return, " ");
	cpl_error_ensure(n_tau > -1, CPL_ERROR_ILLEGAL_INPUT, return, " ");
	cpl_error_ensure(exptime > -1, CPL_ERROR_ILLEGAL_INPUT, return, " ");

	for (int i=0; i<n_tau; i++) {
		/*ToDo AGA: Please verify statement as I inverted the logic and now
		 * I get the same result as the script from Mark! */

		// compute new Qi using the old Qi & old Qacci ("Then, we allow ..." in
		// Mark's script); use of the old Qacci is why the order of this step
		// and the next is the reverse of what's in Mark's script: if we didn't
		// reverse the order, then we'd overwrite the old Qacci before it could
		// be used to generate the new Qi (Mark's script doesn't have this issue
		// because he uses an array to store the value of Qacci for every file,
		// whereas we only store 1 instance of Qacci in order to use less memory)

		// compute new Qacci using illum & rho ("We first consider ..." in
		// Mark's script)
		{
			hdrl_image * QacciC = hdrl_image_duplicate(hiillum);  // "current"
			hdrl_image_mul_image(QacciC, hdrl_imagelist_get(hirhos, i));
			const hdrl_value val = { 1.0 - exp(-exptime/taud[i]), 0.0 };
			hdrl_image_mul_scalar(QacciC, val);
			const hdrl_image * himax = hdrl_imagelist_get_const(himaxs, i);
			// threshholding below uses the image itself as the lower limit
			hdrl_persistence_threshhold_img(QacciC, QacciC, himax);
			hdrl_imagelist_set(Qacc, QacciC, i);  // deallocates "previous" and
			// replaces with "current"
		}

		{
			const hdrl_image * QacciP = hdrl_imagelist_get(Qacc, i);
			hdrl_image * tmp = hdrl_image_duplicate(QacciP);
			hdrl_value val = (hdrl_value){ exp(-delt/taud[i]), 0.0 };
			hdrl_image_mul_scalar(tmp, val);
			hdrl_image * Qi = hdrl_imagelist_get(Q, i);
			hdrl_image_sub_image(tmp, Qi);
			val = (hdrl_value){ delt/taud[i], 0.0 };
			hdrl_image_mul_scalar(tmp, val);
			hdrl_image_add_image(Qi, tmp);
			hdrl_image_delete(tmp);
		}
	}
}

/*---------------------------------------------------------------------------*/
/*
 * @param
 */
/*---------------------------------------------------------------------------*/
static hdrl_imagelist * hdrl_persistence_create_hilist(
        const int                n,
        const cpl_size           nx,
        const cpl_size           ny)
{
    hdrl_imagelist * list = hdrl_imagelist_new();
    for (int i=0; i<n; i++) {
        hdrl_imagelist_set(list, hdrl_image_new(nx, ny), i);
    }

    return list;
}

/*---------------------------------------------------------------------------*/
/*
 * @param
 */
/*---------------------------------------------------------------------------*/
static int hdrl_persistence_get_time_const(
        const cpl_array        * nus)
{
    int n_tau = 0;

    for (int i=0; i<cpl_array_get_size(nus); i++) {
        if (cpl_array_get(nus, i, NULL) > 0) {
            n_tau = n_tau + 1;
        }
    }

    return n_tau;
}

/*---------------------------------------------------------------------------*/
/*
 * @param
 */
/*---------------------------------------------------------------------------*/
static const hdrl_imagelist * hdrl_persistence_convert_max_traps(
        const cpl_array        * nus,
        const hdrl_image       * himax,
        const int                n_tau)
{
    hdrl_imagelist * himaxs = hdrl_imagelist_new();

    for (int i=0; i<n_tau; i++) {
        hdrl_image * hinew = hdrl_image_duplicate(himax);
        const double nu_i = cpl_array_get(nus, i, NULL);
        hdrl_image_mul_scalar(hinew, (hdrl_value){nu_i, 0.0});
        hdrl_imagelist_set(himaxs, hinew, i);
    }

    return himaxs;
}

/*---------------------------------------------------------------------------*/
/*
 * @param
 */
/*---------------------------------------------------------------------------*/
static hdrl_imagelist * hdrl_persistence_calculate_rho(
        const cpl_array        * nus,
        const hdrl_image       * hiden,
        const int                n_tau)
{
    hdrl_imagelist * hirhos = hdrl_imagelist_new();

    for (int i=0; i<n_tau; i++) {
        hdrl_image * hinew = hdrl_image_duplicate(hiden);
        const double nu_i = cpl_array_get(nus, i, NULL);
        hdrl_image_mul_scalar(hinew, (hdrl_value){nu_i, 0.0});
        hdrl_imagelist_set(hirhos, hinew, i);
    }

    return hirhos;
}


/*----------------------------------------------------------------------------*/
/**
 * @brief generate some statistics for a Qi frame
 *
 * @param Qtot       the Qi frame for which to generate stats
 * @param trim_perc  throw away this percentage of the pixels, half on the low
 *                   end and half on the high end, before computing the mean
 *
 * @return a new propertylist populated with stats/keywords, or NULL on error
 */
/*----------------------------------------------------------------------------*/
static cpl_propertylist * hdrl_persistence_calc_stats(
        const hdrl_image       * Qtot,
        const double             trim_perc)
{
    cpl_error_ensure(Qtot, CPL_ERROR_NULL_INPUT, return NULL, " ");
    cpl_error_ensure(trim_perc >= 0.0 && trim_perc < 100.0,
            CPL_ERROR_ILLEGAL_INPUT, return NULL, "0 <= trim_perc < 100");

    hdrl_image * Qtot_local = hdrl_image_duplicate(Qtot);

    cpl_mask * Qtot_local_mask = hdrl_image_get_mask(Qtot_local);
    cpl_mask * mask_threshold =
    		cpl_mask_threshold_image_create(hdrl_image_get_image(Qtot_local), 0.0, DBL_MAX);
    cpl_mask_not(mask_threshold);

    cpl_mask_or(Qtot_local_mask, mask_threshold);
    cpl_mask_delete(mask_threshold);

    cpl_propertylist * qclist = cpl_propertylist_new();
    const int nx_0 = hdrl_image_get_size_x(Qtot_local);
    const int ny_0 = hdrl_image_get_size_y(Qtot_local);
    double trim = (double)floor((nx_0*ny_0)* trim_perc/2.0);

    const hdrl_value mean = hdrl_image_get_mean(Qtot_local);
    const hdrl_value minmax_mean = hdrl_image_get_minmax_mean(Qtot_local, trim, trim);
	const hdrl_value sigclip_mean =	hdrl_image_get_sigclip_mean(Qtot_local, 3., 3., 100);
	const hdrl_value weighted_mean = hdrl_image_get_weighted_mean(Qtot_local);
    const hdrl_value median = hdrl_image_get_median(Qtot_local);

    cpl_propertylist_append_double(qclist, "ESO QC PERSIST MEAN",
    		mean.data);
    cpl_propertylist_append_double(qclist, "ESO QC PERSIST MEANERR",
    		mean.error);

    cpl_propertylist_append_double(qclist, "ESO QC PERSIST MINMAX MEAN",
    		minmax_mean.data);
    cpl_propertylist_append_double(qclist, "ESO QC PERSIST MINMAX MEANERR",
    		minmax_mean.error);

    cpl_propertylist_append_double(qclist, "ESO QC PERSIST SIGCLIP MEAN",
    		sigclip_mean.data);
    cpl_propertylist_append_double(qclist, "ESO QC PERSIST SIGCLIP MEANERR",
    		sigclip_mean.error);

    cpl_propertylist_append_double(qclist, "ESO QC PERSIST WEIGHTED MEAN",
    		weighted_mean.data);
    cpl_propertylist_append_double(qclist, "ESO QC PERSIST WEIGHTED MEANERR",
    		weighted_mean.error);

    cpl_propertylist_append_double(qclist, "ESO QC PERSIST MEDIAN",
    		median.data);
    cpl_propertylist_append_double(qclist, "ESO QC PERSIST MEDIANERR",
    		median.error);

    cpl_propertylist_append_double(qclist, "ESO QC PERSIST STD",
            hdrl_image_get_stdev(Qtot_local));
    cpl_propertylist_append_double(qclist, "ESO QC PERSIST MIN",
            cpl_image_get_min(hdrl_image_get_image_const(Qtot_local)));
    cpl_propertylist_append_double(qclist, "ESO QC PERSIST MAX",
            cpl_image_get_max(hdrl_image_get_image_const(Qtot_local)));

    hdrl_image_delete(Qtot_local);
    return qclist;
}

/**@}*/
