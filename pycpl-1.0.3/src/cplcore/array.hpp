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

#ifndef PYCPL_ARRAY_HPP
#define PYCPL_ARRAY_HPP

#include <memory>
#include <vector>

#include <cpl_array.h>

namespace cpl
{
namespace core
{
const void* cpl_array_unwrap_const(const cpl_array* a);

std::unique_ptr<_cpl_array_, void* (*)(cpl_array*)>
vector_as_temp_array_int(std::vector<int>& to_wrap);

std::unique_ptr<const _cpl_array_, const void* (*)(const cpl_array*)>
vector_as_temp_array_int(const std::vector<int>& to_wrap);

// Creates a vector copy of the cpl_array
template <class T>
std::vector<T> cpl_array_as_vector(cpl_array* input);

}  // namespace core
}  // namespace cpl

#endif  // PYCPL_ARRAY_HPP