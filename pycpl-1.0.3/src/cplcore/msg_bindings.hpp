// This file is part of PyCPL the ESO CPL Python language bindings
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

#ifndef PYCPL_MSG_BINDINGS_HPP
#define PYCPL_MSG_BINDINGS_HPP

#include <pybind11/pybind11.h>

namespace py = pybind11;

/**
 * @brief Binds Messages
 * @param m The Pybind Python module to which Messages are bound (usually named
 *'cpl')
 *
 **/
void bind_message(py::module& m);

#endif  // PYCPL_MSG_BINDINGS_HPP