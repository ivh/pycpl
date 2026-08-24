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

#ifndef PYHDRL_FUNC_EFFICIENCY_HPP_
#define PYHDRL_FUNC_EFFICIENCY_HPP_

#include <hdrl_parameter.h>

#include "hdrlcore/spectrum.hpp"

namespace hdrl
{
namespace func
{

class EfficiencyParameter
{
 public:
  EfficiencyParameter(double Ap, double Am, double G, double Tex, double Atel);
  hdrl_parameter* ptr();
  const hdrl_parameter* ptr() const;

 protected:
  hdrl_parameter* m_interface = nullptr;
};

class EfficiencyResponseParameter
{
 public:
  EfficiencyResponseParameter(double Ap, double Am, double G, double Tex);
  hdrl_parameter* ptr();
  const hdrl_parameter* ptr() const;

 protected:
  hdrl_parameter* m_interface = nullptr;
};

class Efficiency
{
 public:
  static EfficiencyParameter
  create_parameter(double Ap, double Am, double G, double Tex, double Atel);

  static EfficiencyResponseParameter
  create_response_parameter(double Ap, double Am, double G, double Tex);

  static hdrl::core::Spectrum1D
  compute(const hdrl::core::Spectrum1D& I_std_arg,
          const hdrl::core::Spectrum1D& I_std_ref,
          const hdrl::core::Spectrum1D& E_x, const EfficiencyParameter& pars);

  static hdrl::core::Spectrum1D
  compute_response_core(const hdrl::core::Spectrum1D& I_std_arg,
                        const hdrl::core::Spectrum1D& I_std_ref,
                        const hdrl::core::Spectrum1D& E_x,
                        const EfficiencyResponseParameter& pars);
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_EFFICIENCY_HPP_
