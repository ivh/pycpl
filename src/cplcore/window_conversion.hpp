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

#ifndef PYCPL_WINDOW_CONVERSION_HPP
#define PYCPL_WINDOW_CONVERSION_HPP

// Keep pybind11.h the first include, see pybind11 documentation for details:
// https://pybind11.readthedocs.io/en/stable/basics.html#header-and-namespace-conventions
#include <pybind11/pybind11.h>

#include "cplcore/coords.hpp"
#include "cplcore/types.hpp"

namespace py = pybind11;

namespace pybind11
{
namespace detail
{

template <>
struct type_caster<cpl::core::Window>
{
  /**
   * This macro establishes the name 'cpl::core::Window' in
   * function signatures and declares a local variable
   * 'value' of type cpl::core::Window
   */
  PYBIND11_TYPE_CASTER(cpl::core::Window, _("tuple"));

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

    // Python None objects
    if (!source || source.is(py::none())) {
      value = cpl::core::Window::All;
      return true;
    }

    try {
      size_t tuple_size =
          py::module::import("builtins").attr("len")(source).cast<size_t>();
      py::object call_getitem = source.attr("__getitem__");

      if (tuple_size == 4) {
        value = {call_getitem(0).cast<cpl::core::size>(),
                 call_getitem(1).cast<cpl::core::size>(),
                 call_getitem(2).cast<cpl::core::size>(),
                 call_getitem(3).cast<cpl::core::size>()};
        return true;
      } else {
        return false;
      }
    }
    catch (const py::error_already_set& /* unused */) {
      return false;
    }
  }

  /**
   * Conversion part 2 (C++ -> Python): convert an cpl::core::Window instance
   * into a Python object. The second and third arguments are used to indicate
   * the return value policy and parent object (for
   * ``return_value_policy::reference_internal``) and are generally
   * ignored by implicit casters.
   */
  static handle cast(cpl::core::Window src, return_value_policy /* policy */,
                     handle /* parent */)
  {
    return py::make_tuple(src.llx, src.lly, src.urx, src.ury).release();
  }
};

}  // namespace detail
}  // namespace pybind11

#endif  // PYCPL_WINDOW_CONVERSION_HPP