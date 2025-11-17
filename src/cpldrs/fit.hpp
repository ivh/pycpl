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


/**
 * Wraps the cpl drs fit functions as Python functions with type converison
 */

#ifndef PYCPL_FIT_HPP
#define PYCPL_FIT_HPP

#include <memory>
#include <optional>
#include <tuple>
#include <vector>

#include <cpl_array.h>
#include <cpl_type.h>

#include "cplcore/coords.hpp"
#include "cplcore/image.hpp"
#include "cplcore/imagelist.hpp"
#include "cplcore/matrix.hpp"
#include "cplcore/vector.hpp"

namespace cpl
{
namespace drs
{

using size = cpl::core::size;

/**
 *    @brief   Fit a function to a set of data
 *    @param   x_positions
 *         N x D matrix of the positions to fit.
 *         Each matrix row is a D-dimensional position.
 *    @param   sigma_x
 *         Uncertainty (one sigma, gaussian errors assumed)
 *         assosiated with @em x_positions. Taking into account the
 *         uncertainty of the independent variable is currently
 *         unsupported, and this parameter must therefore be set
 *         to NULL.
 *    @param   y_positions
 *         The N values to fit.
 *    @param   sigma_y
 *         Vector of size N containing the uncertainties of
 *         the y-values. If this parameter is NULL, constant
 *         uncertainties are assumed.
 *    @param   starting_guess_params
 *         Vector containing M fit parameters. Must contain
 *         a guess solution on input and contains the best
 *         fit parameters on output.
 *    @param   participating_parameters
 *         Array of size M defining which fit parameters participate
 *         in the fit (non-zero) and which fit parameters are held
 *         constant (zero). At least one element must be non-zero.
 *    @param   evaluate
 *         double evaluate(vector<double> x, vector<double>)
 *         Function that evaluates the fit function
 *         at the position specified by the first argument (an array of
 *         size D) using the fit parameters specified by the second
 *         argument (an array of size M). The result must be output
 *         using the third parameter, and the function must return zero
 *         iff the evaluation succeded.
 *
 *         If we update to C++20 these arguments can be substituted with
 * std::span's (which are more semantically similar to what CPL wants, and
 * avoids copying)
 *    @param   evaluate_derivatives
 *         vector<double> evaluate_derivatives(vector<double> x, vector<double>)
 *         Function that evaluates the first order partial
 *         derivatives of the fit function with respect to the fit
 *         parameters at the position specified by the first argument
 *         (an array of size D) using the parameters specified by the
 *         second argument (an array of size M). The result must
 *         be output using the third parameter (array of size M), and
 *         the function must return zero iff the evaluation succeded.
 *    @param relative_tolerance
 *         The algorithm converges by definition if the relative
 *         decrease in chi squared is less than @em tolerance
 *         @em tolerance_count times in a row. Recommended default:
 *         CPL_FIT_LVMQ_TOLERANCE
 *    @param tolerance_count
 *         The algorithm converges by definition if the relative
 *         decrease in chi squared is less than @em tolerance
 *         @em tolerance_count times in a row. Recommended default:
 *         CPL_FIT_LVMQ_COUNT
 *    @param max_iterations
 *         If this number of iterations is reached without convergence,
 *         the algorithm diverges, by definition. Recommended default:
 *         CPL_FIT_LVMQ_MAXITER
 *
 *
 *    @return  CPL_ERROR_NONE iff OK.
 *
 *    This function makes a minimum chi squared fit of the specified function
 *    to the specified data set using a Levenberg-Marquardt algorithm.
 *
 *     @throws IllegalInputError
 *         if an input matrix/vector is empty, if @em ia
 *         contains only zero values, if any of @em relative_tolerance,
 *         @em tolerance_count or max_iterations @em is non-positive, if N <= M
 *         and @em red_chisq is non-NULL, if any element of @em sigma_x or @em
 * sigma_y is non-positive, or if evaluation of the fit function or its
 * derivative failed.
 *     @throws IncompatibleInputError
 *         if the dimensions of the input
 *         vectors/matrices do not match, or if chi square or covariance
 * computation is requested and @em sigma_y is NULL.
 *     @throws IllegalOutputError
 *         if memory allocation failed.
 *     @throws ContinueError
 *         if the Levenberg-Marquardt algorithm failed to converge.
 *     @throws SingularMatrixError
 *         if the covariance matrix could not be computed.
 *
 */
template <typename F, typename DFDA>
std::tuple<cpl::core::Vector, double, double, cpl::core::Matrix>
fit_lvmq(const cpl::core::Matrix&
             x_positions,  // const std::optional<cpl::core::Matrix> &sigma_x,
         const cpl::core::Vector& y_positions,
         cpl::core::Vector starting_guess_params,
         const std::optional<std::vector<bool>>& participating_parameters,
         F evaluate, DFDA evaluate_derivatives,
         const std::optional<cpl::core::Vector>& sigma_y, double rel_tol,
         int tol_count, int max_iterations);

/**
 *  @brief  Least-squares fit a polynomial to each pixel in a list of images
 *  @param  x_pos      The vector of positions to fit
 *  @param  values     The list of images with values to fit
 *  @param  area       The area of the images to use
 *  @param  mindeg     The smallest degree with a non-zero coefficient
 *  @param  maxdeg     The polynomial degree of the fit, at least mindeg
 *  @param  is_symsamp True iff the x_pos values are symmetric around their mean
 *  @param  pixeltype  The (non-complex) pixel-type of the created image list
 *  @param  fiterror   When non-NULL, the error of the fit. Must be non-complex
 *  @note   values and x_pos must have the same number of elements.
 *  @note   The created imagelist must be deallocated with
 * cpl_imagelist_delete().
 *  @note   x_pos must have at least 1 + (maxdeg - mindeg) distinct values.
 *  @return The image list of the fitted polynomial coefficients or NULL on
 * error.
 *  @see cpl_polynomial_fit()
 *
 *  For each pixel, a polynomial representing the relation value = P(x) is
 *  computed where:
 *  P(x) = x^{mindeg} * (a_0 + a_1 * x + ... + a_{nc-1} * x^{nc-1}),
 *  where mindeg >= 0 and maxdeg >= mindeg, and nc is the number of
 *  polynomial coefficients to determine, nc = 1 + (maxdeg - mindeg).
 *
 *  The returned image list thus contains nc coefficient images,
 *  a_0, a_1, ..., a_{nc-1}.
 *
 *  np is the number of sample points, i.e. the number of elements in x_pos
 *  and number of images in the input image list.
 *
 *  If mindeg is nonzero then is_symsamp is ignored, otherwise
 *  is_symsamp may to be set to CPL_TRUE if and only if the values in x_pos are
 *  known a-priori to be symmetric around their mean, e.g. (1, 2, 4, 6, 10,
 *  14, 16, 18, 19), but not (1, 2, 4, 6, 10, 14, 16). Setting is_symsamp to
 *  CPL_TRUE while mindeg is zero eliminates certain round-off errors.
 *  For higher order fitting the fitting problem known as "Runge's phenomenon"
 *  is minimized using the socalled "Chebyshev nodes" as sampling points.
 *  For Chebyshev nodes is_symsamp can be set to CPL_TRUE.
 *
 *  Even though it is not an error, it is hardly useful to use an image of pixel
 *  type integer for the fitting error. An image of pixel type float should on
 *  the other hand be sufficient for most fitting errors.
 *
 *  The call requires the following number of FLOPs, where
 *  nz is the number of pixels in any one image in the imagelist:
 *
 *  2 * nz * nc * (nc + np) + np * nc^2 + nc^3/3 + O(nc * (nc + np)).
 *
 *  If mindeg is zero an additional nz * nc^2 FLOPs are required.
 *
 *  If fiterror is non-NULL an additional 2 * nz * nc * np FLOPs are required.
 *
 *  Bad pixels in the input is suported as follows:
 *  First all pixels are fitted ignoring any bad pixel maps in the input. If
 *  this succeeds then each fit, where bad pixel(s) are involved is redone.
 *  During this second pass all input pixels flagged as bad are ignored.
 *  For each pixel to be redone, the remaining good samples are passed to
 *  cpl_polynomial_fit(). The input is_symsamp is ignored in this second pass.
 *  The reduced number of samples may reduce the number of sampling points to
 *  equal the number of coefficients to fit. In this case the fit has another
 *  meaning (any non-zero residual is due to rounding errors, not a fitting
 *  error). If for a given fit bad pixels reduces the number of sampling points
 *  to less than the number of coefficients to fit, then as many coefficients
 * are fit as there are sampling points. The higher order coefficients are set
 * to zero and flagged as bad. If a given pixel has no good samples, then the
 *  resulting fit will consist of zeroes, all flagged as bad.
 *
 *  @throws NullInputError if an input const pointer is NULL
 *  @throws IllegalInputError if mindeg is negative or maxdeg is less than
 * mindeg or if llx or lly are smaller than 1 or if urx or ury is smaller than
 *          llx and lly respectively.
 *  @throws AccessOutOfRangeError if urx or ury exceed the size of values.
 *  @throws IncompatibleInputError if x_pos and values have different lengths,
 *          or if fiterror is non-NULL with a different size than that of
 * values, or if the input images do not all have the same dimensions and pixel
 * type.
 *  @throws DataNotFoundError if x_pos contains less than nc values.
 *  @throws SingularMatrixError if x_pos contains less than nc distinct values.
 *  @throws UnsupportedModeError if the chosen pixel type is not one of
 *          CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT, CPL_TYPE_INT.
 */
std::shared_ptr<cpl::core::ImageList> fit_imagelist_polynomial(
    const cpl::core::Vector& x_pos, const cpl::core::ImageList& values,
    size mindeg, size maxdeg, bool is_symsamp, cpl_type pixeltype,
    std::optional<std::shared_ptr<cpl::core::ImageBase>> fiterror,
    std::optional<cpl::core::Window> area);


using fit_gaussian_output =
    std::tuple<std::optional<std::vector<double>>, double,
               std::optional<double>, std::optional<cpl::core::Matrix>, double,
               double, double, std::optional<cpl::core::Matrix>,
               std::vector<double>>;

/**
 * @brief Fit a 2D gaussian to image values.
 *
 * @param input Input image with data values to fit.
 * @param xpos X position of center of fitting domain.
 * @param ypos Y position of center of fitting domain.
 * @param xsize X size of fitting domain. It must be at least 3 pixels.
 * @param ysize Y size of fitting domain. It must be at least 3 pixels.
 * @param errors Optional preallocated array for returning the statistical error
 * associated to each fitted parameter. This array must be of type
 * CPL_TYPE_DOUBLE, and it must have exactly 7 elements. This makes mandatory to
 * specify im_err. Note that the returned values are the square root of the
 * diagonal elements (variances) of the covariance matrix (see ahead).
 * @param parameters Preallocated array for returning the values of the best-fit
 * gaussian parameters (the parametrisation of the fitted gaussian is described
 * in the main documentation section, below). This array must be of type
 * CPL_TYPE_DOUBLE, and it must have exactly 7 elements. Generally, when passed
 * to this function, this array would not be initialised (all elements are
 * "invalid"). A first-guess for the gaussian parameters is not mandatory: but
 * it is possible to specify here a first-guess value for each parameter.
 * First-guess values can also be specified just for a subset of parameters.
 * @param frozen_params Optional array, used for flagging the parameters to
 * freeze.
 *
 * This function fits a 2d gaussian to pixel values within a specified
 * region by minimizing \f$\chi^2\f$ using a Levenberg-Marquardt algorithm.
 * The gaussian model adopted here is based on the well-known cartesian form
 *
 * \f[
 * z = B + \frac{A}{2 \pi \sigma_x \sigma_y \sqrt{1-\rho^2}}
 * \exp\left({-\frac{1}{2\left(1-\rho^2\right)}
 * \left(\left(\frac{x - \mu_x}{\sigma_x}\right)^2
 * -2\rho\left(\frac{x - \mu_x}{\sigma_x}\right)
 * \left(\frac{y - \mu_y}{\sigma_y}\right)
 * + \left(\frac{y - \mu_y}{\sigma_y}\right)^2\right)}\right)
 * \f]
 *
 * where \f$B\f$ is a background level and \f$A\f$ the volume of the
 * gaussian (they both can be negative!), making 7 parameters altogether.
 * Conventionally the parameters are indexed from 0 to 6 in the elements
 * of the arrays @em parameters, @em err_params, @em fit_params, and of
 * the 7x7 @em covariance matrix:
 *
 * \f{eqnarray*}{
 * \mathrm{parameters[0]} &=& B \\
 * \mathrm{parameters[1]} &=& A \\
 * \mathrm{parameters[2]} &=& \rho \\
 * \mathrm{parameters[3]} &=& \mu_x \\
 * \mathrm{parameters[4]} &=& \mu_y \\
 * \mathrm{parameters[5]} &=& \sigma_x \\
 * \mathrm{parameters[6]} &=& \sigma_y
 * \f}
 *
 * The semi-axes \f$a, b\f$ and the orientation \f$\theta\f$ of the
 * ellipse at 1-sigma level are finally derived from the fitting
 * parameters as:
 * \f{eqnarray*}{
 * \theta &=& \frac{1}{2} \arctan \left(2 \rho \frac{\sigma_x \sigma_y}
 *                        {\sigma_x^2 - \sigma_y^2}\right) \\
 * a &=& \sigma_x \sigma_y \sqrt{2(1-\rho^2) \frac{\cos 2\theta}
 *                         {\left(\sigma_x^2 + \sigma_y^2\right) \cos 2\theta
 *                         + \sigma_y^2 - \sigma_x^2}} \\
 * b &=& \sigma_x \sigma_y \sqrt{2(1-\rho^2) \frac{\cos 2\theta}
 *                         {\left(\sigma_x^2 + \sigma_y^2\right) \cos 2\theta
 *                         - \sigma_y^2 + \sigma_x^2}}
 * \f}
 *
 * Note that \f$\theta\f$ is counted counterclockwise starting from the
 * positive direction of the \f$x\f$ axis, ranging bewteen \f$-\pi/2\f$ and
 * \f$+\pi/2\f$ radians.
 *
 * If the correlation \f$\rho = 0\f$ and \f$\sigma_x \geq \sigma_y\f$
 * (within uncertainties) the ellipse is either a circle or its major axis
 * is aligned with the \f$x\f$ axis, so it is conventionally set
 *
 * \f{eqnarray*}{
 * \theta &=& 0 \\
 * a &=& \sigma_x \\
 * b &=& \sigma_y
 * \f}
 *
 * If the correlation \f$\rho = 0\f$ and \f$\sigma_x < \sigma_y\f$
 * (within uncertainties) the major axis of the ellipse
 * is aligned with the \f$y\f$ axis, so it is conventionally set
 *
 * \f{eqnarray*}{
 * \theta &=& \frac{\pi}{2} \\
 * a &=& \sigma_y \\
 * b &=& \sigma_x
 * \f}
 *
 * If requested, the 3x3 covariance matrix G associated to the
 * derived physical quantities is also computed, applying the usual
 * \f[
 *          \mathrm{G} = \mathrm{J} \mathrm{C} \mathrm{J}^\mathrm{T}
 * \f]
 * where J is the Jacobian of the transformation
 * \f$
 * (B, A, \rho, \mu_x, \mu_y, \sigma_x, \sigma_y) \rightarrow (\theta, a, b)
 * \f$
 * and C is the 7x7 matrix of the gaussian parameters.
 */
fit_gaussian_output fit_image_gaussian(
    const cpl::core::ImageBase& input, size xpos, size ypos, size xsize,
    size ysize, std::optional<std::shared_ptr<cpl::core::ImageBase>> errors,
    // First guesses are all optional for each individual guess
    cpl_array* parameters,  // While it would be nice to pass in the vector
                            // directly, not binding cpl_array prevents usage of
                            // invalid values this function relies on
    // Just convert at the bindings level so the user can pass in py::none for
    // invalid values,
    std::optional<std::vector<bool>> frozen_params);

}  // namespace drs
}  // namespace cpl

#endif  // PYCPL_FIT_HPP