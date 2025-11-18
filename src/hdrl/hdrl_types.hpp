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

#ifndef PYCPL_HDRL_TYPES_HPP
#define PYCPL_HDRL_TYPES_HPP

#include <hdrl_types.h>

namespace cpl
{
namespace hdrl
{

/**
 * @brief A simple value-error pair structure used throughout HDRL
 */
struct Value
{
  double data;
  double error;

  Value() : data(0.0), error(0.0) {}
  Value(double d, double e) : data(d), error(e) {}
  explicit Value(hdrl_value v) : data(v.data), error(v.error) {}

  operator hdrl_value() const
  {
    hdrl_value v;
    v.data = data;
    v.error = error;
    return v;
  }
};

}  // namespace hdrl
}  // namespace cpl

#endif  // PYCPL_HDRL_TYPES_HPP
