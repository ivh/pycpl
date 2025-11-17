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

#include "cplcore/propertylist.hpp"


#include <algorithm>
#include <cstdio>
#include <regex>
#include <sstream>
#include <type_traits>
#include <variant>

#include <cpl_type.h>

#include "cplcore/error.hpp"
#include "cplcore/property.hpp"

namespace cpl
{
namespace core
{

template <class U>  // Alias to avoid repeating cpl::core::Property:: every time
constexpr auto try_upcast = &cpl::core::Property::try_upcast<U>;

std::deque<Property>
copy_props(cpl_propertylist* to_steal)
{
  size list_size =
      Error::throw_errors_with(cpl_propertylist_get_size, to_steal);
  std::deque<Property> retval;

  for (size i = 0; i < list_size; ++i) {
    Property temp_borrowed =
        Error::throw_errors_with(cpl_propertylist_get, to_steal, i);
    // Copy the property already in the list using this constructor
    retval.push_back(Property(temp_borrowed));
    // Since it is borrowed, it must not be destructed:
    Property::unwrap(std::move(temp_borrowed));
  }
  return retval;
}

PropertyList::PropertyList(cpl_propertylist* to_steal) noexcept
    : m_props(copy_props(to_steal))
{
  cpl_propertylist_delete(to_steal);
}

size
PropertyList::get_size() const
{
  return m_props.size();
}

bool
PropertyList::has(std::string_view name) const
{
  return std::any_of(
      m_props.begin(), m_props.end(),
      [name](const Property& prop) { return name == prop.get_name(); });
}

const Property&
PropertyList::get(long position) const
{
  if (position > m_props.size()) {
    std::ostringstream err_msg;
    err_msg << "Index " << position << " does not is too large "
            << "for a PropertyList of size " << m_props.size();
    throw IllegalInputError(PYCPL_ERROR_LOCATION, err_msg.str().c_str());
  } else if (position < 0) {
    throw IllegalInputError(PYCPL_ERROR_LOCATION,
                            "Negative values not allowed for position");
  } else {
    return m_props.at(position);
  }
}

Property&
PropertyList::get(long position)
{
  if (position > m_props.size()) {
    std::ostringstream err_msg;
    err_msg << "Index " << position << " does not is too large "
            << "for a PropertyList of size " << m_props.size();
    throw IllegalInputError(PYCPL_ERROR_LOCATION, err_msg.str().c_str());
  } else if (position < 0) {
    throw IllegalInputError(PYCPL_ERROR_LOCATION,
                            "Negative values not allowed for position");
  } else {
    return m_props.at(position);
  }
}

std::optional<std::reference_wrapper<const Property>>
PropertyList::get(std::string_view name) const
{
  auto property = std::find_if(
      m_props.begin(), m_props.end(),
      [name](const Property& prop) -> bool { return name == prop.get_name(); });
  if (property == m_props.end()) {
    return {};
  } else {
    return {*property};
  }
}

std::optional<std::reference_wrapper<Property>>
PropertyList::get(std::string_view name)
{
  auto property = std::find_if(
      m_props.begin(), m_props.end(),
      [name](const Property& prop) -> bool { return name == prop.get_name(); });
  if (property == m_props.end()) {
    return {};
  } else {
    return {*property};
  }
}

std::optional<std::reference_wrapper<Property>>
PropertyList::get_regexp(const std::string& regexp, bool invert)
{
  try {
    std::regex filter(regexp, std::regex::extended | std::regex::nosubs);
    auto pos = std::find_if(
        m_props.begin(), m_props.end(),
        [&filter, invert](const Property& prop) -> bool {
          return invert == (std::regex_match(prop.get_name(), filter) == false);
        });
    if (pos == m_props.end()) {
      return {};
    }
    return {*pos};
  }
  catch (const std::regex_error& error) {
    throw IllegalInputError(PYCPL_ERROR_LOCATION, error.what());
  }
}

void
PropertyList::append(const PropertyList& other)
{
  std::copy(other.m_props.begin(), other.m_props.end(),
            std::back_inserter(m_props));
}

int
PropertyList::erase(long position)
{
  if (position > m_props.size()) {
    std::ostringstream err_msg;
    err_msg << "Index " << position << " does not is too large "
            << "for a PropertyList of size " << m_props.size();
    throw IllegalInputError(PYCPL_ERROR_LOCATION, err_msg.str().c_str());
  } else if (position < 0) {
    throw IllegalInputError(PYCPL_ERROR_LOCATION,
                            "Negative values not allowed for position");
  } else {
    auto old_end = m_props.end();
    auto new_end = m_props.erase(m_props.begin() + position);
    return old_end - new_end;  // Should be either 1 or 0
  }
}

int
PropertyList::erase(std::string_view name)
{
  auto new_end = std::remove_if(
      m_props.begin(), m_props.end(),
      [name](const Property& prop) { return name == prop.get_name(); });
  int num_erased = m_props.end() - new_end;

  m_props.erase(new_end, m_props.end());

  return num_erased;
}

int
PropertyList::erase_regexp(const std::string& regexp, bool invert)
{
  try {
    std::regex filter(regexp, std::regex::extended | std::regex::nosubs);
    auto pos = std::remove_if(
        m_props.begin(), m_props.end(),
        [&filter, invert](const Property& prop) -> bool {
          return invert == (std::regex_match(prop.get_name(), filter) == false);
        });
    size num_erased = m_props.end() - pos;
    m_props.erase(pos, m_props.end());
    return num_erased;
  }
  catch (const std::regex_error& error) {
    throw IllegalInputError(PYCPL_ERROR_LOCATION, error.what());
  }
}

void
PropertyList::clear()
{
  m_props.clear();
}

void
PropertyList::append(const Property& property)
{
  m_props.push_back(property);
}

void
PropertyList::append(std::string name, Property::value_type value)
{
  // Check variant, taken from the property bindings
  cpl_type inferred = std::visit(
      [](auto&& arg) -> cpl_type {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool>) {
          return CPL_TYPE_BOOL;
        } else if constexpr (std::is_same_v<T, int>) {
          return CPL_TYPE_INT;
        } else if constexpr (std::is_same_v<T, float>) {
          return CPL_TYPE_FLOAT;
        } else if constexpr (std::is_same_v<T, char>) {
          return CPL_TYPE_CHAR;
        } else if constexpr (std::is_same_v<T, std::string>) {
          return CPL_TYPE_STRING;
        } else if constexpr (std::is_same_v<T, double>) {
          return CPL_TYPE_DOUBLE;
        } else if constexpr (std::is_same_v<T, long>) {
          return CPL_TYPE_LONG;
        } else if constexpr (std::is_same_v<T, long long>) {
          return CPL_TYPE_LONG_LONG;
        } else if constexpr (std::is_same_v<T, std::complex<float>>) {
          return CPL_TYPE_FLOAT_COMPLEX;
        } else if constexpr (std::is_same_v<T, std::complex<double>>) {
          return CPL_TYPE_DOUBLE_COMPLEX;
        } else {
          throw cpl::core::IllegalInputError(
              PYCPL_ERROR_LOCATION, "value is of a incompatible property type");
        }
      },
      value);
  m_props.push_back(Property(name, inferred, value));
}

void
PropertyList::prepend(const Property& property)
{
  m_props.push_front(property);
}

void
PropertyList::insert(long position, const Property& property)
{
  if (position > m_props.size()) {
    std::ostringstream err_msg;
    err_msg << "Index " << position << " does not is too large "
            << "for a PropertyList of size " << m_props.size();
    throw IllegalInputError(PYCPL_ERROR_LOCATION, err_msg.str().c_str());
  } else if (position < 0) {
    throw IllegalInputError(PYCPL_ERROR_LOCATION,
                            "Negative values not allowed for position");
  } else {
    m_props.insert(m_props.begin() + position, property);
  }
}

bool
PropertyList::insert(std::string_view here, const Property& property)
{
  auto found_iter = std::find_if(
      m_props.begin(), m_props.end(),
      [here](const Property& prop) -> bool { return prop.get_name() == here; });

  if (found_iter == m_props.end()) {
    return false;
  } else {
    m_props.insert(found_iter, property);
    return true;
  }
}

bool
PropertyList::insert_after(std::string_view after, const Property& property)
{
  auto found_iter = std::find_if(m_props.begin(), m_props.end(),
                                 [after](const Property& prop) -> bool {
                                   return prop.get_name() == after;
                                 });

  if (found_iter == m_props.end()) {
    return false;
  } else {
    m_props.insert(++found_iter, property);
    return true;
  }
}

// /**
//  * @brief Implementation-detial global variable acting as a lambdas'
//  *        stack & return variable, for the compare_func_trampoline
//  *
//  * This is required since Function-pointers can't access per-call
//  * data, unless explicitly allowed by CPL's cpl_propertylist_sort function
//  * (using, e.g. some void* user_data parameter). The workaround is to
//  * use this global as said user_data, and ensure it's not modified
//  * by multiple threads at once. (thread local OR you can protect by a mutex)
//  *
//  * TODO: Ask the CPL people to add user data parameters
//  */
// static thread_local PropertyList::compare_func user_compare;

// int compare_func_trampoline(const cpl_property *first, const cpl_property
// *second) {
//     Property temp_first(const_cast<cpl_property *>(first));
//     Property temp_second(const_cast<cpl_property *>(second));
//     int retval = user_compare(temp_first, temp_second);
//     Property::unwrap(std::move(temp_second));
//     Property::unwrap(std::move(temp_first));
//     return retval;
// }

void
PropertyList::sort(PropertyList::compare_func compare)
{
  std::sort(m_props.begin(), m_props.end(), compare);
}

void
PropertyList::save(const std::filesystem::path& filename, unsigned mode) const
{
  Error::throw_errors_with(cpl_propertylist_save, ptr().get(), filename.c_str(),
                           mode);
}

std::string
PropertyList::dump() const
{
  // Open char pointer as stream
  char* charBuff;
  size_t len;
  FILE* stream = open_memstream(&charBuff, &len);
  Error::throw_errors_with(cpl_propertylist_dump, ptr().get(), stream);

  // Flush to char pointer
  fflush(stream);
  // Cast to std::string
  std::string returnString(charBuff);
  fclose(stream);
  free(charBuff);

  return returnString;
}

std::unique_ptr<struct _cpl_propertylist_, void (*)(cpl_propertylist*)>
PropertyList::ptr() const
{
  cpl_propertylist* ptr = Error::throw_errors_with(cpl_propertylist_new);

  // Append will duplicate the properties
  try {
    for (auto prop : m_props) {
      Error::throw_errors_with(cpl_propertylist_append_property, ptr,
                               prop.ptr());
    }
  }
  catch (...) {
    cpl_propertylist_delete(ptr);
    throw;
  }

  return std::unique_ptr<struct _cpl_propertylist_,
                         void (*)(cpl_propertylist*)>(ptr,
                                                      cpl_propertylist_delete);
}

PropertyList::iterator
PropertyList::begin()
{
  return m_props.begin();
}

PropertyList::const_iterator
PropertyList::begin() const
{
  return m_props.begin();
}

PropertyList::iterator
PropertyList::end()
{
  return m_props.end();
}

PropertyList::const_iterator
PropertyList::end() const
{
  return m_props.end();
}

PropertyList::reverse_iterator
PropertyList::rbegin()
{
  return m_props.rbegin();
}

PropertyList::const_reverse_iterator
PropertyList::rbegin() const
{
  return m_props.rbegin();
}

PropertyList::reverse_iterator
PropertyList::rend()
{
  return m_props.rend();
}

PropertyList::const_reverse_iterator
PropertyList::rend() const
{
  return m_props.rend();
}

PropertyList
load_propertylist(const std::filesystem::path& name, size position)
{
  return Error::throw_errors_with(cpl_propertylist_load, name.c_str(),
                                  position);
}

PropertyList
load_propertylist_regexp(const std::filesystem::path& name, size position,
                         const std::string& regexp, bool invert)
{
  return Error::throw_errors_with(cpl_propertylist_load_regexp, name.c_str(),
                                  position, regexp.c_str(), invert);
}

}  // namespace core
}  // namespace cpl
