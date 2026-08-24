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

#include "hdrlfunc/efficiency_bindings.hpp"

#include <memory>

#include <pybind11/pybind11.h>

#include "hdrlfunc/efficiency.hpp"


namespace py = pybind11;

void
bind_efficiency(py::module_& m)
{
  py::class_<hdrl::func::EfficiencyParameter,
             std::shared_ptr<hdrl::func::EfficiencyParameter>>(
      m, "EfficiencyParameter")
      .def(py::init<double, double, double, double, double>(), py::arg("Ap"),
           py::arg("Am"), py::arg("G"), py::arg("Tex"), py::arg("Atel"),
           R"docstring(
           Constructor for Efficiency parameter.

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
           Atel : float
               Collecting area of the telescope [cm2].
           )docstring");

  py::class_<hdrl::func::EfficiencyResponseParameter,
             std::shared_ptr<hdrl::func::EfficiencyResponseParameter>>(
      m, "EfficiencyResponseParameter")
      .def(py::init<double, double, double, double>(), py::arg("Ap"),
           py::arg("Am"), py::arg("G"), py::arg("Tex"),
           R"docstring(
           Constructor for Efficiency Response parameter.

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

  py::class_<hdrl::func::Efficiency>(m, "Efficiency")
      .def_static(
          "create_parameter",
          [](double Ap, double Am, double G, double Tex, double Atel) {
            return std::make_shared<hdrl::func::EfficiencyParameter>(
                hdrl::func::Efficiency::create_parameter(Ap, Am, G, Tex, Atel));
          },
          py::arg("Ap"), py::arg("Am"), py::arg("G"), py::arg("Tex"),
          py::arg("Atel"), R"docstring(
          Create an HDRL efficiency parameter.

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
          Atel : float
              Collecting area of the telescope [cm2].

          Returns
          -------
          hdrl.func.EfficiencyParameter
              A newly alocated parameter.
          )docstring")

      .def_static(
          "create_response_parameter",
          [](double Ap, double Am, double G, double Tex) {
            return std::make_shared<hdrl::func::EfficiencyResponseParameter>(
                hdrl::func::Efficiency::create_response_parameter(Ap, Am, G,
                                                                  Tex));
          },
          py::arg("Ap"), py::arg("Am"), py::arg("G"), py::arg("Tex"),
          R"docstring(
          Create an HDRL response parameter.

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
          hdrl.func.EfficiencyResponseParameter
              A newly alocated parameter.
          )docstring")

      .def_static(
          "compute",
          [](const std::shared_ptr<hdrl::core::Spectrum1D>& I_std_arg,
             const std::shared_ptr<hdrl::core::Spectrum1D>& I_std_ref,
             const std::shared_ptr<hdrl::core::Spectrum1D>& E_x,
             const std::shared_ptr<hdrl::func::EfficiencyParameter>& pars) {
            return hdrl::func::Efficiency::compute(*I_std_arg, *I_std_ref, *E_x,
                                                   *pars);
          },
          py::arg("I_std_arg"), py::arg("I_std_ref"), py::arg("E_x"),
          py::arg("pars"), R"docstring(
          Compute HDRL efficiency.

          Parameters
          ----------
          I_std_arg : hdrl.core.Spectrum1D
              Std star observed spectrum, wavelength in [nm].
          I_std_ref : hdrl.core.Spectrum1D
              Std start model spectrum, wavelength in [nm].
          E_x : hdrl.core.Spectrum1D
              Atm. extinction model spectrum, wavelength in [nm].
          pars : hdrl.func.EfficiencyParameter
              Parameters.

          Returns
          -------
          hdrl.core.Spectrum1D
              Efficiency.
          )docstring")

      .def_static(
          "compute_response_core",
          [](const std::shared_ptr<hdrl::core::Spectrum1D>& I_std_arg,
             const std::shared_ptr<hdrl::core::Spectrum1D>& I_std_ref,
             const std::shared_ptr<hdrl::core::Spectrum1D>& E_x,
             const std::shared_ptr<hdrl::func::EfficiencyResponseParameter>&
                 pars) {
            return hdrl::func::Efficiency::compute_response_core(
                *I_std_arg, *I_std_ref, *E_x, *pars);
          },
          py::arg("I_std_arg"), py::arg("I_std_ref"), py::arg("E_x"),
          py::arg("pars"), R"docstring(
          Compute HDRL response core.

          Parameters
          ----------
          I_std_arg : hdrl.core.Spectrum1D
              Std star observed spectrum, wavelength in [nm].
          I_std_ref : hdrl.core.Spectrum1D
              Std start model spectrum, wavelength in [nm].
          E_x : hdrl.core.Spectrum1D
              Atm. extinction model spectrum, wavelength in [nm].
          pars : hdrl.func.EfficiencyParameter
              Parameters.

          Returns
          -------
          hdrl.core.Spectrum1D
              Response.
          )docstring");
}
