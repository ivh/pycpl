// This file is part of the PyHDRL Python language bindings
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

#ifndef PYHDRL_CORE_SPECTRUM1D_HPP_
#define PYHDRL_CORE_SPECTRUM1D_HPP_

#include <cstring>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

#include <cpl_array.h>
#include <cpl_image.h>
#include <hdrl_correlation.h>
#include <hdrl_image.h>
#include <hdrl_imagelist.h>
#include <hdrl_parameter.h>
#include <hdrl_spectrumlist.h>
#include <hdrl_spectrum_defs.h>
#include <hdrl_spectrum_resample.h>
#include <hdrl_spectrum_shift.h>
#include <hdrl_types.h>

#include "error.hpp"
#include "imagelist.hpp"
#include "parameter.hpp"

namespace hdrl
{
namespace core
{

// Forward declarations
class Spectrum1D;
class Spectrum1DList;
class ImageList;

// Enum wrapper for wave scale
enum class WaveScale
{
  LINEAR = hdrl_spectrum1D_wave_scale_linear,
  LOG = hdrl_spectrum1D_wave_scale_log
};

// Enum wrapper for interpolation methods
enum class InterpolationMethod
{
  LINEAR = hdrl_spectrum1D_interp_linear,
  CSPLINE = hdrl_spectrum1D_interp_cspline,
  AKIMA = hdrl_spectrum1D_interp_akima
};

// Wrapper class for hdrl_xcorrelation_result
class XCorrelationResult
{
 public:
  XCorrelationResult(double shift, double error, double quality);
  XCorrelationResult(hdrl_xcorrelation_result* result);
  ~XCorrelationResult();

  // Disable copying
  XCorrelationResult(const XCorrelationResult&) = delete;
  XCorrelationResult& operator=(const XCorrelationResult&) = delete;

  // Enable moving
  XCorrelationResult(XCorrelationResult&& other) noexcept;
  XCorrelationResult& operator=(XCorrelationResult&& other) noexcept;

  // Accessors
  double get_shift() const { return m_shift; }
  double get_error() const { return m_error; }
  double get_quality() const { return m_quality; }
  hdrl_xcorrelation_result* get_raw() const { return m_result; }

 private:
  double m_shift;
  double m_error;
  double m_quality;
  hdrl_xcorrelation_result* m_result;
};

class Spectrum1D
{
 public:
  // --- Environment Configuration ---
  static void init_environment();

  // --- Constructors/Destructors ---
  Spectrum1D();
  explicit Spectrum1D(hdrl_spectrum1D* spectrum);
  Spectrum1D(const hdrl_spectrum1D* spectrum);
  ~Spectrum1D();

  // Disable copying
  Spectrum1D(const Spectrum1D&) = delete;
  Spectrum1D& operator=(const Spectrum1D&) = delete;

  // Enable moving
  Spectrum1D(Spectrum1D&& other) noexcept;
  Spectrum1D& operator=(Spectrum1D&& other) noexcept;

  // --- Static Constructors ---
  static Spectrum1D create(const double* flux, const double* flux_e,
                          const double* wavelengths, size_t size, WaveScale scale);

  static Spectrum1D create_error_free(const double* flux, const double* wavelengths,
                                     size_t size, WaveScale scale);

  static Spectrum1D create_error_DER_SNR(const double* flux, size_t half_window,
                                        const double* wavelengths, size_t size, WaveScale scale);

  static Spectrum1D create_analytic(std::function<std::pair<double, double>(double)> func,
                                    const double* wavelengths, size_t size, WaveScale scale);

  static Spectrum1DList create_list(const std::vector<Spectrum1D>& spectra);

  // --- Core Methods ---
  size_t size() const;
  WaveScale get_scale() const;
  const hdrl_image* get_flux() const;
  const cpl_array* get_wavelengths() const;

  // --- Arithmetic Operations ---
  void mul_scalar(double scalar);
  Spectrum1D mul_scalar_create(double scalar) const;
  void div_scalar(double scalar);
  Spectrum1D div_scalar_create(double scalar) const;
  void add_scalar(double scalar);
  Spectrum1D add_scalar_create(double scalar) const;
  void sub_scalar(double scalar);
  Spectrum1D sub_scalar_create(double scalar) const;
  void pow_scalar(double scalar);
  Spectrum1D pow_scalar_create(double scalar) const;
  void exp_scalar(double scalar);
  Spectrum1D exp_scalar_create(double scalar) const;

  // --- Spectrum-Spectrum Operations ---
  Spectrum1D div_spectrum_create(const Spectrum1D& other) const;
  Spectrum1D mul_spectrum_create(const Spectrum1D& other) const;
  Spectrum1D add_spectrum_create(const Spectrum1D& other) const;
  Spectrum1D sub_spectrum_create(const Spectrum1D& other) const;

  void div_spectrum(const Spectrum1D& other);
  void mul_spectrum(const Spectrum1D& other);
  void add_spectrum(const Spectrum1D& other);
  void sub_spectrum(const Spectrum1D& other);

  // --- Spectrum Shift Operations ---
  XCorrelationResult compute_shift_xcorrelation(const Spectrum1D& other, size_t half_win,
                                              bool normalize = true) const;

  static Parameter create_shift_fit_parameter(double wguess, double range_wmin,
                                             double range_wmax, double fit_wmin,
                                             double fit_wmax, double fit_half_win);

  double compute_shift_fit(const Parameter& par) const;

  // --- Wavelength Operations ---
  void wavelength_shift(double shift);
  Spectrum1D wavelength_shift_create(double shift) const;

  void wavelength_mult_scalar_linear(double scale);
  Spectrum1D wavelength_mult_scalar_linear_create(double scale) const;

  void wavelength_convert_to_linear();
  Spectrum1D wavelength_convert_to_linear_create() const;

  void wavelength_convert_to_log();
  Spectrum1D wavelength_convert_to_log_create() const;

  // --- Spectrum Selection ---
  Spectrum1D select_window(double lambda_min, double lambda_max, bool is_internal = false) const;

  // --- Spectrum Resample Operations ---
  static hdrl_spectrum1D_interpolation_method to_c_interp_method(InterpolationMethod method);
  static Parameter create_interpolation_parameter(InterpolationMethod method);
  static Parameter create_fit_parameter(int k, int nCoeff);
  static Parameter create_windowed_fit_parameter(int k, int nCoeff, long window, double factor);
  static Parameter create_integration_parameter();

  Spectrum1D resample(const Spectrum1D& other, InterpolationMethod method = InterpolationMethod::AKIMA) const;
  Spectrum1D resample_to_wavelengths(const std::vector<double>& wavelengths,
                                    InterpolationMethod method = InterpolationMethod::AKIMA) const;
  Spectrum1D resample_fit(const std::vector<double>& wavelengths, int k, int nCoeff) const;
  Spectrum1D resample_windowed_fit(const std::vector<double>& wavelengths, int k, int nCoeff,
                                  long window, double factor) const;
  Spectrum1D resample_integrate(const std::vector<double>& wavelengths) const;

  // --- I/O ---
  void save(const std::string& filename) const;

  // --- Data Access ---
  std::vector<double> get_flux_vector() const;
  std::vector<double> get_wavelength_vector() const;
  std::vector<double> get_flux_error_vector() const;

  // --- Utility Methods ---
  bool is_compatible_with(const Spectrum1D& other) const;
  Spectrum1D duplicate() const;

  // --- Raw Access (for bindings) ---
  hdrl_spectrum1D* get_raw() const { return m_interface; }

  // Get raw pointer
  hdrl_spectrum1D* ptr() { return m_interface; }
  const hdrl_spectrum1D* ptr() const { return m_interface; }

 private:
  hdrl_spectrum1D* m_interface;

  // Helper method for single point resampling
  Spectrum1D handle_single_point_resample(double target_wavelength) const;
};

#if defined HDRL_USE_EXPERIMENTAL || defined HDRL_USE_PRIVATE
struct CollapseResult {
    Spectrum1D result;
    ImageList aligned_images;
};
#endif

// Wrapper class for hdrl_spectrum1Dlist
class Spectrum1DList {
 public:
  // Constructors/Destructors
  Spectrum1DList();
  explicit Spectrum1DList(hdrl_spectrum1Dlist* list);
  ~Spectrum1DList();

  // Disable copying
  Spectrum1DList(const Spectrum1DList&) = delete;
  Spectrum1DList& operator=(const Spectrum1DList&) = delete;

  // Enable moving
  Spectrum1DList(Spectrum1DList&& other) noexcept;
  Spectrum1DList& operator=(Spectrum1DList&& other) noexcept;

  // Static constructors
  static Spectrum1DList create();
  static Spectrum1DList create_from_array(const std::vector<Spectrum1D>& spectra);

  // Accessors
  size_t size() const;
  Spectrum1D get(size_t index) const;
  void set(size_t index, const Spectrum1D& spectrum);
  Spectrum1D unset(size_t index);

  // Operations
  Spectrum1DList duplicate() const;

  // Get raw pointer
  hdrl_spectrum1Dlist* ptr() { return m_list; }
  const hdrl_spectrum1Dlist* ptr() const { return m_list; }

#if defined HDRL_USE_EXPERIMENTAL || defined HDRL_USE_PRIVATE
  CollapseResult collapse(const Parameter& stacking_par,
                         const std::vector<double>& wavelengths,
                         const Parameter& resample_par,
                         bool mark_bpm_in_interpolation) const;
#endif

 private:
  hdrl_spectrum1Dlist* m_list;
};

}  // namespace core
}  // namespace hdrl

#endif  // PYHDRL_CORE_SPECTRUM1D_HPP_



