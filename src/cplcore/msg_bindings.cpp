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

#include "cplcore/msg_bindings.hpp"

#include "cplcore/msg.hpp"
#include "path_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive

void
bind_message(py::module& m)
{
  py::class_<cpl::core::Msg> msg(m, "Msg");

  msg.doc() = R"pydoc(
        This module provides functions to display and log messages. The following operations are supported:
        
        - Enable messages output to terminal or to log file.
        - Optionally adding informative tags to messages.
        - Setting width for message line wrapping.
        - Control the message indentation level.
        - Filtering messages according to their severity level.

        This module is configured via the `set_config` method and controls how messages are output.
        )pydoc";

  py::enum_<cpl_msg_severity>(msg, "SeverityLevel")
      .value("DEBUG", CPL_MSG_DEBUG)
      .value("INFO", CPL_MSG_INFO)
      .value("WARNING", CPL_MSG_WARNING)
      .value("ERROR", CPL_MSG_ERROR)
      .value("OFF", CPL_MSG_OFF);

  // FIXME: Note that start_log expects a std::filesystem::path as argument,
  //        which means that implicit conversion from a python string to a
  //        filesystem path is needed, and which requires path_conversion.h to
  //        be included. This is not easily visible from the following
  //        definition. May be using a lambda with explicit argument list would
  //        make this clearer?
  msg.def_static("start_file", &cpl::core::Msg::start_log, py::arg("verbosity"),
                 py::arg("name") = ".logfile",
                 R"pydoc(
        Begin log to file.

        Typically called at the start of a script.

        If this has already been called previously, the previous file log will stop and restart with the new logger.

        Parameters
        ----------
        Verbosity : cpl.core.Msg.SeverityLevel 
            Verbosity level
        name : str
            Filename to begin logging to

        Raises
        ------
        cpl.core.IllegalInputError
            If name is longer than 72 characters
        )pydoc")
      .def_static(
          "stop_file", &cpl::core::Msg::stop_log,
          "Close the current log file if running. Will not throw an error if "
          "logging is not currently active. This routine may be called in case "
          "the logging should be terminated before the end of a program.")
      .def_static(
          "set_config",
          // Unfortunately we need to capture the python object self and do
          // nothing to it. Python class rules.
          [](py::kwargs kwargs) -> void {
            if (kwargs.contains("level")) {
              cpl::core::Msg::set_level(kwargs["level"].cast<int>());
            }

            if (kwargs.contains("domain")) {
              cpl::core::Msg::set_domain(kwargs["domain"].cast<std::string>());
            }

            if (kwargs.contains("width")) {
              cpl::core::Msg::set_width(kwargs["width"].cast<int>());
            }

            if (kwargs.contains("indent")) {
              cpl::core::Msg::set_indent(kwargs["indent"].cast<int>());
            }

            if (kwargs.contains("show_threadid")) {
              cpl::core::Msg::set_thread_id_switch(
                  kwargs["show_threadid"].cast<bool>());
            }

            if (kwargs.contains("show_domain")) {
              cpl::core::Msg::set_domain_switch(
                  kwargs["show_domain"].cast<bool>());
            }

            if (kwargs.contains("show_time")) {
              cpl::core::Msg::set_time_switch(kwargs["show_time"].cast<bool>());
            }

            if (kwargs.contains("show_component")) {
              cpl::core::Msg::set_component_switch(
                  kwargs["show_component"].cast<bool>());
            }
          },
          R"pydoc(
        Set CPL Messaging configuration via kwargs as seen in the parameters below

        Parameters
        ----------
        level : cpl.core.Msg.SeverityLevel, optional
                Verbosity level, message below said verbosity level are not printed
        domain : str, optional
                The domain name, also known as a task identifier, typically a pipeline recipe name.
        width : int, optional
                The maximum width of the displayed text. 
        indent : int, optional
                The indentation level. Messages are indented by a number of characters equal to the level. 
                Specifying a negative indentation level would set the indentation level to zero. 
        show_threadid : bool, optional
                True to attach the threadid tag with the messages
        show_domain : bool, optional
                True to attach the domain name tag with the messages
        show_time : bool, optional
                True to attach the time tag with the messages
        show_component : bool, optional
                True to attach the component tag with the messages
        )pydoc")
      .def_static(
          "get_config",
          // Unfortunately we need to capture the python object self and
          // do nothing to it. Python class rules.
          []() -> py::dict {
            py::dict config;
            config["log_name"] = cpl::core::Msg::get_log_name();
            config["level"] = cpl::core::Msg::get_level();
            config["domain"] = cpl::core::Msg::get_domain();
            config["width"] = cpl::core::Msg::get_width();
            config["indent"] = cpl::core::Msg::get_indent();
            config["show_threadid"] = cpl::core::Msg::get_thread_id_switch();
            config["show_domain"] = cpl::core::Msg::get_domain_switch();
            config["show_time"] = cpl::core::Msg::get_time_switch();
            config["show_component"] = cpl::core::Msg::get_component_switch();
            return config;
          },
          "Gets current CPL Messaging configuration")
      .def_static("debug", &cpl::core::Msg::debug, py::arg("component"),
                  py::arg("message"),
                  R"pydoc(
        Display a debug message. 

        Parameters
        ----------
        component : str
                Name of the function generating the message. 
        message : str
                Message to output

        Notes
        -----
        The `show_component` option in the config must be set to True for the component to be visible. 
        )pydoc")
      .def_static("error", &cpl::core::Msg::error, py::arg("component"),
                  py::arg("message"),
                  R"pydoc(
        Display an error message. 

        Parameters
        ----------
        component : str
                Name of the function generating the message. 
        message : str
                Message to output

        Notes
        -----
        The `show_component` option in the config must be set to True for the component to be visible.
        )pydoc")
      .def_static("info", &cpl::core::Msg::info, py::arg("component"),
                  py::arg("message"),
                  R"pydoc(
        Display an information message. 

        Parameters
        ----------
        component : str
                Name of the function generating the message. 
        message : str
                Message to output

        Notes
        -----
        The `show_component` option in the config must be set to True for the component to be visible.
        )pydoc")
      .def_static("warning", &cpl::core::Msg::warning, py::arg("component"),
                  py::arg("message"),
                  R"pydoc(
        Display a warning message. 

        Parameters
        ----------
        component : str
                Name of the function generating the message. 
        message : str
                Message to output

        Notes
        -----
        The `show_component` option in the config must be set to True for the component to be visible.
        )pydoc");
}
