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

#include "cpldrs/wlcalib.hpp"

#include <cmath>

#include <cpl_bivector.h>
#include <cpl_error.h>
#include <cpl_polynomial.h>
#include <cpl_vector.h>

#include "cplcore/error.hpp"

// Global definitions (as to not have multiple instances compiled in)
thread_local std::function<int(const cpl::core::Vector&,
                               const cpl::core::Polynomial&)>
    filler_lambda;

int
filler_trampoline(const cpl::core::Vector& toFill,
                  const cpl::core::Polynomial& disp)
{
  return filler_lambda(toFill, disp);
}

namespace cpl
{
namespace drs
{

SlitModel::SlitModel(std::shared_ptr<cpl::core::Bivector> catalog,
                     double threshold, double wfwhm, int spectrum_size,
                     double wslit)
{
  m_interface = cpl_wlcalib_slitmodel_new();
  // Set values on init
  this->set_spectrum_size(spectrum_size);
  this->set_catalog(catalog);
  this->set_threshold(threshold);
  this->set_wfwhm(wfwhm);
  this->set_wslit(wslit);
}

SlitModel::~SlitModel()
{
  cpl::core::Error::throw_errors_with(cpl_wlcalib_slitmodel_delete,
                                      m_interface);
}

void
SlitModel::set_wslit(double value)
{
  this->m_wslit = value;
  cpl::core::Error::throw_errors_with(cpl_wlcalib_slitmodel_set_wslit,
                                      m_interface, value);
}

double
SlitModel::get_wslit()
{
  return this->m_wslit;
}

void
SlitModel::set_wfwhm(double value)
{
  this->m_wfwhm = value;
  cpl::core::Error::throw_errors_with(cpl_wlcalib_slitmodel_set_wfwhm,
                                      m_interface, value);
}

int
SlitModel::get_spectrum_size()
{
  return this->m_spectrum_size;
}

void
SlitModel::set_spectrum_size(int value)
{
  this->m_spectrum_size = value;
}

double
SlitModel::get_wfwhm()
{
  return this->m_wfwhm;
}

void
SlitModel::set_threshold(double value)
{
  this->m_threshold = value;
  cpl::core::Error::throw_errors_with(cpl_wlcalib_slitmodel_set_threshold,
                                      m_interface, value);
}

double
SlitModel::get_threshold()
{
  return this->m_threshold;
}

void
SlitModel::set_catalog(std::shared_ptr<cpl::core::Bivector> catalog)
{
  // Duplicate the Bivector pointer of catalog's as it can get deleted by this
  // function and the destructor
  cpl_bivector* duplicate = cpl_bivector_new(catalog->get_size());
  // Store the duplicate
  this->m_catalog_ptr = duplicate;
  cpl::core::Error::throw_errors_with(cpl_bivector_copy, this->m_catalog_ptr,
                                      catalog->ptr().get());

  cpl::core::Error::throw_errors_with(
      cpl_wlcalib_slitmodel_set_catalog, m_interface,
      this->m_catalog_ptr);  // Duplicate will be stored in the slitmodel
}

std::shared_ptr<cpl::core::Bivector>
SlitModel::get_catalog()
{
  // Build the duplicate
  size sz = cpl_bivector_get_size(this->m_catalog_ptr);
  cpl_bivector* duplicate = cpl_bivector_new(sz);
  // Store the duplicate
  cpl::core::Error::throw_errors_with(cpl_bivector_copy, duplicate,
                                      this->m_catalog_ptr);
  // Wrap the duplicate to return
  return std::make_shared<cpl::core::Bivector>(duplicate);
}

cpl::core::Vector
SlitModel::fill_line_spectrum(const cpl::core::Polynomial& disp)
{
  cpl_vector* toFill = cpl_vector_new(this->m_spectrum_size);
  cpl::core::Error::throw_errors_with(cpl_wlcalib_fill_line_spectrum, toFill,
                                      m_interface, disp.ptr());
  return cpl::core::Vector(toFill);
}

cpl::core::Vector
SlitModel::fill_logline_spectrum(const cpl::core::Polynomial& disp)
{
  cpl_vector* toFill = cpl_vector_new(this->m_spectrum_size);
  cpl::core::Error::throw_errors_with(cpl_wlcalib_fill_logline_spectrum, toFill,
                                      m_interface, disp.ptr());
  return cpl::core::Vector(toFill);
}

cpl::core::Vector
SlitModel::fill_line_spectrum_fast(const cpl::core::Polynomial& disp)
{
  cpl_vector* toFill = cpl_vector_new(this->m_spectrum_size);
  cpl::core::Error::throw_errors_with(cpl_wlcalib_fill_line_spectrum_fast,
                                      toFill, m_interface, disp.ptr());
  return cpl::core::Vector(toFill);
}

cpl::core::Vector
SlitModel::fill_logline_spectrum_fast(const cpl::core::Polynomial& disp)
{
  cpl_vector* toFill = cpl_vector_new(this->m_spectrum_size);
  cpl::core::Error::throw_errors_with(cpl_wlcalib_fill_logline_spectrum_fast,
                                      toFill, m_interface, disp.ptr());
  return cpl::core::Vector(toFill);
}

std::tuple<cpl::core::Polynomial, double, cpl::core::Vector>
SlitModel::find_best_1d(
    const cpl::core::Vector& spectrum, const cpl::core::Vector& wl_search,
    size nsamples, size hsize,
    // Uncomment below and replace the line after when we get around to properly
    // allowing custom-written filler functions std::function<void(const
    // cpl::drs::SlitModel&, const cpl::core::Vector&,
    // const cpl::core::Polynomial&)> evaluatefiller
    filler filler_func_enum, std::optional<cpl::core::Polynomial> guess)
{
  // Based on enum passed, determine the CPL function pointer to pass
  cpl_error_code (*filler)(cpl_vector*, void*, const cpl_polynomial*);
  switch (filler_func_enum) {
    case LINE: filler = cpl_wlcalib_fill_line_spectrum; break;
    case LOGLINE: filler = cpl_wlcalib_fill_logline_spectrum; break;
    case LINE_FAST: filler = cpl_wlcalib_fill_line_spectrum_fast; break;
    case LOGLINE_FAST: filler = cpl_wlcalib_fill_logline_spectrum_fast; break;
  }

  cpl_polynomial* result = cpl_polynomial_new(1);  // Needs to be 1 dimensional
  cpl_polynomial* guess_cpl = guess.has_value() ? guess.value().ptr() : result;

  double xcmax;
  cpl_vector* xcorrs_cpl =
      cpl_vector_new(pow(nsamples, wl_search.get_size()) *
                     (1 + 2 * hsize));  // xcorrs have a size of
                                        // (at least) N^D*(1 + 2 * hsize).
                                        // N is nsamples
                                        // D is the length of wl_search
  cpl::core::Error::throw_errors_with(
      cpl_wlcalib_find_best_1d, result, guess_cpl, spectrum.ptr(), m_interface,
      filler, wl_search.ptr(), nsamples, hsize, &xcmax, xcorrs_cpl);
  return std::make_tuple(cpl::core::Polynomial(result), xcmax,
                         cpl::core::Vector(xcorrs_cpl));
}

}  // namespace drs
}  // namespace cpl