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

#ifndef PYHDRL_CORE_PYCPL_MASK_HPP_
#define PYHDRL_CORE_PYCPL_MASK_HPP_

#include <cpl_mask.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include "hdrlcore/error.hpp"
#include "hdrlcore/pycpl_types.hpp"

namespace py = pybind11;

// using size = cpl::core::size;

namespace pybind11
{
namespace detail
{
template <>
struct type_caster<hdrl::core::pycpl_mask>
{
  /**
   * This macro establishes the name 'hdrl::core::pycpl_mask' in
   * function signatures and declares a local variable
   * 'value' of type hdrl::core::pycpl_mask
   */

  PYBIND11_TYPE_CASTER(hdrl::core::pycpl_mask, _("cpl.core.Mask"));

  /**
   * Conversion part 1 (Python->C++): convert a PyObject into a
   * hdrl::core::pycpl_mask instance or return false upon failure. The second
   * argument indicates whether implicit conversions should be applied.
   */
  bool load(handle src, bool /* conversion */)
  {
    // Extract PyObject from handle
    // Borrowed being true means the refcount is still OK after this scope ends
    py::object source = reinterpret_borrow<py::object>(src);
    py::module_ pycpl_core = py::module_::import("cpl.core");

    // Allow for None to result in a null ptr (cpl_mask* m = NULL)
    if (!source || source.is(py::none())) {  // Python None objects
      value.m = nullptr;
      return true;
    }
    // If the object type is not cpl.core.Image, throw an error
    if (!py::isinstance(source, pycpl_core.attr("Mask"))) {
      value.m = nullptr;
      throw hdrl::core::InvalidTypeError(HDRL_ERROR_LOCATION,
                                         "Expected cpl.core.Mask type");
    }
    // Create a new cpl_image with the contents of the cpl.core.Image object
    try {
      int mw = source.attr("width").cast<int>();
      int mh = source.attr("height").cast<int>();
      int mn = mw * mh;
      py::bytes buf = source.attr("_mask").attr("get_bytes")(0, mn);
      py::list arr = py::list(buf);
      std::vector<cpl_binary> mask(mn);
      for (int i = 0; i < mn; i++) {
        mask[i] = (arr[i].cast<bool>()) ? CPL_BINARY_1 : CPL_BINARY_0;
      }
      // create the mask by directly accessing the vector's cpl_binary* data
      cpl_mask* new_mask = cpl_mask_wrap(mw, mh, mask.data());
      value.m = cpl_mask_duplicate(new_mask);
      return true;
    }
    catch (py::error_already_set& err) {
      return false;
    }
  }

  /**
   * Conversion part 2 (C++ -> Python): convert an hdrl::core::pycpl_mask
   * instance into a Python object. The second and third arguments are used to
   * indicate the return value policy and parent object (for
   * ``return_value_policy::reference_internal``) and are generally
   * ignored by implicit casters.
   */
  static handle cast(hdrl::core::pycpl_mask src, return_value_policy, handle)
  {
    // if the pointer is null, return a None object
    if (src.m == nullptr) {
      return py::none();
    }

    cpl_mask* input = src.m;
    // Create a new cpl.core.Mask object and fill with contents of the
    // cpl_image*
    py::module_ pycpl_core = py::module_::import("cpl.core");
    int mw = cpl_mask_get_size_x(input);
    int mh = cpl_mask_get_size_y(input);
    int mn = mw * mh;
    cpl_binary* mdata = cpl_mask_get_data(input);
    if (mdata == nullptr) {
      return py::none();
    }
    std::vector<int> mask(mn);
    for (int i = 0; i < mn; i++) {
      mask[i] = static_cast<int>(mdata[i]);
    }
    py::array mask_data = py::array(py::buffer_info(
        mask.data(),
        sizeof(int),  // itemsize
        py::format_descriptor<int>::format(),
        2,                              // ndim
        std::vector<cpl_size>{mh, mw},  // shape
        std::vector<cpl_size>{(cpl_size)sizeof(int) * mw, sizeof(int)}
        // strides
        ));
    py::list mask_list = mask_data.attr("tolist")();
    py::object new_mask = pycpl_core.attr("Mask")(mask_list);
    return new_mask.release();
  }
};

}  // namespace detail
}  // namespace pybind11

#endif  // PYHDRL_CORE_PYCPL_MASK_HPP_