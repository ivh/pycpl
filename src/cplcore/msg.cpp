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

#include "cplcore/msg.hpp"

#include "cplcore/error.hpp"

namespace cpl
{
namespace core
{

int Msg::current_indentation = 0;
int Msg::current_width = 0;
bool Msg::display_thread_id = false;
bool Msg::display_domain = false;
bool Msg::display_time = false;
bool Msg::display_component = false;

void
Msg::start_log(cpl_msg_severity verbosity,
               const std::filesystem::path& filename)
{
  stop_log();  // Close previous logger
  Error::throw_errors_with(cpl_msg_set_log_name, filename.c_str());
  Error::throw_errors_with(cpl_msg_set_log_level,
                           verbosity);  // Start logging to file
}

void
Msg::stop_log()
{
  Error::throw_errors_with(cpl_msg_stop_log);  // Close previous logger
}

std::filesystem::path
Msg::get_log_name()
{
  return std::filesystem::path(
      std::string(Error::throw_errors_with(cpl_msg_get_log_name)));
}

void
Msg::set_level(int verbosity)
{
  cpl_msg_severity toSet;

  // For compatibility with the python logging module levels,
  // map logging library levels to CPL levels
  if (verbosity > CPL_MSG_OFF) {
    switch (verbosity) {
      // TODO: logging.NOTSET and logging.CRITICAL not accounted for with no cpl
      // equivalents, vice versa for CPL_MSG_OFF
      case 10:  // logging.DEBUG
        toSet = CPL_MSG_DEBUG;
        break;
      case 20:  // logging.INFO
        toSet = CPL_MSG_INFO;
        break;
      case 30:  // logging.WARNING
        toSet = CPL_MSG_WARNING;
        break;
      case 40:  // logging.ERROR
        toSet = CPL_MSG_ERROR;
        break;
      default:
        throw IllegalInputError(PYCPL_ERROR_LOCATION,
                                std::to_string(verbosity) +
                                    " is invalid verbosity value");
        break;
    }
  } else if (verbosity >= 0) {
    // Straight cast from int: since its most likely coming from the pycpl
    // enums
    toSet = (cpl_msg_severity)verbosity;
  } else {
    throw IllegalInputError(PYCPL_ERROR_LOCATION,
                            std::to_string(verbosity) +
                                " is invalid verbosity value");
  }
  Error::throw_errors_with(cpl_msg_set_level, toSet);
}

cpl_msg_severity
Msg::get_level()
{
  return Error::throw_errors_with(cpl_msg_get_level);
}

void
Msg::set_domain(const std::string& name)
{
  Error::throw_errors_with(cpl_msg_set_domain, name.c_str());
}

std::string
Msg::get_domain()
{
  return std::string(Error::throw_errors_with(cpl_msg_get_domain));
}

void
Msg::debug(const std::string& component, const std::string& message)
{
  cpl_msg_debug(component.c_str(), "%s",
                message.c_str());  // Function doesn't throw any errors
}

void
Msg::error(const std::string& component, const std::string& message)
{
  cpl_msg_error(component.c_str(), "%s",
                message.c_str());  // Function doesn't throw any errors
}

void
Msg::info(const std::string& component, const std::string& message)
{
  cpl_msg_info(component.c_str(), "%s",
               message.c_str());  // Function doesn't throw any errors
}

void
Msg::warning(const std::string& component, const std::string& message)
{
  cpl_msg_warning(component.c_str(), "%s",
                  message.c_str());  // Function doesn't throw any errors
}

void
Msg::set_width(int widthSet)
{
  Error::throw_errors_with(cpl_msg_set_width, widthSet);
  current_width = widthSet;
}

int
Msg::get_width()
{
  return current_width;
}

void
Msg::set_indent(int level)
{
  Error::throw_errors_with(cpl_msg_indent, level);
  current_indentation = level;
}

int
Msg::get_indent()
{
  return current_indentation;
}

void
Msg::set_thread_id_switch(bool setting)
{
  if (setting) {
    cpl_msg_set_threadid_on();
  } else {
    cpl_msg_set_threadid_off();
  }
  display_thread_id = setting;
}

bool
Msg::get_thread_id_switch()
{
  return display_thread_id;
}

void
Msg::set_domain_switch(bool setting)
{
  if (setting) {
    cpl_msg_set_domain_on();
  } else {
    cpl_msg_set_domain_off();
  }
  display_domain = setting;
}

bool
Msg::get_domain_switch()
{
  return display_domain;
}

void
Msg::set_time_switch(bool setting)
{
  if (setting) {
    cpl_msg_set_time_on();
  } else {
    cpl_msg_set_time_off();
  }
  display_time = setting;
}

bool
Msg::get_time_switch()
{
  return display_time;
}

void
Msg::set_component_switch(bool setting)
{
  if (setting) {
    cpl_msg_set_component_on();
  } else {
    cpl_msg_set_component_off();
  }
  display_component = setting;
}

bool
Msg::get_component_switch()
{
  return display_component;
}

}  // namespace core
}  // namespace cpl
