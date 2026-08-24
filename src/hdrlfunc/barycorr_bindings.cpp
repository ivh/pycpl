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

#include "hdrlfunc/barycorr_bindings.hpp"

#include <memory>
#include <tuple>

#include <pybind11/complex.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "hdrlcore/pycpl_table.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_types.hpp"
#include "hdrlfunc/barycorr.hpp"

namespace py = pybind11;

using hdrl::func::Barycorr;

void
bind_barycorr(py::module& m)
{
  py::class_<Barycorr, std::shared_ptr<Barycorr>>(m, "Barycorr")
      .def(py::init(), R"docstring(
           The hdrl.func.Barycorr class provides an interface to 
           calculation of the barycentric correction.
      )docstring")

      .def_static(
          "compute",
          [](std::tuple<double, double> target,
             std::tuple<double, double, double> observer,
             const hdrl::core::pycpl_table& eop_table, double mjd_obs,
             double time_to_mid_exposure,
             double pressure = Barycorr::kPressureDefault,
             double temperature = Barycorr::kTemperatureDefault,
             double humidity = Barycorr::kHumidityDefault,
             double wavelength = Barycorr::kWavelengthDefault) -> double {
            double ra = std::get<0>(target);
            double dec = std::get<1>(target);
            double lat = std::get<0>(observer);
            double lon = std::get<1>(observer);
            double height = std::get<2>(observer);

            return Barycorr::compute(ra, dec, eop_table, mjd_obs, lat, lon,
                                     height, time_to_mid_exposure, pressure,
                                     temperature, humidity, wavelength);
          },
          py::arg("target"), py::arg("observer"), py::arg("eop_table"),
          py::arg("mjd_obs"), py::arg("time_to_mid_exposure"),
          py::arg("pressure") = Barycorr::kPressureDefault,
          py::arg("temperature") = Barycorr::kTemperatureDefault,
          py::arg("humidity") = Barycorr::kHumidityDefault,
          py::arg("wavelength") = Barycorr::kWavelengthDefault,
          R"docstring(
            Compute the barycentric correction for an observation, using the ERFA function
            eraApco13().

            Parameters
            ----------
            target : tuple
                A tuple (ra, dec) in degrees.
            observer : tuple
                A tuple (lat, lon, height) where latitude and longitude are in degrees,
                and height is in meters.
            eop_table : cpl.core.Table
                The Earth Orientation Parameter (EOP) table.
            mjd_obs : float
                Modified Julian Date of the observation.
            time_to_mid_exposure : float
                Time to mid exposure in seconds.
            pressure : float, optional
                Atmospheric pressure in hPa (default: 0.0).
            temperature : float, optional
                Ambient temperature in degrees Celsius (default: 0.0).
            humidity : float, optional
                Relative humidity (0 to 1, default: 0.0).
            wavelength : float, optional
                Observing wavelength in micrometers (default: 0.0).

            Returns
            -------
            float
                Computed barycentric correction in m/s.
        )docstring");
}
