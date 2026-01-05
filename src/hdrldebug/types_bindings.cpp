// This file is part of the PyHDRL Python language bindings
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

#include "hdrldebug/types_bindings.hpp"

#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "hdrlcore/pycpl_image.hpp"         // IWYU pragma: keep
#include "hdrlcore/pycpl_imagelist.hpp"     // IWYU pragma: keep
#include "hdrlcore/pycpl_mask.hpp"          // IWYU pragma: keep
#include "hdrlcore/pycpl_propertylist.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_table.hpp"         // IWYU pragma: keep
#include "hdrlcore/pycpl_vector.hpp"        // IWYU pragma: keep
#include "hdrlcore/pycpl_wcs.hpp"           // IWYU pragma: keep
#include "hdrldebug/types.hpp"

namespace py = pybind11;

void
bind_hdrl_types(py::module& m)
{
  py::class_<hdrl::debug::Types, std::shared_ptr<hdrl::debug::Types>>
      types_class(m, "Types", py::buffer_protocol());

  types_class.doc() = R"docstring(
      A hdrl.debug.Types is a helper class to run tests for the custom type converters.
      The custom type caster handles the conversion dynamically. 

      It is not intended to be used by pipeline developers. 

      It is only intended for use by PyHDRL developers.
      )docstring";

  types_class.def(py::init<>())
      .def_static(
          "table",
          [](hdrl::core::pycpl_table tab) {
            return hdrl::debug::Types::table(tab);
          },
          py::arg("tab"),
          R"pydoc(
      Utility function to help test the custom type caster for cpl.core.Table type.

      Parameters
      ----------
      tab : cpl.core.Table
            Table to be converted

      Returns
      -------
      cpl.core.Table
            A newly allocated Table.

       )pydoc")
      .def_static(
          "propertylist",
          [](hdrl::core::pycpl_propertylist plist) {
            return hdrl::debug::Types::propertylist(plist);
          },
          py::arg("plist"),
          R"pydoc(
      Utility function to help test the custom type caster for cpl.core.PropertyList type.

      Parameters
      ----------
      plist : cpl.core.PropertyList
            PropertyList to be converted

      Returns
      -------
      cpl.core.PropertyList
            A newly allocated PropertyList.

       )pydoc")
      .def_static(
          "imagelist",
          [](hdrl::core::pycpl_imagelist imlist) {
            return hdrl::debug::Types::imagelist(imlist);
          },
          py::arg("imagelist"),
          R"pydoc(
      Utility function to help test the custom type caster for cpl.core.ImageList type.

      Parameters
      ----------
      imagelist : cpl.core.ImageList
            ImageList to be converted

      Returns
      -------
      cpl.core.ImageList
            A newly allocated ImageList.

       )pydoc")
      .def_static(
          "mask",
          [](hdrl::core::pycpl_mask m) { return hdrl::debug::Types::mask(m); },
          py::arg("m"),
          R"pydoc(
      Utility function to help test the custom type caster for cpl.core.Mask type.

      Parameters
      ----------
      m : cpl.core.Mask
            Mask to be converted

      Returns
      -------
      cpl.core.Mask
            A newly allocated mask.

       )pydoc")
      .def_static(
          "wcs",
          [](hdrl::core::pycpl_wcs w) { return hdrl::debug::Types::wcs(w); },
          py::arg("w"),
          R"pydoc(
      Utility function to help test the custom type caster for cpl.drs.WCS type.

      Parameters
      ----------
      w : cpl.drs.WCS
            WCS to be converted
      Returns
      -------
      cpl.drs.WCS
            A newly allocated WCS.

       )pydoc")
      .def_static(
          "vector",
          [](hdrl::core::pycpl_vector v) {
            return hdrl::debug::Types::vector(v);
          },
          py::arg("v"),
          R"pydoc(
      Utility function to help test the custom type caster for cpl.core.Vector type.

      Parameters
      ----------
      v : cpl.core.Vector
            Vector to be converted

      Returns
      -------
      cpl.core.Vector
            A newly allocated Vector.

       )pydoc")
      .def_static(
          "image",
          [](hdrl::core::pycpl_image im) {
            return hdrl::debug::Types::image(im);
          },
          py::arg("image"),
          R"pydoc(
      Utility function to help test the custom type caster for cpl.core.Image type.

      Parameters
      ----------
      image : cpl.core.Image
            Image to be converted

      Returns
      -------
      cpl.core.Image
            A newly allocated Image.

       )pydoc");
}
