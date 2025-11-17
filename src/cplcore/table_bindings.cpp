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

#include "cplcore/table_bindings.hpp"

#include <algorithm>
#include <complex>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <cpl_memory.h>
#include <cpl_table.h>
#include <pybind11/complex.h>
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "cplcore/error.hpp"
#include "cplcore/table.hpp"
#include "cplcore/type_bindings.hpp"
#include "dump_handler.hpp"
#include "path_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive

namespace py = pybind11;

using size = cpl::core::size;
using namespace py::literals;

cpl_array*
py_array_to_cpl(py::object obj)
{
  py::array arr;
  try {
    arr = (py::array)obj;
  }
  catch (const py::cast_error& /* unused */) {
    std::ostringstream err_msg;
    err_msg << "expected numpy compatible type, not"
            << obj.get_type().attr("__name__").cast<std::string>();
    throw cpl::core::InvalidTypeError(PYCPL_ERROR_LOCATION,
                                      err_msg.str().c_str());
  }
  std::optional<cpl_type> np_derived_type = cpl::numpy_type_to_cpl(arr.dtype());

  if (np_derived_type.has_value() && np_derived_type.value() == CPL_TYPE_LONG) {
    np_derived_type = CPL_TYPE_LONG_LONG;
  }
  // if no type was found, try a string type
  if (!np_derived_type.has_value()) {
    np_derived_type = CPL_TYPE_STRING;
  }

  switch (np_derived_type.value()) {
    case CPL_TYPE_STRING: {
      try {
        std::vector<std::string> v = arr.cast<std::vector<std::string>>();
        std::vector<char*> result;
        result.reserve(v.size() + 1);
        std::transform(v.begin(), v.end(), std::back_inserter(result),
                       [](std::string& s) -> char* { return (char*)s.data(); });
        result.push_back(nullptr);
        char** res = result.data();
        // Convert to cpl_array and then add it to the table
        cpl_array* new_arr = cpl_array_wrap_string(res, v.size());
        return cpl_array_duplicate(new_arr);
        break;
      }
      catch (const py::cast_error& /* unused */) {
        throw cpl::core::InvalidTypeError(
            PYCPL_ERROR_LOCATION,
            "Python Array is not of a compatible type. Parsing array as string "
            "type failed.");
        break;
      }
    }
    case CPL_TYPE_INT: {
      py::buffer_info buf = arr.request();
      int* data_ptr = static_cast<int*>(buf.ptr);
      // Convert to cpl_array and then add it to the table
      cpl_array* new_arr = cpl_array_wrap_int(
          data_ptr, buf.size);  // If its not the same depth the size will allow
                                // it to throw on insert
      return new_arr;
      break;
    }
    case CPL_TYPE_FLOAT: {
      py::buffer_info buf = arr.request();
      float* data_ptr = static_cast<float*>(buf.ptr);
      // Convert to cpl_array and then add it to the table
      cpl_array* new_arr = cpl_array_wrap_float(
          data_ptr, buf.size);  // If its not the same depth the size will allow
                                // it to throw on insert
      return new_arr;
      break;
    }
    case CPL_TYPE_DOUBLE: {
      py::buffer_info buf = arr.request();
      double* data_ptr = static_cast<double*>(buf.ptr);
      // Convert to cpl_array and then add it to the table
      cpl_array* new_arr = cpl_array_wrap_double(
          data_ptr, buf.size);  // If its not the same depth the size will allow
                                // it to throw on insert
      return new_arr;
      break;
    }
    case CPL_TYPE_FLOAT_COMPLEX: {
      std::vector<std::complex<float>> as_vec =
          arr.cast<std::vector<std::complex<float>>>();
      float _Complex* data =
          (float _Complex*)cpl_calloc(as_vec.size(), sizeof(float _Complex));
      std::memcpy(data, &as_vec[0], as_vec.size() * sizeof(float _Complex));
      cpl_array* new_arr = cpl_array_wrap_float_complex(
          data, as_vec.size());  // If its not the same depth the size will
                                 // allow it to throw on insert
      return new_arr;
      break;
    }
    case CPL_TYPE_DOUBLE_COMPLEX: {
      std::vector<std::complex<double>> as_vec =
          arr.cast<std::vector<std::complex<double>>>();
      double _Complex* data =
          (double _Complex*)cpl_calloc(as_vec.size(), sizeof(double _Complex));
      std::memcpy(data, &as_vec[0], as_vec.size() * sizeof(double _Complex));
      cpl_array* new_arr = cpl_array_wrap_double_complex(
          data, as_vec.size());  // If its not the same depth the size will
                                 // allow it to throw on insert
      return new_arr;
      break;
    }

    case CPL_TYPE_LONG:
    case CPL_TYPE_LONG_LONG: {
      std::vector<long long> vec_arr = arr.cast<std::vector<long long>>();
      long long* data =
          (long long*)cpl_calloc(vec_arr.size(), sizeof(long long));

      std::memcpy(data, &vec_arr[0], vec_arr.size() * sizeof(long long));
      // Convert to cpl_array and then add it to the table
      cpl_array* new_arr = cpl_array_wrap_long_long(
          data, vec_arr.size());  // If its not the same depth the size will
                                  // allow it to throw on insert
      return new_arr;
      break;
    }
    default: {
      throw cpl::core::InvalidTypeError(
          PYCPL_ERROR_LOCATION, "Python Array is not of a compatible type");
      break;
    }
  }
}
py::array
cpl_array_to_py(const cpl_array* input)
/**
 * This module provides functions to create and user PyCPL array.
 * converts CPL array objects to Python (numpy) arrays
 * A CPL array contains elements of a given type.
 * - cpl.core.Type.INT
 * - cpl.core.Type.FLOAT
 * - cpl.core.Type.DOUBLE
 * - cpl.core.Type.DOUBLE_COMPLEX
 * - cpl.core.Type.FLOAT_COMPLEX
 * - cpl.core.Type.LONG
 * - cpl.core.Type.LONG.LONG
 *
 * @param data : iterable
      A 1d iterable containing in the case of a CPL array

 * @return python array
          Buffer info data consisting of no of directons, size of input array
 and strides

 */
{  // TODO: Return reference rather than copy
   // check pointer is null if yes return none
  if (input == nullptr) {
    return (py::none());
  } else {
    switch (cpl::core::Error::throw_errors_with(cpl_array_get_type, input)) {
      case CPL_TYPE_STRING: {
        size arr_size = cpl_array_get_size(input);
        char** data = cpl_array_get_data_string(const_cast<cpl_array*>(input));
        std::vector<std::string> as_vec(arr_size);
        for (int i = 0; i < arr_size; i++) {
          if (data != nullptr) {
            as_vec[i] = std::string(data[i]);
          } else {
            as_vec[i] = std::string();
          }
        }
        py::array res_arr = py::cast(as_vec);
        return res_arr;
        break;
      }
      case CPL_TYPE_INT: {
        int* data = cpl_array_get_data_int(const_cast<cpl_array*>(input));
        return py::array(py::buffer_info(
            data,
            sizeof(int),  // itemsize
            py::format_descriptor<int>::format(),
            1,                                             // ndim
            std::vector<size>{cpl_array_get_size(input)},  // shape
            std::vector<size>{sizeof(int)}                 // strides
            ));
        break;
      }
      case CPL_TYPE_FLOAT: {
        float* data = cpl_array_get_data_float(const_cast<cpl_array*>(input));
        return py::array(py::buffer_info(
            data,
            sizeof(float),  // itemsize
            py::format_descriptor<float>::format(),
            1,                                                 // ndim
            std::vector<cpl_size>{cpl_array_get_size(input)},  // shape
            std::vector<cpl_size>{sizeof(float)}               // strides
            ));
        break;
      }
      case CPL_TYPE_DOUBLE: {
        double* data = cpl_array_get_data_double(const_cast<cpl_array*>(input));
        return py::array(py::buffer_info(
            data,
            sizeof(double),  // itemsize
            py::format_descriptor<double>::format(),
            1,                                                 // ndim
            std::vector<cpl_size>{cpl_array_get_size(input)},  // shape
            std::vector<cpl_size>{sizeof(double)}              // strides
            ));
        break;
      }
      case CPL_TYPE_FLOAT_COMPLEX: {
        size arr_size = cpl_array_get_size(input);
        float _Complex* data =
            cpl_array_get_data_float_complex(const_cast<cpl_array*>(input));
        std::vector<std::complex<float>> as_vec(arr_size);

        for (int i = 0; i < arr_size; i++) {
          as_vec[i] = cpl::core::complexf_to_cpp(data[i]);
        }
        return py::array(py::buffer_info(
            as_vec.data(),
            sizeof(std::complex<float>),  // itemsize
            py::format_descriptor<std::complex<float>>::format(),
            1,                                              // ndim
            std::vector<size>{arr_size},                    // shape
            std::vector<size>{sizeof(std::complex<float>)}  // strides
            ));
        break;
      }
      case CPL_TYPE_DOUBLE_COMPLEX: {
        size arr_size = cpl_array_get_size(input);
        double _Complex* data =
            cpl_array_get_data_double_complex(const_cast<cpl_array*>(input));
        std::vector<std::complex<double>> as_vec(arr_size);

        for (int i = 0; i < arr_size; i++) {
          as_vec[i] = cpl::core::complexd_to_cpp(data[i]);
        }
        return py::array(py::buffer_info(
            as_vec.data(),
            sizeof(std::complex<double>),  // itemsize
            py::format_descriptor<std::complex<double>>::format(),
            1,                                               // ndim
            std::vector<size>{arr_size},                     // shape
            std::vector<size>{sizeof(std::complex<double>)}  // strides
            ));
        break;
      }

      case CPL_TYPE_LONG:
      case CPL_TYPE_LONG_LONG: {
        long long* data =
            cpl_array_get_data_long_long(const_cast<cpl_array*>(input));
        return py::array(py::buffer_info(
            data,
            sizeof(long long),  // itemsize
            py::format_descriptor<long long>::format(),
            1,                                             // ndim
            std::vector<size>{cpl_array_get_size(input)},  // shape
            std::vector<size>{sizeof(long long)}           // strides
            ));
        break;
      }
      default: {
        throw cpl::core::InvalidTypeError(PYCPL_ERROR_LOCATION,
                                          "Selected column is of invalid type");
        break;
      }
    }
  }
}

void
bind_table(py::module& m)
{
  py::class_<cpl::core::Table, std::shared_ptr<cpl::core::Table>> table(
      m, "Table");

  // https://github.com/pybind/pybind11/pull/2616/
  // This was solved, but not for pybind 2.5.0, so the workaround is done here,
  // too: Running exec and eval on Python 2 and 3 adds `builtins` module under
  // `__builtins__` key to globals if not yet present.
  // Python 3.8 made PyRun_String behave similarly. Let's also do that for
  // older versions, for consistency.
  py::object global = m.attr("__dict__");
  if (!global.contains("__builtins__"))
    global["__builtins__"] = py::module::import("builtins");

  table.doc() = R"(
        This module provides functions to create and user PyCPL tables. 

        A CPL table is made of columns, and a column consists of an array of elements of a given 
        type. 
        
        The following types are supported, 
        - cpl.core.Type.INT 
        - cpl.core.Type.LONG_LONG 
        - cpl.core.Type.FLOAT 
        - cpl.core.Type.DOUBLE
        - cpl.core.Type.DOUBLE_COMPLEX
        - cpl.core.Type.FLOAT_COMPLEX
        - cpl.core.Type.STRING. 
        
        Moreover, it is possible to define columns of arrays, i.e. columns whose elements are arrays 
        of all the basic types listed above. Within the same column all arrays must have the same 
        type and the same length.
        
        A table column is accessed by specifying its name. The ordering of the columns within a table 
        is undefined: a CPL table is not an n-tuple of columns, but just a set of columns. The N elements 
        of a column are counted from 0 to N-1, with element 0 on top. The set of all the table columns 
        elements with the same index constitutes a table row, and table rows are counted according to the 
        same convention. 
        
        It is possible to flag each table row as "selected" or "unselected", and each column element as 
        "valid" or "invalid". Selecting table rows is mainly a way to extract just those table parts 
        fulfilling any given condition, 
        while invalidating column elements is a way to exclude such elements from any computation. A CPL table 
        is created with all rows selected, and a column is created with all elements invalidated.

        New columns can be allocated either by calling the appropriate function (:py:meth:`new_column` for
        regular columns, :py:meth:`new_column_array` for array columns) or setting directly via index
        (however the given array must be of the number of table rows). 
        See __setitem__ docs for more info. 

        Array column elements must all be of the same length.

        New CPL tables can be built from an existing table-like object, or via the `empty` static method.
        See the Parameters section for building from existing data

        Parameters
        ----------
        input : object
            Data used to build the new CPL table object. This data source must only contain objects of
            types compatible with CPL tables

            `input` can be the following types:
            - astropy.table.QTable
            - pandas.Dataframe
            - numpy.recarray

        Raises
        ------
        cpl.core.InvalidTypeError
            If one of the columns in `input` is not a CPL compatible type

        )";

  // We want to use return_value_policy::rreference_internal to return the below
  // TableColumn. However, pybind regular casting of TableColumn to py::handle
  // will use return_value_policy::move, ignoring what we want. See commit
  // cbd16a82 of  pybind11. The conditions when return value policy is not
  // ignored are: Return a pointer (It's not possible for TableColumn to be a
  // plain pointer, except possibly if we were to use cpl_columns) an lvalue
  // reference (again not Feasable because we're trying to move ownership of
  // TableColumn) Or we can make the caster for TableColumn NOT be derived from
  // type_caster_generic (A lot of work) An alternative is also to do this using
  // a python-implemented class:
  py::exec(
      R"(
                        from collections.abc import MutableSequence, Collection
                        import numpy as np
                        class _TableColumn(MutableSequence):
                                '''
                                Provides an accessor to table via column to allow 2d indexing, for example:
                                
                                tableColumn = table["columnName"]

                                where tableColumn is an instance of _TableColumn
                                '''
                                def __init__(self, table, column):
                                        self.table = table
                                        self.column = column
                                
                                def __getitem__(self, index):
                                        return self.table[self.column, index]

                                def __setitem__(self, index, value):
                                        self.table[self.column, index] = value

                                def __delitem__(self, index):
                                        raise ValueError("Cannot delete cells from a column")

                                def __len__(self):
                                        return self.table.shape[0]

                                @property
                                def as_array(self):
                                    return self.__array__()

                                def insert(self, index, value):
                                        raise ValueError("Cannot insert cells into a column")

                                def __array__(self, dtype=None, copy=None):
                                        rval = self.table.column_array(self.column)
                                        return np.ma.MaskedArray(rval[0],mask=rval[1])
                )",
      global);

  py::enum_<cpl_table_select_operator>(table, "Operator")
      .value("NOT_EQUAL_TO", CPL_NOT_EQUAL_TO)
      .value("EQUAL_TO", CPL_EQUAL_TO)
      .value("GREATER_THAN", CPL_GREATER_THAN)
      .value("NOT_GREATER_THAN", CPL_NOT_GREATER_THAN)
      .value("LESS_THAN", CPL_LESS_THAN)
      .value("NOT_LESS_THAN", CPL_NOT_LESS_THAN);

  table
      .def(
          py::init([](py::object input) -> cpl::core::Table* {
            py::object isinstance =
                py::module::import("builtins").attr("isinstance");
            try {
              py::object dataframe_class =
                  py::module::import("pandas").attr("DataFrame");
              if (isinstance(input, dataframe_class)
                      .cast<bool>()) {  // Is a dataframe class

                input =
                    input.attr("to_records")(false);  // Turn into numpy recarry
              }
            }
            catch (const py::error_already_set& ex) {
              if (!ex.matches(PyExc_ModuleNotFoundError)) {
                throw ex;
              }
              // Doesn't have pandas installed, don't do anything
            }
            try {
              py::object table_class =
                  py::module::import("astropy.table").attr("Table");
              if (isinstance(input, table_class)
                      .cast<bool>()) {             // Is a dataframe class
                input = input.attr("as_array")();  // Turn into numpy array
              }
            }
            catch (const py::error_already_set& ex) {
              if (!ex.matches(PyExc_ModuleNotFoundError)) {
                throw ex;
              }
              // Doesn't have pandas installed, don't do anything
            }

            py::buffer buf = input.cast<py::buffer>();
            py::buffer_info info = buf.request();
            // Check shape is ok
            if (info.ndim != 1) {
              std::ostringstream err_msg;
              err_msg << "expected 2-dimensional buffer, not "
                      << info.shape.size() << "-dimensional buffer";

              throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                                 err_msg.str());
            }
            size row_count = info.shape[0];


            cpl::core::Table* new_table = new cpl::core::Table(row_count);
            std::vector<std::string> columns =
                input.attr("dtype")
                    .attr("names")
                    .cast<std::vector<std::string>>();
            for (std::string col_name : columns) {
              try {
                buf = input[col_name.c_str()].cast<py::buffer>();
              }
              catch (const py::cast_error& /* unused */) {
                std::ostringstream err_msg;
                err_msg << "expected numpy array, or implementor of cpython "
                           "buffer protocol, not "
                        << input[col_name.c_str()]
                               .get_type()
                               .attr("__name__")
                               .cast<std::string>();
                throw cpl::core::InvalidTypeError(PYCPL_ERROR_LOCATION,
                                                  err_msg.str().c_str());
              }
              info = buf.request();

              py::object numpy_dtype = input[col_name.c_str()].attr("dtype");

              if (std::optional<cpl_type> np_converted_type =
                      cpl::numpy_type_to_cpl(numpy_dtype)) {
                try {
                  switch (np_converted_type.value()) {
                    case CPL_TYPE_INT: {
                      std::vector<int> vec =
                          input[col_name.c_str()].cast<std::vector<int>>();
                      new_table->wrap_int(vec, col_name);
                      break;
                    }
                    case CPL_TYPE_FLOAT: {
                      std::vector<float> vec =
                          input[col_name.c_str()].cast<std::vector<float>>();
                      new_table->wrap_float(vec, col_name);
                      break;
                    }
                    case CPL_TYPE_DOUBLE: {
                      std::vector<double> vec =
                          input[col_name.c_str()].cast<std::vector<double>>();
                      new_table->wrap_double(vec, col_name);
                      break;
                    }
                    case CPL_TYPE_FLOAT_COMPLEX: {
                      std::vector<std::complex<float>> vec =
                          input[col_name.c_str()]
                              .cast<std::vector<std::complex<float>>>();
                      new_table->wrap_float_complex(vec, col_name);
                      break;
                    }
                    case CPL_TYPE_DOUBLE_COMPLEX: {
                      std::vector<std::complex<double>> vec =
                          input[col_name.c_str()]
                              .cast<std::vector<std::complex<double>>>();
                      new_table->wrap_double_complex(vec, col_name);
                      break;
                    }
                    case CPL_TYPE_LONG:
                    case CPL_TYPE_LONG_LONG: {
                      std::vector<long long> vec =
                          input[col_name.c_str()]
                              .cast<std::vector<long long>>();
                      new_table->wrap_long_long(vec, col_name);
                      break;
                    }
                    default: {
                      // No compatible types
                      std::ostringstream err_msg;
                      err_msg << "Type " << numpy_dtype.cast<std::string>()
                              << " in column " << col_name
                              << " cannot be cast into a CPL Table compatible "
                                 "type (int, float, double, long, long long, "
                                 "string, cpl_array compatible types)";
                      throw cpl::core::InvalidTypeError(PYCPL_ERROR_LOCATION,
                                                        err_msg.str().c_str());
                      break;
                    }
                  }
                }
                catch (const std::exception& column_type_error) {
                  throw column_type_error;
                }
              } else if (numpy_dtype.attr("__eq__")(
                             py::module::import("builtins").attr("object"))) {
                // Could be a string or list type that numpy has labelled an
                // object
                py::object unknown_object = input[col_name.c_str()];

                try {
                  std::vector<std::string> pystrings =
                      unknown_object.cast<std::vector<std::string>>();
                  new_table->wrap_string(pystrings, col_name);
                }
                catch (const py::cast_error& /* unused */) {
                  // Not a string array, try something else
                  try {
                    std::vector<py::object> arrays =
                        unknown_object.cast<std::vector<py::object>>();
                    py::array first = arrays[0];
                    std::optional<cpl_type> np_derived_type =
                        cpl::numpy_type_to_cpl(first.dtype());
                    if (np_derived_type.value() == CPL_TYPE_LONG) {
                      np_derived_type = CPL_TYPE_LONG_LONG;
                    }
                    new_table->new_column_array(
                        col_name, np_derived_type.value(), first.size());

                    switch (np_derived_type.value()) {
                      case CPL_TYPE_INT: {
                        for (int i = 0; i < arrays.size(); i++) {
                          py::array_t<int> as_arr = arrays[i];
                          py::buffer_info array_buffer = as_arr.request();
                          int* data_ptr = static_cast<int*>(array_buffer.ptr);
                          // Convert to cpl_array and then add it to the table
                          cpl_array* new_arr = cpl_array_wrap_int(
                              data_ptr,
                              array_buffer
                                  .size);  // If its not the same depth the size
                                           // will allow it to throw on insert
                          new_table->set_array(col_name.c_str(), i, new_arr);
                        }
                        break;
                      }
                      case CPL_TYPE_FLOAT: {
                        for (int i = 0; i < arrays.size(); i++) {
                          py::array_t<float> as_arr = arrays[i];
                          py::buffer_info array_buffer = as_arr.request();
                          float* data_ptr =
                              static_cast<float*>(array_buffer.ptr);
                          // Convert to cpl_array and then add it to the table
                          cpl_array* new_arr = cpl_array_wrap_float(
                              data_ptr,
                              array_buffer
                                  .size);  // If its not the same depth the size
                                           // will allow it to throw on insert
                          new_table->set_array(col_name.c_str(), i, new_arr);
                        }
                        break;
                      }
                      case CPL_TYPE_DOUBLE: {
                        for (int i = 0; i < arrays.size(); i++) {
                          py::array_t<double> as_arr = arrays[i];
                          py::buffer_info array_buffer = as_arr.request();
                          double* data_ptr =
                              static_cast<double*>(array_buffer.ptr);
                          // Convert to cpl_array and then add it to the table
                          cpl_array* new_arr = cpl_array_wrap_double(
                              data_ptr,
                              array_buffer
                                  .size);  // If its not the same depth the size
                                           // will allow it to throw on insert
                          new_table->set_array(col_name.c_str(), i, new_arr);
                        }
                        break;
                      }
                      case CPL_TYPE_FLOAT_COMPLEX: {
                        for (int i = 0; i < arrays.size(); i++) {
                          std::vector<std::complex<float>> as_vec =
                              arrays[i]
                                  .cast<std::vector<std::complex<float>>>();

                          float _Complex* data = (float _Complex*)cpl_calloc(
                              as_vec.size(), sizeof(float _Complex));
                          std::memcpy(data, &as_vec[0],
                                      as_vec.size() * sizeof(float _Complex));
                          cpl_array* new_arr = cpl_array_wrap_float_complex(
                              data,
                              as_vec.size());  // If its not the same depth the
                                               // size will allow it to throw
                                               // on insert
                          new_table->set_array(col_name.c_str(), i, new_arr);
                        }
                        break;
                      }
                      case CPL_TYPE_DOUBLE_COMPLEX: {
                        for (int i = 0; i < arrays.size(); i++) {
                          std::vector<std::complex<double>> as_vec =
                              arrays[i]
                                  .cast<std::vector<std::complex<double>>>();
                          double _Complex* data = (double _Complex*)cpl_calloc(
                              as_vec.size(), sizeof(double _Complex));
                          std::memcpy(data, &as_vec[0],
                                      as_vec.size() * sizeof(double _Complex));
                          cpl_array* new_arr = cpl_array_wrap_double_complex(
                              data,
                              as_vec.size());  // If its not the same depth the
                                               // size will allow it to throw
                                               // on insert
                          new_table->set_array(col_name.c_str(), i, new_arr);
                        }
                        break;
                      }

                      case CPL_TYPE_LONG:
                      case CPL_TYPE_LONG_LONG: {
                        for (int i = 0; i < arrays.size(); i++) {
                          std::vector<long long> vec_arr =
                              arrays[i].cast<std::vector<long long>>();
                          long long* data = (long long*)cpl_calloc(
                              vec_arr.size(), sizeof(long long));

                          std::memcpy(data, &vec_arr[0],
                                      vec_arr.size() * sizeof(long long));
                          // Convert to cpl_array and then add it to the table
                          cpl_array* new_arr = cpl_array_wrap_long_long(
                              data,
                              vec_arr.size());  // If its not the same depth
                                                // the size will allow it to
                                                // throw on insert
                          new_table->set_array(col_name.c_str(), i, new_arr);
                        }
                        break;
                      }
                      default:
                        throw cpl::core::InvalidTypeError(
                            PYCPL_ERROR_LOCATION,
                            col_name +
                                " is a type that cannot be cast to a "
                                "compatible table column type");
                        break;
                    }
                  }
                  catch (const py::cast_error& /* unused */) {
                    std::ostringstream err_msg;
                    err_msg << "Type " << numpy_dtype.cast<std::string>()
                            << " in column " << col_name
                            << " cannot be cast into a CPL Table compatible "
                               "type (int, float, double, long, long long, "
                               "string, cpl_array compatible types)";
                    throw cpl::core::InvalidTypeError(PYCPL_ERROR_LOCATION,
                                                      err_msg.str().c_str());
                  }
                }
              }
            }
            return new_table;
          }),
          py::arg("input"), R"pydoc(

        )pydoc")
      .def_static(
          "empty",
          [](size rows) -> cpl::core::Table { return cpl::core::Table(rows); },
          py::arg("rows"), R"docstring(
      Construct empty table with a set number of rows

      Parameters
      ----------
      rows : int
          number of rows in the new table

      Returns
      -------
      cpl.core.Table
          New empty Table with `rows` number of rows

      Raises
      ------
      cpl.core.IllegalInputError
          `rows` is negative
      )docstring")
      .def("copy_structure", &cpl::core::Table::copy_structure,
           py::arg("toCopy"), R"pydoc(
        Copy the structure (column names, types and units) from another table

        This function assignes to a columnless table the same column structure
        (names, types, units) of a given model table. All columns are physically
        created in the new table, and they are initialised to contain just
        invalid elements.

        Parameters
        ----------
        toCopy : cpl.core.Table
            table from which the structure is to be copied from.

        Raises
        ------
        cpl.core.IllegalInputError
            if `self` contains columns
        )pydoc")
      .def_property_readonly(
          "shape",
          [](cpl::core::Table& self) -> py::tuple {
            return py::make_tuple(self.get_nrow(), self.get_ncol());
          },
          "Shape of the table in the format (rows, columns)")
      .def("new_column", &cpl::core::Table::new_column, py::arg("name"),
           py::arg("type"), R"pydoc(
        Create an empty column in a table.

        Creates a new column of specified `type`, excluding array types
        (for creating a column of arrays use the function :py:meth:`new_column_array`,
        where the column depth must also be specified).

        The new column name must be different from  any other column name
        in the table. All the elements of the new column are marked as invalid.

        Parameters
        ----------
        name : str
            Name of the new column.
        type : cpl.core.Type
            Type of the new column.

        Raises
        ------
        cpl.core.InvalidTypeError
            `type` is not supported by cpl.core.Table
        cpl.core.IllegalOutputError
            column with the same `name` already exists in the table
        )pydoc")
      .def("new_column_array", &cpl::core::Table::new_column_array,
           py::arg("name"), py::arg("type"), py::arg("depth"), R"pydoc(
        Create an empty column of arrays in a table.

        This creates a new column of specified array length.

        Parameters
        ----------
        name : str
            Name of the new column.
        type : cpl.core.Type
            Type of the new column.
        depth : int
            Depth of the new column.

        Raises
        ------
        cpl.core.InvalidTypeError
            `type` is not supported by cpl.core.Table
        cpl.core.IllegalInputError
            The specified `depth` is negative
        cpl.core.IllegalOutputError
            column with the same `name` already exists in the table
        )pydoc")
      // .def("wrap_int",
      //      &cpl::core::Table::wrap_int)  // TODO: wrap functions require
      //      casting
      //                                    // a list/array, numpy or otherwise,
      //                                    to
      //                                    // a pointer of the appropriate
      //                                    type.
      //                                    // Prehaps combine into single
      //                                    function
      // .def("wrap_long_long", &cpl::core::Table::wrap_long_long)

      .def("__contains__", &cpl::core::Table::has_column)
      .def(
          "get_column_type",
          [](cpl::core::Table& self, std::string& col_name) -> cpl_type {
            cpl_type column_type = self.get_column_type(col_name);
            if ((column_type & CPL_TYPE_POINTER)) {
              return CPL_TYPE_POINTER;
            }
            return column_type;
          },
          py::arg("name"), R"pydoc(
        Get the type of a table column.

        Get the type of a column.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        cpl.core.Type
            The column type of `name`

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` not found in table.

        )pydoc")
      .def("get_column_depth", &cpl::core::Table::get_column_depth,
           py::arg("name"), R"pydoc(
        Get the depth of a table column.

        Get the depth of a column. Columns of type array always have positive
        depth, while columns listing numbers or character strings have depth 0.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        int
            Column depth

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` not found in table.
        )pydoc")

      .def_property_readonly("column_names",
                             &cpl::core::Table::get_column_names,
                             "list of all column names")
      // Single value settings
      .def(
          "__setitem__",
          [](cpl::core::Table& self, std::pair<std::string, py::slice> location,
             double setting) -> void {
            size_t start, stop, step, slicelength;
            location.second.compute(self.get_nrow(), &start, &stop, &step,
                                    &slicelength);
            self.fill_column_window(location.first, start, stop - start,
                                    setting);
          },
          py::arg("location"), py::arg("setting"),
          R"(
        Fill column with double value in slice range. Location in format [column_name, start_index:stop_index]. Column type must be numerical.
        Example:

        t["double_col", 1:4]= 4.5

        Slice must be within Table range (0<=start_index<stop_index<= len(t))
                )")
      .def(
          "__setitem__",
          [](cpl::core::Table& self, std::pair<std::string, py::slice> location,
             long long setting) -> void {
            size_t start, stop, step, slicelength;
            location.second.compute(self.get_nrow(), &start, &stop, &step,
                                    &slicelength);
            self.fill_column_window(location.first, start, stop - start,
                                    setting);
          },
          py::arg("location"), py::arg("setting"),
          R"pydoc(
        Fill column with long long value in slice range. Location in format [column_name, start_index:stop_index]. Column type must be numerical.
        Example:

        t["long_long_col", 1:4]= 4555

        Slice must be within Table range (0<=start_index<stop_index<= len(t))
        )pydoc")
      .def(
          "__setitem__",
          [](cpl::core::Table& self, std::pair<std::string, py::slice> location,
             std::complex<double> setting) -> void {
            size_t start, stop, step, slicelength;
            location.second.compute(self.get_nrow(), &start, &stop, &step,
                                    &slicelength);
            self.fill_column_window_complex(location.first, start, stop - start,
                                            setting);
          },
          py::arg("location"), py::arg("setting"),
          R"pydoc(
        Fill column with double complex value in slice range. Location in format [column_name, start_index:stop_index]. Column must a complex type.
        Example:

        t["double_complex_col", 1:4]= 4.0+52.0j

        Slice must be within Table range (0<=start_index<stop_index<= len(t))
        )pydoc")
      .def(
          "__setitem__",
          [](cpl::core::Table& self, std::pair<std::string, py::slice> location,
             std::string setting) -> void {
            size_t start, stop, step, slicelength;
            location.second.compute(self.get_nrow(), &start, &stop, &step,
                                    &slicelength);
            self.fill_column_window_string(location.first, start, stop - start,
                                           setting);
          },
          py::arg("location"), py::arg("setting"),
          R"pydoc(
        Fill column with string value in slice range. Location in format [column_name, start_index:stop_index]. Column must be of type cpl.core.Type.STRING
        Example:

        t["string_col", 1:4]= "value"

        Slice must be within Table range (0<=start_index<stop_index<= len(t))
        )pydoc")
      .def(
          "__setitem__",
          [](cpl::core::Table& self, std::pair<std::string, py::slice> location,
             py::object setting) -> void {
            size_t start, stop, step, slicelength;
            location.second.compute(self.get_nrow(), &start, &stop, &step,
                                    &slicelength);

            cpl_array* to_set = py_array_to_cpl(setting);
            self.fill_column_window_array(location.first, start, stop - start,
                                          to_set);
            cpl_array_delete(to_set);
          },
          py::arg("location"), py::arg("setting"),
          R"pydoc(Fill column with array value in slice range. Location in format [column_name, start_index:stop_index]. Must be an array column
        Example:

        t["array_col", 1:4]= [1,2,3]

        Slice must be within Table range (0<=start_index<stop_index<= len(t))
        )pydoc")
      .def(
          "__setitem__",
          [](cpl::core::Table& self, std::pair<std::string, int> location,
             double setting) -> void {
            self.set(location.first, location.second, setting);
          },
          py::arg("location"), py::arg("setting"),
          "Set double value at location [column_name,row]. Column must be of a "
          "numerical type.")
      .def(
          "__setitem__",
          [](cpl::core::Table& self, std::pair<std::string, int> location,
             long long setting) -> void {
            self.set(location.first, location.second, setting);
          },
          py::arg("location"), py::arg("setting"),
          "Set long long value at location [column_name,row]. Column must be "
          "of a numerical type.")
      .def(
          "__setitem__",
          [](cpl::core::Table& self, std::pair<std::string, int> location,
             std::complex<double> setting) -> void {
            self.set_complex(location.first, location.second, setting);
          },
          py::arg("location"), py::arg("setting"),
          "Set double complex value at location [column_name,row]. Column must "
          "be of a complex type.")
      .def(
          "__setitem__",
          [](cpl::core::Table& self, std::pair<std::string, int> location,
             std::string setting) -> void {
            self.set_string(location.first, location.second, setting);
          },
          py::arg("location"), py::arg("setting"),
          "Set string value at location [column_name,row]. Column must be of "
          "cpl.core.Type.STRING.")
      .def(
          "__setitem__",
          [](cpl::core::Table& self, std::pair<std::string, int> location,
             py::object setting) -> void {  // Array column type
            cpl_array* to_set = py_array_to_cpl(setting);
            self.set_array(location.first, location.second, to_set);
          },
          py::arg("location"), py::arg("setting"),
          "Set array at location [column_name,row]. Column must be an array "
          "type.")
      // Allocating whole column
      .def(
          "__setitem__",
          [](cpl::core::Table& self, std::string location,
             py::object setting) -> void {  // Array column type
            py::array arr;
            if (setting.is(py::none())) {
              std::ostringstream err_msg;
              err_msg << "expected array compatible type, not None";
              throw cpl::core::IncompatibleInputError(PYCPL_ERROR_LOCATION,
                                                      err_msg.str().c_str());
            }

            // Check for array-like types to differentiate from scalars
            if (py::isinstance<py::list>(setting) ||
                py::isinstance<py::array>(setting)) {
              // Directly convert to numpy array for further processing
              arr = py::array(setting);
            } else {
              // Scalar detected - raise an error because assigning a scalar
              // directly to an entire column is inappropriate
              if (self.has_column(location)) {
                throw cpl::core::InvalidTypeError(
                    PYCPL_ERROR_LOCATION,
                    "Cannot assign a scalar directly to an entire column. "
                    "Please use a list or numpy array with the same number of "
                    "rows as the table.");
              }
            }

            py::buffer_info info = arr.request();

            if (info.shape[0] != self.get_nrow()) {
              std::ostringstream err_msg;
              err_msg << "Length of values"
                      << "(" << arr.size() << ")"
                      << " does not match table with "
                      << "(" << self.get_nrow() << ")"
                      << " rows";
              throw cpl::core::IncompatibleInputError(PYCPL_ERROR_LOCATION,
                                                      err_msg.str().c_str());
            }
            py::object numpy_dtype = arr.dtype();
            if (info.ndim == 2) {
              try {
                std::vector<py::object> arrays =
                    arr.cast<std::vector<py::object>>();
                py::array first = arrays[0];
                std::optional<cpl_type> np_derived_type =
                    cpl::numpy_type_to_cpl(first.dtype());
                if (np_derived_type.value() == CPL_TYPE_LONG) {
                  np_derived_type = CPL_TYPE_LONG_LONG;
                }
                self.new_column_array(location, np_derived_type.value(),
                                      first.size());
                switch (np_derived_type.value()) {
                  case CPL_TYPE_INT: {
                    for (int i = 0; i < arrays.size(); i++) {
                      py::array_t<int> as_arr = arrays[i];
                      py::buffer_info buf = as_arr.request();
                      int* data_ptr = static_cast<int*>(buf.ptr);
                      // Convert to cpl_array and then add it to the table
                      cpl_array* new_arr = cpl_array_wrap_int(
                          data_ptr,
                          buf.size);  // If its not the same depth the size will
                                      // allow it to throw on insert
                      self.set_array(location, i, new_arr);
                    }
                    break;
                  }
                  case CPL_TYPE_FLOAT: {
                    for (int i = 0; i < arrays.size(); i++) {
                      py::array_t<float> as_arr = arrays[i];
                      py::buffer_info buf = as_arr.request();
                      float* data_ptr = static_cast<float*>(buf.ptr);
                      // Convert to cpl_array and then add it to the table
                      cpl_array* new_arr = cpl_array_wrap_float(
                          data_ptr,
                          buf.size);  // If its not the same depth the size will
                                      // allow it to throw on insert
                      self.set_array(location, i, new_arr);
                    }
                    break;
                  }
                  case CPL_TYPE_DOUBLE: {
                    for (int i = 0; i < arrays.size(); i++) {
                      py::array_t<double> as_arr = arrays[i];
                      py::buffer_info buf = as_arr.request();
                      double* data_ptr = static_cast<double*>(buf.ptr);
                      // Convert to cpl_array and then add it to the table
                      cpl_array* new_arr = cpl_array_wrap_double(
                          data_ptr,
                          buf.size);  // If its not the same depth the size will
                                      // allow it to throw on insert
                      self.set_array(location, i, new_arr);
                    }
                    break;
                  }
                  case CPL_TYPE_FLOAT_COMPLEX: {
                    for (int i = 0; i < arrays.size(); i++) {
                      std::vector<std::complex<float>> as_vec =
                          arrays[i].cast<std::vector<std::complex<float>>>();
                      float _Complex* data = (float _Complex*)cpl_calloc(
                          as_vec.size(), sizeof(float _Complex));
                      std::memcpy(data, &as_vec[0],
                                  as_vec.size() * sizeof(float _Complex));
                      cpl_array* new_arr = cpl_array_wrap_float_complex(
                          data,
                          as_vec.size());  // If its not the same depth the size
                                           // will allow it to throw on insert
                      self.set_array(location, i, new_arr);
                    }
                    break;
                  }
                  case CPL_TYPE_DOUBLE_COMPLEX: {
                    for (int i = 0; i < arrays.size(); i++) {
                      std::vector<std::complex<double>> as_vec =
                          arrays[i].cast<std::vector<std::complex<double>>>();
                      double _Complex* data = (double _Complex*)cpl_calloc(
                          as_vec.size(), sizeof(double _Complex));
                      std::memcpy(data, &as_vec[0],
                                  as_vec.size() * sizeof(double _Complex));
                      cpl_array* new_arr = cpl_array_wrap_double_complex(
                          data,
                          as_vec.size());  // If its not the same depth the size
                                           // will allow it to throw on insert
                      self.set_array(location, i, new_arr);
                    }
                    break;
                  }

                  case CPL_TYPE_LONG:
                  case CPL_TYPE_LONG_LONG: {
                    for (int i = 0; i < arrays.size(); i++) {
                      std::vector<long long> vec_arr =
                          arrays[i].cast<std::vector<long long>>();
                      long long* data = (long long*)cpl_calloc(
                          vec_arr.size(), sizeof(long long));

                      std::memcpy(data, &vec_arr[0],
                                  vec_arr.size() * sizeof(long long));
                      // Convert to cpl_array and then add it to the table
                      cpl_array* new_arr = cpl_array_wrap_long_long(
                          data,
                          vec_arr
                              .size());  // If its not the same depth the size
                                         // will allow it to throw on insert
                      self.set_array(location, i, new_arr);
                    }
                    break;

                    default:
                      throw cpl::core::InvalidTypeError(
                          PYCPL_ERROR_LOCATION,
                          "Passed array is of a type not compatible with "
                          "cpl.core.Table");
                      break;
                  }
                }
              }
              catch (const py::cast_error& /* unused */) {
                std::ostringstream err_msg;
                err_msg << "Type " << numpy_dtype.cast<std::string>()
                        << " Invalid";
                throw cpl::core::InvalidTypeError(PYCPL_ERROR_LOCATION,
                                                  err_msg.str().c_str());
              }
            } else if (std::optional<cpl_type> np_derived_type =
                           cpl::numpy_type_to_cpl(numpy_dtype)) {
              // If the column exists, cast based on the column type, otherwise
              // try to infer the type
              if (self.has_column(location)) {
                switch (self.get_column_type(
                    location)) {  // cast based on column type
                  case CPL_TYPE_INT: {
                    std::vector<int> vec = arr.cast<std::vector<int>>();
                    self.copy_data_int(location, vec);
                    break;
                  }
                  case CPL_TYPE_FLOAT: {
                    std::vector<float> vec = arr.cast<std::vector<float>>();
                    self.copy_data_float(location, vec);
                    break;
                  }
                  case CPL_TYPE_DOUBLE: {
                    std::vector<double> vec = arr.cast<std::vector<double>>();
                    self.copy_data_double(location, vec);
                    break;
                  }
                  case CPL_TYPE_FLOAT_COMPLEX: {
                    std::vector<std::complex<float>> vec =
                        arr.cast<std::vector<std::complex<float>>>();
                    self.copy_data_float_complex(location, vec);
                    break;
                  }
                  case CPL_TYPE_DOUBLE_COMPLEX: {
                    std::vector<std::complex<double>> vec =
                        arr.cast<std::vector<std::complex<double>>>();
                    self.copy_data_double_complex(location, vec);
                    break;
                  }
                  case CPL_TYPE_LONG_LONG: {
                    std::vector<long long> vec =
                        arr.cast<std::vector<long long>>();
                    self.copy_data_long_long(location, vec);
                    break;
                  }
                  default: {
                    // No compatible types
                    std::ostringstream err_msg;
                    err_msg << "Column is of an invalid type"
                            << py::cast(np_derived_type.value())
                                   .cast<std::string>();
                    throw cpl::core::InvalidTypeError(PYCPL_ERROR_LOCATION,
                                                      err_msg.str().c_str());
                    break;
                  }
                }
              } else {
                switch (
                    np_derived_type.value()) {  // cast based on inferred type
                  case CPL_TYPE_INT: {
                    std::vector<int> vec = arr.cast<std::vector<int>>();
                    self.wrap_int(vec, location);
                    break;
                  }
                  case CPL_TYPE_FLOAT: {
                    std::vector<float> vec = arr.cast<std::vector<float>>();
                    self.wrap_float(vec, location);
                    break;
                  }
                  case CPL_TYPE_DOUBLE: {
                    std::vector<double> vec = arr.cast<std::vector<double>>();
                    self.wrap_double(vec, location);
                    break;
                  }
                  case CPL_TYPE_FLOAT_COMPLEX: {
                    std::vector<std::complex<float>> vec =
                        arr.cast<std::vector<std::complex<float>>>();
                    self.wrap_float_complex(vec, location);
                    break;
                  }
                  case CPL_TYPE_DOUBLE_COMPLEX: {
                    std::vector<std::complex<double>> vec =
                        arr.cast<std::vector<std::complex<double>>>();
                    self.wrap_double_complex(vec, location);
                    break;
                  }
                  case CPL_TYPE_LONG:
                  case CPL_TYPE_LONG_LONG: {
                    std::vector<long long> vec =
                        arr.cast<std::vector<long long>>();
                    self.wrap_long_long(vec, location);
                    break;
                  }
                  default: {
                    // No compatible types
                    std::ostringstream err_msg;
                    err_msg << "Input array is of an invalid type"
                            << py::cast(np_derived_type.value())
                                   .cast<std::string>();
                    throw cpl::core::InvalidTypeError(PYCPL_ERROR_LOCATION,
                                                      err_msg.str().c_str());
                    break;
                  }
                }
              }
            } else if (numpy_dtype.attr("__eq__")(
                           py::module::import("builtins").attr("object"))) {
              // Could be a string or list type that numpy has labelled an
              // object
              try {
                std::vector<std::string> pystrings =
                    arr.cast<std::vector<std::string>>();
                if (self.has_column(location)) {
                  self.copy_data_string(location, pystrings);
                } else {
                  self.wrap_string(pystrings, location);
                }
              }
              catch (const py::cast_error& /* unused */) {
                // Not a string array, try something else
              }
            }

          },
          py::arg("column_name"), py::arg("setting"),
          R"pydoc(
        Set column values with numpy array compatible object. Must be of same size as the number of table rows, for example:

        .. code-block:: python

          t = table(3)
          t["new_column"]=[1,2,3]

        Will create new column "new_column" of inferred type long long with values 1,2,3. This also works with array columns e.g.

        .. code-block:: python
        
          t = table(3)
          t["new_column"]=[[1,2,3], [4,5,6], [7,8,9]]
        )pydoc")
      .def(
          "__getitem__",
          [](cpl::core::Table& self,
             std::pair<std::string, int> location) -> py::object {
            py::object np = py::module::import("numpy");
            cpl_type col_type = self.get_column_type(location.first);
            if (col_type & CPL_TYPE_POINTER) {  // Is an array type
              std::pair<const cpl_array*, int> result =
                  self.get_array(location.first, location.second);
              if (result.second == 1) {
                return py::cast(
                    std::pair<py::array, bool>(py::array(), result.second));
              } else {
                return py::cast(std::pair<py::array, bool>(
                    cpl_array_to_py(result.first), result.second));
              }
            } else {
              switch (col_type) {
                case CPL_TYPE_INT: {
                  std::pair<int, int> result =
                      self.get_int(location.first, location.second);
                  return py::cast(
                      std::pair<int, bool>(result.first, result.second));
                }
                case CPL_TYPE_FLOAT: {
                  std::pair<float, int> result =
                      self.get_float(location.first, location.second);
                  return py::cast(
                      std::pair<float, bool>(result.first, result.second));
                }
                case CPL_TYPE_DOUBLE: {
                  std::pair<double, int> result =
                      self.get_double(location.first, location.second);
                  return py::cast(
                      std::pair<double, bool>(result.first, result.second));
                }
                case CPL_TYPE_LONG_LONG: {
                  std::pair<long long, int> result =
                      self.get_long_long(location.first, location.second);
                  return py::cast(
                      std::pair<long long, bool>(result.first, result.second));
                }
                case CPL_TYPE_FLOAT_COMPLEX: {
                  std::pair<std::complex<float>, int> result =
                      self.get_complex_float(location.first, location.second);
                  return py::cast(std::pair<std::complex<float>, bool>(
                      result.first, result.second));
                }
                case CPL_TYPE_DOUBLE_COMPLEX: {
                  std::pair<std::complex<double>, int> result =
                      self.get_complex_double(location.first, location.second);
                  return py::cast(std::pair<std::complex<double>, bool>(
                      result.first, result.second));
                }
                case CPL_TYPE_STRING: {
                  std::pair<std::string, int> result =
                      self.get_string(location.first, location.second);
                  return py::cast(std::pair<std::string, bool>(result.first,
                                                               result.second));
                }
                default:
                  // Throw invalid type error
                  throw cpl::core::InvalidTypeError(
                      PYCPL_ERROR_LOCATION,
                      "Selected column is of invalid type");
                  break;
              }
            }
          },
          py::arg("location"), "Get value from location [column_name, row]")
      .def(
          "__getitem__",
          [table](cpl::core::Table& self, int row_number) {
            if ((row_number >= self.get_nrow()) or (row_number < 0)) {
              throw py::index_error();
            }
            py::dict row;
            std::vector<std::string> column_names = self.get_column_names();
            std::for_each(column_names.begin(), column_names.end(),
                          [&self, &table, &row,
                           &row_number](const std::string column_name) {
                            py::object value = table.attr("__getitem__")(
                                self, std::pair(column_name, row_number));
                            row.attr("__setitem__")(column_name, value);
                          });
            return row;
          },
          "Get table row")
      .def(
          "__getitem__",
          [m](cpl::core::Table& self, const std::string& col_name) {
            return m.attr("_TableColumn")(&self, col_name);
          },
          py::return_value_policy::reference_internal,
          "Get table column values")
      .def("set_column_depth", &cpl::core::Table::set_column_depth,
           py::arg("name"), py::arg("depth"), R"pydoc(
        Modify depth of a column of arrays

        This function is applicable just to columns of arrays. The contents
        of the arrays in the specified column will be unchanged up to the
        lesser of the new and old depths. If the depth is increased, the
        extra array elements would be flagged as invalid. Existing references
        to the array data should be considered invalid after calling this method.

        Parameters
        ----------
        name : str
            Column name.
        depth : int
            New column depth.
        )pydoc")  // Throws type mismatch error if column is not
                  // of array type: way to bake in "Oh dear this
                  // thing is an array type" message?
      .def("__str__",
           [](cpl::core::Table& self) -> std::string {
             size nrows = self.get_nrow();
             size count = 5;
             std::string dump_string("Empty table");
             if (nrows > 0) {
               if (nrows < count) {
                 count = nrows;
               }
               dump_string = self.dump(0, count);
             }
             return dump_string;
           })
      .def("__repr__", &cpl::core::Table::dump_structure)
      .def("__len__", &cpl::core::Table::get_nrow)

      .def("abs", &cpl::core::Table::abs_column, py::arg("name"), R"pydoc(
        Compute the absolute value of column values.

        Each column element is replaced by its absolute value.
        Invalid elements are not modified by this operation.
        If the column is complex, its type will be turned to
        real (cpl.core.Type.FLOAT_COMPLEX will be changed into cpl.core.Type.FLOAT,
        and cpl.core.Type.DOUBLE_COMPLEX will be changed into cpl.core.Type.DOUBLE).

        Parameters
        ----------
        name : str
            Table column name.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical, or is an array column
        )pydoc")
      .def("get_column_dimensions", &cpl::core::Table::get_column_dimensions,
           py::arg("name"), R"pydoc(
        Get the number of dimensions of a table column of arrays.

        Get the number of dimensions of a column. If a column is not an array
        column, or if it has no dimensions, 1 is returned.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        int
            Column number of dimensions, or 0 in case of failure.

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` not found in table.
        )pydoc")
      .def("set_column_dimensions", &cpl::core::Table::set_column_dimensions,
           py::arg("name"), py::arg("dimensions"), R"pydoc(
        Set the dimensions of a table column of arrays.

        Set the number of dimensions of a column. If the  dimensions array
        has size less than 2, nothing is done and no error is returned.

        Parameters
        ----------
        name : str
            Column name.
        dimensions : list of int
            the sizes of the column dimensions

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` not found in table.
        cpl.core.IllegalInputError
            A column with the given `name` is not of type cpl.core.Type.ARRAY, or `dimensions` contains invalid elements
        cpl.core.IncompatibleInputError
            The specified dimensions are incompatible with the total number of elements in the column arrrays.
        )pydoc")
      .def("get_column_dimension", &cpl::core::Table::get_column_dimension,
           py::arg("name"), py::arg("indx"), R"pydoc(
        Get size of one dimension of a table column of arrays.

        Get the size of one dimension of a column. If a column is not an array
        column, or if it has no dimensions, 1 is returned.

        Parameters
        ----------
        name : str
            Column name.
        indx : int
            Indicate dimension to query (0 = x, 1 = y, 2 = z, etc.).

        Returns
        -------
        int
            Size of queried dimension of the column, or zero in case of error.

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` not found in table.
        cpl.core.UnsupportedModeError
            A column with the given `name` is not of type cpl.core.Type.ARRAY
        cpl.core.AccessOutOfRangeError
            The specified `indx` array is not compatible with the column dimensions
        cpl.core.IncompatibleInputError
            The specified dimensions are incompatible with the total number of elements in the column arrays.
        )pydoc")
      .def("set_column_unit", &cpl::core::Table::set_column_unit,
           py::arg("name"), py::arg("unit"), R"pydoc(
        Give a new unit to a table column.

        The input unit string is duplicated before being used as the column
        unit.

        The unit associated to a column has no effect on any operation performed
        on columns, and it must be considered just an optional description of
        the content of a column. It is however saved to a FITS file when using
        save().

        Parameters
        ----------
        name : str
            Column name.
        unit : str
            New unit.

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` not found in table.
        )pydoc")
      .def("get_column_unit", &cpl::core::Table::get_column_unit,
           py::arg("name"), R"pydoc(
        Get the unit of a table column.

        Return the unit of a column if present, otherwise `None` is
        returned.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        str
            Unit of column.

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` not found in table.
        )pydoc")
      .def("set_column_format", &cpl::core::Table::set_column_format,
           py::arg("name"), py::arg("format"), R"pydoc(
        Give a new format to a table column.

        The input format string is duplicated before being used as the column
        format. If no format is set, "%s" will be used if
        the column is of type cpl.core.Type.STRING, "%1.5e" if the column is
        of type  cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE, and "%7d" if it is
        of type  cpl.core.Type.INT. The format associated to a column has no
        effect on any operation performed on columns, and it is used just
        while printing a table using the function :py:meth:`dump`.

        This information is lost after saving the table in FITS format using  save().

        Parameters
        ----------
        name : str
            Column name.
        format : str
            New format.

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` not found in table.
        )pydoc")
      .def("get_column_format", &cpl::core::Table::get_column_format,
           py::arg("name"), R"pydoc(
        Get the format of a table column.

        Return the format of a column.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        str
            Format of column.

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` not found in table.
        )pydoc")

      .def("erase_column", &cpl::core::Table::erase_column, py::arg("name"),
           R"pydoc(
        Delete a column from a table.

        Delete a column from a table. If the table is left without columns,
        also the selection flags are lost.

        Parameters
        ----------
        name : str
            Name of table column to delete.

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` not found in table.
        )pydoc")
      .def("erase_window", &cpl::core::Table::erase_window, py::arg("start"),
           py::arg("count"), R"pydoc(
        Delete a table segment.

        A portion of the table data is physically removed.

        Parameters
        ----------
        start : int
            First row to delete.
        count : int
            Number of rows to delete.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            The table has length of zero, or `start` is outside the table range.
        cpl.core.IllegalInputError
            `count` is negative
        )pydoc")
      .def("erase_selected", &cpl::core::Table::erase_selected, R"pydoc(
        Delete the selected rows of a table.

        A portion of the table data is physically removed, and the table
        selection flags are set back to "all selected".
        )pydoc")
      .def("insert_window", &cpl::core::Table::insert_window, py::arg("start"),
           py::arg("count"), R"pydoc(
        Insert a segment of rows into table data.

        Insert a segment of empty rows, just containing invalid elements.
        Setting `start` to a number greater than the column length is legal,
        and has the effect of appending extra rows at the end of the table:
        this is equivalent to expanding the table using set_size().
        The input column may also have zero length.

        The table selection flags are set back to "all selected".

        Parameters
        ----------
        start : int
            Row where to insert the segment.
        count : int
            Length of the segment.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            `start` is negative
        cpl.core.IllegalInputError
            `count` is negative
        )pydoc")

      .def("compare_structure", &cpl::core::Table::compare_structure,
           py::arg("other"), R"pydoc(
        Compare the structure of two tables.

        Two tables have the same structure if they have the same number
        of columns, with the same names, the same types, and the same units.
        The order of the columns is not relevant.

        Parameters
        ----------
        other : cpl.core.Table
            Other table to compare with.

        Returns
        -------
        bool
            True if the tables have the same structure, otherwise False.
        )pydoc")
      .def("insert", &cpl::core::Table::insert, py::arg("insert_table"),
           py::arg("row"), R"pydoc(
        Insert a table into `self`

        The input tables must have the same structure, as defined by the function

        Parameters
        ----------
        insert_table : cpl.core.Table
            Table to be inserted in the target table.
        row : int
            Row where to insert the insert table.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            `row` is negative
        cpl.core.IncompatibleInputError
            `self` and `insert_table` do not have the same structure.
        )pydoc")
      .def("get", &cpl::core::Table::get, py::arg("name"), py::arg("row"),
           R"pydoc(
        Read a value from a numerical column.

        Rows are counted starting from 0.

        Parameters
        ----------
        name : str
            Name of table column to be accessed.
        row : int
            Position of element to be read.

        Returns
        -------
        float
            Value read. In case of invalid table element 0.0 is returned.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            `self` has zero length or `row` is outside table `self`'s boundaries
        cpl.core.DataNotFoundError
            A column with the given `name` is not found in `self`.
        cpl.core.InvalidTypeError
            The specified column is not numerical, or is a column of arrays.
        )pydoc")
      .def("set_invalid", &cpl::core::Table::set_invalid, py::arg("name"),
           py::arg("row"), R"pydoc(
        Flag a column element as invalid.
        
        The column element given by the column name `name` and the row number `row` is flagged as invalid.
        This also means that the data which was stored in this table cell becomes inaccessible. To reset
        an invalid column cell it must be updated with a new value.

        Parameters
        ----------
        name : str
            Name of table column to access.
        row : int
            Table row to set to invalid.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            `self` has zero length or `row` is outside table `self`'s boundaries
        cpl.core.DataNotFoundError
            A column with the given `name` is not found in `self`.
        )pydoc")
      .def("count_invalid", &cpl::core::Table::count_invalid, py::arg("name"),
           R"pydoc(
        Count number of invalid values in a table column.

        Count number of invalid elements in a table column.

        Parameters
        ----------
        name : str
            Name of table column to examine.

        Returns
        -------
        int
            Number of invalid elements in a table column.

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` is not found in `self`.
        )pydoc")

      .def("has_invalid", &cpl::core::Table::has_invalid, py::arg("name"),
           R"pydoc(
        Check if a column contains at least one invalid value.

        Check if there are invalid elements in a column. In case of columns
        of arrays, invalid values within an array are not considered.

        Parameters
        ----------
        name : str
            Name of table column to access.

        Returns
        -------
        bool
            True if the column contains at least one invalid element

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` is not found in `self`.
        )pydoc")
      .def("has_valid", &cpl::core::Table::has_valid, py::arg("name"), R"pydoc(
        Check if a column contains at least one valid value.

        Check if there are valid elements in a column. In case of columns
        of arrays, invalid values within an array are not considered.

        Parameters
        ----------
        name : str
            Name of table column to access.

        Returns
        -------
        bool
            True if the column contains at least one valid element

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` is not found in `self`.

        )pydoc")
      .def("set_column_invalid", &cpl::core::Table::set_column_invalid,
           py::arg("name"), py::arg("start"), py::arg("count"), R"pydoc(
        Invalidate a column segment.

        All the column elements in the specified interval are invalidated.
        In the case of either a string or an  rray column, the
        corresponding strings or arrays are set free. If the sum of start
        and count exceeds the number of rows in the table, the column is
        invalidated up to its end.

        Parameters
        ----------
        name : str
            Name of table column to access.
        start : int
            Position where to begin invalidation.
        count : int
            Number of column elements to invalidate.

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` is not found in `self`.
        cpl.core.AccessOutOfRangeError
            `self` has zero length, or `start` is outside the table boundaries.
        cpl.core.IllegalInputError
            `count` is negative
        )pydoc")
      .def("is_valid", &cpl::core::Table::is_valid, py::arg("name"),
           py::arg("row"), R"pydoc(
        Check if a column element is valid.

        Check if a column element is valid.

        Parameters
        ----------
        name : str
            Name of table column to access.
        row : int
            Column element to examine.

        Returns
        -------
        bool
            True if the column element is valid, False if invalid

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` is not found in `self`.
        )pydoc")
      .def("move_column", &cpl::core::Table::move_column, py::arg("name"),
           py::arg("from_table"), R"pydoc(
        Move a column from a table to `self`.

        Move a column from a table to `self`.

        Parameters
        ----------
        name : str
            Name of column to move.
        from_table : cpl.core.Table
            Source table.

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `name` is not found in `from_table`.
        cpl.core.IncompatibleInputError
            `self` and `from_table` do not have the same number of rows
        cpl.core.IllegalInputError
            `self` and `from_table` are the same table (object)
        cpl.core.IllegalOutputError
            `name` already exists as a column in `self`
        )pydoc")
      .def("duplicate_column", &cpl::core::Table::duplicate_column,
           py::arg("to_name"), py::arg("from_table"), py::arg("from_name"),
           R"pydoc(
        Copy a column from a table to `self`.

        Copy a column from a table to `self`. The column is duplicated. A column
        may be duplicated also within the same table.

        Parameters
        ----------
        to_name : str
            New name of copied column.
        from_table : cpl.core.Table
            Source table.
        from_name : str
            Name of column to copy.

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `from_name` is not found in `from_table`.
        cpl.core.IncompatibleInputError
            `self` and `from_table` do not have the same number of rows
        cpl.core.IllegalOutputError
            `to_name` already exists as a column in `self`
        )pydoc")
      .def("name_column", &cpl::core::Table::name_column, py::arg("from_name"),
           py::arg("to_name"), R"pydoc(
        Rename a table column.

        This function is used to change the name of a column.

        Parameters
        ----------
        from_name : str
            Name of table column to rename.
        to_name : str
            New name of column.

        Raises
        ------
        cpl.core.DataNotFoundError
            A column with the given `from_name` is not found in `self`.
        cpl.core.IllegalOutputError
            `name` already exists as a column in `self`
        )pydoc")

      // TODO: any function defined from here probably has incomplete raises
      // docs
      .def("set_size", &cpl::core::Table::set_size, py::arg("new_length"),
           R"pydoc(
        Resize a table to a new number of rows.

        The contents of the columns will be unchanged up to the lesser of the
        new and old sizes. If the table is expanded, the extra table rows would
        just contain invalid elements. The table selection flags are set back
        to "all selected". Existing eferences to the column data should be considered
        invalid after calling this method.

        Parameters
        ----------
        new_length : int
            New number of rows in table.
        )pydoc")

      .def("__deepcopy__",
           [](cpl::core::Table& self,
              py::dict /* memo */) -> std::shared_ptr<cpl::core::Table> {
             return self.duplicate();
           })
      .def("extract", &cpl::core::Table::extract, py::arg("start"),
           py::arg("count"), R"pydoc(
        Create a table from a section of another table.

        A number of consecutive rows are copied from an input table to a
        newly created table. The new table will have the same structure of
        the original table (see function  :py:meth:`compare_structure`).
        If the sum of  start and  count goes beyond the end of the
        input table, rows are copied up to the end. All the rows of the
        new table are selected, i.e., existing selection flags are not
        transferred from the old table to the new one.

        Parameters
        ----------
        start : int
            First row to be copied to new table.
        count : int
            Number of rows to be copied.

        Returns
        -------
        cpl.core.Table
            The new table.

        )pydoc")
      .def("cast_column", &cpl::core::Table::cast_column, py::arg("from_name"),
           py::arg("to_name"), py::arg("type"), R"pydoc(
        Cast a numeric or complex column to a new numeric or complex type column.

        A new column of the specified type is created, and the content of the
        given numeric column is cast to the new type. If the input column type
        is identical to the specified type the column is duplicated as is done
        by the function  :py:meth:`duplicate_column`. Note that a column of
        arrays is always cast to another column of arrays of the specified type,
        unless it has depth 1. Consistently, a column of numbers can be cast
        to a column of arrays of depth 1.
        Here is a complete summary of how any (legal)  type specification
        would be interpreted, depending on the type of the input column:

        from_name type = cpl.core.Type.XXX | cpl.core.Type.POINTER
        specified type = cpl.core.Type.XXX | cpl.core.Type.POINTER
        to_name   type = cpl.core.Type.XXX | cpl.core.Type.POINTER

        from_name type = cpl.core.Type.XXX | cpl.core.Type.POINTER (depth > 1)
        specified type = cpl.core.Type.XXX
        to_name   type = cpl.core.Type.XXX | cpl.core.Type.POINTER

        from_name type = cpl.core.Type.XXX | cpl.core.Type.POINTER (depth = 1)
        specified type = cpl.core.Type.XXX
        to_name   type = cpl.core.Type.XXX

        from_name type = cpl.core.Type.XXX
        specified type = cpl.core.Type.XXX | cpl.core.Type.POINTER
        to_name   type = cpl.core.Type.XXX | cpl.core.Type.POINTER (depth = 1)

        from_name type = cpl.core.Type.XXX
        specified type = cpl.core.Type.POINTER
        to_name   type = cpl.core.Type.XXX | cpl.core.Type.POINTER (depth = 1)

        from_name type = cpl.core.Type.XXX
        specified type = cpl.core.Type.YYY
        to_name   type = cpl.core.Type.YYY

        from_name type = cpl.core.Type.XXX | cpl.core.Type.POINTER
        specified type = cpl.core.Type.YYY | cpl.core.Type.POINTER
        to_name   type = cpl.core.Type.YYY | cpl.core.Type.POINTER

        from_name type = cpl.core.Type.XXX | cpl.core.Type.POINTER (depth > 1)
        specified type = cpl.core.Type.YYY
        to_name   type = cpl.core.Type.YYY | cpl.core.Type.POINTER

        from_name type = cpl.core.Type.XXX | cpl.core.Type.POINTER (depth = 1)
        specified type = cpl.core.Type.YYY
        to_name   type = cpl.core.Type.YYY

        from_name type = cpl.core.Type.XXX
        specified type = cpl.core.Type.YYY | cpl.core.Type.POINTER
        to_name   type = cpl.core.Type.YYY | cpl.core.Type.POINTER (depth = 1)

        Parameters
        ----------
        from_name : str
            Name of table column to cast.
        to_name : str
            Name of new table column.
        type : cpl.core.Type
            Type of new table column.
        )pydoc")
      .def("add_columns", &cpl::core::Table::add_columns, py::arg("to_name"),
           py::arg("from_name"), R"pydoc(
        Add the values of two numeric or complex table columns.

        The columns are summed element by element, and the result of the sum is
        stored in the target column. The columns' types may differ, and in that
        case the operation would be performed using the standard C upcasting
        rules, with a final cast of the result to the target column type.
        Invalid elements are propagated consistently: if either or both members
        of the sum are invalid, the result will be invalid too. Underflows and
        overflows are ignored.

        Parameters
        ----------
        to_name : str
            Name of target column.
        from_name : str
            Name of source column.
        )pydoc")
      .def("subtract_columns", &cpl::core::Table::subtract_columns,
           py::arg("to_name"), py::arg("from_name"), R"pydoc(
        Subtract two numeric or complex table columns.

        The columns are subtracted element by element, and the result of the
        subtraction is stored in the target column. See the documentation of
        the function  :py:meth:`add_columns` for further details.

        Parameters
        ----------
        to_name : str
            Name of target column.
        from_name : str
            Name of column to be subtracted from target column.
        )pydoc")
      .def("multiply_columns", &cpl::core::Table::multiply_columns,
           py::arg("to_name"), py::arg("from_name"), R"pydoc(
        Multiply two numeric or complex table columns.

        The columns are multiplied element by element, and the result of the
        multiplication is stored in the target column. See the documentation of
        the function  :py:meth:`add_columns` for further details.

        Parameters
        ----------
        to_name : str
            Name of target column.
        from_name : str
            Name of column to be multiplied with target column.
        )pydoc")
      .def("divide_columns", &cpl::core::Table::divide_columns,
           py::arg("to_name"), py::arg("from_name"), R"pydoc(
        Divide two numeric or complex table columns.

        The columns are divided element by element, and the result of the
        division is stored in the target column. The columns' types may
        differ, and in that case the operation would be performed using
        the standard C upcasting rules, with a final cast of the result
        to the target column type. Invalid elements are propagated consistently:
        if either or both members of the division are invalid, the result
        will be invalid too. Underflows and overflows are ignored, but a
        division by exactly zero will set an invalid column element.

        Parameters
        ----------
        to_name : str
            Name of target column.
        from_name : str
            Name of column dividing the target column.
        )pydoc")
      .def("add_scalar", &cpl::core::Table::add_scalar, py::arg("name"),
           py::arg("value"), R"pydoc(
        Add a constant value to a numerical or complex column.

        The operation is always performed in double precision, with a final
        cast of the result to the target column type. Invalid elements are
        are not modified by this operation.

        Parameters
        ----------
        name : str
            Column name.
        value : float
            Value to add.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical, or is an array column
        )pydoc")
      .def("add_scalar_complex", &cpl::core::Table::add_scalar_complex,
           py::arg("name"), py::arg("value"), R"pydoc(
        Add a constant complex value to a numerical or complex column.

        The operation is always performed in double precision, with a final
        cast of the result to the target column type. Invalid elements are
        are not modified by this operation.

        Parameters
        ----------
        name : str
            Column name.
        value : complex
            Value to add.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical, or is an array column
        )pydoc")

      .def("subtract_scalar", &cpl::core::Table::subtract_scalar,
           py::arg("name"), py::arg("value"), R"pydoc(
        Subtract a constant value from a numerical or complex column.

        See the description of the function  :py:meth:`add_scalar`.

        Parameters
        ----------
        name : str
            Column name.
        value : float
            Value to subtract.
        )pydoc")
      .def("subtract_scalar_complex",
           &cpl::core::Table::subtract_scalar_complex, py::arg("name"),
           py::arg("value"), R"pydoc(
        Subtract a constant complex value from a numerical or complex column.

        The operation is always performed in double precision, with a final
        cast of the result to the target column type. Invalid elements are
        are not modified by this operation.

        Parameters
        ----------
        name : str
            Column name.
        value : complex
            Value to subtract.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical, or is an array column
        )pydoc")
      .def("multiply_scalar", &cpl::core::Table::multiply_scalar,
           py::arg("name"), py::arg("value"), R"pydoc(
        Multiply a numerical or complex column by a constant.

        The operation is always performed in double precision, with a final
        cast of the result to the target column type. Invalid elements are
        are not modified by this operation.

        Parameters
        ----------
        name : str
            Column name.
        value : float
            Multiplication factor.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical, or is an array column
        )pydoc")
      .def("multiply_scalar_complex",
           &cpl::core::Table::multiply_scalar_complex, py::arg("name"),
           py::arg("value"), R"pydoc(
        Multiply a numerical or complex column by a complex constant.

        The operation is always performed in double precision, with a final
        cast of the result to the target column type. Invalid elements are
        are not modified by this operation.

        Parameters
        ----------
        name : str
            Column name.
        value : complex
            Multiplication factor.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical, or is an array column
        )pydoc")
      .def("divide_scalar", &cpl::core::Table::divide_scalar, py::arg("name"),
           py::arg("value"), R"pydoc(
        Divide a numerical or complex column by a constant.

        The operation is always performed in double precision, with a final
        cast of the result to the target column type. Invalid elements are
        not modified by this operation.

        Parameters
        ----------
        name : str
            Column name.
        value : float
            Divisor value.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical, or is an array column
        cpl.core.DivisionByZeroError
            `value` is equal to 0.0
        )pydoc")
      .def("divide_scalar_complex", &cpl::core::Table::divide_scalar_complex,
           py::arg("name"), py::arg("value"), R"pydoc(
        Divide a numerical or complex column by a complex constant.

        The operation is always performed in double precision, with a final
        cast of the result to the target column type. Invalid elements are
        not modified by this operation.

        Parameters
        ----------
        name : str
            Column name.
        value : complex
            Divisor value.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical, or is an array column
        cpl.core.DivisionByZeroError
            `value` is equal to 0.0
        )pydoc")
      .def("logarithm_column", &cpl::core::Table::logarithm_column,
           py::arg("name"), py::arg("base"), R"pydoc(
        Compute the logarithm of column values.

        Each column element is replaced by its logarithm in the specified base.
        The operation is always performed in double precision, with a final
        cast of the result to the target column type. Invalid elements are
        not modified by this operation, but zero or negative elements are
        invalidated by this operation. In case of complex numbers, values
        very close to the origin may cause an overflow. The imaginary part
        of the result is chosen in the interval [-pi/ln(base),pi/ln(base)],
        so it should be kept in mind that doing the logarithm of exponential
        of a complex number will not always express the phase angle with the
        same number. For instance, the exponential in base 2 of (5.00, 5.00)
        is (-30.33, -10.19), and the logarithm in base 2 of the latter will
        be expressed as (5.00, -4.06).

        Parameters
        ----------
        name : str
            Table column name.
        base : float
            Logarithm base.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical, or is an array column
        cpl.core.IllegalInputError
            `base` is not positive
        )pydoc")
      .def("exponential_column", &cpl::core::Table::exponential_column,
           py::arg("name"), py::arg("base"), R"pydoc(
        Compute the exponential of column values.

        Each column element is replaced by its exponential in the specified base.
        The operation is always performed in double precision, with a final
        cast of the result to the target column type. Invalid elements are
        not modified by this operation.

        Parameters
        ----------
        name : str
            Column name.
        base : float
            Exponential base.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical, or is an array column
        cpl.core.IllegalInputError
            `base` is not positive
        )pydoc")
      .def("conjugate_column", &cpl::core::Table::conjugate_column,
           py::arg("name"), R"pydoc(
        Compute the complex conjugate of column values.

        Each column element is replaced by its complex conjugate.
        The operation is always performed in double precision, with a final
        cast of the result to the target column type. Invalid elements are
        not modified by this operation.

        Parameters
        ----------
        name : str
            Column name.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical, or is an array column
        )pydoc")
      .def("power_column", &cpl::core::Table::power_column, py::arg("name"),
           py::arg("exponent"), R"pydoc(
        Compute the power of numerical column values.

        Each column element is replaced by its power to the specified exponent.
        For float and float complex the operation is performed in single precision,
        otherwise it is performed in double precision and then rounded if the column
        is of an integer type. Results that would or do cause domain errors or
        overflow are marked as invalid.

        Parameters
        ----------
        name : str
            Name of column of numerical type
        exponent : float
            Constant exponent.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical
        )pydoc")
      .def("arg_column", &cpl::core::Table::arg_column, py::arg("name"),
           R"pydoc(
        Compute the phase angle value of table column elements.

        Each column element is replaced by its phase angle value.
        The phase angle will be in the range of [-pi,pi].
        Invalid elements are not modified by this operation.
        If the column is complex, its type will be turned to
        real (cpl.core.Type.FLOAT_COMPLEX will be changed into cpl.core.Type.FLOAT,
        and cpl.core.Type.DOUBLE_COMPLEX will be changed into cpl.core.Type.DOUBLE).

        Parameters
        ----------
        name : str
            Column name.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical or complex
        )pydoc")
      .def("real_column", &cpl::core::Table::real_column, py::arg("name"),
           R"pydoc(
        Compute the real part value of table column elements.

        Each column element is replaced by its real party value only.
        Invalid elements are not modified by this operation.
        If the column is complex, its type will be turned to
        real (cpl.core.Type.FLOAT_COMPLEX will be changed into cpl.core.Type.FLOAT,
        and cpl.core.Type.DOUBLE_COMPLEX will be changed into cpl.core.Type.DOUBLE).

        Parameters
        ----------
        name : str
            Column name.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical or complex
        )pydoc")
      .def("imag_column", &cpl::core::Table::imag_column, py::arg("name"),
           R"pydoc(
        Compute the imaginary part value of table column elements.

        Each column element is replaced by its imaginary party value only.
        Invalid elements are not modified by this operation.
        If the column is complex, its type will be turned to
        real (cpl.core.Type.FLOAT_COMPLEX will be changed into cpl.core.Type.FLOAT,
        and cpl.core.Type.DOUBLE_COMPLEX will be changed into cpl.core.Type.DOUBLE).
        Existing references to the column data should be considered as invalid after
        calling this method.

        Parameters
        ----------
        name : str
            Column name.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`
        cpl.core.InvalidTypeError
            If the requested column is not numerical or complex
        )pydoc")
      .def("get_column_mean_complex",
           &cpl::core::Table::get_column_mean_complex, py::arg("name"), R"pydoc(
        Compute the mean value of a numerical or complex column.

        Invalid column values are excluded from the computation. The table
        selection flags have no influence on the result.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        complex
            Mean value. In case of error 0.0 is returned.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`, or it just contains invalid elements, or table length is zero
        cpl.core.InvalidTypeError
            If the requested column is not numerical or complex
        )pydoc")
      .def("get_column_mean", &cpl::core::Table::get_column_mean,
           py::arg("name"), R"pydoc(
        Compute the mean value of a numerical column.

        Invalid column values are excluded from the computation. The table
        selection flags have no influence on the result.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        float
            Mean value

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`, or it just contains invalid elements, or table length is zero
        cpl.core.InvalidTypeError
            If the requested column is not numerical
        )pydoc")
      .def("get_column_median", &cpl::core::Table::get_column_median,
           py::arg("name"), R"pydoc(
        Compute the median value of a numerical column.

        Invalid column values are excluded from the computation. The table
        selection flags have no influence on the result.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        float
            Median value

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`, or it just contains invalid elements, or table length is zero
        cpl.core.InvalidTypeError
            If the requested column is not numerical
        )pydoc")
      .def("get_column_stdev", &cpl::core::Table::get_column_stdev,
           py::arg("name"), R"pydoc(
        Find the standard deviation of a table column.

        Invalid column values are excluded from the computation of the
        standard deviation. If just one valid element is found, 0.0 is
        returned but no error is set. The table selection flags have no
        influence on the result.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        float
            Standard deviation

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`, or it just contains invalid elements, or table length is zero
        cpl.core.InvalidTypeError
            If the requested column is not numerical
        )pydoc")
      .def("get_column_max", &cpl::core::Table::get_column_max, py::arg("name"),
           R"pydoc(
        Get maximum value in a numerical column.

        Invalid column values are excluded from the computation. The table
        selection flags have no influence on the result.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        float
            Maximum value. See documentation of  :py:meth:`get_column_mean`.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`, or it just contains invalid elements, or table length is zero
        cpl.core.InvalidTypeError
            If the requested column is not numerical
        )pydoc")
      .def("get_column_min", &cpl::core::Table::get_column_min, py::arg("name"),
           R"pydoc(
        Get minimum value in a numerical column.

        Invalid column values are excluded from the computation. The table
        selection flags have no influence on the result.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        float
            Minimum value. See documentation of  :py:meth:`get_column_mean`.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`, or it just contains invalid elements, or table length is zero
        cpl.core.InvalidTypeError
            If the requested column is not numerical
        )pydoc")
      .def("get_column_maxpos", &cpl::core::Table::get_column_maxpos,
           py::arg("name"), R"pydoc(
        Get position of maximum in a numerical column.

        Invalid column values are excluded from the search. The return value is the
        position of the maximum value where rows are counted starting from 0.

        If more than one column element correspond to the max value, the position
        with the lowest row number is returned.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        int
            Returned row position of maximum value in column `name`

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`, or it just contains invalid elements, or table length is zero
        cpl.core.InvalidTypeError
            If the requested column is not numerical
        )pydoc")
      .def("get_column_minpos", &cpl::core::Table::get_column_minpos,
           py::arg("name"), R"pydoc(
        Get position of minimum in a numerical column.

        Invalid column values are excluded from the search. The return value is the
        position of the minimum value where rows are counted starting from 0.

        If more than one column element correspond to the minimum value, the position
        with the lowest row number is returned.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        int
            Returned row position of minimum value in column `name`

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with the specified `name` is not found in `self`, or it just contains invalid elements, or table length is zero
        cpl.core.InvalidTypeError
            If the requested column is not numerical
        )pydoc")

      .def("erase_invalid_rows", &cpl::core::Table::erase_invalid_rows, R"pydoc(
        Remove from a table columns and rows just containing invalid elements.

        Table columns and table rows just containing invalid elements are deleted
        from the table, i.e. a column or a row is deleted only if all of its
        elements are invalid. The selection flags are set back to "all selected"
        even if no rows or columns are removed.

        Notes
        -----
        If the input table just contains invalid elements, all columns are deleted.
        )pydoc")
      .def("erase_invalid", &cpl::core::Table::erase_invalid, R"pydoc(
        Remove from a table all columns just containing invalid elements,
        and then all rows containing at least one invalid element.

        Firstly, all columns consisting just of invalid elements are deleted
        from the table. Next, the remaining table rows containing at least
        one invalid element are also deleted from the table.

        The function is similar to the function :py:meth:`erase_invalid_rows`,
        except for the criteria to remove rows containing invalid elements after
        all invalid columns have been removed. While :py:meth:`erase_invalid_rows`
        requires all elements to be invalid in order to remove a row from the
        table, this function requires only one (or more) elements to be invalid.

        Notes
        -----
        If the input table just contains invalid elements, all columns are deleted.
        )pydoc")

      .def("select_row", &cpl::core::Table::select_row, py::arg("row"), R"pydoc(
        Flag a table row as selected.

        Flag a table row as selected. Any previous selection is kept.

        Parameters
        ----------
        row : int
            Row to select.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            `self` has a length of zero, or `row` is outside the table boundaries.
        )pydoc")
      .def("select_all", &cpl::core::Table::select_all, R"pydoc(
        Select all table rows.

        The table selection flags are reset, meaning that they are
        all marked as selected. This is the initial state of any
        table.
        )pydoc")
      .def("unselect_row", &cpl::core::Table::unselect_row, py::arg("row"),
           R"pydoc(
        Flag a table row as unselected.

        Flag a table row as unselected. Any previous selection is kept.

        Parameters
        ----------
        row : int
            Row to unselect.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            `self` has a length of zero, or `row` is outside the table boundaries.
        )pydoc")
      .def("unselect_all", &cpl::core::Table::unselect_all, R"pydoc(
        Unselect all table rows.

        The table selection flags are all unset, meaning that no table
        rows are selected.
        )pydoc")

      .def(
          "dump",
          [](cpl::core::Table& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode, size start, py::object count,
             std::optional<bool> show) -> std::string {
            size actual_count;
            if (count.is_none()) {
              actual_count = self.get_nrow() - start;
            } else {
              actual_count = count.cast<size>();
            }
            // TODO: we could put some more errors here around invalid access
            // etc, but cpl_table_dump is fairly casual about it
            return dump_handler(filename.value(), mode.value(),
                                self.dump(start, actual_count), show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w", py::arg("start") = 0,
          py::arg("count") = py::none(), py::arg("show") = true,
          R"pydoc(
        Dump the Table contents to a file, stdout or a string.
          
        This function is mainly intended for debug purposes.
        All column elements are printed according to the column formats,
        that may be specified for each table column with the function.

        Parameters
        ----------
        filename : str, optional
            file path to dump table contents to
        mode : str, optional
            File mode to save the file, default 'w' overwrites contents.
        start : int
            First row to print
        count : int
            Number of rows to print
        show : bool, optional
            Send table contents to stdout. Defaults to True.

        Returns
        -------
        str 
            Multiline string containing the dump of the table contents.
        )pydoc")

      .def("shift_column", &cpl::core::Table::shift_column, py::arg("name"),
           py::arg("shift"), R"pydoc(
        Shift the position of numeric or complex column values.

        The position of all column values is shifted by the specified amount.
        If  shift is positive, all values will be moved toward the bottom
        of the column, otherwise toward its top. In either case as many column
        elements as the amount of the  shift will be left undefined, either
        at the top or at the bottom of the column according to the direction
        of the shift. These column elements will be marked as invalid. This
        function is applicable just to numeric and complex columns, and not
        to strings and or array types. The selection flags are always set back
        to "all selected" after this operation.

        Parameters
        ----------
        name : str
            Name of table column to shift.
        shift : int
            Shift column values by so many rows.
        )pydoc")
      .def("and_selected_invalid", &cpl::core::Table::and_selected_invalid,
           py::arg("name"), R"pydoc(
        Select from selected table rows all rows with an invalid value in a specified column.

        For all the already selected table rows, all the rows containing valid
        values at the specified column are unselected. See also the function

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        int
            New number of selected rows

        )pydoc")
      .def("or_selected_invalid", &cpl::core::Table::or_selected_invalid,
           py::arg("name"), R"pydoc(
        Select from unselected table rows all rows with an invalid value in a specified column.

        For all the unselected table rows, all the rows containing invalid
        values at the specified column are selected.

        Parameters
        ----------
        name : str
            Column name.

        Returns
        -------
        int
            New number of selected rows

        )pydoc")
      .def(
          "fill_invalid",
          [](cpl::core::Table& self, std::string column_name,
             py::object value) -> void {
            switch ((int)self.get_column_type(column_name)) {
              case CPL_TYPE_LONG_LONG:
              case CPL_TYPE_LONG_LONG |
                  CPL_TYPE_POINTER:  // Needs to work with array types as well
                self.fill_invalid_long_long(column_name,
                                            value.cast<long long>());
                break;
              case CPL_TYPE_FLOAT:
              case CPL_TYPE_FLOAT | CPL_TYPE_POINTER:
                self.fill_invalid_float(column_name, value.cast<float>());
                break;
              case CPL_TYPE_DOUBLE:
              case CPL_TYPE_DOUBLE | CPL_TYPE_POINTER:
                self.fill_invalid_double(column_name, value.cast<double>());
                break;
              case CPL_TYPE_INT:
              case CPL_TYPE_INT | CPL_TYPE_POINTER:
                self.fill_invalid_int(column_name, value.cast<int>());
                break;
              case CPL_TYPE_FLOAT_COMPLEX:
              case CPL_TYPE_FLOAT_COMPLEX | CPL_TYPE_POINTER:
                self.fill_invalid_float_complex(
                    column_name, value.cast<std::complex<float>>());
                break;
              case CPL_TYPE_DOUBLE_COMPLEX:
              case CPL_TYPE_DOUBLE_COMPLEX | CPL_TYPE_POINTER:
                self.fill_invalid_double_complex(
                    column_name, value.cast<std::complex<double>>());
                break;
              case CPL_TYPE_STRING:
                throw cpl::core::InvalidTypeError(
                    PYCPL_ERROR_LOCATION,
                    "fill_invalid not available for string type. No operation "
                    "can be performed");
                break;
              default:
                throw cpl::core::InvalidTypeError(
                    PYCPL_ERROR_LOCATION,
                    " selected column is of an invalid data type. No operation "
                    "can be performed");
                break;
            }
          },
          py::arg("name"), py::arg("value"), R"pydoc(
        Write a numerical value to invalid elements.

        The value will adapt to the column type

        Parameters
        ----------
        name : str
            Column name.
        value : float, int, complex, or array
            Value to write to invalid column elements.

        Notes
        -----
        Assigning a value to an invalid numerical element will not make it valid,
        but assigning a value to an element consisting of an array of numbers
        will make the array element valid.
        )pydoc")
      .def("or_selected_string", &cpl::core::Table::or_selected_string,
           py::arg("name"), py::arg("operator"), py::arg("string"), R"pydoc(
        Select from unselected table rows, by comparing a column of string values to the given `string`.

        For all the unselected table rows, the values of the specified column are compared with the
        reference string. The table rows fulfilling the comparison are selected. An invalid element never
        fulfills any comparison by definition.

        If `operator` is equal to cpl.core.Operator.EQUAL_TO or cpl.core.Operator.NOT_EQUAL_TO
        then the comparison string is treated as a regular expression.

        Allowed relational operators are
        * cpl.core.Operator.EQUAL_TO
        * cpl.core.Operator.NOT_EQUAL_TO
        * cpl.core.Operator.GREATER_THAN
        * cpl.core.Operator.NOT_GREATER_THAN
        * cpl.core.Operator.LESS_THAN
        * cpl.core.Operator.NOT_LESS_THAN

        Parameters
        ----------
        name : str
            Column name.
        operator : cpl.core.Operator
            Relational Operator. See extended summary for allowed operators.
        string : str
            Reference character string

        Returns
        -------
        int
            New number of selected rows

        Raises
        ------
        cpl.core.DataNotFoundError
            if the column of the given `name` wasn't found.
        cpl.core.TypeMismatchError
            Column of the given `name` is not of type cpl.core.Type.STRING
        cpl.core.IllegalInputError
            Invalid regular expression

        See Also
        --------
        cpl.core.Table.and_selected_string : To select from already selected rows using string comparison
        )pydoc")  // First attempt a string
                  // object, without needing to
                  // bother with the switch
                  // statement and extra casting
      .def(
          "or_selected_numerical",
          [](cpl::core::Table& self, std::string column_name,
             cpl_table_select_operator op, py::object value) -> size {
            size nselected = 0;
            switch (self.get_column_type(column_name)) {
              case CPL_TYPE_LONG_LONG:
                nselected = self.or_selected_long_long(column_name, op,
                                                       value.cast<long long>());
                break;
              case CPL_TYPE_FLOAT:
                nselected = self.or_selected_float(column_name, op,
                                                   value.cast<float>());
                break;
              case CPL_TYPE_DOUBLE:
                nselected = self.or_selected_double(column_name, op,
                                                    value.cast<double>());
                break;
              case CPL_TYPE_INT:
                nselected =
                    self.or_selected_int(column_name, op, value.cast<int>());
                break;
              case CPL_TYPE_FLOAT_COMPLEX:
                nselected = self.or_selected_float_complex(
                    column_name, op, value.cast<std::complex<float>>());
                break;
              case CPL_TYPE_DOUBLE_COMPLEX:
                nselected = self.or_selected_double_complex(
                    column_name, op, value.cast<std::complex<double>>());
                break;
              default:
                throw cpl::core::TypeMismatchError(
                    PYCPL_ERROR_LOCATION,
                    "Selected column is not numerical or is an array type");
                break;
            }
            return nselected;
          },
          py::arg("name"), py::arg("operator"), py::arg("value"), R"pydoc(
        Select from unselected table rows, by comparing a column of numerical values to the reference `value`

        For all the unselected table rows, the values of the specified
        column are compared with the reference value.

        The column is of a numerical type if its type is:
        * cpl.core.Type.INT
        * cpl.core.Type.FLOAT
        * cpl.core.Type.DOUBLE
        * cpl.core.Type.DOUBLE_COMPLEX
        * cpl.core.Type.FLOAT_COMPLEX
        * cpl.core.Type.LONG_LONG

        All table rows fulfilling the comparison are selected. An invalid element never
        fulfills any comparison by definition.
        Allowed relational operators are
        * cpl.core.Operator.EQUAL_TO
        * cpl.core.Operator.NOT_EQUAL_TO
        * cpl.core.Operator.GREATER_THAN
        * cpl.core.Operator.NOT_GREATER_THAN
        * cpl.core.Operator.LESS_THAN
        * cpl.core.Operator.NOT_LESS_THAN

        Parameters
        ----------
        name : str
            Column name.
        operator : cpl.core.Operator
            Relational Operator. See extended summary for allowed operators.
        value : int, float or complex
            Reference value

        Returns
        -------
        int
            New number of selected rows

        Raises
        ------
        cpl.core.DataNotFoundError
            if the column of the given `name` wasn't found.
        cpl.core.TypeMismatchError
            Column of the given `name` is not numerical

        See Also
        --------
        cpl.core.Table.and_selected_numerical : To select from already selected rows using numerical operator comparison
        )pydoc")
      .def("and_selected_string", &cpl::core::Table::and_selected_string,
           py::arg("name"), py::arg("operator"), py::arg("string"), R"pydoc(
        Select from selected table rows, by comparing a column of string values to the given `string`.

        For all the already selected table rows, the values of the specified
        column are compared with the reference string.

        If `operator` is equal to cpl.core.Operator.EQUAL_TO or cpl.core.Operator.NOT_EQUAL_TO
        then the comparison string is treated as a regular expression.

        All table rows not fulfilling the comparison are unselected. An invalid element never
        fulfills any comparison by definition.
        Allowed relational operators are
        * cpl.core.Operator.EQUAL_TO
        * cpl.core.Operator.NOT_EQUAL_TO
        * cpl.core.Operator.GREATER_THAN
        * cpl.core.Operator.NOT_GREATER_THAN
        * cpl.core.Operator.LESS_THAN
        * cpl.core.Operator.NOT_LESS_THAN

        Parameters
        ----------
        name : str
            Column name.
        operator : cpl.core.Operator
            Relational Operator. See extended summary for allowed operators.
        string : str
            Reference character string

        Returns
        -------
        int
            New number of selected rows

        Raises
        ------
        cpl.core.DataNotFoundError
            if the column of the given `name` wasn't found.
        cpl.core.TypeMismatchError
            Column of the given `name` is not of type cpl.core.Type.STRING
        cpl.core.IllegalInputError
            Invalid regular expression

        See Also
        --------
        cpl.core.Table.or_selected_string : To select from unselected rows using string comparison
        )pydoc")
      .def(
          "and_selected_numerical",
          [](cpl::core::Table& self, std::string column_name,
             cpl_table_select_operator op, py::object value) -> size {
            size nselected = 0;
            switch (self.get_column_type(column_name)) {
              case CPL_TYPE_LONG_LONG:
                nselected = self.and_selected_long_long(
                    column_name, op, value.cast<long long>());
                break;
              case CPL_TYPE_FLOAT:
                nselected = self.and_selected_float(column_name, op,
                                                    value.cast<float>());
                break;
              case CPL_TYPE_DOUBLE:
                nselected = self.and_selected_double(column_name, op,
                                                     value.cast<double>());
                break;
              case CPL_TYPE_INT:
                nselected =
                    self.and_selected_int(column_name, op, value.cast<int>());
                break;
              case CPL_TYPE_FLOAT_COMPLEX:
                nselected = self.and_selected_float_complex(
                    column_name, op, value.cast<std::complex<float>>());
                break;
              case CPL_TYPE_DOUBLE_COMPLEX:
                nselected = self.and_selected_double_complex(
                    column_name, op, value.cast<std::complex<double>>());
                break;
              default:
                throw cpl::core::InvalidTypeError(
                    PYCPL_ERROR_LOCATION, "column is not of a numerical type");
                break;
            }
            return nselected;
          },
          py::arg("name"), py::arg("operator"), py::arg("value"), R"pydoc(
        Select from already selected table rows, by comparing a column of numercal values to the reference `value`

        For all the already selected table rows, the values of the specified
        column are compared with the reference value.

        The column is of a numerical type if its type is:
        * cpl.core.Type.INT
        * cpl.core.Type.FLOAT
        * cpl.core.Type.DOUBLE
        * cpl.core.Type.DOUBLE_COMPLEX
        * cpl.core.Type.FLOAT_COMPLEX
        * cpl.core.Type.LONG_LONG

        All table rows not fulfilling the comparison are unselected. An invalid element never
        fulfills any comparison by definition.
        Allowed relational operators are
        * cpl.core.Operator.EQUAL_TO
        * cpl.core.Operator.NOT_EQUAL_TO
        * cpl.core.Operator.GREATER_THAN
        * cpl.core.Operator.NOT_GREATER_THAN
        * cpl.core.Operator.LESS_THAN
        * cpl.core.Operator.NOT_LESS_THAN

        Parameters
        ----------
        name : str
            Column name.
        operator : cpl.core.Operator
            Relational Operator. See extended summary for allowed operators.
        value : int, float or complex
            Reference value

        Returns
        -------
        int
            New number of selected rows

        Raises
        ------
        cpl.core.DataNotFoundError
            if the column of the given `name` wasn't found.
        cpl.core.TypeMismatchError
            Column of the given `name` is not numerical

        See Also
        --------
        cpl.core.Table.or_selected_numerical : To select from unselected rows using numerical operator comparison
        )pydoc")
      .def("and_selected_window", &cpl::core::Table::and_selected_window,
           py::arg("start"), py::arg("count"), R"pydoc(
        Select from selected rows only those within a table segment.

        All the selected table rows that are outside the specified interval are
        unselected. If the sum of `start` and `count` goes beyond the end
        of the input table, rows are checked up to the end of the table.

        Parameters
        ----------
        start : int
            First row of table segment.
        count : int
            Length of segment

        Returns
        -------
        int
            New number of selected rows

        Raises
        ------
        cpl.core.AccessOutOfRange
            `self` has zero length, or `start` is outside `self`'s boundaries
        cpl.core.IllegalInputError
            `count` is negative

        See Also
        --------
        cpl.core.Table.or_selected_window : To select from unselected rows using a specified segment
        )pydoc")
      .def("or_selected_window", &cpl::core::Table::or_selected_window,
           py::arg("start"), py::arg("count"), R"pydoc(
        Select from unselected rows only those within a table segment.

        All the unselected table rows that are within the specified interval are
        selected. If the sum of `start` and `count` goes beyond the end
        of the input table, rows are checked up to the end of the table.

        Parameters
        ----------
        start : int
            First row of table segment.
        count : int
            Length of segment

        Returns
        -------
        int
            New number of selected rows

        Raises
        ------
        cpl.core.AccessOutOfRange
            `self` has zero length, or `start` is outside `self`'s boundaries
        cpl.core.IllegalInputError
            `count` is negative

        See Also
        --------
        cpl.core.Table.and_selected_window : To select from already selected rows using a specified segment
        )pydoc")
      .def("not_selected", &cpl::core::Table::not_selected, R"pydoc(
        Select unselected table rows, and unselect selected ones.

        Returns
        -------
        int
            New number of selected rows
        )pydoc")
      .def("and_selected", &cpl::core::Table::and_selected, py::arg("name1"),
           py::arg("operator"), py::arg("name2"), R"pydoc(
        Select from selected table rows, by comparing the values of two columns.

        Either both columns must be numerical, or they both must be strings.
        The comparison between strings is lexicographical. Neither can be a
        complex or array type.

        For all the already selected table rows, the values of the specified
        column are compared. The table rows not fulfilling the comparison
        are unselected. Invalid elements from either columns never fulfill
        any comparison by definition.

        For this function, the column is of a numerical type if its type is:
        * cpl.core.Type.INT
        * cpl.core.Type.FLOAT
        * cpl.core.Type.DOUBLE
        * cpl.core.Type.LONG_LONG

        All table rows not fulfilling the comparison are unselected. An invalid element never
        fulfills any comparison by definition.
        Allowed relational operators are
        * cpl.core.Operator.EQUAL_TO
        * cpl.core.Operator.NOT_EQUAL_TO
        * cpl.core.Operator.GREATER_THAN
        * cpl.core.Operator.NOT_GREATER_THAN
        * cpl.core.Operator.LESS_THAN
        * cpl.core.Operator.NOT_LESS_THAN

        Parameters
        ----------
        name1 : str
            Name of the first table column
        operator : cpl.core.Operator
            Relational Operator. See extended summary for allowed operators.
        name2 : str
            Name of second table column.

        Returns
        -------
        int
            New number of selected rows

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with any of the specified names is not found in table.
        cpl.core.InvalidTypeError
            Invalid types for comparison.

        See Also
        --------
        cpl.core.Table.or_selected : To select from unselected rows using column comparison
        )pydoc")
      .def("or_selected", &cpl::core::Table::or_selected, py::arg("name1"),
           py::arg("operator"), py::arg("name2"), R"pydoc(
        Select from unselected table rows, by comparing the values of two columns.

        Either both columns must be numerical, or they both must be strings.
        The comparison between strings is lexicographical. Neither can be a
        complex or array type.

        For all unselected table rows, the values of the specified
        column are compared. All the table rows fulfilling the comparison are selected.
        Invalid elements from either columns never fulfill any comparison by definition.

        For this function, the column is of a numerical type if its type is:
        * cpl.core.Type.INT
        * cpl.core.Type.FLOAT
        * cpl.core.Type.DOUBLE
        * cpl.core.Type.LONG_LONG

        Allowed relational operators are
        * cpl.core.Operator.EQUAL_TO
        * cpl.core.Operator.NOT_EQUAL_TO
        * cpl.core.Operator.GREATER_THAN
        * cpl.core.Operator.NOT_GREATER_THAN
        * cpl.core.Operator.LESS_THAN
        * cpl.core.Operator.NOT_LESS_THAN

        Parameters
        ----------
        name1 : str
            Name of the first table column
        operator : cpl.core.Operator
            Relational Operator. See extended summary for allowed operators.
        name2 : str
            Name of second table column.

        Returns
        -------
        int
            New number of selected rows

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with any of the specified names is not found in table.
        cpl.core.InvalidTypeError
            Invalid types for comparison.

        See Also
        --------
        cpl.core.Table.and_selected : To select from already selected rows using column comparison
        )pydoc")
      .def("is_selected", &cpl::core::Table::is_selected, py::arg("row"),
           R"pydoc(
        Determine whether a table row is selected or not.

        Parameters
        ----------
        row : int
            Table row to check.

        Returns
        -------
        bool
            True if row is selected. False if its not.

        Raises
        ------
        cpl.core.DataNotFoundError
            If a column with any of the specified names is not found in table.
        cpl.core.InvalidTypeError
            Invalid types for comparison.

        See Also
        --------
        cpl.core.Table.and_selected : To select from already selected rows using column comparison
        )pydoc"

           )
      .def_property_readonly("selected", &cpl::core::Table::count_selected,
                             "int : number of selected rows in given table.")

      .def(
          "where_selected",
          [](cpl::core::Table& self) -> py::array {
            cpl_array* index_arr = self.where_selected();
            size* sizeData = cpl_array_get_data_cplsize(index_arr);
            py::array return_arr(py::buffer_info(
                sizeData,
                sizeof(size),  // itemsize
                py::format_descriptor<size>::format(),
                1,                                                 // ndim
                std::vector<size>{cpl_array_get_size(index_arr)},  // shape
                std::vector<size>{sizeof(size)}                    // strides
                ));
            cpl_array_delete(index_arr);
            return return_arr;
          },
          R"pydoc(
        Get array of indexes to selected table rows

        Get array of indexes to selected table rows. If no rows are selected,
        an array of size zero is returned.

        Returns
        -------
        list of int
            Indexes to selected table rows
        )pydoc")

      .def("sort", &cpl::core::Table::sort, py::arg("reflist"), R"pydoc(
        Sort table rows according to columns values.

        The table rows are sorted according to the values of the specified
        reference columns. The reference column names are listed in the input

        Parameters
        ----------
        reflist : cpl.core.Propertylist
            Names of reference columns with corresponding sorting mode.
        )pydoc")
      .def("save", &cpl::core::Table::save, py::arg("pheader"),
           py::arg("header"), py::arg("filename"), py::arg("mode"), R"pydoc(
        Save a CPL table to a FITS file.

        This function can be used to convert a CPL table into a binary FITS
        table extension. If the  mode is set to  cpl.core.IO.CREATE, a new
        FITS file will be created containing an empty primary array, with
        just one FITS table extension. An existing (and writable) FITS file
        with the same name would be overwritten. If the  mode flag is set
        to  cpl.core.IO.EXTEND, a new table extension would be appended to an
        existing FITS file. If  mode is set to  cpl.core.IO.APPEND it is possible
        to add rows to the last FITS table extension of the output FITS file.

        Note that the modes  cpl.core.IO.EXTEND and  cpl.core.IO.APPEND require that
        the target file must be writable (and do not take for granted that a file
        is writable just because it was created by the same application,
        as this depends on the system  umask).

        When using the mode  cpl.core.IO.APPEND additional requirements must be
        fulfilled, which are that the column properties like type, format, units,
        etc. must match as the properties of the FITS table extension to which the
        rows should be added exactly. In particular this means that both tables use
        the same null value representation for integral type columns!

        Two property lists may be passed to this function, both
        optionally. The first property list,  pheader, is just used if
        the  mode is set to  cpl.core.IO.CREATE, and it is assumed to
        contain entries for the FITS file primary header. In  pheader any
        property name related to the FITS convention, as ``SIMPLE``, ``BITPIX``,
        ``NAXIS``, ``EXTEND``, ``BLOCKED``, and ``END``, are ignored: such
        entries would be written anyway to the primary header and set to some
        standard values.

        If a no pheader is passed, the primary array would be created
        with just such entries, that are mandatory in any regular FITS file.
        The second property list,  header, is assumed to contain entries
        for the FITS table extension header. In this property list any
        property name related to the FITS convention, as ``XTENSION``,
        ``BITPIX``, ``NAXIS``, ``PCOUNT``, ``GCOUNT``, and ``END``, and to
        the table structure, as ``TFIELDS``, ``TTYPEi``, ``TUNITi``,
        ``TDISPi``, ``TNULLi``, ``TFORMi``, would be ignored: such
        entries are always computed internally, to guarantee their
        consistency with the actual table structure. A ``DATE`` keyword
        containing the date of table creation in ISO8601 format is also
        added automatically.

        Using the mode  cpl.core.IO.APPEND requires that the column properties of
        the table to be appended are compared to the column properties of the
        target FITS extension for each call, which introduces a certain overhead.
        This means that appending a single table row at a time may not be
        efficient and is not recommended. Rather than writing one row at a
        time one should write table chunks containing a suitable number or rows.

        Parameters
        ----------
        pheader : cpl.core.Propertylist
            Primary header entries.
        header : cpl.core.Propertylist
            Table header entries.
        filename : str
            Name of output FITS file.
        mode : unsigned
            Output mode.

        Notes
        -----
        Invalid strings in columns of type  cpl.core.Type.STRING are
        written to FITS as blanks.
        )pydoc")
      .def_static("load", &cpl::core::Table::load, py::arg("filename"),
                  py::arg("xtnum"), py::arg("check_nulls") = true, R"pydoc(
        Load a FITS table extension to generate a new cpl.core.Table

        The selected FITS file table extension is just read and converted into the cpl.core.Table object.

        Parameters
        ----------
        filename : str
            Name of FITS file with at least one table extension.
        xtnum : int
            Number of extension to read, starting from 1.
        check_nulls : bool, optional
            If set to False, identified invalid values are not marked.

        Returns
        -------
        cpl.core.Table
            New cpl.core.Table from loaded data.

        Raises
        ------
        cpl.core.FileNotFoundError
            A file named as specified in `filename` is not found.
        cpl.core.BadFileFormatError
            The input file is not in FITS format.
        cpl.core.IllegalInputError
            The specified FITS file extension is not a table, or, if it is a table, it has more than 9999 columns.
        cpl.core.AccessOutOfRangeError
            `xtnum` is greater than the number of FITS extensions in the FITS file, or is less than 1.
        cpl.core.DataNotFoundError
            The FITS table has no rows or no columns.
        cpl.core.UnspecifiedError
            Generic error condition, that should be reported to the CPL Team.

        See Also
        --------
        cpl.core.Table.load_window : Load part of the FITS table extension
        )pydoc")
      .def_static(
          "load_window",
          [](std::string filename, int xtnum, int start, int nrows,
             bool check_nulls,
             std::vector<std::string> cols) -> cpl::core::Table {
            return cpl::core::Table::load_window(filename, xtnum, check_nulls,
                                                 cols, start, nrows);
          },
          py::arg("filename"), py::arg("xtnum"), py::arg("start"),
          py::arg("nrow"), py::arg("check_nulls") = true,
          py::arg("cols") = std::vector<std::string>(), R"pydoc(
        Load part of a FITS table extension to generate a new cpl.core.Table

        The selected FITS file table extension is just read and converted into the cpl.core.Table object.

        Parameters
        ----------
        filename : str
            Name of FITS file with at least one table extension.
        xtnum : int
            Number of extension to read, starting from 1.
        start : int
            First table row to extract.
        nrow : int
            Number of rows to extract.
        check_nulls : bool, optional
            If set to False, identified invalid values are not marked.
        cols : list of str, optional
            List of the names of the columns to extract. If not given all columns are selected.

        Returns
        -------
        cpl.core.Table
            New cpl.core.Table from loaded data.

        Raises
        ------
        cpl.core.FileNotFoundError
            A file named as specified in `filename` is not found.
        cpl.core.BadFileFormatError
            The input file is not in FITS format.
        cpl.core.IllegalInputError
            The specified FITS file extension is not a table, or, if it is a table, it has more than 9999 columns.
        cpl.core.AccessOutOfRangeError
            `xtnum` is greater than the number of FITS extensions in the FITS file, or is less than 1. Or `start` is either less than zero, or greater than the number of rows in the table.
        cpl.core.DataNotFoundError
            The FITS table has no rows or no columns.
        cpl.core.UnspecifiedError
            Generic error condition, that should be reported to the CPL Team.

        See Also
        --------
        cpl.core.Table.load_window : Load the entire FITS table extension
        )pydoc")
      .def("extract_selected", &cpl::core::Table::extract_selected, R"pydoc(
        Create a new table from the selected rows of another table.

        A new table is created, containing a copy of all the selected
        rows of the input table. In the output table all rows are selected.

        Returns
        -------
        cpl.core.Table
            New cpl.core.Table of selected rows
        )pydoc")
      .def(
          "column_array",
          [](cpl::core::Table& self,
             const std::string& column_name) -> py::object {
            py::object warn = py::module::import("warnings").attr("warn");
            py::object RuntimeWarning =
                py::module::import("builtins").attr("RuntimeWarning");
            cpl_type col_type = self.get_column_type(column_name);
            // FIXME: Each switch statement should just call a template function
            //        instead of this
            switch ((int)col_type) {
                // If 2d array
              case CPL_TYPE_STRING | CPL_TYPE_POINTER: {
                size depth = self.get_column_depth(column_name);
                size rows = self.get_nrow();
                std::vector<std::string> as_vec(depth * rows);
                bool* mask = (bool*)calloc(depth * rows, sizeof(bool));
                int max_len = 0;
                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                      as_vec[i * depth + arr_idx] = std::string();
                      mask[i * depth + arr_idx] = true;
                    }
                  } else {
                    std::pair<const cpl_array*, int> result =
                        self.get_array(column_name, i);
                    if (result.second == 0) {
                      cpl_array* toget = const_cast<cpl_array*>(result.first);
                      char** from_array = cpl_array_get_data_string(toget);
                      for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                        if (from_array != nullptr) {
                          std::string copyme = std::string(from_array[arr_idx]);
                          as_vec[i * depth + arr_idx] = copyme;
                          if (copyme.length() > max_len) {
                            max_len = copyme.length();
                          }

                        } else {
                          as_vec[i * depth + arr_idx] = std::string();
                        }
                      }
                    } else {
                      for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                        as_vec[i * depth + arr_idx] = std::string();
                        mask[i * depth + arr_idx] = true;
                      }
                    }
                  }
                }
                // We cannot use py::buffer_info with format="w"
                // due to numpy/pybind11 bugs. Instead, py::cast
                // detects the string type correctly.
                py::array new_arr = py::cast((as_vec));
                // reshape to a 2D array
                new_arr = new_arr.reshape({rows, depth});
                py::array mask_arr(py::buffer_info(
                    mask,
                    sizeof(bool),  // itemsize
                    py::format_descriptor<bool>::format(),
                    2,                               // ndim
                    std::vector<size>{rows, depth},  // shape
                    std::vector<size>{(size)sizeof(bool) * depth, sizeof(bool)}
                    // strides
                    ));
                free(mask);
                return py::cast(
                    std::pair<py::array, py::array>(new_arr, mask_arr));
                break;
              }

              case CPL_TYPE_LONG_LONG | CPL_TYPE_POINTER: {
                size depth = self.get_column_depth(column_name);
                size rows = self.get_nrow();
                long long* data =
                    (long long*)calloc(depth * rows, sizeof(long long));
                bool* mask = (bool*)calloc(depth * rows, sizeof(bool));
                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                      data[i * depth + arr_idx] = 0;
                      mask[i * depth + arr_idx] = true;
                    }
                  } else {
                    std::pair<const cpl_array*, int> result =
                        self.get_array(column_name, i);
                    if (result.second == 0) {
                      cpl_array* toget = const_cast<cpl_array*>(result.first);
                      long long* from_array =
                          cpl_array_get_data_long_long(toget);
                      std::memcpy(data + i * depth, from_array,
                                  sizeof(double) * depth);
                    } else {
                      for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                        data[i * depth + arr_idx] = 0;
                        mask[i * depth + arr_idx] = true;
                      }
                    }
                  }
                }

                py::array new_arr(py::buffer_info(
                    data,
                    sizeof(long long),  // itemsize
                    py::format_descriptor<long long>::format(),
                    2,                               // ndim
                    std::vector<size>{rows, depth},  // shape
                    std::vector<size>{(size)sizeof(long long) * depth,
                                      sizeof(long long)}  // strides
                    ));
                py::array mask_arr(py::buffer_info(
                    mask,
                    sizeof(bool),  // itemsize
                    py::format_descriptor<bool>::format(),
                    2,                               // ndim
                    std::vector<size>{rows, depth},  // shape
                    std::vector<size>{(size)sizeof(bool) * depth, sizeof(bool)}
                    // strides
                    ));

                free(data);
                free(mask);
                return py::cast(
                    std::pair<py::array, py::array>(new_arr, mask_arr));
                break;
              }

              case CPL_TYPE_INT | CPL_TYPE_POINTER: {
                size depth = self.get_column_depth(column_name);
                size rows = self.get_nrow();
                int* data = (int*)calloc(depth * rows, sizeof(int));
                bool* mask = (bool*)calloc(depth * rows, sizeof(bool));
                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                      data[i * depth + arr_idx] = 0;
                      mask[i * depth + arr_idx] = true;
                    }
                  } else {
                    std::pair<const cpl_array*, int> result =
                        self.get_array(column_name, i);
                    if (result.second == 0) {
                      cpl_array* toget = const_cast<cpl_array*>(result.first);
                      int* from_array = cpl_array_get_data_int(toget);
                      std::memcpy(data + i * depth, from_array,
                                  sizeof(int) * depth);
                    } else {
                      for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                        data[i * depth + arr_idx] = 0;
                        mask[i * depth + arr_idx] = true;
                      }
                    }
                  }
                }

                py::array new_arr(py::buffer_info(
                    data,
                    sizeof(int),  // itemsize
                    py::format_descriptor<int>::format(),
                    2,                               // ndim
                    std::vector<size>{rows, depth},  // shape
                    std::vector<size>{(size)sizeof(int) * depth, sizeof(int)}
                    // strides
                    ));
                py::array mask_arr(py::buffer_info(
                    mask,
                    sizeof(bool),  // itemsize
                    py::format_descriptor<bool>::format(),
                    2,                               // ndim
                    std::vector<size>{rows, depth},  // shape
                    std::vector<size>{(size)sizeof(bool) * depth, sizeof(bool)}
                    // strides
                    ));

                free(data);
                free(mask);
                return py::cast(
                    std::pair<py::array, py::array>(new_arr, mask_arr));
                break;
              }
              case CPL_TYPE_DOUBLE | CPL_TYPE_POINTER: {
                size depth = self.get_column_depth(column_name);
                size rows = self.get_nrow();
                double* data = (double*)calloc(depth * rows, sizeof(double));
                bool* mask = (bool*)calloc(depth * rows, sizeof(bool));
                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                      data[i * depth + arr_idx] = 0.0;
                      mask[i * depth + arr_idx] = true;
                    }
                  } else {
                    std::pair<const cpl_array*, int> result =
                        self.get_array(column_name, i);
                    if (result.second == 0) {
                      cpl_array* toget = const_cast<cpl_array*>(result.first);
                      double* from_array = cpl_array_get_data_double(toget);
                      std::memcpy(data + i * depth, from_array,
                                  sizeof(double) * depth);
                    } else {
                      for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                        data[i * depth + arr_idx] = 0.0;
                        mask[i * depth + arr_idx] = true;
                      }
                    }
                  }
                }


                py::array new_arr(py::buffer_info(
                    data,
                    sizeof(double),  // itemsize
                    py::format_descriptor<double>::format(),
                    2,                               // ndim
                    std::vector<size>{rows, depth},  // shape
                    std::vector<size>{(size)sizeof(double) * depth,
                                      sizeof(double)}  // strides
                    ));
                py::array mask_arr(py::buffer_info(
                    mask,
                    sizeof(bool),  // itemsize
                    py::format_descriptor<bool>::format(),
                    2,                               // ndim
                    std::vector<size>{rows, depth},  // shape
                    std::vector<size>{(size)sizeof(bool) * depth, sizeof(bool)}
                    // strides
                    ));

                free(data);
                free(mask);
                return py::cast(
                    std::pair<py::array, py::array>(new_arr, mask_arr));
                break;
              }
              case CPL_TYPE_FLOAT | CPL_TYPE_POINTER:  // Float array
              {
                size depth = self.get_column_depth(column_name);
                size rows = self.get_nrow();
                float* data = (float*)calloc(depth * rows, sizeof(float));
                bool* mask = (bool*)calloc(depth * rows, sizeof(bool));

                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                      data[i * depth + arr_idx] = 0.0;
                      mask[i * depth + arr_idx] = true;
                    }

                  } else {
                    std::pair<const cpl_array*, int> result =
                        self.get_array(column_name, i);
                    if (result.second == 0) {
                      cpl_array* toget = const_cast<cpl_array*>(result.first);
                      float* from_array = cpl_array_get_data_float(toget);
                      std::memcpy(data + i * depth, from_array,
                                  sizeof(float) * depth);
                    } else {
                      for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                        data[i * depth + arr_idx] = 0.0;
                        mask[i * depth + arr_idx] = true;
                      }
                    }
                  }
                }
                py::array new_arr(py::buffer_info(
                    data,
                    sizeof(float),  // itemsize
                    py::format_descriptor<float>::format(),
                    2,                               // ndim
                    std::vector<size>{rows, depth},  // shape
                    std::vector<size>{(size)sizeof(float) * depth,
                                      sizeof(float)}  // strides
                    ));
                py::array mask_arr(py::buffer_info(
                    mask,
                    sizeof(bool),  // itemsize
                    py::format_descriptor<bool>::format(),
                    2,                               // ndim
                    std::vector<size>{rows, depth},  // shape
                    std::vector<size>{(size)sizeof(bool) * depth, sizeof(bool)}
                    // strides
                    ));

                free(data);
                free(mask);
                return py::cast(
                    std::pair<py::array, py::array>(new_arr, mask_arr));

                break;
              }
              case CPL_TYPE_DOUBLE_COMPLEX | CPL_TYPE_POINTER: {
                size depth = self.get_column_depth(column_name);
                size rows = self.get_nrow();
                std::vector<std::complex<double>> as_vec(depth * rows);
                bool* mask = (bool*)calloc(depth * rows, sizeof(bool));
                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                      as_vec[i * depth + arr_idx] =
                          std::complex<double>(0.0, 0.0);
                      mask[i * depth + arr_idx] = true;
                    }
                  } else {
                    std::pair<const cpl_array*, int> result =
                        self.get_array(column_name, i);
                    if (result.second == 0) {
                      cpl_array* toget = const_cast<cpl_array*>(result.first);
                      double _Complex* from_array =
                          cpl_array_get_data_double_complex(toget);
                      for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                        as_vec[i * depth + arr_idx] =
                            cpl::core::complexd_to_cpp(
                                from_array[arr_idx]);  // Row pos + array index
                                                       // offset
                      }
                    } else {
                      for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                        as_vec[i * depth + arr_idx] =
                            std::complex<double>(0.0, 0.0);
                        mask[i * depth + arr_idx] = true;
                      }
                    }
                  }
                }
                py::array new_arr(py::buffer_info(
                    as_vec.data(),
                    sizeof(std::complex<double>),  // itemsize
                    py::format_descriptor<std::complex<double>>::format(),
                    2,                               // ndim
                    std::vector<size>{rows, depth},  // shape
                    std::vector<size>{(size)sizeof(std::complex<double>) *
                                          depth,
                                      sizeof(std::complex<double>)}  // strides
                    ));
                py::array mask_arr(py::buffer_info(
                    mask,
                    sizeof(bool),  // itemsize
                    py::format_descriptor<bool>::format(),
                    2,                               // ndim
                    std::vector<size>{rows, depth},  // shape
                    std::vector<size>{(size)sizeof(bool) * depth, sizeof(bool)}
                    // strides
                    ));
                free(mask);
                return py::cast(
                    std::pair<py::array, py::array>(new_arr, mask_arr));
                break;
              }
              case CPL_TYPE_FLOAT_COMPLEX | CPL_TYPE_POINTER: {
                size depth = self.get_column_depth(column_name);
                size rows = self.get_nrow();
                std::vector<std::complex<float>> as_vec(depth * rows);
                bool* mask = (bool*)calloc(depth * rows, sizeof(bool));
                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                      as_vec[i * depth + arr_idx] =
                          std::complex<float>(0.0, 0.0);
                      mask[i * depth + arr_idx] = true;
                    }
                  } else {
                    std::pair<const cpl_array*, int> result =
                        self.get_array(column_name, i);
                    if (result.second == 0) {
                      cpl_array* toget = const_cast<cpl_array*>(result.first);
                      float _Complex* from_array =
                          cpl_array_get_data_float_complex(toget);
                      for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                        as_vec[i * depth + arr_idx] =
                            cpl::core::complexf_to_cpp(
                                from_array[arr_idx]);  // Row pos + array index
                                                       // offset
                      }
                    } else {
                      for (int arr_idx = 0; arr_idx < depth; arr_idx++) {
                        as_vec[i * depth + arr_idx] =
                            std::complex<float>(0.0, 0.0);
                        mask[i * depth + arr_idx] = true;
                      }
                    }
                  }
                }
                py::array new_arr(py::buffer_info(
                    as_vec.data(),
                    sizeof(std::complex<float>),  // itemsize
                    py::format_descriptor<std::complex<float>>::format(),
                    2,                               // ndim
                    std::vector<size>{rows, depth},  // shape
                    std::vector<size>{(size)sizeof(std::complex<float>) * depth,
                                      sizeof(std::complex<float>)}  // strides
                    ));
                py::array mask_arr(py::buffer_info(
                    mask,
                    sizeof(bool),  // itemsize
                    py::format_descriptor<bool>::format(),
                    2,                               // ndim
                    std::vector<size>{rows, depth},  // shape
                    std::vector<size>{(size)sizeof(bool) * depth, sizeof(bool)}
                    // strides
                    ));
                free(mask);
                return py::cast(
                    std::pair<py::array, py::array>(new_arr, mask_arr));

                break;
              }
              // For the column itself.
              case CPL_TYPE_INT: {
                std::pair<int*, int> result = self.get_data<int>(column_name);
                size rows = self.get_nrow();
                std::vector<int> avec(rows, 0);
                py::array ares = py::cast(avec);
                std::vector<bool> mask_vec(rows, false);

                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    mask_vec[i] = true;
                  }
                }

                if (result.second == 0) {
                  ares = py::array(py::buffer_info(
                      result.first,
                      sizeof(int),  // itemsize
                      py::format_descriptor<int>::format(),
                      1,                                   // ndim
                      std::vector<size>{self.get_nrow()},  // shape
                      std::vector<size>{sizeof(int)}       // strides
                      ));
                } else {
                  // an error occurred reading in the column data, set all to be
                  // invalid
                  for (int i = 0; i < rows; i++) {
                    mask_vec[i] = true;
                  }
                }

                py::array mask_arr = py::cast(mask_vec);
                return py::cast(
                    std::pair<py::array, py::array>(ares, mask_arr));
                break;
              }
              case CPL_TYPE_FLOAT: {
                std::pair<float*, int> result =
                    self.get_data<float>(column_name);
                size rows = self.get_nrow();
                std::vector<float> avec(rows, 0.0);
                py::array ares = py::cast(avec);
                std::vector<bool> mask_vec(rows, false);

                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    mask_vec[i] = true;
                  }
                }
                if (result.second == 0) {
                  ares = py::array(py::buffer_info(
                      result.first,
                      sizeof(float),  // itemsize
                      py::format_descriptor<float>::format(),
                      1,                                   // ndim
                      std::vector<size>{self.get_nrow()},  // shape
                      std::vector<size>{sizeof(float)}     // strides
                      ));
                } else {
                  // an error occurred reading in the column data, set all to be
                  // invalid
                  for (int i = 0; i < rows; i++) {
                    mask_vec[i] = true;
                  }
                }

                py::array mask_arr = py::cast(mask_vec);
                return py::cast(
                    std::pair<py::array, py::array>(ares, mask_arr));
                break;
              }
              case CPL_TYPE_DOUBLE: {
                std::pair<double*, int> result =
                    self.get_data<double>(column_name);
                size rows = self.get_nrow();
                std::vector<double> avec(rows, 0.0);
                py::array ares = py::cast(avec);
                std::vector<bool> mask_vec(rows, false);

                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    mask_vec[i] = true;
                  }
                }

                if (result.second == 0) {
                  ares = py::array(py::buffer_info(
                      result.first,
                      sizeof(double),  // itemsize
                      py::format_descriptor<double>::format(),
                      1,                                   // ndim
                      std::vector<size>{self.get_nrow()},  // shape
                      std::vector<size>{sizeof(double)}    // strides
                      ));
                } else {
                  // an error occurred reading in the column data, set all to be
                  // invalid
                  for (int i = 0; i < rows; i++) {
                    mask_vec[i] = true;
                  }
                }

                py::array mask_arr = py::cast(mask_vec);
                return py::cast(
                    std::pair<py::array, py::array>(ares, mask_arr));
                break;
              }
              case CPL_TYPE_FLOAT_COMPLEX: {
                std::pair<float _Complex*, int> result =
                    self.get_data<float _Complex>(column_name);
                size rows = self.get_nrow();
                std::vector<std::complex<float>> avec(
                    rows, std::complex<float>(0.0, 0.0));
                py::array ares = py::cast(avec);
                std::vector<bool> mask_vec(rows, false);

                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    mask_vec[i] = true;
                  }
                }
                if (result.second == 0) {
                  std::vector<std::complex<float>> as_vec(self.get_nrow());
                  for (int i = 0; i < self.get_nrow(); i++) {
                    as_vec[i] = cpl::core::complexf_to_cpp(result.first[i]);
                  }
                  ares = py::array(py::buffer_info(
                      as_vec.data(),
                      sizeof(std::complex<float>),  // itemsize
                      py::format_descriptor<std::complex<float>>::format(),
                      1,                                              // ndim
                      std::vector<size>{self.get_nrow()},             // shape
                      std::vector<size>{sizeof(std::complex<float>)}  // strides
                      ));
                } else {
                  // an error occurred reading in the column data, set all to be
                  // invalid
                  for (int i = 0; i < rows; i++) {
                    mask_vec[i] = true;
                  }
                }
                py::array mask_arr = py::cast(mask_vec);
                return py::cast(
                    std::pair<py::array, py::array>(ares, mask_arr));
                break;
              }

              case CPL_TYPE_DOUBLE_COMPLEX: {
                std::pair<double _Complex*, int> result =
                    self.get_data<double _Complex>(column_name);
                size rows = self.get_nrow();
                std::vector<std::complex<double>> avec(
                    rows, std::complex<double>(0.0, 0.0));
                py::array ares = py::cast(avec);
                std::vector<bool> mask_vec(rows, false);

                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    mask_vec[i] = true;
                  }
                }
                if (result.second == 0) {
                  std::vector<std::complex<double>> as_vec(self.get_nrow());
                  for (int i = 0; i < self.get_nrow(); i++) {
                    as_vec[i] = cpl::core::complexd_to_cpp(result.first[i]);
                  }
                  ares = py::array(py::buffer_info(
                      as_vec.data(),
                      sizeof(std::complex<double>),  // itemsize
                      py::format_descriptor<std::complex<double>>::format(),
                      1,                                   // ndim
                      std::vector<size>{self.get_nrow()},  // shape
                      std::vector<size>{sizeof(std::complex<double>)}
                      // strides
                      ));
                } else {
                  // an error occurred reading in the column data, set all to be
                  // invalid
                  for (int i = 0; i < rows; i++) {
                    mask_vec[i] = true;
                  }
                }

                py::array mask_arr = py::cast(mask_vec);
                return py::cast(
                    std::pair<py::array, py::array>(ares, mask_arr));
                break;
              }
              case CPL_TYPE_LONG_LONG: {
                std::pair<long long*, int> result =
                    self.get_data<long long>(column_name);
                size rows = self.get_nrow();
                std::vector<long long> avec(rows, 0);
                py::array ares = py::cast(avec);
                std::vector<bool> mask_vec(rows, false);

                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    mask_vec[i] = true;
                  }
                }

                if (result.second == 0) {
                  ares = py::array(py::buffer_info(
                      result.first,
                      sizeof(long long),  // itemsize
                      py::format_descriptor<long long>::format(),
                      1,                                    // ndim
                      std::vector<size>{self.get_nrow()},   // shape
                      std::vector<size>{sizeof(long long)}  // strides
                      ));
                } else {
                  // an error occurred reading in the column data, set all to be
                  // invalid
                  for (int i = 0; i < rows; i++) {
                    mask_vec[i] = true;
                  }
                }

                py::array mask_arr = py::cast(mask_vec);
                return py::cast(
                    std::pair<py::array, py::array>(ares, mask_arr));
                break;
              }

              case CPL_TYPE_STRING: {
                // If we're copying anyway just use one of our older functions
                std::pair<char**, int> result =
                    self.get_data_string(column_name);
                size rows = self.get_nrow();
                std::vector<std::string> avec(rows, std::string());
                py::array ares = py::cast(avec);
                std::vector<bool> mask_vec(rows, false);

                for (int i = 0; i < rows; i++) {
                  if (!self.is_valid(column_name, i)) {
                    mask_vec[i] = true;
                  }
                }
                if (result.second == 0) {
                  std::vector<std::string> as_vec(self.get_nrow());
                  for (int i = 0; i < self.get_nrow(); i++) {
                    if (result.first[i] != nullptr) {
                      as_vec[i] = std::string(result.first[i]);
                    } else {
                      // handle case where string in row i was not set
                      as_vec[i] = std::string();
                    }
                  }
                  ares = py::array(py::cast(as_vec));
                } else {
                  // an error occurred reading in the column data, set all to be
                  // invalid
                  for (int i = 0; i < rows; i++) {
                    mask_vec[i] = true;
                  }
                }

                py::array mask_arr = py::cast(mask_vec);
                return py::cast(
                    std::pair<py::array, py::array>(ares, mask_arr));
                break;
              }
              default: {
                throw cpl::core::InvalidTypeError(
                    PYCPL_ERROR_LOCATION,
                    "column is of invalid type, cannot be cast to numpy array");
              }
            }
          },
          py::arg("name"), R"pydoc(
        Get column data in the form of a `numpy.ndarray`

        The array will be returned with the corresponding type as the column, containing in order all the values contained within column `name`.

        If the column is an array type, then the array returned will be 2d, with each nested array represents a value.

        Parameters
        ----------
        `name` : str
            column to extract values

        Returns
        -------
        numpy.ndarray
            array of values contained within column `name` in `self`.

        Raises
        ------
        cpl.core.DataNotFoundError
            If column `name` does not exist in `self`
        cpl.core.InvalidTypeError
            If the column type cannot be cast to a numpy array
        )pydoc")
      .def(
          "to_records",
          [](cpl::core::Table& self) -> py::object {
            auto locals = py::dict("to_convert"_a = self);
            py::exec(R"(
                        import numpy as np
                        arrays=[]
                        for name in to_convert.column_names:
                                arrays.append(np.array(to_convert[name]))
                        output=np.rec.fromarrays(arrays, names=to_convert.column_names)
                )",
                     py::globals(), locals);
            py::object returnObject = locals["output"];
            return returnObject;
          },
          R"pydoc(
        Convert the cpl.core.Table to a numpy recarray

        Returns
        -------
        numpy.recarray
            numpy recarray containing entries and values from the cpl.core.Table
        )pydoc");
}
