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

// Wrapper class, but not around a CPL class, instead this wraps the
// CPL abstraction of the errorstate.

#ifndef PYCPL_ERROR_HPP
#define PYCPL_ERROR_HPP

#include <stdexcept>
#include <vector>

#include <cpl_error.h>
#include <cpl_errorstate.h>

#include "cplcore/errorframe.hpp"

namespace cpl
{
namespace core
{
class Error : public std::exception
{
 public:
  /**
   * This forces Error to be pure virtual class
   * So that any users are forced to create a subclass.
   *
   * This function MUST be callable from base class constructors
   * (i.e. when the implementor's class isn't instantiated at all)
   * This is the case for ErrorInstance (code being a constexpr)
   *
   * We don't want cpl::core::Errors being thrown because
   * they can't be caught by their appropriate subclasses.
   * For example:
   *
   * @code
   * try {
   *
   *   in some function:
   *   throw cpl::core::Error(CPL_ERROR_FILE_NOT_FOUND);
   *
   * } (catch cpl::core::FileNotFound &err) {
   *   create_file_when_necessary();
   * } (catch cpl::core::Error &other_error) {
   *   unnecessarily_fail();
   * }
   * @endcode
   *
   * The user would expect that the cpl::core::FileNotFound be thrown
   * when a file is not found, but allowing Errors to be instantiated
   * (i.e. allowing them to be not pure virtual)
   * skirts around this, which is not what we want.
   */
  virtual cpl_error_code get_code() const noexcept = 0;

  /**
   * @brief constructs a child class based on the error code and arguments
   *
   * What is returned is a pointer to any of DuplicatingStreamError,
   * FileIOError, etc... Based on what the value of code is (at runtime)
   */
  static Error*
  make_error(cpl_error_code code, const std::string& function_name,
             const std::string& file_name, unsigned line,
             const std::string& error_message);

  static Error* make_trace(std::vector<ErrorFrame>&& chronological_errors);

  /**
   * @brief Copies an existing errors, creating a new error of the same
   * subclass, same line, function name, file name, message.
   *
   * What is returned is a pointer to any of DuplicatingStreamError,
   * FileIOError, etc... Based on what the value of code is (at runtime)
   */
  static Error* make_copy(const Error& to_copy);

  /**
   * @brief Runs equivalent to the given function,
   *        Except that any errors added to the errorstate
   *        during the call are thrown as an Error exception,
   *        after the function has completed.
   *
   *        errorstate is reverted to the state it was at before
   *        this function was called after it is called. (Minus any
   *        missing history errors due to running out of space on the stack)
   *
   * Example usage:
   * @code
   * void cpl_do_something(int arg1, int arg2) {
   *     cpl_error_set_message(cpl_func, CPL_ERROR_FILE_IO, "My file IO error");
   * }
   *
   *     try {
   *         cpl::core::Error::throw_errors_with(cpl_do_something, arg1, arg2);
   *
   *         ... This code is only reachable if cpl_do_something did not raise a
   * cpl error... } catch(cpl::core::Error &e) {
   *         ... This code is only reachable if cpl_do_something did raise a cpl
   * error... std::shared_ptr<cpl::core::Error> err = e.get_last_error();
   *         assert(err.get_message() == "File read/write error: My file IO
   * error"); assert(err.get_code() == CPL_ERROR_FILE_IO);
   *         assert(err.get_function_name() == "cpl_do_something");
   *     }
   * @endcode
   *
   * @note 2 Args... packs are required, solution from
   * https://stackoverflow.com/a/26995098 Because the Function pointer and the
   * arguments themselves don't exactly match they must be cased (using
   * std::forward) and allowed to be different.
   */
  template <class Return, class... Args1, class... Args2>
  static Return throw_errors_with(Return wrapped(Args1... args), Args2... args)
  {
    cpl_errorstate prestate = cpl_errorstate_get();

    auto ret_val = wrapped(std::forward<Args2>(args)...);

    Error::throw_errors_after(prestate);

    return ret_val;
  }

  /**
   * @brief specialisation of throw_errors_with for void-returning functions
   */
  template <class... Args1, class... Args2>
  static void throw_errors_with(void wrapped(Args1... args), Args2... args)
  {
    cpl_errorstate prestate = cpl_errorstate_get();

    wrapped(std::forward<Args2>(args)...);

    Error::throw_errors_after(prestate);
  }

  /**
   * @brief Throws a Error if any new errors have been raised since
   *        previous_error was created (using cpl_errorstate_get())
   *        Use throw_errors_with if you're just doing it for 1 function
   *
   * Example usage:
   * @code
   *     try {
   *         cpl_errorstate prestate = cpl_errorstate_get();
   *
   *         do_something(arg1, arg2);
   *         do_something_else(); //if you're only doing 1 thing, use
   * Error::throw_errors_with
   *
   *         cpl::core::Error::throw_errors_after(prestate);
   *
   *         ... This code is only reachable if do_something did not raise a cpl
   * error... } catch(cpl::core::Error &e) {
   *         ... This code is only reachable if do_something did raise a cpl
   * error...
   *     }
   * @endcode
   */
  static void throw_errors_after(cpl_errorstate previous_error);

  /**
   * @returns The most recent error recorded to this Error
   */
  ErrorFrame last() const;
  /**
   * @returns All errors recorded on this Error, in order from
   *          first to most recent.
   */
  const std::vector<ErrorFrame>& trace() const;

  virtual const char* what() const noexcept override;

  bool operator==(Error& other) const noexcept;
  bool operator!=(Error& other) const noexcept;

 protected:
  /*
  Copy & Move can maybe change the cpl_error_code,
  Which is completley unwatned. But the functionality
  implemented in them is used by ErrorInstance<cpl_error_code>, so they
  are protected here, for use only in ErrorInstance<cpl_error_code>::
  */

  /**
   * @brief Creates a list of errors that occurred, from 0th element
   *        being the first error, to last element having occurred
   *        most recently.MUST HAVE AT LEAST 1 ELEMENT
   * @see cpl::core::Error::pop_errors_after
   */
  Error(std::vector<ErrorFrame>&& chronological_errors);

 private:
  /**
   * @brief cpl_errorstate_equal(previous_error) MUST RETURN FALSE
   *        Constructs an Error chronological_errors argument
   *        using CPL errors raised since previous_error was recorded.
   *
   * To use this with the constructor:
   * @code
   *     cpl_errorstate prestate = cpl_errorstate_get();
   *     ... Do something that might raise a CPL error...
   *     if(!cpl_errorstate_equal(prestate)) {
   *         throw Error(Error::pop_errors_after(prestate));
   *     }
   * @endcode
   *
   * A cpl_errorstate keeps track of how many errors are on the CPL
   * error state. So, when new errors are 'throw' by CPL, the cpl_errorstate
   * increases. This lets us determine how many errors a specific function
   * call has 'throw', by recording the cpl_errorstate before the function
   * is called, then comparing that with the current cpl_errorstate.
   *
   * This also allows access to errors that were thrown in said function only.
   * These are the errors that are used to construct this Error.
   *
   * Note: This is not a constructor overload itself because I was not able
   * to call std::runtime_error(string) constructor, without ending up calling
   * the pop_errors_after twice.
   * @code
   * Error(cpl_errorstate previous_error) :
   *     m_errors(Error::pop_errors_after(previous_error)
   *     std::runtime_error(m_errors),
   *     // m_errors here is initialized after std::runtime_error
   *     std::runtime_error(Error::ctor_full_message()),
   *     // So, neither of the above incantations can access the m_error when
   * they need it.
   *     // So instead, I tried to call using the pop_errors_after:
   *     std::runtime_error(Error::pop_errors_after(previous_error)),
   *     // But since this modifies the current error stack, that's a no-go.
   * @endcode
   */
  static std::vector<ErrorFrame>
  pop_errors_after(cpl_errorstate previous_error);

  std::vector<ErrorFrame> m_errors;
  // The following allow the std::runtime_error to have its what()
  // string set properly at construction time of this Error.
  std::string m_full_message;
};

/**
 * @brief Warning: Only use the explicit instantiations of this class, below.
 *        Specifies a class corresopnding to a CPL error code
 *
 * Each CPL Error code has a corresponding instance of this class template,
 * with an appropriate std:: exception superclass, depending on the type of CPL
 * error.
 *
 * Users should only use the instances of this template using their proper
 * names, e.g. cpl::error::BadFileFormat instead of
 * ErrorInstance<CPL_ERROR_BAD_FILE_FORMAT>.
 *
 * For construction of instances of these classes, Something special must be
 * done: Because we can't construct an ErrorInstance<non-constexpr-arg>, use
 * RTTI/Reflection, keeping in mind that all ErrorInstances are
 * reinterpret-castable between each other of the same TSuperExc.
 */
template <cpl_error_code cplcode, typename TSuperExc>
class ErrorInstance : public Error, public TSuperExc
{
  static std::vector<ErrorFrame>
  errors_with_cause(ErrorFrame&& last, const ErrorInstance& cause)
  {
    std::vector<ErrorFrame> retval(cause.trace());
    retval.emplace_back(std::move(last));
    return retval;
  }

 public:
  /**
   * @brief Used by throw_errors_with, this constructs a whole error with
   *        stack trace.
   *
   * @param chronological_errors All errors in this error instance. The first
   *                             error must have its get_code() equal to the
   *                             cplcode template parameter of this class
   * @throws std::invalid_argument if the above condition is not met
   */
  ErrorInstance(std::vector<ErrorFrame>&& chronological_errors)
      : Error(std::move(chronological_errors)), TSuperExc(Error::what())
  {
    if (cplcode != (chronological_errors.cend() - 1)->get_code()) {
      throw std::invalid_argument(
          "chronological_errors last error code must match constructed "
          "ErrorInstance::code");
    }
  }

  /**
   * @brief Create a new CPL error not based on a pre-existing cpl error being
   * thrown
   * @param function_name Function name string
   * @param file_name File the error is thrown in
   * @param line Line number error is thrown from
   * @param error_message Detailed error message akin to std::exception(message)
   * @note function_name, file_name and line are filled in by
   * PYCPL_ERROR_LOCATION
   * @code
   * throw NullInputError(PYCPL_ERROR_LOCATION, "Input x was null")
   * @endcode
   */
  ErrorInstance(std::string function_name, std::string file_name, unsigned line,
                std::string error_message)
      : Error(std::vector<ErrorFrame>({ErrorFrame(
            cplcode, function_name, file_name, line, error_message)})),
        TSuperExc(Error::what())
  {
  }

  /**
   * @brief Create a new CPL error based on an existing error, but only as a
   *        cause. i.e. Adds line/function/file context and can change error
   * type.
   * @param function_name Function name string
   * @param file_name File the error is thrown in
   * @param line Line number error is thrown from
   * @param error_message Detailed error message akin to std::exception(message)
   * @note function_name, file_name and line are filled in by
   * PYCPL_ERROR_LOCATION
   * @param cause Error that this is based on
   * @code
   * throw NullInputError(PYCPL_ERROR_LOCATION, "Input x was null")
   * @endcode
   */
  ErrorInstance(std::string function_name, std::string file_name, unsigned line,
                std::string error_message, const ErrorInstance& cause)
      : Error(errors_with_cause(
            ErrorFrame(cplcode, function_name, file_name, line, error_message),
            cause)),
        TSuperExc(Error::what())
  {
  }

  ErrorInstance(const ErrorInstance<cplcode, TSuperExc>& other) = default;
  ErrorInstance(ErrorInstance<cplcode, TSuperExc>&& other) noexcept = default;
  ErrorInstance<cplcode, TSuperExc>&
  operator=(const ErrorInstance<cplcode, TSuperExc>& other) = default;
  ErrorInstance<cplcode, TSuperExc>&
  operator=(ErrorInstance<cplcode, TSuperExc>&& other) noexcept = default;

  virtual const char* what() const noexcept override { return Error::what(); };

  virtual cpl_error_code get_code() const noexcept override { return cplcode; }

  static constexpr cpl_error_code code = cplcode;
};

/*
    These classes are explicitly instantiated here, so that they may be used as
   CPPNAME

    HOWEVER, without 'extern' this causes a problem where every file that
   #includes "errorframe.hpp" generates their own implementations of
   ErrorInstance<...>, resulting in multiple .o object files having a
   ErrorInstance<CODE1, SUPER_EXC1>, causing linker errors with the macOS linker
   (But not Linux linker)

    To avoid this, the 'explicit instantiation declaration' is declared here.
   Without a definition, anything #including "error.hpp" will NOT have their own
   ErrorInstances. Instead, it's up to the explicit instantiation definition in
   error.cpp to define the single implementation for ErrorInstance. I.e. the
   only implementation of ErrorInstance<...> is in error.o, and anything
   #including "error.hpp" links to that single implementation
*/

#define PYCPL_EXCEPTION_EXPLICIT_INSTANTIATION_DECLARATION_CALLBACK( \
    CODE, SUPER_EXC, CPPNAME, ERROR_DESCRIPTION)                     \
  extern template class ErrorInstance<CODE, SUPER_EXC>;              \
  using CPPNAME = ErrorInstance<CODE, SUPER_EXC>;


PYCPL_EXCEPTION_ENUMERATOR(
    PYCPL_EXCEPTION_EXPLICIT_INSTANTIATION_DECLARATION_CALLBACK)

}  // namespace core
}  // namespace cpl

#endif  // PYCPL_ERROR_HPP