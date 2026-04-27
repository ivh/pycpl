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

#include <pybind11/pybind11.h>

#include "hdrlfunc/efficiency.hpp"
#include "hdrlfunc/efficiency_bindings.hpp"

namespace py = pybind11;

void bind_efficiency(py::module& m) {
    // Expose the Efficiency class and its static methods
    py::class_<hdrl::func::Efficiency>(m, "Efficiency")
        .def_static("create_parameter",
                    &hdrl::func::Efficiency::create_parameter,
                    "Create an HDRL efficiency parameter",
                    py::arg("Ap"), py::arg("Am"), py::arg("G"), py::arg("Tex"), py::arg("Atel"))

        .def_static("create_response_parameter",
                    &hdrl::func::Efficiency::create_response_parameter,
                    "Create an HDRL response parameter",
                    py::arg("Ap"), py::arg("Am"), py::arg("G"), py::arg("Tex"))

        .def_static("compute",
                    &hdrl::func::Efficiency::compute,
                    "Compute HDRL efficiency",
                    py::arg("I_std_arg"), py::arg("I_std_ref"), py::arg("E_x"), py::arg("pars"))

        .def_static("compute_response_core",
                    &hdrl::func::Efficiency::compute_response_core,
                    "Compute HDRL response core",
                    py::arg("I_std_arg"), py::arg("I_std_ref"), py::arg("E_x"), py::arg("pars"));
}


