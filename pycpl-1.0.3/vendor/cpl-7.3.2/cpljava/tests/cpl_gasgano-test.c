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

#include "cpl_test.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <jni.h>

#include "ltdl.h"

#include "org_eso_cpl_jni_CPLControl.h"
#include "org_eso_cpl_jni_JNIParameterImp.h"
#include "org_eso_cpl_jni_JNIRecipe.h"
#include "org_eso_cpl_jni_LibraryLoader.h"
#include "org_eso_cpl_jni_PluginLibrary.h"

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/

int
main(void)
{
    jstring nullstring;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Only tests whether the function can actually be called, i.e. linking */
    Java_org_eso_cpl_jni_CPLControl_nativeEnsureSetup(NULL, 0);

    cpl_test_error(CPL_ERROR_NONE);

    /* Only tests the function in a non-useful mode */
    nullstring = Java_org_eso_cpl_jni_CPLControl_nativeGetVersion(NULL, 0);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_null(nullstring);

    return cpl_test_end(0);
}
