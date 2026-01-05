# This file is part of PyCPL the ESO CPL Python language bindings
# Copyright (C) 2020-2024 European Southern Observatory
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

#[=======================================================================[.rst:
FindHDRL
--------

Find the High Level Data Reduction Library headers and libraries. The module
can be configured using the following variables:

    HDRL_ROOT:    Location where HDRL is installed

or the corresponding environment variable HDRL_ROOT.

The module also takes into account the environment variable HDRLDIR.
If it is set it should point to the location of the HDRL installation.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``HDRL::hdrl``
  The hdrl library component.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``HDRL_FOUND``
  "True" if both, HDRL header files and the HDRL libraries were found.

``HDRL_INCLUDE_DIR``
  Path where the HDRL header files are located.

``HDRL_LIBRARY``
  Path where the hdrl library is located.

``HDRL_VERSION``
  Full version string of the HDRL libraries.

#]=======================================================================]

cmake_policy(VERSION 3.12)

include(FindPackageHandleStandardArgs)

if(NOT PKG_CONFIG_FOUND)
    find_package(PkgConfig QUIET)
endif()

# For backwards compatibility also honor the HDRLDIR environment variable.
if(NOT HDRL_ROOT AND (NOT "$ENV{HDRLDIR}" STREQUAL ""))
    set(HDRL_ROOT "$ENV{HDRLDIR}")
endif()

if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_HDRL QUIET IMPORTED_TARGET GLOBAL hdrl)
    if(PC_HDRL_FOUND)
        set(HDRL_VERSION ${PC_HDRL_VERSION})
    endif()
endif()

# Search for the header files
find_path(HDRL_INCLUDE_DIR
          NAMES hdrl.h
          PATH_SUFFIXES hdrl
          HINTS ${HDRL_ROOT}/include ${PC_HDRL_INCLUDE_DIRS})
mark_as_advanced(HDRL_INCLUDE_DIR)

# Search for the library
set(FIND_LIBRARY_USE_LIB64_PATHS True)
find_library(HDRL_LIBRARY
             NAMES hdrl
             HINTS ${HDRL_ROOT}/lib ${PC_HDRL_LIBRARY_DIRS})
mark_as_advanced(HDRL_LIBRARY)

# Determine the library version from the header files
if(HDRL_INCLUDE_DIR AND NOT HDRL_VERSION)
    if(EXISTS ${HDRL_INCLUDE_DIR}/hdrl_version.h)
        file(STRINGS "${HDRL_INCLUDE_DIR}/hdrl_version.h"
             hdrl_version
             REGEX "^[\t ]*#define[\t ]+HDRL_VERSION_STRING[\t ]+\"[0-9]+\.[0-9]+.*\"")
        if(hdrl_version)
            string(REGEX REPLACE
                   "^[\t ]*#define[\t ]+HDRL_VERSION_STRING[\t ]+\"([0-9]+\.[0-9][^a-z \"]*).*" "\\1"
                   HDRL_VERSION ${hdrl_version})
        endif()
        unset(hdrl_version)
    endif()
endif()

find_package_handle_standard_args(HDRL
                                  REQUIRED_VARS HDRL_INCLUDE_DIR HDRL_LIBRARY
                                  VERSION_VAR HDRL_VERSION)

if(HDRL_FOUND)
    if(NOT TARGET HDRL::hdrl)
        add_library(HDRL::hdrl UNKNOWN IMPORTED)
        set_target_properties(HDRL::hdrl PROPERTIES
                              INTERFACE_INCLUDE_DIRECTORIES "${HDRL_INCLUDE_DIR}")
        if(EXISTS "${HDRL_LIBRARY}")
            set_target_properties(HDRL::hdrl PROPERTIES
                                  IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                                  IMPORTED_LOCATION ${HDRL_LIBRARY})
        endif()
    endif()
endif()
