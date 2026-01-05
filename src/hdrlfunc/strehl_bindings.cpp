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

#include "hdrlfunc/strehl_bindings.hpp"

#include <memory>


#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include "hdrlcore/image.hpp"
#include "hdrlfunc/strehl.hpp"

namespace py = pybind11;

using hdrl::func::Strehl;

void
bind_strehl(py::module& m)
{
  auto strehlresult_class =
      py::class_<hdrl::func::StrehlResult,
                 std::shared_ptr<hdrl::func::StrehlResult>>(
          m, "StrehlResult", py::buffer_protocol());

  py::class_<Strehl, std::shared_ptr<Strehl>>(m, "Strehl")
      .def(py::init<double, double, double, double, double, double, double,
                    double>(),
           py::arg("wavelength"), py::arg("m1"), py::arg("m2"),
           py::arg("pixel_scale_x"), py::arg("pixel_scale_y"),
           py::arg("flux_radius"), py::arg("bkg_radius_low"),
           py::arg("bkg_radius_high"),
           R"docstring(
            Initialize a Strehl object with its 8 scalar parameters.
            )docstring")
      .def(
          "compute",
          [](hdrl::func::Strehl& self,
             std::shared_ptr<hdrl::core::Image> himage) {
            return self.compute(himage);
          },
          py::arg("himage"),
          R"docstring(
      The raw image is assumed to be pre-processed to remove the instrument
      signatures (bad pixels, etc.) and the natural noise sources (sky background,
      etc.). Nethertheless this function allows also the user to correct a residual
      background by setting the parameters bkg_radius_low, bkg_radius_high.
      The PSF is identified and its integrated flux (controlled by the parameter
      flux_radius) is normalized to 1. The
      PSF baricenter is computed and used to generate the ideal PSF (with
      integrated flux normalized to 1) which takes into account the telescope pupil
      characteristics (radius m1, central obstruction, m2, ...), the
      wavelength wavelength, at which the image has been obtained and the
      related pixel scale (pixel_scale_x, pixel_scale_y,). Finally the Strehl
      ratio is computed dividing the maximum intensity of the image PSF by the
      maximum intensity of the ideal PSF and the associated error is also computed.

      Parameters
      ----------
      himage : The image to process

      Returns
      -------
      strehl_result: The Strehl value object

      )docstring")
      .def_property_readonly("wavelength", &Strehl::get_wavelength,
                             "Wavelength")
      .def_property_readonly("m1", &Strehl::get_m1, "M1")
      .def_property_readonly("m2", &Strehl::get_m2, "M2")
      .def_property_readonly("pixel_scale_x", &Strehl::get_pixel_scale_x,
                             "Pixel scale x")
      .def_property_readonly("pixel_scale_y", &Strehl::get_pixel_scale_y,
                             "Pixel scale y")
      .def_property_readonly("flux_radius", &Strehl::get_flux_radius,
                             "Flux radius")
      .def_property_readonly("bkg_radius_low", &Strehl::get_bkg_radius_low,
                             "Background radius low")
      .def_property_readonly("bkg_radius_high", &Strehl::get_bkg_radius_high,
                             "Background radius high");

  strehlresult_class.doc() = R"docstring(
      A hdrl.func.StrehlResult class is a container for the results of hdrl.func.Strehl.compute().
      The results consist of ... a number of values.

      These can be accessed via ... getters.

      Example
      -------
      .. code-block:: python

        result = hdrl.func.Strehl.compute(...)
      )docstring";

  strehlresult_class
      .def_property_readonly("strehl_value",
                             &hdrl::func::StrehlResult::get_strehl_value,
                             "strehl_value")
      .def_property_readonly("strehl_error",
                             &hdrl::func::StrehlResult::get_strehl_error,
                             "strehl_error")
      .def_property_readonly("star_x", &hdrl::func::StrehlResult::get_star_x,
                             "star_x")
      .def_property_readonly("star_y", &hdrl::func::StrehlResult::get_star_y,
                             "star_y")
      .def_property_readonly(
          "star_peak", &hdrl::func::StrehlResult::get_star_peak, "star_peak")
      .def_property_readonly("star_peak_error",
                             &hdrl::func::StrehlResult::get_star_peak_error,
                             "star_peak_error")
      .def_property_readonly(
          "star_flux", &hdrl::func::StrehlResult::get_star_flux, "star_flux")
      .def_property_readonly("star_flux_error",
                             &hdrl::func::StrehlResult::get_star_flux_error,
                             "star_flux_error")
      .def_property_readonly("star_background",
                             &hdrl::func::StrehlResult::get_star_background,
                             "star_background")
      .def_property_readonly(
          "star_background_error",
          &hdrl::func::StrehlResult::get_star_background_error,
          "star_background_error")
      .def_property_readonly(
          "computed_background_error",
          &hdrl::func::StrehlResult::get_computed_background_error,
          "computed_background_error")
      .def_property_readonly("nbackground_pixels",
                             &hdrl::func::StrehlResult::get_nbackground_pixels,
                             "nbackground_pixels");
}
