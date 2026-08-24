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

#ifndef PYHDRL_CORE_VALUE_HPP_
#define PYHDRL_CORE_VALUE_HPP_

#include <cpl_type.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include "hdrlcore/pycpl_types.hpp"

namespace py = pybind11;

namespace pybind11
{
namespace detail
{
template <>
struct type_caster<hdrl::core::Value>
{
  PYBIND11_TYPE_CASTER(hdrl::core::Value, _("tuple"));

  bool load(handle src, bool /* conversion */)
  {
    // Extract PyObject from handle
    // Borrowed being true means the refcount is still OK after this scope ends
    py::object source = reinterpret_borrow<py::object>(src);

    // Python None objects
    if (!source || source.is(py::none())) {
      hdrl_value empty;
      value.v = empty;
      return true;
    }
    try {
      int tuple_size =
          py::module::import("builtins").attr("len")(source).cast<int>();
      py::object call_getitem = source.attr("__getitem__");

      // Need to cater case where invalid flag is given; it is ignored for the
      // purposes here...
      if (tuple_size >= 2) {
        if (HDRL_TYPE_DATA == CPL_TYPE_FLOAT) {
          value.v.data = call_getitem(0).cast<float>();
          value.v.error = call_getitem(1).cast<float>();
        } else {
          value.v.data = call_getitem(0).cast<double>();
          value.v.error = call_getitem(1).cast<double>();
        }

        return true;
      } else {
        return false;
      }
    }
    catch (py::error_already_set& err) {
      return false;
    }
  }

  static handle cast(hdrl::core::Value src, return_value_policy, handle)
  {
    py::module_ col = py::module_::import("collections");
    py::object ntup = col.attr("namedtuple")("Value", "data error invalid");
    py::object result;
    if (std::isnan(src.v.data)) {
      result = ntup(py::none(), py::none(), true);
    } else {
      result = ntup(src.v.data, src.v.error, false);
    }
    return result.release();
  }
};

}  // namespace detail
}  // namespace pybind11

#endif  // PYHDRL_CORE_VALUE_HPP_
