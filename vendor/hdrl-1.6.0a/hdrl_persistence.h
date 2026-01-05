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

#ifndef HDRL_PERSISTENCE_H
#define HDRL_PERSISTENCE_H

/*-----------------------------------------------------------------------------
                                Includes
 -----------------------------------------------------------------------------*/
#include <cpl.h>

#include "hdrl.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                  Typedefs
 -----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------
                            Functions prototypes
 -----------------------------------------------------------------------------*/

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
		cpl_propertylist       ** persistence_qc);

/*-----------------------------------------------------------------------------
             Private declarations - must not be used outside of hdrl
 -----------------------------------------------------------------------------*/

#ifdef HDRL_USE_PRIVATE

#endif

CPL_END_DECLS

#endif
