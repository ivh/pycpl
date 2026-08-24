// This file is part of the PyHDRL Python language bindings
// Copyright (C) 2023-2026 European Southern Observatory
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

#include "hdrlfunc/fringe_bindings.hpp"

#include <memory>

#include <pybind11/numpy.h>

#include "hdrlcore/pycpl_image.hpp"      // IWYU pragma: keep
#include "hdrlcore/pycpl_imagelist.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_mask.hpp"       // IWYU pragma: keep
#include "hdrlcore/pycpl_table.hpp"      // IWYU pragma: keep
#include "hdrlfunc/fringe.hpp"

namespace py = pybind11;

void
bind_fringe(py::module& m)
{
  // necessary to declare ahead of time to get around docstring bug see
  // https://pybind11.readthedocs.io/en/stable/advanced/misc.html#avoiding-c-types-in-docstrings
  auto fringe_class =
      py::class_<hdrl::func::Fringe, std::shared_ptr<hdrl::func::Fringe>>(
          m, "Fringe", py::buffer_protocol());
  auto fringecorrectresult_class =
      py::class_<hdrl::func::Fringe::CorrectResult,
                 std::shared_ptr<hdrl::func::Fringe::CorrectResult>>(
          m, "FringeCorrectResult");

  fringe_class.doc() = R"docstring(
      The hdrl.func.Fringe class provides an interface to fringe pattern detection 
      and correction in astronomical images. 

      The class provides two main methods:
      - compute(): Creates a master fringe pattern from a list of fringe images
      - correct(): Applies fringe correction to images using a master fringe pattern

      Parameters
      ----------
      None
        The Fringe class does not require any parameters for initialization.
      
      Returns
      -------
      hdrl.func.Fringe

      Notes
      -----
      The fringe functions work with HDRL image lists and can handle optional
      object masks and static masks for improved fringe detection and correction.

      Examples
      --------
      .. code-block:: python

        # Example 1: Compute master fringe pattern
        
        # Create fringe object
        fringe = hdrl.func.Fringe()
        
        # Create collapse method for combining images
        collapse = hdrl.func.Collapse.Mean()
        
        # Optional: Create static mask for fringe regions
        stat_mask = cpl.core.Mask(dx, dy)
        # Set mask for fringe-affected regions
        
        # Compute master fringe pattern
        # ilist_fringe is an hdrl.core.ImageList with fringe images
        # ilist_obj is optional cpl.core.ImageList with object masks (can be None)
        fringe.compute(ilist_fringe, ilist_obj, stat_mask, collapse)
        master_fringe = fringe.master
        contrib_map = fringe.contrib_map
        qctable = fringe.qctable

        # Example 2: Apply fringe correction
        
        # Apply correction to images using master fringe
        # ilist_fringe is an hdrl.core.ImageList with images to correct
        # masterfringe is the master fringe pattern from compute()
        result = fringe.correct(ilist_fringe, ilist_obj, stat_mask, masterfringe)
        qctable = result.qctable

      )docstring";

  fringe_class
      .def(py::init<>(), R"docstring(
          Create a Fringe computation object.
          
          The Fringe class provides methods for detecting and correcting
          fringe patterns in astronomical images.
      )docstring")
      .def(
          "compute",
          [](hdrl::func::Fringe& self,
             std::shared_ptr<hdrl::core::ImageList> ilist_fringe,
             hdrl::core::pycpl_imagelist ilist_obj,
             hdrl::core::pycpl_mask stat_mask,
             hdrl::func::Collapse collapse_params) {
            self.compute(ilist_fringe, ilist_obj, stat_mask, collapse_params);
          },
          py::arg("ilist_fringe"), py::arg("ilist_obj") = py::none(),
          py::arg("stat_mask") = py::none(), py::arg("collapse_params"),
          R"docstring(
               Compute a master fringe pattern from a list of fringe images.
               
               This method combines multiple fringe images to create a master fringe
               pattern with improved signal-to-noise ratio. The method uses the
               specified collapse algorithm to combine the images.
               
               Parameters
               ----------
               ilist_fringe : hdrl.core.ImageList
                   List of fringe images to combine into master fringe pattern.
               ilist_obj : cpl.core.ImageList or None, optional
                   Optional list of object masks. If provided, must have same size
                   and dimensions as ilist_fringe. Default is None.
               stat_mask : cpl.core.Mask or None, optional
                   Optional static mask for fringe regions. If provided, must have
                   same dimensions as fringe images. Default is None.
               collapse_params : hdrl.func.Collapse
                   Collapse method for combining fringe images.
               
               Returns
               -------
               None
               
               Raises
               ------
               hdrl.core.NullInputError
                   If ilist_fringe is None or empty, or if collapse_params is None.
               hdrl.core.IncompatibleInputError
                   If ilist_obj dimensions don't match ilist_fringe, or if
                   stat_mask dimensions don't match fringe images.
           )docstring")
      .def_property_readonly(
          "master", &hdrl::func::Fringe::get_master,
          "Master fringe pattern from the last compute() call.")
      .def_property_readonly("contrib_map",
                             &hdrl::func::Fringe::get_contrib_map,
                             "Contribution map from the last compute() call.")
      .def_property_readonly("qctable", &hdrl::func::Fringe::get_qctable,
                             "QC table from the last compute() call.")
      .def(
          "correct",
          [](hdrl::func::Fringe& self,
             std::shared_ptr<hdrl::core::ImageList> ilist_fringe,
             hdrl::core::pycpl_imagelist ilist_obj,
             hdrl::core::pycpl_mask stat_mask,
             std::shared_ptr<hdrl::core::Image> masterfringe)
              -> hdrl::func::Fringe::CorrectResult {
            return self.correct(ilist_fringe, ilist_obj, stat_mask,
                                masterfringe);
          },
          py::arg("ilist_fringe"), py::arg("ilist_obj") = py::none(),
          py::arg("stat_mask") = py::none(),
          py::arg("masterfringe") = py::none(),
          R"docstring(
               Apply fringe correction to images using a master fringe pattern.
               
               This method corrects fringe patterns in images by subtracting
               a scaled version of the master fringe pattern. The scaling is
               determined by fitting the fringe amplitude for each image.
               
               Parameters
               ----------
               ilist_fringe : hdrl.core.ImageList
                   List of images to correct for fringe patterns.
               ilist_obj : cpl.core.ImageList or None, optional
                   Optional list of object masks. If provided, must have same size
                   and dimensions as ilist_fringe. Default is None.
               stat_mask : cpl.core.Mask or None, optional
                   Optional static mask for fringe regions. If provided, must have
                   same dimensions as fringe images. Default is None.
               masterfringe : hdrl.core.Image
                   Master fringe pattern to use for correction. If None, uses
                   the master computed by compute().
               
               Returns
               -------
               FringeCorrectResult
                   Result object containing a cpl.core.Table, the quality control table with
                   background levels and fringe amplitudes for each image
               
               Raises
               ------
               hdrl.core.NullInputError
                   If ilist_fringe is None or empty, or if masterfringe is None.
               hdrl.core.IncompatibleInputError
                   If ilist_obj dimensions don't match ilist_fringe, if
                   stat_mask dimensions don't match fringe images, or if
                   masterfringe dimensions don't match fringe images.
               
               Notes
               -----
               The correction is applied in-place to the images in ilist_fringe.
               The method modifies the input images directly.
           )docstring");

  // FringeCorrectResult
  fringecorrectresult_class.doc() = R"docstring(
      A hdrl.func.FringeCorrectResult class is a container for the results of hdrl.func.Fringe.correct().
      The results consist of a quality control table with background levels and fringe amplitudes.
      
      These can be accessed via the qctable attribute of the object.
      
      Example
      -------
      .. code-block:: python
          
          result = fringe.correct(ilist_fringe, ilist_obj, stat_mask, masterfringe)
          qctable = result.qctable
      )docstring";

  fringecorrectresult_class
      .def_readonly("qctable", &hdrl::func::Fringe::CorrectResult::qctable,
                    "cpl.core.Table : Quality control table with background "
                    "levels and fringe amplitudes")
      .def("__repr__", [](const hdrl::func::Fringe::CorrectResult&) {
        return "FringeCorrectResult(qctable=<cpl.core.Table>)";
      });
}
