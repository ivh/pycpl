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

#include "hdrlcore/image_bindings.hpp"

#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "dump_handler.hpp"
#include "hdrlcore/image.hpp"
#include "hdrlcore/pycpl_image.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_mask.hpp"   // IWYU pragma: keep
#include "hdrlcore/pycpl_types.hpp"
#include "hdrlcore/pycpl_window.hpp"  // IWYU pragma: keep
#include "hdrlcore/value.hpp"         // IWYU pragma: keep
#include "path_conversion.hpp"        // IWYU pragma: keep


namespace py = pybind11;

void
bind_hdrl_image(py::module& m)
{
  py::class_<hdrl::core::Image, std::shared_ptr<hdrl::core::Image>> image_class(
      m, "Image", py::buffer_protocol());

  image_class.doc() = R"docstring(
      A hdrl.core.Image is a HDRL image (a two-dimensional array
      object) containing data and its associated errors. It provides a similar
      API to cpl.core.Image and performs linear error propagation where it makes sense.

      The pixel indexing follows 0-indexing with the lower left corner having index (0, 0). The pixel
      buffer is stored row-wise so for optimum performance any pixel-wise access should be done likewise.

      The pixel ordering is of the order (y, x) to be consistent  with PyCPL

      Parameters
      ----------
      data : cpl.core.Image
            Image with data values.  
      error : cpl.core.Image
            Image with error values.

      Notes
      -----
      A new empty hdrl.core.Image of width x height dimension can be created using hdrl.core.Image.zeros

      See Also
      --------
      hdrl.core.Image.zeros : Create a new zeros filled hdrl.core.Image of width x height dimensions.

      )docstring";

  image_class
      // we may want to remove the <int,int> constructor here and just have the
      // zeros one like in PyCPL .def(py::init<int,int>())
      .def(py::init<hdrl::core::pycpl_image, hdrl::core::pycpl_image>())
      .def_property_readonly("width", &hdrl::core::Image::get_width,
                             "int: Width of the image")
      .def_property_readonly("height", &hdrl::core::Image::get_height,
                             "int: Height of the image")
      .def_property_readonly("image", &hdrl::core::Image::get_image,
                             "cpl.core.Image: The primary image")
      .def_property_readonly("error", &hdrl::core::Image::get_error,
                             "cpl.core.Image: the error image")
      .def_property_readonly("mask", &hdrl::core::Image::get_mask,
                             "cpl.core.Mask: The image mask")
      .def_property_readonly(
          "size", &hdrl::core::Image::get_size,
          "int : Total number of pixels in the image (width*height)")

      .def(
          "extract",
          [](hdrl::core::Image& self, hdrl::core::pycpl_window window) {
            return self.extract(window);
          },
          py::arg("window") = py::none()  //.none(true) = py::none())
          ,
          R"pydoc(
      Dump the image contents to a file, stdout or a string.
      Extract copy of window from hdrl.core.Image

      Parameters
      ----------
      window : tuple(int,int,int,int), optional
              Window to dump with `value` in the format (llx, lly, urx, ury) where:
              - `llx` Lower left X coordinate
              - `lly` Lower left Y coordinate
              - `urx` Upper right X coordinate 
              - `ury` Upper right Y coordinate
              Defaults to None (no window).

      Returns
      -------
      hdrl.core.Image
          A newly allocated hdrl.core.Image containing the window.
          )pydoc")
      .def("copy_into", &hdrl::core::Image::copy_into, py::arg("other"),
           py::arg("ypos"), py::arg("xpos"), R"pydoc(
      Copy one hdrl.core.Image into another

      The two input images must be of the same type, namely one of
      cpl.core.Type.INT, cpl.core.Type.FLOAT, cpl.core.Type.DOUBLE.

      Parameters
      ----------
        other : hdrl.core.Image
            The inserted image.
        ypos : int
            the y pixel position in `self` where the lower left pixel of
            `other` should go (from 0 to the y-1 size of `self`)
        xpos : int
            the x pixel position in `self` where the lower left pixel of
            `other` should go (from 0 to the x-1 size of `self`)
      
      See Also
      --------
      hdrl.core.Image.insert_into : Copy cpl.core.Image into an hdrl.core.Image
        )pydoc")

      .def("insert_into", &hdrl::core::Image::insert_into, py::arg("image"),
           py::arg("error").none(true), py::arg("ypos"), py::arg("xpos"),
           R"pydoc(
      Copy cpl.core.Image into an hdrl.core.Image

      Parameters
      ----------
      image :      cpl.core.Image
            the inserted image
      error :      cpl.core.Image
            the inserted error, may be NULL
      ypos : int
            the y pixel position in image 1 where the lower left pixel of
            image 2 should go (from 1 to the y size of image 1)
      xpos : int
            the x pixel position in image 1 where the lower left pixel of
            image 2 should go (from 1 to the x size of image 1)

      See Also
      --------
      hdrl.core.Image.copy_into : Copy one hdrl.core.Image into another
       )pydoc")

      .def(
          "get_pixel",
          [](hdrl::core::Image& self, cpl_size ypos, cpl_size xpos) {
            return self.get_pixel(ypos, xpos);
          },
          py::arg("ypos"), py::arg("xpos"),
          R"pydoc(
      Gets pixel values of hdrl.core.Image

      Parameters
      ----------
      ypos: int
                y coordinate
      xpos: int
                x coordinate

      Returns
      -------
      namedtuple
         The namedtuple Value contains two doubles:
         If desired, the namedtuple can be converted to a dictionary using its ._asdict() method.
         It returns the pixel data value (data) and its error (error).

      See Also
      --------
      hdrl.core.Image.set_pixel : Sets pixel values of hdrl.core.Image
       )pydoc")

      .def(
          "set_pixel",
          [](hdrl::core::Image& self, cpl_size ypos, cpl_size xpos,
             hdrl::core::Value v) { self.set_pixel(ypos, xpos, v); },
          py::arg("ypos"), py::arg("xpos"), py::arg("value"),
          R"pydoc(
      Sets pixel values of hdrl.core.Image

      Parameters
      ----------
      ypos: int
                y coordinate
      xpos: int
                x coordinate
      value: tuple(float, float)
            Data value to set. The first component is the data value, the second is the error value.

      See Also
      --------
      hdrl.core.Image.get_pixel : Gets pixel values of hdrl.core.Image
       )pydoc")

      .def(
          "__getitem__",
          [](hdrl::core::Image& self, cpl_size ypos, cpl_size xpos) {
            if (ypos > self.get_height() || ypos < 0) {
              throw py::index_error("Image height index out of range");
            }
            if (xpos > self.get_width() || xpos < 0) {
              throw py::index_error("Image width index out of range");
            }
            return self.get_pixel(ypos, xpos);
          },
          py::arg("ypos"), py::arg("xpos"))
      .def(
          "__setitem__",
          [](hdrl::core::Image& self, cpl_size ypos, cpl_size xpos,
             hdrl::core::Value v) {
            if (ypos > self.get_height() || ypos < 0) {
              throw py::index_error("Image index out of range");
            }
            if (xpos > self.get_width() || xpos < 0) {
              throw py::index_error("Image width index out of range");
            }
            return self.set_pixel(ypos, xpos, v);
          },
          py::arg("ypos"), py::arg("xpos"), py::arg("value"))
      .def("__deepcopy__",
           [](hdrl::core::Image& self, py::dict /* unused */) {
             return self.duplicate();
           })
      // .def("__eq__", [](hdrl::core::Image& self, hdrl::core::Image other)
      // TODO
      // {
      //       // return (self->get_data() == other->get_data() &&
      //       self->get_error() == other->get_error());
      //       // if ( self.error == ) {
      //       //       return
      //       // }
      //       // return self == other;
      // }, py::arg("other")
      // )

      .def(
          "div_image_create",
          [](hdrl::core::Image& self,
             std::shared_ptr<hdrl::core::Image> other) {
            return self.div_image_create(other);
          },
          py::arg("other"),
          R"pydoc(
      Divide two images and return the resulting image.

      Parameters
      ----------
      other : hdrl.core.Image
            Image that `self` is divided by.

      Returns
      -------
      hdrl.core.Image
            A newly allocated image.

      See Also
      --------
      hdrl.core.Image.div_image : Divides self Image values by other Image values. Modified in place.
      hdrl.core.Image.div_scalar : Elementwise division of an image with a scalar. Modified in place.

       )pydoc")

      .def(
          "div_image",
          [](hdrl::core::Image& self,
             std::shared_ptr<hdrl::core::Image> other) {
            self.div_image(other);
          },
          py::arg("other"),
          R"pydoc(
      Divides self Image values by other Image values. Modified in place.

      Parameters
      ----------
      other : hdrl.core.Image
            Image that `self` is divided by.

      See Also
      --------
      hdrl.core.Image.div_scalar : Elementwise division of an image with a scalar. Modified in place.
      hdrl.core.Image.div_image_create : Divide two images and return the resulting image.
      
       )pydoc")

      .def(
          "div_scalar",
          [](hdrl::core::Image& self, hdrl::core::Value v) {
            self.div_scalar(v);
          },
          py::arg("value"),
          R"pydoc(
      Elementwise division of an image with a scalar. Modified in place.

      Parameters
      ----------
      value : tuple(float, float)
            Non-zero number to divide with. The first component is the data value, the second is the error value.

      See Also
      --------
      hdrl.core.Image.div_image : Divides self Image values by other Image values. Modified in place.
      hdrl.core.Image.div_image_create : Divide two images and return the resulting image.
       
       )pydoc")

      .def(
          "mul_image_create",
          [](hdrl::core::Image& self,
             std::shared_ptr<hdrl::core::Image> other) {
            return self.mul_image_create(other);
          },
          py::arg("other"),
          R"pydoc(
      Multiply two images and return the resulting image.

      Parameters
      ----------
      other : hdrl.core.Image
            Image that `self` is multiplied by.

      Returns
      -------
      hdrl.core.Image
            A newly allocated image.

      See Also
      --------
      hdrl.core.Image.mul_image :  Multiplies self Image values by other Image values. Modified in place.
      hdrl.core.Image.mul_scalar : Elementwise multiplication of an image by a scalar. Modified in place.
       )pydoc")

      .def(
          "mul_image",
          [](hdrl::core::Image& self,
             std::shared_ptr<hdrl::core::Image> other) {
            self.mul_image(other);
          },
          py::arg("other"),
          R"pydoc(
      Multiplies self Image values by other Image values. Modified in place.
      
      Parameters
      ----------
      other : hdrl.core.Image
            Image that `self` is multiplied by.
      See Also
      --------
      hdrl.core.Image.mul_image_create :  Multiply two images and return the resulting image.
      hdrl.core.Image.mul_scalar : Elementwise multiplication of an image by a scalar. Modified in place.
      
       )pydoc")

      .def(
          "mul_scalar",
          [](hdrl::core::Image& self, hdrl::core::Value v) {
            self.mul_scalar(v);
          },
          py::arg("value"),
          R"pydoc(
      Elementwise multiplication of an image by a scalar. Modified in place.

      Parameters
      ----------
      value : tuple(float, float)
            Non-zero number to multiply with. The first component is the data value, the second is the error value.

      See Also
      --------
      hdrl.core.Image.mul_image_create :  Multiply two images and return the resulting image.
      hdrl.core.Image.mul_image : Multiplies self Image values by other Image values. Modified in place.
       )pydoc")

      .def(
          "sub_image_create",
          [](hdrl::core::Image& self,
             std::shared_ptr<hdrl::core::Image> other) {
            return self.sub_image_create(other);
          },
          py::arg("other"),
          R"pydoc(
      Subtract two images and return the resulting image.

      Parameters
      ----------
      other : hdrl.core.Image
            Image subtracted from self.
      Returns
      -------
      hdrl.core.Image
            A newly allocated image.

      See Also
      --------
      hdrl.core.Image.sub_image : Subtracts other Image values from self Image values. Modified in place.
      hdrl.core.Image.sub_scalar : Elementwise subtraction of a scalar from an image. Modified in place.
       )pydoc")

      .def(
          "sub_image",
          [](hdrl::core::Image& self,
             std::shared_ptr<hdrl::core::Image> other) {
            self.sub_image(other);
          },
          py::arg("other"),
          R"pydoc(
      Subtracts other Image values from self Image values. Modified in place.

      Parameters
      ----------
      other : hdrl.core.Image
            Image subtracted from self.

      See Also
      --------
      hdrl.core.Image.sub_image_create : Subtract two images and return the resulting image.
      hdrl.core.Image.sub_scalar : Elementwise subtraction of a scalar from an image. Modified in place.
       )pydoc")

      .def(
          "sub_scalar",
          [](hdrl::core::Image& self, hdrl::core::Value v) {
            self.sub_scalar(v);
          },
          py::arg("value"),
          R"pydoc(
      Elementwise subtraction of a scalar from an image. Modified in place.

      Parameters
      ----------
      value : tuple(float, float)
            Non-zero number to subtract from self. The first component is the data value, the second is the error value.

      See Also
      --------
      hdrl.core.Image.sub_image : Subtracts other Image values from self Image values. Modified in place.
      hdrl.core.Image.sub_image_create : Subtract two images and return the resulting image.
       )pydoc")

      .def(
          "add_image_create",
          [](hdrl::core::Image& self,
             std::shared_ptr<hdrl::core::Image> other) {
            return self.add_image_create(other);
          },
          py::arg("other"),
          R"pydoc(
      Add two images and return the resulting image.

      Parameters
      ----------
      other : hdrl.core.Image
            Image added to self

      Returns
      -------
      hdrl.core.Image
            A newly allocated image.

      See Also
      --------
      hdrl.core.Image.add_image : Adds values from Image other to self. Modified in place.
      hdrl.core.Image.add_scalar : Elementwise addition of a scalar to an image. Modified in place.
       )pydoc")

      .def(
          "add_image",
          [](hdrl::core::Image& self,
             std::shared_ptr<hdrl::core::Image> other) {
            self.add_image(other);
          },
          py::arg("other"),
          R"pydoc(
      Adds values from Image other to self. Modified in place.

      Parameters
      ----------
      other : hdrl.core.Image
            Image added to self

      See Also
      --------
      hdrl.core.Image.add_image_create : Add two images and return the resulting image.
      hdrl.core.Image.add_scalar : Elementwise addition of a scalar to an image. Modified in place.
       )pydoc")

      .def(
          "add_scalar",
          [](hdrl::core::Image& self, hdrl::core::Value v) {
            self.add_scalar(v);
          },
          py::arg("value"),
          R"pydoc(
      Elementwise addition of a scalar to an image. Modified in place.

      Parameters
      ----------
      value : tuple(float, float)
            Non-zero number to add to image values. The first component is the data value, the second is the error value.

      See Also
      --------
      hdrl.core.Image.add_image : Adds values from Image other to self. Modified in place.
      hdrl.core.Image.add_image_create : Add two images and return the resulting image.
       )pydoc")

      .def(
          "exp_scalar_create",
          [](hdrl::core::Image& self, hdrl::core::Value v) {
            return self.exp_scalar_create(v);
          },
          py::arg("base"),
          R"pydoc(
      Computes the exponential of an image by a scalar creating a new image.

      Parameters
      ----------
      base: tuple(float, float)
            Base of the power. The first component is the data value, the second is the error value.

      Returns
      -------
      hdrl.core.Image
            A new image containing the powered data.

      See Also
      --------
      hdrl.core.Image.exp_scalar : Computes the exponential of an image by a scalar. Modified in place.
       )pydoc")

      .def(
          "exp_scalar",
          [](hdrl::core::Image& self, hdrl::core::Value v) {
            self.exp_scalar(v);
          },
          py::arg("base"),
          R"pydoc(
      Computes the exponential of an image by a scalar. Modified in place.

      Parameters
      ----------
      base: tuple(float, float)
            Base of the power. The first component is the data value, the second is the error value.
      

      See Also
      --------
      hdrl.core.Image.exp_scalar_create : Computes the exponential of an image by a scalar creating a new image.
       )pydoc")

      .def(
          "pow_scalar",
          [](hdrl::core::Image& self, hdrl::core::Value v) {
            self.pow_scalar(v);
          },
          py::arg("exponent"),
          R"pydoc(
      Computes the power of an image by a scalar. Modified in place.

      Parameters
      ----------
      exponent : tuple(float, float)
         Exponent of the power. The first component is the data value, the second is the error value.

      
      See Also
      --------
      hdrl.core.Image.pow_scalar_create : Computes the power of an image by a scalar creating a new image.
       )pydoc")

      .def(
          "pow_scalar_create",
          [](hdrl::core::Image& self, hdrl::core::Value v) {
            return self.pow_scalar_create(v);
          },
          py::arg("exponent"),
          R"pydoc(
      Computes the power of an image by a scalar creating a new image.

      Parameters
      ----------
      exponent : tuple(float, float)
         Exponent of the power. The first component is the data value, the second is the error value.

      Returns
      -------
      hdrl.core.Image
           A new image containing the powered data.

      
      See Also
      --------
      hdrl.core.Image.pow_scalar : Computes the power of an image by a scalar. Modified in place.
       )pydoc")

      .def("turn", &hdrl::core::Image::turn, py::arg("rot"),
           R"pydoc(
      Rotate an image by a multiple of 90 degrees clockwise. Modified in place.

      Parameters
      ----------
      rot : int
          Value for rotating image by 90 deg in the counterclockwise direction.

       )pydoc")

      .def(
          "reject",
          [](hdrl::core::Image& self, cpl_size ypos, cpl_size xpos) {
            self.reject(ypos, xpos);
          },
          py::arg("ypos"), py::arg("xpos"),
          R"pydoc(
      Marks pixel as bad.

      Parameters
      ----------
      ypos: int
                y coordinate
      xpos: int
                x coordinate

      See Also
      --------
      hdrl.core.Image.reject_from_mask : Sets the bad pixel mask of hdrl.core.Image
      hdrl.core.Image.is_rejected : Return if the pixel is marked bad
      hdrl.core.Image.reject_value : Reject pixels with the specified special value(s).
      hdrl.core.Image.count_rejected : Returns the number of rejected pixels.
       )pydoc")

      .def(
          "reject_from_mask",
          [](hdrl::core::Image& self, hdrl::core::pycpl_mask map) {
            self.reject_from_mask(map);
          },
          py::arg("map"),
          R"pydoc(
      Sets the bad pixel mask of hdrl.core.Image

      Parameters
      ----------
      map : cpl.core.Mask
            Bad pixel mask to set.

      
      See Also
      --------
      hdrl.core.Image.reject : Marks pixel as bad.
      hdrl.core.Image.is_rejected : Return if the pixel is marked bad
      hdrl.core.Image.reject_value : Reject pixels with the specified special value(s).
      hdrl.core.Image.count_rejected : Returns the number of rejected pixels.
       )pydoc")

      .def("duplicate", &hdrl::core::Image::duplicate,
           R"pydoc( 
      Copy hdrl.core.Image
      
      Returns
      -------
      hdrl.core.Image
            New image with duplicate values.
      
      See Also
      --------
      hdrl.core.Image.copy_into : Copy one hdrl.core.Image into another.
      )pydoc")
      .def(
          "accept",
          [](hdrl::core::Image& self, cpl_size ypos, cpl_size xpos) {
            self.accept(ypos, xpos);
          },
          py::arg("ypos"), py::arg("xpos"),
          R"pydoc(
      Marks pixel as good.

      Parameters
      ----------
      ypos: int
                y coordinate
      xpos: int
                x coordinate

      See Also
      --------
      hdrl.core.Image.accept_all :  Returns the number of rejected pixels.
       )pydoc")

      .def(
          "is_rejected",
          [](hdrl::core::Image& self, cpl_size ypos, cpl_size xpos) {
            return self.is_rejected(ypos, xpos);
          },
          py::arg("ypos"), py::arg("xpos"),
          R"pydoc(
      Return if the pixel is marked bad

      Parameters
      ----------
      ypos: int
                y coordinate
      xpos: int
                x coordinate

      See Also
      --------
      hdrl.core.Image.reject_from_mask : Sets the bad pixel mask of hdrl.core.Image
      hdrl.core.Image.reject : Marks pixel as bad.
      hdrl.core.Image.reject_value : Reject pixels with the specified special value(s).
      hdrl.core.Image.count_rejected : Returns the number of rejected pixels.
       )pydoc")

      .def("count_rejected", &hdrl::core::Image::count_rejected,
           R"pydoc(
      Returns the number of rejected pixels.
      
      See Also
      --------
      hdrl.core.Image.reject_from_mask : Sets the bad pixel mask of hdrl.core.Image
      hdrl.core.Image.reject : Marks pixel as bad.
      hdrl.core.Image.reject_value : Reject pixels with the specified special value(s)
      hdrl.core.Image.is_rejected : Return if the pixel is marked bad
      )pydoc")
      .def(
          "reject_value",
          [](hdrl::core::Image& self, py::set values) {
            // The following is copied from cpl::core::Image bindings
            int mode = 0;
            // Need to iterate through the set of values instead of using
            // .contains() so that we can use std::isnan().
            py::iterator iter = values.begin();
            while (iter != py::iterator::sentinel()) {
              // Get a value from the set by dereferencing the iterator and
              // explicitly casting to double.
              auto value = (*iter).cast<double>();
              // For each value check against the supported special values and
              // make sure the corresponding bits are set.
              if (value == 0.0) {
                mode = mode | CPL_VALUE_ZERO;
              } else if (std::isinf(value)) {
                if (value > 0) {
                  mode = mode | CPL_VALUE_PLUSINF;
                } else {
                  mode = mode | CPL_VALUE_MINUSINF;
                }
              } else if (std::isnan(value)) {
                mode = mode | CPL_VALUE_NAN;
              } else {
                throw hdrl::core::UnsupportedModeError(
                    HDRL_ERROR_LOCATION,
                    "Reject values must be 0, -Inf, +Inf or NaN");
              }
              ++iter;
            }
            self.reject_value(static_cast<cpl_value>(mode));
          },
          py::arg("values"),
          R"pydoc(
        Reject pixels with the specified special value(s)

      Parameters
      ----------
        values: set
          The set of special values that should be marked as rejected pixels.
          The supported special values are 0, math.inf, -math.inf, math.nan
          and their numpy equivalents, and any combination is allowed.

      Raises
      ------
        hdrl.core.UnsupportedModeError
          If something other than one of the supported special values is in
          the values parameter.
        hdrl.core.InvalidTypeError
          If the image is a complex type.
      

      See Also
      --------
      hdrl.core.Image.reject_from_mask : Sets the bad pixel mask of hdrl.core.Image
      hdrl.core.Image.reject : Marks pixel as bad.
      hdrl.core.Image.is_rejected : Return if the pixel is marked bad.
      hdrl.core.Image.count_rejected : Returns the number of rejected pixels.
      )pydoc")
      .def("accept_all", &hdrl::core::Image::accept_all,
           R"pydoc(
            Returns the number of rejected pixels.
      
      See Also
      --------
      hdrl.core.Image.accept : Marks pixel as good.
      )pydoc")
      /*
      .def_static("check", [](hdrl::core::pycpl_image i){
          hdrl::core::Image::check(i);
      }, py::arg("image"), R"docstring(
      "Check an image
      )docstring")
      .def_static("pass_through", [](hdrl::core::pycpl_image i){
          return hdrl::core::Image::pass_through(i);
      }, py::arg("image"), R"docstring(
      "Pass through an image
      )docstring")
*/
      .def("get_sqsum", &hdrl::core::Image::get_sqsum,
           R"pydoc(
      Computes the sum of all pixel values and the error of a squared image.
      
      Returns
      -------
      namedtuple
         The namedtuple Value contains two doubles: It returns the squared sum (data) and its error (error).
         If desired, the namedtuple can be converted to a dictionary using its ._asdict() method.

      
      See Also
      --------
      hdrl.core.Image.get_sum : Computes the sum of all pixel values and the associated error of an image.
      )pydoc")
      .def("get_sum", &hdrl::core::Image::get_sum,
           R"pydoc(
      Computes the sum of all pixel values and the associated error of an image.

      Returns
      -------
      namedtuple
         The namedtuple Value contains two doubles: It returns the sum (data) and its error (error).
         If desired, the namedtuple can be converted to a dictionary using its ._asdict() method.

      
      See Also
      --------
      hdrl.core.Image.get_sqsum : Computes the sum of all pixel values and the error of a squared image.
      )pydoc")
      .def("get_stdev", &hdrl::core::Image::get_stdev,
           R"pydoc(
      Computes the standard deviation of the data of an image
      
      Returns
      -------
       float
            The standard deviation of the data of an image.
      
      )pydoc")
      .def("get_median", &hdrl::core::Image::get_median,
           R"pydoc(
      Computes the median and associated error of an image.
      
      Returns
      -------
       namedtuple
         The namedtuple Value contains two doubles: It returns the median (data) and its error (error).
         If desired, the namedtuple can be converted to a dictionary using its ._asdict() method.

      )pydoc")
      .def("get_mean", &hdrl::core::Image::get_mean,
           R"pydoc(
      Computes mean pixel value and associated error of an image.
      
      Returns
      -------
       namedtuple
         The namedtuple Value contains two doubles: It returns the mean (data) and its error (error).
         If desired, the namedtuple can be converted to a dictionary using its ._asdict() method.
      
      
      See Also
      --------
      hdrl.core.Image.get_weighted_mean : Computes the weighted mean and associated error of an image.
      hdrl.core.Image.get_minmax_mean : Computes the minmax rejected mean and the associated error of an image.
      hdrl.core.Image.get_sigclip_mean : Computes the sigma-clipped mean and associated error of an image.
      )pydoc")
      .def("get_weighted_mean", &hdrl::core::Image::get_weighted_mean,
           R"pydoc(
      Computes the weighted mean and associated error of an image.
      
      Returns
      -------
       namedtuple
         The namedtuple Value contains two doubles: It returns the weighted mean (data) and its error (error).
         If desired, the namedtuple can be converted to a dictionary using its ._asdict() method.
      
      
      See Also
      --------
      hdrl.core.Image.get_mean : Computes mean pixel value and associated error of an image.
      hdrl.core.Image.get_minmax_mean : Computes the minmax rejected mean and the associated error of an image.
      hdrl.core.Image.get_sigclip_mean : Computes the sigma-clipped mean and associated error of an image.
      )pydoc")
      .def("get_sigclip_mean", &hdrl::core::Image::get_sigclip_mean,
           py::arg("kappa_low"), py::arg("kappa_high"), py::arg("niter"),
           R"pydoc(
      Computes the sigma-clipped mean and associated error of an image.

      Parameters
      ----------
      kappa_low: float
            low sigma bound
      kappa_high: float
            high sigma bound.
      niter: int
            maximum number of clipping iterators.

      Returns
      -------
       namedtuple
         The namedtuple Value contains two doubles: It returns the clipped mean (data) and its error (error).
         If desired, the namedtuple can be converted to a dictionary using its ._asdict() method.

      See Also
      --------
      hdrl.core.Image.get_minmax_mean : Computes the minmax rejected mean and the associated error of an image.
      hdrl.core.Image.get_mean : Computes mean pixel value and associated error of an image.
      hdrl.core.Image.get_weighted_mean : Computes the weighted mean and associated error of an image.
       )pydoc")

      .def("get_minmax_mean", &hdrl::core::Image::get_minmax_mean,
           py::arg("nlow"), py::arg("nhigh"),
           R"pydoc(
      Computes the minmax rejected mean and the associated error of an image.

      Parameters
      ----------
      nlow: float
            Number of low pixels to reject.
      nhigh: float
            Number of high pixels to reject.

      Returns
      -------
      namedtuple
         The namedtuple Value contains two doubles: It returns the  minmax rejected mean (data) and its error (error).
         If desired, the namedtuple can be converted to a dictionary using its ._asdict() method.


      See Also
      --------
      hdrl.core.Image.get_sigclip_mean : Computes the sigma-clipped mean and associated error of an image.
      hdrl.core.Image.get_weighted_mean : Computes the weighted mean and associated error of an image.
      hdrl.core.Image.get_mean : Computes mean pixel value and associated error of an image.
      )pydoc")

      .def("get_mode", &hdrl::core::Image::get_mode, py::arg("histo_min"),
           py::arg("histo_max"), py::arg("bin_size"), py::arg("method"),
           py::arg("niter"),
           R"pydoc(
      Computes the mode and the associated error of an image.

      Parameters
      ----------
      histo_min : float
        minimum value of low pixels to be uses
      histo_max : float
         maximum value of high pixels to be used
      bin_size : float
          the size of the histogram bin
      method : hdrl.func.Collapse.Method
            method to use for the mode computation
      niter : int
       number of iterations to compute the error of the mode

      Returns
      -------
      namedtuple
         The namedtuple Value contains two doubles: It returns the mode (data) and its error (error).
         If desired, the namedtuple can be converted to a dictionary using its ._asdict() method.

       )pydoc")

      .def("__str__",
           [](hdrl::core::Image& self) {
             return self.dump(hdrl::core::pycpl_window::All);
           })
      .def("__repr__", &hdrl::core::Image::dump_structure)
      .def(
          "dump",
          [](hdrl::core::Image& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<hdrl::core::pycpl_window> window,
             std::optional<bool> show) {
            std::string dstr = self.dump(window);
            return dump_handler(filename.value(), mode.value(), dstr,
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("window") = py::none(), py::arg("show") = true,
          R"pydoc(
      Dump the image contents to a file, stdout or a string.

      This function is intended just for debugging. It prints the contents of an image
      to the file path specified by `filename`. 
      If a `filename` is not specified, output goes to stdout (unless `show` is False). 
      In both cases, the contents are also returned as a string.

      Parameters
      ----------
      filename : str, optional
            File to dump image contents to
      mode : str, optional
            Mode to open the file with. Defaults to "w" (write, overwriting the contents of
            the file if it already exists), but can also be set to "a" (append, creating the file
            if it does not already exist or appending to the end of it if it does).
      window : tuple(int,int,int,int), optional
              Window to dump with `value` in the format (llx, lly, urx, ury) where:
              - `llx` Lower left X coordinate
              - `lly` Lower left Y coordinate
              - `urx` Upper right X coordinate 
              - `ury` Upper right Y coordinate
              Defaults to None (no window).
      show : bool, optional
          Send image contents to stdout. Defaults to True.

      Returns
      -------
      str 
          A multiline string containing the dump of the image contents.
          )pydoc")
      .def_static(
          "zeros",
          [](int width, int height) {
            return new hdrl::core::Image(width, height);
          },
          py::arg("width"), py::arg("height"), R"docstring(
      Create a new zeros filled hdrl.core.Image of width x height dimensions.

      Parameters
      -----------
      width : int
            Width of Image.
      height : int
            Height of Image.

      Returns
      -------
      hdrl.core.Image
            New hdrl.core.Image (width x height) initialised with all 0’s.
      )docstring");
}
