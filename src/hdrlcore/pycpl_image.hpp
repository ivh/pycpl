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

#ifndef PYHDRL_CORE_PYCPL_IMAGE_HPP_
#define PYHDRL_CORE_PYCPL_IMAGE_HPP_

#include <complex>

#include <cpl_image_io.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include "hdrlcore/error.hpp"
#include "hdrlcore/pycpl_types.hpp"

namespace py = pybind11;

namespace pybind11
{
namespace detail
{
template <>
struct type_caster<hdrl::core::pycpl_image>
{
  /**
   * This macro establishes the name 'hdrl::core::pycpl_image' in
   * function signatures and declares a local variable
   * 'value' of type hdrl::core::pycpl_image
   */

  PYBIND11_TYPE_CASTER(hdrl::core::pycpl_image, _("cpl.core.Image"));

  /**
   * Conversion part 1 (Python->C++): convert a PyObject into a
   * cpl::core::Window instance or return false upon failure. The second
   * argument indicates whether implicit conversions should be applied.
   */
  bool load(handle src, bool /* conversion */)
  {
    // Extract PyObject from handle
    // Borrowed being true means the refcount is still OK after this scope ends
    py::object source = reinterpret_borrow<py::object>(src);
    py::module_ pycpl_core = py::module_::import("cpl.core");

    // Allow for None to result in a null ptr (cpl_image* im = NULL)
    if (!source || source.is(py::none())) {  // Python None objects
      value.im = nullptr;
      return true;
    }
    // If the object type is not cpl.core.Image, throw an error
    if (!py::isinstance(source, pycpl_core.attr("Image"))) {
      value.im = nullptr;
      throw hdrl::core::InvalidTypeError(HDRL_ERROR_LOCATION,
                                         "Expected cpl.core.Image type");
    }
    // Create a new cpl_image with the contents of the cpl.core.Image object
    try {
      cpl_image* new_img;

      int width = source.attr("width").cast<int>();
      int height = source.attr("height").cast<int>();
      cpl_type type = source.attr("type").cast<cpl_type>();

      // retrieve the pixel data
      py::array arr = source.attr("as_array")();
      py::buffer_info buf = arr.request();

      switch (type) {
        case CPL_TYPE_INT: {
          int* data = static_cast<int*>(buf.ptr);
          new_img = cpl_image_wrap_int(width, height, data);
          break;
        }
        case CPL_TYPE_FLOAT: {
          float* data = static_cast<float*>(buf.ptr);
          new_img = cpl_image_wrap_float(width, height, data);
          break;
        }
        case CPL_TYPE_DOUBLE: {
          double* data = static_cast<double*>(buf.ptr);
          new_img = cpl_image_wrap_double(width, height, data);
          break;
        }
        case CPL_TYPE_FLOAT_COMPLEX: {
          _Complex float* data = static_cast<_Complex float*>(buf.ptr);
          new_img = cpl_image_wrap_float_complex(width, height, data);
          break;
        }
        case CPL_TYPE_DOUBLE_COMPLEX: {
          _Complex double* data = static_cast<_Complex double*>(buf.ptr);
          new_img = cpl_image_wrap_double_complex(width, height, data);
          break;
        }
        default: {
          throw hdrl::core::InvalidTypeError(
              HDRL_ERROR_LOCATION, "Unsupported cpl.core.Image data type");
          break;
        }
      }
      // Add the bad pixel mask if present
      hdrl::core::pycpl_mask mask =
          source.attr("bpm").cast<hdrl::core::pycpl_mask>();
      if (mask.m != nullptr) {
        // Add the mask to the image
        cpl_image_reject_from_mask(new_img, mask.m);
        // cpl_image_set_bpm(new_img,mask.m);
      }
      value.im = cpl_image_duplicate(new_img);
      cpl_image_unwrap(new_img);
      return true;
    }
    catch (py::error_already_set& err) {
      return false;
    }
  }

  /**
   * Conversion part 2 (C++ -> Python): convert an hdrl::core::pycpl_image
   * instance into a Python object. The second and third arguments are used to
   * indicate the return value policy and parent object (for
   * ``return_value_policy::reference_internal``) and are generally
   * ignored by implicit casters.
   */
  static handle cast(hdrl::core::pycpl_image src, return_value_policy, handle)
  {
    // If the pointer is null, return a None object
    if (src.im == nullptr) {
      return py::none();
    }

    cpl_image* input = src.im;
    int width = cpl_image_get_size_x(input);
    int height = cpl_image_get_size_y(input);
    cpl_type type = cpl_image_get_type(input);

    // Retrieve the pixel data
    py::array pixel_data;
    switch (type) {
      case CPL_TYPE_INT: {
        pixel_data = py::array(py::buffer_info(
            cpl_image_get_data_int(input), sizeof(int),
            py::format_descriptor<int>::format(), 2,
            std::vector<cpl_size>{height, width},
            std::vector<cpl_size>{(cpl_size)sizeof(int) * width, sizeof(int)}));
        break;
      }
      case CPL_TYPE_FLOAT: {
        pixel_data = py::array(py::buffer_info(
            cpl_image_get_data_float(input), sizeof(float),
            py::format_descriptor<float>::format(), 2,
            std::vector<cpl_size>{height, width},
            std::vector<cpl_size>{(cpl_size)sizeof(float) * width,
                                  sizeof(float)}));
        break;
      }
      case CPL_TYPE_DOUBLE: {
        pixel_data = py::array(py::buffer_info(
            cpl_image_get_data_double(input), sizeof(double),
            py::format_descriptor<double>::format(), 2,
            std::vector<cpl_size>{height, width},
            std::vector<cpl_size>{(cpl_size)sizeof(double) * width,
                                  sizeof(double)}));
        break;
      }
      case CPL_TYPE_FLOAT_COMPLEX: {
        pixel_data = py::array(py::buffer_info(
            cpl_image_get_data_float_complex(input),
            sizeof(std::complex<float>),
            py::format_descriptor<std::complex<float>>::format(), 2,
            std::vector<cpl_size>{height, width},
            std::vector<cpl_size>{(cpl_size)sizeof(std::complex<float>) * width,
                                  sizeof(std::complex<float>)}));
        break;
      }
      case CPL_TYPE_DOUBLE_COMPLEX: {
        pixel_data = py::array(py::buffer_info(
            cpl_image_get_data_double_complex(input),
            sizeof(std::complex<double>),
            py::format_descriptor<std::complex<double>>::format(), 2,
            std::vector<cpl_size>{height, width},
            std::vector<cpl_size>{(cpl_size)sizeof(std::complex<double>) *
                                      width,
                                  sizeof(std::complex<double>)}));
        break;
      }
      default: {
        throw hdrl::core::InvalidTypeError(
            HDRL_ERROR_LOCATION, "Unsupported cpl.core.Image data type");
        break;
      }
    }
    // Create a new cpl.core.Image object and fill with contents of the
    // cpl_image*
    py::module_ pycpl_core = py::module_::import("cpl.core");
    py::object im = pycpl_core.attr("Image")(pixel_data, type);

    // Add the bad pixel mask if present
    hdrl::core::pycpl_mask bpm =
        hdrl::core::Error::throw_errors_with(cpl_image_get_bpm, input);
    if (bpm.m != nullptr) {
      im.attr("bpm") = pycpl_core.attr("Mask")(bpm);
    }
    return im.release();
  }
};

}  // namespace detail
}  // namespace pybind11

#endif  // PYHDRL_CORE_PYCPL_IMAGE_HPP_