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

#include "dump_handler.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

// The function py::set_error is only available in pybind11 2.12.0 or newer
#if (PYBIND11_VERSION_MAJOR >= 2) && (PYBIND11_VERSION_MINOR >= 12)
#define PYCPL_PYBIND11_SET_ERROR py::set_error
#else
#define PYCPL_PYBIND11_SET_ERROR PyErr_SetString
#endif

namespace py = pybind11;

inline static bool
is_exclusive(const std::string& haystack, unsigned char needle,
             const std::string& exclude)
{
  return (haystack.find(needle) != std::string::npos) &&
         (haystack.find_first_of(exclude) == std::string::npos);
}

// Map Python file mode to C++ fopen file access flags. Valid Python mode
// characters are: r, w, a, x, b, t, +
// Except for t and x the mode characters have a corresponding file access flag
// with the same meaning. Python's binary mode 'b' is accepted, but has no
// effect with std::fopen() except on Windows. For std::fopen() 'x' is a
// qualifier which may be appended to 'w' and 'w+' modes.
inline static std::string
make_access_flags(std::string fmode)
{
  // If present, remove 't' and 'b' flags.
  fmode.erase(std::remove_if(fmode.begin(), fmode.end(),
                             [](unsigned char c) -> bool {
                               return (c == 'b' || c == 't');
                             }),
              fmode.end());

  // Map Python file mode to C++ file access flags
  std::string access_flags = "";
  std::string::size_type pos = fmode.find_first_of("rwax");
  bool exclusive = false;
  if (fmode[pos] == 'x') {
    access_flags.push_back('w');
    exclusive = true;
  } else {
    access_flags.push_back(fmode[pos]);
  }
  if (fmode.find('+') != std::string::npos) {
    access_flags.push_back('+');
  }
  if (exclusive) {
    access_flags.push_back('x');
  }
  return access_flags;
}

std::string
dump_handler(std::filesystem::path fpath, std::string fmode,
             std::string message, bool show)
{
  std::FILE* stream = stdout;

  if (!fpath.empty()) {
    // Check for invalid file mode flags
    if (fmode.find_first_not_of("rwaxbt+") != std::string::npos) {
      std::string error = "invalid mode: '" + fmode + "'";
      throw py::value_error(error.c_str());
    }
    if ((std::count(fmode.begin(), fmode.end(), 'r') > 1) ||
        (std::count(fmode.begin(), fmode.end(), 'w') > 1) ||
        (std::count(fmode.begin(), fmode.end(), 'a') > 1) ||
        (std::count(fmode.begin(), fmode.end(), 'x') > 1)) {
      std::string error = "invalid mode: '" + fmode + "'";
      throw py::value_error(error.c_str());
    }
    if (!is_exclusive(fmode, 'r', "wax") && !is_exclusive(fmode, 'w', "rax") &&
        !is_exclusive(fmode, 'a', "rwx") && !is_exclusive(fmode, 'x', "rwa")) {
      throw py::value_error(
          "must have exactly one of create/read/write/append mode");
    }
    if ((fmode.find('b') != std::string::npos) &&
        (fmode.find('t') != std::string::npos)) {
      throw py::value_error("can't have text and binary mode at once");
    }

    // Dumping the message to a file requires write access, so
    // a read-only file mode is not acceptable.
    if ((fmode.find('r') != std::string::npos) &&
        (fmode.find('+') == std::string::npos)) {
      throw py::value_error("create/write/append access is required");
    }

    stream = std::fopen(fpath.c_str(), make_access_flags(fmode).c_str());
    if (!stream) {
      switch (errno) {
        case ENOENT: {
          std::string emsg = "[Errno " + std::to_string(errno) +
                             "] File not found: '" + fpath.string() + "'";
          PYCPL_PYBIND11_SET_ERROR(PyExc_FileNotFoundError, emsg.c_str());
          break;
        }
        case EACCES: {
          std::string emsg = "[Errno " + std::to_string(errno) +
                             "] Permission denied: '" + fpath.string() + "'";
          PYCPL_PYBIND11_SET_ERROR(PyExc_PermissionError, emsg.c_str());
          break;
        }
        case EEXIST: {
          std::string emsg = "[Errno " + std::to_string(errno) +
                             "] File exists: '" + fpath.string() + "'";
          PYCPL_PYBIND11_SET_ERROR(PyExc_FileExistsError, emsg.c_str());
          break;
        }
        default: {
          std::string emsg = "[Errno " + std::to_string(errno) + "]" +
                             std::strerror(errno) + ": '" + fpath.string() +
                             "'";
          PYCPL_PYBIND11_SET_ERROR(PyExc_IOError, emsg.c_str());
          break;
        }
      }
      throw py::error_already_set();
    }
  }

  // Dump message to output stream. Although std::fputs() should be fine,
  // be conservative and write one character at a time!
  if ((stream != stdout) || (show == true)) {
    for (unsigned char c : message) {
      int rc = std::fputc(c, stream);
      if ((rc == EOF) && std::ferror(stream)) {
        std::string emsg = "error dumping message to ";
        emsg.append((stream == stdout) ? "'standard output'"
                                       : "stream: '" + fpath.string() + "'");
        PYCPL_PYBIND11_SET_ERROR(PyExc_IOError, emsg.c_str());
        throw py::error_already_set();
      }
    }
  }

  // Release resources if needed
  if (stream != stdout) {
    std::fclose(stream);
  }
  return message;
}
