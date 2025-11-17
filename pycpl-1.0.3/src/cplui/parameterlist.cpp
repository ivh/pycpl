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


// Container class ParameterList which is similar in functionality to
// cpl_parameterlist, but instead using vectors to store parameter C++ objects

#include "cplui/parameterlist.hpp"

#include <cstdio>

#include "cplcore/error.hpp"

namespace cpl
{
namespace ui
{
ParameterList::ParameterList(cpl_parameterlist* p_list)
{
  // For each parameter currently in the cpl parameterlist, generate object of
  // the appropriate type
  /* Loop through all the parameters in the list */

  for (cpl_parameter* p = cpl_parameterlist_get_first(p_list); p != NULL;
       p = cpl_parameterlist_get_next(p_list)) {
    // Set value of p to default here while we don't have access to enums
    // Need to do this as cpl_parameter_new_x(...) does not set to defaults
    // automatically. Incorporate this switch statement into ParameterValue when
    // enums are made
    /* Determine the class of the parameter */
    cpl_parameter_class pclass = cpl_parameter_get_class(p);
    // Add container of appropriate class to vector
    switch (pclass) {
      case CPL_PARAMETER_CLASS_VALUE:
        append(std::make_shared<ParameterValue>(p));
        break;
      case CPL_PARAMETER_CLASS_RANGE:
        append(std::make_shared<ParameterRange>(p));
        break;
      case CPL_PARAMETER_CLASS_ENUM: append(std::make_shared<ParameterEnum>(p));
      default: break;
    }
  }
}

bool
ParameterList::operator==(ParameterList& other)
{
  if (size() != other.size()) {
    return false;
  }

  // Parameter-wise comparison
  for (int i = 0; i < size(); ++i) {
    if (get_at(i).get() != other.get_at(i).get()) {
      return false;
    }
  }

  return true;
}

bool
ParameterList::operator!=(ParameterList& other)
{
  return !operator==(other);
}

void
ParameterList::append(std::shared_ptr<Parameter> parameter)
{
  m_parameters.push_back(parameter);
}

int
ParameterList::size()
{
  return m_parameters.size();
}

std::shared_ptr<Parameter>
ParameterList::operator[](size_type index)
{
  return m_parameters[index];
}

std::shared_ptr<Parameter>
ParameterList::get_first()
{
  return m_parameters.front();
}

std::shared_ptr<Parameter>
ParameterList::get_last()
{
  return m_parameters.back();
}

std::shared_ptr<Parameter>
ParameterList::get_at(size_type index)
{
  return m_parameters.at(index);
}

std::string
ParameterList::dump() const
{
  char* charBuff;
  size_t len;
  // Open char pointer as stream
  FILE* stream = open_memstream(&charBuff, &len);
  // release is needed here instead of get (which creates a segfault)
  cpl_parameterlist* plist = (this->ptr()).release();
  if (plist != nullptr) {
    cpl::core::Error::throw_errors_with(cpl_parameterlist_dump, plist, stream);

    // Flush to char pointer
    fflush(stream);
    // Cast to std::string
    std::string returnString(charBuff);
    fclose(stream);
    free(charBuff);
    return returnString;
  } else {
    return std::string("Error accessing internal cpl_parameterlist*");
  }
}

std::unique_ptr<struct _cpl_parameterlist_, void (*)(cpl_parameterlist*)>
ParameterList::ptr() const
{
  cpl_parameterlist* ptr =
      cpl::core::Error::throw_errors_with(cpl_parameterlist_new);
  // May have to make a duplicate of the vector m_parameters here and iterate
  // over that instead?
  // Append will duplicate the properties
  try {
    for (auto par : m_parameters) {
      cpl::core::Error::throw_errors_with(
          cpl_parameterlist_append, ptr,
          cpl_parameter_duplicate(
              (par->ptr())));  // TODO: Figure out how to do it safely like
                               // PropertyList::ptr()
                               // );
    }
  }
  catch (...) {
    cpl_parameterlist_delete(ptr);
    throw;
  }

  return std::unique_ptr<struct _cpl_parameterlist_,
                         void (*)(cpl_parameterlist*)>(
      ptr, cpl_parameterlist_delete);
}

}  // namespace ui
}  // namespace cpl
