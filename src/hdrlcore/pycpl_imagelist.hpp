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

#ifndef PYHDRL_CORE_PYCPL_IMAGELIST_HPP_
#define PYHDRL_CORE_PYCPL_IMAGELIST_HPP_

#include <cpl_imagelist.h>
#include <cpl_imagelist_io.h>
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
struct type_caster<hdrl::core::pycpl_imagelist>
{
  /**
   * This macro establishes the name 'hdrl::core::pycpl_imagelist' in
   * function signatures and declares a local variable
   * 'value' of type hdrl::core::pycpl_imagelist
   */

  PYBIND11_TYPE_CASTER(hdrl::core::pycpl_imagelist, _("cpl.core.ImageList"));

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
      value.il = nullptr;
      return true;
    }
    // If the object type is not cpl.core.Image, throw an error
    if (!py::isinstance(source, pycpl_core.attr("ImageList"))) {
      value.il = nullptr;
      throw hdrl::core::InvalidTypeError(HDRL_ERROR_LOCATION,
                                         "Expected cpl.core.ImageList type");
    }
    // Create a new cpl_image with the contents of the cpl.core.Image object
    try {
      value.il = cpl_imagelist_new();

      int list_size =
          py::module::import("builtins").attr("len")(source).cast<int>();
      py::object call_getitem = source.attr("__getitem__");
      for (int i = 0; i < list_size; ++i) {
        hdrl::core::pycpl_image img =
            call_getitem(i).cast<hdrl::core::pycpl_image>();

        // Need to clone the image here because setting the
        // list element transfers ownership!
        cpl_imagelist_set(value.il, cpl_image_duplicate(img.im), i);
      }
      return true;
    }
    catch (py::error_already_set& err) {
      return false;
    }
  }

  /**
   * Conversion part 2 (C++ -> Python): convert an hdrl::core::pycpl_imagelist
   * instance into a Python object. The second and third arguments are used to
   * indicate the return value policy and parent object (for
   * ``return_value_policy::reference_internal``) and are generally
   * ignored by implicit casters.
   */
  static handle
  cast(hdrl::core::pycpl_imagelist src, return_value_policy, handle)
  {
    // If the pointer is null, return a None object
    if (src.il == nullptr) {
      return py::none();
    }

    py::module_ pycpl_core = py::module_::import("cpl.core");
    py::object imlist = pycpl_core.attr("ImageList")();
    cpl_imagelist* input = src.il;
    int list_size = cpl_imagelist_get_size(input);
    for (int i = 0; i < list_size; ++i) {
      hdrl::core::pycpl_image img = cpl_imagelist_get(input, i);
      imlist.attr("append")(img);
    }
    return imlist.release();
  }
};

}  // namespace detail
}  // namespace pybind11

#endif  // PYHDRL_CORE_PYCPL_IMAGELIST_HPP_