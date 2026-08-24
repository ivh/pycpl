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

#include "hdrlfunc/collapse_bindings.hpp"

#include <cpl_type.h>
#include <hdrl_mode_defs.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "hdrlfunc/collapse.hpp"

namespace py = pybind11;

void
bind_collapse(py::module& m)
{
  auto collapse_class =
      py::class_<hdrl::func::Collapse, std::shared_ptr<hdrl::func::Collapse>>(
          m, "Collapse", py::buffer_protocol());

  collapse_class.doc() = R"docstring(
      A hdrl.func.Collapse is a helper class consisting of static functions that conduct Collapse operations like sum,
      mean, or standard deviation with an hdrl.core.Imagelist. hdrl.func.Collapse class cannot be instantiated on its own,
      but only via one of the following constructors:
      - hdrl.func.Collapse.Mean: Mean Collapse operation on HDRL ImageList.
      - hdrl.func.Collapse.Median: Median Collapse operation on HDRL ImageList.
      - hdrl.func.Collapse.SigClip: Sigma Clipped Collapse operation on HDRL ImageList.
      - hdrl.func.Collapse.MinMax: Min-Max Clipped  Collapse operation on HDRL ImageList.
      - hdrl.func.Collapse.Mode: Mode Collapse operation on HDRL ImageList.

      Once the constructors are set up with the desired parameters, you can call  hdrl.core.Collapse.compute() or pass it to
      a function that accepts the Collapse instance (e.g. hdrl.func.Flat.compute).

      Notes
      ----------
      Sigma Clipping or Median Collapse methods are recommended for robust application against outliers (e.g. cosmic ray
      hits). However, Median has a low statistical efficiency, so will display higher uncertainty than images collapsed using
      sigma clipping with few outliers.
      )docstring";

  // this could alternatively be placed elsewhere
  // since several collapse routines make use of it, this is a good place for
  // now
  py::enum_<hdrl_mode_type>(collapse_class, "Method",
                            "Method to use for the mode computation.")
      .value("Median", HDRL_MODE_MEDIAN, R"pydoc(
        This method is the most robust method and can/should be used for very asymmetric point distributions.
        )pydoc")
      .value("Fit", HDRL_MODE_FIT, R"pydoc(
        This method gives accurate results for symmetric distributions but is also more likely to fail.
        )pydoc")
      .value("Weighted", HDRL_MODE_WEIGHTED, R"pydoc(
        This method can/should be used with asymmetric distributions.
        )pydoc");

  collapse_class
  // FIXME: Check if this is just dead code or if it serves some purpose.
  //        Keep disabled as it was commented out!
#if 0
      .def(py::init<>())
      .def_static(
          "collapse",
          [](hdrl::core::ImageList himlist,
             hdrl::func::collapse_type collapse) {
            return hdrl::func::Collapse::collapse(himlist, collapse);
          },
          py::arg("himlist"), py::arg("collapse"))
#endif
      .def(
          "compute",
          [](hdrl::func::Collapse& self,
             std::shared_ptr<hdrl::core::ImageList> himlist) {
            return self.compute(himlist);
          },
          py::arg("himlist"), R"pydoc(
        Implement the Collapse operations for an HDRL Imagelist to a single image.

        Parameters
        ----------
            himlist : hdrl.core.ImageList
                Image to collapse using the operation defined in `self`.
        Returns
        ----------
             namedtuple
                The namedtuple has two components- one hdrl.core.Image (out), containing the collapsed image
                and one cpl.core.Image (contrib), which is the output mask containing an integer map that counts how
                many pixels contributed to each pixel image.

        Notes
        ----------
        These operations can also be accessed directly by the hdrl.core.ImageList objects.

        See Also
        --------
        hdrl.core.ImageList.collapse : Perform Collapse operation on HDRL ImageList.
        hdrl.core.ImageList.collapse_mean : Perform Mean Collapse operation on HDRL ImageList.
        hdrl.core.ImageList.collapse_weighted_mean : Perform Weighted Mean Collapse operation on HDRL ImageList.
        hdrl.core.ImageList.collapse_sigclip : Perform Sigma Clipped Collapse operation on HDRL ImageList.
        hdrl.core.ImageList.collapse_minmax : Perform Min-max clipped Collapse operation on HDRL ImageList.
        hdrl.core.ImageList.collapse_mode : Perform Mode Collapse operation on HDRL ImageList.

      )pydoc")
      .def_static(
          "Mean", []() { return new hdrl::func::Collapse("mean"); },
          R"docstring(
        Creates an instance of hdrl.func.Collapse class with Mean parameters.

        Mean and associated error are computed with standard formulae.

        Returns
        ----------
             namedtuple
                The namedtuple has two components- one hdrl.core.Image (out), containing the collapsed image
                and one cpl.core.Image (contrib), which is the output mask containing an integer map that counts how
                many pixels contributed to each pixel image.

        See Also
        ----------
        hdrl.core.ImageList.collapse : Perform Collapse operation on HDRL ImageList.
        hdrl.core.ImageList.collapse_mean : Perform Mean Collapse operation on HDRL ImageList.
      )docstring")
      .def_static(
          "WeightedMean", []() { return new hdrl::func::Collapse("wmean"); },
          R"docstring(
        Creates an instance of hdrl.func.Collapse class with Weighted Mean parameters.

        Weighted mean and associated error are computed with standard formulae

        Returns
        ----------
             namedtuple
                The namedtuple has two components- one hdrl.core.Image (out), containing the collapsed image
                and one cpl.core.Image (contrib), which is the output mask containing an integer map that counts how
                many pixels contributed to each pixel image.

        See Also
        ---------
        hdrl.core.ImageList.collapse : Perform Collapse operation on HDRL ImageList.
        hdrl.core.ImageList.collapse_weighted_mean : Perform Weighted Mean Collapse operation on HDRL ImageList.

      )docstring")
      .def_static(
          "Median", []() { return new hdrl::func::Collapse("median"); },
          R"pydoc(
        The median collapse of an ImageList to a single image.

        Median and associated error are computed with standard formulae.

        Returns
        --------
             namedtuple
                The namedtuple has two components- one hdrl.core.Image (out), containing the collapsed image,
                one cpl.core.Image (contrib), which is the output mask containing an integer map that counts how
                many pixels contributed to each pixel image.

        See Also
        ----------
        hdrl.core.ImageList.collapse_median : Perform Median Collapse operation on HDRL ImageList.
        hdrl.func.Collapse.compute : Perform Collapse function on an HDRL ImageList to create one HDRL Image.

      )pydoc")
      .def_static(
          "Sigclip",
          [](double kappa_low, double kappa_high, int niter) {
            return new hdrl::func::Collapse(kappa_low, kappa_high, niter);
          },
          py::arg("kappa_low"), py::arg("kappa_high"), py::arg("niter"),
          R"pydoc(
        The sigma clipped collapse of an ImageList to a single image.

        Sigma-clipped mean and associated error, computed similarly as for mean but
        without taking the clipped values into account.

        Parameters
        ----------
        kappa_low : float
            low sigma bound
        kappa_high : float
            high sigma bound
        niter : int
            number of clipping iterators

        Returns
        --------
             namedtuple
                The namedtuple has four components- one hdrl.core.Image (out), containing the collapsed image,
                one cpl.core.Image (contrib), which is the output mask containing an integer map that counts how
                many pixels contributed to each pixel image, one cpl.core.Image (reject_low) containing low rejection threshold,
                one cpl.core.Image (reject_high) containing high rejection threshold.

        See Also
        ----------
        hdrl.core.ImageList.collapse_sigclip : Perform Sigma Clipped Collapse operation on HDRL ImageList.
        hdrl.func.Collapse.compute : Perform Collapse function on an HDRL ImageList to create one HDRL Image.

      )pydoc")
      .def_static(
          "MinMax",
          [](double nlow, double nhigh) {
            return new hdrl::func::Collapse(nlow, nhigh);
          },
          py::arg("nlow"), py::arg("nhigh"), R"docstring(
        Creates an instance of hdrl.func.Collapse class with Min-Max clipped parameters.

        Minmax-clipped mean and associated error, computed similarly as for mean but without taking the clipped values
        into account

        Returns
        ----------
             namedtuple
                The namedtuple has four components- one hdrl.core.Image (out), containing the collapsed image,
                one cpl.core.Image (contrib), which is the output mask containing an integer map that counts how
                many pixels contributed to each pixel image, one cpl.core.Image (reject_low) containing low rejection threshold,
                one cpl.core.Image (reject_high) containing high rejection threshold.

        See Also
        ----------
        hdrl.core.ImageList.collapse : Perform Collapse operation on HDRL ImageList.
        hdrl.core.ImageList.collapse_minmax : Perform MinMax Collapse operation on HDRL ImageList.
      )docstring")
      .def_static(
          "Mode",
          [](double histo_min, double histo_max, double bin_size,
             hdrl_mode_type mode_method, cpl_size error_niter) {
            return new hdrl::func::Collapse(histo_min, histo_max, bin_size,
                                            mode_method, error_niter);
          },
          py::arg("histo_min"), py::arg("histo_max"), py::arg("bin_size"),
          py::arg("mode_method"), py::arg("error_niter"),
          R"docstring(
        Creates an instance of hdrl.func.Collapse class with Mode parameters.

        Parameters
        ----------
        histo_min : float
            minimum value of low pixels to use
        histo_max : float
            maximum value of high pixels to be use
        bin_size : float
            size of the histogram bin
        mode_method : hdrl.func.Collapse.Mode
            mode_method to use for the mode computation
        error_niter : int
            size of the histogram bin

        Returns
        ----------
             namedtuple
                The namedtuple has two components- one hdrl.core.Image (out), containing the collapsed image
                and one cpl.core.Image (contrib), which is the output mask containing an integer map that counts how
                many pixels contributed to each pixel image.

        See Also
        ----------
        hdrl.core.ImageList.collapse : Perform Collapse operation on HDRL ImageList.
        hdrl.core.ImageList.collapse_mode : Perform Mode Collapse operation on HDRL ImageList.
      )docstring")
      .def_property_readonly("nhigh", &hdrl::func::Collapse::get_nhigh)
      .def_property_readonly("nlow", &hdrl::func::Collapse::get_nlow)
      .def_property_readonly("histo_min", &hdrl::func::Collapse::get_histo_min)
      .def_property_readonly("histo_max", &hdrl::func::Collapse::get_histo_max)
      .def_property_readonly("bin_size", &hdrl::func::Collapse::get_bin_size)
      .def_property_readonly("method", &hdrl::func::Collapse::get_method)
      .def_property_readonly("error_niter",
                             &hdrl::func::Collapse::get_error_niter)
      .def_property_readonly("kappa_low", &hdrl::func::Collapse::get_kappa_low)
      .def_property_readonly("kappa_high",
                             &hdrl::func::Collapse::get_kappa_high)
      .def_property_readonly("niter", &hdrl::func::Collapse::get_niter);
}
