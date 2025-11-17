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

#include "cplcore/table.hpp"

#include <complex>
#include <cstdio>
#include <cstring>

#include <cpl_memory.h>

#include "cplcore/array.hpp"
#include "cplcore/error.hpp"

namespace cpl
{
namespace core
{
Table::Table(size length)
    : m_interface(Error::throw_errors_with(cpl_table_new, length))
{
}

Table::Table(cpl_table* to_steal) noexcept : m_interface(to_steal) {}

void
Table::new_column(const std::string& name, cpl_type type)
{
  Error::throw_errors_with(cpl_table_new_column, m_interface, name.c_str(),
                           type);
}

// TOCONSULT
void
Table::new_column_array(const std::string& name, cpl_type type, size depth)
{
  Error::throw_errors_with(cpl_table_new_column_array, m_interface,
                           name.c_str(), type, depth);
}

void
Table::wrap_int(std::vector<int>& data, const std::string& name)
{
  int* data_copy = (int*)cpl_calloc(get_nrow(), sizeof(int));
  std::memcpy(data_copy, &data[0],
              data.size() * sizeof(int));  // copy entire block of memory
  std::memset(&data_copy[data.size()],
              0,  // setting the elements of data_copy past the end of data to 0
              (get_nrow() - data.size()) * sizeof(int));
  Error::throw_errors_with(cpl_table_wrap_int, m_interface, data_copy,
                           name.c_str());
}

void
Table::wrap_long_long(std::vector<long long>& data, const std::string& name)
{
  long long* data_copy = (long long*)cpl_calloc(get_nrow(), sizeof(long long));
  std::memcpy(data_copy, &data[0], data.size() * sizeof(long long));
  std::memset(&data_copy[data.size()], 0,
              (get_nrow() - data.size()) * sizeof(long long));
  Error::throw_errors_with(cpl_table_wrap_long_long, m_interface, data_copy,
                           name.c_str());
}

void
Table::wrap_float(std::vector<float> data, const std::string& name)
{
  float* data_copy = (float*)cpl_calloc(get_nrow(), sizeof(float));
  std::memcpy(data_copy, &data[0], data.size() * sizeof(float));
  std::memset(&data_copy[data.size()], 0,
              (get_nrow() - data.size()) * sizeof(float));
  Error::throw_errors_with(cpl_table_wrap_float, m_interface, data_copy,
                           name.c_str());
}

void
Table::wrap_double(std::vector<double>& data, const std::string& name)
{
  double* data_copy = (double*)cpl_calloc(get_nrow(), sizeof(double));
  std::memcpy(data_copy, &data[0], data.size() * sizeof(double));
  std::memset(&data_copy[data.size()], 0,
              (get_nrow() - data.size()) * sizeof(double));
  Error::throw_errors_with(cpl_table_wrap_double, m_interface, data_copy,
                           name.c_str());
}

void
Table::wrap_float_complex(std::vector<std::complex<float>> data,
                          const std::string& name)
{
  float _Complex* data_copy =
      (float _Complex*)cpl_calloc(get_nrow(), sizeof(float _Complex));
  std::memcpy(data_copy, &data[0], data.size() * sizeof(float _Complex));
  std::memset(&data_copy[data.size()], 0,
              (get_nrow() - data.size()) * sizeof(float _Complex));
  Error::throw_errors_with(cpl_table_wrap_float_complex, m_interface, data_copy,
                           name.c_str());
}

void
Table::wrap_double_complex(std::vector<std::complex<double>> data,
                           const std::string& name)
{
  double _Complex* input_ptr =
      (double _Complex*)cpl_calloc(data.size(), sizeof(double _Complex));
  std::memcpy(input_ptr, &data[0], data.size() * sizeof(double _Complex));
  cpl::core::Error::throw_errors_with(cpl_table_wrap_double_complex,
                                      m_interface, input_ptr, name.c_str());
}

void
Table::wrap_string(const std::vector<std::string>& data,
                   const std::string& name)
{
  char** cstrings = (char**)cpl_calloc(data.size(), sizeof(char*));
  for (int i = 0; i < data.size(); i++) {
    cstrings[i] =
        strdup(data[i].c_str());  // returns pointer to a string, which is a
                                  // duplicate of the string
  }
  Error::throw_errors_with(cpl_table_wrap_string, m_interface, cstrings,
                           name.c_str());
}

// void* Table::unwrap(const std::string& name) {
//     Error::throw_errors_with(cpl_table_unwrap,m_interface,name.c_str());
// }
void
Table::copy_structure(const Table& other)
{
  Error::throw_errors_with(cpl_table_copy_structure, m_interface,
                           other.m_interface);
}

Table::~Table() { Error::throw_errors_with(cpl_table_delete, m_interface); }

size
Table::get_nrow() const
{
  return Error::throw_errors_with(cpl_table_get_nrow, m_interface);
}

size
Table::get_ncol() const
{
  return Error::throw_errors_with(cpl_table_get_ncol, m_interface);
}

std::pair<std::complex<double>, int>
Table::get_complex_double(const std::string& name, size row)
{
  int flag;
  std::complex<double> ret = complexd_to_cpp(Error::throw_errors_with(
      cpl_table_get_complex, m_interface, name.c_str(), row, &flag));

  return std::pair(ret, flag);
}

std::pair<std::complex<float>, int>
Table::get_complex_float(const std::string& name, size row)
{
  int flag;
  std::complex<float> ret = complexf_to_cpp(Error::throw_errors_with(
      cpl_table_get_complex, m_interface, name.c_str(), row, &flag));

  return std::pair(ret, flag);
}

cpl_type
Table::get_column_type(const std::string& name) const
{
  return Error::throw_errors_with(cpl_table_get_column_type, m_interface,
                                  name.c_str());
}

void
Table::set_column_unit(const std::string& name, const std::string* unit_ptr)
{
  const char* unit = unit_ptr == nullptr ? nullptr : unit_ptr->c_str();
  Error::throw_errors_with(cpl_table_set_column_unit, m_interface, name.c_str(),
                           unit);
}

std::optional<std::string>
Table::get_column_unit(const std::string& name) const
{
  const char* nullable_unit = Error::throw_errors_with(
      cpl_table_get_column_unit, m_interface, name.c_str());
  if (nullable_unit == nullptr) {
    return {};
  } else {
    return std::string(nullable_unit);
  }
}

void
Table::set_column_format(const std::string& name, const std::string* format_ptr)
{
  const char* format = format_ptr == nullptr ? nullptr : format_ptr->c_str();
  Error::throw_errors_with(cpl_table_set_column_format, m_interface,
                           name.c_str(), format);
}

std::string
Table::get_column_format(const std::string& name) const
{
  // This doesn't reutrn NULL unless an error was thrown
  return std::string(Error::throw_errors_with(cpl_table_get_column_format,
                                              m_interface, name.c_str()));
}

void
Table::set_column_depth(const std::string& name, size depth)
{
  Error::throw_errors_with(cpl_table_set_column_depth, m_interface,
                           name.c_str(), depth);
}

size
Table::get_column_depth(const std::string& name) const
{
  return Error::throw_errors_with(cpl_table_get_column_depth, m_interface,
                                  name.c_str());
}

size
Table::get_column_dimensions(const std::string& name) const
{
  return Error::throw_errors_with(cpl_table_get_column_dimensions, m_interface,
                                  name.c_str());
}

void
Table::set_column_dimensions(const std::string& name,
                             const std::vector<int>& dimensions)
{
  auto dims_ptr = cpl::core::vector_as_temp_array_int(dimensions);
  Error::throw_errors_with(cpl_table_set_column_dimensions, m_interface,
                           name.c_str(), dims_ptr.get());
}

size
Table::get_column_dimension(const std::string& name, size indx) const
{
  return Error::throw_errors_with(cpl_table_get_column_dimension, m_interface,
                                  name.c_str(), indx);
}

void
Table::set(const std::string& name, size row, double value)
{
  Error::throw_errors_with(cpl_table_set, m_interface, name.c_str(), row,
                           value);
}

void
Table::set_complex(const std::string& name, size row,
                   std::complex<double> value)
{
  Error::throw_errors_with(cpl_table_set_complex, m_interface, name.c_str(),
                           row, cpl::core::complex_to_c(value));
}

void
Table::set_string(const std::string& name, size row, const std::string& value)
{
  Error::throw_errors_with(cpl_table_set_string, m_interface, name.c_str(), row,
                           value.c_str());
}

void
Table::set_array(const std::string& name, size row, const cpl_array* array)
{
  Error::throw_errors_with(cpl_table_set_array, m_interface, name.c_str(), row,
                           array);
}

void
Table::fill_column_window(const std::string& name, size start, size count,
                          double value)
{
  Error::throw_errors_with(cpl_table_fill_column_window, m_interface,
                           name.c_str(), start, count, value);
}

void
Table::fill_column_window_int(const std::string& name, size start, size count,
                              int value)
{
  Error::throw_errors_with(cpl_table_fill_column_window_int, m_interface,
                           name.c_str(), start, count, value);
}

void
Table::fill_column_window_long_long(const std::string& name, size start,
                                    size count, long long value)
{
  Error::throw_errors_with(cpl_table_fill_column_window_long_long, m_interface,
                           name.c_str(), start, count, value);
}

void
Table::fill_column_window_float(const std::string& name, size start, size count,
                                float value)
{
  Error::throw_errors_with(cpl_table_fill_column_window_float, m_interface,
                           name.c_str(), start, count, value);
}

void
Table::fill_column_window_double(const std::string& name, size start,
                                 size count, double value)
{
  Error::throw_errors_with(cpl_table_fill_column_window_double, m_interface,
                           name.c_str(), start, count, value);
}

void
Table::fill_column_window_complex(const std::string& name, size start,
                                  size count, std::complex<double> value)
{
  Error::throw_errors_with(cpl_table_fill_column_window_complex, m_interface,
                           name.c_str(), start, count,
                           cpl::core::complex_to_c(value));
}

void
Table::fill_column_window_float_complex(const std::string& name, size start,
                                        size count, std::complex<float> value)
{
  Error::throw_errors_with(cpl_table_fill_column_window_float_complex,
                           m_interface, name.c_str(), start, count,
                           cpl::core::complex_to_c(value));
}

void
Table::fill_column_window_double_complex(const std::string& name, size start,
                                         size count, std::complex<double> value)
{
  Error::throw_errors_with(cpl_table_fill_column_window_double_complex,
                           m_interface, name.c_str(), start, count,
                           cpl::core::complex_to_c(value));
}

void
Table::fill_column_window_string(const std::string& name, size start,
                                 size count, const std::string& value)
{
  Error::throw_errors_with(cpl_table_fill_column_window_string, m_interface,
                           name.c_str(), start, count, value.c_str());
}

void
Table::fill_column_window_array(const std::string& name, size start, size count,
                                const cpl_array* array)
{
  Error::throw_errors_with(cpl_table_fill_column_window_array, m_interface,
                           name.c_str(), start, count, array);
}

void
Table::copy_data_int(const std::string& name, std::vector<int>& data)
{
  if (data.size() != get_nrow()) {
    // Data is not the same length as the number of table rows
  }
  Error::throw_errors_with(cpl_table_copy_data_int, m_interface, name.c_str(),
                           &data[0]);
}

void
Table::copy_data_long_long(const std::string& name,
                           std::vector<long long>& data)
{
  Error::throw_errors_with(cpl_table_copy_data_long_long, m_interface,
                           name.c_str(), &data[0]);
}

void
Table::copy_data_float(const std::string& name, std::vector<float>& data)
{
  Error::throw_errors_with(cpl_table_copy_data_float, m_interface, name.c_str(),
                           &data[0]);
}

void
Table::copy_data_double(const std::string& name, std::vector<double>& data)
{
  Error::throw_errors_with(cpl_table_copy_data_double, m_interface,
                           name.c_str(), &data[0]);
}

void
Table::copy_data_float_complex(const std::string& name,
                               std::vector<std::complex<float>>& data)
{
  float _Complex* data_copy =
      (float _Complex*)cpl_calloc(get_nrow(), sizeof(float _Complex));
  std::memcpy(data_copy, &data[0], data.size() * sizeof(float _Complex));
  std::memset(&data_copy[data.size()], 0,
              (get_nrow() - data.size()) * sizeof(float _Complex));
  Error::throw_errors_with(cpl_table_copy_data_float_complex, m_interface,
                           name.c_str(), data_copy);
  cpl_free(data_copy);
}

void
Table::copy_data_double_complex(const std::string& name,
                                std::vector<std::complex<double>>& data)
{
  double _Complex* data_copy =
      (double _Complex*)cpl_calloc(get_nrow(), sizeof(double _Complex));
  std::memcpy(data_copy, &data[0], data.size() * sizeof(double _Complex));
  std::memset(&data_copy[data.size()], 0,
              (get_nrow() - data.size()) * sizeof(double _Complex));
  Error::throw_errors_with(cpl_table_copy_data_double_complex, m_interface,
                           name.c_str(), data_copy);
  cpl_free(data_copy);
}

void
Table::copy_data_string(const std::string& name, std::vector<std::string>& data)
{
  const char** cstrings =
      (const char**)cpl_calloc(data.size(), sizeof(const char*));
  for (int i = 0; i < data.size(); i++) {
    cstrings[i] = data[i].c_str();
  }
  Error::throw_errors_with(cpl_table_copy_data_string, m_interface,
                           name.c_str(), cstrings);
  cpl_free(cstrings);
}

void
Table::shift_column(const std::string& name, size shift)
{
  Error::throw_errors_with(cpl_table_shift_column, m_interface, name.c_str(),
                           shift);
}

void
Table::set_invalid(const std::string& name, size row)
{
  Error::throw_errors_with(cpl_table_set_invalid, m_interface, name.c_str(),
                           row);
}

void
Table::set_column_invalid(const std::string& name, size start, size count)
{
  Error::throw_errors_with(cpl_table_set_column_invalid, m_interface,
                           name.c_str(), start, count);
}

bool
Table::is_valid(const std::string& name, size row) const
{
  return Error::throw_errors_with(cpl_table_is_valid, m_interface, name.c_str(),
                                  row);
}

size
Table::count_invalid(const std::string& name) const
{
  return Error::throw_errors_with(cpl_table_count_invalid, m_interface,
                                  name.c_str());
}

bool
Table::has_invalid(const std::string& name) const
{
  return Error::throw_errors_with(cpl_table_has_invalid, m_interface,
                                  name.c_str());
}

bool
Table::has_valid(const std::string& name) const
{
  return Error::throw_errors_with(cpl_table_has_valid, m_interface,
                                  name.c_str());
}

void
Table::fill_invalid_int(const std::string& name, int code)
{
  Error::throw_errors_with(cpl_table_fill_invalid_int, m_interface,
                           name.c_str(), code);
}

void
Table::fill_invalid_long_long(const std::string& name, long long code)
{
  Error::throw_errors_with(cpl_table_fill_invalid_long_long, m_interface,
                           name.c_str(), code);
}

void
Table::fill_invalid_float(const std::string& name, float code)
{
  Error::throw_errors_with(cpl_table_fill_invalid_float, m_interface,
                           name.c_str(), code);
}

void
Table::fill_invalid_double(const std::string& name, double code)
{
  Error::throw_errors_with(cpl_table_fill_invalid_double, m_interface,
                           name.c_str(), code);
}

void
Table::fill_invalid_float_complex(const std::string& name,
                                  std::complex<float> code)
{
  Error::throw_errors_with(cpl_table_fill_invalid_float_complex, m_interface,
                           name.c_str(), cpl::core::complex_to_c(code));
}

void
Table::fill_invalid_double_complex(const std::string& name,
                                   std::complex<double> code)
{
  Error::throw_errors_with(cpl_table_fill_invalid_double_complex, m_interface,
                           name.c_str(), cpl::core::complex_to_c(code));
}

void
Table::erase_column(const std::string& name)
{
  Error::throw_errors_with(cpl_table_erase_column, m_interface, name.c_str());
}

void
Table::move_column(const std::string& name, Table& from_table)
{
  Error::throw_errors_with(cpl_table_move_column, m_interface, name.c_str(),
                           from_table.m_interface);
}

void
Table::duplicate_column(const std::string& to_name, const Table& from_table,
                        const std::string& from_name)
{
  Error::throw_errors_with(cpl_table_duplicate_column, m_interface,
                           to_name.c_str(), from_table.m_interface,
                           from_name.c_str());
}

std::shared_ptr<Table>
Table::duplicate()
{
  return std::make_shared<Table>(
      Error::throw_errors_with(cpl_table_duplicate, m_interface));
}

void
Table::name_column(const std::string& from_name, const std::string& to_name)
{
  Error::throw_errors_with(cpl_table_name_column, m_interface,
                           from_name.c_str(), to_name.c_str());
}

int
Table::has_column(const std::string& name) const
{
  return Error::throw_errors_with(cpl_table_has_column, m_interface,
                                  name.c_str());
}

std::vector<std::string>
Table::get_column_names() const
{
  cpl_array* names_arr =
      Error::throw_errors_with(cpl_table_get_column_names, m_interface);
  std::vector<std::string> output;
  size num_names = cpl_array_get_size(names_arr);
  for (size i = 0; i < num_names; ++i) {
    output.emplace_back(
        cpl_array_get_string(names_arr, i));  // insert new element
  }
  cpl_array_delete(names_arr);
  return output;
}

void
Table::set_size(size new_length)
{
  Error::throw_errors_with(cpl_table_set_size, m_interface, new_length);
}

Table::Table(const Table& other)
    : m_interface(
          Error::throw_errors_with(cpl_table_duplicate, other.m_interface))
{
}

Table
Table::extract(size start, size count) const
{
  return Table(
      Error::throw_errors_with(cpl_table_extract, m_interface, start, count));
}

Table
Table::extract_selected() const
{
  return Table(
      Error::throw_errors_with(cpl_table_extract_selected, m_interface));
}

cpl_array*
Table::where_selected() const
{
  return Error::throw_errors_with(cpl_table_where_selected, m_interface);
}

void
Table::erase_selected()
{
  Error::throw_errors_with(cpl_table_erase_selected, m_interface);
}

void
Table::erase_window(size start, size count)
{
  Error::throw_errors_with(cpl_table_erase_window, m_interface, start, count);
}

void
Table::insert_window(size start, size count)
{
  Error::throw_errors_with(cpl_table_insert_window, m_interface, start, count);
}

bool
Table::compare_structure(const Table& table2) const
{
  // Weird situation where 0 is true, but 1 is false? guess we can just use a
  // not operator to fix
  return !(bool)Error::throw_errors_with(cpl_table_compare_structure,
                                         m_interface, table2.m_interface);
}

void
Table::insert(const Table& insert_table, size row)
{
  Error::throw_errors_with(cpl_table_insert, m_interface,
                           insert_table.m_interface, row);
}

void
Table::cast_column(const std::string& from_name, const std::string& to_name,
                   cpl_type type)
{
  Error::throw_errors_with(cpl_table_cast_column, m_interface,
                           from_name.c_str(), to_name.c_str(), type);
}

void
Table::add_columns(const std::string& to_name, const std::string& from_name)
{
  Error::throw_errors_with(cpl_table_add_columns, m_interface, to_name.c_str(),
                           from_name.c_str());
}

void
Table::subtract_columns(const std::string& to_name,
                        const std::string& from_name)
{
  Error::throw_errors_with(cpl_table_subtract_columns, m_interface,
                           to_name.c_str(), from_name.c_str());
}

void
Table::multiply_columns(const std::string& to_name,
                        const std::string& from_name)
{
  Error::throw_errors_with(cpl_table_multiply_columns, m_interface,
                           to_name.c_str(), from_name.c_str());
}

void
Table::divide_columns(const std::string& to_name, const std::string& from_name)
{
  Error::throw_errors_with(cpl_table_divide_columns, m_interface,
                           to_name.c_str(), from_name.c_str());
}

void
Table::add_scalar(const std::string& name, double value)
{
  Error::throw_errors_with(cpl_table_add_scalar, m_interface, name.c_str(),
                           value);
}

void
Table::subtract_scalar(const std::string& name, double value)
{
  Error::throw_errors_with(cpl_table_subtract_scalar, m_interface, name.c_str(),
                           value);
}

void
Table::multiply_scalar(const std::string& name, double value)
{
  Error::throw_errors_with(cpl_table_multiply_scalar, m_interface, name.c_str(),
                           value);
}

void
Table::divide_scalar(const std::string& name, double value)
{
  Error::throw_errors_with(cpl_table_divide_scalar, m_interface, name.c_str(),
                           value);
}

void
Table::add_scalar_complex(const std::string& name, std::complex<double> value)
{
  Error::throw_errors_with(cpl_table_add_scalar_complex, m_interface,
                           name.c_str(), cpl::core::complex_to_c(value));
}

void
Table::subtract_scalar_complex(const std::string& name,
                               std::complex<double> value)
{
  Error::throw_errors_with(cpl_table_subtract_scalar_complex, m_interface,
                           name.c_str(), cpl::core::complex_to_c(value));
}

void
Table::multiply_scalar_complex(const std::string& name,
                               std::complex<double> value)
{
  Error::throw_errors_with(cpl_table_multiply_scalar_complex, m_interface,
                           name.c_str(), cpl::core::complex_to_c(value));
}

void
Table::divide_scalar_complex(const std::string& name,
                             std::complex<double> value)
{
  Error::throw_errors_with(cpl_table_divide_scalar_complex, m_interface,
                           name.c_str(), cpl::core::complex_to_c(value));
}

void
Table::abs_column(const std::string& name)
{
  Error::throw_errors_with(cpl_table_abs_column, m_interface, name.c_str());
}

void
Table::logarithm_column(const std::string& name, double base)
{
  Error::throw_errors_with(cpl_table_logarithm_column, m_interface,
                           name.c_str(), base);
}

void
Table::power_column(const std::string& name, double exponent)
{
  Error::throw_errors_with(cpl_table_power_column, m_interface, name.c_str(),
                           exponent);
}

void
Table::exponential_column(const std::string& name, double base)
{
  Error::throw_errors_with(cpl_table_exponential_column, m_interface,
                           name.c_str(), base);
}

void
Table::conjugate_column(const std::string& name)
{
  Error::throw_errors_with(cpl_table_conjugate_column, m_interface,
                           name.c_str());
}

void
Table::real_column(const std::string& name)
{
  Error::throw_errors_with(cpl_table_real_column, m_interface, name.c_str());
}

void
Table::imag_column(const std::string& name)
{
  Error::throw_errors_with(cpl_table_imag_column, m_interface, name.c_str());
}

void
Table::arg_column(const std::string& name)
{
  Error::throw_errors_with(cpl_table_arg_column, m_interface, name.c_str());
}

void
Table::erase_invalid_rows()
{
  Error::throw_errors_with(cpl_table_erase_invalid_rows, m_interface);
}

void
Table::erase_invalid()
{
  Error::throw_errors_with(cpl_table_erase_invalid, m_interface);
}

double
Table::get_column_max(const std::string& name) const
{
  return Error::throw_errors_with(cpl_table_get_column_max, m_interface,
                                  name.c_str());
}

double
Table::get_column_min(const std::string& name) const
{
  return Error::throw_errors_with(cpl_table_get_column_min, m_interface,
                                  name.c_str());
}

size
Table::get_column_maxpos(const std::string& name) const
{
  size row;
  Error::throw_errors_with(cpl_table_get_column_maxpos, m_interface,
                           name.c_str(), &row);
  return row;
}

size
Table::get_column_minpos(const std::string& name) const
{
  size row;
  Error::throw_errors_with(cpl_table_get_column_minpos, m_interface,
                           name.c_str(), &row);
  return row;
}

double
Table::get_column_mean(const std::string& name) const
{
  return Error::throw_errors_with(cpl_table_get_column_mean, m_interface,
                                  name.c_str());
}

std::complex<double>
Table::get_column_mean_complex(const std::string& name) const
{
  return complexd_to_cpp(Error::throw_errors_with(
      cpl_table_get_column_mean_complex, m_interface, name.c_str()));
}

double
Table::get_column_median(const std::string& name) const
{
  return Error::throw_errors_with(cpl_table_get_column_median, m_interface,
                                  name.c_str());
}

double
Table::get_column_stdev(const std::string& name) const
{
  return Error::throw_errors_with(cpl_table_get_column_stdev, m_interface,
                                  name.c_str());
}

void
Table::sort(const PropertyList& reflist)
{
  Error::throw_errors_with(cpl_table_sort, m_interface, reflist.ptr().get());
}

size
Table::and_selected_window(size start, size count)
{
  return Error::throw_errors_with(cpl_table_and_selected_window, m_interface,
                                  start, count);
}

size
Table::or_selected_window(size start, size count)
{
  return Error::throw_errors_with(cpl_table_or_selected_window, m_interface,
                                  start, count);
}

size
Table::not_selected()
{
  return Error::throw_errors_with(cpl_table_not_selected, m_interface);
}

void
Table::select_row(size row)
{
  Error::throw_errors_with(cpl_table_select_row, m_interface, row);
}

void
Table::unselect_row(size row)
{
  Error::throw_errors_with(cpl_table_unselect_row, m_interface, row);
}

void
Table::select_all()
{
  Error::throw_errors_with(cpl_table_select_all, m_interface);
}

void
Table::unselect_all()
{
  Error::throw_errors_with(cpl_table_unselect_all, m_interface);
}

size
Table::and_selected_invalid(const std::string& name)
{
  return Error::throw_errors_with(cpl_table_and_selected_invalid, m_interface,
                                  name.c_str());
}

size
Table::or_selected_invalid(const std::string& name)
{
  return Error::throw_errors_with(cpl_table_or_selected_invalid, m_interface,
                                  name.c_str());
}

size
Table::and_selected_int(const std::string& name, cpl_table_select_operator op,
                        int value)
{
  return Error::throw_errors_with(cpl_table_and_selected_int, m_interface,
                                  name.c_str(), op, value);
}

size
Table::or_selected_int(const std::string& name, cpl_table_select_operator op,
                       int value)
{
  return Error::throw_errors_with(cpl_table_or_selected_int, m_interface,
                                  name.c_str(), op, value);
}

size
Table::and_selected_long_long(const std::string& name,
                              cpl_table_select_operator op, long long value)
{
  return Error::throw_errors_with(cpl_table_and_selected_long_long, m_interface,
                                  name.c_str(), op, value);
}

size
Table::or_selected_long_long(const std::string& name,
                             cpl_table_select_operator op, long long value)
{
  return Error::throw_errors_with(cpl_table_or_selected_long_long, m_interface,
                                  name.c_str(), op, value);
}

size
Table::and_selected_float(const std::string& name, cpl_table_select_operator op,
                          float value)
{
  return Error::throw_errors_with(cpl_table_and_selected_float, m_interface,
                                  name.c_str(), op, value);
}

size
Table::or_selected_float(const std::string& name, cpl_table_select_operator op,
                         float value)
{
  return Error::throw_errors_with(cpl_table_or_selected_float, m_interface,
                                  name.c_str(), op, value);
}

size
Table::and_selected_double(const std::string& name,
                           cpl_table_select_operator op, double value)
{
  return Error::throw_errors_with(cpl_table_and_selected_double, m_interface,
                                  name.c_str(), op, value);
}

size
Table::or_selected_double(const std::string& name, cpl_table_select_operator op,
                          double value)
{
  return Error::throw_errors_with(cpl_table_or_selected_double, m_interface,
                                  name.c_str(), op, value);
}

size
Table::and_selected_float_complex(const std::string& name,
                                  cpl_table_select_operator op,
                                  std::complex<float> value)
{
  return Error::throw_errors_with(cpl_table_and_selected_float_complex,
                                  m_interface, name.c_str(), op,
                                  cpl::core::complex_to_c(value));
}

size
Table::or_selected_float_complex(const std::string& name,
                                 cpl_table_select_operator op,
                                 std::complex<float> value)
{
  return Error::throw_errors_with(cpl_table_or_selected_float_complex,
                                  m_interface, name.c_str(), op,
                                  cpl::core::complex_to_c(value));
}

size
Table::and_selected_double_complex(const std::string& name,
                                   cpl_table_select_operator op,
                                   std::complex<double> value)
{
  return Error::throw_errors_with(cpl_table_and_selected_double_complex,
                                  m_interface, name.c_str(), op,
                                  cpl::core::complex_to_c(value));
}

size
Table::or_selected_double_complex(const std::string& name,
                                  cpl_table_select_operator op,
                                  std::complex<double> value)
{
  return Error::throw_errors_with(cpl_table_or_selected_double_complex,
                                  m_interface, name.c_str(), op,
                                  cpl::core::complex_to_c(value));
}

size
Table::and_selected_string(const std::string& name,
                           cpl_table_select_operator op,
                           const std::string& string)
{
  return Error::throw_errors_with(cpl_table_and_selected_string, m_interface,
                                  name.c_str(), op, string.c_str());
}

size
Table::or_selected_string(const std::string& name, cpl_table_select_operator op,
                          const std::string& string)
{
  return Error::throw_errors_with(cpl_table_or_selected_string, m_interface,
                                  name.c_str(), op, string.c_str());
}

size
Table::and_selected(const std::string& name1, cpl_table_select_operator op,
                    const std::string& name2)
{
  return Error::throw_errors_with(cpl_table_and_selected, m_interface,
                                  name1.c_str(), op, name2.c_str());
}

size
Table::or_selected(const std::string& name1, cpl_table_select_operator op,
                   const std::string& name2)
{
  return Error::throw_errors_with(cpl_table_or_selected, m_interface,
                                  name1.c_str(), op, name2.c_str());
}

bool
Table::is_selected(size row) const
{
  return (bool)Error::throw_errors_with(cpl_table_is_selected, m_interface,
                                        row);
}

size
Table::count_selected() const
{
  return Error::throw_errors_with(cpl_table_count_selected, m_interface);
}

std::string
Table::dump_structure() const
{
  char* charBuff;
  size_t len;
  // Open char pointer as stream
  FILE* stream = open_memstream(&charBuff, &len);
  Error::throw_errors_with(cpl_table_dump_structure, m_interface, stream);
  // Flush to char pointer
  fflush(stream);
  // Cast to std::string
  std::string returnString(charBuff);
  fclose(stream);
  free(charBuff);

  return returnString;
}

std::string
Table::dump(size start, size count) const
{
  char* charBuff;
  size_t len;
  // Open char pointer as stream
  FILE* stream = open_memstream(&charBuff, &len);
  Error::throw_errors_with(cpl_table_dump, m_interface, start, count, stream);

  // Flush to char pointer
  fflush(stream);
  // Cast to std::string
  std::string returnString(charBuff);
  fclose(stream);
  free(charBuff);

  return returnString;
}

Table
Table::load(const std::string& filename, int xtnum, bool check_nulls)
{
  return Table(Error::throw_errors_with(cpl_table_load, filename.c_str(), xtnum,
                                        check_nulls));
}

Table
Table::load_window(const std::string& filename, int xtnum, bool check_nulls,
                   const std::vector<std::string>& selcol_vector, size firstrow,
                   size nrow)
{
  if (selcol_vector.size() == 0) {
    return Table(Error::throw_errors_with(cpl_table_load_window,
                                          filename.c_str(), xtnum, check_nulls,
                                          (cpl_array*)NULL, firstrow, nrow));
  }
  cpl_array* selcol = cpl_array_new(selcol_vector.size(), CPL_TYPE_STRING);
  for (int i = 0; i < selcol_vector.size(); i++) {
    cpl_array_set_string(selcol, i, selcol_vector[i].c_str());
  }
  Table retTable(Error::throw_errors_with(cpl_table_load_window,
                                          filename.c_str(), xtnum, check_nulls,
                                          selcol, firstrow, nrow));
  cpl_array_delete(selcol);
  return retTable;
}

void
Table::save(std::optional<cpl::core::PropertyList> pheader,
            const cpl::core::PropertyList& header, const std::string& filename,
            unsigned mode) const
{
  auto headerptr = header.ptr();

  if (auto ph = pheader) {
    auto pheader_ptr = ph->ptr();
    Error::throw_errors_with(cpl_table_save, m_interface, pheader_ptr.get(),
                             headerptr.get(), filename.c_str(), mode);

  } else {
    Error::throw_errors_with(cpl_table_save, m_interface, nullptr,
                             headerptr.get(), filename.c_str(), mode);
  }
}

std::pair<int, int>
Table::get_int(const std::string& name, size row)
{
  int flag;
  int ret = Error::throw_errors_with(cpl_table_get_int, m_interface,
                                     name.c_str(), row, &flag);


  return std::pair(ret, flag);
}

std::pair<float, int>
Table::get_float(const std::string& name, size row)
{
  int flag;
  float ret = Error::throw_errors_with(cpl_table_get_float, m_interface,
                                       name.c_str(), row, &flag);
  return std::pair(ret, flag);
}

std::pair<double, int>
Table::get(const std::string& name, size row)
{
  int flag;
  double ret = Error::throw_errors_with(cpl_table_get, m_interface,
                                        name.c_str(), row, &flag);
  return std::pair(ret, flag);
}

std::pair<double, int>
Table::get_double(const std::string& name, size row)
{
  int flag;
  double ret = Error::throw_errors_with(cpl_table_get_double, m_interface,
                                        name.c_str(), row, &flag);
  return std::pair(ret, flag);
}

std::pair<long long, int>
Table::get_long_long(const std::string& name, size row)
{
  int flag;

  long long ret = Error::throw_errors_with(cpl_table_get_long_long, m_interface,
                                           name.c_str(), row, &flag);
  return std::pair(ret, flag);
}

std::pair<std::string, int>
Table::get_string(const std::string& name, size row)
{
  const char* result = Error::throw_errors_with(cpl_table_get_string,
                                                m_interface, name.c_str(), row);
  if (result == nullptr) {
    return std::pair(std::string(), 1);
  } else {
    return std::pair(std::string(result), 0);
  }
  return std::pair(std::string(), 0);
}

std::pair<const cpl_array*, int>
Table::get_array(const std::string& name, size row)
{
  const cpl_array* result = Error::throw_errors_with(
      cpl_table_get_array, m_interface, name.c_str(), row);
  return std::pair(result, (result == nullptr) ? 1 : 0);
}

template <>
std::pair<int*, int>
Table::get_data<int>(const std::string& name)
{
  int* result = Error::throw_errors_with(cpl_table_get_data_int, m_interface,
                                         name.c_str());
  return std::pair(result, (result == nullptr) ? 1 : 0);
}

template <>
std::pair<float*, int>
Table::get_data<float>(const std::string& name)
{
  float* result = Error::throw_errors_with(cpl_table_get_data_float,
                                           m_interface, name.c_str());
  return std::pair(result, (result == nullptr) ? 1 : 0);
}

template <>
std::pair<long long*, int>
Table::get_data<long long>(const std::string& name)
{
  long long* result = Error::throw_errors_with(cpl_table_get_data_long_long,
                                               m_interface, name.c_str());
  return std::pair(result, (result == nullptr) ? 1 : 0);
}

template <>
std::pair<double*, int>
Table::get_data<double>(const std::string& name)
{
  double* result = Error::throw_errors_with(cpl_table_get_data_double,
                                            m_interface, name.c_str());
  return std::pair(result, (result == nullptr) ? 1 : 0);
}

template <>
std::pair<double _Complex*, int>
Table::get_data<double _Complex>(const std::string& name)
{
  double _Complex* result = Error::throw_errors_with(
      cpl_table_get_data_double_complex, m_interface, name.c_str());
  return std::pair(result, (result == nullptr) ? 1 : 0);
}

template <>
std::pair<float _Complex*, int>
Table::get_data<float _Complex>(const std::string& name)
{
  float _Complex* result = Error::throw_errors_with(
      cpl_table_get_data_float_complex, m_interface, name.c_str());
  return std::pair(result, (result == nullptr) ? 1 : 0);
}

std::pair<char**, int>
Table::get_data_string(const std::string& name)
{
  char** result = Error::throw_errors_with(cpl_table_get_data_string,
                                           m_interface, name.c_str());
  return std::pair(result, (result == nullptr) ? 1 : 0);
}

const cpl_table*
Table::ptr() const
{
  return m_interface;
}

cpl_table*
Table::unwrap(Table&& self)
{
  cpl_table* interface = self.m_interface;
  self.m_interface = nullptr;
  return interface;
}
}  // namespace core
}  // namespace cpl
