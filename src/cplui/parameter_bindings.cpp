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

#include "cplui/parameter_bindings.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <pybind11/stl.h>

#include "cplui/parameter.hpp"
#include "cplui/parameterlist.hpp"
#include "dump_handler.hpp"
#include "path_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive

namespace py = pybind11;

template <typename parameter_type>

/**
 * @brief Pybind-compatible wrapper around set_value
 *        allowing lossy Float->Int convesions (with a warning)
 * @param self The cpl::ui::Parameter instance on which the value property was
 * modified
 * @param new_value The new value of said parameter, from Python.
 *
 * A Parameter's set_value function requires that the given value_type
 * matches the type of the parameter. The only exception is putting an
 * integer into a float-type parameter. However, to be more Pythonic,
 * we would like to allow Python users to use whatever number they want
 * on an integer-type parameter, too.
 *
 * This function, therefore, wraps set_value to lossily cast float to
 * int if nessecary, then perform the set_value.
 *
 * A Python UserWarning is emitted if said lossy conversion occurs
 *
 * This function is intended to be a substitute for
 * '&cpl::ui::Parameter::set_value'
 */
void
parameter_set_value_allowing_lossy(parameter_type& self, py::object new_value)
{
  py::object warnings = py::module::import("warnings");
  py::object builtins = py::module::import("builtins");
  if (
      // If this is a lossy cast:
      py::isinstance(new_value, builtins.attr("float")) &&
      self.get_dataType() == CPL_TYPE_INT) {
    warnings.attr("warn")(
        "An Integer type CPL Parameter received a floating-point value. Lossy "
        "conversion will occur",
        builtins.attr("RuntimeWarning"));
    self.set_value(static_cast<int>(new_value.cast<double>()));
  } else {
    self.set_value(new_value.cast<cpl::ui::value_type>());
  }
  // Set the presence flag to indicate that the value has at some point been
  // set.
  self.set_presence(true);
}

// See frame_bindings.hpp for doc comment
void
bind_parameters(py::module& m)
{
  py::class_<cpl::ui::Parameter, std::shared_ptr<cpl::ui::Parameter>> parameter(
      m, "Parameter");

  parameter.doc() = R"pydoc(
        Parameters provide a standard way to pass, for instance, command line information to
        different components of an application.

        The fundamental parts of a parameter are its name, a context to which it belongs (a
        specific component of an application for instance), its current value and a default
        value.

        The implementation supports three classes of parameters:

          - A plain value (cpl.ui.ParameterValue)
          - A range of values (cpl.ui.ParameterRange)
          - An enumeration value (cpl.ui.ParameterEnum)

        cpl.ui.Parameter is the base class for the three parameter classes.

        When a parameter is created it is created for a particular value type. The type of
        a parameter's current and default value may be:

          - cpl.core.Type.BOOL
          - cpl.core.Type.INT
          - cpl.core.Type.DOUBLE
          - cpl.core.Type.STRING

        These types are inferred upon Parameter creation.

        (NOTE: as of writing the validation of parameter values on assignment is not yet
        implemented in CPL. PyCPL does not intend to layer this feature over CPL and thus will
        not include validation until CPL itself does.)

    )pydoc";

  py::enum_<cpl_parameter_mode>(
      parameter,
      "ParameterMode")  // Unsure if we need all aliases? Giraffe only sets CLI
      .value("CLI", CPL_PARAMETER_MODE_CLI)
      .value("ENV", CPL_PARAMETER_MODE_ENV)
      .value("CFG", CPL_PARAMETER_MODE_CFG);

  parameter
      .def_property_readonly("name", &cpl::ui::Parameter::get_name,
                             "The read-only unique name of the parameter")
      .def_property_readonly("context", &cpl::ui::Parameter::get_context,
                             "The context in which the parameter belongs to")
      .def_property_readonly("description",
                             &cpl::ui::Parameter::get_description,
                             "comment or description describing the parameter")
      .def_property_readonly("data_type", &cpl::ui::Parameter::get_dataType,
                             "CPL data type of the parameter")
      .def_property_readonly(
          "help", &cpl::ui::Parameter::get_help,
          "description on how the parameter is used and its effects")
      .def_property("tag", &cpl::ui::Parameter::get_tag,
                    &cpl::ui::Parameter::set_tag, "user definable tag");
  // Bind get_class, under typeClass as class is a keyword

  py::class_<cpl::ui::ParameterValue, cpl::ui::Parameter,
             std::shared_ptr<cpl::ui::ParameterValue>>(m, "ParameterValue",
                                                       R"pydoc(
        Plain parameter value. Stores a single value with no boundaries. CPL data type is inferred on default value given.

        Inherits all properties in cpl.ui.Parameter

        Parameters
        ----------
        name : str
          The unique name of the parameter
        description :str
          comment or description describing the parameter
        context : str
          The context in which the parameter belongs to
        default : bool, int, float or str
          The default and initialised value of the parameter
             )pydoc")
      /*
      NOTE: The bool overload must come before the int overload.
      For a similar reason to ParameterValue's value_type definition (see
      parameter.hpp) If bool were not to come before int, then pybind uses the
      int constructor when python bools are received.
      */
      .def(py::init<std::string, std::string, std::string, bool>(),
           py::arg("name"), py::arg("description"), py::arg("context"),
           py::arg("default"))
      .def(py::init<std::string, std::string, std::string, int>(),
           py::arg("name"), py::arg("description"), py::arg("context"),
           py::arg("default"))
      .def(py::init<std::string, std::string, std::string, double>(),
           py::arg("name"), py::arg("description"), py::arg("context"),
           py::arg("default"))
      .def(py::init<std::string, std::string, std::string, std::string>(),
           py::arg("name"), py::arg("description"), py::arg("context"),
           py::arg("default"))
      .def_property_readonly("name", &cpl::ui::ParameterValue::get_name)
      .def_property_readonly("context", &cpl::ui::ParameterValue::get_context)
      .def_property_readonly("description",
                             &cpl::ui::ParameterValue::get_description)
      .def_property_readonly("data_type",
                             &cpl::ui::ParameterValue::get_dataType,
                             "CPL data type of the parameter")
      .def_property(
          "value", &cpl::ui::ParameterValue::get_value,
          &parameter_set_value_allowing_lossy<cpl::ui::ParameterValue>,
          "current value of the parameter")
      .def_property(
          "presence", &cpl::ui::ParameterValue::get_presence,
          &cpl::ui::ParameterValue::set_presence,
          "flag to indicate if the parameter has been changed from its default")
      .def_property(
          "cli_alias",
          [](cpl::ui::ParameterValue& a) -> std::string {
            return a.get_alias(CPL_PARAMETER_MODE_CLI);
          },
          [](cpl::ui::ParameterValue& a, std::string& alias) -> void {
            a.set_alias(CPL_PARAMETER_MODE_CLI, alias);
          },
          "named used to identify the parameter being set as a the command "
          "line parameter")
      .def_property(
          "env_alias",
          [](cpl::ui::ParameterValue& a) -> std::string {
            return a.get_alias(CPL_PARAMETER_MODE_ENV);
          },
          [](cpl::ui::ParameterValue& a, std::string& alias) -> void {
            a.set_alias(CPL_PARAMETER_MODE_ENV, alias);
          },
          "named used to identify the parameter being set as an environment "
          "variable")
      .def_property(
          "cfg_alias",
          [](cpl::ui::ParameterValue& a) -> std::string {
            return a.get_alias(CPL_PARAMETER_MODE_CFG);
          },
          [](cpl::ui::ParameterValue& a, std::string& alias) -> void {
            a.set_alias(CPL_PARAMETER_MODE_CFG, alias);
          },
          "named used to identify the parameter being set in a .cfg file")
      .def_property_readonly("default", &cpl::ui::ParameterValue::get_default,
                             "default value of the parameter")
      .def("__repr__",
           [](cpl::ui::ParameterValue& a) -> py::str {
             // There is no simple, general method to convert arbitrary data
             // types to a string in C++17, however this is straightforward to
             // do using Python's string formatting functionality. To take
             // advantage of this flexibility we implement __repr__() and
             // __str__() methods by creating a Python string object and use its
             // format() method to insert string representations of the required
             // attributes.

             // Create a Python string object
             py::str representation =
                 "<cpl.ui.ParameterValue: name={!r}, value={!r}>";
             // Use Python string .format() method to insert name & value.
             return representation.format(a.get_name(), a.get_value());
           })
      .def("__str__", &cpl::ui::ParameterValue::dump)
      .def(
          "dump",
          [](cpl::ui::ParameterValue& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(), self.dump(),
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("show") = true,
          R"pydoc(
        Dump a parameter contents to a file, stdout or a string.

        Each element is preceded by its index number (starting with 1!) and
        written on a single line.

        Comment lines start with the hash character.

        Parameters
        ----------
        filename : str, optional
            File to dump parameter contents to
        mode : str, optional
            Mode to open the file with. Defaults to "w" (write, overwriting the contents of the file if it already exists),
            but can also be set to "a" (append, creating the file if it does not already exist or appending to the end of
            it if it does).
        show : bool, optional
            Send parameter contents to stdout. Defaults to True.

        Returns
        -------
        str 
            Multiline string containing the dump of the parameter contents.
        )pydoc")
      .def("__eq__",
           [](cpl::ui::ParameterValue& self, py::object eq_arg) -> bool {
             // If eq_arg were to be a cpl::ui::Parameter (avoiding complication
             // here), then running Parameter == NotAParameter would raise a
             // type error in Python, So instead, it must be cast manually,
             // here, to catch said type error.
             try {
               cpl::ui::ParameterValue* casted =
                   eq_arg.cast<cpl::ui::ParameterValue*>();
               return self == *casted;
             }
             catch (const py::cast_error& wrong_type) {
               return false;  // Type mismatch should return False in python
             }
           });

  py::class_<cpl::ui::ParameterRange, cpl::ui::ParameterValue,
             std::shared_ptr<cpl::ui::ParameterRange>>(m, "ParameterRange",
                                                       R"pydoc(
        Range parameter. On construction expects the default value, followed by the minimum value and the maximum value.
        CPL data type is inferred on default value given.

        Inherits all properties in cpl.ui.ParameterValue and cpl.ui.Parameter

        Parameters
        ----------
        name : str
          The unique name of the parameter
        description :str
          comment or description describing the parameter
        context : str
          The context in which the parameter belongs to
        default : int or float
          The default and initialised value of the parameter
        min : int or float
          Minimum value of the parameter. Must be of the same data type as default.
        max : int or float
          Maximum value of the parameter. Must be of the same data type as default.
             )pydoc")
      .def(py::init<std::string, std::string, std::string, int, int, int>(),
           py::arg("name"), py::arg("description"), py::arg("context"),
           py::arg("default"), py::arg("min"), py::arg("max"))
      .def(py::init<std::string, std::string, std::string, double, double,
                    double>(),
           py::arg("name"), py::arg("description"), py::arg("context"),
           py::arg("default"), py::arg("min"), py::arg("max"))
      .def_property(
          "value", &cpl::ui::ParameterRange::get_value,
          &parameter_set_value_allowing_lossy<cpl::ui::ParameterValue>,
          "Current value of the parameter")
      .def_property_readonly("default", &cpl::ui::ParameterRange::get_default,
                             "Default value of the parameter")
      .def_property_readonly("max", &cpl::ui::ParameterRange::get_max,
                             "Maximum value of the parameter")
      .def_property_readonly("min", &cpl::ui::ParameterRange::get_min,
                             "Minimum value of the parameter")
      .def("__repr__",
           [](cpl::ui::ParameterRange& a) -> py::str {
             // See Parameter.__repr__() above for an explanation of the use of
             // Python strings
             py::str representation =
                 "<cpl.ui.ParameterRange: name={!r}, value={!r}, min={!r}, "
                 "max={!r}>";
             // Use Python string .format() method to insert name, value, min
             // and max.
             return representation.format(a.get_name(), a.get_value(),
                                          a.get_min(), a.get_max());
           })
      .def("__str__", &cpl::ui::ParameterRange::dump)
      .def(
          "dump",
          [](cpl::ui::ParameterRange& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(), self.dump(),
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("show") = true,
          R"pydoc(
        Dump a parameter contents to a file, stdout or a string.

        Each element is preceded by its index number (starting with 1!) and
        written on a single line.

        Comment lines start with the hash character.

        Parameters
        ----------
        filename : str, optional
            File to dump parameter contents to
        mode : str, optional
            Mode to open the file with. Defaults to "w" (write, overwriting the contents of the file if it already exists),
            but can also be set to "a" (append, creating the file if it does not already exist or appending to the end of
            it if it does).
        show : bool, optional
            Send parameter contents to stdout. Defaults to True.

        Returns
        -------
        str 
            Multiline string containing the dump of the parameter contents.
        )pydoc")
      .def("__eq__",
           [](cpl::ui::ParameterRange& self, py::object eq_arg) -> bool {
             // See Parameter.__eq__() above for an explanation of the below
             // casting
             try {
               cpl::ui::ParameterRange* casted =
                   eq_arg.cast<cpl::ui::ParameterRange*>();
               return self == *casted;
             }
             catch (const py::cast_error& wrong_type) {
               return false;  // Type mismatch should return False in python
             }
           });

  py::class_<cpl::ui::ParameterEnum, cpl::ui::ParameterValue,
             std::shared_ptr<cpl::ui::ParameterEnum>>(m, "ParameterEnum",
                                                      R"pydoc(
        Enumeration parameter. On construction expects the default value, followed by the list of the possible enumeration
        values. Note that the default value must be a member of the list of possible enumeration.
        values.

        CPL data type is inferred on default value given.

        Inherits all properties in cpl.ui.ParameterValue and cpl.ui.Parameter

        Parameters
        ----------
        name : str
          The unique name of the parameter
        description :str
          comment or description describing the parameter
        context : str
          The context in which the parameter belongs to
        default : int, float or str
          The default and initialised value of the parameter
        alternatives : list of int, float or str
          list of enumeration alternatives, including the default value. Must be of the same type as default.
             )pydoc")
      .def(py::init<std::string, std::string, std::string, int,
                    std::vector<int>&>(),
           py::arg("name"), py::arg("description"), py::arg("context"),
           py::arg("default"), py::arg("alternatives"))
      .def(py::init<std::string, std::string, std::string, double,
                    std::vector<double>&>(),
           py::arg("name"), py::arg("description"), py::arg("context"),
           py::arg("default"), py::arg("alternatives"))
      .def(py::init<std::string, std::string, std::string, std::string,
                    std::vector<std::string>&>(),
           py::arg("name"), py::arg("description"), py::arg("context"),
           py::arg("default"), py::arg("alternatives"))
      .def_property(
          "value", &cpl::ui::ParameterEnum::get_value,
          &parameter_set_value_allowing_lossy<cpl::ui::ParameterValue>)
      .def_property_readonly("default", &cpl::ui::ParameterEnum::get_default)
      .def_property_readonly("alternatives",
                             &cpl::ui::ParameterEnum::get_alternatives,
                             "possible enumeration alternatives value can be")
      .def("__repr__",
           [](cpl::ui::ParameterEnum& a) -> py::str {
             /* See Parameter.__repr__() above for an explanation of the use of
              * Python strings */

             // Create a Python string object
             py::str representation =
                 "<cpl.ui.ParameterEnum: name={!r}, value={!r}, "
                 "alternatives={!r}>";
             // Use Python string .format() method to insert name, value, and
             // list of alternatives
             return representation.format(a.get_name(), a.get_value(),
                                          py::cast(a.get_alternatives()));
           })
      .def("__str__", &cpl::ui::ParameterEnum::dump)
      .def(
          "dump",
          [](cpl::ui::ParameterEnum& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(), self.dump(),
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("show") = true,
          R"pydoc(
        Dump a parameter contents to a file, stdout or a string.

        Each element is preceded by its index number (starting with 1!) and
        written on a single line.

        Comment lines start with the hash character.

        Parameters
        ----------
        filename : str, optional
            File to dump parameter contents to
        mode : str, optional
            Mode to open the file with. Defaults to "w" (write, overwriting the contents of the file if it already exists),
            but can also be set to "a" (append, creating the file if it does not already exist or appending to the end of
            it if it does).
        show : bool, optional
            Send parameter contents to stdout. Defaults to True.

        Returns
        -------
        str 
            Multiline string containing the dump of the parameter contents.
        )pydoc")

      .def("__eq__",
           [](cpl::ui::ParameterEnum& self, py::object eq_arg) -> bool {
             // See Parameter.__eq__() above for an explanation of the below
             // casting
             try {
               cpl::ui::ParameterEnum* casted =
                   eq_arg.cast<cpl::ui::ParameterEnum*>();
               return self == *casted;
             }
             catch (const py::cast_error& wrong_type) {
               return false;  // Type mismatch should return False in python
             }
           });

  py::class_<cpl::ui::ParameterList, std::shared_ptr<cpl::ui::ParameterList>>
      parameterlist(m, "ParameterList");

  parameterlist.doc() = R"pydoc(
        Container type for cpl.ui.Parameter.

        It provides a convenient way to pass a set of parameters to various functions e.g. recipes.

        Parameters are accessed by index or iteration.
    )pydoc";

  parameterlist.def(py::init<>(), "Create an empty ParameterList")
      .def(
          py::init([](py::iterable parameters) -> cpl::ui::ParameterList {
            cpl::ui::ParameterList new_list;
            for (py::handle it : parameters) {
              try {
                std::shared_ptr<cpl::ui::Parameter> toInsert =
                    py::cast<std::shared_ptr<cpl::ui::Parameter>>(it);
                new_list.append(toInsert);
              }
              catch (const py::cast_error& wrong_type) {
                throw py::type_error(
                    std::string("expected iterable of cpl.ui.Parameter, not ") +
                    it.get_type().attr("__name__").cast<std::string>());
              }
            }
            return new_list;
          }),
          py::arg("params"),
          R"pydoc(
        Generate a ParameterList object with an iterable of the cpl.ui.Parameter objects

        Parameters
        ----------
        params : iterable
          iterable container with the parameters to store in the ParameterList
          )pydoc")
      .def("append", &cpl::ui::ParameterList::append, py::arg("param"),
           R"pydoc(
            Append a parameter to the end of a ParameterList.

            Parameters
            ----------
            param : cpl.ui.Parameter
              parameter to insert
            )pydoc")
      .def("__repr__",
           [](cpl::ui::ParameterList& a) -> py::str {
             // See Parameter.__repr__() above for an explanation of the use of
             // Python strings
             py::str representation = "<cpl.ui.ParameterList, {} Parameters>";
             // User Python string .format() method to insert size
             return representation.format(a.size());
           })
      .def("__len__", &cpl::ui::ParameterList::size)
      .def("__str__", &cpl::ui::ParameterList::dump)
      .def(
          "__getitem__",  // Int indexing
          [](cpl::ui::ParameterList& a,
             signed int index) -> std::shared_ptr<cpl::ui::Parameter> {
            if (index < 0) {
              return a.get_at(index + a.size());
            } else {
              return a.get_at(index);
            }
          },
          py::arg("index"), "Retrieve a parameter by index")
      .def(
          "__getitem__",  // String indexing
          [](cpl::ui::ParameterList& a,
             std::string index) -> std::shared_ptr<cpl::ui::Parameter> {
            for (int i = 0; i < a.size(); i++) {
              if (index.compare(a.get_at(i)->get_name()) == 0)
                return a.get_at(i);
            }
            throw(py::key_error(index));
          },
          py::arg("name"), "Retrieve a parameter by name")
      .def(
          "dump",
          [](cpl::ui::ParameterList& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(), self.dump(),
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("show") = true,
          R"pydoc(
        Dump a parameter list contents to a file, stdout or a string.

        Each element is preceded by its index number (starting with 1!) and
        written on a single line.

        Comment lines start with the hash character.

        Parameters
        ----------
        filename : str, optional
            File to dump parameter list contents to
        mode : str, optional
            Mode to open the file with. Defaults to "w" (write, overwriting the contents of the file if it already exists),
            but can also be set to "a" (append, creating the file if it does not already exist or appending to the end of
            it if it does).
        show : bool, optional
            Send parameter list contents to stdout. Defaults to True.

        Returns
        -------
        str 
            Multiline string containing the dump of the parameter list contents.
        )pydoc")

      .def("__eq__",
           [](cpl::ui::ParameterList& self, py::object eq_arg) -> bool {
             try {
               cpl::ui::ParameterList* casted =
                   eq_arg.cast<cpl::ui::ParameterList*>();
               return self == *casted;
             }
             catch (const py::cast_error& wrong_type) {
               return false;  // Type mismatch should return False in python
             }
           });
}
