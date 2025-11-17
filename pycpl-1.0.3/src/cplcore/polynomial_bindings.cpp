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

#include "cplcore/polynomial_bindings.hpp"

#include <algorithm>
#include <deque>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "cplcore/polynomial.hpp"
#include "cplcore/vector_bindings.hpp"
#include "dump_handler.hpp"
#include "path_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive

namespace py = pybind11;

using size = cpl::core::size;

/*
 * @brief Constructs a cpl::core::Polynomial from any numpy Polynomial or
 * arguments to create a numpy Polynomial
 */
static cpl::core::Polynomial py_polynomial_constructor(py::object numpy_poly);

/**
 * An iterator over polynomial (not the C++ STL kind of iterator)
 * that is bound to python as an iterator (has __next__, __iter__)
 *
 * I did not create a C++ iterator for the C++ cpl::core::Polynomial class
 * because:
 *  - It  would NOT keep around the cpl::core::Polynomial class (unless we force
 *    every cpl::core::Polynomial to be wrapped in a shared_ptr)
 *  - It's quite a lot of boilerplate.
 *  - It would require accessing the cpl::core::Polynomial::data
 *    instead of cpl::core::Polynomial::get() (since it doesn't return a
 * pointer, and STL iterators need pointers) However
 *  - This is not really usable to any C++ code.
 */
struct PolynomialIterator
{
  py::object polynomial;
  size index;
};

void
bind_polynomial(py::module& cplcore)
{
  // py::module m = cplcore.def_submodule("polynomial", "Polynomial herlper
  // functions and class");
  py::class_<cpl::core::Polynomial, std::shared_ptr<cpl::core::Polynomial>>
      poly_class(cplcore, "Polynomial");

  poly_class.doc() = R"pydoc(
        This module provides functions to handle uni- and multivariate polynomials.

        Comparing the two polynomials

            P1(x) = p0 + p1.x + p4.x^2

        and
        
            P2(x,y) = p0 + p1.x + p2.y + p3.x.y + p4.x^2 + p5.y^2

        P1(x) may evaluate to more accurate results than P2(x,0), especially around the roots.
        The base constructor creates a new polynomial with degree 0 and evaluates as 0.

        Parameters
        ----------
        dim : int
            The positive polynomial dimensions (number of variables)

        Notes
        -----
        Note that a polynomial like P3(z) = p0 + p1.z + p2.z^2 + p3.z^3, z=x^4 is preferable to p4(x) = p0 + p1.x^4 + p2.x^8 + p3.x^12.
        Polynomials are evaluated using Horner's method. For multivariate polynomials the evaluation is performed one dimension at a time,
        starting with the lowest dimension and proceeding upwards through the higher dimensions.
        )pydoc";

  poly_class
      // Since this Polynomial is itself iterable, this copy constructor isn't
      // necessary (it can be passed as argument to iterable constructor)
      // but it's more performant to do it in C++. This copy constructor
      // has to be .def'd before the iterable one, or the iterable one will
      // be used for cpl.core.Polynomials
      // TODO: Well iterables aren't suported for construction anyway so I
      //       guess we can just comment this out. Uncomment if they end up
      //       supported
      // .def(py::init<const cpl::core::Polynomial&>(), "Duplicates a
      // polynomial")
      .def(py::init<size>(), py::arg("dim"))
      .def_static("from_numpy", &py_polynomial_constructor, py::arg("data"),
                  R"pydoc(
        Construct a CPL polynomial from a numpy polynomial

        Parameters
        ----------
        data : np.polynomial.polynomial.Polynomial
            Input polynomial
        )pydoc")
      .def("__repr__",
           [](cpl::core::Polynomial& self) -> py::str {
             py::str representation = "<cpl.core.Polynomial, degree {}>";
             return representation.format(self.get_degree());
           })
      .def("__str__", &cpl::core::Polynomial::dump)
      .def(
          "copy",
          [](const cpl::core::Polynomial& self) -> cpl::core::Polynomial {
            auto duplicate = cpl::core::Polynomial(self.get_dimension());
            duplicate.copy(self);
            return duplicate;
          },
          R"pydoc(
        Return a copy of the Polynomial.

        Returns
        -------
        cpl.core.Polynomial
            A new Polynomial containing a copy of the coefficients of the original.
      )pydoc")
      .def(
          "dump",
          [](cpl::core::Polynomial& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(), self.dump(),
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("show") = true,
          R"pydoc(
            Dump the Polynomial contents to a file, stdout or a string.

            This function is intended just for debugging. It just prints the elements of a polynomial, ordered in rows and columns
            to the file path specified by `filename`. 
            If a `filename` is not specified, output goes to stdout (unless `show` is False).

            Parameters
            ----------
            filename : str, optional
                File to dump polynomial contents to
            mode : str, optional
                Mode to open the file with. Defaults to "w" (write, overwriting the contents of the file if it already exists),
                but can also be set to "a" (append, creating the file if it does not already exist or appending to the end of
                it if it does).
            show : bool, optional
                Send polynomial contents to stdout. Defaults to True.

            Returns
            -------
            str 
                Multiline string containing the dump of the polynomial contents.

          )pydoc")
      .def("compare", &cpl::core::Polynomial::compare, py::arg("other"),
           py::arg("tol"), R"pydoc(
        Compare the coefficients of two polynomials

        The two polynomials are considered equal if they have identical
        dimensions and the absolute difference between their coefficients
        does not exceed the given tolerance.

        This means that the following two polynomials per definition are
        considered different:

        .. code-block:: none

            P1(x1) = 3*x1 different from P2(x1,x2) = 3*x1.

        If all input parameters are valid and `self` and `other` point to the same
        polynomial the function returns 0.

        If two polynomials have different dimensions, the return value is this
        (positive) difference.

        If two 1D-polynomials differ, the return value is 1 plus the degree of the
        lowest order differing coefficient.

        If for a higher dimension they differ, it is 1 plus the degree of a
        differing coefficient.

        Parameters
        ----------
        other : cpl.core.Polynomial
            The 2nd polynomial
        tol : float
            The absolute (non-negative) tolerance

        Returns
        -------
        int
            0 when equal, positive when different, negative on error.

        Raises
        ------
        cpl.core.IllegalInputError
            If `tol` is negative
        )pydoc")
      .def_property_readonly("degree", &cpl::core::Polynomial::get_degree,
                             "The degree is the highest sum of exponents (with "
                             "a non-zero coefficient). If there are no "
                             "non-zero coefficients the degree is zero.")
      .def_property_readonly("dimension", &cpl::core::Polynomial::get_dimension,
                             "The dimension of the polynomial.")
      .def("get_coeff", &cpl::core::Polynomial::get_coeff, py::arg("pows"),
           R"pydoc(
        Get a coefficient of the polynomial. 

        Requesting the value of a coefficient that has not been set is allowed, in this case zero is returned.

        Parameters
        ----------
        pows : list of ints
            The non-negative power(s) of the variable(s)

        Raises
        ------
        cpl.core.IllegalInputError
            if pows contains negative values

        Notes
        -----
        For an N-dimensional polynomial the complexity is O(N) 

        Examples
        -------
        coeff = poly1d.get_coeff([3])
            Requesting the value of a coefficient that has not been set is allowed, in this case zero is returned.
        )pydoc")
      .def("set_coeff", &cpl::core::Polynomial::set_coeff, py::arg("pows"),
           py::arg("value"), R"pydoc(
        Set a coefficient of the polynomial
        
        `pows` is assumed to have the size of the polynomial dimension.

        If the coefficient is already there, it is overwritten, if not, a new coefficient is added to the polynomial. This may cause the degree of the polynomial to be increased, or if the new coefficient is zero, to decrease.

        Parameters
        ----------
        pows : list of ints
            The non-negative power(s) of the variable(s)
        value : float
            the coefficient

        Raises
        ------
        cpl.core.IllegalInputError
            if pows contains negative values

        Notes
        -----
        For an N-dimensional polynomial the complexity is O(N)
        )pydoc")
      // FIXME: Using a parameter sections for the following overloadend
      // functions causes Sphinx to issue a critical warning regarding an
      // unexpected section title. As a consequence manual formatting is used
      // here as a workaround. Once Sphinx is able to deal with this correctly
      // the 'automatic' formatting should be put back.
      .def("eval", &cpl::core::Polynomial::eval, py::arg("x"), R"pydoc(
        Evaluate the polynomial at the given point using Horner's rule.

        A polynomial with no non-zero coefficients evaluates as 0.

        The length of `x` must match the polynomial dimension.

        :Parameters:
            **x** (*cpl.core.Vector*) -- Points of evaluation

        :Returns: The computed value or undefined on error. 
        :Return type: float

        :Raises:
            **cpl.core.IncompatibleInputError** -- if the length of `x` differs from the dimension

        **Notes**
        
        With n coefficients the complexity is about 2n FLOPs.
      )pydoc")
      .def(
          "eval",
          [](cpl::core::Polynomial& self, py::iterable points) -> double {
            cpl::core::Vector points_vec = py_vec_constructor(points);
            return self.eval(points_vec);
          },
          py::arg("x"), R"pydoc(
        Evaluate the polynomial at the given point using Horner's rule.

        A polynomial with no non-zero coefficients evaluates as 0.

        The length of `x` must match the polynomial dimension.

        :Parameters:
            **x** (*iterable of float*) -- Points of evaluation

        :Returns: The computed value or undefined on error. 
        :Return type: float

        :Raises:
            **cpl.core.IncompatibleInputError** -- if the length of `x` differs from the dimension

        **Notes**
        
        With n coefficients the complexity is about 2n FLOPs.
        )pydoc")
      .def("extract", &cpl::core::Polynomial::extract, py::arg("dim"),
           py::arg("other"), R"pydoc(
        Collapse one dimension of a multi-variate polynomial by composition

        The dimension of the polynomial `self` must be one greater than that of the
        other polynomial. Given these two polynomials the dimension `dim` of `self` is
        collapsed by creating a new polynomial from::

            self(x0, x1, ..., x{dim-1}, other(x0, x1, ..., x{dim-1}, x{dim+1}, x{dim+2}, ..., x{n-1}), x{dim+1}, x{dim+2}, ..., x{n-1}).

        The created polynomial thus has a dimension which is one less than the
        polynomial self and which is equal to that of the other polynomial.
        Collapsing one dimension of a 1D-polynomial is equivalent to evaluating it,
        which can be done with eval_1d().

        `other` polynomial must currently have a degree of zero, i.e. it must
        be a constant.

        Parameters
        ----------
        dim : int
            The dimension to collapse (zero for first dimension)
        other : cpl.core.Polynomial
            The polynomial to replace dimension dim of `self`

        Returns
        -------
        cpl.core.Polynomial
            The collapsed polynomial or NULL on error

        Raises
        ------
        cpl.core.InvalidTypeError
            if the polynomial is uni-variate.
        cpl.core.IllegalInputError
            if dim is negative.
        cpl.core.AccessOutOfRangeError
            if dim exceeds the dimension of self.
        cpl.core.IncompatibleInputError
            if other has the wrong dimension.

        Notes
        -----
        `other` only allowed to be a constant is to be fixed by CPL

        The collapse uses Horner's rule and requires for n coefficients requires
        about 2n FLOPs.
        )pydoc")
      .def("derivative", &cpl::core::Polynomial::derivative, py::arg("dim"),
           R"pydoc(
        Compute a first order partial derivative

        The dimension of the polynomial is preserved, even if the operation may cause
        the polynomial to become independent of the dimension `dim` variable.

        `self` is modified to store the result

        Parameters
        ----------
        dim : int
            The dimension to differentiate (zero for first dimension)

        Raises
        ------
        cpl.core.IllegalInputError
            if `dim` is negative.
        cpl.core.AccessOutOfRangeError
            if `dim` exceeds the dimension of `self`.

        Notes
        -----
        The call requires n FLOPs, where n is the number of (non-zero) polynomial
        coefficients whose power in dimension dim is at least 1.
        )pydoc")
      .def("fill_polynomial", &cpl::core::Polynomial::fill_polynomial,
           py::arg("p"), py::arg("x0"), py::arg("d"),
           R"pydoc(
        Evaluate a 1D-polynomial on equidistant points using Horner's rule

        The evaluation points are x_i = x0 + i * d, i=0, 1, ..., n-1,
        where n is the length of the vector.

        Parameters
        ----------
        out_size : int
            How many points to evaluate (size of returned vector)
        x0 : float
            The first point of evaluation
        d : float
            The increment between points of evaluation

        Returns
        -------
        cpl.core.Vector
            The evaluated points

        Raises
        ------
        cpl.core.InvalidTypeError
            If the polynomial `self` is not 1D

        Notes
        -----
        The call requires about 2nm FLOPs, where m+1 is the number of coefficients in
        p.
        )pydoc")
      .def("solve_1d", &cpl::core::Polynomial::solve_1d, py::arg("x0"),
           py::arg("mul"), R"pydoc(
        A real solution to p(x) = 0 using Newton-Raphsons method

        Even if a real solution exists, it may not be found if the first guess is
        too far from the solution. But a solution is guaranteed to be found if all
        roots of p are real (except in the case where the derivative at the first
        guess happens to be zero, see below - for an n-degree polynomial there are up
        to n-1 such guesses). If the constant term is zero, the solution 0 will be
        returned regardless of the first guess.

        Parameters
        ----------
        x0 : float
            First guess of the solution
        mul : int
            The root multiplicity (or 1 if unknown)

        Returns
        -------
        float
            The solution

        Raises
        ------
        cpl.core.InvalidTypeError
            if the polynomial has the wrong dimension
        cpl.core.IllegalInputError
            if the multiplicity is non-positive
        cpl.core.ContinueError
            If the algorithm does not converge
        cpl.core.DivisionByZeroError
            if a division by zero occurs

        Notes
        ------
        No solution is found when the iterative process stops because:
        1) It can not proceed because p`(x) = 0 (cpl.core.DivisionByZeroError).
        2) Only a finite number of iterations are allowed (cpl.core.ContinueError).
        Either can happen due to an an actual lack of a real solution or due to an
        insufficiently good first guess.

        The accuracy and robustness deteriorates with increasing multiplicity
        of the solution. This is also the case with numerical multiplicity,
        i.e. when multiple solutions are located close together.

        `mul` is assumed to be the multiplicity of the solution. Knowledge of the
        root multiplicity often improves the robustness and accuracy. If there
        is no knowledge of the root multiplicity mul should be 1.
        Setting mul to a too high value should be avoided.
        )pydoc")
      .def("eval_1d", &cpl::core::Polynomial::eval_1d, py::arg("x"), R"pydoc(
        Evaluate a univariate (1D) polynomial using Horner's rule.

        A polynomial with no non-zero coefficents evaluates to 0 with a derivative that does likewise.

        Parameters
        ----------
        x : float
            Point of evaluation

        Returns
        -------
        tuple(float, float)
            in the format (result, pd), where pd is the derivative evaluated at x

        Raises
        ------
        cpl.core.InvalidTypeError
            if the polynomial is not (1D) univariate

        Notes
        -----
        The result is computed as :math:`p_0 + x * ( p_1 + x * ( p_2 + ... x * p_n ))`
        and requires 2n FLOPs where n+1 is the number of coefficients.

        The derivative is computed using a nested Horner rule.
        )pydoc")
      .def("eval_1d_diff", &cpl::core::Polynomial::eval_1d_diff, py::arg("a"),
           py::arg("b"), R"pydoc(
        Evaluate p(a) - p(b) using Horner's rule.

        Parameters
        ----------
        a : float
          The evaluation point of the minuend
        b : float
          The evaluation point of the subtrahend

        Returns
        -------
        tuple(float, float)
            in the format (result, ppa), where ppa is the result of p(a)

        Raises
        ------
        cpl.core.InvalidTypeError
            if the polynomial has the wrong dimension

        Notes
        -----
        The call requires about 4n FLOPs where n is the number of coefficients in
        self, which is the same as that required for two separate polynomial
        evaluations. cpl_polynomial_eval_1d_diff() is however more accurate.

        The underlying algorithm is the same as that used in eval_1d()
        when the derivative is also requested.
        )pydoc")
      .def("eval_2d", &cpl::core::Polynomial::eval_2d, py::arg("x"),
           py::arg("y"), R"pydoc(
        Evaluate a bivariate (2D) polynomial using Horner's rule and compute the derivatives.

        A polynomial with no non-zero coefficents evaluates to 0 with a
        derivative that does likewise.

        Parameters
        ----------
        x : float
          x component of the evaluation point
        y : float
          y component of the evaluation point

        Returns
        -------
        tuple(float, tuple(float, float))
            in the format (result, gradient), where gradient is a tuple of 2 floats containing
            the X and Y components of the gradient vector at the evaluated point

        Raises
        ------
        cpl.core.InvalidTypeError
            if the polynomial is not 2D

        Notes
        -----
        The result is computed as :math:`p_0 + x * ( p_1 + x * ( p_2 + ... x * p_n ))`

        The derivative is computed using a nested Horner rule.
        )pydoc")
      .def("eval_3d", &cpl::core::Polynomial::eval_3d, py::arg("x"),
           py::arg("y"), py::arg("z"), R"pydoc(
        Evaluate a trivariate (3D) polynomial using Horner's rule and compute
        the derivatives.

        A polynomial with no non-zero coefficents evaluates to 0 with a
        derivative that does likewise.

        Parameters
        ----------
        x : float
          x component of the evaluation point
        y : float
          y component of the evaluation point
        z : float
          z component of the evaluation point

        Returns
        -------
        tuple(float, tuple(float, float, float))
            in the format (result, gradient), where gradient is a tuple of 3 floats containing
            the X, Y and Z components of the gradient vector at the evaluated point

        Raises
        ------
        cpl.core.InvalidTypeError
            if the polynomial is not 3D

        Notes
        -----
        The result is computed as :math:`p_0 + x * ( p_1 + x * ( p_2 + ... x  * p_n ))`

        If the derivative is requested it is computed using a nested Horner rule.
        )pydoc")
      .def("fit", &cpl::core::Polynomial::fit, py::arg("samppos"),
           py::arg("fitvals"), py::arg("dimdeg"), py::arg("maxdeg"),
           py::arg("sampsym").none(true) = py::none(),
           py::arg("fitsigm").none(true) = py::none(),
           py::arg("mindeg").none(true) = py::none(), R"pydoc(
        Fit a polynomial to a set of samples in a least squares sense

        Any pre-set non-zero coefficients in self are overwritten or reset by the fit.

        For 1D-polynomials N = 1 + maxdeg - mindeg coefficients are fitted. A non-zero
        mindeg ensures that the fitted polynomial has a fix-point at zero.

        The number of distinct samples should exceed the number of coefficients to
        fit. The number of distinct samples may also equal the number of coefficients
        to fit, but in this case the fit has another meaning (any non-zero residual
        is due to rounding errors, not a fitting error).
        It is an error to try to fit more coefficients than there are distinct
        samples.

        In 1D the sampling points as pairs average u_0 (with an odd
        number of samples one sample must equal u_0).

        In 2D the sampling points are symmetric in the 2D-plane.
        For the first dimension sampling symmetry means that the sampling is line-
        symmetric around y = u_1, while for the second dimension, sampling symmetry
        implies line-symmetry around x = u_2. Point symmetry around
        (x,y) = (u_1, u_2) means that both sampsym[0] and sampsym[1] may be set to
        true. For Chebyshev nodes sampsym can be set to True.

        Parameters
        ----------
        samppos : cpl.core.Matrix
            Matrix of p sample positions, with d rows and p columns
        fitvals : cpl.core.Vector
            Vector of the p values to fit
        dimdeg : cpl_boolean
            True iff there is a fitting degree per dimension. 
            If dimdeg is false, an n-degree coefficient is fitted iff
            mindeg <= n <= maxdeg.
            If dimdeg is true, nci = 1 + maxdeg[i] - mindeg[i] 
            coefficients are fitted for dimension i
        maxdeg : list of ints
            Pointer to 1 or d maximum fitting degree(s), at least mindeg
        sampsym : list of bools, optional
            d booleans, true iff the sampling is symmetric.
            sampsym is ignored if mindeg is nonzero, otherwise the caller 
            may use sampsym to indicate an a priori knowledge that the sampling positions are symmetric
        fitsigm : cpl.core.Vector, optional
            Uncertainties of the sampled values, or None for all ones.
            If relative uncertainties of the sampled values are known, they may be
            passed via fitsigm. Not passing means that all uncertaintiesequals one. 
        mindeg : cpl.core.Size, optional
            Pointer to 1 or d minimum fitting degree(s)

        Raises
        ------
        cpl.core.IllegalInputError
            if a mindeg value is negative, or if a maxdeg value is less than the corresponding value
        cpl.core.DataNotFoundError
            if the number of columns in samppos is less than the number of coefficients to be determined
        cpl.core.IncompatibleInputError
            if samppos, fitvals or fitsigm have incompatible sizes
        cpl.core.SingularMatrixError
            if samppos contains too few distinct values
        cpl.core.DivisionByZeroError
            if an element in fitsigm is zero
        cpl.core.UnsupportedModeError
            if the polynomial dimension exceeds two

        Examples
        ------
        fit1d = cpl.core.Polynomial(1)
        samppos1d = my_sampling_points_1d() # 1-row matrix
        fitvals   = my_sampling_values()
        sampsym = [True]
        maxdeg1d = 4 # Fit 5 coefficients
        fit1d.fit(sappos1d,fitsvals, False, [maxdeg1d], sampsym=sampsym)

        fit2d = cpl.core.Polynomial(2)
        samppos2d = my_sampling_points_2d() # 2-row matrix
        fitvals   = my_sampling_values()
        maxdeg2d = [2, 1] # Fit 6 coefficients
        fit2d.fit(sappos2d,fitsvals, False, [maxdeg2d])

        fit3d = cpl.core.Polynomial(3)
        samppos3d = my_sampling_points_3d() # 2-row matrix
        fitvals   = my_sampling_values()
        maxdeg3d = [2, 1, 2] # Fit 6 coefficients
        fit3d.fit(sappos3d,fitsvals, False, [maxdeg3d])

        Notes
        -----
        Currently only 1D (uni-variate), 2D (bi-variate) and 3D (tri-variate)
        polynomials are supported. For all but uni-variate polynomials mindeg must be zero.
        
        For a univariate (1D) fit the call requires 6MN + N^3/3 + 7/2N^2 + O(M) FLOPs
        where M is the number of data points and where N is the number of polynomial
        coefficients to fit, N = 1 + maxdeg - mindeg.

        For a bivariate fit the call requires MN^2 + N^3/3 + O(MN) FLOPs where M
        is the number of data points and where N is the number of polynomial
        coefficients to fit.

        The fit is done in the following steps:
        1) If fitsigm is not None. The factors are applied to the values.
        2) If mindeg is zero, the sampling positions are first transformed into
        Xhat_i = X_i - mean(X_i), i=1, .., dimension.
        3) The Vandermonde matrix is formed from Xhat. If fitsigm is not None,
        the weights are also taken into account.
        4) The normal equations of the Vandermonde matrix is solved.
        5) If mindeg is zero, the resulting polynomial in Xhat is transformed
        back to X.
        Warning: An increase in the polynomial degree will normally reduce the
        fitting error. However, due to rounding errors and the limited accuracy
        of the solver of the normal equations, an increase in the polynomial degree
        may at some point cause the fitting error to _increase_. In some cases this
        happens with an increase of the polynomial degree from 8 to 9.
        )pydoc")

      .def("fit_residual", &cpl::core::Polynomial::fit_residual,
  // FIXME: Below would handle iterative types, but causes segfaults
  // if given an iterable type not vector. Since its not standard to
  // allow iterables in place of vectors, going to block this off to
  // just vectors
#if 0
          [](cpl::core::Polynomial& self, py::object fitvals_py,
             py::object samppos_py,
             py::object fitsigm_py) -> std::pair<cpl::core::Vector, double> {
            // TODO: when as_cpl_matrix is implemented, accept it
            cpl::core::Matrix* samppos;
            try {
              samppos = py::cast<cpl::core::Matrix*>(samppos_py);
            }
            catch (const py::cast_error& /* unused */) {
              // As a wokraround for missing as_cpl_matrix,
              // we use the python constructor
              //
              // Not sure where this would ever be deallocated.
              py::object samppos_py_converted =
                  py::module::import("cpl.core").attr("Matrix")(samppos_py);
              samppos = py::cast<cpl::core::Matrix*>(samppos_py_converted);
            }
            if (samppos == nullptr) {
              throw py::type_error(
                  "Expected matrix or 2D-List of doubles, not None");
            }

            auto fitvals = as_cpl_vec(fitvals_py);
            if (!fitvals.second.has_value()) {
               throw py::type_error(
                   "Expected list of doubles (fitvals) or CPL vector, not None");
            }
            // fitsigm is allowed to be null
            auto fitsigm_opt = as_cpl_vec(fitsigm_py);
            const cpl::core::Vector* fitsigm = nullptr;
            if (fitsigm_opt.second.has_value()) {
              fitsigm = &(fitsigm_opt.second->get());
            }
            return self.fit_residual(*fitvals.second, fitsigm, *samppos);
          },
#endif
           py::arg("fitvals"), py::arg("samppos"),
           py::arg("fitsigm") = py::none(), R"pydoc(
        Compute the residual of a polynomial fit using `self` as the fitted polynomial

        If the relative uncertainties of the sampled values are known, they may be
        passed via fitsigm. Passing None means that all uncertainties equal one. The
        uncertainties are taken into account when computing the reduced
        chi square value.

        Parameters
        ----------
        fitvals : cpl.core.Vector
            Vector of the p fitted values
        samppos : cpl.core.Matrix
            Matrix of p sample positions, with d rows and p columns

        fitsigm : cpl.core.Vector, optional
            Uncertainties of the sampled values or passed None for a uniform
            uncertainty
        Returns
        -------
        tuple(cpl.core.Vector, double)
            tuple in the format (result, rechisq) where:
            - result is the vector of the fitting residuals, same size as fitvals
            - redchisq is the reduced chi square of the fit

        Raises
        ------
        cpl.core.IncompatibleInputError
            if samppos, fitvals, fitsigm have incompatible sizes
        cpl.core.DivisionByZeroError
            if an element in fitsigm is zero
        cpl.core.DataNotFoundError
            if the number of columns in samppos is less than than the number of coefficients in the fitted polynomial.

        )pydoc")
      .def("shift_1d", &cpl::core::Polynomial::shift_1d, py::arg("i"),
           py::arg("u"), R"pydoc(
        Modify p, p(x0, x1, ..., xi, ...) := (x0, x1, ..., xi+u, ...)

        Shifting the polynomial p(x) = x^n with u = 1 will generate the binomial
        coefficients for n.

        Shifting the coordinate system to (x,y) for the 2D-polynomium poly2d::

            poly2d.shift_1d(0,x)
            poly2d.shift_1d(1,y)

        Parameters
        ----------
        i : int
            The dimension to shift (0 for first)
        u : float
            The shift

        Raises
        ------
        cpl.core.IllegalInputError
            if i is negative
        cpl.core.AccessOutOfRangeError if i exceeds the dimension of p
        )pydoc")
      .def("add", &cpl::core::Polynomial::add, py::arg("other"), R"pydoc(
        Add a polynomial with the same dimension as `self`

        Parameters
        ----------
        other : cpl.core.Polynomial
            polynomial to add

        Raises
        ------
        cpl.core.IncompatibleInputError
            if `self` and `other` do not have identical dimensions
        )pydoc")
      .def("subtract", &cpl::core::Polynomial::subtract, py::arg("other"),
           R"pydoc(
        Subtract a polynomial with the same dimension as `self`

        Parameters
        ----------
        other : cpl.core.Polynomial
            polynomial to subtract

        Raises
        ------
        cpl.core.IncompatibleInputError
            if `self` and `other` do not have identical dimensions
        )pydoc")
      .def("multiply", &cpl::core::Polynomial::multiply, py::arg("other"),
           R"pydoc(
        Multiply with a polynomial with the same dimension as `self`

        Parameters
        ----------
        other : cpl.core.Polynomial
            polynomial to multiply with

        Raises
        ------
        cpl.core.IncompatibleInputError
            if `self` and `other` do not have identical dimensions
        )pydoc")
      .def("multiply_scalar", &cpl::core::Polynomial::multiply_scalar,
           py::arg("factor"), R"pydoc(
        Multiply polynomial `self` with a scalar

        Parameters
        ----------
        factor : float
            factor to multiply with

        Notes
        -----
        If factor is zero, all coefficients are reset, if it is 1 all are copied
        )pydoc")
      .def("__eq__", &cpl::core::Polynomial::operator==)
      .def("__eq__",
           [](cpl::core::Polynomial& /* self */,
              py::object /* other */) -> bool { return false; })
      .def("__ne__", &cpl::core::Polynomial::operator!=)
      .def("__ne__",
           [](cpl::core::Polynomial& /* self */,
              py::object /* other */) -> bool { return true; })
      .def("__deepcopy__",
           [](cpl::core::Polynomial& self, py::dict /* memo */)
               -> cpl::core::Polynomial { return self.duplicate(); });
}

struct poly_terms
{
  size dimensionality;
  std::vector<std::pair<std::deque<size>, double>> terms;
};

/**
 * Given a numpy.polynomial.Polynomial or double, determines how many dimensions
 * said polynomial has. (A 'double' is 0-dimensional, a Polynomial with scalar
 * coefficients is 1 dimensional, a Polynomial with 1-dimensional polynomials
 * as some of the coefficients is 2-dimensional)
 */
poly_terms
numpy_poly_collect_terms(py::object coeffs)
{
  py::array next_dimension;
  try {
    // Recursive case: Collect terms of a polynomial
    next_dimension = py::cast<py::array>(coeffs.attr("coef"));
  }
  catch (const py::error_already_set& /* unused */) {  // AttributeError
    // Base case: This coeffs wasn't a numpy polynomial
    return {0, {{{}, py::cast<double>(coeffs)}}};
  }
  poly_terms output{};
  size i = 0;
  for (auto element : next_dimension) {
    auto lower_terms =
        numpy_poly_collect_terms(py::reinterpret_borrow<py::object>(element));
    if (lower_terms.dimensionality != 0 ||
        lower_terms.terms.at(0).second != 0.0) {
      output.dimensionality =
          std::max(lower_terms.dimensionality + 1, output.dimensionality);
      // Move terms to the parent terms object:
      for (auto term : lower_terms.terms) {
        // This is the coefficient of the i'th power of x
        // (Where this polynomial is P(x, y, ...))
        // so the term goes from x²y³ to x³y²z³
        term.first.push_front(i);
        output.terms.push_back(term);
      }
    }
    ++i;
  }
  return output;
}

cpl::core::Polynomial
py_polynomial_constructor(py::object numpy_poly)
{
  poly_terms terms;
  try {
    terms = numpy_poly_collect_terms(numpy_poly);
  }
  catch (const py::cast_error& /* unused */) {
    // Assume this is an iterable of double coefs if it's not a polynomial
    terms = {1, {}};
    size i = 0;
    for (auto elem : py::cast<py::iterable>(numpy_poly)) {
      double d_elem =
          py::cast<double>(py::reinterpret_borrow<py::object>(elem));
      terms.terms.emplace_back(std::deque{i}, d_elem);
      ++i;
    }
  }

  cpl::core::Polynomial retval(terms.dimensionality);
  for (auto term : terms.terms) {
    // We need contiguous memory for set_coeff to work
    // but term.first is generated by push_front, so must be deque
    std::vector powers(term.first.begin(), term.first.end());
    retval.set_coeff(powers, term.second);
  }
  return retval;
}

// FIXME: The following namespace/function is not used anywhere, however
//        it is also not clear for what it should have been used, or if
//        this is part of an unfinished feature. So for now just guard it
//        but keep it!
#if 0
namespace as_cpl_polynomial_types
{
using deleter_ty = std::function<void(cpl::core::Polynomial*)>;
using unique_ty = std::unique_ptr<cpl::core::Polynomial, deleter_ty>;
using storage_ty = std::optional<unique_ty>;
using return_ty = std::optional<std::reference_wrapper<cpl::core::Polynomial>>;
}  // namespace as_cpl_polynomial_types

/*
 * Convert any compatible python object/None to a cpl::core::Polynomial (.second
 * of return value)
 *
 * If a Numpy Polynomial or arguments to make one are passed in, a CPL
 * Polynomial is created. If a Python None is passed in, an empty optional is
 * returned. Otherwise, a python Type error is thrown
 *
 * The returned unique_ptr owns any created cpl::core::Polynomials, if there was
 * one created. Keep it around for as long as you wish to use the
 * reference_wrapper<cpl::core::Polynomial>.
 *
 * TODO: Modifications to the polynomial aren't reflected to passed in python
 * object but that could be done in the custom deleter.
 */
static std::pair<as_cpl_polynomial_types::storage_ty,
                 as_cpl_polynomial_types::return_ty>
as_cpl_polynomial(py::object poly)
{
  using namespace as_cpl_polynomial_types;
  if (poly.is_none()) {
    return std::make_pair<storage_ty, return_ty>({}, {});
  }

  try {
    // Case: The cpl::core::Polynomial already exists, wrapped by a
    // cpl.core.Polynomial class. Pybind can unwrap it for us:

    // Casting to a Polynomial* (as opposed to a Polynomial&) is required for
    // pybind to NOT create a temporary Polynomial.

    cpl::core::Polynomial* already_created =
        py::cast<cpl::core::Polynomial*>(poly);
    return std::make_pair<storage_ty, return_ty>({}, {*already_created});
  }
  catch (const py::cast_error& /* unused */) {
    try {
      // Case: The input isn't a cpl::core::Polynomial, so we create one
      // using the converting constructor

      // TODO: Deleter that copies data from the polynomial back to the python
      // obj
      unique_ty storage(
          new cpl::core::Polynomial(std::move(py_polynomial_constructor(poly))),
          [](cpl::core::Polynomial* to_delete) -> void { delete to_delete; });

      return std::make_pair<storage_ty, return_ty>(std::move(storage),
                                                   *(storage.get()));
    }
    catch (const py::cast_error& /* unused */) {
      throw py::type_error(
          "Expected cpl.core.Polynomial, numpy.polynomial.Polynomial or "
          "array_like");
    }
  }
}
#endif
