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

#include "hdrlfunc/catalogue_bindings.hpp"

#include <memory>

#include "hdrlcore/pycpl_image.hpp"         // IWYU pragma: keep
#include "hdrlcore/pycpl_mask.hpp"          // IWYU pragma: keep
#include "hdrlcore/pycpl_propertylist.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_table.hpp"         // IWYU pragma: keep
#include "hdrlcore/pycpl_wcs.hpp"           // IWYU pragma: keep
#include "hdrlfunc/catalogue.hpp"

namespace py = pybind11;

void
bind_catalogue(py::module_& m)
{
  // necessary to declare ahead of time to get around docstring bug
  // see
  // https://pybind11.readthedocs.io/en/stable/advanced/misc.html#avoiding-c-types-in-docstrings
  auto catalogue_class =
      py::class_<hdrl::func::Catalogue, std::shared_ptr<hdrl::func::Catalogue>>(
          m, "Catalogue");
  auto catalogueresult_class =
      py::class_<hdrl::func::Catalogue::Result,
                 std::shared_ptr<hdrl::func::Catalogue::Result>>(
          m, "CatalogueResult");

  // Catalogue
  catalogue_class.doc() = R"docstring(
      A hdrl.func.Catalogue class provides an interface to object detection and 
      catalogue generation from astronomical images. 
      )docstring";

  catalogue_class
      .def(py::init<int, double, bool, double, bool, int, double, double,
                    double, int>(),
           py::arg("obj_min_pixels"), py::arg("obj_threshold"),
           py::arg("obj_deblending"), py::arg("obj_core_radius"),
           py::arg("bkg_estimate"), py::arg("bkg_mesh_size"),
           py::arg("bkg_smooth_fwhm"), py::arg("det_eff_gain"),
           py::arg("det_saturation"), py::arg("resulttype"),
           R"docstring(
      Create a Catalogue computation object for object detection and catalogue generation.
      
      This class provides functionality to detect objects in astronomical images,
      perform background estimation, and generate catalogues with photometric and
      astrometric measurements.
      
      Parameters
      ----------
      obj_min_pixels : int
          Minimum pixel area for each detected object. Objects smaller than this
          will be rejected from the catalogue.
      obj_threshold : float
          Detection threshold in sigma above sky. Objects below this threshold
          will not be detected.
      obj_deblending : bool
          Use deblending algorithm to separate overlapping objects.
      obj_core_radius : float
          Value of Rcore in pixels for object detection.
      bkg_estimate : bool
          Estimate background from input image. If False, it is assumed the input
          is already background corrected with median 0.
      bkg_mesh_size : int
          Background smoothing box size in pixels.
      bkg_smooth_fwhm : float
          The FWHM of the Gaussian kernel used in convolution for object detection.
      det_eff_gain : float
          Detector gain value to rescale and convert intensity to electrons.
      det_saturation : float
          Detector saturation value in ADU.
      resulttype : int
          Requested output type using bitwise flags:
          - 1: Background image only
          - 2: Segmentation map only  
          - 4: Complete catalogue only
          - 7: All outputs (background, segmentation map, and catalogue)
          
      Example
      -------
      .. code-block:: python
          
          # Create catalogue object for basic object detection
          cat = hdrl.func.Catalogue(
              obj_min_pixels=10,
              obj_threshold=3.0,
              obj_deblending=True,
              obj_core_radius=2.0,
              bkg_estimate=True,
              bkg_mesh_size=64,
              bkg_smooth_fwhm=2.0,
              det_eff_gain=1.0,
              det_saturation=65535.0,
              resulttype=7  # All outputs
          )
          
      Notes
      -----
      The algorithm performs local sky background estimation and removal,
      detects objects and blends, assigns image pixels to each object,
      and performs astrometry, photometry and shape analysis on the
      detected objects.
      )docstring")
      .def("compute", &hdrl::func::Catalogue::compute, py::arg("image"),
           py::arg("confidence_map") = hdrl::core::pycpl_image(),
           py::arg("wcs") = hdrl::core::pycpl_wcs(),
           R"docstring(
      Compute object catalogue from an astronomical image.
      
      This function builds an object catalogue from the input image. The algorithm
      performs local sky background estimation and removal, detects objects and blends,
      assigns image pixels to each object, and performs astrometry, photometry and
      shape analysis on the detected objects.
      
      Parameters
      ----------
      image : cpl.core.Image
          Input image for catalogue computation. The image should be in
          units of ADU (Analog-to-Digital Units).
      confidence_map : cpl.core.Image, optional
          Optional confidence map providing uncertainty information for each pixel.
          If None, no confidence map is used. Must contain only positive numbers
          if provided.
      wcs : cpl.core.WCS, optional
          Optional WCS information for astrometric measurements. If None,
          no WCS information is used and astrometry will not be performed.
      
      Returns
      -------
      hdrl.func.CatalogueResult
          A result object containing:
          - catalogue: cpl.core.Table with the object catalogue including positions, fluxes, and shape parameters
          - segmentation_map: cpl.core.Image with the segmentation map showing object assignments
          - background: cpl.core.Image with the estimated background
          - qclist: cpl.core.PropertyList with quality control information
      
      Example
      -------
      .. code-block:: python
          
          # Load image and WCS
          image = cpl.core.Image.load("science_image.fits")
          wcs = cpl.drs.WCS(cpl.core.PropertyList.load("science_image.fits"))
          
          # Create catalogue object
          cat = hdrl.func.Catalogue(
              obj_min_pixels=10,
              obj_threshold=3.0,
              obj_deblending=True,
              obj_core_radius=2.0,
              bkg_estimate=True,
              bkg_mesh_size=64,
              bkg_smooth_fwhm=2.0,
              det_eff_gain=1.0,
              det_saturation=65535.0,
              resulttype=7
          )
          
          # Compute catalogue
          result = cat.compute(image, wcs=wcs)
          
          # Access results
          catalogue = result.catalogue
          segmap = result.segmentation_map
          background = result.background
          qc_info = result.qclist
          
      Notes
      -----
      The confidence_map must contain only positive numbers if provided.
      
      The function automatically handles image type conversion to double precision
      if needed.
      
      Raises
      ------
      hdrlcore.NullInputError
          If image is None.
      hdrlcore.IllegalInputError
          If parameter validation fails (e.g., invalid parameter values).
      hdrlcore.IncompatibleInputError
          If confidence_map contains negative values or other incompatible inputs.
      )docstring")
      .def_property_readonly(
          "obj_min_pixels", &hdrl::func::Catalogue::get_obj_min_pixels,
          "int : Minimum pixel area for each detected object")
      .def_property_readonly("obj_threshold",
                             &hdrl::func::Catalogue::get_obj_threshold,
                             "float : Detection threshold in sigma above sky")
      .def_property_readonly("obj_deblending",
                             &hdrl::func::Catalogue::get_obj_deblending,
                             "bool : Use deblending algorithm")
      .def_property_readonly("obj_core_radius",
                             &hdrl::func::Catalogue::get_obj_core_radius,
                             "float : Value of Rcore in pixels")
      .def_property_readonly("bkg_estimate",
                             &hdrl::func::Catalogue::get_bkg_estimate,
                             "bool : Estimate background from input")
      .def_property_readonly("bkg_mesh_size",
                             &hdrl::func::Catalogue::get_bkg_mesh_size,
                             "int : Background smoothing box size")
      .def_property_readonly(
          "bkg_smooth_fwhm", &hdrl::func::Catalogue::get_bkg_smooth_fwhm,
          "float : FWHM of Gaussian kernel for object detection")
      .def_property_readonly("det_eff_gain",
                             &hdrl::func::Catalogue::get_det_eff_gain,
                             "float : Detector gain value")
      .def_property_readonly("det_saturation",
                             &hdrl::func::Catalogue::get_det_saturation,
                             "float : Detector saturation value")
      .def_property_readonly("resulttype",
                             &hdrl::func::Catalogue::get_resulttype,
                             "int : Requested output type");

  // CatalogueResult
  catalogueresult_class.doc() = R"docstring(
      A hdrl.func.CatalogueResult class is a container for the results of hdrl.func.Catalogue.compute().
      The results consist of a catalogue table, segmentation map, background image, and quality control information.
      
      These can be accessed via the catalogue, segmentation_map, background, and qclist attributes of the object.
      
      Example
      -------
      .. code-block:: python
          
          result = hdrl.func.Catalogue.compute(image, wcs=wcs)
          catalogue = result.catalogue
          segmap = result.segmentation_map
          background = result.background
          qc_info = result.qclist
      )docstring";

  catalogueresult_class
      .def_readonly("catalogue", &hdrl::func::Catalogue::Result::catalogue,
                    "cpl.core.Table : Object catalogue with positions, fluxes, "
                    "and shape parameters")
      .def_readonly(
          "segmentation_map", &hdrl::func::Catalogue::Result::segmentation_map,
          "cpl.core.Image : Segmentation map showing object assignments")
      .def_readonly("background", &hdrl::func::Catalogue::Result::background,
                    "cpl.core.Image : Estimated background image")
      .def_readonly("qclist", &hdrl::func::Catalogue::Result::qclist,
                    "cpl.core.PropertyList : Quality control information")
      .def("__repr__", [](const hdrl::func::Catalogue::Result&) {
        return "CatalogueResult(catalogue=<cpl.core.Table>, "
               "segmentation_map=<cpl.core.Image>, "
               "background=<cpl.core.Image>, qclist=<cpl.core.PropertyList>)";
      });
}
