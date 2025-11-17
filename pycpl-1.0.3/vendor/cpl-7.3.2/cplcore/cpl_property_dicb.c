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

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_property_dicb.h"

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_property_dicb   DICB specific property functionality
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_property_dicb.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Set the DICB type of the name
  @param self  The property, must be non-NULL
  @return   nothing
  @note This internal function has no error checking
  @see cpl_property_compare_sortkey(), cpl_propertylist_sort(), cpl_fits_set_key

  The function does not validate its input according to the FITS standard,
  it merely provides the type specified for DICB in order to sort the FITS
  keys.

  The correctness of the ordering in this function is difficult to verify
  because CFITSIO itself orders several cards in accordance with the FITS
  standard. For the same reason, any incorrectness regarding such keys will
  have no effect on any written FITS header.

 */
/*----------------------------------------------------------------------------*/
void
cpl_property_set_sortkey_dicb(cpl_property *self)
{
    const size_t size = cpl_property_get_size_name(self);
    const char *key = cpl_property_get_name_(self);

    /* The default value for a FITS key of length less than 9 */
    cpl_property_sorttype_dicb kt = CPL_DICB_PRIMARY;


    /* The sort key must fit in a single char */
    cx_assert(CPL_DICB_END == (unsigned char)CPL_DICB_END);

    /* First matching the length of the key to the length of the string literal
       allows for a static-length memcmp() which should be inline-able. */

    /* NB: The indentation that aligns the calls to memcmp() helps to
       ensure that strings of identical length share the correct branch */

    switch (size) { /* The number of characters excludes the null-byte */
        case 0:
        case 1:
            break;

        case 2:
            if (!memcmp(key, "RA", 2))
                kt = CPL_DICB_DESCRIBE;
            break;

        case 3:
            if (!memcmp(key, "DEC", 3))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "LST", 3))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "UTC", 3))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "END", 3))
                kt = CPL_DICB_END;
            break;

        case 4:
            if (!memcmp(key, "DATE", 4))
                kt = CPL_DICB_DESCRIBE;
            break;

        case 5:
            if (!memcmp(key, "BZERO", 5))
                kt = CPL_DICB_BZERO;
            else if (!memcmp(key, "NAXIS", 5))
                kt = CPL_DICB_NAXIS;
            else if (!memcmp(key, "GROUP", 5))
                kt = CPL_DICB_GROUP;
            else if (!memcmp(key, "BLANK", 5))
                kt = CPL_DICB_DESCRIBE;
            /* With NAXIS 1-9, these are WCS keys */
            else if (key[3] == '_' &&
                     (!memcmp(key, "PC", 2) || !memcmp(key, "PV", 2) ||
                      !memcmp(key, "PS", 2) || !memcmp(key, "CD", 2)) &&
                     '1' <= key[2] && key[2] <= '9' && '1' <= key[4] &&
                     key[4] <= '9')
                kt = CPL_DICB_WCS;
            else if (!memcmp(key, "ESO ", 4))
                kt = CPL_DICB_HIERARCH_ESO;

            break;

        case 6:
            if (!memcmp(key, "BITPIX", 6))
                kt = CPL_DICB_BITPIX;
            else if (!memcmp(key, "BSCALE", 6))
                kt = CPL_DICB_BSCALE;
            else if (!memcmp(key, "EXTEND", 6))
                kt = CPL_DICB_EXTEND;
            else if (!memcmp(key, "PCOUNT", 6))
                kt = CPL_DICB_PCOUNT;
            else if (!memcmp(key, "GCOUNT", 6))
                kt = CPL_DICB_GCOUNT;
            else if (!memcmp(key, "SIMPLE", 6))
                kt = CPL_DICB_TOP;
            else if (!memcmp(key, "ORIGIN", 6))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "OBJECT", 6))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "PI-COI", 6))
                kt = CPL_DICB_DESCRIBE;
            /* With NAXIS 10-99, these are WCS keys */
            else if (CPL_UNLIKELY(key[3] == '_' && (!memcmp(key, "PC", 2) ||
                                                    !memcmp(key, "PV", 2) ||
                                                    !memcmp(key, "PS", 2) ||
                                                    !memcmp(key, "CD", 2))) &&
                     '1' <= key[2] && key[2] <= '9' && '1' <= key[4] &&
                     key[4] <= '9' && '0' <= key[5] && key[5] <= '9')
                kt = CPL_DICB_WCS;
            else if (CPL_UNLIKELY(key[4] == '_' && (!memcmp(key, "PC", 2) ||
                                                    !memcmp(key, "PV", 2) ||
                                                    !memcmp(key, "PS", 2) ||
                                                    !memcmp(key, "CD", 2))) &&
                     '1' <= key[2] && key[2] <= '9' && '0' <= key[3] &&
                     key[3] <= '9' && '1' <= key[5] && key[5] <= '9')
                kt = CPL_DICB_WCS;
            else if (CPL_UNLIKELY(!memcmp(key, "ESO ", 4)))
                kt = CPL_DICB_HIERARCH_ESO;
            else {
                cpl_property_sorttype_dicb mykt = kt;

                /* NAXISn/TFORMn/TBCOLn, n = 1..9 */

                if (!memcmp(key, "NAXIS", 5))
                    mykt = CPL_DICB_NAXISn;
                else if (!memcmp(key, "TFORM", 5))
                    mykt = CPL_DICB_TFORMn;
                else if (!memcmp(key, "TBCOL", 5))
                    mykt = CPL_DICB_TBCOLn;
                else if (!memcmp(key, "CRVAL", 5))
                    mykt = CPL_DICB_WCS;
                else if (!memcmp(key, "CRPIX", 5))
                    mykt = CPL_DICB_WCS;
                else if (!memcmp(key, "CDELT", 5))
                    mykt = CPL_DICB_WCS;
                else if (!memcmp(key, "CTYPE", 5))
                    mykt = CPL_DICB_WCS;
                else if (!memcmp(key, "CUNIT", 5))
                    mykt = CPL_DICB_WCS;
                else if (!memcmp(key, "CRDER", 5))
                    mykt = CPL_DICB_WCS;
                else if (!memcmp(key, "CSYER", 5))
                    mykt = CPL_DICB_WCS;

                if (mykt != kt && '1' <= key[5] && key[5] <= '9') {
                    kt = mykt;
                }
            }
            break;
        case 7:
            if (!memcmp(key, "HISTORY", 7))
                kt = CPL_DICB_HISTORY;
            else if (!memcmp(key, "COMMENT", 7))
                kt = CPL_DICB_COMMENT;
            else if (!memcmp(key, "TFIELDS", 7))
                kt = CPL_DICB_TFIELDS;
            else if (!memcmp(key, "EXPTIME", 7))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "MJD-OBS", 7))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "MJD-END", 7))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "EQUINOX", 7))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "TIMESYS", 7))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "RADESYS", 7))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "WCSAXES", 7))
                kt = CPL_DICB_WCS;
            else if (!memcmp(key, "WCSNAME", 7))
                kt = CPL_DICB_WCS;
            else if (CPL_UNLIKELY(!memcmp(key, "ESO ", 4)))
                kt = CPL_DICB_HIERARCH_ESO;
            else {
                cpl_property_sorttype_dicb mykt = kt;

                /* NAXISn/TFORMn/TBCOLn, n = 10..99 */

                if (!memcmp(key, "TFORM", 5))
                    mykt = CPL_DICB_TFORMn;
                else if (!memcmp(key, "TBCOL", 5))
                    mykt = CPL_DICB_TBCOLn;
                else if (CPL_UNLIKELY(!memcmp(key, "NAXIS", 5)))
                    mykt = CPL_DICB_NAXISn;
                else if (CPL_UNLIKELY(!memcmp(key, "CRVAL", 5)))
                    mykt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(!memcmp(key, "CRPIX", 5)))
                    mykt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(!memcmp(key, "CDELT", 5)))
                    mykt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(!memcmp(key, "CTYPE", 5)))
                    mykt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(!memcmp(key, "CUNIT", 5)))
                    mykt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(!memcmp(key, "CRDER", 5)))
                    mykt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(!memcmp(key, "CSYER", 5)))
                    mykt = CPL_DICB_WCS;

                if (mykt != kt && '1' <= key[5] && key[5] <= '9' &&
                    '0' <= key[6] && key[6] <= '9') {
                    kt = mykt;
                } /* With NAXIS 10-99, these are WCS keys */
                else if (CPL_UNLIKELY(key[3] == '_' &&
                                      (!memcmp(key, "PC", 2) ||
                                       !memcmp(key, "PV", 2) ||
                                       !memcmp(key, "PS", 2) ||
                                       !memcmp(key, "CD", 2))) &&
                         '1' <= key[2] && key[2] <= '9' && '1' <= key[4] &&
                         key[4] <= '9' && '0' <= key[5] && key[5] <= '9' &&
                         '0' <= key[6] && key[6] <= '9')
                    kt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(key[4] == '_' &&
                                      (!memcmp(key, "PC", 2) ||
                                       !memcmp(key, "PV", 2) ||
                                       !memcmp(key, "PS", 2) ||
                                       !memcmp(key, "CD", 2))) &&
                         '1' <= key[2] && key[2] <= '9' && '0' <= key[3] &&
                         key[3] <= '9' && '1' <= key[5] && key[5] <= '9' &&
                         '0' <= key[6] && key[6] <= '9')
                    kt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(key[5] == '_' &&
                                      (!memcmp(key, "PC", 2) ||
                                       !memcmp(key, "PV", 2) ||
                                       !memcmp(key, "PS", 2) ||
                                       !memcmp(key, "CD", 2))) &&
                         '1' <= key[2] && key[2] <= '9' && '0' <= key[3] &&
                         key[3] <= '9' && '0' <= key[4] && key[4] <= '9' &&
                         '1' <= key[6] && key[6] <= '9')
                    kt = CPL_DICB_WCS;
            }
            break;

        case 8:
            if (!memcmp(key, "XTENSION", 8))
                kt = CPL_DICB_TOP;
            else if (!memcmp(key, "TELESCOP", 8))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "INSTRUME", 8))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "DATE-OBS", 8))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "DATE-END", 8))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "OBSERVER", 8))
                kt = CPL_DICB_DESCRIBE;
            else if (!memcmp(key, "RADECSYS", 8))
                kt = CPL_DICB_DESCRIBE;
            else if (CPL_UNLIKELY(!memcmp(key, "ESO ", 4)))
                kt = CPL_DICB_HIERARCH_ESO;
            else {
                cpl_property_sorttype_dicb mykt = kt;

                /* NAXISn/TFORMn/TBCOLn, n = 100..999 */

                if (!memcmp(key, "TFORM", 5))
                    mykt = CPL_DICB_TFORMn;
                else if (!memcmp(key, "TBCOL", 5))
                    mykt = CPL_DICB_TBCOLn;
                else if (CPL_UNLIKELY(!memcmp(key, "NAXIS", 5)))
                    mykt = CPL_DICB_NAXISn;
                else if (CPL_UNLIKELY(!memcmp(key, "CRVAL", 5)))
                    mykt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(!memcmp(key, "CRPIX", 5)))
                    mykt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(!memcmp(key, "CDELT", 5)))
                    mykt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(!memcmp(key, "CTYPE", 5)))
                    mykt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(!memcmp(key, "CUNIT", 5)))
                    mykt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(!memcmp(key, "CRDER", 5)))
                    mykt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(!memcmp(key, "CSYER", 5)))
                    mykt = CPL_DICB_WCS;

                if (mykt != kt && '1' <= key[5] && key[5] <= '9' &&
                    '0' <= key[6] && key[6] <= '9' && '0' <= key[7] &&
                    key[7] <= '9') {
                    kt = mykt;
                } /* With NAXIS 100-999, these are WCS keys */
                else if (CPL_UNLIKELY(key[4] == '_' &&
                                      (!memcmp(key, "PC", 2) ||
                                       !memcmp(key, "PV", 2) ||
                                       !memcmp(key, "PS", 2) ||
                                       !memcmp(key, "CD", 2))) &&
                         '1' <= key[2] && key[2] <= '9' && '0' <= key[3] &&
                         key[3] <= '9' && '1' <= key[5] && key[5] <= '9' &&
                         '0' <= key[6] && key[6] <= '9' && '0' <= key[7] &&
                         key[7] <= '9')
                    kt = CPL_DICB_WCS;
                else if (CPL_UNLIKELY(key[5] == '_' &&
                                      (!memcmp(key, "PC", 2) ||
                                       !memcmp(key, "PV", 2) ||
                                       !memcmp(key, "PS", 2) ||
                                       !memcmp(key, "CD", 2))) &&
                         '1' <= key[2] && key[2] <= '9' && '0' <= key[3] &&
                         key[3] <= '9' && '0' <= key[4] && key[4] <= '9' &&
                         '1' <= key[6] && key[6] <= '9' && '0' <= key[7] &&
                         key[7] <= '9')
                    kt = CPL_DICB_WCS;
            }
            break;

        default:
            /* With a length of 9 or more, this is not a standard key... */

            if (CPL_LIKELY(!memcmp(key, "ESO ", 4))) {
                /* NB: The FITS key cannot end with a space, so these checks
               are only needed here in the default branch */

                /* Specific HIERARCH ESO types take precedence
               - most frequent ones first */

                /* Each type can have an additional character appended to it, e.g.
               "ESO INS1 DID" so only the first three characters are compared */

                if (!memcmp(key + 4, "DET", 3))
                    kt = CPL_DICB_HIERARCH_DET;
                else if (!memcmp(key + 4, "INS", 3))
                    kt = CPL_DICB_HIERARCH_INS;
                else if (!memcmp(key + 4, "OBS", 3))
                    kt = CPL_DICB_HIERARCH_OBS;
                else if (!memcmp(key + 4, "TEL", 3))
                    kt = CPL_DICB_HIERARCH_TEL;
                else if (!memcmp(key + 4, "TPL", 3))
                    kt = CPL_DICB_HIERARCH_TPL;
                else if (!memcmp(key + 4, "DPR", 3))
                    kt = CPL_DICB_HIERARCH_DPR;
                else if (!memcmp(key + 4, "GEN", 3))
                    kt = CPL_DICB_HIERARCH_GEN;
                else if (!memcmp(key + 4, "LOG", 3))
                    kt = CPL_DICB_HIERARCH_LOG;
                else if (!memcmp(key + 4, "PRO", 3))
                    kt = CPL_DICB_HIERARCH_PRO;
                else
                    kt = CPL_DICB_HIERARCH_ESO;
            }
            else {
                kt = CPL_DICB_HIERARCH;
            }

            break;
    }

    cpl_property_set_sortkey_(self, (cpl_property_sorttype)kt);
}

/**@}*/
