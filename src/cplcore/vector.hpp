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
 * Wraps the cpl_vector struct as a C++ class cpl::core::Vector
 * Implementing all operations that a cpl_vector can do.
 *
 * This class is optional from the Python programmer's perspective, as they can
 * use a python list, of which there should be an automatic conversion to this
 * vector (see vector_conversion.hpp, TODO)
 */

#ifndef PYCPL_VECTOR_HPP
#define PYCPL_VECTOR_HPP

#include <filesystem>
#include <optional>
#include <tuple>
#include <vector>

#include <cpl_type.h>
#include <cpl_vector.h>

#include "cplcore/matrix.hpp"
#include "cplcore/propertylist.hpp"
#include "cplcore/types.hpp"

namespace cpl
{
namespace core
{

using size = cpl::core::size;

struct fit_gaussian_output;

/**
 * A @em cpl_vector is an object containing a list of values (as doubles)
 * and the (always positive) number of values. The functionalities provided
 * here are simple ones like sorting, statistics, or simple operations.
 * The @em BiVector object is composed of two of these vectors.
 * No special provisions are made to handle special values like NaN or Inf,
 * for data with such elements, a @em std::vector object may be preferable.
 */
class Vector
{
 public:
  Vector(std::vector<double> values);
  Vector(cpl_vector* to_steal) noexcept;
  Vector(Vector&& other) noexcept;
  Vector& operator=(const Vector& other);
  Vector& operator=(Vector&& other) noexcept;

  /**
   * @brief Create a new cpl_vector
   * @param n Number of element of the cpl_vector
   *
   * There is no default values assigned to the created object, they are
   * undefined until they are set.
   *
   * @return 1 newly allocated cpl_vector
   */
  Vector(size n);

  /**
   * @brief Create a cpl_vector from existing data
   * @param n Number of elements in the vector
   * @param data Pointer to array of n doubles
   *
   * @return 1 newly allocated cpl_vector or NULL on error
   */
  static Vector wrap(size n, double* data);

  /**
   * @brief Delete a cpl_vector
   *
   * If the vector @em v is @c NULL, nothing is done and no error is set.
   *
   */
  ~Vector();

  /**
   * @brief Delete a cpl_vector except the data array
   *
   * @return A pointer to the data array or NULL if the input is NULL.
   */
  void* unwrap();

  /**
   * @brief Read a list of values from an ASCII file and create a cpl_vector
   * @param filename Name of the input ASCII file
   *
   * Parse an input ASCII file values and create a cpl_vector from it
   * Lines beginning with a hash are ignored, blank lines also.
   * In valid lines the value is preceeded by an integer, which is ignored.
   *
   * In addition to normal files, FIFO (see man mknod) are also supported.
   *
   * @return 1 newly allocated cpl_vector
   * @throws FileIoError if the file cannot be read
   */
  static Vector read(const std::filesystem::path& filename);

  /**
   * @brief Dump a cpl_vector to a string
   *
   * Each element is preceded by its index number (starting with 1!) and
   * written on a single line.
   *
   * Comment lines start with the hash character.
   *
   * @return String with the vector contents.
   */
  std::string dump() const;

  /**
   * @brief Load a list of values from a FITS file
   * @param filename Name of the input file
   * @param xtnum Extension number in the file (0 for primary HDU)
   *
   * This function loads a vector from a FITS file (NAXIS=1), using cfitsio.
   *
   * 'xtnum' specifies from which extension the vector should be loaded.
   * This could be 0 for the main data section or any number between 1 and N,
   * where N is the number of extensions present in the file.
   *
   * @return 1 newly allocated cpl_vector
   * @throws IllegalInputError if the extension is not valid
   * @throws FileIoError if the file cannot be read
   */
  static Vector load(const std::filesystem::path& filename, size xtnum);

  /**
   * @brief Save a vector to a FITS file
   * @param filename Name of the file to write
   * @param type The type used to represent the data in the file
   * @param pl Property list for the output header or empty
   * @param mode The desired output options (combined with bitwise or)
   *
   * This function saves a vector to a FITS file (NAXIS=1), using cfitsio.
   * If a property list is provided, it is written to the named file before the
   * pixels are written.
   * If the image is not provided, the created file will only contain the
   * primary header. This can be useful to create multiple extension files.
   *
   * The type used in the file can be one of:
   * CPL_TYPE_UCHAR  (8 bit unsigned),
   * CPL_TYPE_SHORT  (16 bit signed),
   * CPL_TYPE_USHORT (16 bit unsigned),
   * CPL_TYPE_INT    (32 bit signed),
   * CPL_TYPE_FLOAT  (32 bit floating point), or
   * CPL_TYPE_DOUBLE (64 bit floating point).
   * Use CPL_TYPE_DOUBLE when no loss of information is required.
   *
   * Supported output modes are CPL_IO_CREATE (create a new file) and
   * CPL_IO_EXTEND  (append to an existing file)
   *
   * If you are in append mode, make sure that the file has writing
   * permissions. You may have problems if you create a file in your
   * application and append something to it with the umask set to 222. In
   * this case, the file created by your application would not be writable,
   * and the append would fail.
   *
   * @return Reference to this.
   * @throws IllegalInputError if the type or the mode is not supported
   * @throws FileNotCreatedError if the output file cannot be created
   * @throws FileIoError if the data cannot be written to the file
   */
  void save(const std::filesystem::path& filename, cpl_type type,
            std::optional<cpl::core::PropertyList> pl, unsigned mode) const;

  /**
   * @brief This function duplicates an existing vector and allocates memory
   *
   * @return a newly allocated cpl_vector
   */
  Vector(const Vector& v);

  /**
   * @brief This function copies contents of a vector into another vector
   * @param source source cpl_vector
   *
   */
  void copy(const Vector& source);

  /**
   * @brief Get the size of the vector
   *
   * @return The size or -1 in case of an error
   */
  size get_size() const;

  /**
   * @brief Get a pointer to the data part of the vector
   *
   * The returned pointer refers to already allocated data.
   *
   * @return Pointer to the data
   */
  double* data();

  /**
   * @brief Get a pointer to the data part of the vector
   *
   * @return Pointer to the data
   */
  const double* data() const;

  /**
   * @brief Get an element of the vector
   * @param idx the index of the element (0 to nelem-1)
   *
   * @return The element value
   * @throws IllegalInputError if idx is negative
   */
  double get(size idx) const;

  /**
   * @brief Resize the vector
   * @param newsize The new (positive) number of elements in the vector
   *
   */
  void set_size(size newsize);

  /**
   * @brief Set an element of the vector
   * @param idx the index of the element (0 to nelem-1)
   * @param value the value to set in the vector
   *
   * @throws IllegalInputError if idx is negative
   */
  void set(size idx, double value);

  /**
   * @brief Add a cpl_vector to another
   * @param v2 Second cpl_vector
   *
   * The second vector is added to the first one. The input first vector is
   * modified.
   *
   * The input vectors must have the same size.
   *
   */
  void add(const Vector& v2);

  /**
   * @brief Subtract a cpl_vector from another
   * @param v2 Second cpl_vector
   *
   */
  void subtract(const Vector& v2);

  /**
   * @brief Multiply two vectors component-wise
   * @param v2 Second cpl_vector
   *
   */
  void multiply(const Vector& v2);

  /**
   * @brief Divide two vectors element-wise
   * @param v2 Second cpl_vector
   *
   * If an element in v2 is zero v1 is not modified and
   * CPL_ERROR_DIVISION_BY_ZERO is returned.
   *
   * @throws IncompatibleInputError if v1 and v2 have different sizes
   */
  void divide(const Vector& v2);

  /**
   * @brief Compute the vector dot product.
   * @param v2 Another vector of the same size
   *
   * The same vector may be passed twice, in which case the square of its 2-norm
   * is computed.
   *
   * @return The (non-negative) product or negative on error.
   */
  double product(const Vector& v2) const;

  /**
   * @brief Perform a cyclic shift to the right of the elements of a vector
   * @param other Vector to read from, or NULL for in-place shift of self
   * @param shift The number of positions to cyclic right-shift
   *
   * A shift of +1 will move the last element to the first, a shift of -1 will
   * move the first element to the last, a zero-shift will perform a copy (or
   * do nothing in case of an in-place operation).
   *
   * A non-integer shift will perform the shift in the Fourier domain. Large
   * discontinuities in the vector to shift will thus lead to FFT artifacts
   * around each discontinuity.
   *
   * @throws IncompatibleInputError if self and other have different sizes
   * @throws UnsupportedModeError if the shift is non-integer and FFTW is
   */
  void cycle(double shift);

  /**
   * @brief Sort a cpl_vector
   * @param dir CPL_SORT_ASCENDING or CPL_SORT_DESCENDING
   *
   * The input cpl_vector is modified to sort its values in either ascending
   * (CPL_SORT_ASCENDING) or descending (CPL_SORT_DESCENDING) order.
   *
   * @throws IllegalInputError if dir is neither CPL_SORT_DESCENDING nor
   */
  void sort(cpl_sort_direction dir);

  /**
   * @brief Elementwise addition of a scalar to a vector
   * @param addend Number to add
   *
   * Add a number to each element of the cpl_vector.
   *
   */
  void add_scalar(double addend);

  /**
   * @brief Elementwise subtraction of a scalar from a vector
   * @param subtrahend Number to subtract
   *
   * Subtract a number from each element of the cpl_vector.
   *
   */
  void subtract_scalar(double subtrahend);

  /**
   * @brief Elementwise multiplication of a vector with a scalar
   * @param factor Number to multiply with
   *
   * Multiply each element of the cpl_vector with a number.
   *
   */
  void multiply_scalar(double factor);

  /**
   * @brief Elementwise division of a vector with a scalar
   * @param divisor Non-zero number to divide with
   *
   * Divide each element of the cpl_vector with a number.
   *
   */
  void divide_scalar(double divisor);

  /**
   * @brief Compute the element-wise logarithm.
   * @param base Logarithm base.
   *
   * The base and all the vector elements must be positive and the base must be
   * different from 1, or a cpl_error_code will be returned and the vector will
   * be left unmodified.
   *
   * @throws IllegalInputError if base is negative or zero or if one of the
   */
  void logarithm(double base);

  /**
   * @brief Compute the exponential of all vector elements.
   * @param base Exponential base.
   *
   * If the base is zero all vector elements must be positive and if the base is
   * negative all vector elements must be integer, otherwise a cpl_error_code is
   * returned and the vector is unmodified.
   *
   * @throws IllegalInputError base and v are not as requested
   */
  void exponential(double base);

  /**
   * @brief Compute the power of all vector elements.
   * @param exponent Constant exponent.
   *
   * If the exponent is negative all vector elements must be non-zero and if
   * the exponent is non-integer all vector elements must be non-negative,
   * otherwise a cpl_error_code is returned and the vector is unmodified.
   *
   * Following the behaviour of C99 pow() function, this function sets 0^0 = 1.
   *
   * @throws IllegalInputError if v and exponent are not as requested
   */
  void power(double exponent);

  /**
   * @brief Fill a cpl_vector
   * @param val Value used to fill the cpl_vector
   *
   * Input vector is modified
   *
   */
  void fill(double val);

  /**
   * @brief Compute the sqrt of a cpl_vector
   *
   * The sqrt of the data is computed. The input cpl_vector is modified
   *
   * If an element in v is negative v is not modified and
   * CPL_ERROR_ILLEGAL_INPUT is returned.
   *
   */
  void sqrt();

  /**
   * @brief In a @em sorted vector find the element closest to the given value
   * @param key Value to find
   *
   * Bisection (Binary search) is used to find the element.
   *
   * If two (neighboring) elements with different values both minimize
   * fabs(sorted[index] - key) the index of the larger element is returned.
   *
   * If the vector contains identical elements that minimize
   * fabs(sorted[index] - key) then it is undefined which element has its index
   * returned.
   *
   * @return The index that minimizes fabs(sorted[index] - key) or
   *     negative on error
   */
  size bisect(double key) const;

  /**
   * @brief Extract a sub_vector from a vector
   * @param istart Start index (from 0 to number of elements - 1)
   * @param istop Stop  index (from 0 to number of elements - 1)
   * @param istep Extract every step element
   *
   * FIXME: Currently istop must be greater than istart.
   * FIXME: Currently istep must equal 1.
   *
   * @return A newly allocated cpl_vector
   */
  Vector extract(size istart, size istop, size istep) const;

  /**
   * @brief Get the index of the minimum element of the cpl_vector
   *
   * @return The index (0 for first) of the minimum value or negative on error
   */
  size get_minpos() const;

  /**
   * @brief Get the index of the maximum element of the cpl_vector
   *
   * @return The index (0 for first) of the maximum value or negative on error
   */
  size get_maxpos() const;

  /**
   * @brief Get the minimum of the cpl_vector
   *
   * @return The minimum value of the vector or undefined on error
   */
  double get_min() const;

  /**
   * @brief Get the maximum of the cpl_vector
   *
   * @return the maximum value of the vector or undefined on error
   */
  double get_max() const;

  /**
   * @brief Get the sum of the elements of the cpl_vector
   *
   * @return the sum of the elements of the vector or undefined on error
   */
  double get_sum() const;

  /**
   * @brief Compute the mean value of vector elements.
   *
   * @return Mean value of vector elements or undefined on error.
   */
  double get_mean() const;

  /**
   * @brief Compute the median of the elements of a vector.
   *
   * @return Median value of the vector elements or undefined on error.
   */
  double get_median();

  /**
   * @brief Compute the median of the elements of a vector.
   *
   * For a finite population or sample, the median is the middle value of an odd
   * number of values (arranged in ascending order) or any value between the two
   * middle values of an even number of values. The criteria used for an even
   * number of values in the input array is to choose the mean between the two
   * middle values. Note that in this case, the median might not be a value
   * of the input array. Also, note that in the case of integer data types,
   * the result will be converted to an integer. Consider to transform your int
   * array to float if that is not the desired behavior.
   *
   * @return Median value of the vector elements or undefined on error.
   */
  double get_median_const() const;

  /**
   * @brief Compute the bias-corrected standard deviation of a vectors elements
   *
   * S(n-1) = sqrt((1/n-1) sum((xi-mean)^2) (i=1 -> n)
   *
   * The length of v must be at least 2.
   *
   * @return standard deviation of the elements or a negative number on error.
   */
  double get_stdev() const;

  /**
   * @brief Cross-correlation of two vectors
   * @param v1 1st vector to correlate
   * @param v2 2nd vector to correlate
   *
   * vxc must have an odd number of elements, 2*half_search+1, where half_search
   * is the half-size of the search domain.
   *
   * The length of v2 may not exceed that of v1.
   * If the difference in length between v1 and v2 is less than half_search
   * then this difference must be even (if the difference is odd resampling
   * of v2 may be useful).
   *
   * The cross-correlation is computed with shifts ranging from -half_search
   * to half_search.
   *
   * On succesful return element i (starting with 0) of vxc contains the cross-
   * correlation at offset i-half_search. On error vxc is unmodified.
   *
   * The cross-correlation is in fact the dot-product of two unit-vectors and
   * ranges therefore from -1 to 1.
   *
   * The cross-correlation is, in absence of rounding errors, commutative only
   * for equal-sized vectors, i.e. changing the order of v1 and v2 will move
   * element j in vxc to 2*half_search - j and thus change the return value from
   * i to 2*half_search - i.
   *
   * If, in absence of rounding errors, more than one shift would give the
   * maximum cross-correlation, rounding errors may cause any one of those
   * shifts to be returned. If rounding errors have no effect the index
   * corresponding to the shift with the smallest absolute value is returned
   * (with preference given to the smaller of two indices that correspond to the
   * same absolute shift).
   *
   * If v1 is longer than v2, the first element in v1 used for the resulting
   * cross-correlation is
   * max(0,shift + (cpl_vector_get_size(v1)-cpl_vector_get_size(v2))/2).
   *
   * Cross-correlation with half_search == 0 requires about 8n FLOPs, where
   * n = cpl_vector_get_size(v2).
   * Each increase of half_search by 1 requires about 4n FLOPs more, when all of
   * v2's elements can be cross-correlated, otherwise the extra cost is about
   * 4m, where m is the number of elements in v2 that can be cross-correlated,
   * n - half_search <= m < n.
   *
   * Example of 1D-wavelength calibration (error handling omitted for brevity):
   *
   * cpl_vector   * model = my_model(dispersion);
   * cpl_vector   * vxc   = cpl_vector_new(1+2*maxshift);
   * const cpl_size shift = cpl_vector_correlate(vxc, model, observed) -
   * maxshift; cpl_error_code error = cpl_polynomial_shift_1d(dispersion, 0,
   * (double)shift);
   *
   * cpl_msg_info(cpl_func, "Shifted dispersion relation by %" CPL_SIZE_FORMAT
   * " pixels, shift);
   *
   * @return Index of maximum cross-correlation, or negative on error
   * @throws IllegalInputError if v1 and v2 have different sizes or if vxc
   */
  size correlate(const Vector& v1, const Vector& v2);

  /**
   * @brief Apply a low-pass filter to a cpl_vector
   * @param filter_type Type of filter to use
   * @param hw Filter half-width
   *
   * This type of low-pass filtering consists in a convolution with a given
   * kernel. The chosen filter type determines the kind of kernel to apply for
   * convolution.
   * Supported kernels are CPL_LOWPASS_LINEAR and CPL_LOWPASS_GAUSSIAN.
   *
   * In the case of CPL_LOWPASS_GAUSSIAN, the gaussian sigma used is
   * 1/sqrt(2). As this function is not meant to be general and cover all
   * possible cases, this sigma is hardcoded and cannot be changed.
   *
   * The returned signal has exactly as many samples as the input signal.
   *
   * @return Pointer to newly allocated cpl_vector
   * @throws IllegalInputError if filter_type is not supported or if hw is
   */
  Vector filter_lowpass_create(cpl_lowpass filter_type, size hw) const;

  /**
   * @brief Apply a 1D median filter of given half-width to a cpl_vector
   * @param hw Filter half-width
   *
   * This function applies a median smoothing to a cpl_vector and returns a
   * newly allocated cpl_vector containing a median-smoothed version of the
   * input.
   *
   * The returned cpl_vector has exactly as many samples as the input one. The
   * outermost hw values are copies of the input, each of the others is set to
   * the median of its surrounding 1 + 2 * hw values.
   *
   * For historical reasons twice the half-width is allowed to equal the
   * vector length, although in this case the returned vector is simply a
   * duplicate of the input one.
   *
   * If different processing of the outer values is needed or if a more general
   * kernel is needed, then cpl_image_filter_mask() can be called instead with
   * CPL_FILTER_MEDIAN and the 1D-image input wrapped around self.
   *
   * @return Pointer to newly allocated cpl_vector
   * @throws IllegalInputError if hw is negative or bigger than half the vector
   */
  Vector filter_median_create(size hw) const;

  /**
   * @brief This function duplicates an existing vector and allocates memory.
   *
   *
   * @return a newly allocated cpl_vector or NULL in case of an error
   */
  Vector duplicate();
  /**
   * @brief Fill a vector with a kernel profile
   * @param type Type of kernel profile
   * @param radius Radius of the profile in pixels
   *
   * A number of predefined kernel profiles are available:
   * - CPL_KERNEL_DEFAULT: default kernel, currently CPL_KERNEL_TANH
   * - CPL_KERNEL_TANH: Hyperbolic tangent
   * - CPL_KERNEL_SINC: Sinus cardinal
   * - CPL_KERNEL_SINC2: Square sinus cardinal
   * - CPL_KERNEL_LANCZOS: Lanczos2 kernel
   * - CPL_KERNEL_HAMMING: Hamming kernel
   * - CPL_KERNEL_HANN: Hann kernel
   * - CPL_KERNEL_NEAREST: Nearest neighbor kernel (1 when dist < 0.5, else 0)
   *
   * @throws IllegalInputError if radius is non-positive, or in case of the
   */
  void fill_kernel_profile(cpl_kernel type, double radius);

  /**
   * @brief Apply a 1d gaussian fit
   * @param sigma_x Uncertainty (one sigma, gaussian errors assumed)
   *     assosiated with @em x. Taking into account the
   *     uncertainty of the independent variable is currently
   *     unsupported, and this parameter must therefore be set
   *     to NULL.
   * @param y Values to fit
   * @param sigma_y Uncertainty (one sigma, gaussian errors assumed)
   *     associated with @em y.
   *     If NULL, constant uncertainties are assumed.
   * @param fit_pars Specifies which parameters participate in the fit
   *     (any other parameters will be held constant). Possible
   *     values are CPL_FIT_CENTROID, CPL_FIT_STDEV,
   *     CPL_FIT_AREA, CPL_FIT_OFFSET and any bitwise
   *     combination of these. As a shorthand for including all
   *     four parameters in the fit, use CPL_FIT_ALL.
   *
   * This function fits to the input vectors a 1d gaussian function of the form
   *
   * f(x) = @em area / sqrt(2 pi @em sigma^2) *
   * exp( -(@em x - @em x0)^2/(2 @em sigma^2)) + @em offset
   *
   * (@em area > 0) by minimizing chi^2 using a Levenberg-Marquardt algorithm.
   *
   * The values to fit are read from the input vector @em x. Optionally, a
   * vector
   *
   * Optionally, the mean squared error, the reduced chi square and the
   * covariance matrix of the best fit are computed . Set corresponding
   * parameter to NULL to ignore.
   *
   * If the covariance matrix is requested and successfully computed, the
   * diagonal elements (the variances) are guaranteed to be positive.
   *
   * Occasionally, the Levenberg-Marquardt algorithm fails to converge to a set
   * of sensible parameters. In this case (and only in this case), a
   * CPL_ERROR_CONTINUE is set. To allow the caller to recover from this
   * particular error,
   * the parameters @em x0, @em sigma, @em area and @em offset will on output
   * contain estimates of the best fit parameters, specifically estimated as the
   * median position, the median of the absolute residuals multiplied by 1.4828,
   * the minimum flux value and the maximum flux difference multiplied by
   * sqrt(2 pi sigma^2), respectively. In this case, @em mse, @em red_chisq and
   *
   * A CPL_ERROR_SINGULAR_MATRIX occurs if the covariance matrix cannot be
   * computed. In that case all other output parameters are valid.
   *
   * Current limitations
   * - Taking into account the uncertainties of the independent variable
   * is not supported.
   *
   * @return Reference to this.
   * @throws InvalidTypeError if the specified @em fit_pars is not a bitwise
   * @throws UnsupportedModeError @em sigma_x is non-NULL.
   * @throws IncompatibleInputError if the sizes of any input vectors are
   * @throws IllegalInputError if any input noise values, @em sigma or @em area
   * @throws IllegalOutputError if memory allocation failed.
   * @throws ContinueError if the fitting algorithm failed.
   */
  static std::tuple<double, double, double, double, double, double,
                    cpl::core::Matrix>
  fit_gaussian(const Vector& x, const Vector& y, cpl_fit_mode fit_pars,
               std::optional<std::reference_wrapper<Vector>> sigma_y,
               std::optional<double> x0, std::optional<double> sigma,
               std::optional<double> area, std::optional<double> offset);

  /**
   * @brief Create Right Half of a symmetric smoothing kernel for LSS
   * @param slitw The slit width [pixel]
   * @param fwhm The spectral FWHM [pixel]
   *
   * @return Right Half of (symmetric) smoothing vector
   * Due to being deprecated in CPL, this is not bound
   */
  // static Vector new_lss_kernel(double slitw, double fwhm);

  /**
   * @brief Convolve a 1d-signal with a symmetric 1D-signal
   * @param conv_kernel Vector with symmetric convolution function
   *
   * Due to being deprecated in CPL, this is not bound
   */
  // Vector &convolve_symmetric(const Vector& conv_kernel);

  bool operator==(const Vector& other) const;
  bool operator!=(const Vector& other) const;

  const cpl_vector* ptr() const;
  cpl_vector* ptr();

  /**
   * @brief Relieves self Vector of ownership of the underlying cpl_vector*
   * pointer, if it is owned.
   *
   * This is a counterpart to Vector(cpl_vector *to_steal);
   *
   * @note Make sure to use cpl_vector_delete to delete the returned
   * cpl_vector*, or turn it back into a Vector with Vector(cpl_vector
   * *to_steal)
   */
  static cpl_vector* unwrap(Vector&& self);

 private:
  cpl_vector* m_interface;
};

}  // namespace core
}  // namespace cpl

#endif  // PYCPL_VECTOR_HPP