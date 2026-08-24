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

#include "hdrlfunc/flat_bindings.hpp"

#include <memory>

#include <hdrl_flat.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "hdrlcore/pycpl_mask.hpp"  // IWYU pragma: keep
#include "hdrlfunc/flat.hpp"

namespace py = pybind11;

void
bind_flat(py::module& m)
{
  py::class_<hdrl::func::Flat, std::shared_ptr<hdrl::func::Flat>> flat_class(
      m, "Flat", py::buffer_protocol());

  flat_class.doc() = R"docstring(
      The hdrl.func.Flat class provides an interface to combining single flatfields 
      into a master flatfield with increased signal-to-noise ratio. 

      Once the class has been instantiated with the desired parameters, the master flat 
      can be created using the member function hdrl.func.Flat.compute().

      Parameters
      ----------
      filter_size_x : int
        The dimension of the smoothing kernel (median filter) in the x-direction in pixel units.
      filter_size_y : int
        The dimension of the smoothing kernel (median filter) in the y-direction in pixel units.
      method : hdrl.func.Flat.Mode
        The flat combination algorithm mode, either
        hdrl.func.Flat.FreqLow or hdrl.func.Flat.FreqHigh.
      
      Returns
      -------
      hdrl.func.Flat

      Notes
      -----
      See hdrl.func.Flat.Mode for descriptions of each algorithm.

      Examples
      --------
      .. code-block:: python

        # Example 1: with a stat_mask 
        
        # choose a collapse method
        collapse = hdrl.func.Collapse.Mean()
        # dx and dy are image dimensions
        stat_mask = cpl.core.Mask(dx,dy)
        # set the mask for a window defined by (r1_llx, r1_lly, r1_urx, r1_ury)
        for j in range(r1_lly, r1_ury):
          for i in range(r1_llx, r1_urx):
              stat_mask[j - 1][i - 1] = True
        # create the hdrl.func.Flat instance
        flat = hdrl.func.Flat(filter_size_x, filter_size_y, hdrl.func.Flat.Mode.FreqLow)
        # compute the master flat with imglist being an hdrl.core.ImageList 
        # holding the images to combine into the master flat
        results = flat.compute(imglist, collapse, stat_mask)
        mflat = results.master
        cmap = results.contrib_map

        # Example 2: without a stat_mask

        collapse = hdrl.func.Collapse.Median()
        stat_mask = None
        # create the hdrl.func.Flat instance
        flat = hdrl.func.Flat(filter_size_x, filter_size_y, hdrl.func.Flat.Mode.FreqLow)
        # compute the master flat with imglist being an hdrl.core.ImageList 
        # holding the images to combine into the master flat
        results = flat.compute(imglist, collapse, stat_mask)

      )docstring";

  // this could alternatively be placed elsewhere
  // since several collapse routines make use of it, this is a good place for
  // now
  py::enum_<hdrl_flat_method>(flat_class, "Mode",
                              "The flat combination algorithm mode.")
      .value("FreqLow", HDRL_FLAT_FREQ_LOW,
             R"pydoc(
      This algorithm derives the low frequency part of the master flatfield – often also denoted as the shape of the flatfield. 

      The algorithm multiplicatively normalizes the input images by the median (considered to be 
      noiseless) of the image to unity. An optional static mask `stat_mask` can be provided to the
      algorithm in order to define the pixels that should be taken into account when computing the 
      normalisation factor. This allows the user to normalize the flatfield e.g. only by the illuminated
      section. In the next step, all normalized images are collapsed into a single master flatfield.
      The collapsing can be done with all methods currently implemented in HDRL (see hdrl.func.Collapse
      or Sect. 3.2.2 of the HDRL manual for an overview). Finally, the master flatfield is smoothed by 
      a median filter controlled by `filter_size_x` and `filter_size_y`. The associated error of the 
      final master frame is the error derived via error propagation of the previous steps, i.e. the 
      smoothing itself is considered noiseless. Please note that, if the smoothing kernel is set to unity, 
      i.e. filter_size_x = 1 and filter_size_y = 1, no final smoothing will take place but the resulting 
      masterframe is simply the collapsed normalized flatfield.
      )pydoc")
      .value("FreqHigh", HDRL_FLAT_FREQ_HIGH,
             R"pydoc(
      This algorithm derives the high frequency part of the master flatfield – often also denoted as the 
      pixel-to-pixel variation of the flatfield.

      The algorithm first divides each input image by the smooth image obtained with a median filter 
      (the latter is controlled by the parameters `filter_size_x` and `filter_size_y`). Concerning the 
      error propagation, the smoothed image is considered to be noiseless, i.e. the relative error 
      associated to the normalised images is the same as the one of the input images. Then all residual 
      images are collapsed into a single master flatfield. The collapsing can be done with all methods 
      currently implemented in HDRL (see hdrl.func.Collapse or Sect. 3.2.2 of the HDRL manual for an 
      overview).
      
      To distinguish between illuminated and not illuminated regions/pixels (i.e. orders in an echelle flat image), the user may provide an optional static mask `stat_mask` to the algorithm. In this case the smoothing procedure is done twice, once for the illuminated region and once for the blanked region. This ensures that the information of one region does not influence the other regions during the smoothing process.
      )pydoc");

  flat_class
      .def(py::init<cpl_size, cpl_size, hdrl_flat_method>(),
           py::arg("filter_size_x"), py::arg("filter_size_y"),
           py::arg("method"))
      // todo: make stat_mask std::optional
      .def(
          "compute",
          [](hdrl::func::Flat& self,
             std::shared_ptr<hdrl::core::ImageList> hdrl_data,
             hdrl::func::Collapse collapse, hdrl::core::pycpl_mask stat_mask) {
            return self.compute(hdrl_data, collapse, stat_mask);
          },
          py::arg("hdrl_data"), py::arg("collapse"), py::arg("stat_mask"),
          R"docstring(
       Compute the master flat image and a contribution map image. 

       Parameters
       ----------

       hdrl_data : hdrl.core.ImageList
         The imagelist of images to combine into the master flat.
        
       collapse : hdrl.func.Collapse
         Specifies the collapsing algorithm to apply to `hdrl_data`.

       stat_mask : cpl.core.Mask
         A static mask to distinguish between illuminated and dark regions.
         If no static mask is needed, this can be set to None. 

       Returns
       -------
       namedtuple
         The namedtuple FlatResult contains two output products: a master flat image (hdrl.core.Image) and 
         a contribution map image (cpl.core.Image).
         The respective products may be accessed via the attributes master and contrib_map.
         If desired, the namedtuple can be converted to a dictionary using its ._asdict() method.

       Notes
       -----
       See hdrl.func.Flat.Mode for descriptions of each algorithm.

      )docstring")
      .def_property_readonly("sizex", &hdrl::func::Flat::get_filter_size_x,
                             "int : dimension of the smoothing kernel (median "
                             "filter) in the x-direction in pixel units.")
      .def_property_readonly("sizey", &hdrl::func::Flat::get_filter_size_y,
                             "int : dimension of the smoothing kernel (median "
                             "filter) in the y-direction in pixel units.")
      .def_property_readonly(
          "method", &hdrl::func::Flat::get_method,
          "hdrl.func.Flat.Mode : flat combination algorithm mode.");
}
