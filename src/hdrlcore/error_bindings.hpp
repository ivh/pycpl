// This file is part of the PyHDRL Python language bindings
// Copyright (C) 2020-2024 European Southern Observatory
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef PYHDRL_CORE_ERROR_BINDINGS_HPP_
#define PYHDRL_CORE_ERROR_BINDINGS_HPP_

#include <pybind11/pybind11.h>

namespace py = pybind11;

/**
 * @brief Binds CPL Errors and ErrorStacks (from error.hpp)
 * @param m The Pybind Python module to which Errors & ErrorStacks are bound
 * (usually named 'cplcore')
 *
 * The following is the list of Python objects that are bound when this function
 * is run: This function is intended to be called by the 'main' binding function
 * in src/bindings.cpp
 */
void bind_hdrl_errors(py::module& m);

#endif  // PYHDRL_CORE_ERROR_BINDINGS_HPP_