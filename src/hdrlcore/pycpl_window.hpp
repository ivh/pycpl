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

#ifndef PYHDRL_CORE_PYCPL_WINDOW_HPP_
#define PYHDRL_CORE_PYCPL_WINDOW_HPP_

#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <cpl_type.h>

#include "hdrlcore/pycpl_types.hpp"


namespace py = pybind11;

namespace pybind11
{
namespace detail
{
template <>
struct type_caster<hdrl::core::pycpl_window>
{
  PYBIND11_TYPE_CASTER(hdrl::core::pycpl_window, _("tuple"));

  bool load(handle src, bool /* conversion */)
  {
    py::object source = reinterpret_borrow<py::object>(src);
    py::module_ pycpl_core = py::module_::import("cpl.core");

    if (!source || source.is(py::none())) {
      value = hdrl::core::pycpl_window::All;
      return true;
    }
#if 0
    // FIXME: Figure out if this is just dead code or part of an
    //        incomplete or not yet exisitng feature
    if (!(py::isinstance(source,pycpl_core.attr("Window")) ||
    source.is(py::tuple())){
      value = hdrl::core::pycpl_window::All;
      throw hdrl::core::InvalidTypeError(HDRL_ERROR_LOCATION,
                                         "Expected cpl.core.Window or tuple.");
    }
#endif
    try {
      int tuple_size =
          py::module::import("builtins").attr("len")(source).cast<int>();
      py::object call_getitem = source.attr("__getitem__");

      if (tuple_size == 4) {
        value = {static_cast<cpl_size>(call_getitem(0).cast<int>()),
                 static_cast<cpl_size>(call_getitem(1).cast<int>()),
                 static_cast<cpl_size>(call_getitem(2).cast<int>()),
                 static_cast<cpl_size>(call_getitem(3).cast<int>())};
        return true;
      } else {
        return false;
      }
    }
    catch (py::error_already_set& err) {
      return false;
    }
  }

  static handle cast(hdrl::core::pycpl_window src, return_value_policy, handle)
  {
    return py::make_tuple(src.llx, src.lly, src.urx, src.ury).release();
  }
};

}  // namespace detail
}  // namespace pybind11

#endif  // PYHDRL_CORE_PYCPL_WINDOW_HPP_
