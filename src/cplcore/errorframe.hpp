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


// Wrapper class used to manage CPL's internal errors and recipe errors

#ifndef PYCPL_ERRORFRAME_HPP
#define PYCPL_ERRORFRAME_HPP

#include <string>

#include <cpl_error.h>

/**
 * @brief Paramaeters for creating a CPL Exception, adding the context & current
 * location.
 * @see cpl_error_set_message(), cpl_error_set()
 * @example
 *  throw InvalidTypeError(PYCPL_ERROR_LOCATION, "Template instantiation did not
 * match CPL type");
 */
#define PYCPL_ERROR_LOCATION __func__, __FILE__, __LINE__

namespace cpl
{
namespace core
{
/**
 * @brief Superclass to all CPL derived errors
 */
class ErrorFrame
{
 public:
  ErrorFrame(cpl_error_code code, const std::string& function_name,
             const std::string& file_name, unsigned line,
             const std::string& error_message);

  unsigned get_line() const noexcept;
  cpl_error_code get_code() const noexcept;
  std::string get_function_name() const noexcept;
  std::string get_file_name() const noexcept;
  std::string get_error_message() const noexcept;

  const char* what() const noexcept;

  // 31/01/2022: Made the copy constructor noexcept(false) due to std::string
  // having a noexcept(false) copy constructor, making the default constructor
  // implicitly false This caused compilation errors on gcc9 (but weirdly not
  // gcc10) No apparent importance to have this noexcept(true) at the moment so
  // to ensure gcc9 compatibility noexcept specifier has been removed so that
  // the copy constructor will just use the exception specifier defaulted by the
  // compiler.
  ErrorFrame(const ErrorFrame& other) = default;

  bool operator==(const ErrorFrame& other) const noexcept;
  bool operator!=(const ErrorFrame& other) const noexcept;

  ErrorFrame(ErrorFrame&& other) noexcept = default;
  ErrorFrame& operator=(const ErrorFrame& other) noexcept = default;
  ErrorFrame& operator=(ErrorFrame&& other) noexcept = default;

 private:
  unsigned m_line;
  cpl_error_code m_code;
  std::string m_function_name;
  std::string m_file_name;
  std::string m_error_message;

  /**
   * A Python-looking error message
   * equivalent for CPL errors: specifically,
   * this returns Line, File, Function name and error code
   * information added to the actual error message.
   *
   * Created in the constructor of this class
   */
  std::string m_full_message;
};

/**
 * @brief Executes the given macro for every CPL Error code known to pycpl
 * except CPL_ERROR_NONE CALLBACK_MACRO takes the argumets cpl_error_code,
 * exception (TSuperExc), and the name bound to the template instantiation
 * Example:
 * #define MY_CALLBACK(CODE, SUPER_EXC, CPP_NAME) assert
 * cpl::core::CPP_NAME(...).get_code() == CODE
 * PYCPL_EXCEPTION_ENUMERATOR(MY_CALLBACK)
 *
 * CODE is of the form CPL_ERROR_X where X is a known CPL error code name
 * SUPER_EXC is usually std::runtime_error or std::logic_error, but may be other
 * exceptions CPP_NAME is the unqualified name, e.g. DataNotFound or FileIO
 */
#define PYCPL_EXCEPTION_ENUMERATOR(CALLBACK_MACRO)                             \
  /**< This is used some places in the codebase (e.g. cpl_propertylist_insert) \
   */                                                                          \
  CALLBACK_MACRO(CPL_ERROR_UNSPECIFIED, std::runtime_error, UnspecifiedError,  \
                 Unspecified error)                                            \
  /**< The actual CPL error has been lost. Do not use to create a CPL error */ \
  CALLBACK_MACRO(CPL_ERROR_HISTORY_LOST, std::runtime_error, ErrorLostError,   \
                 Actual CPL error has been lost.)                              \
  /**< Could not duplicate output stream */                                    \
  CALLBACK_MACRO(CPL_ERROR_DUPLICATING_STREAM, std::runtime_error,             \
                 DuplicatingStreamError, Could not duplicate output stream.)   \
  /**< Could not associate a stream with a file descriptor */                  \
  CALLBACK_MACRO(CPL_ERROR_ASSIGNING_STREAM, std::runtime_error,               \
                 AssigningStreamError,                                         \
                 Could not associate a stream with a file descriptor.)         \
  /**< Permission denied */                                                    \
  CALLBACK_MACRO(CPL_ERROR_FILE_IO, std::runtime_error, FileIOError,           \
                 Access to file IO denied.)                                    \
  /**< Input file had not the expected format */                               \
  CALLBACK_MACRO(CPL_ERROR_BAD_FILE_FORMAT, std::runtime_error,                \
                 BadFileFormatError, Input file had not the expected format.)  \
  /**< Attempted to open a file twice */                                       \
  CALLBACK_MACRO(CPL_ERROR_FILE_ALREADY_OPEN, std::runtime_error,              \
                 FileAlreadyOpenError, Attempted to open a file twice.)        \
  /**< Could not create a file */                                              \
  CALLBACK_MACRO(CPL_ERROR_FILE_NOT_CREATED, std::runtime_error,               \
                 FileNotCreatedError, Could not create a file.)                \
  /**< A specified file or directory was not found */                          \
  CALLBACK_MACRO(CPL_ERROR_FILE_NOT_FOUND, std::runtime_error,                 \
                 FileNotFoundError,                                            \
                 A specified file or directory was not found.)                 \
  /**< Data searched within a valid object were not found */                   \
  CALLBACK_MACRO(CPL_ERROR_DATA_NOT_FOUND, std::runtime_error,                 \
                 DataNotFoundError,                                            \
                 Data searched within a valid object were not found.)          \
  /**< Data were accessed beyond boundaries */                                 \
  CALLBACK_MACRO(CPL_ERROR_ACCESS_OUT_OF_RANGE, std::range_error,              \
                 AccessOutOfRangeError, Data were accessed beyond boundaries.) \
  /**< A @c NULL pointer was found where a valid pointer was expected */       \
  CALLBACK_MACRO(                                                              \
      CPL_ERROR_NULL_INPUT, std::invalid_argument, NullInputError,             \
      A NULL pointer was found where a valid pointer was expected              \
          .Shouldnt appear in PyCPL but present in case such an error arises.) \
  /**< Data that had to be processed together did not match */                 \
  CALLBACK_MACRO(CPL_ERROR_INCOMPATIBLE_INPUT, std::invalid_argument,          \
                 IncompatibleInputError,                                       \
                 Data that had to be processed together did not match.)        \
  /**< Illegal values were detected */                                         \
  CALLBACK_MACRO(CPL_ERROR_ILLEGAL_INPUT, std::invalid_argument,               \
                 IllegalInputError, Illegal values were detected.)             \
  /**< A given operation would have generated an illegal object */             \
  CALLBACK_MACRO(CPL_ERROR_ILLEGAL_OUTPUT, std::runtime_error,                 \
                 IllegalOutputError,                                           \
                 A given operation would have generated an illegal object.)    \
  /**< The requested functionality is not supported */                         \
  CALLBACK_MACRO(CPL_ERROR_UNSUPPORTED_MODE, std::runtime_error,               \
                 UnsupportedModeError,                                         \
                 The requested functionality is not supported.)                \
  /**< Could not invert a matrix */                                            \
  CALLBACK_MACRO(CPL_ERROR_SINGULAR_MATRIX, std::runtime_error,                \
                 SingularMatrixError, Could not invert a matrix.)              \
  /**< Attempted to divide a number by zero */                                 \
  CALLBACK_MACRO(CPL_ERROR_DIVISION_BY_ZERO, std::runtime_error,               \
                 DivisionByZeroError, Attempted to divide a number by zero.)   \
  /**< Data were not of the expected type */                                   \
  CALLBACK_MACRO(CPL_ERROR_TYPE_MISMATCH, std::runtime_error,                  \
                 TypeMismatchError, Data were not of the expected type.)       \
  /**< Data type was unsupported or invalid */                                 \
  CALLBACK_MACRO(CPL_ERROR_INVALID_TYPE, std::runtime_error, InvalidTypeError, \
                 Data type was unsupported or invalid.)                        \
  /**< An iterative process did not converge */                                \
  CALLBACK_MACRO(CPL_ERROR_CONTINUE, std::runtime_error, ContinueError,        \
                 An iterative process did not converge.)                       \
  /**< The WCS functionalities are missing */                                  \
  CALLBACK_MACRO(CPL_ERROR_NO_WCS, std::runtime_error, NoWCSError,             \
                 The WCS functionalities are missing.)                         \
  /**< To permit extensibility of error handling.*/                            \
  /*It is a coding error to use this within CPL itself! */                     \
  CALLBACK_MACRO(                                                              \
      CPL_ERROR_EOL, std::runtime_error, EOLError,                             \
      To permit extensibility of error handling.Do not raise this in Python    \
          as it will be a conding error in itself)

}  // namespace core
}  // namespace cpl

#endif  // PYCPL_ERRORFRAME_HPP