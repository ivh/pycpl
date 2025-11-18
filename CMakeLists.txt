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

if(NOT DEFINED PYCPL_RECIPE_DIR)
    if(DEFINED ENV{PYCPL_RECIPE_DIR})
        set(PYCPL_RECIPE_DIR $ENV{PYCPL_RECIPE_DIR})
    else()
        get_filename_component(PYCPL_RECIPE_DIR "${CPL_CPLCORE_LIBRARY}" DIRECTORY)
        string(APPEND PYCPL_RECIPE_DIR "/esopipes-plugins")
    endif()
endif()
message(STATUS "Builtin default recipe directory is ${PYCPL_RECIPE_DIR}")

if(NOT EXISTS ${PYCPL_RECIPE_DIR})
    message(WARNING "Configured default recipe directory '${PYCPL_RECIPE_DIR}' does not yet exist!")
    message(VERBOSE "The above path determines the default cpl.ui.Recipe.recipe_dir, and is used at Run-time.")
endif()

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
    src/hdrl/hdrl_image.cpp
    src/hdrl/hdrl_image_bindings.cpp
    src/dump_handler.cpp
    src/pycpl.cpp)

target_compile_definitions(cpl PRIVATE 
    PYCPL_VERSION=\"${PYCPL_VERSION}\"
    PYCPL_RECIPE_DIR=\"${PYCPL_RECIPE_DIR}\"
    $<$<BOOL:${UNIX}>:_XOPEN_SOURCE=700>
    )
target_compile_options(cpl PRIVATE
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-pipe -g3 -ggdb -O0 -rdynamic -fno-inline -fno-builtin -pedantic -Wextra -Wall -W -Wcast-align -Winline -Wmissing-noreturn -Wpointer-arith -Wshadow -Wsign-compare -Wundef -Wunreachable-code -Wwrite-strings -Wmissing-field-initializers -Wmissing-format-attribute>
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:Clang,AppleClang>>:-pipe -g3 -O0 -fno-inline -fno-builtin -pedantic -Wextra -Wall -W -Wcast-align -Winline -Wimplicit-function-declaration -Wmissing-noreturn -Wincompatible-pointer-types -Wpointer-arith -Wshadow -Wsign-compare -Wundef -Wunreachable-code -Wwrite-strings -Wmissing-field-initializers -Wmissing-format-attribute>
    $<$<AND:$<BOOL:${SANITIZE}>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:-fsanitize=${SANITIZE} -fno-omit-frame-pointer>)
target_include_directories(cpl BEFORE
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PUBLIC ${CPL_INCLUDE_DIR} ${HDRL_INCLUDE_DIR})
target_link_options(cpl PRIVATE
    $<$<AND:$<BOOL:${SANITIZE}>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:-fsanitize=${SANITIZE}>)
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
