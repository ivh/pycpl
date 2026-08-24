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

#include "hdrlfunc/maglim_bindings.hpp"

#include <memory>
#include <utility>

#include <hdrl_utils.h>

#include "hdrlcore/parameter.hpp"
#include "hdrlcore/pycpl_image.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_mask.hpp"   // IWYU pragma: keep
#include "hdrlfunc/collapse.hpp"
#include "hdrlfunc/maglim.hpp"

namespace py = pybind11;

void
bind_maglim(py::module& m)
{
  // Expose the Maglim class
  auto maglim_class =
      py::class_<hdrl::func::Maglim, std::shared_ptr<hdrl::func::Maglim>>(
          m, "Maglim", py::buffer_protocol());

  maglim_class.doc() = R"docstring(
        A hdrl.func.Maglim is a helper class for computing limiting magnitudes.
        
        Parameters
        ----------
        zeropoint : float
            Zeropoint for magnitude calculation.
        fwhm : float
            Full width at half maximum for the PSF.
        kernel_size_x : int
            Kernel size in x direction.
        kernel_size_y : int
            Kernel size in y direction.
        extend_method : hdrl.func.Maglim.ImageExtendMethod
            Image extension method (Nearest or Mirror).
        mode_param : hdrl.func.Collapse or hdrl.core.Parameter
            Mode parameter for the computation.
    )docstring";

  // Bind the enum to the Maglim class
  py::enum_<hdrl_image_extend_method>(
      maglim_class, "ImageExtendMethod",
      "Image extension method for maglim computation.")
      .value("Nearest", HDRL_IMAGE_EXTEND_NEAREST,
             "Extend using nearest value.")
      .value("Mirror", HDRL_IMAGE_EXTEND_MIRROR, "Extend using mirror value.");

  maglim_class
      .def(
          py::init([](double zeropoint, double fwhm, int kernel_size_x,
                      int kernel_size_y, hdrl_image_extend_method extend_method,
                      py::object mode_param) {
            // Handle both Collapse objects and Parameter objects
            std::shared_ptr<hdrl::core::Parameter> param_ptr;

            if (mode_param.is_none()) {
              param_ptr = nullptr;
            } else if (py::isinstance<hdrl::func::Collapse>(mode_param)) {
              // Borrow from Collapse: Collapse owns the hdrl_parameter;
              // stealing into Parameter would double-free when several Maglim
              // instances share one Collapse or when Collapse outlives
              // temporary Maglims.
              hdrl::func::Collapse& collapse =
                  mode_param.cast<hdrl::func::Collapse&>();
              hdrl_parameter* raw = collapse.ptr();
              auto borrowed = std::shared_ptr<hdrl_parameter>(
                  raw, [](hdrl_parameter*) noexcept {});
              param_ptr =
                  std::make_shared<hdrl::core::Parameter>(std::move(borrowed));
            } else if (py::isinstance<hdrl::core::Parameter>(mode_param)) {
              param_ptr =
                  mode_param.cast<std::shared_ptr<hdrl::core::Parameter>>();
            } else {
              throw py::type_error(
                  "mode_param must be a Collapse object, Parameter object, or "
                  "None");
            }

            return new hdrl::func::Maglim(zeropoint, fwhm, kernel_size_x,
                                          kernel_size_y, extend_method,
                                          param_ptr);
          }),
          py::arg("zeropoint"), py::arg("fwhm"), py::arg("kernel_size_x"),
          py::arg("kernel_size_y"), py::arg("extend_method"),
          py::arg("mode_param"),
          R"docstring(
                 Create a Maglim computation object.
                 
                 Parameters
                 ----------
                 zeropoint : float
                     Zeropoint for magnitude calculation.
                 fwhm : float
                     Full width at half maximum for the PSF.
                 kernel_size_x : int
                     Kernel size in x direction.
                 kernel_size_y : int
                     Kernel size in y direction.
                 extend_method : hdrl.func.Maglim.ImageExtendMethod
                     Image extension method (Nearest or Mirror).
                 mode_param : hdrl.func.Collapse or hdrl.core.Parameter or None
                     Mode parameter for the computation.
             )docstring")
      .def("compute", &hdrl::func::Maglim::compute, py::arg("image"),
           R"docstring(
                 Compute the limiting magnitude for the given image.
                 
                 Parameters
                 ----------
                 image : hdrl.core.Image
                     The input image.
                 
                 Returns
                 -------
                 float
                     The computed limiting magnitude.
             )docstring")
      .def_property_readonly("zeropoint", &hdrl::func::Maglim::get_zeropoint,
                             "Zeropoint for magnitude calculation.")
      .def_property_readonly("fwhm", &hdrl::func::Maglim::get_fwhm,
                             "Full width at half maximum for the PSF.")
      .def_property_readonly("kernel_size_x",
                             &hdrl::func::Maglim::get_kernel_size_x,
                             "Kernel size in x direction.")
      .def_property_readonly("kernel_size_y",
                             &hdrl::func::Maglim::get_kernel_size_y,
                             "Kernel size in y direction.")
      .def_property_readonly("extend_method",
                             &hdrl::func::Maglim::get_extend_method,
                             "Image extension method.")
      .def_property_readonly("mode_param", &hdrl::func::Maglim::get_mode_param,
                             "Mode parameter for the computation.");
}