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

#ifndef CX_STRUTILS_H
#define CX_STRUTILS_H

#include <stdarg.h>

#include "cxmacros.h"
#include "cxtypes.h"


CX_BEGIN_DECLS

/*
 * String comparison functions.
 */

cxint cx_strcasecmp(const cxchar *, const cxchar *);
cxint cx_strncasecmp(const cxchar *, const cxchar *, cxsize);
cxint cx_strempty(const cxchar *, const cxchar *);


/*
 * Utility functions modifing their string argument
 */

cxchar *cx_strlower(cxchar *);
cxchar *cx_strupper(cxchar *);

cxchar *cx_strtrim(cxchar *);
cxchar *cx_strrtrim(cxchar *);
cxchar *cx_strstrip(cxchar *);


/*
 * Utility functions which do not create a new string
 */

cxchar *cx_strskip(const cxchar *, cxint (*)(cxint));


/*
 * Utilities returning a newly allocated string.
 */

cxchar *cx_strdup(const cxchar *);
cxchar *cx_strndup(const cxchar *, cxsize);
cxchar *cx_strvdupf(const cxchar *, va_list) CX_GNUC_PRINTF(1, 0);

cxchar *cx_stpcpy(cxchar *, const cxchar *);

cxchar **cx_strsplit(const cxchar *, const cxchar *, cxint);
void cx_strfreev(cxchar **sarray);

cxchar *cx_strjoinv(const cxchar *, cxchar **);

CX_END_DECLS

#endif /* CX_STRUTILS_H */
