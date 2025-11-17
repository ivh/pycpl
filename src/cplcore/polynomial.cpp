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

#include "cplcore/polynomial.hpp"

#include <cstdio>
#include <cstring>
#include <limits>
#include <sstream>

#include "cplcore/error.hpp"

namespace cpl
{
namespace core
{
Polynomial::Polynomial(cpl_polynomial* to_steal) noexcept
    : m_interface(to_steal)
{
}

Polynomial::Polynomial(Polynomial&& other) noexcept
    : m_interface(other.m_interface)
{
  other.m_interface = nullptr;
}

Polynomial&
Polynomial::operator=(const Polynomial& other)
{
  m_interface = nullptr;
  // If the below does throw, we don't want m_interface to point anywhere
  m_interface =
      Error::throw_errors_with(cpl_polynomial_duplicate, other.m_interface);
  return *this;
}

Polynomial&
Polynomial::operator=(Polynomial&& other) noexcept
{
  m_interface = other.m_interface;
  other.m_interface = nullptr;
  return *this;
}

Polynomial::Polynomial(size dim)
    : m_interface(Error::throw_errors_with(cpl_polynomial_new, dim))
{
}

Polynomial::~Polynomial()
{
  Error::throw_errors_with(cpl_polynomial_delete, m_interface);
}

std::string
Polynomial::dump() const
{
  char* charBuff;
  size_t len;
  FILE* stream = open_memstream(&charBuff, &len);  // Open char pointer as
                                                   // stream
  Error::throw_errors_with(cpl_polynomial_dump, m_interface, stream);
  fflush(stream);                      // Flush to char pointer
  std::string returnString(charBuff);  // Cast to std::string
  fclose(stream);
  free(charBuff);

  return returnString;
}

Polynomial::Polynomial(const Polynomial& self)
    : m_interface(
          Error::throw_errors_with(cpl_polynomial_duplicate, self.ptr()))
{
}

void
Polynomial::copy(const Polynomial& other)
{
  Error::throw_errors_with(cpl_polynomial_copy, m_interface, other.ptr());
}

double
Polynomial::get_coeff(const std::vector<size>& pows) const
{
  if (pows.size() != get_dimension()) {
    std::ostringstream ss;
    ss << "get_coeff takes a list of exactly ";
    ss << get_dimension() << " length (dimensionality), but was received ";
    ss << " list of length " << pows.size();
    throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION, ss.str());
  }
  return Error::throw_errors_with(cpl_polynomial_get_coeff, m_interface,
                                  &pows[0]);
}

void
Polynomial::set_coeff(const std::vector<size>& pows, double value)
{
  if (pows.size() != get_dimension()) {
    std::ostringstream ss;
    ss << "set_coeff takes a list of exactly ";
    ss << get_dimension() << " length (dimensionality), but was received ";
    ss << " list of length " << pows.size();
    throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION, ss.str());
  }
  Error::throw_errors_with(cpl_polynomial_set_coeff, m_interface, &pows[0],
                           value);
}

int
Polynomial::compare(const Polynomial& other, double tol) const
{
  return Error::throw_errors_with(cpl_polynomial_compare, m_interface,
                                  other.ptr(), tol);
}

size
Polynomial::get_dimension() const
{
  return Error::throw_errors_with(cpl_polynomial_get_dimension, m_interface);
}

size
Polynomial::get_degree() const
{
  return Error::throw_errors_with(cpl_polynomial_get_degree, m_interface);
}

double
Polynomial::eval(const Vector& x) const
{
  return Error::throw_errors_with(cpl_polynomial_eval, m_interface, x.ptr());
}

std::tuple<double, cpl::core::Vector>
Polynomial::eval_2d(double x, double y) const
{
  double gradient[2] =
      {};  // create double array of 2 elements to contain the X and Y
           // components of the gradient vector at the evaluated point
  double result = Error::throw_errors_with(cpl_polynomial_eval_2d, m_interface,
                                           x, y, gradient);
  cpl::core::Vector gradient_vec = cpl::core::Vector::wrap(2, gradient);
  return std::make_tuple(result, std::move(gradient_vec));
}

std::tuple<double, cpl::core::Vector>
Polynomial::eval_3d(double x, double y, double z) const
{
  double gradient[3] =
      {};  // create double array of 3 elements to contain the X, Y and Z
           // components of the gradient vector at the evaluated point
  double result = Error::throw_errors_with(cpl_polynomial_eval_3d, m_interface,
                                           x, y, z, gradient);
  cpl::core::Vector gradient_vec = cpl::core::Vector::wrap(3, gradient);
  return std::make_tuple(result, std::move(gradient_vec));
}

Polynomial
Polynomial::extract(size dim, const Polynomial& other) const
{
  return Error::throw_errors_with(cpl_polynomial_extract, m_interface, dim,
                                  other.ptr());
}

void
Polynomial::add(const Polynomial& second)
{
  Error::throw_errors_with(cpl_polynomial_add, m_interface, m_interface,
                           second.ptr());
}

void
Polynomial::subtract(const Polynomial& second)
{
  Error::throw_errors_with(cpl_polynomial_subtract, m_interface, m_interface,
                           second.ptr());
}

void
Polynomial::multiply(const Polynomial& second)
{
  Error::throw_errors_with(cpl_polynomial_multiply, m_interface, m_interface,
                           second.ptr());
}

void
Polynomial::multiply_scalar(double factor)
{
  Error::throw_errors_with(cpl_polynomial_multiply_scalar, m_interface,
                           m_interface, factor);
}

void
Polynomial::derivative(size dim)
{
  Error::throw_errors_with(cpl_polynomial_derivative, m_interface, dim);
}

void
Polynomial::fit(const cpl::core::Matrix& samppos,
                const cpl::core::Vector& fitsvals, bool dimdeg,
                std::vector<size> maxdeg,
                std::optional<std::vector<bool>> sampsym,
                std::optional<cpl::core::Vector> fitsigm,
                std::optional<std::vector<size>> mindeg)
{
  cpl_boolean dimdeg_cpl = (cpl_boolean)dimdeg;
  cpl_boolean* sampsym_ptr = nullptr;
  // The rest of the checks (samppos matrix matching dimensionality x |fitvals|)
  // are done inside cpl_polynomial_fit.
  if (sampsym.has_value()) {
    if (sampsym->size() != get_dimension()) {
      throw cpl::core::IncompatibleInputError(
          PYCPL_ERROR_LOCATION,
          "sampsym must match Polynomial's dimensionality");
    }
    // Create cpl_boolean array, because thats what CPL uses
    int len = sampsym.value().size();
    sampsym_ptr = new cpl_boolean[len];
    for (int i = 0; i < len; i++) {
      sampsym_ptr[i] = (cpl_boolean)((bool)sampsym.value()[i]);
    }
  }
  if (mindeg.has_value()) {
    if (mindeg->size() != get_dimension()) {
      throw cpl::core::IncompatibleInputError(
          PYCPL_ERROR_LOCATION,
          "mindeg must match Polynomial's dimensionality");
    }
  }
  if (maxdeg.size() != get_dimension()) {
    throw cpl::core::IncompatibleInputError(
        PYCPL_ERROR_LOCATION, "maxdeg must match Polynomial's dimensionality");
  }

  Error::throw_errors_with(
      cpl_polynomial_fit, m_interface, samppos.ptr(), sampsym_ptr,
      fitsvals.ptr(), fitsigm.has_value() ? fitsigm->ptr() : nullptr,
      dimdeg_cpl, mindeg.has_value() ? mindeg->data() : nullptr, maxdeg.data());
  delete sampsym_ptr;
}

std::pair<cpl::core::Vector, double>
Polynomial::fit_residual(const cpl::core::Vector& fitvals,
                         const cpl::core::Matrix& samppos,
                         const cpl::core::Vector* fitsigm) const
{
  // cpl_vector_fill_polynomial_fit_residual will set the size of this output
  // vector But to C++ initialize, it needs an initial value >0 so to avoid
  // allocation, we use the same value that the function would set it to anyway:
  // fitvals size
  std::pair<cpl::core::Vector, double> retval(fitvals.get_size(), 0.0);
  Error::throw_errors_with(cpl_vector_fill_polynomial_fit_residual,
                           retval.first.ptr(), fitvals.ptr(),
                           fitsigm == nullptr ? nullptr : fitsigm->ptr(),
                           m_interface, samppos.ptr(), &retval.second);
  return retval;
}

std::tuple<double, double>
Polynomial::eval_1d(double x) const
{
  double pd;
  double res =
      Error::throw_errors_with(cpl_polynomial_eval_1d, m_interface, x, &pd);
  return std::make_tuple(res, pd);
}

std::tuple<double, double>
Polynomial::eval_1d_diff(double a, double b) const
{
  double ppa;
  double res = Error::throw_errors_with(cpl_polynomial_eval_1d_diff,
                                        m_interface, a, b, &ppa);
  return std::make_tuple(res, ppa);
}

cpl::core::Vector
Polynomial::fill_polynomial(size out_size, double x0, double d) const
{
  // cpl_vector_fill_polynomial_fit_residual will set the size of this output
  // vector But to C++ initialize, it needs an initial value >0 so to avoid
  // allocation, we use the same value that the function would set it to anyway:
  // fitvals size
  cpl::core::Vector retval(out_size);
  Error::throw_errors_with(cpl_vector_fill_polynomial, retval.ptr(),
                           m_interface, x0, d);
  return retval;
}

double
Polynomial::solve_1d(double x0, size mul) const
{
  double res;
  Error::throw_errors_with(cpl_polynomial_solve_1d, m_interface, x0, &res, mul);
  return res;
}

void
Polynomial::shift_1d(size i, double u)
{
  Error::throw_errors_with(cpl_polynomial_shift_1d, m_interface, i, u);
}

/*
Due to being deprecated in CPL, these are not bound
std::pair<Polynomial, double> Polynomial::fit_1d(
    const Vector& x_pos,
    const Vector& values,
    size degree
)
std::pair<Polynomial, double> Polynomial::fit_2d(
    const Bivector& xy_pos,
    const Vector& values,
    size degree
)
Polynomial Polynomial::new_lss_kernel(double slitw, double fwhm)
Polynomial &Polynomial::convolve_symmetric(const Polynomial& conv_kernel)
*/

bool
Polynomial::operator==(const Polynomial& other) const
{
  return compare(other, std::numeric_limits<double>::epsilon()) == 0;
}

bool
Polynomial::operator!=(const Polynomial& other) const
{
  return !operator==(other);
}

const cpl_polynomial*
Polynomial::ptr() const
{
  return m_interface;
}

cpl_polynomial*
Polynomial::ptr()
{
  return m_interface;
}

cpl_polynomial*
Polynomial::unwrap(Polynomial&& self)
{
  cpl_polynomial* interface = self.m_interface;
  self.m_interface = nullptr;
  return interface;
}

Polynomial
Polynomial::duplicate() const
{
  return Polynomial(
      Error::throw_errors_with(cpl_polynomial_duplicate, m_interface));
}

}  // namespace core
}  // namespace cpl
