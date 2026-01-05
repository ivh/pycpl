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

#ifndef PYHDRL_CORE_PYCPL_PROPERTYLIST_HPP_
#define PYHDRL_CORE_PYCPL_PROPERTYLIST_HPP_

#include <complex>
#include <string>

#include <cpl_error.h>
#include <cpl_property.h>
#include <cpl_propertylist.h>
#include <cpl_type.h>
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
struct type_caster<hdrl::core::pycpl_propertylist>
{
  /**
   * This macro establishes the name 'hdrl::core::pycpl_propertylist' in
   * function signatures and declares a local variable
   * 'value' of type hdrl::core::pycpl_propertylist
   */

  PYBIND11_TYPE_CASTER(hdrl::core::pycpl_propertylist,
                       _("cpl.core.PropertyList"));

  /**
   * Conversion part 1 (Python->C++): convert a PyObject into a
   * hdrl::core::pycpl_propertylist instance or return false upon failure. The
   * second argument indicates whether implicit conversions should be applied.
   */
  bool load(handle src, bool /* conversion */)
  {
    // Extract PyObject from handle
    // Borrowed being true means the refcount is still OK after this scope ends
    py::object source = reinterpret_borrow<py::object>(src);
    py::module_ pycpl_core = py::module_::import("cpl.core");

    // Allow for None to result in a null ptr (cpl_propertylist* pl = NULL)
    if (!source || source.is(py::none())) {  // Python None objects
      value.pl = nullptr;
      return true;
    }
    // If the object type is not cpl.core.Image, throw an error
    if (!py::isinstance(source, pycpl_core.attr("PropertyList"))) {
      value.pl = nullptr;
      throw hdrl::core::InvalidTypeError(HDRL_ERROR_LOCATION,
                                         "Expected cpl.core.PropertyList type");
    }
    // Create a new cpl_image with the contents of the cpl.core.Image object
    try {
      // int n = source.attr("size").cast<int>();
      cpl_propertylist* new_pl = cpl_propertylist_new();
      py::object call_getitem = source.attr("__getitem__");
      int list_size =
          py::module::import("builtins").attr("len")(source).cast<int>();
      for (int i = 0; i < list_size; i++) {
        py::object prop = call_getitem(i);
        cpl_type ptype = prop.attr("type").cast<cpl_type>();
        std::string pname = prop.attr("name").cast<std::string>();
        // Make the minimum property
        cpl_property* p = cpl_property_new(pname.c_str(), ptype);

        py::object comment = prop.attr("comment");
        if (!comment.is(py::none())) {
          // If we have a comment, add it to the property
          std::string pcomment = comment.cast<std::string>();
          cpl_property_set_comment(p, pcomment.c_str());
        }
        py::object pvalue = prop.attr("value");
        if (!pvalue.is(py::none())) {
          switch (ptype) {
            case CPL_TYPE_STRING: {
              cpl_property_set_string(p, (pvalue.cast<std::string>()).c_str());
              break;
            }
            case CPL_TYPE_CHAR: {
              cpl_property_set_char(p, pvalue.cast<char>());
              break;
            }
            case CPL_TYPE_BOOL: {
              cpl_property_set_bool(p, pvalue.cast<bool>());
              break;
            }
            case CPL_TYPE_INT: {
              cpl_property_set_int(p, pvalue.cast<int>());
              break;
            }
            case CPL_TYPE_LONG: {
              cpl_property_set_long(p, pvalue.cast<long>());
              break;
            }
            case CPL_TYPE_LONG_LONG: {
              cpl_property_set_long_long(p, pvalue.cast<long long>());
              break;
            }
            case CPL_TYPE_FLOAT: {
              cpl_property_set_float(p, pvalue.cast<float>());
              break;
            }
            case CPL_TYPE_DOUBLE: {
              cpl_property_set_double(p, pvalue.cast<double>());
              break;
            }
            case CPL_TYPE_FLOAT_COMPLEX: {
              std::complex<float> cval = pvalue.cast<std::complex<float>>();
              float _Complex z;
              reinterpret_cast<double (&)[2]>(z)[0] = cval.real();
              reinterpret_cast<double (&)[2]>(z)[1] = cval.imag();
              cpl_property_set_float_complex(p, z);
              break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX: {
              std::complex<double> cval = pvalue.cast<std::complex<double>>();
              double _Complex z;
              reinterpret_cast<double (&)[2]>(z)[0] = cval.real();
              reinterpret_cast<double (&)[2]>(z)[1] = cval.imag();
              cpl_property_set_double_complex(p, z);
              break;
            }
            default: {
              throw hdrl::core::InvalidTypeError(
                  HDRL_ERROR_LOCATION,
                  "Unsupported cpl.core.Property type found in property list.");
              break;
            }
          }
        }
        cpl_propertylist_append_property(new_pl, p);
      }

      value.pl = cpl_propertylist_duplicate(new_pl);
      cpl_error_reset();
      return true;
    }
    catch (py::error_already_set& err) {
      return false;
    }
  }

  /**
   * Conversion part 2 (C++ -> Python): convert an
   * hdrl::core::pycpl_propertylist instance into a Python object. The second
   * and third arguments are used to indicate the return value policy and parent
   * object (for
   * ``return_value_policy::reference_internal``) and are generally
   * ignored by implicit casters.
   */
  static handle
  cast(hdrl::core::pycpl_propertylist src, return_value_policy, handle)
  {
    // If the pointer is null, return a None object
    if (src.pl == nullptr) {
      return py::none();
    }

    cpl_propertylist* input = src.pl;
    // Create a new cpl.core.PropertyList object and fill with contents of the
    // cpl_propertylist* input
    py::module_ pycpl_core = py::module_::import("cpl.core");
    int n = cpl_propertylist_get_size(input);
    py::object new_plist = pycpl_core.attr("PropertyList")();
    for (int i = 0; i < n; i++) {
      cpl_error_reset();
      cpl_property* prop = cpl_propertylist_get(input, i);
      cpl_type ptype = cpl_property_get_type(prop);
      const char* pname = cpl_property_get_name(prop);
      py::object pycpl_type;
      py::object new_prop;
      switch (ptype) {
        case CPL_TYPE_STRING: {
          pycpl_type = pycpl_core.attr("Type").attr("STRING");
          new_prop = pycpl_core.attr("Property")(pname, pycpl_type);
          const char* val = cpl_property_get_string(prop);
          const char* com = cpl_property_get_comment(prop);
          if (val != nullptr) {
            new_prop.attr("value") = val;
          }
          if (com != nullptr) {
            new_prop.attr("comment") = com;
          }
          break;
        }
        case CPL_TYPE_CHAR: {
          pycpl_type = pycpl_core.attr("Type").attr("CHAR");
          new_prop = pycpl_core.attr("Property")(pname, pycpl_type);
          char val = cpl_property_get_char(prop);
          const char* com = cpl_property_get_comment(prop);
          if (cpl_error_get_code() == CPL_ERROR_NONE) {
            new_prop.attr("value") = val;
          }
          if (com != nullptr) {
            new_prop.attr("comment") = com;
          }
          break;
        }
        case CPL_TYPE_BOOL: {
          pycpl_type = pycpl_core.attr("Type").attr("BOOL");
          new_prop = pycpl_core.attr("Property")(pname, pycpl_type);
          int val = cpl_property_get_bool(prop);
          const char* com = cpl_property_get_comment(prop);
          if (cpl_error_get_code() == CPL_ERROR_NONE) {
            new_prop.attr("value") = val;
          }
          if (com != nullptr) {
            new_prop.attr("comment") = com;
          }
          break;
        }
        case CPL_TYPE_INT: {
          pycpl_type = pycpl_core.attr("Type").attr("INT");
          new_prop = pycpl_core.attr("Property")(pname, pycpl_type);
          int val = cpl_property_get_int(prop);
          const char* com = cpl_property_get_comment(prop);
          if (cpl_error_get_code() == CPL_ERROR_NONE) {
            new_prop.attr("value") = val;
          }
          if (com != nullptr) {
            new_prop.attr("comment") = com;
          }
          break;
        }
        case CPL_TYPE_LONG: {
          pycpl_type = pycpl_core.attr("Type").attr("LONG");
          new_prop = pycpl_core.attr("Property")(pname, pycpl_type);
          long val = cpl_property_get_long(prop);
          const char* com = cpl_property_get_comment(prop);
          if (cpl_error_get_code() == CPL_ERROR_NONE) {
            new_prop.attr("value") = val;
          }
          if (com != nullptr) {
            new_prop.attr("comment") = com;
          }
          break;
        }
        case CPL_TYPE_LONG_LONG: {
          pycpl_type = pycpl_core.attr("Type").attr("LONG_LONG");
          new_prop = pycpl_core.attr("Property")(pname, pycpl_type);
          long long val = cpl_property_get_long_long(prop);
          const char* com = cpl_property_get_comment(prop);
          if (cpl_error_get_code() == CPL_ERROR_NONE) {
            new_prop.attr("value") = val;
          }
          if (com != nullptr) {
            new_prop.attr("comment") = com;
          }
          break;
        }
        case CPL_TYPE_FLOAT: {
          pycpl_type = pycpl_core.attr("Type").attr("FLOAT");
          new_prop = pycpl_core.attr("Property")(pname, pycpl_type);
          float val = cpl_property_get_float(prop);
          const char* com = cpl_property_get_comment(prop);
          if (cpl_error_get_code() == CPL_ERROR_NONE) {
            new_prop.attr("value") = val;
          }
          if (com != nullptr) {
            new_prop.attr("comment") = com;
          }
          break;
        }
        case CPL_TYPE_DOUBLE: {
          pycpl_type = pycpl_core.attr("Type").attr("DOUBLE");
          new_prop = pycpl_core.attr("Property")(pname, pycpl_type);
          double val = cpl_property_get_double(prop);
          const char* com = cpl_property_get_comment(prop);
          if (cpl_error_get_code() == CPL_ERROR_NONE) {
            new_prop.attr("value") = val;
          }
          if (com != nullptr) {
            new_prop.attr("comment") = com;
          }
          break;
        }
        case CPL_TYPE_FLOAT_COMPLEX: {
          pycpl_type = pycpl_core.attr("Type").attr("FLOAT_COMPLEX");
          new_prop = pycpl_core.attr("Property")(pname, pycpl_type);
          float _Complex val = cpl_property_get_float_complex(prop);
          const char* com = cpl_property_get_comment(prop);
          if (cpl_error_get_code() == CPL_ERROR_NONE) {
            float (&z)[2] = reinterpret_cast<float (&)[2]>(val);
            new_prop.attr("value") = std::complex<float>(z[0], z[1]);
            ;
          }
          if (com != nullptr) {
            new_prop.attr("comment") = com;
          }
          break;
        }
        case CPL_TYPE_DOUBLE_COMPLEX: {
          pycpl_type = pycpl_core.attr("Type").attr("DOUBLE_COMPLEX");
          new_prop = pycpl_core.attr("Property")(pname, pycpl_type);
          double _Complex val = cpl_property_get_double_complex(prop);
          const char* com = cpl_property_get_comment(prop);
          if (cpl_error_get_code() == CPL_ERROR_NONE) {
            double (&z)[2] = reinterpret_cast<double (&)[2]>(val);
            new_prop.attr("value") = std::complex<double>(z[0], z[1]);
            ;
          }
          if (com != nullptr) {
            new_prop.attr("comment") = com;
          }
          break;
        }
        default: {
          throw hdrl::core::InvalidTypeError(
              HDRL_ERROR_LOCATION,
              "Unsupported cpl.core.Property type found in property list.");
          break;
        }
      }
      // Append to the list
      new_plist.attr("append")(new_prop);
    }
    cpl_error_reset();
    return new_plist.release();
  }
};

}  // namespace detail
}  // namespace pybind11

#endif  // PYHDRL_CORE_PYCPL_PROPERTYLIST_HPP_