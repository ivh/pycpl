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

#include "cpldrs/wlcalib_bindings.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <pybind11/stl.h>

#include "cplcore/bivector.hpp"
#include "cplcore/error.hpp"
#include "cplcore/polynomial.hpp"
#include "cplcore/vector.hpp"
#include "cpldrs/wlcalib.hpp"

void
bind_wlcalib(py::module& m)
{
  py::module wlcalib =
      m.def_submodule("wlcalib", "Wavelength calibration functions");

  py::class_<cpl::drs::SlitModel, std::shared_ptr<cpl::drs::SlitModel>>
      slitmodel_class(wlcalib, "SlitModel", R"pydoc(
        Line model to generate a spectrum.

        The model comprises these elements:

        - Slit Width
        - FWHM of transfer function
        - Truncation threshold of the transfer function
        - Catalog of lines (typically arc or sky)

        The units of the X-values of the lines is a length, it is assumed to be the
        same as that of the Y-values of the dispersion relation (e.g. meter), the
        units of slit width and the FWHM are assumed to be the same as the X-values
        of the dispersion relation (e.g. pixel), while the units of the produced
        spectrum will be that of the Y-values of the lines.

        The line profile is truncated at this distance [pixel] from its maximum:

        .. math::

            x_{\mathrm{max}} = w/2 + k\sigma

        where w is the slit width, k is the threshold and
        :math:`\sigma = w_{\mathrm{FWHM}}/(2\sqrt{2\log(2)})`
        where :math:`w_{\mathrm{FWHM}}` is the Full Width at Half Maximum (FWHM)
        of the transfer function.

        The units of the X-values of the lines is a length, it is assumed to be the
        same as that of the Y-values of the dispersion relation (e.g. meter), the
        units of slit width and the FWHM are assumed to be the same as the X-values
        of the dispersion relation (e.g. pixel), while the units of the produced
        spectrum will be that of the Y-values of the lines.

        Parameters
        ----------
        catalog : cpl.core.Bivector
            the catalog of lines to be used by the spectrum filler
        wfwhm : float
            the FWHM of th etransfer function to be used by the spectrum filler
        wslit : float
            the slit width to be used by the spectrum filler
        spectrum_size : int
            The size of the spectrum, returned by the spectrum filler functions
        threshold : float
            The threshold for truncating the transfer function, default 5 (recommended).
      )pydoc");


  py::object NamedTuple = py::module::import("collections").attr("namedtuple");

  py::object best_find_1d_tuple = NamedTuple(
      "BestFit1dResult",
      py::cast(std::vector<std::string>({"result", "xcmax", "xcorrs"})));


  slitmodel_class
      .def(py::init<std::shared_ptr<cpl::core::Bivector>, double, double, int,
                    double>(),
           py::arg("catalog"), py::arg("wfwhm"), py::arg("wslit"),
           py::arg("spectrum_size"), py::arg("threshold") = 5.0,
           R"pydoc(
    Create a new line model to be initialized.



    Return
    ------
    cpl.drs.SlitModel
        Newly created line model

    Raises
    ------
    cpl.core.IllegalInputError
        if threshold, wfwhm or wslit is non-positive
    )pydoc")
      .def_property("wslit", &cpl::drs::SlitModel::get_wslit,
                    &cpl::drs::SlitModel::set_wslit,
                    R"pydoc(
    Slit width to be used by the spectrum filler.
    )pydoc")
      .def_property(
          "wfwhm", &cpl::drs::SlitModel::get_wfwhm,
          &cpl::drs::SlitModel::set_wfwhm,
          "Set the FWHM of the transfer function to be used by the spectrum "
          "filler.")
      .def_property("threshold", &cpl::drs::SlitModel::get_threshold,
                    &cpl::drs::SlitModel::set_threshold, R"pydoc(
    The FWHM of the transfer function to be used by the spectrum filler.

    The threshold should be high enough to ensure a good line profile, but 
    not too high to make the spectrum generation too costly. 5 is the CPL recommended.
    )pydoc")
      .def_property("catalog", &cpl::drs::SlitModel::get_catalog,
                    &cpl::drs::SlitModel::set_catalog,
                    R"pydoc(
    Set the catalog of lines to be used by the spectrum filler

    The values in the X-vector must be increasing. The catalog values will be copied into 
    the slitmodel and thus modification of the passed Bivector will not impact the internal
    Slitmodel catalog, and vice versa. 
    )pydoc")
      .def("fill_line_spectrum", &cpl::drs::SlitModel::fill_line_spectrum,
           py::arg("dispersion"),
           R"pydoc(
    Generate a 1D spectrum from a model and a dispersion relation from the line intensities.

    Parameters
    ----------
    disp : cpl.core.Polynomial
        1D-Dispersion relation, at least of degree 1

    Returns
    -------
    cpl.core.Vector
        A vector of self.spectrum_size, containing the spectrum generated.

    Notes
    -----
    Each line profile is given by the convolution of the Dirac delta function
    with a Gaussian with :math:`sigma = w_{\mathrm{FWHM}}/(2\sqrt{2\log(2)})` and
    a top-hat with the slit width as width. This continuous line profile is then
    integrated over each pixel, wherever the intensity is above the threshold
    set by the given model. For a given line the value on a given pixel
    requires the evaluation of two calls to erf().
    )pydoc")
      .def("fill_logline_spectrum", &cpl::drs::SlitModel::fill_logline_spectrum,
           py::arg("dispersion"),
           R"pydoc(
    Generate a 1D spectrum from a model and a dispersion relation from log(1 + the line intensities).

    Parameters
    ----------
    disp : cpl.core.Polynomial
        1D-Dispersion relation, at least of degree 1

    Returns
    -------
    cpl.core.Vector
        A vector of self.spectrum_size, containing the spectrum generated.

    Notes
    -----
    Each line profile is given by the convolution of the Dirac delta function
    with a Gaussian with ..math:: sigma = w_{\mathrm{FWHM}}/(2\sqrt{2\log(2)}) and a
    top-hat with the slit width as width. This continuous line profile is then
    integrated over each pixel, wherever the intensity is above the threshold
    set by the given model. For a given line the value on a given pixel
    requires the evaluation of two calls to erf().
    )pydoc")
      .def("fill_line_spectrum_fast",
           &cpl::drs::SlitModel::fill_line_spectrum_fast, py::arg("dispersion"),
           R"pydoc(
    Generate a 1D spectrum from a model and a dispersion relation from the line intensities, approximating the line profile for speed.
    
    The approximation preserves the position of the maximum, the symmetry and
    the flux of the line profile.

    The fast spectrum generation can be useful when the model spectrum includes
    many catalog lines.

    Parameters
    ----------
    disp : cpl.core.Polynomial
        1D-Dispersion relation, at least of degree 1

    Returns
    -------
    cpl.core.Vector
        A vector of self.spectrum_size, containing the spectrum generated.

    Notes
    -----
    Each line profile is given by the convolution of the Dirac delta function
    with a Gaussian with

    .. math::

        \sigma = w_{\mathrm{FWHM}}/(2\sqrt{2\log(2)}) and a

    top-hat with the slit width as width. This continuous line profile is then
    integrated over each pixel, wherever the intensity is above the threshold
    set by the given model. The use of a given line in a spectrum requires the 
    evaluation of four calls to erf().

    )pydoc")
      .def("fill_logline_spectrum_fast",
           &cpl::drs::SlitModel::fill_logline_spectrum_fast,
           py::arg("dispersion"),
           R"pydoc(
    Generate a 1D spectrum from a model and a dispersion relation from
    log(1 + the line intensities), approximating the line profile for speed.

    The approximation preserves the position of the maximum, the symmetry and the
    flux of the line profile.

    The fast spectrum generation can be useful when the model spectrum includes many
    catalog lines.

    Parameters
    ----------
    disp : cpl.core.Polynomial
        1D-Dispersion relation, at least of degree 1

    Returns
    -------
    cpl.core.Vector
        A vector of self.spectrum_size, containing the spectrum generated.

    Notes
    -----
    Each line profile is given by the convolution of the Dirac delta function
    with a Gaussian with :math:`\sigma = w_{\mathrm{FWHM}}/(2\sqrt{2\log(2)})` and a
    top-hat with the slit width as width. This continuous line profile is then
    integrated over each pixel, wherever the intensity is above the threshold
    set by the given model. The use of a given line in a spectrum requires the 
    evaluation of four calls to erf().
    )pydoc")
      .def(
          "find_best_1d",
          [slitmodel_class, best_find_1d_tuple](
              cpl::drs::SlitModel& self, const cpl::core::Vector& spectrum,
              const cpl::core::Vector& wl_search, cpl::drs::size nsamples,
              cpl::drs::size hsize,

              py::function filler,
              // Uncomment below when we get around to properly allowing
              // custom-written filler functions std::function<void(const
              // cpl::drs::SlitModel&, const cpl::core::Vector&, const
              // cpl::core::Polynomial&)> evaluatefiller,
              std::optional<cpl::core::Polynomial> guess) -> py::object {
            cpl::drs::filler filler_enum;

            // As this function is low priority this is a temporary measure
            // so users can at least use fit_best_1d with the CPL functions it
            // is made for If there are resources in the future then hopefully
            // the below can be replaced with passing an actual callback
            if (filler.is(slitmodel_class.attr("fill_line_spectrum"))) {
              filler_enum =
                  cpl::drs::filler::LINE;  // Changed enum based on the function
                                           // passed, pass the enum to c++ layer
            } else if (filler.is(
                           slitmodel_class.attr("fill_logline_spectrum"))) {
              filler_enum = cpl::drs::filler::LOGLINE;
            } else if (filler.is(
                           slitmodel_class.attr("fill_line_spectrum_fast"))) {
              filler_enum = cpl::drs::filler::LINE_FAST;
            } else if (filler.is(slitmodel_class.attr(
                           "fill_logline_spectrum_fast"))) {
              filler_enum = cpl::drs::filler::LOGLINE_FAST;
            } else {
              throw cpl::core::IllegalInputError(
                  PYCPL_ERROR_LOCATION,
                  "filler function must be from cpl:  "
                  "cpl.drs.wlcalib.SlitModel.fill_line_spectrum, "
                  "cpl.drs.wlcalib.SlitModel.fill_line_spectrum_fast, "
                  "cpl.drs.wlcalib.SlitModel.fill_logline_spectrum, "
                  "cpl.drs.wlcalib.SlitModel.fill_logline_spectrum_fast");
            }

            py::tuple res = py::cast(self.find_best_1d(
                spectrum, wl_search, nsamples, hsize, filler_enum, guess));
            return best_find_1d_tuple(*res);
          },
          py::arg("spectrum"), py::arg("wl_search"), py::arg("nsamples"),
          py::arg("hsize"), py::arg("filler"),
          py::arg("guess").none(true) = py::none(),
          R"pydoc(
    Find the best 1D dispersion polynomial in a given search space

    Find the polynomial that maximizes the cross-correlation between an
    observed 1D-spectrum and a model spectrum based on the polynomial
    dispersion relation.
    
    Parameters
    ----------
    spectrum : cpl.core.Vector
        The vector with the observed 1D-spectrum
    wl_search : cpl.core.Vector
        Search range around the anchor points
    nsamples : int 
        Number of samples around the anchor points
    hsize : int
        Maximum (pixel) displacement of the polynomial guess
    filler : function(cpl.core.Vector, cpl.core.Polynomial)
        The function used to make the spectrum. Currently only supports fill functions
        in cpl.drs.wlcalib, including:

        - cpl.drs.wlcalib.SlitModel.fill_line_spectrum
        - cpl.drs.wlcalib.SlitModel.fill_line_spectrum_fast
        - cpl.drs.wlcalib.SlitModel.fill_logline_spectrum
        - cpl.drs.wlcalib.SlitModel.fill_logline_spectrum_fast
    guess : cpl.core.Polynomial, optional
        1D-polynomial with the guess. If not given the guess will simply be a 1D-Polynomial 
        with no coefficients

    Return
    -------
    NamedTuple(cpl.core.Polynomial, float, cpl.core.Vector)
        NamedTuple in the format (result, xcmax, xcoors) where:

        - result: the resulting best 1D dispersion polynomial
        - xcmax: the maximum cross-correlation
        - xcoors: the correlation values    

    Raises
    ------
    cpl.core.InvalidTypeError
        if an input polynomial is not 1D
    cpl.core.IllegalInputError
        if wl_search size is less than 2, nsamples is less than 1, hsize is negative, or 
        wl_search contains a zero search bound.
    cpl.core.DataNotFoundError
        if no model spectra can be created with the calling SlitModel and passed filler
    )pydoc");
}