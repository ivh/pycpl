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

#ifndef PYHDRL_EFFICIENCY_HPP_
#define PYHDRL_EFFICIENCY_HPP_

#include <hdrl_efficiency.h>
#include <hdrl_types.h>
#include "hdrlcore/spectrum.hpp"
#include "hdrlcore/parameter.hpp"

namespace hdrl {
namespace func {

class Efficiency {
public:
    // Create an HDRL efficiency parameter
    static hdrl::core::Parameter create_parameter(
        double Ap, double Am, double G, double Tex, double Atel);

    // Create an HDRL response parameter
    static hdrl::core::Parameter create_response_parameter(
        double Ap, double Am, double G, double Tex);

    // Compute HDRL efficiency
    static hdrl::core::Spectrum1D compute(
        const hdrl::core::Spectrum1D& I_std_arg, const hdrl::core::Spectrum1D& I_std_ref,
        const hdrl::core::Spectrum1D& E_x, const hdrl::core::Parameter& pars);

    // Compute HDRL response core
    static hdrl::core::Spectrum1D compute_response_core(
        const hdrl::core::Spectrum1D& I_std_arg, const hdrl::core::Spectrum1D& I_std_ref,
        const hdrl::core::Spectrum1D& E_x, const hdrl::core::Parameter& pars);
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_EFFICIENCY_HPP_


