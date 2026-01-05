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

#include "hdrlfunc/overscan_bindings.hpp"

#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "hdrlcore/pycpl_types.hpp"
#include "hdrlcore/pycpl_window.hpp"  // IWYU pragma: keep
#include "hdrlfunc/overscan.hpp"

namespace py = pybind11;

using hdrl::core::Image;
using hdrl::func::Overscan;

void
bind_overscan(py::module& m)
{
  py::class_<hdrl::core::pycpl_window>(m, "Window")
      .def(py::init<>())
      .def_readwrite("llx", &hdrl::core::pycpl_window::llx)
      .def_readwrite("lly", &hdrl::core::pycpl_window::lly)
      .def_readwrite("urx", &hdrl::core::pycpl_window::urx)
      .def_readwrite("ury", &hdrl::core::pycpl_window::ury)
      .def("__repr__", [](const hdrl::core::pycpl_window& w) {
        return "<Window llx=" + std::to_string(w.llx) +
               ", lly=" + std::to_string(w.lly) +
               ", urx=" + std::to_string(w.urx) +
               ", ury=" + std::to_string(w.ury) + ">";
      });

  py::class_<Overscan, std::shared_ptr<Overscan>> overscan_class(
      m, "Overscan", py::buffer_protocol());

  overscan_class
      .def(py::init<const std::string&, double, int, hdrl::func::Collapse,
                    const hdrl::core::pycpl_window&>(),
           py::arg("direction"), py::arg("ccd_ron"), py::arg("box_hsize"),
           py::arg("collapse"), py::arg("region"))

      // Argument image is a std::shared_ptr<hdrl::core::Image>
      .def("compute", &Overscan::compute, py::arg("image"),
           R"pbdoc(
                 Compute the overscan correction model from an HDRL Image.

                 Parameters:
                     input_image (Image): The image to compute the overscan model from.

                 Returns:
                     tuple OverscanResult with:
                         - correction
                         - contribution
                         - chi2
                         - red_chi2
                         - sigclip_reject_low
                         - sigclip_reject_high
                         - minmax_reject_low
                         - minmax_reject_high
            )pbdoc")

      .def(
          "correct",
          [](Overscan& self, std::shared_ptr<Image> input_image,
             std::optional<hdrl::core::pycpl_window> region) {
            return self.correct(input_image, region);
          },
          py::arg("input_image"), py::arg("region") = std::nullopt,
          R"pbdoc(
                  Apply overscan correction to an HDRL Image.

                  Parameters:
                         input_image (Image): Image to correct.
                         region (tuple[int, int, int, int], optional): Region to apply correction.

                  Returns:
                        namedtuple OverscanCorrectResult with:
                        - corrected: corrected image (Image)
                        - badmask: corresponding bad pixel mask (pycpl_image)
            )pbdoc")

      .def_property_readonly("direction", &Overscan::get_direction)
      .def_property_readonly("ccd_ron", &Overscan::get_ccd_ron)
      .def_property_readonly("box_hsize", &Overscan::get_box_hsize)
      .def_property_readonly("overscan_region", &Overscan::get_overscan_region,
                             "Returns the overscan region as a Window object.");
}
