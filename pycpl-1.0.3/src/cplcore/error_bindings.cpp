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

#include "cplcore/error_bindings.hpp"

#include <map>
#include <mutex>
#include <sstream>
#include <string>

#include <cpl_error.h>
#include <pyerrors.h>  // PyError_SetObject

#include <pybind11/eval.h>
#include <pybind11/stl.h>  // List conversion for Error constructor

#include "cplcore/error.hpp"
#include "cplcore/errorframe.hpp"

namespace py = pybind11;

static std::map<cpl_error_code, py::object> python_error_classes;
static std::once_flag error_classes_once;

/*
 * @brief Creates, without pybind, python classes that allow for inheriting
 * RuntimeError with added members. This adds cpl.core.Error, and all
 * cpl.core.SomethingError classes.
 */
static void create_python_classes(py::module& m, py::object& errorframe_cls,
                                  py::object& error_cls);

/*
 * @brief (call once) Initializes all python subclasses to cpl.core.Error
 *        E.g. cpl.core.FileIOError, cpl.core.NullInputError... will all be
 * defined Also initializes python_error_classes map (hence this is call_once)
 */
static void initialize_python_error_classes(py::module& m);

static py::object python_class_for_cpl(const cpl::core::ErrorFrame& cpl_exc);

void
bind_errors(py::module& m)
{
  /**
   * Pybind11 doesn't allow for register_exception to take complicated classes
   * (i.e. with class methods).
   *
   * These methods are REQUIRED for CPL errors to be of any use.
   *
   * Hence the following doesn't follow the standard Pybind11 way of registering
   * errors. Also, Pybind11 doesn't allow for inheriting from builtin python
   * classes (such as RuntimeError), so we must work around it somehow.
   *
   * That leaves less elegant solutions. The issue is documented here,
   * https://github.com/pybind/pybind11/issues/1281
   *
   * To inherit from inbuilt classes, a workaround is implemented here:
   * However it does not register (for type conversion) bindings with pybind11
   * https://github.com/pybind/pybind11/issues/1193#issuecomment-429451094
   *
   * Another possible workaround would be to directly call PyErr_NewException,
   * which has an argument for the base class, but again, that does not
   * register for type conversion.
   *
   * I have tried to register type conversions manually, however I could not
   * figure out how to register with the appropriate pointer types
   *
   * I also tried another workaround mentioned: Use a python class that
   * multiple-inherits from our Error as well as python RuntimeError
   * https://github.com/pybind/pybind11/issues/1193#issuecomment-354935001
   * However CPython doesn't like multiple inheritance from a python builtin and
   * a C++ bound class.
   *
   * So we settle on a permutation of the above solution: Instead of inheriting
   * from both Error data class and the RuntimeError, we only inherit from
   * RuntimeError, and have a class member for the ErrorData, defining
   * functions/properties that pass through to said member.
   *
   * Conversion from cpl::core::Error* is done in the exception translator, and
   * in every other C++ function that would otherwise expose a cpl::core::Error
   * (all getters) So that the python user never sees an _Error_Data class
   * unless they specifically look at the _data member
   */

  /* Data-holding base classes */

  py::class_<cpl::core::ErrorFrame> errorframe_cls(m, "ErrorFrame");
  py::class_<cpl::core::Error> error_cls(m, "_Error_Data");

  static_assert(sizeof(int) == sizeof(cpl_error_code));
  errorframe_cls
      .def_property_readonly(
          "code",
          [](const cpl::core::ErrorFrame& self) -> unsigned int {
            return static_cast<unsigned int>(self.get_code());
          })
      .def_property_readonly("line", &cpl::core::ErrorFrame::get_line)
      .def_property_readonly("function",
                             &cpl::core::ErrorFrame::get_function_name)
      .def_property_readonly("file", &cpl::core::ErrorFrame::get_file_name)
      .def_property_readonly("message",
                             &cpl::core::ErrorFrame::get_error_message)
      .def("__eq__", &cpl::core::ErrorFrame::operator==)
      .def("__eq__",
           [](cpl::core::ErrorFrame& /* self */,
              py::object /* other */) -> bool { return false; })
      .def("__str__", &cpl::core::ErrorFrame::what)
      .def("__repr__",
           [](cpl::core::ErrorFrame& self) -> std::string {
             std::ostringstream repr;
             py::object py_repr = py::module::import("builtins").attr("repr");
             repr << "<cpl.core.ErrorFrame function="
                  << py_repr(self.get_function_name())
                  << " code=" << static_cast<int>(self.get_code())
                  << " file=" << py_repr(self.get_file_name())
                  << " line=" << py_repr(self.get_line())
                  << " message=" << py_repr(self.get_error_message()) << ">";
             return repr.str();
           })
      .def(py::pickle(
          [](cpl::core::ErrorFrame& e) -> py::tuple {
            return py::make_tuple(static_cast<int>(e.get_code()),
                                  e.get_function_name(), e.get_file_name(),
                                  e.get_line(), e.get_error_message());
          },
          [](py::tuple t) -> cpl::core::ErrorFrame {
            return cpl::core::ErrorFrame(
                static_cast<cpl_error_code>(t[1].cast<int>()),
                t[0].cast<std::string>(), t[2].cast<std::string>(),
                t[3].cast<unsigned>(), t[4].cast<std::string>());
          }))
      .def("error_class", &python_class_for_cpl);

  error_cls
      .def(py::init([](unsigned code, const std::string& function_name,
                       const std::string& file_name, unsigned line,
                       const std::string& error_message) -> cpl::core::Error* {
        return cpl::core::Error::make_error(static_cast<cpl_error_code>(code),
                                            function_name, file_name, line,
                                            error_message);
      }))
      .def(py::init([](py::iterable trace) -> cpl::core::Error* {
        std::vector<cpl::core::ErrorFrame> chronological_errors;
        auto inserter = std::back_inserter(chronological_errors);
        std::transform(
            trace.begin(), trace.end(), inserter,
            [](pybind11::iterator::reference i) -> cpl::core::ErrorFrame {
              return py::cast<cpl::core::ErrorFrame>(i);
            });
        if (chronological_errors.size() < 1) {
          throw py::value_error("Expected at least 1 iterated element");
        }
        return cpl::core::Error::make_trace(std::move(chronological_errors));
      }))
      .def(py::init([](const cpl::core::Error& other) -> cpl::core::Error* {
        return cpl::core::Error::make_copy(other);
      }))
      .def_property_readonly("trace", &cpl::core::Error::trace)
      .def_property_readonly("last", &cpl::core::Error::last)
      .def("__eq__", &cpl::core::ErrorFrame::operator==)
      .def("__eq__",
           [](cpl::core::Error& /* self */, py::object /* other */) -> bool {
             return false;
           })
      .def("__str__", &cpl::core::Error::what)
      .def("__repr__",
           [](const cpl::core::Error& self) -> std::string {
             return std::string("_Error_Data(") +
                    py::module::import("builtins")
                        .attr("repr")(self.trace())
                        .cast<std::string>() +
                    ")";
           })
      .def(py::pickle(
          [](cpl::core::Error& e) -> std::vector<cpl::core::ErrorFrame> {
            return e.trace();
          },
          [](std::vector<cpl::core::ErrorFrame> t) -> cpl::core::Error* {
            return cpl::core::Error::make_trace(std::vector(t));
          }));

  create_python_classes(m, errorframe_cls, error_cls);


  py::register_exception_translator([](std::exception_ptr p) -> void {
    // Instance of _Error_Data
    py::object error_data_obj;
    // python class: cpl.core.IllegalInputError/FileIOError/...
    py::object specific_error_class;

    try {
      if (p)
        std::rethrow_exception(p);
    }
    catch (const cpl::core::Error* e) {
      specific_error_class = python_class_for_cpl(e->last());

      // This catch case assumes that ownership is transferred to here
      // because Error::make_* functions are what usually throw an Error*,
      // and Error::make_* gives ownership to the caller.

      // (pybind thinks that cpl::core::Error is uncopyable)
      // ownership is transferred into the Python bound _Error_Data
      error_data_obj = py::module::import("cpl.core").attr("_Error_Data")(e);
    }
    catch (const cpl::core::Error& e) {
      specific_error_class = python_class_for_cpl(e.last());

      // This catch case assumes regular ownership/referencing rules
      // with regard to the error: The error is owned by the C++ runtime
      // or whatever threw it. A copy must be made to transfer to python.

      /*
          We can't just do .attr("_Error_Data")(e) here, because it
          throws return_value_policy = copy, but type cpl::core::Error is
         non-copyable!. So instead we make it a copy manually (and since it's an
         rvalue reference, pybind doesn't try to copy it again, as far as I can
         tell)
      */
      error_data_obj = py::module::import("cpl.core")
                           .attr("_Error_Data")(cpl::core::Error::make_copy(e));
    }

    /*
        Since the PYCPL_ERROR_PYCLASS_CALLBACK doesn't define __init__,
        It's superclass, cpl.core.Error's, __init__ is called.
        The cpl.core._Error_Data overload is used, not cpl.core.Error
        or List of Error, as those are for python code
    */
    py::object specific_exception = specific_error_class(error_data_obj);

    // SetObject lets us raise any python object (the object = second argument,
    // class of said object = first argument)
    PyErr_SetObject(specific_error_class.ptr(),
                    specific_exception.release().ptr());
  });
}

py::object
python_class_for_cpl(const cpl::core::ErrorFrame& cpl)
{
  return python_error_classes.find(cpl.get_code())->second;
}

void
create_python_classes(py::module& m, py::object& /* errorframe_cls */,
                      py::object& /* error_cls */)
{
  py::object property = py::module::import("builtins").attr("property");

  // https://github.com/pybind/pybind11/pull/2616/
  // This was solved, but not for pybind 2.5.0, so the workaround is done here,
  // too: Running exec and eval on Python 2 and 3 adds `builtins` module under
  // `__builtins__` key to globals if not yet present.
  // Python 3.8 made PyRun_String behave similarly. Let's also do that for
  // older versions, for consistency.
  py::object global = m.attr("__dict__");
  if (!global.contains("__builtins__"))
    global["__builtins__"] = py::module::import("builtins");

  py::exec(
      R"(
        from collections.abc import Sequence
        from abc import abstractmethod
        from inspect import getframeinfo, stack

        class Error(Exception, Sequence):
            """
            **Abstract** base class of all CPL exceptions,
            Do not instantiate this class, instead use cpl.core.NullInputError, cpl.core.InvalidArgumentError, or any other subclass.
            **However** this class implements has all documentation for those error
            subclasses.

            In order to copy a cpl error, where you do not know the type of the
            error, use the cpl.core.Error.create classmethod, as create can 
            dispatch to the relevant subclass.

            Examples
            --------
            .. code-block:: python
            
              try:
                  # Some PyCPL functions are called here
              except cpl.core.IllegalInputError as e:
                  print(str(e.message))
              except cpl.core.Error as e:
                  print(str(e))
            """

            def __init__(self, *args):
                """ Use Error.create(...) or a known subclass e.g. InvalidTypeError;
                This class not instantiable by itself.

                This method has several overloads:
                    * (function_name: str, file_name: str, line: unsigned, error_message: str)
                      Creates a new error, (Only 1 frame in the trace)
                      
                    * (copy: Error)
                      Copy constructor copies the given error

                      If the given error does not match this Error subclass,
                      an Value error is raised
                    
                    * (trace: List of Error)
                      Given a list of Errors, this creates a stack trace out of those
                      errors (essentially concatenating them) and produces a type the same 
                      as the final error in the list

                      If the last error in the trace does not match this Error subclass,
                      an Value error is raised
                    
                    * (data: _Error_Data,)
                      Used internally to create the Error from C++ cpl::core::Error's

                      If the given error does not match this Error subclass,
                      an Value error is raised
                """

                try:
                    if len(args) == 0:
                        raise TypeError('Expected at least one arg')
                    elif len(args) == 1 and type(args[0]) is str:
                        caller= getframeinfo(stack()[1][0])
                        message=args[0]
                        code = self.code
                        self._data = _Error_Data(code, caller.function, caller.filename, caller.lineno, message)

                    elif len(args) == 4:
                        function_name, file_name, line, error_message = args
                        code = self.code
                        self._data = _Error_Data(code, function_name, file_name, line, error_message)
                    
                    else:
                        if isinstance(args[0], _Error_Data):
                            self._data = args[0]
                        elif isinstance(args[0], Error):
                            self._data = _Error_Data(args[0])
                        else:
                            try:
                                # Create list of cpl.core.ErrorFrame from the Error list
                                self._data = _Error_Data((
                                    frame
                                    for err in args[0]
                                    for frame in err.trace
                                ))
                            except AttributeError:
                                raise TypeError('Expected Iterable of cpl.core.Error')

                except TypeError:
                    # Not an iterable
                    raise TypeError('Expected one of the following overloads: \n' +\
                        '    (iterable of cpl.core.Error)'+\
                        '    cpl.core._Error_Data\n'+\
                        '    cpl.core.Error\n'+\
                        '    int, str, str, unsigned, str\n'+\
                        '  not ' + repr(tuple((arg.__class__.__name__ for arg in args)))
                    )


                if self._data.last.code != self.code:
                    raise ValueError("Expected an error matching " + self.__class__.__name__ +\
                        ", not " + data.last.error_class().__name__)
            
            @classmethod
            def create(cls, *args):
                """Create a subclass of Error, choosing subclass based on input arguments,
                so you don't need to know which subclass of error to create one.
                Instantiating a InvalidTypeError, FileIOError, etc... are preferred
                over using this function, when you know the error you're creating.

                This method has several overloads:
                    * (copy: Error)
                      Copy constructor copies the given error
                    
                    * (trace: List of Error)
                      Given a list of Errors, this creates a stack trace out of those
                      errors (essentially concatenating them) and produces a type the same 
                      as the final error in the list
                    
                    * (code: int, function_name: str, file_name: str, line: unsigned, error_message: str)
                      Creates a new error, (Only 1 frame in the trace) based on CPL error code
                      Cpl error codes are available on subclasses as the 'code' class member
                      e.g. IllegalInputError.code
                    
                    * (data: _Error_Data)
                      Since Error is a wrapper around _Error_Data, this is the main constructor

                The class that is returned is a subclass of cpl.core.Error
                """

                import itertools

                try:
                    if len(args) == 5:
                        code, function_name, file_name, line, error_message = args

                        return  cls.create(_Error_Data(code, function_name, file_name, line, error_message))

                    if isinstance(args[0], _Error_Data):
                        return args[0].last.error_class()(*args)
                    elif isinstance(args[0], Error):
                        return cls.create(args[0]._data)
                    else:
                        # Peek first cpl.core.Error to determine error class
                        iterator = iter(args[0])
                        first_err = next(iterator)
                        # To undo the 'peek', chain the first element and next elems:
                        return first_err._data.last.error_class()(
                            itertools.chain((first_err,), iterator)
                        )
                except TypeError:
                    # Not an iterable
                    raise TypeError('Expected one of the following overloads: \n' +\
                        '    (iterable of cpl.core.Error)'+\
                        '    cpl.core._Error_Data\n'+\
                        '    cpl.core.Error\n'+\
                        '    int, str, str, unsigned, str\n'+\
                        '  not ' + repr(tuple((arg.__class__.__name__ for arg in args)))
                    )

            
            @property
            def file(self):
                """C/C++ File where this error occurred or was re-thrown"""
                return self._data.last.file
            
            @property
            def line(self):
                """Line number (in a C/C++ file) where this error or was re-thrown"""
                return self._data.last.line
            
            @property
            def function(self):
                return self._data.last.function

            @property
            def message(self):
                return self._data.last.message
            
            @property
            def trace(self):
                return self._data.trace
            
            def __len__(self):
                return len(self._data.trace)
            
            def __getitem__(self, index):
                # The iterable of ErrorFrames constructor is used
                return Error.create(_Error_Data((self._data.trace[index],)))
            
            def __eq__(self, other):
                if isinstance(other, Error):
                    return self._data == other._data
                else:
                    return False

            def __str__(self):
                return str(self._data)
            
            def __repr__(self):
                if len(self) == 1:
                    # Single frame error
                    return 'cpl.core.' + self.__class__.__name__ + repr((self.function, self.file, self.line, self.message))
                else:
                    # Multiple frames
                    return 'cpl.core.' + self.__class__.__name__ + '(' + repr(list(self)) + ')'
        )",
      global);

  std::call_once(error_classes_once, initialize_python_error_classes, m);
}

void
initialize_python_error_classes(py::module& m)
{
  const char* pyequiv_exc;

  // initialize_python_error_classes should only be called once
  assert(python_error_classes.size() == 0);

  // https://github.com/pybind/pybind11/pull/2616/
  // This was solved, but not for pybind 2.5.0, so the workaround is done here,
  // too: Running exec and eval on Python 2 and 3 adds `builtins` module under
  // `__builtins__` key to globals if not yet present.
  // Python 3.8 made PyRun_String behave similarly. Let's also do that for
  // older versions, for consistency.
  py::object global = m.attr("__dict__");
  if (!global.contains("__builtins__"))
    global["__builtins__"] = py::module::import("builtins");

#define PYCPL_ERROR_PYCLASS_CALLBACK(CODE, SUPER_EXC, CPPNAME,               \
                                     ERROR_DESCRIPTION)                      \
  /*Determine what python type is an appropriate superclass (Depends on C++  \
   * superclass)*/                                                           \
  if (std::is_same<SUPER_EXC, std::runtime_error>::value) {                  \
    pyequiv_exc = "RuntimeError";                                            \
  } else if (std::is_same<SUPER_EXC, std::invalid_argument>::value) {        \
    pyequiv_exc = "ValueError";                                              \
  } else if (std::is_same<SUPER_EXC, std::range_error>::value) {             \
    pyequiv_exc = "LookupError";                                             \
  } else {                                                                   \
    static_assert(std::is_same<SUPER_EXC, std::runtime_error>::value ||      \
                  std::is_same<SUPER_EXC, std::invalid_argument>::value ||   \
                  std::is_same<SUPER_EXC, std::range_error>::value);         \
  }                                                                          \
  /* As required by the Error class, code() needs to be overridden (Like C++ \
   * Side) */                                                                \
  py::exec(std::string("class " CX_STRINGIFY(CPPNAME) "(Error, ") +          \
               pyequiv_exc + "):\n" + "    \"\"\"" +                         \
               CX_STRINGIFY(ERROR_DESCRIPTION) +                             \
               "\n\n"                                                        \
               "    A CPL Error subclass. This is a CPL Error that is "      \
               "thrown from C/C++\n"                                         \
               "    and has C/C++ stacktrace available, with line numbers, " \
               "file names, function\n"                                      \
               "    names, and CPL Error codes. See cpl.core.Error help "    \
               "documentation for more\n"                                    \
               "    help on members and methods (scroll down to inherited "  \
               "methods)\n"                                                  \
               "    \"\"\"\n"                                                \
               "    code = " +                                               \
               std::to_string(static_cast<int>(CODE)) + "\n",                \
           global);                                                          \
  python_error_classes.insert(                                               \
      std::make_pair(CODE, m.attr(CX_STRINGIFY(CPPNAME))));

  PYCPL_EXCEPTION_ENUMERATOR(PYCPL_ERROR_PYCLASS_CALLBACK)
}