/*
 * hdrl_download.c
 *
 * Stub implementation for pycpl wheel build - libcurl not required.
 * These functions are not used by any HDRL algorithm and are not bound by PyHDRL.
 */

/*
 * This file is part of HDRL
 * Copyright (C) 2022 European Southern Observatory
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

#include <stddef.h>
#include <cpl.h>

char * hdrl_download_url_to_buffer(const char * url, size_t * data_length)
{
    (void)url;
    (void)data_length;
    cpl_error_set_message(cpl_func, CPL_ERROR_UNSUPPORTED_MODE,
        "hdrl_download_url_to_buffer: libcurl support not compiled in");
    return NULL;
}

cpl_error_code hdrl_download_url_to_file(const char * url, const char * filename)
{
    (void)url;
    (void)filename;
    return cpl_error_set_message(cpl_func, CPL_ERROR_UNSUPPORTED_MODE,
        "hdrl_download_url_to_file: libcurl support not compiled in");
}
