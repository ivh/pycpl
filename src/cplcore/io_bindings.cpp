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

#include "cplcore/io_bindings.hpp"

#include <cpl_io.h>

namespace py = pybind11;

void
bind_io(py::module& m)
{
  // Enums
  py::enum_<_cpl_io_type_>(m, "io", py::arithmetic(),
                           "I/O modes for file storage operations. See "
                           "http://heasarc.nasa.gov/docs/software/fitsio/"
                           "compression.html for compression mode details.")
      .value("CREATE", CPL_IO_CREATE,
             "Overwrite the file, if it already exists.")
      .value("EXTEND", CPL_IO_EXTEND, "Append a new extension to the file.")
      .value("APPEND", CPL_IO_APPEND,
             "Append to the last data unit of the file.")
      .value("COMPRESS_GZIP", CPL_IO_COMPRESS_GZIP,
             "Use FITS tiled-image compression with GZIP algorithm.")
      .value("COMPRESS_RICE", CPL_IO_COMPRESS_RICE,
             "Use FITS tiled-image compression with RICE algorithm.")
      .value("COMPRESS_HCOMPRESS", CPL_IO_COMPRESS_HCOMPRESS,
             "Use FITS tiled-image compression with HCOMPRESS algorithm.")
      .value("COMPRESS_PLIO", CPL_IO_COMPRESS_PLIO,
             "Use FITS tiled-image compression with PLIO algorithm.")
      // TODO: This is a temporary solution to keep consistent with how CPL_IO
      // enums are used in C. This isn't often done in Python so work out past
      // August if we should keep it like this or move towards making IO options
      // as args in appropriate save functions.
      .def("__or__",
           [](_cpl_io_type_& a, _cpl_io_type_& b) -> int { return a | b; });
}
