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

#ifndef PYHDRL_CORE_PARAMETER_HPP_
#define PYHDRL_CORE_PARAMETER_HPP_

#include <hdrl_parameter.h>

#include <memory>

namespace hdrl
{
namespace core
{

/**
 * Owns an hdrl_parameter with shared ownership so copies share the underlying
 * pointer safely (required for Python bindings and HDRL C APIs that only
 * borrow).
 */
class Parameter
{
 public:
  Parameter();
  explicit Parameter(hdrl_parameter* to_steal);
  /** Wrap an existing shared_ptr (e.g. borrowed lifetime from Collapse). */
  explicit Parameter(std::shared_ptr<hdrl_parameter> impl);
  hdrl_parameter* ptr();
  const hdrl_parameter* ptr() const;

 private:
  std::shared_ptr<hdrl_parameter> m_interface;
};


}  // namespace core
}  // namespace hdrl

#endif  // PYHDRL_CORE_PARAMETER_HPP_