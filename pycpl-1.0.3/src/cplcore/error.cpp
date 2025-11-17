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

#include "cplcore/error.hpp"

#include <assert.h>
#include <algorithm>
#include <sstream>
#include <string>

namespace cpl
{
namespace core
{
/*
    The following 2 declarations/definitoins are
    for the internal use of ctor_errors
*/

/**
 * @brief Implementation-detail global variable acting as a lambdas'
 *        stack & return variable, for the dumper_function
 *
 * This is required since Function-pointers can't access per-call
 * data, unless explicitly allowed by CPL's cpl_errorstate_dump function
 * (using, e.g. some void* user_data parameter). The workaround is to
 * use this global as said user_data, and ensure it's not modified
 * by multiple threads at once. (thread local OR you can protect by a mutex)
 */
static thread_local std::vector<ErrorFrame> dump_accumulation;

/**
 * @brief Implementation-detail function called by cpl_errorstate_dump to
 *        generate cpl::core::Errors from the input error (using
 * cpl_error_getters).
 */
static void
dumper_function(unsigned /* current */, unsigned /* first */,
                unsigned /* last */)
{
  // In this part of the code, CPL errorstates are in read-only mode,
  // and the cpl_error_get_X function return for the error at 'current'

  dump_accumulation.emplace_back(ErrorFrame(
      cpl_error_get_code(), cpl_error_get_function(), cpl_error_get_file(),
      cpl_error_get_line(), cpl_error_get_message()));
};

Error*
Error::make_error(cpl_error_code code, const std::string& function_name,
                  const std::string& file_name, unsigned line,
                  const std::string& error_message)
{
  switch (code) {
#define PYCPL_MAKE_ERROR_CALLBACK(CODE, SUPER_EXC, CPPNAME, ERROR_DESCRIPTION) \
  case CODE: return new CPPNAME(function_name, file_name, line, error_message);

    PYCPL_EXCEPTION_ENUMERATOR(PYCPL_MAKE_ERROR_CALLBACK)

    default:
      std::ostringstream ss;
      ss << "There is no corresponding C++ Exception for the  CPL error code: "
         << static_cast<int>(code);
      throw std::runtime_error(ss.str());
  }
}

Error*
Error::make_trace(std::vector<ErrorFrame>&& chronological_errors)
{
  cpl_error_code code =
      chronological_errors[chronological_errors.size() - 1].get_code();
  switch (code) {
#define PYCPL_MAKE_TRACE_CALLBACK(CODE, SUPER_EXC, CPPNAME, ERROR_DESCRIPTION) \
  case CODE: return new CPPNAME(std::move(chronological_errors));

    PYCPL_EXCEPTION_ENUMERATOR(PYCPL_MAKE_TRACE_CALLBACK)

    default:
      std::ostringstream ss;
      ss << "There is no corresponding C++ Exception for the  CPL error code: "
         << static_cast<int>(code);
      throw std::runtime_error(ss.str());
  }
}

Error*
Error::make_copy(const Error& other)
{
  switch (other.get_code()) {
#define PYCPL_MAKE_COPY_CALLBACK(CODE, SUPER_EXC, CPPNAME, ERROR_DESCRIPTION) \
  case CODE: return new CPPNAME(reinterpret_cast<const CPPNAME&>(other));

    PYCPL_EXCEPTION_ENUMERATOR(PYCPL_MAKE_COPY_CALLBACK)

    default:
      std::ostringstream ss;
      ss << "There is no corresponding C++ Exception for the  CPL error code: "
         << static_cast<int>(other.get_code());
      throw std::runtime_error(ss.str());
  }
}

void
Error::throw_errors_after(cpl_errorstate previous_error)
{
  if (!cpl_errorstate_is_equal(previous_error)) {
    throw Error::make_trace(Error::pop_errors_after(previous_error));
  }
}

ErrorFrame
Error::last() const
{
  return m_errors.at(m_errors.size() - 1);
}

const std::vector<ErrorFrame>&
Error::trace() const
{
  return m_errors;
}

const char*
Error::what() const noexcept
{
  return m_full_message.c_str();
}

bool
Error::operator==(Error& /* other */) const noexcept
{
  return m_errors.end() == std::adjacent_find(m_errors.begin(), m_errors.end(),
                                              std::not_equal_to<>());
}

bool
Error::operator!=(Error& other) const noexcept
{
  return !operator==(other);
}

Error::Error(std::vector<ErrorFrame>&& chronological_errors)
    : m_errors(chronological_errors)
{
  assert(m_errors.size() > 0);

  std::ostringstream error_message;
  // Like python
  error_message << "CPL error stack trace (most recent error last):\n";
  for (auto individual_err : m_errors) {
    error_message << individual_err.what() << "\n\n";
  }
  m_full_message = error_message.str();
};

std::vector<ErrorFrame>
Error::pop_errors_after(cpl_errorstate previous_error)
{
  assert(!cpl_errorstate_is_equal(previous_error));

  // Dump the error(s) in chronological order:
  // Call my function for every error
  cpl_errorstate_dump(previous_error, CPL_FALSE, &dumper_function);

  // Mark all errors as recovered from to CPL
  cpl_errorstate_set(previous_error);

  std::vector new_errors = dump_accumulation;
  dump_accumulation.clear();

  assert(new_errors.size() > 0);
  return new_errors;
}

#define PYCPL_EXCEPTION_EXPLICIT_INSTANTIATION_DEFINITION_CALLBACK( \
    CODE, SUPER_EXC, CPPNAME, ERROR_DESCRIPTION)                    \
  template class ErrorInstance<CODE, SUPER_EXC>;


PYCPL_EXCEPTION_ENUMERATOR(
    PYCPL_EXCEPTION_EXPLICIT_INSTANTIATION_DEFINITION_CALLBACK)

}  // namespace core
}  // namespace cpl