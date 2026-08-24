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

#ifndef PYHDRL_CORE_PYCPL_WCS_HPP_
#define PYHDRL_CORE_PYCPL_WCS_HPP_

#include <cstring>
#include <string>

#include <cpl_memory.h>
#include <cpl_propertylist.h>
#include <cpl_type.h>
#include <cpl_wcs.h>
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
struct type_caster<hdrl::core::pycpl_wcs>
{
  /**
   * This macro establishes the name 'hdrl::core::pycpl_wcs' in
   * function signatures and declares a local variable
   * 'value' of type hdrl::core::pycpl_wcs
   */

  PYBIND11_TYPE_CASTER(hdrl::core::pycpl_wcs, _("cpl.drs.WCS"));

  /**
   * Conversion part 1 (Python->C++): convert a PyObject into a
   * hdrl::core::pycpl_wcs instance or return false upon failure. The second
   * argument indicates whether implicit conversions should be applied.
   */
  bool load(handle src, bool /* conversion */)
  {
    // Extract PyObject from handle
    // Borrowed being true means the refcount is still OK after this scope ends
    py::object source = reinterpret_borrow<py::object>(src);
    py::module_ pycpl_drs = py::module_::import("cpl.drs");

    // Allow for None to result in a null ptr (cpl_wcs* pl = NULL)
    if (!source || source.is(py::none())) {  // Python None objects
      value.w = nullptr;
      return true;
    }
    // If the object type is not cpl.drs.WCS, throw an error
    if (!py::isinstance(source, pycpl_drs.attr("WCS"))) {
      value.w = nullptr;
      throw hdrl::core::InvalidTypeError(HDRL_ERROR_LOCATION,
                                         "Expected cpl.drs.WCS type");
    }

    try {
      py::object obj = source.attr("_handle");
      if ((obj && !obj.is_none()) && py::isinstance<py::capsule>(obj)) {
        py::capsule wcs = obj.cast<py::capsule>();
        if (strncmp(wcs.name(), "cpl_native_wcs_info", 20) == 0) {
          value.w = cpl_wcs_duplicate(wcs.get_pointer<cpl_wcs>());
          return true;
        } else {
          return false;
        }
      } else {
        return false;
      }
    }
    catch (const py::error_already_set& /*unused */) {
      return false;
    }
  }

  /**
   * Conversion part 2 (C++ -> Python): convert an hdrl::core::pycpl_wcs
   * instance into a Python object. The second and third arguments are used to
   * indicate the return value policy and parent object (for
   * ``return_value_policy::reference_internal``) and are generally
   * ignored by implicit casters.
   */
  static handle cast(hdrl::core::pycpl_wcs src, return_value_policy, handle)
  {
    // if the pointer is null, return a None object
    if (src.w == nullptr) {
      return py::none();
    }

    // The input cpl_wcs object to convert to Python. First we must convert it
    // to a propertylist.
    cpl_wcs* input = src.w;

    // An empty propertylist for pyhdrl_wcs_to_propertylist
    cpl_propertylist* pl = cpl_propertylist_new();

    // pyhdrl_wcs_to_propertylist is a copy of hdrl_wcs_to_propertylist taken
    // from HDRL
    cpl_error_code err =
        hdrl::core::pyhdrl_wcs_to_propertylist(input, pl, CPL_FALSE);

    // convert the propertylist to a pycpl_propertylist object so we can use it
    // with a py::object
    hdrl::core::pycpl_propertylist new_pl = hdrl::core::pycpl_propertylist(pl);

    // Create a new cpl.drs.WCS object and fill with contents of the cpl_wcs*
    // input
    py::module_ pycpl_drs = py::module_::import("cpl.drs");
    py::object new_wcs = pycpl_drs.attr("WCS")(new_pl);
    // Return the result
    return new_wcs.release();
  }
};

}  // namespace detail
}  // namespace pybind11

#endif  // PYHDRL_CORE_PYCPL_WCS_HPP_