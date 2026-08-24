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

#ifndef PYHDRL_CORE_PYCPL_TABLE_HPP_
#define PYHDRL_CORE_PYCPL_TABLE_HPP_

#include <complex>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <cpl_array.h>
#include <cpl_table.h>
#include <cpl_type.h>
#include <pybind11/complex.h>
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "hdrlcore/error.hpp"
#include "hdrlcore/pycpl_types.hpp"

namespace py = pybind11;

namespace
{
inline cpl_array*
pycpl_array_wrap(int* data, cpl_size length)
{
  return cpl_array_wrap_int(data, length);
}

inline cpl_array*
pycpl_array_wrap(long long* data, cpl_size length)
{
  return cpl_array_wrap_long_long(data, length);
}

inline cpl_array*
pycpl_array_wrap(float* data, cpl_size length)
{
  return cpl_array_wrap_float(data, length);
}

inline cpl_array*
pycpl_array_wrap(double* data, cpl_size length)
{
  return cpl_array_wrap_double(data, length);
}

inline cpl_array*
pycpl_array_wrap(float _Complex* data, cpl_size length)
{
  return cpl_array_wrap_float_complex(data, length);
}

inline cpl_array*
pycpl_array_wrap(double _Complex* data, cpl_size length)
{
  return cpl_array_wrap_double_complex(data, length);
}

inline cpl_array*
pycpl_array_wrap(char** data, cpl_size length)
{
  return cpl_array_wrap_string(data, length);
}

using array_view = std::unique_ptr<cpl_array, decltype(cpl_array_unwrap)*>;

template <typename T>
array_view
make_array_view(py::array array)
{
  py::buffer_info buffer = array.request();
  T* buffer_data = static_cast<T*>(buffer.ptr);
  return array_view(pycpl_array_wrap(buffer_data, buffer.size),
                    cpl_array_unwrap);
}

template <typename T>
T* pycpl_array_get_data(cpl_array* /* unused*/) = delete;

template <>
int*
pycpl_array_get_data(cpl_array* array)
{
  return cpl_array_get_data_int(array);
}

template <>
long long*
pycpl_array_get_data(cpl_array* array)
{
  return cpl_array_get_data_long_long(array);
}

template <>
float*
pycpl_array_get_data(cpl_array* array)
{
  return cpl_array_get_data_float(array);
}

template <>
double*
pycpl_array_get_data(cpl_array* array)
{
  return cpl_array_get_data_double(array);
}

template <>
float _Complex*
pycpl_array_get_data(cpl_array* array)
{
  return cpl_array_get_data_float_complex(array);
}

template <>
double _Complex*
pycpl_array_get_data(cpl_array* array)
{
  return cpl_array_get_data_double_complex(array);
}

template <typename T>
T*
pycpl_buffer_from_array_column(cpl_table* input, const char* cname)
{
  cpl_size nrows = cpl_table_get_nrow(input);
  cpl_size cdepth = cpl_table_get_column_depth(input, cname);
  T* dst_buffer = static_cast<T*>(std::calloc(cdepth * nrows, sizeof(T)));
  for (int irow = 0; irow < nrows; ++irow) {
    bool is_valid = cpl_table_is_valid(input, cname, irow) == 1;
    // Check if a column row is invalid, i.e. trying to get the data
    // array will return a nullptr. For invalid column rows fill the
    // 2D buffer row with value zero. For valid rows copy the array
    // data of the column row to the 2D buffer row.
    if (!is_valid) {
      for (cpl_size aidx = 0; aidx < cdepth; ++aidx) {
        dst_buffer[irow * cdepth + aidx] = T(0);
      }
    } else {
      cpl_array* src_array =
          const_cast<cpl_array*>(cpl_table_get_array(input, cname, irow));
      T* src_buffer = pycpl_array_get_data<T>(src_array);
      std::memcpy(dst_buffer + irow * cdepth, src_buffer, sizeof(T) * cdepth);
    }
  }
  return dst_buffer;
}
}  // namespace

namespace pybind11
{
namespace detail
{
template <>
struct type_caster<hdrl::core::pycpl_table>
{
  /**
   * This macro establishes the name 'hdrl::core::pycpl_table' in
   * function signatures and declares a local variable
   * 'value' of type hdrl::core::pycpl_table
   */

  PYBIND11_TYPE_CASTER(hdrl::core::pycpl_table, _("cpl.core.Table"));

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

    // allow for None to result in a null ptr (cpl_table* im = NULL)
    if (!source || source.is(py::none())) {  // Python None objects
      value.t = nullptr;
      return true;
    }
    // if the object type is not cpl.core.Table, throw an error
    if (!py::isinstance(source, pycpl_core.attr("Table"))) {
      value.t = nullptr;
      throw hdrl::core::InvalidTypeError(HDRL_ERROR_LOCATION,
                                         "Expected cpl.core.Table type");
    }
    // Create a new pycpl_table with the contents of the cpl.core.Table object
    try {
      py::module_ numpy_masked_array = py::module_::import("numpy.ma");

      int nrows =
          py::module::import("builtins").attr("len")(source).cast<int>();
      py::list column_names = source.attr("column_names").cast<py::list>();
      int ncols =
          py::module::import("builtins").attr("len")(column_names).cast<int>();

      // FIXME: Make sure to use table and get rid of the bare cpl_table pointer
      //        alias. This will ensure that allocated memory is properly
      //        cleaned up once pycpl_table gets its missing destructor!
      //        Once the destructor is in place one can also get rid of the
      //        helper table and use the standard typecaster result value
      //        directly!
      hdrl::core::pycpl_table table;
      table.t = cpl_table_new(nrows);

      for (int i = 0; i < ncols; ++i) {
        std::string cname = column_names[i].cast<std::string>();

        // Get column data, column type, and column depth of the
        // current column of the input cpl.core.Table. The
        // column data is returned as a numpy masked array
        py::object cdata =
            source.attr("__getitem__")(cname.c_str()).attr("as_array");
        cpl_type ctype =
            source.attr("get_column_type")(cname.c_str()).cast<cpl_type>();
        int cdepth = source.attr("get_column_depth")(cname.c_str()).cast<int>();

        // Get input column number of dimensions. Ordinary columns are expected
        // to have ndim == 1, while array type columns have ndim == 2. Any
        // different number of dimensions is not supported!
        int ndim = cdata.attr("ndim").cast<int>();

        if ((ndim == 2) && (ctype == CPL_TYPE_POINTER)) {
          py::dtype atype = py::dtype(cdata.attr("dtype"));
          std::optional<cpl_type> array_type =
              hdrl::core::pycpl_numpy_type_to_cpl(atype);
          if (array_type.has_value() && (array_type.value() == CPL_TYPE_LONG)) {
            array_type = CPL_TYPE_LONG_LONG;
          }
          if (!array_type.has_value()) {
            array_type = CPL_TYPE_STRING;
          }

          typedef std::vector<py::object>::size_type array_size;
          switch (array_type.value()) {
            case CPL_TYPE_INT: {
              cpl_table_new_column_array(table.t, cname.c_str(), CPL_TYPE_INT,
                                         cdepth);
              // Transform the column data masked array into an ordinary numpy
              // array. Replacing invalid entries.
              py::array cdata_array =
                  numpy_masked_array.attr("filled")(cdata, 0);
              std::vector<py::object> data_arrays =
                  cdata_array.cast<std::vector<py::object>>();
              for (array_size aidx = 0; aidx < data_arrays.size(); ++aidx) {
                py::array array = data_arrays[aidx];
                array_view data_view = make_array_view<int>(array);
                cpl_table_set_array(table.t, cname.c_str(), aidx,
                                    data_view.get());
              }
              break;
            }
            case CPL_TYPE_LONG:
            case CPL_TYPE_LONG_LONG: {
              cpl_table_new_column_array(table.t, cname.c_str(),
                                         CPL_TYPE_LONG_LONG, cdepth);
              // Transform the column data masked array into an ordinary numpy
              // array. Replacing invalid entries.
              py::array cdata_array =
                  numpy_masked_array.attr("filled")(cdata, 0);
              std::vector<py::object> data_arrays =
                  cdata_array.cast<std::vector<py::object>>();
              for (array_size aidx = 0; aidx < data_arrays.size(); ++aidx) {
                py::array array = data_arrays[aidx];
                array_view data_view = make_array_view<long long>(array);
                cpl_table_set_array(table.t, cname.c_str(), aidx,
                                    data_view.get());
              }
              break;
            }
            case CPL_TYPE_FLOAT: {
              cpl_table_new_column_array(table.t, cname.c_str(), CPL_TYPE_FLOAT,
                                         cdepth);
              // Transform the column data masked array into an ordinary numpy
              // array. Replacing invalid entries.
              py::array cdata_array =
                  numpy_masked_array.attr("filled")(cdata, 0.);
              std::vector<py::object> data_arrays =
                  cdata_array.cast<std::vector<py::object>>();
              for (array_size aidx = 0; aidx < data_arrays.size(); ++aidx) {
                py::array array = data_arrays[aidx];
                array_view data_view = make_array_view<float>(array);
                cpl_table_set_array(table.t, cname.c_str(), aidx,
                                    data_view.get());
              }
              break;
            }
            case CPL_TYPE_DOUBLE: {
              cpl_table_new_column_array(table.t, cname.c_str(),
                                         CPL_TYPE_DOUBLE, cdepth);
              // Transform the column data masked array into an ordinary numpy
              // array. Replacing invalid entries.
              py::array cdata_array =
                  numpy_masked_array.attr("filled")(cdata, 0.);
              std::vector<py::object> arrays =
                  cdata_array.cast<std::vector<py::object>>();
              for (array_size aidx = 0; aidx < arrays.size(); ++aidx) {
                py::array array = arrays[aidx];
                array_view data_view = make_array_view<double>(array);
                cpl_table_set_array(table.t, cname.c_str(), aidx,
                                    data_view.get());
              }
              break;
            }
            case CPL_TYPE_FLOAT_COMPLEX: {
              cpl_table_new_column_array(table.t, cname.c_str(),
                                         CPL_TYPE_FLOAT_COMPLEX, cdepth);
              // Transform the column data masked array into an ordinary numpy
              // array. Replacing invalid entries.
              py::array cdata_array =
                  numpy_masked_array.attr("filled")(cdata, 0.);
              std::vector<py::object> data_arrays =
                  cdata_array.cast<std::vector<py::object>>();
              for (array_size aidx = 0; aidx < data_arrays.size(); ++aidx) {
                py::array array = data_arrays[aidx];
                array_view data_view = make_array_view<float _Complex>(array);
                cpl_table_set_array(table.t, cname.c_str(), aidx,
                                    data_view.get());
              }
              break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX: {
              cpl_table_new_column_array(table.t, cname.c_str(),
                                         CPL_TYPE_DOUBLE_COMPLEX, cdepth);
              // Transform the column data masked array into an ordinary numpy
              // array. Replacing invalid entries.
              py::array cdata_array =
                  numpy_masked_array.attr("filled")(cdata, 0.);
              std::vector<py::object> data_arrays =
                  cdata_array.cast<std::vector<py::object>>();
              for (array_size aidx = 0; aidx < data_arrays.size(); ++aidx) {
                py::array array = data_arrays[aidx];
                array_view data_view = make_array_view<double _Complex>(array);
                cpl_table_set_array(table.t, cname.c_str(), aidx,
                                    data_view.get());
              }
              break;
            }
            case CPL_TYPE_STRING: {
              // String arrays need to be converted explicitly and
              // cannot be handled by calling make_array_view. The reason
              // is the local variable required to store the result
              // of the conversion std::string to char*.
              cpl_table_new_column_array(table.t, cname.c_str(),
                                         CPL_TYPE_STRING, cdepth);
              py::array cdata_array =
                  numpy_masked_array.attr("filled")(cdata, "");
              std::vector<py::object> data_arrays =
                  cdata_array.cast<std::vector<py::object>>();
              for (array_size aidx = 0; aidx < data_arrays.size(); ++aidx) {
                py::array array = data_arrays[aidx];
                std::vector<std::string> slist =
                    array.cast<std::vector<std::string>>();
                std::vector<char*> strings;
                strings.reserve(slist.size());
                std::transform(
                    slist.begin(), slist.end(), std::back_inserter(strings),
                    [](std::string& s) -> char* { return (char*)s.data(); });
                cpl_array* data_view =
                    cpl_array_wrap_string(strings.data(), strings.size());
                cpl_table_set_array(table.t, cname.c_str(), aidx, data_view);
                cpl_array_unwrap(data_view);
              }
              break;
            }
            default: {
              throw hdrl::core::InvalidTypeError(
                  HDRL_ERROR_LOCATION,
                  "Unsupported cpl.core.Table array data type");
              break;
            }
          }
        } else if (ndim == 1) {
          cpl_table_new_column(table.t, cname.c_str(), ctype);
          switch (ctype) {
            case CPL_TYPE_INT: {
              py::array arr = numpy_masked_array.attr("filled")(cdata, 0);
              py::buffer_info buf = arr.request();
              int* data = static_cast<int*>(buf.ptr);
              // cpl_table_wrap_int(table.t,data,cname);
              for (int j = 0; j < nrows; j++) {
                cpl_table_set_int(table.t, cname.c_str(), j, data[j]);
              }
              break;
            }

            case CPL_TYPE_LONG:
            case CPL_TYPE_LONG_LONG: {
              py::array arr = numpy_masked_array.attr("filled")(cdata, 0);
              py::buffer_info buf = arr.request();
              long long* data = static_cast<long long*>(buf.ptr);
              for (int j = 0; j < nrows; j++) {
                cpl_table_set_long_long(table.t, cname.c_str(), j, data[j]);
              }
              break;
            }
            case CPL_TYPE_FLOAT: {
              py::array arr = numpy_masked_array.attr("filled")(cdata, 0.0);
              py::buffer_info buf = arr.request();
              float* data = static_cast<float*>(buf.ptr);
              // cpl_table_wrap_float(table.t,data,cname);
              for (int j = 0; j < nrows; j++) {
                cpl_table_set_float(table.t, cname.c_str(), j, data[j]);
              }
              break;
            }
            case CPL_TYPE_DOUBLE: {
              py::array arr = numpy_masked_array.attr("filled")(cdata, 0.0);
              py::buffer_info buf = arr.request();
              double* data = static_cast<double*>(buf.ptr);
              // cpl_table_wrap_double(table.t,data,cname);
              for (int j = 0; j < nrows; j++) {
                cpl_table_set_double(table.t, cname.c_str(), j, data[j]);
              }
              break;
            }
            case CPL_TYPE_FLOAT_COMPLEX: {
              py::array arr = numpy_masked_array.attr("filled")(cdata, 0.0);
              py::buffer_info buf = arr.request();
              float _Complex* data = static_cast<float _Complex*>(buf.ptr);
              // cpl_table_wrap_float_complex(table.t,data,cname);
              for (int j = 0; j < nrows; j++) {
                cpl_table_set_float_complex(table.t, cname.c_str(), j, data[j]);
              }
              break;
            }
            case CPL_TYPE_DOUBLE_COMPLEX: {
              py::array arr = numpy_masked_array.attr("filled")(cdata, 0.0);
              py::buffer_info buf = arr.request();
              double _Complex* data = static_cast<double _Complex*>(buf.ptr);
              // cpl_table_wrap_double_complex(table.t,data,cname);
              for (int j = 0; j < nrows; j++) {
                cpl_table_set_double_complex(table.t, cname.c_str(), j,
                                             data[j]);
              }
              break;
            }
            case CPL_TYPE_STRING: {
              std::vector<std::string> vals =
                  numpy_masked_array.attr("filled")(cdata, "")
                      .attr("tolist")()
                      .cast<std::vector<std::string>>();
              for (int j = 0; j < nrows; j++) {
                std::string sval = vals[j];
                cpl_table_set_string(table.t, cname.c_str(), j, sval.c_str());
              }
              break;
            }
            default:
              throw hdrl::core::InvalidTypeError(
                  HDRL_ERROR_LOCATION, "Unsupported cpl.core.Table data type");
              break;
          }  // end switch
        } else {
          // Safety net! Should never be reached!
          std::ostringstream msg;
          msg << "Unsupported cpl.core.Table column type (type code: " << ctype
              << ", dimensions: " << ndim << ") in column '" << cname << "'";
          throw hdrl::core::InvalidTypeError(HDRL_ERROR_LOCATION,
                                             msg.str().c_str());
          break;
        }
        // for non-array columns
        // if the masked array has masked values that
        // need to be set as invalid elements
        bool is_masked =
            numpy_masked_array.attr("is_masked")(cdata).cast<bool>();
        if (is_masked) {
          if (ndim == 2) {
            typedef std::vector<py::object>::size_type array_size;

            // since the mask is also 2D, we need to handle it carefully
            std::vector<py::object> masks =
                cdata.attr("mask").cast<std::vector<py::object>>();
            for (array_size aidx = 0; aidx < masks.size(); ++aidx) {
              std::vector<bool> masked =
                  masks[aidx].attr("tolist")().cast<std::vector<bool>>();
              // For array column values each array element is flagged as masked
              // (invalid). Therefore when checking if a table element is
              // invalid it is sufficient to check only the mask value
              // corresponding to the first array element
              if (masked[0]) {
                cpl_table_set_invalid(table.t, cname.c_str(), aidx);
              }
            }
          } else {
            std::vector<bool> masked =
                cdata.attr("mask").attr("tolist")().cast<std::vector<bool>>();
            for (int row = 0; row < nrows; ++row) {
              if (masked[row]) {
                cpl_table_set_invalid(table.t, cname.c_str(), row);
              }
            }
          }
        }
      }
      // Move the newly constructed table (but see FIXME above!!!)
      value.t = table.t;
      table.t = nullptr;
      return true;
    }
    catch (py::error_already_set& err) {
      return false;
    }
  }

  /**
   * Conversion part 2 (C++ -> Python): convert an hdrl::core::pycpl_table
   * instance into a Python object. The second and third arguments are used to
   * indicate the return value policy and parent object (for
   * ``return_value_policy::reference_internal``) and are generally
   * ignored by implicit casters.
   */
  static handle cast(hdrl::core::pycpl_table src, return_value_policy, handle)

  {
    // If the pointer is null, return a None object
    if (src.t == nullptr) {
      return py::none();
    }

    cpl_table* input = src.t;
    int nrows = cpl_table_get_nrow(input);
    int ncols = cpl_table_get_ncol(input);
    cpl_array* column_names = cpl_table_get_column_names(input);

    // Create a new cpl.core.Table object and fill with contents of the
    // cpl_table*
    py::module_ pycpl_core = py::module_::import("cpl.core");
    py::object new_table = pycpl_core.attr("Table").attr("empty")(nrows);
    py::object call_setitem = new_table.attr("__setitem__");
    for (int cidx = 0; cidx < ncols; ++cidx) {
      const char* cname = cpl_array_get_string(column_names, cidx);
      cpl_size cdepth = cpl_table_get_column_depth(input, cname);
      cpl_type ctype = cpl_table_get_column_type(input, cname);
      cpl_size count_invalid = cpl_table_count_invalid(input, cname);
      switch (int(ctype)) {
        // First handle the array types.
        // FIXME: Since the call_setitem calls new_column_array eventually,
        //        we cannot use it here as this would result in a
        //        cpl.core.IllegalOutputError as it would be adding the same
        //        column name twice. Maybe use another "__setitem__" overload
        //        instead, or remove this comment!
        case CPL_TYPE_INT | CPL_TYPE_POINTER: {
          int* data = pycpl_buffer_from_array_column<int>(input, cname);
          py::array new_arr(py::buffer_info(
              data,
              sizeof(int),  // itemsize
              py::format_descriptor<int>::format(),
              2,                                     // ndim
              std::vector<cpl_size>{nrows, cdepth},  // shape
              std::vector<cpl_size>{(cpl_size)sizeof(int) * cdepth, sizeof(int)}
              // strides
              ));
          call_setitem(cname, new_arr);
          break;
        }
        case CPL_TYPE_LONG_LONG | CPL_TYPE_POINTER: {
          long long* data =
              pycpl_buffer_from_array_column<long long>(input, cname);
          py::array new_arr(py::buffer_info(
              data,
              sizeof(long long),  // itemsize
              py::format_descriptor<long long>::format(),
              2,                                     // ndim
              std::vector<cpl_size>{nrows, cdepth},  // shape
              std::vector<cpl_size>{(cpl_size)sizeof(long long) * cdepth,
                                    sizeof(long long)}  // strides
              ));
          call_setitem(cname, new_arr);
          break;
        }

        case CPL_TYPE_FLOAT | CPL_TYPE_POINTER: {
          float* data = pycpl_buffer_from_array_column<float>(input, cname);
          py::array new_arr(py::buffer_info(
              data,
              sizeof(float),  // itemsize
              py::format_descriptor<float>::format(),
              2,                                     // ndim
              std::vector<cpl_size>{nrows, cdepth},  // shape
              std::vector<cpl_size>{(cpl_size)sizeof(float) * cdepth,
                                    sizeof(float)}  // strides
              ));
          call_setitem(cname, new_arr);
          break;
        }
        case CPL_TYPE_DOUBLE | CPL_TYPE_POINTER: {
          double* data = pycpl_buffer_from_array_column<double>(input, cname);
          py::array new_arr(py::buffer_info(
              data,
              sizeof(double),  // itemsize
              py::format_descriptor<double>::format(),
              2,                                     // ndim
              std::vector<cpl_size>{nrows, cdepth},  // shape
              std::vector<cpl_size>{(cpl_size)sizeof(double) * cdepth,
                                    sizeof(double)}  // strides
              ));
          call_setitem(cname, new_arr);
          break;
        }
        case CPL_TYPE_FLOAT_COMPLEX | CPL_TYPE_POINTER: {
          float _Complex* data =
              pycpl_buffer_from_array_column<float _Complex>(input, cname);
          py::array new_arr(py::buffer_info(
              data,
              sizeof(std::complex<float>),  // itemsize
              py::format_descriptor<std::complex<float>>::format(),
              2,                                     // ndim
              std::vector<cpl_size>{nrows, cdepth},  // shape
              std::vector<cpl_size>{(cpl_size)sizeof(std::complex<float>) *
                                        cdepth,
                                    sizeof(std::complex<float>)}  // strides
              ));
          call_setitem(cname, new_arr);
          break;
        }
        case CPL_TYPE_DOUBLE_COMPLEX | CPL_TYPE_POINTER: {
          double _Complex* data =
              pycpl_buffer_from_array_column<double _Complex>(input, cname);
          py::array new_arr(py::buffer_info(
              data,
              sizeof(std::complex<double>),  // itemsize
              py::format_descriptor<std::complex<double>>::format(),
              2,                                     // ndim
              std::vector<cpl_size>{nrows, cdepth},  // shape
              std::vector<cpl_size>{(cpl_size)sizeof(std::complex<double>) *
                                        cdepth,
                                    sizeof(std::complex<double>)}  // strides
              ));
          call_setitem(cname, new_arr);
          break;
        }
        case CPL_TYPE_STRING | CPL_TYPE_POINTER: {
          // Due to a bug in PyCPL (np_derived_type is not set to string for
          // 2d arrays), we have to define the column and assign each row as
          // we go... For more info see PIPE-11183
          new_table.attr("new_column_array")(cname, CPL_TYPE_STRING, cdepth);

          for (int irow = 0; irow < nrows; ++irow) {
            std::vector<std::string> slist(cdepth);
            bool is_valid = cpl_table_is_valid(input, cname, irow) == 1;
            // If the column row data is valid, copy the C strings of the
            // column row data array to a vector of std::strings, as there
            // is no format_descriptor for C strings (char *).
            // Invalid column rows are ignored, i.e. their value is undefined
            // as far as this type conversion is concerned!
            if (is_valid) {
              cpl_array* src_array = const_cast<cpl_array*>(
                  cpl_table_get_array(input, cname, irow));
              char** src_data = cpl_array_get_data_string(src_array);
              for (int aidx = 0; aidx < cdepth; ++aidx) {
                if (src_data[aidx] != nullptr) {
                  slist[aidx] = std::string(src_data[aidx]);
                }
                py::array dst_buffer = py::cast((slist));
                new_table.attr("__setitem__")(std::make_tuple(cname, irow),
                                              dst_buffer);
              }
            }
          }
          break;
        }
        case CPL_TYPE_STRING: {
          // There is no format_descriptor for C strings (char*). An explicit
          // conversion from C strings to std::string is needed utilizing
          // a vector of std::strings as intermediate buffer.
          char** src_data = cpl_table_get_data_string(input, cname);
          std::vector<std::string> slist(nrows);
          for (int irow = 0; irow < nrows; ++irow) {
            if (src_data[irow] != nullptr) {
              slist[irow] = std::string(src_data[irow]);
            }
          }
          py::array dst_buffer = py::cast(slist);
          call_setitem(cname, dst_buffer);
          break;
        }
        case CPL_TYPE_INT: {
          py::array dst_buffer = py::array(
              py::buffer_info(cpl_table_get_data_int(input, cname), sizeof(int),
                              py::format_descriptor<int>::format(), 1,
                              std::vector<cpl_size>{nrows},
                              std::vector<cpl_size>{sizeof(int)}));
          call_setitem(cname, dst_buffer);
          break;
        }
        case CPL_TYPE_LONG:
        case CPL_TYPE_LONG_LONG: {
          py::array dst_buffer = py::array(py::buffer_info(
              cpl_table_get_data_long_long(input, cname), sizeof(long long),
              py::format_descriptor<long long>::format(), 1,
              std::vector<cpl_size>{nrows},
              std::vector<cpl_size>{sizeof(long long)}));
          call_setitem(cname, dst_buffer);
          break;
        }
        case CPL_TYPE_FLOAT: {
          py::array dst_buffer = py::array(py::buffer_info(
              cpl_table_get_data_float(input, cname), sizeof(float),
              py::format_descriptor<float>::format(), 1,
              std::vector<cpl_size>{nrows},
              std::vector<cpl_size>{sizeof(float)}));
          call_setitem(cname, dst_buffer);
          break;
        }
        case CPL_TYPE_DOUBLE: {
          py::array dst_buffer = py::array(py::buffer_info(
              cpl_table_get_data_double(input, cname), sizeof(double),
              py::format_descriptor<double>::format(), 1,
              std::vector<cpl_size>{nrows},
              std::vector<cpl_size>{sizeof(double)}));
          call_setitem(cname, dst_buffer);
          break;
        }
        case CPL_TYPE_FLOAT_COMPLEX: {
          py::array dst_buffer = py::array(py::buffer_info(
              cpl_table_get_data_float_complex(input, cname),
              sizeof(std::complex<float>),
              py::format_descriptor<std::complex<float>>::format(), 1,
              std::vector<cpl_size>{nrows},
              std::vector<cpl_size>{sizeof(std::complex<float>)}));
          call_setitem(cname, dst_buffer);
          break;
        }
        case CPL_TYPE_DOUBLE_COMPLEX: {
          py::array dst_buffer = py::array(py::buffer_info(
              cpl_table_get_data_double_complex(input, cname),
              sizeof(std::complex<double>),
              py::format_descriptor<std::complex<double>>::format(), 1,
              std::vector<cpl_size>{nrows},
              std::vector<cpl_size>{sizeof(std::complex<double>)}));
          call_setitem(cname, dst_buffer);
          break;
        }
        default: {
          throw hdrl::core::InvalidTypeError(
              HDRL_ERROR_LOCATION, "Unsupported cpl.core.Table data type");
          break;
        }
      }
      // Handle any invalid rows
      if (count_invalid > 0) {
        for (int irow = 0; irow < nrows; ++irow) {
          if (!cpl_table_is_valid(input, cname, irow)) {
            new_table.attr("set_invalid")(cname, irow);
          }
        }
      }
    }

    return new_table.release();
  }
};

}  // namespace detail
}  // namespace pybind11

#endif  // PYHDRL_CORE_PYCPL_TABLE_HPP_
