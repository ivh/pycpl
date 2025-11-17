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

#include "cplcore/vector.hpp"

#include <cstdio>
#include <cstring>

#include "cplcore/error.hpp"

namespace cpl
{
namespace core
{
Vector::Vector(std::vector<double> values)
{
  double* new_data = new double[values.size()];
  std::copy(values.begin(), values.end(), new_data);
  m_interface =
      Error::throw_errors_with(cpl_vector_wrap, values.size(), new_data);
}

Vector::Vector(cpl_vector* to_steal) noexcept : m_interface(to_steal) {}

Vector::Vector(Vector&& other) noexcept : m_interface(other.m_interface)
{
  m_interface =
      Error::throw_errors_with(cpl_vector_duplicate, other.m_interface);
  other.m_interface = nullptr;
}

Vector&
Vector::operator=(const Vector& other)
{
  m_interface = nullptr;
  // If the below does throw, we don't want m_interface to point anywhere
  m_interface =
      Error::throw_errors_with(cpl_vector_duplicate, other.m_interface);
  return *this;
}

Vector&
Vector::operator=(Vector&& other) noexcept
{
  m_interface = other.m_interface;
  other.m_interface = nullptr;
  return *this;
}

Vector::Vector(size n)
    : m_interface(Error::throw_errors_with(cpl_vector_new, n))
{
  double* data = Error::throw_errors_with(cpl_vector_get_data, m_interface);
  std::memset(data, 0, sizeof(double) * n);
}

Vector
Vector::wrap(size n, double* data)
{
  return Vector(Error::throw_errors_with(cpl_vector_wrap, n, data));
}

Vector::~Vector() { Error::throw_errors_with(cpl_vector_delete, m_interface); }

void*
Vector::unwrap()
{
  void* data = Error::throw_errors_with(cpl_vector_unwrap, m_interface);
  m_interface = nullptr;  // Don't let cpl::core::Vector double-free
  return data;
}

Vector
Vector::read(const std::filesystem::path& filename)
{
  return Vector(Error::throw_errors_with(cpl_vector_read, filename.c_str()));
}

std::string
Vector::dump() const
{
  char* charBuff;
  size_t len;
  // Open char pointer as stream
  FILE* stream = open_memstream(&charBuff, &len);
  Error::throw_errors_with(cpl_vector_dump, m_interface, stream);

  // Flush to char pointer
  fflush(stream);
  // Cast to std::string
  std::string returnString(charBuff);
  fclose(stream);
  free(charBuff);

  return returnString;
}

Vector
Vector::load(const std::filesystem::path& filename, size xtnum)
{
  return Vector(
      Error::throw_errors_with(cpl_vector_load, filename.c_str(), xtnum));
}

void
Vector::save(const std::filesystem::path& filename, cpl_type type,
             std::optional<cpl::core::PropertyList> pl_opt, unsigned mode) const
{
  if (auto pl = pl_opt) {
    auto plptr = pl->ptr();
    Error::throw_errors_with(cpl_vector_save, m_interface, filename.c_str(),
                             type, plptr.get(), mode);
  } else {
    Error::throw_errors_with(cpl_vector_save, m_interface, filename.c_str(),
                             type, nullptr, mode);
  }
}

Vector::Vector(const Vector& v)
    : m_interface(Error::throw_errors_with(cpl_vector_duplicate, v.m_interface))
{
}

void
Vector::copy(const Vector& source)
{
  Error::throw_errors_with(cpl_vector_copy, m_interface, source.m_interface);
}

size
Vector::get_size() const
{
  return Error::throw_errors_with(cpl_vector_get_size, m_interface);
}

double*
Vector::data()
{
  return Error::throw_errors_with(cpl_vector_get_data, m_interface);
}

const double*
Vector::data() const
{
  return Error::throw_errors_with(cpl_vector_get_data_const, m_interface);
}

double
Vector::get(size idx) const
{
  return Error::throw_errors_with(cpl_vector_get, m_interface, idx);
}

void
Vector::set_size(size newsize)
{
  Error::throw_errors_with(cpl_vector_set_size, m_interface, newsize);
}

void
Vector::set(size idx, double value)
{
  Error::throw_errors_with(cpl_vector_set, m_interface, idx, value);
}

void
Vector::add(const Vector& v2)
{
  Error::throw_errors_with(cpl_vector_add, m_interface, v2.m_interface);
}

void
Vector::subtract(const Vector& v2)
{
  Error::throw_errors_with(cpl_vector_subtract, m_interface, v2.m_interface);
}

void
Vector::multiply(const Vector& v2)
{
  Error::throw_errors_with(cpl_vector_multiply, m_interface, v2.m_interface);
}

void
Vector::divide(const Vector& v2)
{
  Error::throw_errors_with(cpl_vector_divide, m_interface, v2.m_interface);
}

double
Vector::product(const Vector& v2) const
{
  return Error::throw_errors_with(cpl_vector_product, m_interface,
                                  v2.m_interface);
}

void
Vector::cycle(double shift)
{
  Error::throw_errors_with(cpl_vector_cycle, m_interface, nullptr, shift);
}

void
Vector::sort(cpl_sort_direction dir)
{
  Error::throw_errors_with(cpl_vector_sort, m_interface, dir);
}

void
Vector::add_scalar(double addend)
{
  Error::throw_errors_with(cpl_vector_add_scalar, m_interface, addend);
}

void
Vector::subtract_scalar(double subtrahend)
{
  Error::throw_errors_with(cpl_vector_subtract_scalar, m_interface, subtrahend);
}

void
Vector::multiply_scalar(double factor)
{
  Error::throw_errors_with(cpl_vector_multiply_scalar, m_interface, factor);
}

void
Vector::divide_scalar(double divisor)
{
  Error::throw_errors_with(cpl_vector_divide_scalar, m_interface, divisor);
}

void
Vector::logarithm(double base)
{
  Error::throw_errors_with(cpl_vector_logarithm, m_interface, base);
}

void
Vector::exponential(double base)
{
  Error::throw_errors_with(cpl_vector_exponential, m_interface, base);
}

void
Vector::power(double exponent)
{
  Error::throw_errors_with(cpl_vector_power, m_interface, exponent);
}

void
Vector::fill(double val)
{
  Error::throw_errors_with(cpl_vector_fill, m_interface, val);
}

void
Vector::sqrt()
{
  Error::throw_errors_with(cpl_vector_sqrt, m_interface);
}

// FIXME: The following is not used. Verify the status. Is this (part of) an
//        unfinished feature, or just dead code? Note that if the function is
//        not needed, the surrounding namespace is also obsolete. If this is
//        dead code, remove it! But before doing so make sure the comment
//        does not contain valuable information!
#if 0
namespace
{

size
bisect_no_rethrow(const cpl_vector* m_interface, double key)
{
  // This is a helper that doesn't have the rethrow_add_context
  // it is required to be a separate function so that Error::throw_errors_with
  // can have its template arguments deduced
  return Error::throw_errors_with(cpl_vector_find, m_interface, key);
}

}  // namespace
#endif

size
Vector::bisect(double key) const
{
  try {
    return Error::throw_errors_with(cpl_vector_find, m_interface, key);
  }
  catch (const cpl::core::IllegalInputError& error) {
    throw cpl::core::IllegalInputError(
        PYCPL_ERROR_LOCATION,
        "Attempt to bisect a vector that was detected as being not ascending",
        error);
  }
}

Vector
Vector::extract(size istart, size istop, size istep) const
{
  return Error::throw_errors_with(cpl_vector_extract, m_interface, istart,
                                  istop, istep);
}

size
Vector::get_minpos() const
{
  return Error::throw_errors_with(cpl_vector_get_minpos, m_interface);
}

size
Vector::get_maxpos() const
{
  return Error::throw_errors_with(cpl_vector_get_maxpos, m_interface);
}

double
Vector::get_min() const
{
  return Error::throw_errors_with(cpl_vector_get_min, m_interface);
}

double
Vector::get_max() const
{
  return Error::throw_errors_with(cpl_vector_get_max, m_interface);
}

double
Vector::get_sum() const
{
  return Error::throw_errors_with(cpl_vector_get_sum, m_interface);
}

double
Vector::get_mean() const
{
  return Error::throw_errors_with(cpl_vector_get_mean, m_interface);
}

double
Vector::get_median()
{
  return Error::throw_errors_with(cpl_vector_get_median, m_interface);
}

double
Vector::get_median_const() const
{
  return Error::throw_errors_with(cpl_vector_get_median_const, m_interface);
}

double
Vector::get_stdev() const
{
  return Error::throw_errors_with(cpl_vector_get_stdev, m_interface);
}

size
Vector::correlate(const Vector& v1, const Vector& v2)
{
  return Error::throw_errors_with(cpl_vector_correlate, m_interface,
                                  v1.m_interface, v2.m_interface);
}

Vector
Vector::filter_lowpass_create(cpl_lowpass filter_type, size hw) const
{
  return Error::throw_errors_with(cpl_vector_filter_lowpass_create, m_interface,
                                  filter_type, hw);
}

Vector
Vector::filter_median_create(size hw) const
{
  return Error::throw_errors_with(cpl_vector_filter_median_create, m_interface,
                                  hw);
}

void
Vector::fill_kernel_profile(cpl_kernel type, double radius)
{
  Error::throw_errors_with(cpl_vector_fill_kernel_profile, m_interface, type,
                           radius);
}

Vector
Vector::duplicate()
{
  cpl_vector* dup = Error::throw_errors_with(cpl_vector_duplicate, m_interface);
  return Vector(dup);
}

std::tuple<double, double, double, double, double, double, cpl::core::Matrix>
Vector::fit_gaussian(const Vector& x, const Vector& y, cpl_fit_mode fit_pars,
                     std::optional<std::reference_wrapper<Vector>> sigma_y,
                     std::optional<double> x0, std::optional<double> sigma,
                     std::optional<double> area, std::optional<double> offset)
{
  // TODO: CPL does not currently support any non-null values for x_sigma.
  // Uncomment and edit the below lines once its supported
  // cpl_vector *sigma_x_interface = nullptr;
  // if(auto sigma_x_val = sigma_x) {
  //     sigma_x_interface = sigma_x_val->get().m_interface;
  // }

  double x0_out, sigma_out, area_out, offset_out, mse_out, red_chisq_out;
  if (!(CPL_FIT_CENTROID & fit_pars)) {
    if (!x0.has_value()) {
      throw cpl::core::IllegalInputError(
          PYCPL_ERROR_LOCATION,
          "cpl.core.FitMode.CENTROID not set, but x0 is not passed");
    }
    x0_out = x0.value();
  }

  if (!(CPL_FIT_STDEV & fit_pars)) {
    if (!sigma.has_value()) {
      throw cpl::core::IllegalInputError(
          PYCPL_ERROR_LOCATION,
          "cpl.core.FitMode.STDEV not set, but sigma is not passed");
    }
    sigma_out = sigma.value();
  }

  if (!(CPL_FIT_AREA & fit_pars)) {
    if (!area.has_value()) {
      throw cpl::core::IllegalInputError(
          PYCPL_ERROR_LOCATION,
          "cpl.core.FitMode.AREA not set, but area is not passed");
    }
    area_out = area.value();
  }

  if (!(CPL_FIT_OFFSET & fit_pars)) {
    if (!offset.has_value()) {
      throw cpl::core::IllegalInputError(
          PYCPL_ERROR_LOCATION,
          "cpl.core.FitMode.OFFSET not set, but offset is not passed");
    }
    offset_out = offset.value();
  }


  const cpl_vector* sigma_y_interface;

  if (auto sigma_y_val = sigma_y) {
    sigma_y_interface = sigma_y_val->get().m_interface;
  } else {
    sigma_y_interface = NULL;
  }
  cpl_matrix* cov;
  // NOTE: y_sigma is required in this function as a duplicate function in the
  // fit gaussian function sets the error state to CPL_ERROR_NULL_INPUT
  // TODO: Check the errors manually rather than throw errors with
  Error::throw_errors_with(cpl_vector_fit_gaussian, x.m_interface, nullptr,
                           y.m_interface, sigma_y_interface, fit_pars, &x0_out,
                           &sigma_out, &area_out, &offset_out, &mse_out,
                           sigma_y.has_value() ? &red_chisq_out : nullptr,
                           sigma_y.has_value() ? &cov : nullptr);
  // output->covariance = ;
  // std::cout<<output->covariance.use_count()<<"\n";
  return std::make_tuple(x0_out, sigma_out, area_out, offset_out, mse_out,
                         sigma_y.has_value() ? red_chisq_out : 0.,
                         sigma_y.has_value() ? cpl::core::Matrix(cov) : NULL);
}

/*
Due to being deprecated in CPL, these are not bound
Vector Vector::new_lss_kernel(double slitw, double fwhm) {
    return
Vector(Error::throw_errors_with(cpl_vector_new_lss_kernel,slitw,fwhm));
}
Vector &Vector::convolve_symmetric(const Vector& conv_kernel) {
    Error::throw_errors_with(cpl_vector_convolve_symmetric,m_interface,conv_kernel.m_interface);
    return *this;
} */

bool
Vector::operator==(const Vector& other) const
{
  return other.get_size() == get_size() &&
         std::memcmp(data(), other.data(), get_size() * sizeof(double)) == 0;
}

bool
Vector::operator!=(const Vector& other) const
{
  return !operator==(other);
}

const cpl_vector*
Vector::ptr() const
{
  return m_interface;
}

cpl_vector*
Vector::ptr()
{
  return m_interface;
}

cpl_vector*
Vector::unwrap(Vector&& self)
{
  cpl_vector* interface = self.m_interface;
  self.m_interface = nullptr;
  return interface;
}


}  // namespace core
}  // namespace cpl
