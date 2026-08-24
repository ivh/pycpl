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

#ifndef CX_MEMORY_H
#define CX_MEMORY_H

#include "cxtypes.h"


CX_BEGIN_DECLS

struct _cx_memory_vtable_
{
    cxptr (*malloc)(cxsize);
    cxptr (*calloc)(cxsize, cxsize);
    cxptr (*realloc)(cxptr, cxsize);
    void (*free)(cxptr);
};

typedef struct _cx_memory_vtable_ cx_memory_vtable;


/*
 * Memory allocation functions
 */

void cx_memory_vtable_set(const cx_memory_vtable *);
cxbool cx_memory_is_system_malloc(void);

cxptr cx_malloc(cxsize);
cxptr cx_malloc_clear(cxsize);
cxptr cx_calloc(cxsize, cxsize);
cxptr cx_realloc(cxptr, cxsize);
void cx_free(cxptr);

CX_END_DECLS

#endif /* CX_MEMORY_H */
