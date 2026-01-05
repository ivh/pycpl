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

#ifndef PYHDRL_CORE_PARAMETER_HPP_
#define PYHDRL_CORE_PARAMETER_HPP_

#include <hdrl_parameter.h>

namespace hdrl
{
namespace core
{

class Parameter
{
 public:
  Parameter();
  Parameter(hdrl_parameter* to_steal);
  // ParameterType get_type();
  // Destructor hdrl_parameter_delete / hdrl_parameter_destroy
  hdrl_parameter* ptr();
  // To add some operator= overloading to allow parameter to be copied/assigned
  // ? Parameter& operator=(const Parameter& other);
 protected:
  hdrl_parameter* m_interface;
};


}  // namespace core
}  // namespace hdrl

#endif  // PYHDRL_CORE_PARAMETER_HPP_