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

#ifndef PYHDRL_FUNC_BARYCORR_HPP_
#define PYHDRL_FUNC_BARYCORR_HPP_

#include <cpl.h>
#include <hdrl.h>
#include <hdrl_barycorr.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace func
{

// ----------------------------------------------------------------------------
// Default atmospheric constants used in barycentric correction calculations
// ----------------------------------------------------------------------------

// Mean sea level pressure in hPa
constexpr double kPressureDefault = 1013.25;

// Standard ambient temperature in Kelvin (≈ 15°C)
constexpr double kTemperatureDefault = 288.15;

// Typical relative humidity (dry - 10%)
constexpr double kHumidityDefault = 0.1;

// Default wavelength in nm (green light, typical for visible instruments)
constexpr double kWavelengthDefault = 550.0;

class Barycorr
{
 public:
  /// Standard atmospheric pressure at sea level in hPa
  static constexpr double kPressureDefault = 1013.25;

  /// Standard temperature (15°C) in Kelvin
  static constexpr double kTemperatureDefault = 288.15;

  /// Typical dry-air relative humidity
  static constexpr double kHumidityDefault = 0.1;

  /// Default wavelength (green light, central V-band) in nm
  static constexpr double kWavelengthDefault = 550.0;

  Barycorr(double ra, double dec, double mjd_obs);
  static double
  compute(double ra, double dec, const hdrl::core::pycpl_table& eop_table,
          double mjd_obs, double lat, double lon, double hveight,
          double time_to_mid_exposure, double pressure = kPressureDefault,
          double temperature = kTemperatureDefault,
          double humidity = kHumidityDefault,
          double wavelength = kWavelengthDefault);

  double get_ra() const;
  double get_dec() const;
  double get_mjd_obs() const;

 private:
  double ra_;
  double dec_;
  double mjd_obs_;
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_BARYCORR_HPP_
