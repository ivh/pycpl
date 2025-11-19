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

#include "hdrl/hdrl_image_bindings.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "hdrl/hdrl_image.hpp"
#include "hdrl/hdrl_types.hpp"

namespace py = pybind11;

void bind_hdrl_image(py::module& m)
{
  // Bind hdrl::Value
  py::class_<cpl::hdrl::Value>(m, "Value", R"pydoc(
    A value with associated error.

    This structure holds a data value and its associated error, which is the
    fundamental type used throughout HDRL for representing measurements with
    uncertainties.
    )pydoc")
      .def(py::init<>(),
           "Create a Value with data=0.0 and error=0.0")
      .def(py::init<double, double>(),
           py::arg("data"), py::arg("error"),
           "Create a Value with specified data and error")
      .def_readwrite("data", &cpl::hdrl::Value::data,
                    "The data value")
      .def_readwrite("error", &cpl::hdrl::Value::error,
                    "The error (uncertainty) associated with the data")
      .def("__repr__",
           [](const cpl::hdrl::Value& v) {
             return "<hdrl.Value data=" + std::to_string(v.data) +
                    " error=" + std::to_string(v.error) + ">";
           });

  // Bind hdrl::Image
  py::class_<cpl::hdrl::Image, std::shared_ptr<cpl::hdrl::Image>>(
      m, "Image", R"pydoc(
    HDRL Image - an image with associated error and bad pixel mask.

    An HDRL image consists of three components:
    - A data image (cpl.core.Image)
    - An error image (cpl.core.Image)
    - A bad pixel mask (cpl.core.Mask)

    This class provides the fundamental data structure for high-level data
    reduction operations that require error propagation and bad pixel tracking.
    )pydoc")
      .def(py::init<cpl::core::size, cpl::core::size>(),
           py::arg("nx"), py::arg("ny"),
           R"pydoc(
           Create a new HDRL image with given dimensions.

           Parameters
           ----------
           nx : int
               Width in pixels
           ny : int
               Height in pixels
           )pydoc")
      .def(py::init<std::shared_ptr<cpl::core::ImageBase>,
                    std::shared_ptr<cpl::core::ImageBase>>(),
           py::arg("data"), py::arg("error") = nullptr,
           R"pydoc(
           Create an HDRL image from a CPL image and optional error image.

           Parameters
           ----------
           data : cpl.core.Image
               The data image
           error : cpl.core.Image, optional
               The error image. If not provided, will be created automatically.
           )pydoc")
      .def(py::init<const cpl::hdrl::Image&>(),
           py::arg("other"),
           "Create a copy of an existing HDRL image")
      .def("get_image",
           static_cast<std::shared_ptr<cpl::core::ImageBase> (cpl::hdrl::Image::*)()>(&cpl::hdrl::Image::get_image),
           "Get the data image")
      .def("get_error",
           static_cast<std::shared_ptr<cpl::core::ImageBase> (cpl::hdrl::Image::*)()>(&cpl::hdrl::Image::get_error),
           "Get the error image")
      .def("get_mask",
           static_cast<std::shared_ptr<cpl::core::Mask> (cpl::hdrl::Image::*)()>(&cpl::hdrl::Image::get_mask),
           "Get the bad pixel mask")
      .def("get_pixel", &cpl::hdrl::Image::get_pixel,
           py::arg("xpos"), py::arg("ypos"),
           R"pydoc(
           Get a pixel value with error at position (x, y).

           Parameters
           ----------
           xpos : int
               X position (1-indexed)
           ypos : int
               Y position (1-indexed)

           Returns
           -------
           tuple
               A tuple (value, is_rejected) where value is an hdrl.Value
               and is_rejected is a boolean indicating if the pixel is bad.
           )pydoc")
      .def("set_pixel", &cpl::hdrl::Image::set_pixel,
           py::arg("xpos"), py::arg("ypos"), py::arg("value"),
           R"pydoc(
           Set a pixel value with error at position (x, y).

           Parameters
           ----------
           xpos : int
               X position (1-indexed)
           ypos : int
               Y position (1-indexed)
           value : hdrl.Value
               Value with data and error to set
           )pydoc")
      .def("get_size_x", &cpl::hdrl::Image::get_size_x,
           "Get the width of the image in pixels")
      .def("get_size_y", &cpl::hdrl::Image::get_size_y,
           "Get the height of the image in pixels")
      .def("extract", &cpl::hdrl::Image::extract,
           py::arg("llx"), py::arg("lly"), py::arg("urx"), py::arg("ury"),
           R"pydoc(
           Extract a sub-region from the image.

           Parameters
           ----------
           llx : int
               Lower-left x coordinate (1-indexed)
           lly : int
               Lower-left y coordinate (1-indexed)
           urx : int
               Upper-right x coordinate (1-indexed)
           ury : int
               Upper-right y coordinate (1-indexed)

           Returns
           -------
           hdrl.Image
               The extracted sub-image
           )pydoc")
      .def("reject", &cpl::hdrl::Image::reject,
           py::arg("xpos"), py::arg("ypos"),
           "Mark a pixel as bad (rejected)")
      .def("reject_from_mask", &cpl::hdrl::Image::reject_from_mask,
           py::arg("mask"),
           "Mark pixels as bad based on a mask")
      .def("is_rejected", &cpl::hdrl::Image::is_rejected,
           py::arg("xpos"), py::arg("ypos"),
           "Check if a pixel is marked as bad")
      .def("count_rejected", &cpl::hdrl::Image::count_rejected,
           "Count the number of rejected (bad) pixels")
      .def("accept", &cpl::hdrl::Image::accept,
           py::arg("xpos"), py::arg("ypos"),
           "Mark a pixel as good (not rejected)")
      .def("accept_all", &cpl::hdrl::Image::accept_all,
           "Mark all pixels as good (clear bad pixel mask)")
      .def("__repr__",
           [](const cpl::hdrl::Image& img) {
             return "<hdrl.Image " + std::to_string(img.get_size_x()) + "x" +
                    std::to_string(img.get_size_y()) + ">";
           });
}
