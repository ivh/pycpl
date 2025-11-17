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

#ifndef PATH_CONVERSION_HPP
#define PATH_CONVERSION_HPP

// Keep pybind11.h the first include, see pybind11 documentation for details:
// https://pybind11.readthedocs.io/en/stable/basics.html#header-and-namespace-conventions
#include <pybind11/pybind11.h>

#include <filesystem>

namespace py = pybind11;

namespace pybind11
{
namespace detail
{

template <>
struct type_caster<std::filesystem::path>
{
  /**
   * This macro establishes the name 'std::filesystem::path' in
   * function signatures and declares a local variable
   * 'value' of type std::filesystem::path
   */
  PYBIND11_TYPE_CASTER(std::filesystem::path, _("str"));

  /**
   * Conversion part 1 (Python->C++): convert a PyObject into a
   * std::filesystem::path instance or return false upon failure. The second
   * argument indicates whether implicit conversions should be applied.
   */
  bool load(handle src, bool /* conversion */)
  {
    // Extract PyObject from handle
    // Borrowed being true means the refcount is still OK after this scope ends
    py::object source = reinterpret_borrow<py::object>(src);

    // Python None objects
    if (!source) {
      return false;
    }

    // PathLike objects have an __fspath__() function
    // which returns either an str or a bytes.
    py::object builtins = py::module::import("builtins");
    py::object pathlib_path = py::module::import("pathlib").attr("Path");
    py::object os_module = py::module::import("os");

    if (py::hasattr(os_module, "PathLike") &&
        py::isinstance(source, os_module.attr("PathLike"))) {
      source = source.attr("__fspath__")();

      if (py::isinstance(source, builtins.attr("bytes"))) {
        // PathLike -> bytes -> C++ conversion:
        value = pybytes_to_fspath(source);
      } else if (py::isinstance(source, builtins.attr("str"))) {
        // PathLike -> str -> bytes -> C++ conversion:
        value = pystring_to_fspath(source);
      } else {
        // Pathlike, but it doesn't give an str or bytes?
        // This cast is explicitly allowed, but raises an exception.
        PyErr_SetString(PyExc_TypeError,
                        "__fspath__ did not return str or bytes");
        throw py::error_already_set();
      }
    } else if (py::isinstance(source, pathlib_path)) {
      // Path -> bytes -> C++ conversion:
      value = pybytes_to_fspath(
          py::module::import("builtins").attr("bytes")(source));
    } else if (py::isinstance(source, builtins.attr("str"))) {
      // str -> bytes -> C++ conversion:
      value = pystring_to_fspath(source);
    } else if (py::isinstance(source, builtins.attr("bytes"))) {
      // bytes -> C++ conversion:
      value = pybytes_to_fspath(source);
    } else {
      // Bad value: Not an str or PathLike
      return false;
    }
    return true;
  }

  /**
   * Conversion part 2 (C++ -> Python): convert an std::filesystem::path
   * instance into a Python object. The second and third arguments are used to
   * indicate the return value policy and parent object (for
   * ``return_value_policy::reference_internal``) and are generally
   * ignored by implicit casters.
   */
  static handle cast(std::filesystem::path src,
                     return_value_policy /* policy */, handle /* parent */)
  {
    // I found this handy function in cpython's posixmodule.c under listdir
    // It is the way to allow the python user to have invalid-utf8 paths,
    // by encoding them using surrogate-escaped' UTF-8 (on POSIX) or another
    // platform-dependent encoding.
    return PyUnicode_DecodeFSDefault(src.c_str());
  }

  /**
   * Converts the given python Bytes object into a std::filesystem::path.
   * The given object can't contain null bytes. Otherwise, no encoding is
   * expected other than that used by the underlying OS/filesystem.
   *
   * E.g. on linux, any string of bytes is allowed.
   *
   * This function may raise exceptions from internal python function calls.
   */
  std::filesystem::path pybytes_to_fspath(py::object source)
  {
    // std::filesystem::path doesn't support embedded 0s in any
    // of its constructors, but PyBytes_AsStringAndSize(..., Null) will
    // 'throw an exception' when such a thing occurs, picked up by
    // PyErr_Occurred, so we don't have to worry about such things in this code.
    //
    // PyBytes_AsStringAndSize returns a pointer to the internal python
    // byte array. This is just borrowed! and is copied immediately
    // (In this case to by using filesystem::path)
    //
    // PyBytes_AsStringAndSize returns 0 on success.
    // If an error occured,
    // This cast is explicitly allowed, but raises an exception.

    char* copy_buffer;
    if (!PyBytes_AsStringAndSize(source.ptr(), &copy_buffer, NULL)) {
      assert(copy_buffer != NULL);
      return std::filesystem::path(copy_buffer);  // Copies the internal buffer
    } else {
      assert(PyErr_Occurred());
      // PyBytes_AsStringAndSize sets the CPython exception if one occurred,
      // But this is C++/pybind so we convert to C++ exception:
      throw error_already_set();
    }
  }

  /**
   * This would be just a std::filesystem::path(source.cast<std::string>())
   * Except that said conversion doesn't follow Pythons os.fsdecode/os.fsencode,
   * Ending up in  that if you give our CPL std::filesystem::path functions a
   * path that isn't valid Unicode, pybind11 raises a RuntimeError
   *
   * This function may raise exceptions from internal python function calls.
   */
  std::filesystem::path pystring_to_fspath(py::object source)
  {
    // Convert to pythons' bytes object using
    // os.fsencode, then use the above code for pybytes_to_fspath
    py::object fsencode = py::module::import("os").attr("fsencode");
    return pybytes_to_fspath(fsencode(source));
  }
};

}  // namespace detail
}  // namespace pybind11

#endif  // PATH_CONVERSION_HPP