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

#ifndef CX_FILEUTILS_H
#define CX_FILEUTILS_H

#include "cxtypes.h"


CX_BEGIN_DECLS

cxlong cx_path_max(const cxchar *);
cxchar *cx_path_alloc(const cxchar *);

CX_END_DECLS

#endif /* CX_FILEUTILS_H */
