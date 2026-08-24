/*
 * This file is part of the ESO C Extension Library
 * Copyright (C) 2001-2026 European Southern Observatory
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CX_UTILS_H
#define CX_UTILS_H

#include <stdarg.h>

#include "cxmacros.h"
#include "cxtypes.h"


CX_BEGIN_DECLS

/*
 * Static information retrieval
 */

const cxchar *cx_program_get_name(void);
void cx_program_set_name(const cxchar *);

cxlong cx_line_max(void);


/*
 * Bit tests
 */

cxint cx_bits_find(cxuint32, cxint);
cxint cx_bits_rfind(cxuint32, cxint);


/*
 * Miscellaneous utility functions
 */

cxint cx_snprintf(cxchar *, cxsize, const cxchar *, ...) CX_GNUC_PRINTF(3, 4);
cxint cx_vsnprintf(cxchar *, cxsize, const cxchar *, va_list)
    CX_GNUC_PRINTF(3, 0);
cxint cx_asprintf(cxchar **, const cxchar *, ...) CX_GNUC_PRINTF(2, 3);
cxint cx_vasprintf(cxchar **, const cxchar *, va_list) CX_GNUC_PRINTF(2, 0);

cxchar *cx_line_alloc(void);

CX_END_DECLS

#endif /* CX_UTILS_H */
