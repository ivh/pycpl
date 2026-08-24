// This file is part of the PyHDRL Python language bindings
// Copyright (C) 2023-2026 European Southern Observatory
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

#include <array>
#include <cstring>
#include <filesystem>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <cpl_array.h>
#include <cpl_image.h>
#include <hdrl_correlation.h>
#include <hdrl_image.h>
#include <hdrl_imagelist.h>
#include <hdrl_parameter.h>
#include <hdrl_spectrum_defs.h>
#include <hdrl_spectrum_resample.h>
#include <hdrl_spectrum_shift.h>
#include <hdrl_spectrumlist.h>
#include <hdrl_types.h>

#include "hdrlcore/imagelist.hpp"
#include "hdrlcore/parameter.hpp"

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

  hdrl_xcorrelation_result* ptr() const { return m_result; }

 private:
  double m_shift;
  double m_error;
  double m_quality;
  hdrl_xcorrelation_result* m_result;
};

class Spectrum1D
{
 public:
  // --- Constructors/Destructors ---
  Spectrum1D();
  explicit Spectrum1D(hdrl_spectrum1D* spectrum);
  Spectrum1D(const hdrl_spectrum1D* spectrum);
  Spectrum1D(const cpl_image* flux, cpl_size half_window,
             const cpl_array* wavelengths, WaveScale scale);
  Spectrum1D(const cpl_image* flux, const cpl_image* flux_error,
             const cpl_array* wavelengths, WaveScale scale);
  Spectrum1D(std::function<hdrl_value(double)> func,
             const cpl_array* wavelengths, WaveScale scale);
  ~Spectrum1D();

  typedef typename std::size_t size_type;

  // Disable copying
  Spectrum1D(const Spectrum1D&) = delete;
  Spectrum1D& operator=(const Spectrum1D&) = delete;

  // Enable moving
  Spectrum1D(Spectrum1D&& other) noexcept;
  Spectrum1D& operator=(Spectrum1D&& other) noexcept;

  // --- Core Methods ---
  size_type get_size() const;
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
  XCorrelationResult
  compute_shift_xcorrelation(const Spectrum1D& other, size_type half_win,
                             bool normalize = true) const;

  double compute_shift_fit(double wguess, std::array<double, 2> wrange,
                           std::array<double, 2> frange, double half_win);

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
  Spectrum1D select_window(double lambda_min, double lambda_max,
                           bool is_internal = false) const;

  // --- Spectrum Resample Operations ---
  Spectrum1D
  resample(const Spectrum1D& other,
           InterpolationMethod method = InterpolationMethod::AKIMA) const;
  Spectrum1D resample_to_wavelengths(
      const std::vector<double>& wavelengths,
      InterpolationMethod method = InterpolationMethod::AKIMA) const;
  Spectrum1D
  resample_fit(const std::vector<double>& wavelengths, int k, int nCoeff) const;
  Spectrum1D
  resample_windowed_fit(const std::vector<double>& wavelengths, int k,
                        int nCoeff, long window, double factor) const;
  Spectrum1D resample_integrate(const std::vector<double>& wavelengths) const;

  // --- I/O ---
  void save(const std::filesystem::path& filename) const;

  // --- Data Access ---
  std::vector<double> get_flux_vector() const;
  std::vector<double> get_wavelength_vector() const;
  std::vector<double> get_flux_error_vector() const;
  std::vector<int> get_bad_pixel_map() const;

  // --- Utility Methods ---
  bool is_compatible_with(const Spectrum1D& other) const;
  std::shared_ptr<Spectrum1D> duplicate() const;
  Spectrum1D reject_pixels(const std::vector<int>& bad_samples) const;
  std::pair<bool, double> is_uniformly_sampled() const;

  // --- Raw Access (for bindings) ---
  hdrl_spectrum1D* ptr() { return m_interface; }

  const hdrl_spectrum1D* ptr() const { return m_interface; }

 private:
  hdrl_spectrum1D* m_interface;

  // Helper method for single point resampling
  Spectrum1D handle_single_point_resample(double target_wavelength) const;
};

struct CollapseResult
{
  std::shared_ptr<Spectrum1D> result;
  std::shared_ptr<ImageList> aligned_images;
  pycpl_image contrib;
};

// Wrapper class for hdrl_spectrum1Dlist
class Spectrum1DList
{
 public:
  // Constructors/Destructors
  Spectrum1DList();
  explicit Spectrum1DList(hdrl_spectrum1Dlist* list);
  Spectrum1DList(std::vector<std::shared_ptr<Spectrum1D>> spectra);
  ~Spectrum1DList();

  typedef typename std::size_t size_type;

  // Disable copying
  Spectrum1DList(const Spectrum1DList&) = delete;
  Spectrum1DList& operator=(const Spectrum1DList&) = delete;

  // Enable moving
  Spectrum1DList(Spectrum1DList&& other) noexcept;
  Spectrum1DList& operator=(Spectrum1DList&& other) noexcept;

  std::shared_ptr<Spectrum1D> operator[](size_type index) const;

  // Accessors
  size_type get_size() const;
  std::shared_ptr<Spectrum1D> get_at(size_type index) const;
  void set(std::shared_ptr<Spectrum1D> spectrum, size_type index);
  void append(std::shared_ptr<Spectrum1D> spectrum);
  std::shared_ptr<Spectrum1D> pop(size_type index);

  // Operations
  std::shared_ptr<Spectrum1DList> duplicate() const;

  // Get raw pointer
  hdrl_spectrum1Dlist* ptr() { return m_list; }

  const hdrl_spectrum1Dlist* ptr() const { return m_list; }

  CollapseResult
  collapse(const Parameter& stacking_par,
           const std::vector<double>& wavelengths,
           const Parameter& resample_par, bool mark_bpm_in_interpolation) const;

 private:
  std::vector<std::shared_ptr<Spectrum1D>> m_spectra;
  hdrl_spectrum1Dlist* m_list;
};

}  // namespace core
}  // namespace hdrl

#endif  // PYHDRL_CORE_SPECTRUM1D_HPP_
