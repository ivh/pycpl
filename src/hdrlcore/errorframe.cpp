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

// Wrapper class used to manage CPL's internal errors and recipe errors

#include "hdrlcore/errorframe.hpp"

#include <sstream>
#include <string>

namespace hdrl
{
namespace core
{
ErrorFrame::ErrorFrame(cpl_error_code code, const std::string& function_name,
                       const std::string& file_name, unsigned int line,
                       const std::string& error_message)
    :  // m_code(code),
      m_line(line), m_code(code), m_function_name(function_name),
      m_file_name(file_name), m_error_message(error_message)
{
  // Create m_full_message (Like python traceback info)
  std::ostringstream full_message;
  full_message << "  File \"" << get_file_name() << "\", line " << get_line()
               << ", in " << get_function_name() <<
      // Format of error message defined as Default message: Custom text
      // in cpl_error_set_message_one_macro
      "\n" << get_error_message();
  m_full_message = full_message.str();
}

unsigned int
ErrorFrame::get_line() const noexcept
{
  return m_line;
}

cpl_error_code
ErrorFrame::get_code() const noexcept
{
  return m_code;
}

std::string
ErrorFrame::get_function_name() const noexcept
{
  return m_function_name;
}

std::string
ErrorFrame::get_file_name() const noexcept
{
  return m_file_name;
}

std::string
ErrorFrame::get_error_message() const noexcept
{
  return m_error_message;
}

const char*
ErrorFrame::what() const noexcept
{
  return m_full_message.c_str();
}

bool
ErrorFrame::operator==(const ErrorFrame& other) const noexcept
{
  return get_code() == other.get_code() && get_line() == other.get_line() &&
         get_function_name() == other.get_function_name() &&
         get_file_name() == other.get_file_name() &&
         get_error_message() == other.get_error_message();
}

bool
ErrorFrame::operator!=(const ErrorFrame& other) const noexcept
{
  return !operator==(other);
}

}  // namespace core
}  // namespace hdrl
