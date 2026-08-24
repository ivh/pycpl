// This file is part of the PyHDRL Python language bindings
// Copyright (C) 2023-2026 European Southern Observatory
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

#include "hdrlfunc/airmass_bindings.hpp"

#include "hdrlcore/value.hpp"  // IWYU pragma: keep
#include "hdrlfunc/airmass.hpp"

namespace py = pybind11;

void
bind_airmass(py::module& m)
{
  // Define the airmass approximation enum
  py::enum_<hdrl_airmass_approx>(m, "AirmassApprox")
      .value("Hardie", HDRL_AIRMASS_APPROX_HARDIE)
      .value("YoungIrvine", HDRL_AIRMASS_APPROX_YOUNG_IRVINE)
      .value("Young", HDRL_AIRMASS_APPROX_YOUNG);

  // Define the Airmass class
  py::class_<hdrl::func::Airmass>(m, "Airmass")
      .def(
          py::init<hdrl::core::Value, hdrl::core::Value, hdrl::core::Value,
                   hdrl::core::Value, hdrl::core::Value, hdrl_airmass_approx>(),
          py::arg("ra"), py::arg("dec"), py::arg("lst"), py::arg("exptime"),
          py::arg("latitude"), py::arg("type"),
          R"docstring(
                 Create an Airmass computation object.

                 Parameters
                 ----------
                 ra : tuple or None
                     Right Ascension in degrees with error (data, error) or None for (0, 0).
                 dec : tuple or None
                     Declination in degrees with error (data, error) or None for (0, 0).
                 lst : tuple or None
                     Local Sidereal Time in seconds with error (data, error) or None for (0, 0).
                 exptime : tuple or None
                     Exposure time in seconds with error (data, error) or None for (0, 0).
                 latitude : tuple or None
                     Observatory latitude in degrees with error (data, error) or None for (0, 0).
                 type : hdrl.func.AirmassApprox
                     Airmass approximation method (Hardie, YoungIrvine, or Young).
             )docstring")
      .def("compute", &hdrl::func::Airmass::compute,
           R"docstring(
                 Compute the effective air mass.

                 This function calculates the effective air mass using the specified
                 approximation method. The air mass is computed from the astronomical
                 coordinates (RA, Dec), local sidereal time, exposure time, and
                 observatory latitude.

                 Returns
                 -------
                 tuple
                     A tuple containing (airmass_data, airmass_error) where:
                     - airmass_data: The computed air mass value
                     - airmass_error: The error in the air mass calculation

                 Notes
                 -----
                 The function uses different approximation methods:
                 - Hardie (1962): Most commonly used method
                 - Young & Irvine (1967): Alternative approximation
                 - Young (1994): More recent approximation

                 The calculation takes into account the Earth's atmospheric refraction
                 and the geometric path length through the atmosphere.

                 Raises
                 ------
                 hdrlcore.IllegalInputError
                     If any input parameter is invalid (e.g., RA outside [0, 360],
                     Dec outside [-90, 90], latitude outside [-90, 90], etc.).
                 )docstring")
      .def_property_readonly("ra", &hdrl::func::Airmass::get_ra)
      .def_property_readonly("dec", &hdrl::func::Airmass::get_dec)
      .def_property_readonly("lst", &hdrl::func::Airmass::get_lst)
      .def_property_readonly("exptime", &hdrl::func::Airmass::get_exptime)
      .def_property_readonly("latitude", &hdrl::func::Airmass::get_latitude)
      .def_property_readonly("type", &hdrl::func::Airmass::get_type);
}
