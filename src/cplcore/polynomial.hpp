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
 * Wraps the cpl_polynomial struct as a C++ class cpl::core::Polynomial
 * Implementing all operations that a cpl_polynomial can do.
 *
 * This class is optional from the Python programmer's perspective, as they can
 * use a python list, of which there should be an automatic conversion to this
 * polynomial (see polynomial_conversion.hpp, TODO)
 */

#ifndef PYCPL_POLYNOMIAL_HPP
#define PYCPL_POLYNOMIAL_HPP

#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <cpl_polynomial.h>

#include "cplcore/matrix.hpp"
#include "cplcore/vector.hpp"

namespace cpl
{
namespace core
{

using size = cpl::core::size;

struct fit_gaussian_output;

/**
 * Polynomial object.
 *
 * The zero-polynomial (i.e. a zero-degree polynomial with a zero-valued
 * coefficient) is regardless of its dimension stored internally as a
 * NULL pointer.
 *
 * A non-zero uni-variate polynomial is stored as an array where the i'th
 * element is the real-valued coefficient of the variable to the i'th power
 * together with a counter of the number of its elements. An N-degree
 * polynomial thus has an array with N+1 elements. The first element is the
 * constant term, the last element is the most significant, it is non-zero.
 *
 * A bi-variate polynomial is also stored as an array with one element for each
 * of its second dimension degrees together with a counter of the number of its
 * elements.
 * Each of these elements is either a pointer to a uni-variate polynomial in
 * the lower dimension or a NULL-pointer if that uni-variate polynomial is the
 * zero-polynomial.
 *
 * Similarly, a higher-dimension polynomial is stored as an array with one
 * element for each of its own dimension degrees and a counter of the number
 * of its elements.
 * Each of these elements is either a pointer to a multi-variate polynomial in
 * the lower dimensions, or a NULL-pointer if that lower-dimension polynomial
 * is the zero-polynomial.
 *
 * A multi-variate polynomial in N-dimensions is thus stored as a tree of depth
 * N-1, where each leaf is a uni-variate polynomial, and where each non-leaf
 * node has as many child nodes as there are non-zero coefficients in that
 * dimension. All non-NULL leaf-nodes are at the same depth (of N-1).
 *
 * An unbalanced tree is allowed to avoid that higher dimension polynomials
 * store an excessive number of zero-valued coefficients.
 *
 * This storage scheme allows for the usage of the Horner rule in each
 * dimension.
 *
 * Storing the following polynomial:
 * @verbatim
 * p(x,y) = p0 + p1.x + p2.y + p3.x.y + p4.x^2 + p5.x.y^3
 * @endverbatim
 * would internally take:
 * @verbatim
 * dim = 2 (x and y),
 * a pointer to a 1D pointer-array of length 3 (Y-degrees, 0..2), where
 * the first pointer is to a struct w. nc = 3 (Y-degree = 0, X-degress 0..2) and
 * the double-array {p0, p1, p4},
 * the second pointer is to a struct w. nc = 2 (Y-degree = 1, X-degress 0..1)
 * and the double-array {p2, p3}, and the third pointer is NULL, indicating a
 * zero-coefficient to Y-degree = 2 and the fourth pointer is to a struct w. nc
 * = 1 (Y-degree = 3, X-degree 1) and the double-array {0, p5}.
 *
 * The root object (2 dimensions, fourth degree) would thus hold this tree:
 * Y:
 * 0 -> p0, p1, p4
 * 1 -> p2, p3
 * 2 -> NULL
 * 3 -> 0, p5
 *
 * Additional notes:
 * 1) Each node in the tree has no knowledge of any higher dimensions above it,
 * so the leaf node in a multi-dimensional tree is indistinguishable from a
 * univariate polynomial.
 *
 * 2) Each non-NULL non-leaf node in the tree has no knowledge of how many
 * dimensions are stored below it, i.e. it has no knowledge of its own
 * dimensionality.
 *
 * 3) Thus, when iterating through a polynomial, the iterator state needs to
 * include the current dimension, which is zero for the lowest and one
 * higher for each dimension above.
 *
 * 4) To allow for a low-complexity growth of the degree of any polynomial,
 * each node has two counters, one for the number of allocated elements,
 * and one for the actual count, which cannot exceed the size counter.
 * If the available space for coefficients is exhausted, double as much
 * space is allocated for the new coefficients.
 *
 * 5) To reduce the number of memory allocations, the allocation of memory for
 * a new node includes memory for the requested number of coefficients
 * (child nodes) at that node (and at least a minimum default number,
 * CPL_POLYNOMIAL_DEFAULT_COEFFS). If the number of coefficients (i.e. the
 * degree) later grows, a new memory allocation for the coefficients is
 * needed and the initial memory set aside for the cofficients remains
 * allocated but unused. The rationale behind this scheme is that the
 * coefficients in a polynomial typically are set either from the highest
 * towards to lowest or vice versa, so typically just one allocation is
 * needed per node.
 *
 * 6) To allow for a simple implementation both leaf- and non-leaf nodes use
 * the same object for storage, the array in this object uses a union of
 * a pointer (for higher dimensions) and a double (for the lowest dimension).
 *
 * 7) Reading from a given node object is only defined using the pointer member
 * for higher dimensions and the double member for the lowest dimension.
 *
 * 8) When during traversal of an N-dimensional tree it is necessary to know the
 * powers in the dimensions above a given coefficient, a integer array with
 * length N is updated at each dimension with its power.
 *
 * 9) A new polynomial can be created internally from an old one, with a change
 * of dimension supported. Changing the dimension means overwriting a union
 * member with a value of the other type. This works as long as the nodes of
 * the new tree keeps all its (non-NULL) leaf-nodes at the same level.
 *
 * 10) The object that holds the root of the tree thus knows its dimensionality.
 * To support O(1) access to its degree, the object also stores the degree
 * of the polynomial, this redundantly stored degree is kept up to date with
 * every change of the polynomial.
 *
 * 11) A valid node has a non-zero coefficient count. If a node's coefficient
 * count collapses to zero (indicating that the corresponding polynomial is
 * the zero-polynomial), the knowledge of this collapse has to be passed
 * up to the higher dimension, which will deallocate the node and set it to
 * NULL.
 *
 * 12) Certain operations on a polynomial may collapse a (lower dimension) node
 * to the zero-polynomial, such operations include: setting a coefficient
 * (to zero), adding or subtracting one polynomial from another. To ensure
 * the absence of zero-polynomials in a returned polynomial, the resulting
 * polynomial is passed to a pruning function that scans the whole tree and
 * removes nodes that are zero-polynomials.
 *
 * 13) For efficiency reasons a function may within its own scope set the
 * coefficient count of its leaf-nodes to zero without the corresponding
 * deallocation. While such a non-standard object may not be passed on to
 * other functions (nor returned), it may be re-populated (avoiding the
 * need for reallocation of arrays). Unless there is a priori knowledge
 * that this repopulation will convert all zero-polynomials to actual
 * polynomials, the object must be pruned (using the relevant function)
 * before being returned to the caller.
 *
 */
class Polynomial
{
 public:
  Polynomial(cpl_polynomial* to_steal) noexcept;
  Polynomial(Polynomial&& other) noexcept;
  Polynomial& operator=(const Polynomial& other);
  Polynomial& operator=(Polynomial&& other) noexcept;

  /**
   * @brief Create a new cpl_polynomial
   * @param dim The positive polynomial dimension (number of variables)
   *
   * A newly created polynomial has degree 0 and evaluates as 0.
   *
   * @return A newly allocated cpl_polynomial or NULL on error
   */
  Polynomial(size dim);

  /**
   * @brief Delete a cpl_polynomial
   *
   * The function deallocates the memory used by the polynomial @em self.
   *
   */
  ~Polynomial();

  /**
   * @brief Dump a polynomial contents into s string, fail on zero-polynomial(s)
   *
   * Each coefficient is preceded by its integer power(s) and
   * written on a single line.
   * If the polynomial has non-zero coefficients, only those are printed,
   * otherwise the (zero-valued) constant term is printed.
   *
   * For an N-dimensional polynomial each line thus consists of N power(s) and
   * their coefficient.
   *
   * @return String with the polynomial contents.
   */
  std::string dump() const;

  /**
   * @brief Duplicate a polynomial
   *
   * @return A newly allocated cpl_polynomial or NULL on error
   */
  Polynomial(const Polynomial& self);

  /**
   * @brief Copy the contents of one polynomial into another one
   * @param other Input polynomial
   *
   * self and other must point to different polynomials.
   *
   * If self already contains coefficients, then they are overwritten.
   *
   * This is the only function that can modify the dimension of a polynomial.
   *
   */
  void copy(const Polynomial& other);

  /**
   * @brief Get a coefficient of the polynomial
   * @param pows The non-negative power(s) of the variable(s)
   *
   * Requesting the value of a coefficient that has not been set is allowed,
   * in this case zero is returned.
   *
   * Example of usage:
   *
   * const cpl_size power       = 3;
   * double         coefficient = cpl_polynomial_get_coeff(poly1d, &power);
   *
   * @return The coefficient or undefined on error
   */
  double get_coeff(const std::vector<size>& pows) const;

  /**
   * @brief Set a coefficient of the polynomial
   * @param pows The non-negative power(s) of the variable(s)
   * @param value The coefficient
   *
   * The array pows is assumed to have the size of the polynomial dimension.
   *
   * If the coefficient is already there, it is overwritten, if not, a new
   * coefficient is added to the polynomial. This may cause the degree of the
   * polynomial to be increased, or if the new coefficient is zero, to decrease.
   *
   * Setting the coefficient of x1^4 * x3^2 in the 4-dimensional polynomial
   * poly4d to 12.3 would be performed by:
   *
   * const cpl_size pows[] = {4, 0, 2, 0};
   * cpl_error_code error  = cpl_polynomial_set_coeff(poly4d, pows, 12.3);
   *
   * Setting the coefficient of x^3 in the 1-dimensional polynomial poly1d to
   * 12.3 would be performed by:
   *
   * const cpl_size power = 3;
   * cpl_error_code error = cpl_polynomial_set_coeff(poly1d, &power, 12.3);
   *
   * For efficiency reasons, multiple coefficients are best inserted with the
   * of the highest powers first.
   *
   */
  void set_coeff(const std::vector<size>& pows, double value);

  /**
   * @brief Compare the coefficients of two polynomials
   * @param other The 2nd polynomial
   * @param tol The absolute (non-negative) tolerance
   *
   * The two polynomials are considered equal iff they have identical
   * dimensions and the absolute difference between their coefficients
   * does not exceed the given tolerance.
   *
   * This means that the following two polynomials per definition are
   * considered different:
   * P1(x1) = 3*x1 different from P2(x1,x2) = 3*x1.
   *
   * If all input parameters are valid and self and other point to the same
   * polynomial the function returns 0.
   *
   * If two polynomials have different dimensions, the return value is this
   * (positive) difference.
   *
   * If two 1D-polynomials differ, the return value is 1 plus the degree of the
   * lowest order differing coefficient.
   *
   * If for a higher dimension they differ, it is 1 plus the degree of a
   * differing coefficient.
   *
   * @return 0 when equal, positive when different, negative on error.
   */
  int compare(const Polynomial& other, double tol) const;

  /**
   * @brief The dimension of the polynomial
   *
   * @return The dimension or negative on error
   */
  size get_dimension() const;

  /**
   * @brief The degree of the polynomial
   *
   * The degree is the highest sum of exponents (with a non-zero coefficient).
   *
   * If there are no non-zero coefficients the degree is zero.
   *
   * @return The degree or negative on error
   */
  size get_degree() const;

  /**
   * @brief Evaluate the polynomial at the given point using Horners rule.
   * @param x Point of evaluation
   *
   * A polynomial with no non-zero coefficients evaluates as 0.
   *
   * With n coefficients the complexity is about 2n FLOPs.
   *
   * @return The computed value or undefined on error.
   * @throws IncompatibleInputError if the length of x differs from the
   * dimension
   */
  double eval(const cpl::core::Vector& x) const;

  /**
   * @brief Evaluate a bivariate (2D) polynomial using Horner's rule and compute
   *         the derivatives if needed.
   * @param x The x component of the evaluation point
   * @param y The y component of the evaluation point
   *
   * A polynomial with no non-zero coefficients evaluates as 0.
   *
   * The result is computed as p_0 + x * ( p_1 + x * ( p_2 + ... x * p_n ))
   *
   * @return The computed value and the gradients (x_gradient, y_gradient)
   * @throws InvalidTypeError if the polynomial is not 2D
   */
  std::tuple<double, cpl::core::Vector> eval_2d(double x, double y) const;

  /**
   * @brief Evaluate a 3D polynomial using Horner's rule and compute the
   *         the derivatives if needed.
   * @param x The x component of the evaluation point
   * @param y The y component of the evaluation point
   * @param z The z component of the evaluation point
   *
   * A polynomial with no non-zero coefficients evaluates as 0.
   *
   * The result is computed as p_0 + x * ( p_1 + x * ( p_2 + ... x * p_n ))
   *
   * @return The computed value and the gradients (x_gradient, y_gradient,
   * z_gradient)
   * @throws InvalidTypeError if the polynomial is not 3D
   */
  std::tuple<double, cpl::core::Vector>
  eval_3d(double x, double y, double z) const;


  /**
   * @brief Collapse one dimension of a multi-variate polynomial by composition
   * @param dim The dimension to collapse (zero for first dimension)
   * @param other The polynomial to replace dimension dim of self
   *
   * The dimension of the polynomial self must be one greater than that of the
   * other polynomial. Given these two polynomials the dimension dim of self is
   * collapsed by creating a new polynomial from
   * self(x0, x1, ..., x{dim-1},
   * other(x0, x1, ..., x{dim-1}, x{dim+1}, x{dim+2}, ..., x{n-1}),
   * x{dim+1}, x{dim+2}, ..., x{n-1}).
   *
   * The created polynomial thus has a dimension which is one less than the
   * polynomial self and which is equal to that of the other polynomial.
   * Collapsing one dimension of a 1D-polynomial is equivalent to evaluating it,
   * which can be done with cpl_polynomial_eval_1d().
   *
   * FIXME: The other polynomial must currently have a degree of zero, i.e. it
   * must be a constant.
   *
   * The collapse uses Horner's rule and requires for n coefficients requires
   * about 2n FLOPs.
   *
   * The returned object is a newly allocated cpl_polynomial that
   * must be deallocated by the caller using cpl_polynomial_delete().
   *
   * @return The collapsed polynomial or NULL on error
   * @throws InvalidTypeError if the polynomial is uni-variate.
   * @throws IllegalInputError if dim is negative.
   * @throws AccessOutOfRangeError if dim exceeds the dimension of self.
   * @throws IncompatibleInputError if other has the wrong dimension.
   */
  Polynomial extract(size dim, const Polynomial& other) const;

  /**
   * @brief Add two polynomials of the same dimension
   * @param first The 1st polynomial to add
   * @param second The 2nd polynomial to add
   *
   * Possible CPL error code set in this function:
   * - CPL_ERROR_NULL_INPUT if an input pointer is NULL
   * - CPL_ERROR_INCOMPATIBLE_INPUT if the polynomials do not have identical
   * dimensions
   *
   */
  void add(const Polynomial& second);

  /**
   * @brief Subtract two polynomials of the same dimension
   * @param first The polynomial to subtract from, or NULL
   * @param second The polynomial to subtract, or NULL
   *
   * Possible CPL error code set in this function:
   * - CPL_ERROR_NULL_INPUT if an input pointer is NULL
   * - CPL_ERROR_INCOMPATIBLE_INPUT if the polynomials do not have identical
   * dimensions
   *
   */
  void subtract(const Polynomial& second);

  /**
   * @brief Multiply two polynomials of the same dimension
   * @param first The polynomial to multiply
   * @param second The polynomial to multiply
   *
   * Possible CPL error code set in this function:
   * - CPL_ERROR_NULL_INPUT if an input pointer is NULL
   * - CPL_ERROR_INCOMPATIBLE_INPUT if the polynomials do not have identical
   * dimensions
   *
   */
  void multiply(const Polynomial& second);

  /**
   * @brief Multiply a polynomial with a scalar
   * @param other The polynomial to scale of same dimension, may equal self
   * @param factor The factor to multiply with
   *
   * Possible CPL error code set in this function:
   * - CPL_ERROR_NULL_INPUT if an input pointer is NULL
   * - CPL_ERROR_INCOMPATIBLE_INPUT if the two dimensions do not match
   *
   */
  void multiply_scalar(double factor);

  /**
   * @brief Compute a first order partial derivative
   * @param dim The dimension to differentiate (zero for first dimension)
   *
   * The dimension of the polynomial is preserved, even if the operation may
   * cause the polynomial to become independent of the dimension dim of the
   * variable.
   *
   * The call requires n FLOPs, where n is the number of (non-zero) polynomial
   * coefficients whose power in dimension dim is at least 1.
   *
   * @throws IllegalInputError if dim is negative.
   */
  void derivative(size dim);

  /**
   * @brief Fit a polynomial to a set of samples in a least squares sense
   * @param samppos cpl::core::Matrix of p sample positions, with d rows and p
   * columns
   * @param sampsym NULL, or d booleans, true iff the sampling is symmetric
   * @param fitvals cpl::core::Vector of the p values to fit
   * @param fitsigm Uncertainties of the sampled values, or NULL for all ones
   *                Non-NULL fitsigm is NOT SUPPORTED yet.
   * @param dimdeg True iff there is a fitting degree per dimension
   * @param mindeg Pointer to 1 or d minimum fitting degree(s), or NULL
   * @param maxdeg Pointer to 1 or d maximum fitting degree(s), at least mindeg
   *
   * Any pre-set non-zero coefficients in self are overwritten or reset by the
   * fit.
   *
   * For 1D-polynomials N = 1 + maxdeg - mindeg coefficients are fitted. A
   * non-zero mindeg ensures that the fitted polynomial has a fix-point at zero.
   *
   * For multi-variate polynomials the fit depends on dimdeg:
   *
   * If dimdeg is false, an n-degree coefficient is fitted iff
   * mindeg <= n <= maxdeg. For a 2D-polynomial this means that
   * N * (N + 1) / 2 coefficients are fitted.
   *
   * If dimdeg is true, nci = 1 + maxdeg[i] + mindeg[i] coefficients are fitted
   * for dimension i, i.e. for a 2D-polynomial N = nc1 * nc2 coefficients are
   * fitted.
   *
   * The number of distinct samples should exceed the number of coefficients to
   * fit. The number of distinct samples may also equal the number of
   * coefficients to fit, but in this case the fit has another meaning (any
   * non-zero residual is due to rounding errors, not a fitting error). It is an
   * error to try to fit more coefficients than there are distinct samples.
   *
   * If the relative uncertainties of the sampled values are known, they may be
   * passed via fitsigm. NULL means that all uncertainties equals one.
   *
   * sampsym is ignored if mindeg is nonzero, otherwise
   * the caller may use sampsym to indicate an a priori knowledge that the
   * sampling positions are symmetric. NULL indicates no knowledge of such
   * symmetry. sampsym[i] may be set to true iff the sampling is symmetric
   * around u_i, where u_i is the mean of the sampling positions in dimension i.
   *
   * In 1D this implies that the sampling points as pairs average u_0 (with an
   * odd number of samples one sample must equal u_0). E.g. both x = (1, 2, 4,
   * 6, 7) and x = (1, 6, 4, 2, 7) have sampling symmetry, while x = (1, 2, 4,
   * 6) does not.
   *
   * In 2D this implies that the sampling points are symmetric in the 2D-plane.
   * For the first dimension sampling symmetry means that the sampling is line-
   * symmetric around y = u_1, while for the second dimension, sampling symmetry
   * implies line-symmetry around x = u_2. Point symmetry around
   * (x,y) = (u_1, u_2) means that both sampsym[0] and sampsym[1] may be set to
   * true.
   *
   * Knowledge of symmetric sampling allows the fit to be both faster and
   * eliminates certain round-off errors.
   *
   * For higher order fitting the fitting problem known as "Runge's phenomenon"
   * is minimized using the socalled "Chebyshev nodes" as sampling points.
   * For Chebyshev nodes sampsym can be set to CPL_TRUE.
   *
   * Warning: An increase in the polynomial degree will normally reduce the
   * fitting error. However, due to rounding errors and the limited accuracy
   * of the solver of the normal equations, an increase in the polynomial degree
   * may at some point cause the fitting error to _increase_. In some cases this
   * happens with an increase of the polynomial degree from 8 to 9.
   *
   * The fit is done in the following steps:
   * 1) If mindeg is zero, the sampling positions are first transformed into
   * Xhat_i = X_i - mean(X_i), i=1, .., dimension.
   * 2) The Vandermonde matrix is formed from Xhat.
   * 3) The normal equations of the Vandermonde matrix is solved.
   * 4) If mindeg is zero, the resulting polynomial in Xhat is transformed
   * back to X.
   *
   * For a univariate (1D) fit the call requires 6MN + N^3/3 + 7/2N^2 + O(M)
   * FLOPs where M is the number of data points and where N is the number of
   * polynomial coefficients to fit, N = 1 + maxdeg - mindeg.
   *
   * For a bivariate fit the call requires MN^2 + N^3/3 + O(MN) FLOPs where M
   * is the number of data points and where N is the number of polynomial
   * coefficients to fit.
   *
   * Examples of usage:
   *
   * cpl_polynomial  * fit1d     = cpl_polynomial_new(1);
   * cpl_matrix      * samppos1d = my_sampling_points_1d(); // 1-row matrix
   * cpl_vector      * fitvals   = my_sampling_values();
   * const bool sampsym   = CPL_TRUE;
   * const cpl_size    maxdeg1d  = 4; // Fit 5 coefficients
   * cpl_error_code    error1d
   * = cpl_polynomial_fit(fit1d, samppos1d, &sampsym, fitvals, NULL,
   * CPL_FALSE, NULL, &maxdeg1d);
   *
   * cpl_polynomial  * fit2d      = cpl_polynomial_new(2);
   * cpl_matrix      * samppos2d  = my_sampling_points_2d(); // 2-row matrix
   * cpl_vector      * fitvals    = my_sampling_values();
   * const cpl_size    maxdeg2d[] = {2, 1}; // Fit 6 coefficients
   * cpl_error_code    error2d
   * = cpl_polynomial_fit(fit2d, samppos2d, NULL, fitvals, NULL, CPL_FALSE,
   * NULL, maxdeg2d);
   *
   * @throws IllegalInputError if a mindeg value is negative, or if a maxdeg
   * value
   * @throws DataNotFoundError if the number of columns in samppos is less than
   * @throws IncompatibleInputError if samppos, fitvals or fitsigm have
   * @throws Singularcpl::core::MatrixError if samppos contains too few distinct
   * values
   * @throws DivisionByZeroError if an element in fitsigm is zero
   */
  void fit(const cpl::core::Matrix& samppos, const cpl::core::Vector& fitsvals,
           bool dimdeg, std::vector<size> maxdeg,
           std::optional<std::vector<bool>> sampsym,
           std::optional<cpl::core::Vector> fitsigm,
           std::optional<std::vector<size>> mindeg);

  /**
   * @brief Compute the residual of this polynomial fit
   * @param fitvals cpl::core::Vector of the p fitted values
   * @param fitsigm Uncertainties of the sampled values or NULL for a uniform
   *     uncertainty
   * @param samppos cpl::core::Matrix of p sample positions, with d rows and p
   * columns
   *
   * It is allowed to pass the same vector as both fitvals and as self,
   * in which case fitvals is overwritten with the residuals.
   *
   * If the relative uncertainties of the sampled values are known, they may be
   * passed via fitsigm. NULL means that all uncertainties equal one. The
   * uncertainties are taken into account when computing the reduced
   * chi square value.
   *
   * If rechisq is non-NULL, the reduced chi square of the fit is computed as
   * well.
   *
   * The mean square error, which was computed directly by the former CPL
   * functions cpl_polynomial_fit_1d_create() and cpl_polynomial_fit_2d_create()
   * can be computed from the fitting residual like this:
   *
   * @return Vector twith the fitting residuals, and the reduced chi square of
   * the fit
   * @throws IncompatibleInputError if samppos, fitvals, fitsigm or fit
   * @throws DivisionByZeroError if an element in fitsigm is zero
   * @throws DataNotFoundError if the number of columns in samppos is less than
   */
  std::pair<cpl::core::Vector, double>
  fit_residual(const cpl::core::Vector& fitvals,
               const cpl::core::Matrix& samppos,
               const cpl::core::Vector* fitsigm) const;

  /**
   * @brief Evaluate a univariate (1D) polynomial using Horners rule.
   * @param x The point of evaluation
   * @param pd Iff pd is non-NULL, the derivative evaluated at x
   *
   * A polynomial with no non-zero coefficents evaluates to 0 with a
   * derivative that does likewise.
   *
   * The result is computed as p_0 + x * ( p_1 + x * ( p_2 + ... x * p_n ))
   * and requires 2n FLOPs where n+1 is the number of coefficients.
   *
   * The derivative is computed using a nested Horner rule.
   * This requires about 4n FLOPs.
   *
   * @return The result and the derivative evaluated at x
   */
  std::tuple<double, double> eval_1d(double x) const;

  /**
   * @brief Evaluate p(a) - p(b) using Horners rule.
   * @param a The evaluation point of the minuend
   * @param b The evaluation point of the subtrahend
   *
   * The call requires about 4n FLOPs where n is the number of coefficients in
   * self, which is the same as that required for two separate polynomial
   * evaluations. cpl_polynomial_eval_1d_diff() is however more accurate.
   *
   * ppa may be NULL. If it is not, *ppa is set to self(a), which is calculated
   * at no extra cost.
   *
   * The underlying algorithm is the same as that used in
   * cpl_polynomial_eval_1d() when the derivative is also requested.
   *
   * @return The difference and the result of p(a)
   */
  std::tuple<double, double> eval_1d_diff(double a, double b) const;

  /**
   * @brief Duplicate a polynomial
   * @return new duplicate polynomial
   */
  Polynomial duplicate() const;

  /**
   * @param out_size How many points to evaluate (size of returned vector)
   * @param x0 The first point of evaluation
   * @param d The increment between points of evaluation
   *
   * The evaluation points are x_i = x0 + i * d, i=0, 1, ..., n-1,
   * where n is the length of the vector.
   *
   * If d is zero it is preferable to simply use
   * cpl_vector_fill(v, cpl_polynomial_eval_1d(p, x0, NULL)).
   *
   * The call requires about 2nm FLOPs, where m+1 is the number of coefficients
   * in p.
   *
   * @return The newly created Polynomial
   */
  cpl::core::Vector fill_polynomial(size out_size, double x0, double d) const;

  /**
   * @brief A real solution to p(x) = 0 using Newton-Raphsons method
   * @param x0 First guess of the solution
   * @param px The solution, on error see below
   * @param mul The root multiplicity (or 1 if unknown)
   *
   * Even if a real solution exists, it may not be found if the first guess is
   * too far from the solution. But a solution is guaranteed to be found if all
   * roots of p are real. If the constant term is zero, the solution 0 will be
   * returned regardless of the first guess.
   *
   * No solution is found when the iterative process stops because:
   * 1) It can not proceed because p`(x) = 0 (CPL_ERROR_DIVISION_BY_ZERO).
   * 2) Only a finite number of iterations are allowed (CPL_ERROR_CONTINUE).
   * Both cases may be due to lack of a real solution or a bad first guess.
   * In these two cases *px is set to the value where the error occurred.
   * In case of other errors *px is unmodified.
   *
   * The accuracy and robustness deteriorates with increasing multiplicity
   * of the solution. This is also the case with numerical multiplicity,
   * i.e. when multiple solutions are located close together.
   *
   * mul is assumed to be the multiplicity of the solution. Knowledge of the
   * root multiplicity often improves the robustness and accuracy. If there
   * is no knowledge of the root multiplicity mul should be 1.
   * Setting mul to a too high value should be avoided.
   *
   * @throws InvalidTypeError if the polynomial has the wrong dimension
   * @throws IllegalInputError if the multiplicity is non-positive
   * @throws DivisionByZeroError if a division by zero occurs
   */
  double solve_1d(double x0, size mul) const;

  /**
   * @brief Modify p, p(x0, x1, ..., xi, ...) := (x0, x1, ..., xi+u, ...)
   * @param i The dimension to shift (0 for first)
   * @param u The shift
   *
   * Shifting the polynomial p(x) = x^n with u = 1 will generate the binomial
   * coefficients for n.
   *
   * Shifting the coordinate system to (x,y) for the 2D-polynomium poly2d:
   *
   * @throws IllegalInputError if i is negative
   */
  void shift_1d(size i, double u);

  bool operator==(const Polynomial& other) const;
  bool operator!=(const Polynomial& other) const;

  const cpl_polynomial* ptr() const;
  cpl_polynomial* ptr();

  /**
   * @brief Relieves self Polynomial of ownership of the underlying
   * cpl_polynomial* pointer, if it is owned.
   *
   * This is a counterpart to Polynomial(cpl_polynomial *to_steal);
   *
   * @note Make sure to use cpl_polynomial_delete to delete the returned
   * cpl_polynomial*, or turn it back into a Polynomial with
   * Polynomial(cpl_polynomial *to_steal)
   */
  static cpl_polynomial* unwrap(Polynomial&& self);

 private:
  cpl_polynomial* m_interface;
};

}  // namespace core
}  // namespace cpl

#endif  // PYCPL_POLYNOMIAL_HPP