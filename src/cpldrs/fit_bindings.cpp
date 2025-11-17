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

#include "cpldrs/fit_bindings.hpp"

#include <cstdio>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <cpl_array.h>
#include <cpl_fit.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "cplcore/error.hpp"
#include "cplcore/image.hpp"
#include "cplcore/matrix.hpp"
#include "cplcore/vector.hpp"
#include "cplcore/window_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive
#include "cpldrs/fit.hpp"

namespace py = pybind11;

using size = cpl::core::size;

void
bind_fit(py::module& m)
{
  py::module mfit =
      m.def_submodule("fit", "High-level functions for non-linear fitting");
  std::string lvmq_pydoc_str = R"pydoc(
        Fit a function to a set of data.

        This function makes a minimum chi squared fit of the specified function to the specified
        data set using a Levenberg-Marquardt algorithm.

        Parameters
        ----------
        x : cpl.core.Matrix
            N x D matrix of the positions to fit. Each matrix row is a D-dimensional position.
        y : cpl.core.Vector
            The N values to fit.
        starting_guess_params : cpl.core.Vector
            Vector containing M fit parameters used for the evaulate function Must contain
            a guess solution on input.
        participating_parameters : list or array of bools or None
            Optional array of size M defining which fit parameters participate
            in the fit (non-zero) and which fit parameters are held
            constant (zero). Pass None to fit all parameters.
        evaluate : function(list or array of float, list or array of float) -> float
            Function that evaluates the fit function
            at the position specified by the first argument (an array of
            size D) using the fit parameters specified by the second
            argument (list or array of size M). The result is the return value of
            the function.
        evaluate_derivatives : function(list or array of float, list or array of float) -> list of float
            Function that evaluates the first order partial
            derivatives of the fit function with respect to the fit
            parameters at the position specified by the first argument
            (an array of size D) using the parameters specified by the
            second argument (an array of size M). The result is the return
            value of the function, being a float array of size M).
        sigma_y : cpl.core.Vector
            Vector of size N containing the uncertainties of
            the y-values.
        relative_tolerance : float, optional
            The algorithm converges by definition if the relative
            decrease in chi squared is less than `tolerance`
            `tolerance_count` times in a row. The current default value
            is the CPL recommended default of %.2f.
        tolerance_count : int
            The algorithm converges by definition if the relative
            decrease in chi squared is less than `tolerance`
            `tolerance_count` times in a row. The current default value
            is the CPL recommended default of %d.
        max_iterations : int
            If this number of iterations is reached without convergence,
            the algorithm diverges, by definition. The current default value
            is the CPL recommended default of %d
        Returns
        -------
        A lvmqResult NamedTuple with the following elements:

        best_fit : list or array of float
            the best fit parameters for the evaluate function.
            Derived from `starting_guess_params` if given.
        mse : float
            the mean squared error of the best fit
        red_chisq : float
            the reduced chi-squared of the best fit. None if `sigma_y` is not passed
        covariance : cpl.core.Matrix
            The formal covariance matrix of the best fi, On success the diagonal
            terms of the covariance matrix are guaranteed to be positive.
            However, terms that involve a constant parameter (as defined by the input
            array `evaluate_derivatives`) are always set to zero. None if `sigma_y`
            is not passed
        )pydoc";  // Will bneed to change formatting for relative_tolerance
                  // default (%.2f) if the default ever changes to be beyond
                  // this in CPL

  int size_s = std::snprintf(
      nullptr, 0, lvmq_pydoc_str.c_str(), CPL_FIT_LVMQ_TOLERANCE,
      CPL_FIT_LVMQ_COUNT,
      CPL_FIT_LVMQ_MAXITER);  // Get size required to store the output
  if (size_s <= 0) {
    throw std::runtime_error("Error during formatting.");
  }
  std::vector<char> lvmq_pydoc_buf(size_s + 1);  // +1 for null terminator
  std::snprintf(&lvmq_pydoc_buf[0], lvmq_pydoc_buf.size(),
                lvmq_pydoc_str.c_str(), CPL_FIT_LVMQ_TOLERANCE,
                CPL_FIT_LVMQ_COUNT, CPL_FIT_LVMQ_MAXITER);

  mfit.def(
          "lvmq",
          [](const cpl::core::Matrix& x_positions,
             // const std::optional<cpl::core::Matrix>& sigma_x,
             const cpl::core::Vector& y_positions,
             cpl::core::Vector starting_guess_params,
             std::function<double(std::vector<double>, std::vector<double>)>
                 evaluate,
             std::function<std::vector<double>(std::vector<double>,
                                               std::vector<double>)>
                 evaluate_derivatives,
             const std::optional<std::vector<bool>>& participating_parameters,
             const std::optional<cpl::core::Vector>& sigma_y, double rel_tol,
             int tol_count, int max_iterations) -> py::object {
            py::object NamedTuple =
                py::module::import("collections").attr("namedtuple");
            py::object lvmq_output = NamedTuple(
                "lvmqResult",
                py::cast(std::vector<std::string>(
                    {"best_fit", "mse", "red_chisq", "covariance"})));

            py::tuple res =
                py::cast(cpl::drs::fit_lvmq<
                         std::function<double(std::vector<double>,
                                              std::vector<double>)>,
                         std::function<std::vector<double>(
                             std::vector<double>, std::vector<double>)>>(
                    x_positions, y_positions, starting_guess_params,
                    participating_parameters, evaluate, evaluate_derivatives,
                    sigma_y, rel_tol, tol_count, max_iterations));
            return lvmq_output(*res);
          },
          py::arg("x"), py::arg("y"), py::arg("starting_guess_params"),
          py::arg("evaluate"), py::arg("evaluate_derivatives"),
          py::arg("participating_parameters").none(true) = py::none(),
          py::arg("sigma_y").none(true) = py::none(),
          py::arg("relative_tolerance") = CPL_FIT_LVMQ_TOLERANCE,
          py::arg("tolerance_count") = CPL_FIT_LVMQ_COUNT,
          py::arg("max_iterations") = CPL_FIT_LVMQ_MAXITER,
          lvmq_pydoc_buf.data())
      .def("imagelist_polynomial", &cpl::drs::fit_imagelist_polynomial,
           py::arg("x_pos"), py::arg("values"), py::arg("mindeg"),
           py::arg("maxdeg"), py::arg("is_symsamp"), py::arg("pixeltype"),
           py::arg("fiterror").none(true) = py::none(),
           py::arg("window").none(true) = py::none(),
           R"pydoc(
        Least-squares fit a polynomial to each pixel in a list of images

        Parameters
        ----------
        x_pos : cpl.core.Vector
            The vector of positions to fit
        values : cpl.core.ImageList
            The list of images with values to fit
        mindeg : int
            The smallest degree with a non-zero coefficient
        maxdeg : int
            The polynomial degree of the fit, at least mindeg
        is_symsamp : bool
            True iff the x_pos values are symmetric around their mean
        pixeltype : cpl.core.Type
            The pixel-type of the created image list
        fiterror : cpl.core.Image, optional
            Image to contain the error of the fit
        window : tuple(int,int,int,int), optional
            If given, the window defining the area of the images to use in the format (x1,y1, x2, y2)

        Returns
        -------
        The image list of the fitted polynomial coefficients

        Raises
        ------
        IllegalInputError if mindeg is negative or maxdeg is less than mindeg or if llx or lly are smaller
            than 1 or if urx or ury is smaller than llx and lly respectively.
        AccessOutOfRange error if x2 or y2 from window exceed the size of the images
        IncompatibleInputError if x_pos and values have different lengths, or if fiterror is given with a
            different size than that of values, or if the input images do not all have the same dimensions
            and pixel type.
        DataNotFoundError if x_pos contains less than nc values
        SingularMatrixError if x_pos contains less than nc distinct values.
        UnsupportedModeError if the chosen pixel type is not one of cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT,
            cpl.core.Type.INT.

        Notes
        -----
        For each pixel, a polynomial representing the relation value = P(x) is
        computed where:

            P(x) = x^{mindeg} * (a_0 + a_1 * x + ... + a_{nc-1} * x^{nc-1}),

        where mindeg >= 0 and maxdeg >= mindeg, and nc is the number of
        polynomial coefficients to determine, nc = 1 + (maxdeg - mindeg).

        The returned image list thus contains nc coefficient images,

            a_0, a_1, ..., a_{nc-1}.

        np is the number of sample points, i.e. the number of elements in x_pos
        and number of images in the input image list.

        If mindeg is nonzero then is_symsamp is ignored, otherwise
        is_symsamp may to be set to CPL_TRUE if and only if the values in x_pos are
        known a-priori to be symmetric around their mean, e.g. (1, 2, 4, 6, 10,
        14, 16, 18, 19), but not (1, 2, 4, 6, 10, 14, 16). Setting is_symsamp to
        True while mindeg is zero eliminates certain round-off errors.

        For higher order fitting the fitting problem known as "Runge's phenomenon"
        is minimized using the socalled "Chebyshev nodes" as sampling points.
        For Chebyshev nodes is_symsamp can be set to True.

        Even though it is not an error, it is hardly useful to use an image of pixel
        type integer for the fitting error. An image of pixel type float should on
        the other hand be sufficient for most fitting errors.

        The call requires the following number of FLOPs, where
        nz is the number of pixels in any one image in the imagelist:

            2 * nz * nc * (nc + np) + np * nc^2 + nc^3/3 + O(nc * (nc + np)).

        If mindeg is zero an additional nz * nc^2 FLOPs are required.

        If fiterror is given an additional 2 * nz * nc * np FLOPs are required.

        Bad pixels in the input is suported as follows:

            First all pixels are fitted ignoring any bad pixel maps in the input. If
            this succeeds then each fit, where bad pixel(s) are involved is redone.
            During this second pass all input pixels flagged as bad are ignored.

            For each pixel to be redone, the remaining good samples are passed to
            cpl_polynomial_fit(). The input is_symsamp is ignored in this second pass.
            The reduced number of samples may reduce the number of sampling points to
            equal the number of coefficients to fit. In this case the fit has another
            meaning (any non-zero residual is due to rounding errors, not a fitting
            error). If for a given fit bad pixels reduces the number of sampling points
            to less than the number of coefficients to fit, then as many coefficients are
            fit as there are sampling points. The higher order coefficients are set to
            zero and flagged as bad. If a given pixel has no good samples, then the
            resulting fit will consist of zeroes, all flagged as bad.
        )pydoc")
      .def(
          "image_gaussian",
          [](const cpl::core::ImageBase& input, size xpos, size ypos,
             size xsize, size ysize,
             std::optional<std::shared_ptr<cpl::core::ImageBase>> errors,
             // First guesses are all optional for each individual guess
             std::optional<std::vector<std::optional<double>>> guesses,
             std::optional<std::vector<bool>> frozen_params) -> py::object {
            py::object NamedTuple =
                py::module::import("collections").attr("namedtuple");
            py::object fit_image_gaussian_output = NamedTuple(
                "FitImageGassianResult",
                py::cast(std::vector<std::string>(
                    {"err_params", "rms", "red_chisq", "covariance", "major",
                     "minor", "angle", "phys_cov", "parameters"})));


            cpl_array* parameters;
            if (guesses.has_value()) {
              if (guesses.value().size() > 7) {
                throw cpl::core::IllegalInputError(
                    PYCPL_ERROR_LOCATION,
                    "Initial guess array has too many elements (more than 7)");
              }
              parameters = cpl_array_new(7, CPL_TYPE_DOUBLE);
              for (int i = 0; i < guesses.value().size(); i++) {
                if (guesses.value()[i].has_value()) {
                  cpl_array_set_double(
                      parameters, i,
                      guesses.value()[i]
                          .value());  // Copy data over that exists
                } else {
                  cpl_array_set_invalid(parameters,
                                        i);  // None value, set to invalid
                }
              }
              cpl_array_fill_window_invalid(
                  parameters, guesses.value().size(),
                  7 - guesses.value().size());  // Fill up to 7 elements

            } else {
              parameters = cpl_array_new(7, CPL_TYPE_DOUBLE);
              cpl_array_fill_window_invalid(parameters, 0, 7);
            }

            py::tuple res = py::cast(cpl::drs::fit_image_gaussian(
                input, xpos, ypos, xsize, ysize, errors, parameters,
                frozen_params));

            cpl_array_delete(
                parameters);  // Delete the parameters at the binding level
            return fit_image_gaussian_output(*res);
          },
          py::arg("input"), py::arg("xpos"), py::arg("ypos"), py::arg("xsize"),
          py::arg("ysize"), py::arg("errors").none(true) = py::none(),
          py::arg("guesses").none(true) = py::none(),
          py::arg("fit_params").none(true) = py::none(),
          // FIXME: Seems like the doxygen docs don't slot in well. Maybe modify
          //        so it appears fine as pydocs
          R"pydoc(
        Fit a 2D gaussian to image values.

        Parameters
        ----------
        input : cpl.core.Image
            Input image with data values to fit.
        xpos : int
            X position of center of fitting domain.
        ypos : int
            Y position of center of fitting domain.
        xsize : int
            X size of fitting domain. It must be at least 3 pixels.
        ysize : int
            Y size of fitting domain. It must be at least 3 pixels.
        errors : cpl.core.Image, optional
            Optional input image with statistical errors associated to data.
        guesses : list or array of 7 floats or None, optional
            7 first-guesses for the gaussian parameters in the format:
            [B, A, rho, mu_x, mu_y,sigma_x, sigma_y]

            If None is passed for a parameter it will be considered
            invalid and not be used as a first-guess for the parameter.

            These parameters are futher detailed in the notes.
        fit_params :list or array of 7 bool elements, optional
            Used to flag parameters for freezing. If an array element is set to False, the
            corresponding parameter will be frozen. Any other value (including an "invalid"
            array element) would indicate a free parameter. If a parameter is frozen, a
            first-guess value must be specified at the corresponding element of the parameters
            array. Default setting is all parameters being free.
        Returns
        -------
        A FitImageGassianResult NamedTuple with the following elements:

        err_params : list of 7 floats
            the statistical error associated to each fitted parameter. None if `errors` is not passed
        rms : float
            returned standard deviation of fit residuals.
        red_chisq : float
            returned reduced chi-squared of fit. None if `errors` is not passed
        covariance : cpl.core.Matrix
            The covariance matrix, None if `errors` is not passed
        major : float
            returned semi-major axis of ellipse at 1-sigma.
        minor : float
            returned semi-minor axis of ellipse at 1-sigma.
        angle : float
            returned angle between X axis and major axis of ellipse, counted counterclockwise (radians).
        phys_cov : cpl.core.Matrix
            3x3 covariance matrix for the derived physical parameters major, minor, and angle, will be
            returned. None if `errors` is not passed
        parameters : list of 7 floats
            Parameters of best fit .

        Notes
        -----
        This function fits a 2d gaussian to pixel values within a specified region by minimizing
        \(\chi^2\) using a Levenberg-Marquardt algorithm. The gaussian model adopted here is based on
        the well-known cartesian form

        \[ z = B + \frac{A}{2 \pi \sigma_x \sigma_y \sqrt{1-\rho^2}} \exp\left({-\frac{1}{2\left(1-\rho^2\right)} \left(\left(\frac{x - \mu_x}{\sigma_x}\right)^2 -2\rho\left(\frac{x - \mu_x}{\sigma_x}\right) \left(\frac{y - \mu_y}{\sigma_y}\right) + \left(\frac{y - \mu_y}{\sigma_y}\right)^2\right)}\right) \]

        where `B` is a background level and `A` the volume of the gaussian (they both can be
        negative!), making 7 parameters altogether. Conventionally the parameters are indexed from 0
        to 6 in the elements of the arrays parameters, err_params, fit_params, and of the 7x7
        covariance matrix:

        [B, A, rho, mu_x, mu_y,sigma_x, sigma_y]

        The semi-axes \(a, b\) and the orientation \(\theta\) of the ellipse at 1-sigma level are
        finally derived from the fitting parameters as:

        \begin{eqnarray*} \theta &=& \frac{1}{2} \arctan \left(2 \rho \frac{\sigma_x \sigma_y} {\sigma_x^2 - \sigma_y^2}\right) \\ a &=& \sigma_x \sigma_y \sqrt{2(1-\rho^2) \frac{\cos 2\theta} {\left(\sigma_x^2 + \sigma_y^2\right) \cos 2\theta + \sigma_y^2 - \sigma_x^2}} \\ b &=& \sigma_x \sigma_y \sqrt{2(1-\rho^2) \frac{\cos 2\theta} {\left(\sigma_x^2 + \sigma_y^2\right) \cos 2\theta - \sigma_y^2 + \sigma_x^2}} \end{eqnarray*}

        Note that \(\theta\) is counted counterclockwise starting from the positive direction of the
        \(x\) axis, ranging bewteen \(-\pi/2\) and \(+\pi/2\) radians.

        If the correlation \(\rho = 0\) and \(\sigma_x \geq \sigma_y\) (within uncertainties) the
        ellipse is either a circle or its major axis is aligned with the \(x\) axis, so it is
        conventionally set

        \begin{eqnarray*} \theta &=& 0 \\ a &=& \sigma_x \\ b &=& \sigma_y \end{eqnarray*}

        If the correlation \(\rho = 0\) and \(\sigma_x < \sigma_y\) (within uncertainties) the
        major axis of the ellipse is aligned with the \(y\) axis, so it is conventionally set

        \begin{eqnarray*} \theta &=& \frac{\pi}{2} \\ a &=& \sigma_y \\ b &=& \sigma_x \end{eqnarray*}

        If requested, the 3x3 covariance matrix G associated to the derived physical quantities
        is also computed, applying the usual

        \[ \mathrm{G} = \mathrm{J} \mathrm{C} \mathrm{J}^\mathrm{T} \]

        where J is the Jacobian of the transformation \( (B, A, \rho, \mu_x, \mu_y, \sigma_x, \sigma_y) \rightarrow (\theta, a, b) \)
        and C is the 7x7 matrix of the gaussian parameters.
        )pydoc");
}