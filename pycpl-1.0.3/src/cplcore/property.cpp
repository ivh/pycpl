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

#include "cplcore/property.hpp"

#include <string>

#include <cpl_type.h>

#include "cplcore/error.hpp"

namespace cpl
{
namespace core
{

Property::Property(cpl_property* to_steal) noexcept : m_interface(to_steal) {}

Property::Property(Property&& other) noexcept : m_interface(other.m_interface)
{
  other.m_interface = nullptr;
}

Property::Property(const std::string& name, cpl_type type)
    : m_interface(
          Error::throw_errors_with(cpl_property_new, name.c_str(), type))
{
  // All known supported types allow property creation to occur as usual,
  // otherwise throw the error. Other constructors do not need this as
  // set_typed_value has this switch statement built in
  switch (type) {
    case CPL_TYPE_STRING:
    case CPL_TYPE_CHAR:
    case CPL_TYPE_BOOL:
    case CPL_TYPE_INT:
    case CPL_TYPE_LONG:
    case CPL_TYPE_LONG_LONG:
    case CPL_TYPE_FLOAT:
    case CPL_TYPE_DOUBLE:
    case CPL_TYPE_FLOAT_COMPLEX:
    case CPL_TYPE_DOUBLE_COMPLEX: break;
    default:
      // Throw invalid type error
      throw InvalidTypeError(
          PYCPL_ERROR_LOCATION,
          "constructor was given an unsupported property type");
  }
}

Property::Property(const std::string& name, cpl_type type,
                   Property::value_type initial_value)
    : m_interface(
          Error::throw_errors_with(cpl_property_new, name.c_str(), type))
{
  set_typed_value(initial_value);
}

Property::Property(const std::string& name, cpl_type type,
                   Property::value_type initial_value,
                   const std::string& comment)
    : m_interface(
          Error::throw_errors_with(cpl_property_new, name.c_str(), type))
{
  set_typed_value(initial_value);
  set_comment(comment);
}

Property::Property(const Property& other)
    : m_interface(
          Error::throw_errors_with(cpl_property_duplicate, other.m_interface))
{
}

Property::~Property()
{
  Error::throw_errors_with(cpl_property_delete, m_interface);
}

size
Property::get_size() const
{
  return Error::throw_errors_with(cpl_property_get_size, m_interface);
}

void
Property::set_name(const std::string& name)
{
  Error::throw_errors_with(cpl_property_set_name, m_interface, name.c_str());
}

void
Property::set_comment(const std::string& comment)
{
  Error::throw_errors_with(cpl_property_set_comment, m_interface,
                           comment.c_str());
}

std::string
Property::dump() const
{
// FIXME: Currently not implemented as cpl_property_dump() is not a public API
#if 0
  // Open char pointer as stream
  char* charBuff;
  size_t len;
  FILE* stream = open_memstream(&charBuff, &len);
  
  Error::throw_errors_with(cpl_property_dump, m_interface, stream);

  // Flush to char pointer
  fflush(stream);
  // Cast to std::string
  std::string returnString(charBuff);
  fclose(stream);
  free(charBuff);

  return returnString;
#endif
  return std::string("Not implemented");
}

void
Property::set_type(cpl_type type)
{
  if (type == get_type()) {
    return;
  }

  const char* name_save =
      Error::throw_errors_with(cpl_property_get_name, m_interface);
  const char* comment_save =
      Error::throw_errors_with(cpl_property_get_comment, m_interface);

  m_interface = Error::throw_errors_with(cpl_property_new, name_save, type);

  if (comment_save != nullptr) {
    Error::throw_errors_with(cpl_property_set_comment, m_interface,
                             comment_save);
  }
}

void
Property::set_value(const Property::value_type& any_value)
{
  // First, attempt to set the value without changing type
  // This will use try_upcast (in set_typed_value) to attempt to use the value
  // It (currently) doesn't do narrowing conversions if they lose information
  // at runtime. In either case, if the cast fails, the type is converted
  // into one that will succeed.
  try {
    set_typed_value(any_value);
  }
  catch (const InvalidTypeError& /* unused */) {
    // The type returned by value_to_cpl_type is guaranteed to be able
    // to hold any_value.
    set_type(value_to_cpl_type(any_value));
    set_typed_value(any_value);
  }
}

void
Property::set_typed_value(const Property::value_type& any_value)
{
  switch (get_type()) {
    case CPL_TYPE_STRING:
      Error::throw_errors_with(cpl_property_set_string, m_interface,
                               try_upcast<std::string>(any_value).c_str());
      break;
    case CPL_TYPE_CHAR:
      Error::throw_errors_with(cpl_property_set_char, m_interface,
                               try_upcast<char>(any_value));
      break;
    case CPL_TYPE_BOOL:
      Error::throw_errors_with(cpl_property_set_bool, m_interface,
                               try_upcast<bool>(any_value));
      break;
    case CPL_TYPE_INT:
      Error::throw_errors_with(cpl_property_set_int, m_interface,
                               try_upcast<int>(any_value));
      break;
    case CPL_TYPE_LONG:
      Error::throw_errors_with(cpl_property_set_long, m_interface,
                               try_upcast<long>(any_value));
      break;
    case CPL_TYPE_LONG_LONG:
      Error::throw_errors_with(cpl_property_set_long_long, m_interface,
                               try_upcast<long long>(any_value));
      break;
    case CPL_TYPE_FLOAT:
      Error::throw_errors_with(cpl_property_set_float, m_interface,
                               try_upcast<float>(any_value));
      break;
    case CPL_TYPE_DOUBLE:
      Error::throw_errors_with(cpl_property_set_double, m_interface,
                               try_upcast<double>(any_value));
      break;
    case CPL_TYPE_FLOAT_COMPLEX:
      Error::throw_errors_with(
          cpl_property_set_float_complex, m_interface,
          cpl::core::complex_to_c(try_upcast<std::complex<float>>(any_value)));
      break;
    case CPL_TYPE_DOUBLE_COMPLEX:
      Error::throw_errors_with(
          cpl_property_set_double_complex, m_interface,
          cpl::core::complex_to_c(try_upcast<std::complex<double>>(any_value)));
      break;
    default:
      throw InvalidTypeError(
          PYCPL_ERROR_LOCATION,
          "set_value_type was given an unsupproted type to cast to");
  }
}

std::string
Property::get_name() const
{
  return std::string(
      Error::throw_errors_with(cpl_property_get_name, m_interface));
}

std::optional<std::string>
Property::get_comment() const
{
  const char* comment =
      Error::throw_errors_with(cpl_property_get_comment, m_interface);
  if (comment == nullptr) {
    return {};
  } else {
    return std::string(comment);
  }
}

cpl_type
Property::get_type() const
{
  return Error::throw_errors_with(cpl_property_get_type, m_interface);
}

Property::value_type
Property::get_value() const
{
  const char* string_value;
  switch (get_type()) {
    case CPL_TYPE_STRING:
      string_value =
          Error::throw_errors_with(cpl_property_get_string, m_interface);
      if (string_value == nullptr) {
        return std::string("");
      } else {
        return std::string(string_value);
      }
    case CPL_TYPE_CHAR:
      return Error::throw_errors_with(cpl_property_get_char, m_interface);
    case CPL_TYPE_BOOL:
      return (bool)Error::throw_errors_with(cpl_property_get_bool, m_interface);
    case CPL_TYPE_INT:
      return Error::throw_errors_with(cpl_property_get_int, m_interface);
    case CPL_TYPE_LONG:
      return Error::throw_errors_with(cpl_property_get_long, m_interface);
    case CPL_TYPE_LONG_LONG:
      return Error::throw_errors_with(cpl_property_get_long_long, m_interface);
    case CPL_TYPE_FLOAT:
      return Error::throw_errors_with(cpl_property_get_float, m_interface);
    case CPL_TYPE_DOUBLE:
      return Error::throw_errors_with(cpl_property_get_double, m_interface);
    case CPL_TYPE_FLOAT_COMPLEX:
      return cpl::core::complexf_to_cpp(Error::throw_errors_with(
          cpl_property_get_float_complex, m_interface));
    case CPL_TYPE_DOUBLE_COMPLEX:
      return cpl::core::complexd_to_cpp(Error::throw_errors_with(
          cpl_property_get_double_complex, m_interface));
    default:
      throw InvalidTypeError(PYCPL_ERROR_LOCATION,
                             "found a type in the wrapped cpl_property* that "
                             "is not know to Property.");
  }
}

const cpl_property*
Property::ptr() const
{
  return m_interface;
}

cpl_property*
Property::ptr()
{
  return m_interface;
}

cpl_property*
Property::unwrap(Property&& self)
{
  cpl_property* interface = self.m_interface;
  self.m_interface = nullptr;
  return interface;
}

bool
Property::operator==(const Property& other) const
{
  return other.get_name() == get_name() &&
         other.get_comment() == get_comment() &&
         other.get_type() == get_type() && other.get_value() == get_value();
}

bool
Property::operator!=(const Property& other) const
{
  return !operator==(other);
}

Property&
Property::operator=(const Property& other)
{
  Error::throw_errors_with(cpl_property_delete, m_interface);
  m_interface = nullptr;  // Don't leave it invalid if the duplication fails
  m_interface = Error::throw_errors_with(cpl_property_duplicate, other.ptr());
  return *this;
}

Property&
Property::operator=(Property&& other)
{
  Error::throw_errors_with(cpl_property_delete, m_interface);

  m_interface = other.m_interface;
  other.m_interface = nullptr;
  return *this;
}

}  // namespace core
}  // namespace cpl
