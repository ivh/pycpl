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

#ifndef CPL_PROPERTY_DICB_H
#define CPL_PROPERTY_DICB_H

#include "cpl_property_impl.h"

/*-----------------------------------------------------------------------------
                        Type definitions
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @enum     cpl_property_sorttype_dicb
  @internal
  @brief    The FITS key types according to DICB
  @note This enum may have no more than 256 entries, among which the value zero
        is reserved to mean undefined.

  This enum stores all possible DICB types for a FITS keyword. These determine
  the order of appearance in a header, they are a crucial point for DICB (ESO)
  compliance. This classification is internal to CPL.

 */
/*----------------------------------------------------------------------------*/
typedef enum cpl_property_sorttype_dicb
{
    CPL_DICB_UNDEF = CPL_PROPERTY_SORT_UNDEF,

    CPL_DICB_TOP,

    /* Mandatory keywords */
    /* All FITS files */
    CPL_DICB_BITPIX,

    /* Per the FITS standard, NAXISn can go from 1 to 999. */
    CPL_DICB_NAXIS,
    CPL_DICB_NAXISn,

    /* Random groups only */
    CPL_DICB_GROUP,
    /* Extensions */
    CPL_DICB_PCOUNT,
    CPL_DICB_GCOUNT,
    /* Main header */
    CPL_DICB_EXTEND,
    /* Images */
    CPL_DICB_BSCALE,
    CPL_DICB_BZERO,
    /* Tables */
    CPL_DICB_TFIELDS,

    /* Per the FITS standard, TBCOLn is indexed and starts with 1 */
    CPL_DICB_TBCOLn,

    /* Per the FITS standard, TBCOLn is indexed and starts with 1 */
    CPL_DICB_TFORMn,

    /* Descriptive keywords defined by DICD */
    CPL_DICB_DESCRIBE,

    /* WCS keywords - may involve dimension digit(s) */
    CPL_DICB_WCS,

    /* Other primary keywords */
    CPL_DICB_PRIMARY,

    /* HIERARCH ESO keywords ordered according to DICB */
    /* Only the two first (3-letter) words count in the ordering */
    CPL_DICB_HIERARCH_XYZ = 1 << 5, /* This bit is shared by these ESO keys */
    CPL_DICB_HIERARCH_DPR = CPL_DICB_HIERARCH_XYZ,
    CPL_DICB_HIERARCH_OBS,
    CPL_DICB_HIERARCH_TPL,
    CPL_DICB_HIERARCH_GEN,
    CPL_DICB_HIERARCH_TEL,
    CPL_DICB_HIERARCH_INS,
    CPL_DICB_HIERARCH_DET,
    CPL_DICB_HIERARCH_LOG,
    CPL_DICB_HIERARCH_PRO,
    /* Other HIERARCH keywords */
    /* Only the first (3-letter "ESO") word count in the ordering */
    CPL_DICB_HIERARCH_ESO = CPL_DICB_HIERARCH_XYZ << 1,
    /* Except, in principle, there could be non-ESO cards */
    CPL_DICB_HIERARCH,

    /* HISTORY and COMMENT */
    CPL_DICB_HISTORY,
    CPL_DICB_COMMENT,
    /* END */
    CPL_DICB_END
} cpl_property_sorttype_dicb;

/*-----------------------------------------------------------------------------
                        Function prototypes
 -----------------------------------------------------------------------------*/

void
cpl_property_set_sortkey_dicb(cpl_property *self) CPL_INTERNAL CPL_ATTR_NONNULL;

#endif
