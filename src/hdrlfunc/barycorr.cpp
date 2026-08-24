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


#include "hdrlfunc/barycorr.hpp"

#include <cstring>

#include <hdrl_barycorr.h>

#include "hdrlcore/error.hpp"

namespace hdrl
{
namespace func
{
// Constructor Implementation
// Define the constructor
Barycorr::Barycorr() {}

// Compute Barycentric Correction
double
Barycorr::compute(double ra, double dec,
                  const hdrl::core::pycpl_table& eop_table, double mjd_obs,
                  double lat, double lon, double height,
                  double time_to_mid_exposure, double pressure,
                  double temperature, double humidity, double wavelength)
{
  double bc_velocity = 0.0;

  if (!eop_table.t) {
    throw hdrl::core::NullInputError(
        HDRL_ERROR_LOCATION,
        "Provided EOP table is invalid, it must not be 'None'");
  }

  hdrl::core::Error::throw_errors_with(
      hdrl_barycorr_compute, ra, dec, eop_table.t, mjd_obs,
      time_to_mid_exposure, lon, lat, height, pressure, temperature, humidity,
      wavelength, &bc_velocity);

  return bc_velocity;
}


}  // namespace func
}  // namespace hdrl
