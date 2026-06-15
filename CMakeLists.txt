# This file is part of PyCPL the ESO CPL Python language bindings
# Copyright (C) 2020-2026 European Southern Observatory
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

cmake_minimum_required(VERSION 3.12)
project(cpl)

# Set policies
cmake_policy(VERSION 3.12)
# Safer library linking
cmake_policy(SET CMP0003 NEW)
# Automatically escape preprocessor definitions
cmake_policy(SET CMP0005 NEW)
# Make syntax problems errors
cmake_policy(SET CMP0010 NEW)
# Input directories must have CMakeLists.txt
cmake_policy(SET CMP0014 NEW)
# Do not interpret quoted or bracketed variables in if statments
cmake_policy(SET CMP0054 NEW)
# Usage of <PackageName>_ROOT variables
cmake_policy(SET CMP0074 NEW)
# Escape RPATH entries in intermediate scripts
cmake_policy(SET CMP0095 NEW)
# Deprecated FindPythonInterp and FindPythonLibs
# CMP0148 is only defined in cmake 3.27+
if(POLICY CMP0148)
    cmake_policy(SET CMP0148 NEW)
endif()


# Add local cmake module search path
list(INSERT CMAKE_MODULE_PATH 0 "${PROJECT_SOURCE_DIR}/cmake")

find_package(PkgConfig)
find_package(CPL "7.2" REQUIRED COMPONENTS cplcore cplui cpldfs cpldrs)
find_package(HDRL REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 REQUIRED)

if(NOT DEFINED PYCPL_RECIPE_DIRS)
    if(DEFINED ENV{PYCPL_RECIPE_DIR})
        string(REPLACE ":" ";" PYCPL_RECIPE_DIRS $ENV{PYCPL_RECIPE_DIR})
    else()
        get_filename_component(PYCPL_RECIPE_DIRS "${CPL_CPLCORE_LIBRARY}" DIRECTORY)
        string(APPEND PYCPL_RECIPE_DIRS "/esopipes-plugins")
    endif()
endif()

foreach(recipe_dir ${PYCPL_RECIPE_DIRS})
    if (NOT EXISTS ${recipe_dir})
        message(WARNING "Configured default recipe directory '${recipe_dir}' does not yet exist!")
    endif()
endforeach()

string(JOIN ":" PYCPL_RECIPE_DIR ${PYCPL_RECIPE_DIRS})
message(STATUS "Builtin default list of recipe search paths is '${PYCPL_RECIPE_DIR}'")
message(VERBOSE "The above list of paths determines the default cpl.ui.Recipe.recipe_dir, and is used at Run-time.")

pybind11_add_module(cpl
    src/cplcore/array.cpp
    src/cplcore/bivector.cpp
    src/cplcore/coords.cpp
    src/cplcore/error_bindings.cpp
    src/cplcore/error.cpp
    src/cplcore/errorframe.cpp
    src/cplcore/filter_bindings.cpp
    src/cplcore/image_bindings.cpp
    src/cplcore/image.cpp
    src/cplcore/imagelist.cpp
    src/cplcore/io_bindings.cpp
    src/cplcore/mask_bindings.cpp
    src/cplcore/mask.cpp
    src/cplcore/matrix_bindings.cpp
    src/cplcore/matrix.cpp
    src/cplcore/msg_bindings.cpp
    src/cplcore/msg.cpp
    src/cplcore/polynomial_bindings.cpp
    src/cplcore/polynomial.cpp
    src/cplcore/property_bindings.cpp
    src/cplcore/property.cpp
    src/cplcore/propertylist.cpp
    src/cplcore/table_bindings.cpp
    src/cplcore/table.cpp
    src/cplcore/type_bindings.cpp
    src/cplcore/types.cpp
    src/cplcore/vector_bindings.cpp
    src/cplcore/vector.cpp
    src/cpldfs/dfs_bindings.cpp
    src/cpldfs/dfs.cpp
    src/cpldrs/apertures_bindings.cpp
    src/cpldrs/apertures.cpp
    src/cpldrs/detector_bindings.cpp
    src/cpldrs/detector.cpp
    src/cpldrs/fft_bindings.cpp
    src/cpldrs/fft.cpp
    src/cpldrs/fit_bindings.cpp
    src/cpldrs/fit.cpp
    src/cpldrs/geom_img_bindings.cpp
    src/cpldrs/geom_img.cpp
    src/cpldrs/photom_bindings.cpp
    src/cpldrs/photom.cpp
    src/cpldrs/ppm_bindings.cpp
    src/cpldrs/ppm.cpp
    src/cpldrs/wcs_bindings.cpp
    src/cpldrs/wcs.cpp
    src/cpldrs/wlcalib_bindings.cpp
    src/cpldrs/wlcalib.cpp
    src/cplui/frame_bindings.cpp
    src/cplui/frame.cpp
    src/cplui/frameset.cpp
    src/cplui/parameter_bindings.cpp
    src/cplui/parameter.cpp
    src/cplui/parameterlist.cpp
    src/cplui/plugin_bindings.cpp
    src/cplui/plugin.cpp
    src/hdrlcore/error_bindings.cpp
    src/hdrlcore/error.cpp
    src/hdrlcore/errorframe.cpp
    src/hdrlcore/image_bindings.cpp
    src/hdrlcore/image.cpp
    src/hdrlcore/imagelist_bindings.cpp
    src/hdrlcore/imagelist.cpp
    src/hdrlcore/parameter.cpp
    src/hdrlcore/pycpl_types.cpp
    src/hdrlcore/spectrum_bindings.cpp
    src/hdrlcore/spectrum.cpp
    src/hdrldebug/types_bindings.cpp
    src/hdrldebug/types.cpp
    src/hdrlfunc/airmass_bindings.cpp
    src/hdrlfunc/airmass.cpp
    src/hdrlfunc/barycorr_bindings.cpp
    src/hdrlfunc/barycorr.cpp
    src/hdrlfunc/bpm_bindings.cpp
    src/hdrlfunc/bpm.cpp
    src/hdrlfunc/catalogue_bindings.cpp
    src/hdrlfunc/catalogue.cpp
    src/hdrlfunc/collapse_bindings.cpp
    src/hdrlfunc/collapse.cpp
    src/hdrlfunc/dar_bindings.cpp
    src/hdrlfunc/dar.cpp
    src/hdrlfunc/efficiency_bindings.cpp
    src/hdrlfunc/efficiency.cpp
    src/hdrlfunc/flat_bindings.cpp
    src/hdrlfunc/flat.cpp
    src/hdrlfunc/fpn_bindings.cpp
    src/hdrlfunc/fpn.cpp
    src/hdrlfunc/fringe_bindings.cpp
    src/hdrlfunc/fringe.cpp
    src/hdrlfunc/lacosmic_bindings.cpp
    src/hdrlfunc/lacosmic.cpp
    src/hdrlfunc/maglim_bindings.cpp
    src/hdrlfunc/maglim.cpp
    src/hdrlfunc/overscan_bindings.cpp
    src/hdrlfunc/overscan.cpp
    src/hdrlfunc/resample_bindings.cpp
    src/hdrlfunc/resample.cpp
    src/hdrlfunc/response_bindings.cpp
    src/hdrlfunc/response.cpp
    src/hdrlfunc/strehl_bindings.cpp
    src/hdrlfunc/strehl.cpp
    src/dump_handler.cpp
    src/pycpl.cpp)

target_compile_definitions(cpl PRIVATE
    PYCPL_VERSION=\"${PYCPL_VERSION}\"
    PYCPL_RECIPE_DIR=\"${PYCPL_RECIPE_DIR}\"
    HDRL_USE_PRIVATE
    $<$<BOOL:${UNIX}>:_XOPEN_SOURCE=800>
    )
target_compile_options(cpl PRIVATE
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-pipe -g3 -ggdb -O0 -rdynamic -fno-inline -fno-builtin -pedantic -Wextra -Wall -W -Wcast-align -Winline -Wmissing-noreturn -Wpointer-arith -Wshadow -Wsign-compare -Wundef -Wunreachable-code -Wwrite-strings -Wmissing-field-initializers -Wmissing-format-attribute>
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:Clang,AppleClang>>:-pipe -g3 -O0 -fno-inline -fno-builtin -pedantic -Wextra -Wall -W -Wcast-align -Winline -Wimplicit-function-declaration -Wmissing-noreturn -Wincompatible-pointer-types -Wpointer-arith -Wshadow -Wsign-compare -Wundef -Wunreachable-code -Wwrite-strings -Wmissing-field-initializers -Wmissing-format-attribute>
    $<$<AND:$<BOOL:${SANITIZE}>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:-fsanitize=${SANITIZE} -fno-omit-frame-pointer>)
target_include_directories(cpl BEFORE
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PUBLIC ${CPL_INCLUDE_DIR} ${HDRL_INCLUDE_DIR}
    PUBLIC "${HDRL_INCLUDE_DIR}/..")
target_link_options(cpl PRIVATE
    $<$<AND:$<BOOL:${SANITIZE}>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:-fsanitize=${SANITIZE}>
    # Force RPATH instead of RUNPATH for transitive library loading
    $<$<PLATFORM_ID:Linux>:-Wl,--disable-new-dtags>)
target_link_libraries(cpl PRIVATE HDRL::hdrl CPL::cpldrs CPL::cpldfs CPL::cplui CPL::cplcore)

set_target_properties(cpl PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS OFF
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS OFF
    BUILD_RPATH_USE_ORIGIN TRUE
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$<IF:$<PLATFORM_ID:Darwin>,@loader_path,$ORIGIN>")
