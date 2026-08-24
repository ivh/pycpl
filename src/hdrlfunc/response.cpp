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

#include "hdrlfunc/response.hpp"

#include <memory>
#include <tuple>

#include "cpl_error.h"
#include "hdrl_efficiency.h"

#include "hdrlcore/error.hpp"

namespace hdrl
{
namespace func
{

using hdrl::core::Error;
using hdrl::core::IllegalInputError;
using hdrl::core::IllegalOutputError;
using hdrl::core::InvalidTypeError;

// Helper function to convert a Python array to a cpl_array
cpl_array*
python_array_to_cpl_array(py::array array)
{
  py::buffer_info buf = array.request();

  if (buf.ptr == nullptr) {
    throw InvalidTypeError(
        HDRL_ERROR_LOCATION,
        "Unable to obtain a buffer descriptor from the Python array");
  }
  cpl_errorstate prestate = cpl_errorstate_get();
  cpl_array* cpl_arr = nullptr;

  if (buf.format == py::format_descriptor<double>::format()) {
    double* data = static_cast<double*>(buf.ptr);
    cpl_arr = cpl_array_wrap_double(data, static_cast<cpl_size>(buf.shape[0]));
  } else if (buf.format == py::format_descriptor<float>::format()) {
    float* data = static_cast<float*>(buf.ptr);
    cpl_arr = cpl_array_wrap_float(data, static_cast<cpl_size>(buf.shape[0]));
  } else if (buf.format == py::format_descriptor<int>::format()) {
    int* data = static_cast<int*>(buf.ptr);
    cpl_arr = cpl_array_wrap_int(data, static_cast<cpl_size>(buf.shape[0]));
  } else {
    throw InvalidTypeError(HDRL_ERROR_LOCATION, "Unsupported array data type");
  }
  hdrl::core::Error::throw_errors_after(prestate);
  return cpl_arr;
}

// Helper function to convert a Python tuple of arrays to a cpl_bivector
cpl_bivector*
python_tuple_to_cpl_bivector(py::tuple tuple)
{
  if (py::len(tuple) != 2) {
    throw IllegalInputError(HDRL_ERROR_LOCATION,
                            "Expected a tuple of two arrays");
  }

  py::array x_array = py::cast<py::array>(tuple[0]);
  py::array y_array = py::cast<py::array>(tuple[1]);

  py::buffer_info x_buf = x_array.request();
  py::buffer_info y_buf = y_array.request();

  if (x_buf.ptr == nullptr || y_buf.ptr == nullptr) {
    throw IllegalInputError(
        HDRL_ERROR_LOCATION,
        "Unable to obtain a buffer descriptor from the Python arrays");
  }

  if (x_buf.shape[0] != y_buf.shape[0]) {
    throw IllegalInputError(HDRL_ERROR_LOCATION,
                            "Arrays in the tuple must be of the same size");
  }

  if (x_buf.format != py::format_descriptor<double>::format() ||
      y_buf.format != py::format_descriptor<double>::format()) {
    throw IllegalInputError(HDRL_ERROR_LOCATION,
                            "Arrays must be of type double");
  }

  double* x_data = static_cast<double*>(x_buf.ptr);
  double* y_data = static_cast<double*>(y_buf.ptr);
  cpl_bivector* bivec = hdrl::core::Error::throw_errors_with(
      cpl_bivector_new, static_cast<cpl_size>(x_buf.shape[0]));
  if (!bivec) {
    throw IllegalInputError(HDRL_ERROR_LOCATION, "Failed to create bivector");
  }

  cpl_vector* x_vec =
      hdrl::core::Error::throw_errors_with(cpl_bivector_get_x, bivec);
  cpl_vector* y_vec =
      hdrl::core::Error::throw_errors_with(cpl_bivector_get_y, bivec);

  for (size_t i = 0; i < static_cast<size_t>(x_buf.shape[0]); ++i) {
    cpl_error_code err_x = cpl_vector_set(x_vec, i, x_data[i]);
    cpl_error_code err_y = cpl_vector_set(y_vec, i, y_data[i]);

    if (err_x != CPL_ERROR_NONE || err_y != CPL_ERROR_NONE) {
      hdrl::core::Error::throw_errors_with(cpl_vector_delete, x_vec);
      hdrl::core::Error::throw_errors_with(cpl_vector_delete, y_vec);
      hdrl::core::Error::throw_errors_with(cpl_bivector_delete, bivec);
      throw IllegalInputError(HDRL_ERROR_LOCATION,
                              "Failed to set vector values");
    }
  }

  return bivec;
}

// ResponseResult constructor and methods
ResponseResult::ResponseResult(hdrl_response_result* result)
    : result_(result, hdrl_response_result_delete),
      final_response_(hdrl::core::Error::throw_errors_with(
          hdrl_spectrum1D_duplicate,
          hdrl_response_result_get_final_response(result))),
      selected_response_(hdrl::core::Error::throw_errors_with(
          hdrl_spectrum1D_duplicate,
          hdrl_response_result_get_selected_response(result))),
      raw_response_(hdrl::core::Error::throw_errors_with(
          hdrl_spectrum1D_duplicate,
          hdrl_response_result_get_raw_response(result))),
      corrected_obs_spectrum_(hdrl::core::Error::throw_errors_with(
          hdrl_spectrum1D_duplicate,
          hdrl_response_result_get_corrected_obs_spectrum(result)))
{
  if (!result) {
    throw IllegalInputError(HDRL_ERROR_LOCATION,
                            "Null response result provided");
  }

  // Check for null spectra
  if (!final_response_.ptr() || !selected_response_.ptr() ||
      !raw_response_.ptr() || !corrected_obs_spectrum_.ptr()) {
    throw IllegalOutputError(HDRL_ERROR_LOCATION,
                             "ResponseResult contains null spectrum data");
  }
}

const hdrl::core::Spectrum1D&
ResponseResult::get_final_response() const
{
  return final_response_;
}

const hdrl::core::Spectrum1D&
ResponseResult::get_selected_response() const
{
  return selected_response_;
}

const hdrl::core::Spectrum1D&
ResponseResult::get_raw_response() const
{
  return raw_response_;
}

const hdrl::core::Spectrum1D&
ResponseResult::get_corrected_obs_spectrum() const
{
  return corrected_obs_spectrum_;
}

size_t
ResponseResult::get_best_telluric_model_idx() const
{
  return hdrl_response_result_get_best_telluric_model_idx(result_.get());
}

double
ResponseResult::get_avg_diff_from_1() const
{
  return hdrl_response_result_get_avg_diff_from_1(result_.get());
}

double
ResponseResult::get_stddev() const
{
  return hdrl_response_result_get_stddev(result_.get());
}

double
ResponseResult::get_telluric_shift() const
{
  return hdrl_response_result_get_telluric_shift(result_.get());
}

double
ResponseResult::get_doppler_shift() const
{
  return hdrl_response_result_get_doppler_shift(result_.get());
}

hdrl_parameter*
ResponseVelocityParameter::ptr()
{
  return m_interface;
}

const hdrl_parameter*
ResponseVelocityParameter::ptr() const
{
  return m_interface;
}

ResponseVelocityParameter::ResponseVelocityParameter(
    double wguess, double range_wmin, double range_wmax, double fit_wmin,
    double fit_wmax, size_t fit_half_win)
{
  if (range_wmin >= range_wmax) {
    throw IllegalInputError(HDRL_ERROR_LOCATION,
                            "range_wmin must be less than range_wmax");
  }

  if (fit_wmin >= fit_wmax) {
    throw IllegalInputError(HDRL_ERROR_LOCATION,
                            "fit_wmin must be less than fit_wmax");
  }

  if (fit_half_win == 0) {
    throw IllegalInputError(HDRL_ERROR_LOCATION,
                            "fit_half_win must be greater than zero");
  }

  m_interface = Error::throw_errors_with(
      hdrl_spectrum1D_shift_fit_parameter_create, wguess, range_wmin,
      range_wmax, fit_wmin, fit_wmax, fit_half_win);
}

hdrl_parameter*
ResponseCalcParameter::ptr()
{
  return m_interface;
}

const hdrl_parameter*
ResponseCalcParameter::ptr() const
{
  return m_interface;
}

ResponseCalcParameter::ResponseCalcParameter(double Ap, double Am, double G,
                                             double Tex)
{
  if (Ap <= 0.0 || Am <= 0.0 || G <= 0.0 || Tex <= 0.0) {
    throw IllegalInputError(HDRL_ERROR_LOCATION,
                            "All parameters must be positive");
  }

  hdrl_value ap_val = {Ap, 0.0};
  hdrl_value am_val = {Am, 0.0};
  hdrl_value g_val = {G, 0.0};
  hdrl_value tex_val = {Tex, 0.0};
  hdrl_value atel_val = {0.0, 0.0};

  m_interface =
      Error::throw_errors_with(hdrl_efficiency_parameter_create, ap_val, am_val,
                               g_val, tex_val, atel_val);
}

hdrl_parameter*
ResponseTelluricParameter::ptr()
{
  return m_interface;
}

const hdrl_parameter*
ResponseTelluricParameter::ptr() const
{
  return m_interface;
}

ResponseTelluricParameter::ResponseTelluricParameter(
    const hdrl::core::Spectrum1DList& telluric_models, double w_step,
    size_t half_win, bool normalize, bool shift_in_log_scale,
    py::tuple quality_areas, py::tuple fit_areas, double lmin, double lmax)
{
  cpl_boolean cpl_normalize = normalize ? CPL_TRUE : CPL_FALSE;
  cpl_boolean cpl_shift_in_log_scale =
      shift_in_log_scale ? CPL_TRUE : CPL_FALSE;

  m_interface = Error::throw_errors_with(
      hdrl_response_telluric_evaluation_parameter_create, telluric_models.ptr(),
      w_step, half_win, cpl_normalize, cpl_shift_in_log_scale,
      python_tuple_to_cpl_bivector(quality_areas),
      python_tuple_to_cpl_bivector(fit_areas), lmin, lmax);
}

hdrl_parameter*
ResponseFitParameter::ptr()
{
  return m_interface;
}

const hdrl_parameter*
ResponseFitParameter::ptr() const
{
  return m_interface;
}

ResponseFitParameter::ResponseFitParameter(size_t radius, py::array fit_points,
                                           double wrange,
                                           py::tuple high_abs_regions)
{
  m_interface =
      Error::throw_errors_with(hdrl_response_fit_parameter_create, radius,
                               python_array_to_cpl_array(fit_points), wrange,
                               python_tuple_to_cpl_bivector(high_abs_regions));
}

ResponseVelocityParameter
Response::velocity_parameter_create(double wguess, double range_wmin,
                                    double range_wmax, double fit_wmin,
                                    double fit_wmax, size_t fit_half_win)
{
  return ResponseVelocityParameter(wguess, range_wmin, range_wmax, fit_wmin,
                                   fit_wmax, fit_half_win);
}

ResponseCalcParameter
Response::calc_parameter_create(double Ap, double Am, double G, double Tex)
{
  return ResponseCalcParameter(Ap, Am, G, Tex);
}

ResponseTelluricParameter
Response::telluric_evaluation_parameter_create(
    const hdrl::core::Spectrum1DList& telluric_models, double w_step,
    size_t half_win, bool normalize, bool shift_in_log_scale,
    py::tuple quality_areas, py::tuple fit_areas, double lmin, double lmax)
{
  return ResponseTelluricParameter(telluric_models, w_step, half_win, normalize,
                                   shift_in_log_scale, quality_areas, fit_areas,
                                   lmin, lmax);
}

ResponseFitParameter
Response::fit_parameter_create(size_t radius, py::array fit_points,
                               double wrange, py::tuple high_abs_regions)
{
  return ResponseFitParameter(radius, fit_points, wrange, high_abs_regions);
}

std::tuple<hdrl::core::Spectrum1D, double, double, double, int>
hdrl::func::Response::evaluate_telluric_models(
    const hdrl::core::Spectrum1D& obs_s,
    const ResponseTelluricParameter& telluric_par)
{
  hdrl_data_t best_mean_minus1 = 0, best_stddev = 0;
  hdrl_data_t best_telluric_shift = 0;
  cpl_size best_idx = -1;

  // Validate inputs
  if (!obs_s.ptr()) {
    throw IllegalInputError(HDRL_ERROR_LOCATION, "Observed spectrum is null");
  }

  if (!telluric_par.ptr()) {
    throw IllegalInputError(HDRL_ERROR_LOCATION, "Telluric parameter is null");
  }

  // Call the HDRL function
  hdrl_spectrum1D* corrected_obs = Error::throw_errors_with(
      hdrl_response_evaluate_telluric_models, obs_s.ptr(), telluric_par.ptr(),
      &best_telluric_shift, &best_mean_minus1, &best_stddev, &best_idx);

  // Check if the result is valid
  if (corrected_obs == nullptr) {
    throw IllegalOutputError(HDRL_ERROR_LOCATION,
                             "HDRL evaluation returned null result");
  }

  // Check if the best_idx is valid
  if (best_idx == static_cast<cpl_size>(-1)) {
    hdrl_spectrum1D_delete(&corrected_obs);
    throw IllegalOutputError(HDRL_ERROR_LOCATION,
                             "HDRL evaluation returned invalid index");
  }

  // Return the result
  return std::make_tuple(hdrl::core::Spectrum1D(corrected_obs),
                         best_telluric_shift, best_mean_minus1, best_stddev,
                         static_cast<int>(best_idx));
}

ResponseResult
Response::compute(const hdrl::core::Spectrum1D& obs_s,
                  const hdrl::core::Spectrum1D& ref_s,
                  const hdrl::core::Spectrum1D& E_x,
                  const ResponseTelluricParameter& telluric_par,
                  const ResponseVelocityParameter& velocity_par,
                  const ResponseCalcParameter& calc_par,
                  const ResponseFitParameter& fit_par)
{
  // Check for null pointers
  if (!obs_s.ptr()) {
    throw IllegalInputError(HDRL_ERROR_LOCATION, "Observed spectrum is null");
  }

  if (!ref_s.ptr()) {
    throw IllegalInputError(HDRL_ERROR_LOCATION, "Reference spectrum is null");
  }

  if (!E_x.ptr()) {
    throw IllegalInputError(HDRL_ERROR_LOCATION, "Efficiency spectrum is null");
  }

  if (!calc_par.ptr()) {
    throw IllegalInputError(HDRL_ERROR_LOCATION,
                            "Calculation parameter is null");
  }

  if (!fit_par.ptr()) {
    throw IllegalInputError(HDRL_ERROR_LOCATION, "Fit parameter is null");
  }

  // Call the HDRL function
  hdrl_response_result* result = Error::throw_errors_with(
      hdrl_response_compute, obs_s.ptr(), ref_s.ptr(), E_x.ptr(),
      telluric_par.ptr(), velocity_par.ptr(), calc_par.ptr(), fit_par.ptr());

  if (!result) {
    throw IllegalOutputError(HDRL_ERROR_LOCATION,
                             "Response computation returned null result");
  }

  return ResponseResult(result);
}

}  // namespace func
}  // namespace hdrl
