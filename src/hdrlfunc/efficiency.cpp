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

#include "efficiency.hpp"
#include <cpl_error.h>

namespace hdrl {
namespace func {

hdrl::core::Parameter Efficiency::create_parameter(
    double Ap, double Am, double G, double Tex, double Atel) {
    hdrl_value ap_val = {Ap, 0.0};
    hdrl_value am_val = {Am, 0.0};
    hdrl_value g_val = {G, 0.0};
    hdrl_value tex_val = {Tex, 0.0};
    hdrl_value atel_val = {Atel, 0.0};
    return hdrl::core::Parameter(hdrl::core::Error::throw_errors_with(hdrl_efficiency_parameter_create, ap_val, am_val, g_val, tex_val, atel_val));
}

hdrl::core::Parameter Efficiency::create_response_parameter(
    double Ap, double Am, double G, double Tex) {
    hdrl_value ap_val = {Ap, 0.0};
    hdrl_value am_val = {Am, 0.0};
    hdrl_value g_val = {G, 0.0};
    hdrl_value tex_val = {Tex, 0.0};
    return hdrl::core::Parameter(hdrl::core::Error::throw_errors_with(hdrl_response_parameter_create, ap_val, am_val, g_val, tex_val));
}

hdrl::core::Spectrum1D Efficiency::compute(
    const hdrl::core::Spectrum1D& I_std_arg, const hdrl::core::Spectrum1D& I_std_ref,
    const hdrl::core::Spectrum1D& E_x, const hdrl::core::Parameter& pars) {
    // Create a non-const copy of pars to call the non-const ptr() method
    hdrl::core::Parameter pars_copy = const_cast<hdrl::core::Parameter&>(pars);
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(hdrl_efficiency_compute,
        I_std_arg.get_raw(), I_std_ref.get_raw(), E_x.get_raw(), pars_copy.ptr());
    return hdrl::core::Spectrum1D(result);
}

hdrl::core::Spectrum1D Efficiency::compute_response_core(
    const hdrl::core::Spectrum1D& I_std_arg, const hdrl::core::Spectrum1D& I_std_ref,
    const hdrl::core::Spectrum1D& E_x, const hdrl::core::Parameter& pars) {
    // Create a non-const copy of pars to call the non-const ptr() method
    hdrl::core::Parameter pars_copy = const_cast<hdrl::core::Parameter&>(pars);
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(hdrl_response_core_compute,
        I_std_arg.get_raw(), I_std_ref.get_raw(), E_x.get_raw(), pars_copy.ptr());
    return hdrl::core::Spectrum1D(result);
}

}  // namespace func
}  // namespace hdrl



