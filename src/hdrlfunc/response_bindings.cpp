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

#include "hdrlfunc/response_bindings.hpp"

#include <memory>

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "hdrlcore/error.hpp"
#include "hdrlfunc/response.hpp"

namespace py = pybind11;

void
bind_response(py::module_& m)
{
  py::class_<hdrl::func::ResponseVelocityParameter,
             std::shared_ptr<hdrl::func::ResponseVelocityParameter>>(
      m, "ResponseVelocityParameter")
      .def(py::init<>())
      .def(py::init<double, double, double, double, double, size_t>(),
           py::arg("wguess"), py::arg("range_wmin"), py::arg("range_wmax"),
           py::arg("fit_wmin"), py::arg("fit_wmax"), py::arg("fit_half_win"),
           R"docstring(
           Constructor for Response Velocity parameter.

           Parameters
           ----------
           wguess : float
               Reference line wavelength position.
           range_wmin : float
               Minimum of wavelength box for line fit.
           range_wmax : float
               Maximum of wavelength box for line fit.
           fit_wmin : float
               Minimum wavelength value used to fit line slope.
           fit_wmax : float
               Maximum wavelength value used to fit line slope.
           fit_half_win : int
               Half box where polynomial fit is performed.
           )docstring");

  py::class_<hdrl::func::ResponseCalcParameter,
             std::shared_ptr<hdrl::func::ResponseCalcParameter>>(
      m, "ResponseCalcParameter")
      .def(py::init<double, double, double, double>(), py::arg("Ap"),
           py::arg("Am"), py::arg("G"), py::arg("Tex"),
           R"docstring(
           Constructor for Response Calc parameter.

           Parameters
           ----------
           Ap : float
               Parameter to indicate if the efficiency is computed at
               airmass = 0, or at a given non zero value.
           Am : float
               Airmass at which the std star was observed.
           G : float
               Gain [ADU/e].
           Tex : float
               Exposure time [s].
           )docstring");

  py::class_<hdrl::func::ResponseTelluricParameter,
             std::shared_ptr<hdrl::func::ResponseTelluricParameter>>(
      m, "ResponseTelluricParameter")
      .def(py::init<>())
      .def(py::init<const hdrl::core::Spectrum1DList&, double, size_t, bool,
                    bool, py::tuple, py::tuple, double, double>(),
           py::arg("telluric_models"), py::arg("w_step"), py::arg("half_win"),
           py::arg("normalize"), py::arg("shift_in_log_scale"),
           py::arg("quality_areas"), py::arg("fit_areas"), py::arg("lmin"),
           py::arg("lmax"),
           R"docstring(
           Constructor for Response Telluric parameter.

           Parameters
           ----------
           telluric_models : hdrl.core.Spectrum1DList
               The available telluric models.
           w_step : float
               Sampling step to use when upsampling model and observed spectrum
               to calculate the cross correlations.
           half_win : int
               Half the search window to be used to find the peak of the cross
               correlation.
           normalize : boolean
               True if the cross correlation should be normalized, False
               otherwise.
           shift_in_log_scale : boolean
               True if the cross correlation has to be calculated in
               logarithmic scale, False otherwise.
           quality_areas : tuple (array of float, array of float)
               Areas where the quality of the fit of the telluric model has to
               be evaluated.
           fit_areas : tuple (array of float, array of float)
               Areas where the median points are extracted from, in order to
               generate the final quality parameters of the telluric model.
           lmin : float
               Minimum wavelength used to calculate the cross-correlation (in
               log scale if shift_in_log_scale = TRUE).
           lmax : float
               Maximum wavelength used to calculate the cross-correlation (in
               log scale if shift_in_log_scale = TRUE).
           )docstring");

  py::class_<hdrl::func::ResponseFitParameter,
             std::shared_ptr<hdrl::func::ResponseFitParameter>>(
      m, "ResponseFitParameter")
      .def(py::init<size_t, py::array, double, py::tuple>(), py::arg("radius"),
           py::arg("fit_points"), py::arg("wrange"),
           py::arg("high_abs_regions"),
           R"docstring(
           Constructor for the hdrl_parameter for the final interpolation of the response.

           Parameters
           ----------
           radius : int
               Radius of the median filter used to smooth the response before
               the final interpolation
           fit_points : array
               Median points where the fit will be calculated.
           wrange : float
               Range around the median point where the median is calculated.
           high_abs_regions : tuple (array of float, array of float)
               High absorption regions that should be skipped when calculating
               the fit. If NULL no skipping is done.
           )docstring");

  // Bind ResponseResult class
  py::class_<hdrl::func::ResponseResult,
             std::shared_ptr<hdrl::func::ResponseResult>>(m, "ResponseResult")
      .def(
          "get_final_response",
          [](const hdrl::func::ResponseResult& self) {
            // Get the raw pointer and create a new Spectrum1D from it
            const hdrl_spectrum1D* raw_ptr = self.get_final_response().ptr();
            if (!raw_ptr) {
              throw py::value_error("Null final response spectrum");
            }
            return std::make_shared<hdrl::core::Spectrum1D>(
                hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_duplicate,
                                                     raw_ptr));
          },
          R"docstring(
           Get the final product of response calculations.

           Returns
           -------
           hdrl.core.Spectrum1D
               Copy of the response spectrum.
           )docstring")
      .def(
          "get_selected_response",
          [](const hdrl::func::ResponseResult& self) {
            const hdrl_spectrum1D* raw_ptr = self.get_selected_response().ptr();
            if (!raw_ptr) {
              throw py::value_error("Null selected response spectrum");
            }
            return std::make_shared<hdrl::core::Spectrum1D>(
                hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_duplicate,
                                                     raw_ptr));
          },
          R"docstring(
           Get the selected response. The selected response is
           the raw response sampled in the fit points. This response is
           going then to be interpolated, creating the final response.

           Returns
           -------
           hdrl.core.Spectrum1D
               Copy of the selected response.
           )docstring")
      .def(
          "get_raw_response",
          [](const hdrl::func::ResponseResult& self) {
            const hdrl_spectrum1D* raw_ptr = self.get_raw_response().ptr();
            if (!raw_ptr) {
              throw py::value_error("Null raw response spectrum");
            }
            return std::make_shared<hdrl::core::Spectrum1D>(
                hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_duplicate,
                                                     raw_ptr));
          },
          R"docstring(
           Get the raw response. The raw response is the ratio between the
           observed spectrum and the reference one, corrected for e.g. gain,
           atmospheric extinction, etc.

           Returns
           -------
           hdrl.core.Spectrum1D
               Copy of the raw response.
           )docstring")
      .def(
          "get_corrected_obs_spectrum",
          [](const hdrl::func::ResponseResult& self) {
            const hdrl_spectrum1D* raw_ptr =
                self.get_corrected_obs_spectrum().ptr();
            if (!raw_ptr) {
              throw py::value_error("Null corrected observation spectrum");
            }
            return std::make_shared<hdrl::core::Spectrum1D>(
                hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_duplicate,
                                                     raw_ptr));
          },
          R"docstring(
           Get the the corrected observed spectrum.

           Returns
           -------
           hdrl.core.Spectrum1D
               The observed spectrum corrected by the telluric model.
           )docstring")
      .def("get_best_telluric_model_idx",
           &hdrl::func::ResponseResult::get_best_telluric_model_idx,
           R"docstring(
           Get the index of the telluric model used for telluric correction.

           Returns
           -------
           int
               The index.
           )docstring")
      .def("get_avg_diff_from_1",
           &hdrl::func::ResponseResult::get_avg_diff_from_1,
           R"docstring(
           Get the value |mean - 1|, where mean is the average of the ratio
           between the corrected observed spectrum and its smoothed fit.
           This value can be used to assess the quality of the match of the
           telluric model with the provided observed spectrum.

           Returns
           -------
           float
               The mean value.
           )docstring")
      .def("get_stddev", &hdrl::func::ResponseResult::get_stddev,
           R"docstring(
           Get the standard deviation of the ratio between the corrected
           observed spectrum and its smoothed fit.
           This value can be used to assess the quality of the match of the
           telluric model with the provided observed spectrum.

           Returns
           -------
           float
               The standard deviation value.
           )docstring")
      .def("get_telluric_shift",
           &hdrl::func::ResponseResult::get_telluric_shift,
           R"docstring(
           Get the shift applied to the telluric model.
           This value can be used to assess the quality of the match of the
           telluric model with the provided observed spectrum.

           Returns
           -------
           float
               The value of shift.
           )docstring")
      .def("get_doppler_shift", &hdrl::func::ResponseResult::get_doppler_shift,
           R"docstring(
           Get the doppler shift used to correct the model.

           Returns
           -------
           float
               The value of doppler shift.
           )docstring")
      .def("__repr__",
           [](const hdrl::func::ResponseResult& self) -> std::string {
             try {
               return "ResponseResult(best_telluric_model_idx=" +
                      std::to_string(self.get_best_telluric_model_idx()) +
                      ", avg_diff_from_1=" +
                      std::to_string(self.get_avg_diff_from_1()) +
                      ", stddev=" + std::to_string(self.get_stddev()) +
                      ", telluric_shift=" +
                      std::to_string(self.get_telluric_shift()) +
                      ", doppler_shift=" +
                      std::to_string(self.get_doppler_shift()) + ")";
             }
             catch (const std::exception&) {
               return "ResponseResult(invalid)";
             }
           });

  // Bind Response class
  py::class_<hdrl::func::Response>(m, "Response")
      .def_static(
          "velocity_parameter_create",
          [](double wguess, double range_wmin, double range_wmax,
             double fit_wmin, double fit_wmax, size_t fit_half_win) {
            return std::make_shared<hdrl::func::ResponseVelocityParameter>(
                hdrl::func::Response::velocity_parameter_create(
                    wguess, range_wmin, range_wmax, fit_wmin, fit_wmax,
                    fit_half_win));
          },
          py::arg("wguess"), py::arg("range_wmin"), py::arg("range_wmax"),
          py::arg("fit_wmin"), py::arg("fit_wmax"), py::arg("fit_half_win"),
          R"docstring(
          Constructor for Response Velocity parameter.

          Parameters
          ----------
          wguess : float
              Reference line wavelength position.
          range_wmin : float
              Minimum of wavelength box for line fit.
          range_wmax : float
              Maximum of wavelength box for line fit.
          fit_wmin : float
              Minimum wavelength value used to fit line slope.
          fit_wmax : float
              Maximum wavelength value used to fit line slope.
          fit_half_win : int
              Half box where polynomial fit is performed.

          Returns
          -------
          hdrl.func.ResponseVelocityParameter
          )docstring")

      .def_static(
          "calc_parameter_create",
          [](double Ap, double Am, double G, double Tex) {
            return std::make_shared<hdrl::func::ResponseCalcParameter>(
                hdrl::func::Response::calc_parameter_create(Ap, Am, G, Tex));
          },
          py::arg("Ap"), py::arg("Am"), py::arg("G"), py::arg("Tex"),
          R"docstring(
          Constructor for Response Calc parameter.

          Parameters
          ----------
          Ap : float
              Parameter to indicate if the efficiency is computed at
              airmass = 0, or at a given non zero value.
          Am : float
              Airmass at which the std star was observed.
          G : float
              Gain [ADU/e].
          Tex : float
              Exposure time [s].

          Returns
          -------
          hdrl.func.ResponseCalcParameter
          )docstring")

      .def_static(
          "telluric_evaluation_parameter_create",
          [](const std::shared_ptr<hdrl::core::Spectrum1DList>& telluric_models,
             double w_step, size_t half_win, bool normalize,
             bool shift_in_log_scale, py::tuple quality_areas,
             py::tuple fit_areas, double lmin, double lmax) {
            return std::make_shared<hdrl::func::ResponseTelluricParameter>(
                hdrl::func::Response::telluric_evaluation_parameter_create(
                    *telluric_models, w_step, half_win, normalize,
                    shift_in_log_scale, quality_areas, fit_areas, lmin, lmax));
          },
          py::arg("telluric_models"), py::arg("w_step"), py::arg("half_win"),
          py::arg("normalize"), py::arg("shift_in_log_scale"),
          py::arg("quality_areas"), py::arg("fit_areas"), py::arg("lmin"),
          py::arg("lmax"),
          R"docstring(
          Constructor for Response Telluric parameter.

          Parameters
          ----------
          telluric_models : hdrl.core.Spectrum1DList
              The available telluric models.
          w_step : float
              Sampling step to use when upsampling model and observed spectrum
              to calculate the cross correlations.
          half_win : int
              Half the search window to be used to find the peak of the cross
              correlation.
          normalize : boolean
              True if the cross correlation should be normalized, False
              otherwise.
          shift_in_log_scale : boolean
              True if the cross correlation has to be calculated in
              logarithmic scale, False otherwise.
          quality_areas : tuple (array of float, array of float)
              Areas where the quality of the fit of the telluric model has to
              be evaluated.
          fit_areas : tuple (array of float, array of float)
              Areas where the median points are extracted from, in order to
              generate the final quality parameters of the telluric model.
          lmin : float
              Minimum wavelength used to calculate the cross-correlation (in
              log scale if shift_in_log_scale = TRUE).
          lmax : float
              Maximum wavelength used to calculate the cross-correlation (in
              log scale if shift_in_log_scale = TRUE).

          Returns
          -------
          hdrl.func.ResponseTelluricParameter
          )docstring")

      .def_static(
          "fit_parameter_create",
          [](size_t radius, py::array fit_points, double wrange,
             py::tuple high_abs_regions) {
            return std::make_shared<hdrl::func::ResponseFitParameter>(
                hdrl::func::Response::fit_parameter_create(
                    radius, fit_points, wrange, high_abs_regions));
          },
          py::arg("radius"), py::arg("fit_points"), py::arg("wrange"),
          py::arg("high_abs_regions"),
          R"docstring(
          Constructor for the hdrl_parameter for the final interpolation of the response.

          Parameters
          ----------
          radius : int
              Radius of the median filter used to smooth the response before
              the final interpolation
          fit_points : array
              Median points where the fit will be calculated.
          wrange : float
              Range around the median point where the median is calculated.
          high_abs_regions : tuple (array of float, array of float)
              High absorption regions that should be skipped when calculating
              the fit. If NULL no skipping is done.

          Returns
          -------
          hdrl.func.ResponseFitParameter
          )docstring")

      .def_static(
          "evaluate_telluric_models",
          [](const std::shared_ptr<hdrl::core::Spectrum1D>& obs_s,
             const std::shared_ptr<hdrl::func::ResponseTelluricParameter>&
                 telluric_par) {
            // Call the function and get the tuple result
            auto result_tuple = hdrl::func::Response::evaluate_telluric_models(
                *obs_s, *telluric_par);

            // Extract the spectrum and duplicate it
            const hdrl::core::Spectrum1D& spectrum = std::get<0>(result_tuple);
            const hdrl_spectrum1D* raw_ptr = spectrum.ptr();
            if (!raw_ptr) {
              throw py::value_error(
                  "Null spectrum in evaluate_telluric_models result");
            }

            // Create a new shared_ptr with the duplicated spectrum
            auto duplicated_spectrum = std::make_shared<hdrl::core::Spectrum1D>(
                hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_duplicate,
                                                     raw_ptr));

            // Return a Python tuple with the duplicated spectrum and other
            // values
            return py::make_tuple(
                duplicated_spectrum,
                std::get<1>(result_tuple),  // best_telluric_shift
                std::get<2>(result_tuple),  // best_mean_minus1
                std::get<3>(result_tuple),  // best_stddev
                std::get<4>(result_tuple)   // best_idx
            );
          },
          py::arg("obs_s"), py::arg("telluric_par"),
          R"docstring(
          This function evaluates all the telluric models and picks the best model.

          Parameters
          ----------
          obs_s : hdrl.core.Spectrum1D
              Observed spectrum.
          telluric_par : hdrl.func.ResponseTelluricParameter
              Telluric correction parameter.

          Returns
          -------
          tuple (hdrl.core.Spectrum1D, float, float, float, int)
              The duplicated spectrum with the best telluric shift, mean,
              standard devation, best model index.
          )docstring")

      .def_static(
          "compute",
          [](const std::shared_ptr<hdrl::core::Spectrum1D>& obs_s,
             const std::shared_ptr<hdrl::core::Spectrum1D>& ref_s,
             const std::shared_ptr<hdrl::core::Spectrum1D>& E_x,
             const std::shared_ptr<hdrl::func::ResponseTelluricParameter>&
                 telluric_par,
             const std::shared_ptr<hdrl::func::ResponseVelocityParameter>&
                 velocity_par,
             const std::shared_ptr<hdrl::func::ResponseCalcParameter>& calc_par,
             const std::shared_ptr<hdrl::func::ResponseFitParameter>& fit_par) {
            return hdrl::func::Response::compute(*obs_s, *ref_s, *E_x,
                                                 *telluric_par, *velocity_par,
                                                 *calc_par, *fit_par);
          },
          py::arg("obs_s"), py::arg("ref_s"), py::arg("E_x"),
          py::arg("telluric_par"), py::arg("velocity_par"), py::arg("calc_par"),
          py::arg("fit_par"),
          R"docstring(
          Computation of the response.

          Parameters
          ----------
          obs_s : hdrl.core.Spectrum1D
              Observed spectrum.
          ref_s : hdrl.core.Spectrum1D
              Reference std star spectrum
          E_x : hdrl.core.Spectrum1D
              Atmospheric Extinction
          telluric_par : hdrl.func.ResponseTelluricParameter
              Telluric correction parameter. NULL if telluric correction is
              skipped.
          velocity_par : hdrl.func.ResponseVelocityParameter
              Doppler shift estimation and compensation. NULL if compensation
              has to be skipped.
          calc_par : hdrl.func.ResponseCalcParameter
              Parameter for the core computation of the response, e.g. exposure
              time.
          fit_par : hdrl.func.ResponseFitParameter
              Parameter for the final interpolation of the response.

          Returns
          -------
          hdrl.func.ResponseResult
              Response result
          )docstring");
}
