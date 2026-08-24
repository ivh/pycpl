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

#ifndef PYHDRL_FUNC_RESPONSE_HPP_
#define PYHDRL_FUNC_RESPONSE_HPP_

#include <memory>
#include <stdexcept>
#include <tuple>

#include "cpl_array.h"
#include "cpl_bivector.h"
#include "hdrl_parameter.h"
#include "pybind11/numpy.h"

#include "hdrl_response.h"
#include "hdrlcore/spectrum.hpp"

namespace py = pybind11;

// Helper function declarations
cpl_array* python_array_to_cpl_array(py::array array);
cpl_bivector* python_tuple_to_cpl_bivector(py::tuple tuple);

namespace hdrl
{
namespace func
{

class ResponseVelocityParameter
{
 public:
  ResponseVelocityParameter() = default;
  ResponseVelocityParameter(double wguess, double range_wmin, double range_wmax,
                            double fit_wmin, double fit_wmax,
                            size_t fit_half_win);
  hdrl_parameter* ptr();
  const hdrl_parameter* ptr() const;

 protected:
  hdrl_parameter* m_interface = nullptr;
};

class ResponseCalcParameter
{
 public:
  ResponseCalcParameter(double Ap, double Am, double G, double Tex);
  hdrl_parameter* ptr();
  const hdrl_parameter* ptr() const;

 protected:
  hdrl_parameter* m_interface = nullptr;
};

class ResponseTelluricParameter
{
 public:
  ResponseTelluricParameter() = default;
  ResponseTelluricParameter(const hdrl::core::Spectrum1DList& telluric_models,
                            double w_step, size_t half_win, bool normalize,
                            bool shift_in_log_scale, py::tuple quality_areas,
                            py::tuple fit_areas, double lmin, double lmax);
  hdrl_parameter* ptr();
  const hdrl_parameter* ptr() const;

 protected:
  hdrl_parameter* m_interface = nullptr;
};

class ResponseFitParameter
{
 public:
  ResponseFitParameter(size_t radius, py::array fit_points, double wrange,
                       py::tuple high_abs_regions);
  hdrl_parameter* ptr();
  const hdrl_parameter* ptr() const;

 protected:
  hdrl_parameter* m_interface = nullptr;
};

class ResponseResult
{
 public:
  ResponseResult(hdrl_response_result* result);
  ~ResponseResult() = default;

  ResponseResult(ResponseResult&&) = default;
  ResponseResult& operator=(ResponseResult&&) = default;

  ResponseResult(const ResponseResult&) = delete;
  ResponseResult& operator=(const ResponseResult&) = delete;

  const hdrl::core::Spectrum1D& get_final_response() const;
  const hdrl::core::Spectrum1D& get_selected_response() const;
  const hdrl::core::Spectrum1D& get_raw_response() const;
  const hdrl::core::Spectrum1D& get_corrected_obs_spectrum() const;
  size_t get_best_telluric_model_idx() const;
  double get_avg_diff_from_1() const;
  double get_stddev() const;
  double get_telluric_shift() const;
  double get_doppler_shift() const;

  // Add __repr__ method for better string representation
  std::string repr() const
  {
    try {
      return "ResponseResult(best_telluric_model_idx=" +
             std::to_string(get_best_telluric_model_idx()) +
             ", avg_diff_from_1=" + std::to_string(get_avg_diff_from_1()) +
             ", stddev=" + std::to_string(get_stddev()) +
             ", telluric_shift=" + std::to_string(get_telluric_shift()) +
             ", doppler_shift=" + std::to_string(get_doppler_shift()) + ")";
    }
    catch (const std::exception&) {
      return "ResponseResult(invalid)";
    }
  }

 private:
  std::unique_ptr<hdrl_response_result, void (*)(hdrl_response_result*)>
      result_;
  hdrl::core::Spectrum1D final_response_;
  hdrl::core::Spectrum1D selected_response_;
  hdrl::core::Spectrum1D raw_response_;
  hdrl::core::Spectrum1D corrected_obs_spectrum_;
};

class Response
{
 public:
  // Parameter creation methods
  static ResponseVelocityParameter
  velocity_parameter_create(double wguess, double range_wmin, double range_wmax,
                            double fit_wmin, double fit_wmax,
                            size_t fit_half_win);

  static ResponseCalcParameter
  calc_parameter_create(double Ap, double Am, double G, double Tex);

  static ResponseTelluricParameter telluric_evaluation_parameter_create(
      const hdrl::core::Spectrum1DList& telluric_models, double w_step,
      size_t half_win, bool normalize, bool shift_in_log_scale,
      py::tuple quality_areas, py::tuple fit_areas, double lmin, double lmax);

  static ResponseFitParameter
  fit_parameter_create(size_t radius, py::array fit_points, double wrange,
                       py::tuple high_abs_regions);

  // Main computation method
  static ResponseResult compute(const hdrl::core::Spectrum1D& obs_s,
                                const hdrl::core::Spectrum1D& ref_s,
                                const hdrl::core::Spectrum1D& E_x,
                                const ResponseTelluricParameter& telluric_par,
                                const ResponseVelocityParameter& velocity_par,
                                const ResponseCalcParameter& calc_par,
                                const ResponseFitParameter& fit_par);

  // Wrapper methods
  static std::tuple<hdrl::core::Spectrum1D, double, double, double, int>
  evaluate_telluric_models(const hdrl::core::Spectrum1D& obs_s,
                           const ResponseTelluricParameter& telluric_par);

  // Helper method to create a response result from raw pointer
  static ResponseResult create_response_result(hdrl_response_result* result)
  {
    if (!result) {
      throw std::runtime_error("Null response result provided");
    }
    return ResponseResult(result);
  }

  // Helper method to validate input spectra
  static void validate_input_spectra(const hdrl::core::Spectrum1D& obs_s,
                                     const hdrl::core::Spectrum1D& ref_s,
                                     const hdrl::core::Spectrum1D& E_x)
  {
    if (!obs_s.ptr()) {
      throw std::runtime_error("Observed spectrum is null");
    }
    if (!ref_s.ptr()) {
      throw std::runtime_error("Reference spectrum is null");
    }
    if (!E_x.ptr()) {
      throw std::runtime_error("Efficiency spectrum is null");
    }

    if (obs_s.get_size() == 0 || ref_s.get_size() == 0 || E_x.get_size() == 0) {
      throw std::runtime_error("One or more spectra are empty");
    }
  }
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_RESPONSE_HPP_
