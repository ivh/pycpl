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

#ifndef PYHDRL_CORE_PYCPL_VECTOR_HPP_
#define PYHDRL_CORE_PYCPL_VECTOR_HPP_

#include <cpl_type.h>
#include <cpl_vector.h>
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
struct type_caster<hdrl::core::pycpl_vector>
{
  /**
   * This macro establishes the name 'hdrl::core::pycpl_vector' in
   * function signatures and declares a local variable
   * 'value' of type hdrl::core::pycpl_vector
   */

  PYBIND11_TYPE_CASTER(hdrl::core::pycpl_vector, _("cpl.core.Vector"));

  /**
   * Conversion part 1 (Python->C++): convert a PyObject into a
   * hdrl::core::pycpl_vector instance or return false upon failure. The second
   * argument indicates whether implicit conversions should be applied.
   */
  bool load(handle src, bool /* conversion */)
  {
    // Extract PyObject from handle
    // Borrowed being true means the refcount is still OK after this scope ends
    py::object source = reinterpret_borrow<py::object>(src);
    py::module_ pycpl_core = py::module_::import("cpl.core");

    // Allow for None to result in a null ptr (cpl_vector* v = NULL)
    if (!source || source.is(py::none())) {  // Python None objects
      value.v = nullptr;
      return true;
    }
    // If the object type is not cpl.core.Image, throw an error
    if (!py::isinstance(source, pycpl_core.attr("Vector"))) {
      value.v = nullptr;
      throw hdrl::core::InvalidTypeError(HDRL_ERROR_LOCATION,
                                         "Expected cpl.core.Vector type");
    }
    // Create a new cpl_image with the contents of the cpl.core.Image object
    try {
      int n = source.attr("size").cast<int>();
      // Retrieve the data
      py::array arr = source.attr("__array__")();
      py::buffer_info buf = arr.request();
      double* data = static_cast<double*>(buf.ptr);
      cpl_vector* new_vec = cpl_vector_wrap(n, data);
      value.v = cpl_vector_duplicate(new_vec);
      return true;
    }
    catch (py::error_already_set& err) {
      return false;
    }
  }

  /**
   * Conversion part 2 (C++ -> Python): convert an hdrl::core::pycpl_vector
   * instance into a Python object. The second and third arguments are used to
   * indicate the return value policy and parent object (for
   * ``return_value_policy::reference_internal``) and are generally
   * ignored by implicit casters.
   */
  static handle cast(hdrl::core::pycpl_vector src, return_value_policy, handle)
  {
    // if the pointer is null, return a None object
    if (src.v == nullptr) {
      return py::none();
    }

    cpl_vector* input = src.v;
    // Create a new cpl.core.Vector object and fill with contents of the
    // cpl_vector* input
    py::module_ pycpl_core = py::module_::import("cpl.core");
    int n = cpl_vector_get_size(input);
    double* data = cpl_vector_get_data(input);
    if (data == nullptr) {
      return py::none();
    }

    py::array vec_data =
        py::array(py::buffer_info(data,
                                  sizeof(double),  // itemsize
                                  py::format_descriptor<double>::format(),
                                  1,                         // ndim
                                  std::vector<cpl_size>{n},  // shape
                                  std::vector<cpl_size>{sizeof(double)}));
    py::list vec_list = vec_data.attr("tolist")();
    py::object new_vec = pycpl_core.attr("Vector")(vec_list);
    return new_vec.release();
  }
};

}  // namespace detail
}  // namespace pybind11

#endif  // PYHDRL_CORE_PYCPL_VECTOR_HPP_