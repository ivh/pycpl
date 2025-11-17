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

#include "cplui/parameter.hpp"

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <sstream>

#include "cplcore/error.hpp"

namespace cpl
{
namespace ui
{
MismatchedParameterException::MismatchedParameterException(
    const cpl_type expected_type)
    : std::logic_error("A parameter of type " +
                       std::string(cpl_type_get_name(expected_type)) +
                       " does not match the received type") {};

ParameterValue::ParameterValue()
{
  m_interface =
      NULL;  // Prevents the destructor from deleting a non-allocated reference
             // if the ParameterEnum ends up throwing an exception
}

ParameterValue::~ParameterValue()
{
  cpl_parameter_delete(
      m_interface);  // Use cpl functions to deallocate cpl structs
}

ParameterValue::ParameterValue(std::string name, std::string description,
                               std::string context, int value)
{
  m_interface = cpl_parameter_new_value(
      name.c_str(), CPL_TYPE_INT, description.c_str(), context.c_str(), value);
};

ParameterValue::ParameterValue(std::string name, std::string description,
                               std::string context, double value)
{
  m_interface =
      cpl_parameter_new_value(name.c_str(), CPL_TYPE_DOUBLE,
                              description.c_str(), context.c_str(), value);
};

ParameterValue::ParameterValue(std::string name, std::string description,
                               std::string context, bool value)
{
  m_interface = cpl_parameter_new_value(
      name.c_str(), CPL_TYPE_BOOL, description.c_str(), context.c_str(), value);
};

ParameterValue::ParameterValue(std::string name, std::string description,
                               std::string context, std::string value)
{
  m_interface = cpl_parameter_new_value(name.c_str(), CPL_TYPE_STRING,
                                        description.c_str(), context.c_str(),
                                        value.c_str());
};

ParameterValue::ParameterValue(cpl_parameter* external)
{
  cpl_type ptype = cpl_parameter_get_type(external);
  // Need to set default first: as its not expected by the recipe
  switch (ptype) {
    case CPL_TYPE_BOOL:
      cpl_parameter_set_bool(external,
                             cpl_parameter_get_default_bool(external));
      break;

    case CPL_TYPE_INT:
      cpl_parameter_set_int(external, cpl_parameter_get_default_int(external));
      break;

    case CPL_TYPE_DOUBLE:
      cpl_parameter_set_double(external,
                               cpl_parameter_get_default_double(external));
      break;

    case CPL_TYPE_STRING:
      cpl_parameter_set_string(external,
                               cpl_parameter_get_default_string(external));
      break;
    default:
      throw cpl::core::TypeMismatchError(
          PYCPL_ERROR_LOCATION, std::string("CPL type ") +
                                    cpl_type_get_name(ptype) +
                                    " is not yet supported by the C++ layer");
  }

  m_interface = external;
}  // Parameter already built, just throw it in the container

const cpl_parameter*
ParameterValue::ptr() const
{
  return m_interface;
}

bool
ParameterValue::operator==(ParameterValue& other)
{
  return get_name() == other.get_name() &&
         get_context() == other.get_context() &&
         get_description() == other.get_description() &&
         get_tag() == other.get_tag() &&
         get_dataType() == other.get_dataType() &&
         get_value() == other.get_value() &&
         get_default() == other.get_default();
}

bool
ParameterValue::operator!=(ParameterValue& other)
{
  return !operator==(other);
}

std::string
ParameterValue::dump() const
{
  char* charBuff;
  size_t len;
  // Open char pointer as stream
  FILE* stream = open_memstream(&charBuff, &len);
  cpl::core::Error::throw_errors_with(cpl_parameter_dump, m_interface, stream);

  // Flush to char pointer
  fflush(stream);
  // Cast to std::string
  std::string returnString(charBuff);
  fclose(stream);
  free(charBuff);

  return returnString;
}

std::string
ParameterValue::get_name()
{
  return std::string(cpl_parameter_get_name(m_interface));
}

std::string
ParameterValue::get_context()
{
  return std::string(cpl_parameter_get_context(m_interface));
}

std::string
ParameterValue::get_description()
{
  return std::string(cpl_parameter_get_help(m_interface));
}

std::string
ParameterValue::get_tag()
{
  // Tag may not have been set, in which case this returns NULL
  auto nullable_tag = cpl_parameter_get_tag(m_interface);
  if (nullable_tag == NULL) {
    return {};
  } else {
    return std::string(nullable_tag);
  }
}

std::string
ParameterValue::get_help()
{
  return std::string(cpl_parameter_get_help(m_interface));
}

std::string
ParameterValue::get_alias(cpl_parameter_mode mode)
{
  const char* alias = cpl_parameter_get_alias(m_interface, mode);
  return std::string(alias);
}

void
ParameterValue::set_alias(cpl_parameter_mode mode, std::string& alias)
{
  cpl_parameter_set_alias(m_interface, mode, alias.c_str());
}

void
ParameterValue::set_tag(std::string& tag)
{
  cpl_parameter_set_tag(m_interface, tag.c_str());
}

cpl_type
ParameterValue::get_dataType()
{
  return cpl_parameter_get_type(m_interface);
  // NEED HANDLER FOR INCOMPATIBLE TYPES
}

value_type
ParameterValue::get_value()
{
  cpl_type ptype = cpl_parameter_get_type(m_interface);
  switch (ptype) {
    case CPL_TYPE_INT: return cpl_parameter_get_int(m_interface);
    case CPL_TYPE_DOUBLE: return cpl_parameter_get_double(m_interface);
    case CPL_TYPE_BOOL: return (bool)cpl_parameter_get_bool(m_interface);
    case CPL_TYPE_STRING:
      return std::string(cpl_parameter_get_string(m_interface));
    default:
      throw cpl::core::TypeMismatchError(
          PYCPL_ERROR_LOCATION, std::string("CPL type ") +
                                    cpl_type_get_name(ptype) +
                                    " is not yet supported by the C++ layer");
  }
}

bool
ParameterValue::get_presence()
{
  // using ptr() to get m_interface instead of accessing directly. Seemed to fix
  // the bus error
  return cpl_parameter_get_default_flag(const_cast<cpl_parameter*>(ptr()));
}

void
ParameterValue::set_presence(bool status)
{
  // using ptr() to get m_interface instead of accessing directly. Seemed to fix
  // the bus error
  cpl_parameter_set_default_flag(const_cast<cpl_parameter*>(ptr()), status);
}

value_type
ParameterValue::get_default()
{
  cpl_type ptype = cpl_parameter_get_type(m_interface);
  switch (ptype) {
    case CPL_TYPE_INT: return cpl_parameter_get_default_int(m_interface);
    case CPL_TYPE_DOUBLE: return cpl_parameter_get_default_double(m_interface);
    case CPL_TYPE_BOOL:
      return (bool)cpl_parameter_get_default_bool(m_interface);
    case CPL_TYPE_STRING:
      return std::string(cpl_parameter_get_default_string(m_interface));
    default:
      throw cpl::core::TypeMismatchError(
          PYCPL_ERROR_LOCATION, std::string("CPL type ") +
                                    cpl_type_get_name(ptype) +
                                    " is not yet supported by the C++ layer");
  }
}

void
ParameterValue::set_value(value_type value)
{
  cpl_type type_of_cpl_parameter = cpl_parameter_get_type(m_interface);
  switch (type_of_cpl_parameter) {
    case CPL_TYPE_INT: {
      if (int* ivalue = std::get_if<int>(&value)) {
        cpl_parameter_set_int(m_interface, *ivalue);
      } else {
        // The python bindings will do any double to int conversions
        throw MismatchedParameterException(type_of_cpl_parameter);
      }
      break;
    }
    case CPL_TYPE_DOUBLE: {
      if (double* dvalue = std::get_if<double>(&value)) {
        cpl_parameter_set_double(m_interface, *dvalue);
      } else if (int* ivalue = std::get_if<int>(&value)) {
        cpl_parameter_set_double(m_interface, *ivalue);
      } else {
        throw MismatchedParameterException(type_of_cpl_parameter);
      }
      break;
    }
    case CPL_TYPE_BOOL: {
      if (bool* bvalue = std::get_if<bool>(&value)) {
        cpl_parameter_set_bool(m_interface, *bvalue);
      } else {
        throw MismatchedParameterException(type_of_cpl_parameter);
      }
      break;
    }
    case CPL_TYPE_STRING: {
      if (std::string* svalue = std::get_if<std::string>(&value)) {
        cpl_parameter_set_string(m_interface, svalue->c_str());
      } else {
        throw MismatchedParameterException(type_of_cpl_parameter);
      }
      break;
    }
    default: {
      throw cpl::core::TypeMismatchError(
          PYCPL_ERROR_LOCATION, std::string("CPL type ") +
                                    cpl_type_get_name(type_of_cpl_parameter) +
                                    " is not yet supported by the C++ layer");
    }
  }
  // FIXME: NEED HANDLER FOR INCOMPATIBLE TYPES
}

ParameterRange::ParameterRange(cpl_parameter* external)
    : ParameterValue(external) {
      };  // While this is only going to be used in C++, will we
          // need to add checks for same type (i.e. range)?

ParameterRange::ParameterRange(std::string name, std::string description,
                               std::string context, int value, int min, int max)
{
  m_interface =
      cpl_parameter_new_range(name.c_str(), CPL_TYPE_INT, description.c_str(),
                              context.c_str(), value, min, max);
};

ParameterRange::ParameterRange(std::string name, std::string description,
                               std::string context, double value, double min,
                               double max)
{
  m_interface = cpl_parameter_new_range(name.c_str(), CPL_TYPE_DOUBLE,
                                        description.c_str(), context.c_str(),
                                        value, min, max);
};

std::string
ParameterRange::dump() const
{
  char* charBuff;
  size_t len;
  // Open char pointer as stream
  FILE* stream = open_memstream(&charBuff, &len);
  cpl::core::Error::throw_errors_with(cpl_parameter_dump, m_interface, stream);

  // Flush to char pointer
  fflush(stream);
  // Cast to std::string
  std::string returnString(charBuff);
  fclose(stream);
  free(charBuff);

  return returnString;
}

bool
ParameterRange::operator==(ParameterRange& other)
{
  return get_name() == other.get_name() &&
         get_context() == other.get_context() &&
         get_description() == other.get_description() &&
         get_tag() == other.get_tag() &&
         get_dataType() == other.get_dataType() &&
         get_value() == other.get_value() &&
         get_default() == other.get_default() && get_min() == other.get_min() &&
         get_max() == other.get_max();
}

bool
ParameterRange::operator!=(ParameterRange& other)
{
  return !operator==(other);
}

value_type
ParameterRange::get_default()
{
  cpl_type ptype = cpl_parameter_get_type(m_interface);
  switch (ptype) {
    case CPL_TYPE_INT: return cpl_parameter_get_default_int(m_interface);
    case CPL_TYPE_DOUBLE: return cpl_parameter_get_default_double(m_interface);
    default:
      throw cpl::core::TypeMismatchError(
          PYCPL_ERROR_LOCATION, std::string("CPL type ") +
                                    cpl_type_get_name(ptype) +
                                    " is not yet supported by the C++ layer");
  }
}

void
ParameterRange::set_value(value_type value)
{
  // Need to decide on how to handle when value<min or value>max
  cpl_type type_of_cpl_parameter = cpl_parameter_get_type(m_interface);
  switch (type_of_cpl_parameter) {
    case CPL_TYPE_INT: {
      if (int* ivalue = std::get_if<int>(&value)) {
        cpl_parameter_set_int(m_interface, *ivalue);
      } else {
        // The python bindings will do any double to int conversions
        throw MismatchedParameterException(type_of_cpl_parameter);
      }
      break;
    }
    case CPL_TYPE_DOUBLE: {
      if (double* dvalue = std::get_if<double>(&value)) {
        cpl_parameter_set_double(m_interface, *dvalue);
      } else if (int* ivalue = std::get_if<int>(&value)) {
        cpl_parameter_set_double(m_interface, *ivalue);
      } else {
        throw MismatchedParameterException(type_of_cpl_parameter);
      }
      break;
    }
    default: {
      throw cpl::core::TypeMismatchError(
          PYCPL_ERROR_LOCATION, std::string("CPL type ") +
                                    cpl_type_get_name(type_of_cpl_parameter) +
                                    " is not yet supported by the C++ layer");
    }
  }
}

value_type
ParameterRange::get_value()
{
  cpl_type ptype = cpl_parameter_get_type(m_interface);
  switch (ptype) {
    case CPL_TYPE_INT: return cpl_parameter_get_int(m_interface);
    case CPL_TYPE_DOUBLE: return cpl_parameter_get_double(m_interface);
    default:
      throw cpl::core::TypeMismatchError(
          PYCPL_ERROR_LOCATION, std::string("CPL type ") +
                                    cpl_type_get_name(ptype) +
                                    " is not yet supported by the C++ layer");
  }
}

value_type
ParameterRange::get_min()
{
  cpl_type ptype = cpl_parameter_get_type(m_interface);
  switch (ptype) {
    case CPL_TYPE_INT: return cpl_parameter_get_range_min_int(m_interface);
    case CPL_TYPE_DOUBLE:
      return cpl_parameter_get_range_min_double(m_interface);
    default:
      throw cpl::core::TypeMismatchError(
          PYCPL_ERROR_LOCATION, std::string("CPL type ") +
                                    cpl_type_get_name(ptype) +
                                    " is not yet supported by the C++ layer");
  }
}

value_type
ParameterRange::get_max()
{
  cpl_type ptype = cpl_parameter_get_type(m_interface);
  switch (ptype) {
    case CPL_TYPE_INT: return cpl_parameter_get_range_max_int(m_interface);
    case CPL_TYPE_DOUBLE:
      return cpl_parameter_get_range_max_double(m_interface);
    default:
      throw cpl::core::TypeMismatchError(
          PYCPL_ERROR_LOCATION, std::string("CPL type ") +
                                    cpl_type_get_name(ptype) +
                                    " is not yet supported by the C++ layer");
  }
}

ParameterEnum::ParameterEnum(cpl_parameter* external)
    : ParameterValue(external) {};  // Parameter already built, just throw it in
                                    // the container

ParameterEnum::ParameterEnum(std::string name, std::string description,
                             std::string context, int def_value,
                             std::vector<int>& alternatives)
{
  int default_idx;
  auto it = std::find(alternatives.begin(), alternatives.end(), def_value);

  if (it != alternatives.end()) {
    default_idx = it - alternatives.begin();
  } else {
    std::ostringstream alternatives_string;
    std::copy(alternatives.begin(), alternatives.end() - 1,
              std::ostream_iterator<int>(alternatives_string, ", "));
    alternatives_string << alternatives.back();
    std::ostringstream err_msg;
    err_msg << "Default value " << def_value
            << " not found in given alternatives [" << alternatives_string.str()
            << "]";
    throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                       err_msg.str().c_str());
  }
  m_interface = cpl_parameter_new_enum_from_array(
      name.c_str(), CPL_TYPE_INT, description.c_str(), context.c_str(),
      default_idx, alternatives.size(), alternatives.data());
};

ParameterEnum::ParameterEnum(std::string name, std::string description,
                             std::string context, double def_value,
                             std::vector<double>& alternatives)
{
  int default_idx;
  auto it = std::find(alternatives.begin(), alternatives.end(), def_value);
  if (it != alternatives.end()) {
    default_idx = it - alternatives.begin();
  } else {
    std::ostringstream alternatives_string;
    std::copy(alternatives.begin(), alternatives.end() - 1,
              std::ostream_iterator<double>(alternatives_string, ", "));
    alternatives_string << alternatives.back();
    std::ostringstream err_msg;
    err_msg << "Default value " << def_value
            << " not found in given alternatives [" << alternatives_string.str()
            << "]";
    throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                       err_msg.str().c_str());
  }
  m_interface = cpl_parameter_new_enum_from_array(
      name.c_str(), CPL_TYPE_DOUBLE, description.c_str(), context.c_str(),
      default_idx, alternatives.size(), alternatives.data());
};

ParameterEnum::ParameterEnum(std::string name, std::string description,
                             std::string context, std::string def_value,
                             std::vector<std::string>& alternatives)
{
  int default_idx = -1;

  std::vector<const char*> cstrings;  // Convert to char **

  for (int i = 0; i < alternatives.size();
       i++) {  // Copy the list over, while also finding the default value
               // position
    if (def_value == alternatives[i]) {
      default_idx = i;
    }
    cstrings.push_back(alternatives[i].c_str());
  }
  if (default_idx == -1) {  // Hasn't been found
    std::ostringstream alternatives_string;
    alternatives_string << "'";
    std::copy(alternatives.begin(), alternatives.end() - 1,
              std::ostream_iterator<std::string>(alternatives_string, "', "));
    alternatives_string << alternatives.back() << "'";
    std::ostringstream err_msg;
    err_msg << "Default value '" << def_value
            << "' not found in given alternatives ["
            << alternatives_string.str() << "]";
    throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                       err_msg.str().c_str());
  }

  m_interface = cpl_parameter_new_enum_from_array(
      name.c_str(), CPL_TYPE_STRING, description.c_str(), context.c_str(),
      default_idx, cstrings.size(), cstrings.data());
};

bool
ParameterEnum::operator==(ParameterEnum& other)
{
  return get_name() == other.get_name() &&
         get_context() == other.get_context() &&
         get_description() == other.get_description() &&
         get_tag() == other.get_tag() &&
         get_dataType() == other.get_dataType() &&
         get_value() == other.get_value() &&
         get_alternatives() == other.get_alternatives();
}

bool
ParameterEnum::operator!=(ParameterEnum& other)
{
  return !operator==(other);
}

value_type
ParameterEnum::get_value()
{
  cpl_type ptype = cpl_parameter_get_type(m_interface);
  switch (ptype) {
    case CPL_TYPE_INT: return cpl_parameter_get_int(m_interface);
    case CPL_TYPE_DOUBLE: return cpl_parameter_get_double(m_interface);
    case CPL_TYPE_STRING:
      return std::string(cpl_parameter_get_string(m_interface));
    default:
      throw cpl::core::TypeMismatchError(
          PYCPL_ERROR_LOCATION, std::string("CPL type ") +
                                    cpl_type_get_name(ptype) +
                                    " is not yet supported by the C++ layer");
  }
}

value_type
ParameterEnum::get_default()
{
  cpl_type ptype = cpl_parameter_get_type(m_interface);
  switch (ptype) {
    case CPL_TYPE_INT: return cpl_parameter_get_default_int(m_interface);
    case CPL_TYPE_DOUBLE: return cpl_parameter_get_default_double(m_interface);
    case CPL_TYPE_STRING:
      return std::string(cpl_parameter_get_default_string(m_interface));
    default:
      throw cpl::core::TypeMismatchError(
          PYCPL_ERROR_LOCATION, std::string("CPL type ") +
                                    cpl_type_get_name(ptype) +
                                    " is not yet supported by the C++ layer");
  }
};

void
ParameterEnum::set_value(value_type value)
{
  // Note: no validation with enum at the moment: would be best to leave with
  // cpl development first
  cpl_type type_of_cpl_parameter = cpl_parameter_get_type(m_interface);
  switch (type_of_cpl_parameter) {
    case CPL_TYPE_INT: {
      if (int* ivalue = std::get_if<int>(&value)) {
        cpl_parameter_set_int(m_interface, *ivalue);
      } else {
        // The python bindings will do any double to int conversions
        throw MismatchedParameterException(type_of_cpl_parameter);
      }
      break;
    }
    case CPL_TYPE_DOUBLE: {
      if (double* dvalue = std::get_if<double>(&value)) {
        cpl_parameter_set_double(m_interface, *dvalue);
      } else if (int* ivalue = std::get_if<int>(&value)) {
        cpl_parameter_set_double(m_interface, *ivalue);
      } else {
        throw MismatchedParameterException(type_of_cpl_parameter);
      }
      break;
    }
    case CPL_TYPE_STRING: {
      if (std::string* svalue = std::get_if<std::string>(&value)) {
        cpl_parameter_set_string(m_interface, svalue->c_str());
      } else {
        throw MismatchedParameterException(type_of_cpl_parameter);
      }
      break;
    }
    default: {
      throw cpl::core::TypeMismatchError(
          PYCPL_ERROR_LOCATION, std::string("CPL type ") +
                                    cpl_type_get_name(type_of_cpl_parameter) +
                                    " is not yet supported by the C++ layer");
    }
  }
  // FIXME: NEED HANDLER FOR INCOMPATIBLE TYPES
}

std::string
ParameterEnum::dump() const
{
  char* charBuff;
  size_t len;
  // Open char pointer as stream
  FILE* stream = open_memstream(&charBuff, &len);
  cpl::core::Error::throw_errors_with(cpl_parameter_dump, m_interface, stream);

  // Flush to char pointer
  fflush(stream);
  // Cast to std::string
  std::string returnString(charBuff);
  fclose(stream);
  free(charBuff);

  return returnString;
}

std::vector<value_type>
ParameterEnum::get_alternatives()
{
  int count = cpl_parameter_get_enum_size(m_interface);
  std::vector<value_type> retVal = std::vector<value_type>();
  switch (cpl_parameter_get_type(m_interface)) {
    case CPL_TYPE_INT:
      for (int i = 0; i < count; i++) {
        retVal.push_back(cpl_parameter_get_enum_int(m_interface, i));
      }
      break;
    case CPL_TYPE_DOUBLE:
      for (int i = 0; i < count; i++) {
        retVal.push_back(cpl_parameter_get_enum_double(m_interface, i));
      }
      break;
    case CPL_TYPE_STRING:
      for (int i = 0; i < count; i++) {
        retVal.push_back(
            std::string(cpl_parameter_get_enum_string(m_interface, i)));
      }
      break;
    default: break;
  }
  return retVal;
};

}  // namespace ui
}  // namespace cpl
