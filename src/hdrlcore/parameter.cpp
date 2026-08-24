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

#include "hdrlcore/parameter.hpp"

#include <hdrl_parameter.h>

namespace hdrl
{
namespace core
{

namespace
{
void
delete_hdrl_parameter(hdrl_parameter* p)
{
  if (p != nullptr) {
    hdrl_parameter_delete(p);
  }
}
}  // namespace

Parameter::Parameter() = default;

Parameter::Parameter(hdrl_parameter* to_steal)
    : m_interface(to_steal, delete_hdrl_parameter)
{
}

Parameter::Parameter(std::shared_ptr<hdrl_parameter> impl)
    : m_interface(std::move(impl))
{
}

hdrl_parameter*
Parameter::ptr()
{
  return m_interface.get();
}

const hdrl_parameter*
Parameter::ptr() const
{
  return m_interface.get();
}

}  // namespace core
}  // namespace hdrl
