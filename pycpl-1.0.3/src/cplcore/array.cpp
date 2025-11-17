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

#include "cplcore/array.hpp"

#include <string>

#include "cplcore/types.hpp"

namespace cpl
{
namespace core
{

using size = cpl::core::size;

// TODO: possibly make this templated
std::unique_ptr<_cpl_array_, void* (*)(cpl_array*)>
vector_as_temp_array_int(std::vector<int>& to_wrap)
{
  return std::unique_ptr<_cpl_array_, void* (*)(cpl_array*)>(
      cpl_array_wrap_int(&to_wrap[0], to_wrap.size()), &cpl_array_unwrap);
}

// The wrapper/unwrapper normally require a non-const argument,
// but I know in this case it'll be fine to const-cast, since with a const
// cpl_array, there's no way for anyone to modify the casted non-const
// arguments
const void*
cpl_array_unwrap_const(const cpl_array* a)
{
  return cpl_array_unwrap(const_cast<cpl_array*>(a));
}

const cpl_array*
cpl_array_wrap_int_const(const int* data, size n)
{
  return cpl_array_wrap_int(const_cast<int*>(data), n);
}

std::unique_ptr<const _cpl_array_, const void* (*)(const cpl_array*)>
vector_as_temp_array_int(const std::vector<int>& to_wrap)
{
  return std::unique_ptr<const _cpl_array_, const void* (*)(const cpl_array*)>(
      cpl_array_wrap_int_const(&to_wrap[0], to_wrap.size()),
      &cpl_array_unwrap_const);
}

template <>
std::vector<int>
cpl_array_as_vector<int>(cpl_array* input)
{
  int* res_data = cpl_array_get_data_int(input);  // Store array data in vector
  size array_size = cpl_array_get_size(input);
  return std::vector<int>(res_data, res_data + array_size);
}

template <>
std::vector<double>
cpl_array_as_vector<double>(cpl_array* input)
{
  double* res_data =
      cpl_array_get_data_double(input);  // Store array data in vector
  size array_size = cpl_array_get_size(input);

  return std::vector<double>(res_data, res_data + array_size);
}

template <>
std::vector<std::string>
cpl_array_as_vector<std::string>(cpl_array* input)
{
  char** res_data =
      cpl_array_get_data_string(input);  // Store array data in vector
  size array_size = cpl_array_get_size(input);
  std::vector<std::string> as_vec(array_size);
  for (int i = 0; i < array_size; i++) {
    as_vec[i] = std::string(res_data[i]);
  }

  return as_vec;
}

}  // namespace core
}  // namespace cpl