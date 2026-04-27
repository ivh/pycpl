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

#include "hdrlfunc/fpn_bindings.hpp"
#include <pybind11/pybind11.h>

#include <string>

#include "hdrlcore/pycpl_image.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_mask.hpp"   // IWYU pragma: keep
#include "hdrlfunc/fpn.hpp"

namespace py = pybind11;

void
bind_fpn(py::module& m)
{
  // Define the Fpn::Result struct for Python
  py::class_<hdrl::func::Fpn::Result>(m, "FpnResult")
      .def_readonly("power_spectrum", &hdrl::func::Fpn::Result::power_spectrum)
      .def_readonly("std", &hdrl::func::Fpn::Result::std)
      .def_readonly("std_mad", &hdrl::func::Fpn::Result::std_mad)
      .def("__repr__", [](const hdrl::func::Fpn::Result& self) {
        return "FpnResult(power_spectrum=<cpl.core.Image>, std=" +
               std::to_string(self.std) +
               ", std_mad=" + std::to_string(self.std_mad) + ")";
      });

  // Define the main function
  m.def("fpn_compute", &hdrl::func::Fpn::compute, py::arg("image"),
        py::arg("mask") = py::none(), py::arg("dc_mask_x") = 1,
        py::arg("dc_mask_y") = 1,
        R"docstring(
        Compute fixed pattern noise on a single image.
        
        This function detects fixed pattern noise on the input image. The algorithm
        first computes the power spectrum of the image using the Fast Fourier Transform (FFT),
        then computes the standard deviation and MAD-based standard deviation of the
        power spectrum excluding the masked region.
        
        Parameters
        ----------
        image : cpl.core.Image
            Input image (bad pixels are not allowed).
        mask : cpl.core.Mask, optional
            Optional input mask applied to the power spectrum. If None, no mask is used.
        dc_mask_x : int, optional
            X-pixel window (>= 1) to discard DC component starting from pixel (1, 1).
            Default is 1.
        dc_mask_y : int, optional
            Y-pixel window (>= 1) to discard DC component starting from pixel (1, 1).
            Default is 1.
        
        Returns
        -------
        FpnResult
            A result object containing:
            - power_spectrum: cpl.core.Image with the computed power spectrum
            - std: float, the standard deviation of the power spectrum
            - std_mad: float, the MAD-based standard deviation of the power spectrum
        
        Notes
        -----
        The power spectrum contains the DC component (the DC term is the 0 Hz
        term and is equivalent to the average of all the samples in the window)
        in pixel (1,1).
        
        The mask created on the fly by setting dc_mask_x and dc_mask_y and the
        optional mask are combined and are both taken into account when calculating
        std and std_mad.
        
        The final mask used to derive std and std_mad is attached to the
        power_spectrum image as a normal cpl mask and can be retrieved using
        power_spectrum.bpm.
        
        Raises
        ------
        hdrlcore.NullInputError
            If image is None.
        hdrlcore.IllegalInputError
            If dc_mask_x < 1 or dc_mask_y < 1, or if image contains bad pixels.
        hdrlcore.IncompatibleInputError
            If mask is not None and its size doesn't match the image size.
        )docstring");
}
