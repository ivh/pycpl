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

#include "hdrlfunc/dar_bindings.hpp"

#include <pybind11/stl.h>

#include "hdrlcore/pycpl_vector.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_wcs.hpp"     // IWYU pragma: keep
#include "hdrlcore/value.hpp"         // IWYU pragma: keep
#include "hdrlfunc/dar.hpp"

namespace py = pybind11;

void
bind_dar(py::module& m)
{
  // Define the Dar::Result struct for Python
  py::class_<hdrl::func::Dar::Result>(m, "DarResult")
      .def_readonly("xShift", &hdrl::func::Dar::Result::xShift)
      .def_readonly("yShift", &hdrl::func::Dar::Result::yShift)
      .def_readonly("xShiftErr", &hdrl::func::Dar::Result::xShiftErr)
      .def_readonly("yShiftErr", &hdrl::func::Dar::Result::yShiftErr)
      .def("__repr__", [](const hdrl::func::Dar::Result&) {
        return "DarResult(xShift=<cpl.core.Vector>, yShift=<cpl.core.Vector>, "
               "xShiftErr=<cpl.core.Vector>, yShiftErr=<cpl.core.Vector>)";
      });

  // Define the Dar class
  py::class_<hdrl::func::Dar>(m, "Dar")
      .def(py::init<hdrl::core::Value, hdrl::core::Value, hdrl::core::Value,
                    hdrl::core::Value, hdrl::core::Value, hdrl::core::Value,
                    hdrl::core::pycpl_wcs>(),
           py::arg("airmass"), py::arg("parang"), py::arg("posang"),
           py::arg("temp"), py::arg("rhum"), py::arg("pres"), py::arg("wcs"),
           R"docstring(
                 Create a DAR (Differential Atmospheric Refraction) computation object.
                 
                 Parameters
                 ----------
                 airmass : tuple or None
                     Air mass value with error (data, error) or None for (0, 0).
                 parang : tuple or None
                     Parallactic angle during exposure in degrees with error (data, error) or None for (0, 0).
                 posang : tuple or None
                     Position angle on the sky in degrees with error (data, error) or None for (0, 0).
                 temp : tuple or None
                     Temperature in Celsius with error (data, error) or None for (0, 0).
                 rhum : tuple or None
                     Relative humidity in percent with error (data, error) or None for (0, 0).
                 pres : tuple or None
                     Pressure in mbar with error (data, error) or None for (0, 0).
                 wcs : cpl.core.WCS or None
                     World Coordinate system (WCS) in degrees or None.
             )docstring")
      .def("compute", &hdrl::func::Dar::compute, py::arg("lambdaRef"),
           py::arg("lambdaIn"),
           R"docstring(
                 Compute differential atmospheric refraction corrections.
                 
                 This function corrects the pixel coordinates of all pixels of a given pixel table
                 for differential atmospheric refraction (DAR). The algorithm computes the DAR offset
                 for the wavelength difference with respect to the reference wavelength, and stores
                 the shift in the coordinates, taking into account the instrument rotation angle on
                 the sky and the parallactic angle at the time of the observations.
                 
                 Parameters
                 ----------
                 lambdaRef : tuple or None
                     Reference wavelength in Angstroms with error (data, error) or None for (0, 0).
                 lambdaIn : cpl.core.Vector
                     One lambda for each plane (in Angstroms).
                 
                 Returns
                 -------
                 DarResult
                     A result object containing:
                     - xShift: cpl.core.Vector, correction for each plane in x-axis (pixels)
                     - yShift: cpl.core.Vector, correction for each plane in y-axis (pixels)
                     - xShiftErr: cpl.core.Vector, error in correction for each plane in x-axis (pixels)
                     - yShiftErr: cpl.core.Vector, error in correction for each plane in y-axis (pixels)
                 
                 Notes
                 -----
                 The resulting correction can be directly applied to the pixel table.
                 
                 The algorithm is based on Filippenko (1982, PASP, 94, 715) and uses the formula
                 from Owens which converts relative humidity to water vapor pressure.
                 
                 The calculation is performed by calling the top-level function hdrl_dar_compute()
                 and the parameters passed to this function can be created by calling the constructor.
                 
                 Raises
                 ------
                 hdrlcore.NullInputError
                     If any required parameter is None.
                 hdrlcore.IllegalInputError
                     If parameter validation fails (e.g., invalid parameter values).
                 hdrlcore.IncompatibleInputError
                     If inputs are incompatible.
                 )docstring")
      .def_property_readonly("airmass", &hdrl::func::Dar::get_airmass)
      .def_property_readonly("parang", &hdrl::func::Dar::get_parang)
      .def_property_readonly("posang", &hdrl::func::Dar::get_posang)
      .def_property_readonly("temp", &hdrl::func::Dar::get_temp)
      .def_property_readonly("rhum", &hdrl::func::Dar::get_rhum)
      .def_property_readonly("pres", &hdrl::func::Dar::get_pres)
      .def_property_readonly("wcs", &hdrl::func::Dar::get_wcs);
}