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


// Aims to wrap cpl_parameter structs as a C++ class
// This wrapper will contain a pointer to the underlying CPL struct, using the
// appropriate CPL method for applying/retrieving information. The method called
// will depend on the variant in the arg, as well as the datatype of the
// underlying cpl_parameter. Currently no constructor for enum parameters,
// however this would be soon implemented with the next release of cpl

#ifndef PYCPL_PARAMETER_HPP
#define PYCPL_PARAMETER_HPP

#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <cpl_parameter.h>
#include <cpl_type.h>

namespace cpl
{
namespace ui
{
class MismatchedParameterException : public std::logic_error
{
 public:
  MismatchedParameterException(const cpl_type expected_type);
};

/*
 * Whilst not stored at any point in the parameter class,
 * This value type interfaces between external code (e.g. python) and the
 * cpl_parameter* interface.
 *
 * Due to a bug with pybind11, the 'bool' type must come before 'int'.
 * Without this, pybind11 gives function arguments (e.g. set_value(value))
 * integers when the python code gave booleans.
 */
typedef std::variant<double, bool, std::string, int>
    value_type;  // Add types to this variant when necessary

class Parameter
{
 public:
  virtual ~Parameter() = default;
  virtual std::string get_name() = 0;
  virtual std::string get_context() = 0;
  virtual std::string get_description() = 0;
  virtual std::string get_tag() = 0;
  virtual std::string get_alias(cpl_parameter_mode mode) = 0;
  virtual std::string get_help() = 0;
  virtual std::string dump() const = 0;
  virtual void set_tag(std::string& tag) = 0;
  virtual cpl_type get_dataType() = 0;
  virtual void set_value(value_type value) = 0;
  virtual value_type get_value() = 0;
  virtual void set_alias(cpl_parameter_mode mode, std::string& alias) = 0;
  virtual const cpl_parameter* ptr() const = 0;
};

class ParameterValue : public virtual Parameter
{
 public:
  ParameterValue();
  ParameterValue(cpl_parameter* external);  // Allows instantiation of
                                            // parameters created by recipes
  ParameterValue(std::string name, std::string description, std::string context,
                 int value);
  ParameterValue(std::string name, std::string description, std::string context,
                 double value);
  ParameterValue(std::string name, std::string description, std::string context,
                 bool value);
  ParameterValue(std::string name, std::string description, std::string context,
                 std::string value);

  ~ParameterValue() override;

  bool operator==(ParameterValue& other);
  bool operator!=(ParameterValue& other);

  std::string dump() const override;
  std::string get_name() override;
  std::string get_context() override;
  std::string get_description() override;
  std::string get_tag() override;
  std::string get_alias(cpl_parameter_mode mode) override;
  std::string get_help() override;
  void set_tag(std::string& tag) override;
  void set_alias(cpl_parameter_mode mode, std::string& alias) override;

  cpl_type get_dataType() override;  // To convert to a proper python enum

  virtual value_type get_value() override;
  virtual value_type get_default();

  /**
   * @brief
   *   Get the presence status flag of the given parameter.
   * @return
   *   The function returns the current setting of the parameters
   *   presence flag. If the parameter is present the function returns true and
   *   false otherwise.
   *
   * The function indicates whether the given parameter @em self was seen
   * by an application while processing the input from the command line, the
   * environment and/or a configuration file. If the parameter was seen the
   * application may set this status flag and query it later using this
   * function.
   *
   */
  bool get_presence();

  /**
   * @brief
   *   Change the presence status flag of the given parameter.
   *
   * @param self    A parameter.
   * @param status  The presence status value to assign.
   *
   * The function sets the presence status flag of the given parameter
   * @em self to the value @em status. true means that
   * the parameter is present. If the presence status should be changed to
   * `not present' the argument @em status must be false.
   *
   */
  void set_presence(bool status);

  virtual void set_value(value_type value) override;
  const cpl_parameter* ptr() const override;

 protected:
  cpl_parameter* m_interface;
};

class ParameterRange
    : public virtual ParameterValue  // cpl range is simply just values with
                                     // restricted value ranges and types
{
 public:
  ParameterRange(cpl_parameter* external);
  ParameterRange(std::string name, std::string description, std::string context,
                 int value, int min, int max);
  ParameterRange(std::string name, std::string description, std::string context,
                 double value, double min, double max);

  ~ParameterRange() = default;  // Call destructor of base class and members

  bool operator==(ParameterRange& other);
  bool operator!=(ParameterRange& other);

  // Overrides necessary as range has a more limited number of compatible data
  // types
  std::string dump() const override;
  value_type get_value() override;
  value_type get_default() override;
  void set_value(value_type value)
      override;  // Will need override to deal with min and max
  // Current bug: value always returns 0
  value_type get_min();
  value_type get_max();
};

// Enum currently only available for parameters that require it. It is not
// supposed to be constructable from Python until the new enum interface is
// released

class ParameterEnum : public ParameterValue
{
 public:
  ParameterEnum(cpl_parameter* external);
  ParameterEnum(std::string name, std::string description, std::string context,
                int def_value, std::vector<int>& alternatives);
  ParameterEnum(std::string name, std::string description, std::string context,
                double def_value, std::vector<double>& alternatives);
  ParameterEnum(std::string name, std::string description, std::string context,
                std::string def_value, std::vector<std::string>& alternatives);

  ~ParameterEnum() = default;
  std::string dump() const override;
  bool operator==(ParameterEnum& other);
  bool operator!=(ParameterEnum& other);
  value_type get_value() override;
  value_type get_default() override;
  void set_value(value_type value)
      override;  // Will need override to deal with min and max

  std::vector<value_type> get_alternatives();
};

}  // namespace ui
}  // namespace cpl

#endif  // PYCPL_PARAMETER_HPP