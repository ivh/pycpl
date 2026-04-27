// This file is part of the PyHDRL Python language bindings
// Copyright (C) 2020-2025 European Southern Observatory
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
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <memory>
#include <stdexcept>
#include <tuple>

#include "hdrlfunc/response.hpp"
#include "hdrlcore/error.hpp"

namespace py = pybind11;

void bind_response(py::module_& m) {
    // Bind ResponseResult class
    py::class_<hdrl::func::ResponseResult, std::shared_ptr<hdrl::func::ResponseResult>>(m, "ResponseResult")
        .def("get_final_response",
            [](const hdrl::func::ResponseResult& self) {
                // Get the raw pointer and create a new Spectrum1D from it
                const hdrl_spectrum1D* raw_ptr = self.get_final_response().ptr();
                    if (!raw_ptr) {
                        throw py::value_error("Null final response spectrum");
                    }
                return std::make_shared<hdrl::core::Spectrum1D>(
                    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_duplicate, raw_ptr));
            })
        .def("get_selected_response",
            [](const hdrl::func::ResponseResult& self) {
                const hdrl_spectrum1D* raw_ptr = self.get_selected_response().ptr();
                if (!raw_ptr) {
                    throw py::value_error("Null selected response spectrum");
                }
                return std::make_shared<hdrl::core::Spectrum1D>(
                    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_duplicate, raw_ptr));
            })
        .def("get_raw_response",
            [](const hdrl::func::ResponseResult& self) {
                const hdrl_spectrum1D* raw_ptr = self.get_raw_response().ptr();
                if (!raw_ptr) {
                    throw py::value_error("Null raw response spectrum");
                }
                return std::make_shared<hdrl::core::Spectrum1D>(
                    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_duplicate, raw_ptr));
            })
        .def("get_corrected_obs_spectrum",
            [](const hdrl::func::ResponseResult& self) {
                const hdrl_spectrum1D* raw_ptr = self.get_corrected_obs_spectrum().ptr();
                if (!raw_ptr) {
                    throw py::value_error("Null corrected observation spectrum");
                }
                return std::make_shared<hdrl::core::Spectrum1D>(
                    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_duplicate, raw_ptr));
            })
        .def("get_best_telluric_model_idx", &hdrl::func::ResponseResult::get_best_telluric_model_idx)
        .def("get_avg_diff_from_1", &hdrl::func::ResponseResult::get_avg_diff_from_1)
        .def("get_stddev", &hdrl::func::ResponseResult::get_stddev)
        .def("get_telluric_shift", &hdrl::func::ResponseResult::get_telluric_shift)
        .def("get_doppler_shift", &hdrl::func::ResponseResult::get_doppler_shift)
        .def("__repr__", [](const hdrl::func::ResponseResult& self) -> std::string {
            try {
                return "ResponseResult(best_telluric_model_idx=" +
                       std::to_string(self.get_best_telluric_model_idx()) +
                       ", avg_diff_from_1=" + std::to_string(self.get_avg_diff_from_1()) +
                       ", stddev=" + std::to_string(self.get_stddev()) +
                       ", telluric_shift=" + std::to_string(self.get_telluric_shift()) +
                       ", doppler_shift=" + std::to_string(self.get_doppler_shift()) + ")";
            } catch (const std::exception&) {
                return "ResponseResult(invalid)";
            }
        });

    // Bind Response class
    py::class_<hdrl::func::Response>(m, "Response")
        .def_static("velocity_parameter_create",
            [](double wguess, double range_wmin, double range_wmax,
               double fit_wmin, double fit_wmax, size_t fit_half_win) {
                return hdrl::func::Response::velocity_parameter_create(
                    wguess, range_wmin, range_wmax, fit_wmin, fit_wmax, fit_half_win);
            },
            py::arg("wguess"), py::arg("range_wmin"), py::arg("range_wmax"),
            py::arg("fit_wmin"), py::arg("fit_wmax"), py::arg("fit_half_win"))

        .def_static("calc_parameter_create",
            [](double Ap, double Am, double G, double Tex) {
                return hdrl::func::Response::calc_parameter_create(Ap, Am, G, Tex);
            },
            py::arg("Ap"), py::arg("Am"), py::arg("G"), py::arg("Tex"))

        .def_static("telluric_evaluation_parameter_create",
            [](const std::shared_ptr<hdrl::core::Spectrum1DList>& telluric_models,
               double w_step, size_t half_win, bool normalize, bool shift_in_log_scale,
               py::tuple quality_areas, py::tuple fit_areas, double lmin, double lmax) {
                return hdrl::func::Response::telluric_evaluation_parameter_create(
                    *telluric_models, w_step, half_win, normalize, shift_in_log_scale,
                    quality_areas, fit_areas, lmin, lmax);
            },
            py::arg("telluric_models"), py::arg("w_step"), py::arg("half_win"),
            py::arg("normalize"), py::arg("shift_in_log_scale"),
            py::arg("quality_areas"), py::arg("fit_areas"),
            py::arg("lmin"), py::arg("lmax"))

        .def_static("fit_parameter_create",
            [](size_t radius, py::array fit_points, double wrange, py::tuple high_abs_regions) {
                return hdrl::func::Response::fit_parameter_create(
                    radius, fit_points, wrange, high_abs_regions);
            },
            py::arg("radius"), py::arg("fit_points"), py::arg("wrange"), py::arg("high_abs_regions"))

        .def_static("evaluate_telluric_models",
            [](const std::shared_ptr<hdrl::core::Spectrum1D>& obs_s,
               const hdrl::core::Parameter& telluric_par) {
                // Call the function and get the tuple result
                auto result_tuple = hdrl::func::Response::evaluate_telluric_models(*obs_s, telluric_par);

                // Extract the spectrum and duplicate it
                const hdrl::core::Spectrum1D& spectrum = std::get<0>(result_tuple);
                const hdrl_spectrum1D* raw_ptr = spectrum.ptr();
                if (!raw_ptr) {
                    throw py::value_error("Null spectrum in evaluate_telluric_models result");
                }

                // Create a new shared_ptr with the duplicated spectrum
                auto duplicated_spectrum = std::make_shared<hdrl::core::Spectrum1D>(
                    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_duplicate, raw_ptr));

                // Return a Python tuple with the duplicated spectrum and other values
                return py::make_tuple(
                    duplicated_spectrum,
                    std::get<1>(result_tuple),  // best_telluric_shift
                    std::get<2>(result_tuple),  // best_mean_minus1
                    std::get<3>(result_tuple),  // best_stddev
                    std::get<4>(result_tuple)   // best_idx
                );
            },
            py::arg("obs_s"), py::arg("telluric_par"))

        .def_static("compute",
            [](const std::shared_ptr<hdrl::core::Spectrum1D>& obs_s,
               const std::shared_ptr<hdrl::core::Spectrum1D>& ref_s,
               const std::shared_ptr<hdrl::core::Spectrum1D>& E_x,
               const hdrl::core::Parameter& telluric_par,
               const hdrl::core::Parameter& velocity_par,
               const hdrl::core::Parameter& calc_par,
               const hdrl::core::Parameter& fit_par) {
                return hdrl::func::Response::compute(
                    *obs_s, *ref_s, *E_x, telluric_par, velocity_par, calc_par, fit_par);
            },
            py::arg("obs_s"), py::arg("ref_s"), py::arg("E_x"),
            py::arg("telluric_par"), py::arg("velocity_par"),
            py::arg("calc_par"), py::arg("fit_par"));
}
