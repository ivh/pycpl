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

#ifndef PYCPL_WLCALIB_HPP
#define PYCPL_WLCALIB_HPP

#include <functional>
#include <memory>
#include <optional>
#include <tuple>

#include <cpl_bivector.h>
#include <cpl_wlcalib.h>

#include "cplcore/bivector.hpp"
#include "cplcore/polynomial.hpp"
#include "cplcore/types.hpp"
#include "cplcore/vector.hpp"

namespace cpl
{
namespace drs
{

using size = cpl::core::size;

extern thread_local std::function<int(const cpl::core::Vector&,
                                      const cpl::core::Polynomial&)>
    filler_lambda;

int filler_trampoline(const cpl::core::Vector& toFill,
                      const cpl::core::Polynomial& disp);

// For find_best_1d to communicate from python to C++ layer
enum filler
{
  LINE,
  LOGLINE,
  LINE_FAST,
  LOGLINE_FAST
};

class SlitModel
{
 public:
  // Methods to create and use a SlitModel
  /**
   * @brief Create a new line model to be initialized
   * @param catalog setting on init of the catalog of lines to be used
   * by the spectrum filter
   * @param threshold setting on init of the (positive) threshold for
   * truncating the transfer function
   * @param wfwhm setting on init of the FWHM of th etransfer function
   * to be used by the spectrum filter
   * @param wslit setting on init of the slit width to be used by the
   * spectrum filter
   *
   * The model comprises these elements:
   * Slit Width
   * FWHM of transfer function
   * Truncation threshold of the transfer function
   * Catalog of lines (typically arc or sky)
   *
   * The units of the X-values of the lines is a length, it is assumed to be the
   * same as that of the Y-values of the dispersion relation (e.g. meter), the
   * units of slit width and the FWHM are assumed to be the same as the X-values
   * of the dispersion relation (e.g. pixel), while the units of the produced
   * spectrum will be that of the Y-values of the lines.
   *
   * @return new instance of SlitModel
   */
  SlitModel(std::shared_ptr<cpl::core::Bivector> catalog, double threshold,
            double wfwhm, int spectrum_size, double wslit);
  /**
   * @brief Free memory associated with a cpl_wlcalib_slitmodel object.
   */
  ~SlitModel();

  /**
   * @brief Set the catalog of lines to be used by the spectrum filler
   * @param catalog The catalog of lines (e.g. arc lines)
   */
  void set_catalog(std::shared_ptr<cpl::core::Bivector> catalog);
  std::shared_ptr<cpl::core::Bivector> get_catalog();

  /**
   * @brief Set the slit width to be used by the spectrum filler
   * @param value The (positive) width of the slit
   */
  void set_wslit(double value);

  /**
   * @brief Get the slit width to be used by the spectrum filler
   * @return value The (positive) width of the slit
   */
  double get_wslit();

  /**
   * @brief Set the FWHM of the transfer function to be used by the spectrum
   * filler
   * @param value The (positive) FWHM
   */
  void set_wfwhm(double value);
  double get_wfwhm();

  /**
   * @brief Set the vector output size to be used by the spectrum
   * filler
   * @param value size of the vector
   */
  void set_spectrum_size(int value);
  int get_spectrum_size();

  /**
   * @brief The (positive) threshold for truncating the transfer function
   * @param value The (non-negative) truncation threshold, 5 is a good value.
   *
   * The line profile is truncated at this distance [pixel] from its maximum:
   * \f$x_{max} = w/2 + k * \sigma,\f$ where
   * \f$w\f$ is the slit width and \f$\sigma = w_{FWHM}/(2\sqrt(2\log(2))),\f$
   * where \f$w_{FWHM}\f$ is the Full Width at Half Maximum (FWHM) of the
   * transfer function and \f$k\f$ is the user supplied value.
   */
  void set_threshold(double value);
  double get_threshold();

  /**
   * @brief Generate a 1D spectrum from a model and a dispersion relation
   * @param disp 1D-Dispersion relation, at least of degree 1
   * @return Vector to fill with spectrum
   *
   * The fill a vector with a spectrum, all parameters
   * of the model  must be initialised either using the constructor
   * or the setter methods. In which then this function can be called.
   *
   *
   * Each line profile is given by the convolution of the Dirac delta function
   * with a Gaussian with \f$\sigma = w_{FWHM}/(2\sqrt(2\log(2))),\f$ and a
   * top-hat with the slit width as width. This continuous line profile is then
   * integrated over each pixel, wherever the intensity is above the threshold
   * set by the given model. For a given line the value on a given pixel
   * requires the evaluation of two calls to @em erf().
   *
   * @throws InvalidTypeError If the input polynomial is not 1D
   * @throws IllegalInputError If the input polynomial is non-increasing over
   * the given input (pixel) range, or if a model parameter is non-physical
   * (e.g. non-positive slit width).
   * @throws DataNotFoundError If no catalog lines are available in the range of
   * the dispersion relation
   * @throws IncompatibleInputError If the wavelengths of two catalog lines are
   * found to be in non-increasing order.
   */
  cpl::core::Vector fill_line_spectrum(const cpl::core::Polynomial& disp);

  /**
   * @brief Generate a 1D spectrum from a model and a dispersion relation
   * @param disp 1D-Dispersion relation, at least of degree 1
   * @return Vector to fill with spectrum
   *
   * The approximation preserves the position of the maximum, the symmetry and
   * the flux of the line profile.
   *
   * The use of a given line in a spectrum requires the evaluation of four calls
   * to @em erf().
   *
   * The fast spectrum generation can be useful when the model spectrum includes
   * many catalog lines.
   */
  cpl::core::Vector fill_line_spectrum_fast(const cpl::core::Polynomial& disp);

  /**
   * @brief Generate a 1D spectrum from a model and a dispersion relation
   * @param disp 1D-Dispersion relation, at least of degree 1
   * @return Vector to fill with spectrum
   */
  cpl::core::Vector fill_logline_spectrum(const cpl::core::Polynomial& disp);


  /**
   * @brief Generate a 1D spectrum from a model and a dispersion relation
   * @param disp 1D-Dispersion relation, at least of degree 1
   * @return Vector to fill with spectrum
   */
  cpl::core::Vector
  fill_logline_spectrum_fast(const cpl::core::Polynomial& disp);


  /**
   * @brief   Find the best 1D dispersion polynomial in a given search space
   * @param   guess       1D-polynomial with the guess
   * @param   spectrum    The vector with the observed 1D-spectrum
   * @param   filler      The function used to make the spectrum
   * @param   wl_search   Search range around the anchor points, same unit as
   * guess
   * @param   nsamples    Number of samples around the anchor points
   * @param   hsize       Maximum (pixel) displacement of the polynomial guess
   * @see cpl_wlcalib_fill_line_spectrum() for the model and filler.
   *
   * Find the polynomial that maximizes the cross-correlation between an
   * observed 1D-spectrum and the model spectrum based on the polynomial
   * dispersion relation.
   *
   * Each element in the vector of wavelength search ranges is in the same unit
   * as the corresponding Y-value of the dispersion relation. Each value in the
   * vector is the width of a search window centered on the corresponding value
   * in the guess polynomial. The length D of the search vector thus determines
   * the dimensionality of the search space for the dispersion polynomial. If
   * for example the search vector consists of three elements, then the three
   * lowest order coefficients of the dispersion relation may be modified by the
   * search.
   *
   * For each candidate polynomial P(x), the polynomial P(x+u), -hsize <= u <=
   * hsize is also evaluated. The half-size hsize may be zero. When it is
   * non-zero, an additional 2 * hsize cross-correlations are performed for each
   * candidate polynomial, one for each possible shift. The maximizing
   * polynomial among those shifted polynomials is kept. A well-chosen half-size
   * can allow for the use of fewer number of samples around the anchor points,
   * leading to a reduction of polynomials to be evaluated.
   *
   * The complexity in terms of model spectra creation is O(N^D) and in terms of
   * cross-correlations O(hsize * N^D), where N is nsamples and D is the length
   * of wl_search.
   *
   * xcorrs must be NULL or have a size of (at least) N^D*(1 + 2 * hsize).
   *
   * @throws IllegalInputError If the size of wl_search is less than 2, or
   * nsamples is less than 1, hsize negative  or if wl_search contains a zero
   * search bound, or if xcorrs is non-NULL and too short.
   * @throws DataNotFoundError If no model spectra can be created using the
   * supplied model and filler
   * @return Tuple of the resulting polynomial, the maximum cross-correlation
   * and the correlation values
   */
  std::tuple<cpl::core::Polynomial, double, cpl::core::Vector> find_best_1d(
      const cpl::core::Vector& spectrum, const cpl::core::Vector& wl_search,
      size nsamples, size hsize,
      // Uncomment below and replace the line after when we get around to
      // properly allowing custom-written filler functions
      // std::function<void(const cpl::drs::SlitModel&, const
      // cpl::core::Vector&, const cpl::core::Polynomial&)> evaluatefiller
      filler filler_func_enum, std::optional<cpl::core::Polynomial> guess);


 private:
  cpl_wlcalib_slitmodel* m_interface;
  double m_wslit;
  double m_wfwhm;
  double m_threshold;
  int m_spectrum_size;
  cpl_bivector*
      m_catalog_ptr;  // Only ever return a duplicate for memory safety reasons
};

}  // namespace drs
}  // namespace cpl

#endif  // PYCPL_WLCALIB_HPP