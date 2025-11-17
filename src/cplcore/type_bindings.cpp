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

#include "cplcore/type_bindings.hpp"

#include <array>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>

#include "cplcore/error.hpp"

namespace py = pybind11;

void
bind_types(py::module& m)
{
  py::enum_<cpl_type> type_class(m, "Type");

  type_class.value("UNSPECIFIED", CPL_TYPE_UNSPECIFIED)
      .value("CHAR", CPL_TYPE_CHAR)
      .value("UCHAR", CPL_TYPE_UCHAR)
      .value("BOOL", CPL_TYPE_BOOL)
      .value("SHORT", CPL_TYPE_SHORT)
      .value("USHORT", CPL_TYPE_USHORT)
      .value("INT", CPL_TYPE_INT)
      .value("UINT", CPL_TYPE_UINT)
      .value("LONG", CPL_TYPE_LONG)
      .value("ULONG", CPL_TYPE_ULONG)
      .value("LONG_LONG", CPL_TYPE_LONG_LONG)
      .value("SIZE", CPL_TYPE_SIZE)
      .value("FLOAT", CPL_TYPE_FLOAT)
      .value("DOUBLE", CPL_TYPE_DOUBLE)
      .value("FLOAT_COMPLEX", CPL_TYPE_FLOAT_COMPLEX)
      .value("DOUBLE_COMPLEX", CPL_TYPE_DOUBLE_COMPLEX)
      .value("STRING", CPL_TYPE_STRING)
      .value("ARRAY", CPL_TYPE_POINTER);
}

namespace cpl
{
std::optional<cpl_type>
pystruct_type_to_cpl(std::string_view orig_format_descriptor)
{
  // CPL types are always in native alignment/endianness, so these prefix
  // characters specifiying alignment/endianness are ignored:
  std::string_view format_descriptor = orig_format_descriptor;
  while (format_descriptor[0] == '@' || format_descriptor[0] == '=' ||
         format_descriptor[0] == '<' || format_descriptor[0] == '>' ||
         format_descriptor[0] == '!' || format_descriptor[0] == 'x') {
    format_descriptor.remove_prefix(1);
  }

  if (format_descriptor == "h") {
    cpl_type ty = CPL_TYPE_SHORT;
    return ty;
  } else if (format_descriptor == "H") {
    cpl_type ty = CPL_TYPE_USHORT;
    return ty;
  } else if (format_descriptor == "i") {
    cpl_type ty = CPL_TYPE_INT;
    return ty;
  } else if (format_descriptor == "I") {
    cpl_type ty = CPL_TYPE_UINT;
    return ty;
  } else if (format_descriptor == "l") {
    cpl_type ty = CPL_TYPE_LONG;
    return ty;
  } else if (format_descriptor == "L") {
    cpl_type ty = CPL_TYPE_ULONG;
    return ty;
  } else if (format_descriptor == "q") {
    cpl_type ty = CPL_TYPE_LONG_LONG;
    return ty;
  } else if (format_descriptor == "Q") {
    // unpported: unsigned long long
    throw cpl::core::InvalidTypeError(
        PYCPL_ERROR_LOCATION, "Unsigned long long is not a supported CPL type");
  } else if (format_descriptor == "n") {
    return CPL_TYPE_SIZE;
  } else if (format_descriptor == "N") {
    return CPL_TYPE_SIZE;
  } else if (format_descriptor == "f") {
    cpl_type ty = CPL_TYPE_FLOAT;
    return ty;
  } else if (format_descriptor == "d") {
    cpl_type ty = CPL_TYPE_DOUBLE;
    return ty;
  } else if (format_descriptor == "s") {
    return CPL_TYPE_STRING;
  } else if (format_descriptor == "p") {
    return CPL_TYPE_STRING;
  } else {
    std::ostringstream err_msg;
    err_msg << "Python struct pack of type " << orig_format_descriptor
            << " does not cast into a single CPL type";
    throw cpl::core::InvalidTypeError(PYCPL_ERROR_LOCATION,
                                      err_msg.str().c_str());
  }
}

bool
pystruct_type_is_native(std::string_view format_descriptor)
{
  while (format_descriptor[0] == '@') {
    format_descriptor.remove_prefix(1);
  }

  if (format_descriptor[0] == '=' || format_descriptor[0] == '<' ||
      format_descriptor[0] == '>' || format_descriptor[0] == '!' ||
      format_descriptor[0] == 'x') {
    return false;
  } else {
    // Should only be 1 member of this struct packing declaration.
    return format_descriptor.size() == 1;
  }
}

namespace
{
std::array<std::pair<py::object, cpl_type>, 18>
init_numpy_type_conversions()
{
  py::object numpy_import = py::module::import("numpy");
  auto elem = [&numpy_import](const char* name, cpl_type ty) {
    return std::make_pair(numpy_import.attr(name), ty);
  };
  return {
      // https://numpy.org/doc/stable/user/basics.types.html
      elem("byte", CPL_TYPE_CHAR),
      elem("ubyte", CPL_TYPE_UCHAR),
      elem("bool_", CPL_TYPE_BOOL),
      elem("short", CPL_TYPE_SHORT),
      elem("ushort", CPL_TYPE_USHORT),
      elem("intc", CPL_TYPE_INT),
      elem("uintc", CPL_TYPE_UINT),
      elem("int_", CPL_TYPE_LONG),
      elem("uint", CPL_TYPE_ULONG),
      elem("longlong", CPL_TYPE_LONG_LONG),
      elem("single", CPL_TYPE_FLOAT),
      elem("double", CPL_TYPE_DOUBLE),
      elem("csingle", CPL_TYPE_FLOAT_COMPLEX),
      elem("cdouble", CPL_TYPE_DOUBLE_COMPLEX),
      // Fixed-sized types that correspond to CPL types directly
      // (Since CPL doesn't do fixed-sized uint_t/int_t
      // this is just for floating-points)
      elem("float32", CPL_TYPE_FLOAT),
      elem("float64", CPL_TYPE_DOUBLE),
      elem("complex64", CPL_TYPE_FLOAT_COMPLEX),
      elem("complex128", CPL_TYPE_DOUBLE_COMPLEX),
  };
}
}  // namespace

std::optional<cpl_type>
numpy_type_to_cpl(py::object numpy_type)
{
  const static thread_local std::array<std::pair<py::object, cpl_type>, 18>
      conversions = init_numpy_type_conversions();

  for (auto objtotype : conversions) {
    if (objtotype.first.equal(numpy_type)) {
      return {objtotype.second};
    }
  }
  return {};
}
}  // namespace cpl
