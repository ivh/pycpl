// This file is part of PyCPL the ESO CPL Python language bindings
// Copyright (C) 2020-2026 European Southern Observatory
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

#ifndef PYCPL_ARRAY_HPP
#define PYCPL_ARRAY_HPP

#include <memory>
#include <string>
#include <vector>

#include <cpl_array.h>

#include "cplcore/types.hpp"

namespace cpl
{
namespace core
{

using size = cpl::core::size;

inline cpl_array*
cpl_array_wrap(int* data, cpl_size length)
{
  return cpl_array_wrap_int(data, length);
}

inline const cpl_array*
cpl_array_wrap(const int* data, cpl_size length)
{
  return cpl_array_wrap_int(const_cast<int*>(data), length);
}

inline cpl_array*
cpl_array_wrap(long long* data, cpl_size length)
{
  return cpl_array_wrap_long_long(data, length);
}

inline const cpl_array*
cpl_array_wrap(const long long* data, cpl_size length)
{
  return cpl_array_wrap_long_long(const_cast<long long*>(data), length);
}

inline cpl_array*
cpl_array_wrap(float* data, cpl_size length)
{
  return cpl_array_wrap_float(data, length);
}

inline const cpl_array*
cpl_array_wrap(const float* data, cpl_size length)
{
  return cpl_array_wrap_float(const_cast<float*>(data), length);
}

inline cpl_array*
cpl_array_wrap(double* data, cpl_size length)
{
  return cpl_array_wrap_double(data, length);
}

inline const cpl_array*
cpl_array_wrap(const double* data, cpl_size length)
{
  return cpl_array_wrap_double(const_cast<double*>(data), length);
}

inline cpl_array*
cpl_array_wrap(float _Complex* data, cpl_size length)
{
  return cpl_array_wrap_float_complex(data, length);
}

inline const cpl_array*
cpl_array_wrap(const float _Complex* data, cpl_size length)
{
  return cpl_array_wrap_float_complex(const_cast<float _Complex*>(data),
                                      length);
}

inline cpl_array*
cpl_array_wrap(double _Complex* data, cpl_size length)
{
  return cpl_array_wrap_double_complex(data, length);
}

inline const cpl_array*
cpl_array_wrap(const double _Complex* data, cpl_size length)
{
  return cpl_array_wrap_double_complex(const_cast<double _Complex*>(data),
                                       length);
}

inline cpl_array*
cpl_array_wrap(char** data, cpl_size length)
{
  return cpl_array_wrap_string(data, length);
}

inline const cpl_array*
cpl_array_wrap(const char** data, cpl_size length)
{
  return cpl_array_wrap_string(const_cast<char**>(data), length);
}

// The wrapper/unwrapper normally require a non-const argument,
// but in this case it will be fine to const-cast, since with a const
// cpl_array, there's no way for anyone to modify the casted non-const
// arguments
inline const void*
cpl_array_unwrap_const(const cpl_array* array)
{
  return cpl_array_unwrap(const_cast<cpl_array*>(array));
}

using array_view = std::unique_ptr<_cpl_array_, decltype(cpl_array_unwrap)*>;
using const_array_view =
    std::unique_ptr<const _cpl_array_, decltype(cpl_array_unwrap_const)*>;

template <typename T>
array_view
make_array_view(std::vector<T>& vector)
{
  return array_view(cpl_array_wrap(vector.data(), vector.size()),
                    cpl_array_unwrap);
}

template <typename T>
const_array_view
make_array_view(const std::vector<T>& vector)
{
  return const_array_view(cpl_array_wrap(vector.data(), vector.size()),
                          cpl_array_unwrap_const);
}

template <typename T>
T* cpl_array_get_data(cpl_array* /* unused*/) = delete;

template <>
int* cpl_array_get_data(cpl_array* array);
template <>
long long* cpl_array_get_data(cpl_array* array);
template <>
float* cpl_array_get_data(cpl_array* array);
template <>
double* cpl_array_get_data(cpl_array* array);
template <>
float _Complex* cpl_array_get_data(cpl_array* array);
template <>
double _Complex* cpl_array_get_data(cpl_array* array);

// Creates a vector copy of the cpl_array
template <typename T>
std::vector<T>
cpl_array_as_vector(cpl_array* input)
{
  T* array_data = cpl_array_get_data<T>(input);  // Store array data in vector
  size array_size = cpl_array_get_size(input);

  return std::vector<T>(array_data, array_data + array_size);
}

template <>
std::vector<std::string> cpl_array_as_vector<std::string>(cpl_array* input);

}  // namespace core
}  // namespace cpl

#endif  // PYCPL_ARRAY_HPP