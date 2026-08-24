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

#include "hdrlfunc/efficiency.hpp"

#include "hdrl_efficiency.h"
#include "hdrl_types.h"

#include "hdrlcore/error.hpp"

namespace hdrl
{
namespace func
{

using hdrl::core::Error;

hdrl_parameter*
EfficiencyParameter::ptr()
{
  return m_interface;
}

const hdrl_parameter*
EfficiencyParameter::ptr() const
{
  return m_interface;
}

EfficiencyParameter::EfficiencyParameter(double Ap, double Am, double G,
                                         double Tex, double Atel)
{
  hdrl_value ap_val = {Ap, 0.0};
  hdrl_value am_val = {Am, 0.0};
  hdrl_value g_val = {G, 0.0};
  hdrl_value tex_val = {Tex, 0.0};
  hdrl_value atel_val = {Atel, 0.0};
  m_interface =
      Error::throw_errors_with(hdrl_efficiency_parameter_create, ap_val, am_val,
                               g_val, tex_val, atel_val);
}

hdrl_parameter*
EfficiencyResponseParameter::ptr()
{
  return m_interface;
}

const hdrl_parameter*
EfficiencyResponseParameter::ptr() const
{
  return m_interface;
}

EfficiencyResponseParameter::EfficiencyResponseParameter(double Ap, double Am,
                                                         double G, double Tex)
{
  hdrl_value ap_val = {Ap, 0.0};
  hdrl_value am_val = {Am, 0.0};
  hdrl_value g_val = {G, 0.0};
  hdrl_value tex_val = {Tex, 0.0};
  m_interface = Error::throw_errors_with(hdrl_response_parameter_create, ap_val,
                                         am_val, g_val, tex_val);
}

EfficiencyParameter
Efficiency::create_parameter(double Ap, double Am, double G, double Tex,
                             double Atel)
{
  return EfficiencyParameter(Ap, Am, G, Tex, Atel);
}

EfficiencyResponseParameter
Efficiency::create_response_parameter(double Ap, double Am, double G,
                                      double Tex)
{
  return EfficiencyResponseParameter(Ap, Am, G, Tex);
}

hdrl::core::Spectrum1D
Efficiency::compute(const hdrl::core::Spectrum1D& I_std_arg,
                    const hdrl::core::Spectrum1D& I_std_ref,
                    const hdrl::core::Spectrum1D& E_x,
                    const EfficiencyParameter& pars)
{
  hdrl_spectrum1D* result = Error::throw_errors_with(
      hdrl_efficiency_compute, I_std_arg.ptr(), I_std_ref.ptr(), E_x.ptr(),
      const_cast<hdrl_parameter*>(pars.ptr()));
  return hdrl::core::Spectrum1D(result);
}

hdrl::core::Spectrum1D
Efficiency::compute_response_core(const hdrl::core::Spectrum1D& I_std_arg,
                                  const hdrl::core::Spectrum1D& I_std_ref,
                                  const hdrl::core::Spectrum1D& E_x,
                                  const EfficiencyResponseParameter& pars)
{
  hdrl_spectrum1D* result = Error::throw_errors_with(
      hdrl_response_core_compute, I_std_arg.ptr(), I_std_ref.ptr(), E_x.ptr(),
      const_cast<hdrl_parameter*>(pars.ptr()));
  return hdrl::core::Spectrum1D(result);
}

}  // namespace func
}  // namespace hdrl
