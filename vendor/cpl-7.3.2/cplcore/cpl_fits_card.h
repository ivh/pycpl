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

#ifndef CPL_FITS_CARD_H
#define CPL_FITS_CARD_H

#include <cpl_propertylist.h>
#include "cpl_property_impl.h"

/*-----------------------------------------------------------------------------
                        Function prototypes
 -----------------------------------------------------------------------------*/

static int
cpl_fits_key_is_unique(const char **[], const cpl_cstr *, cpl_size)
CPL_ATTR_NONNULL CPL_ATTR_PURE;

static void cpl_fits_key_free_unique(const char **[]) CPL_ATTR_NONNULL;
static void cpl_fits_key_reset_unique(const char **[]) CPL_ATTR_NONNULL;

static int
cpl_fits_key_is_comment(const cpl_cstr *) CPL_ATTR_NONNULL CPL_ATTR_PURE;

#endif
