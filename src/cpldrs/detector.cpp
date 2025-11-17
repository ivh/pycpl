// This file is part of PyCPL the ESO CPL Python language bindings
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

#include "cpldrs/detector.hpp"

#include <cpl_detector.h>
#include <cpl_image_io.h>

#include "cplcore/error.hpp"

namespace cpl
{
namespace drs
{
namespace detector
{

void
interpolate_rejected(const cpl::core::ImageBase& toclean)
{
  cpl::core::Error::throw_errors_with(
      cpl_detector_interpolate_rejected,
      const_cast<cpl_image*>(
          toclean.ptr()));  // interpolation of reject is performed inplace
}

std::tuple<double, double>
get_bias_window(const cpl::core::ImageBase& bias_image,
                std::optional<std::tuple<size, size, size, size>> zone_def,
                size ron_hsize, size ron_nsamp)
{
  const size* zone_def_ptr;
  if (zone_def.has_value()) {
    const size zone_def_arr[4] = {
        std::get<0>(zone_def.value()) + 1, std::get<1>(zone_def.value()) + 1,
        std::get<2>(zone_def.value()) + 1,
        std::get<3>(zone_def.value()) + 1};  // Convert to CPL coords
    zone_def_ptr = zone_def_arr;
  } else {
    zone_def_ptr = nullptr;
  }
  double bias, error;
  cpl::core::Error::throw_errors_with(cpl_flux_get_bias_window,
                                      bias_image.ptr(), zone_def_ptr, ron_hsize,
                                      ron_nsamp, &bias, &error);  // Outputs
  return std::make_tuple(bias, error);
}

std::tuple<double, double>
get_noise_window(const cpl::core::ImageBase& diff,
                 std::optional<std::tuple<size, size, size, size>> zone_def,
                 size ron_hsize, size ron_nsamp)
{
  const size* zone_def_ptr;
  if (zone_def.has_value()) {
    const size zone_def_arr[4] = {
        std::get<0>(zone_def.value()) + 1, std::get<1>(zone_def.value()) + 1,
        std::get<2>(zone_def.value()) + 1,
        std::get<3>(zone_def.value()) + 1};  // Convert to CPL coords
    zone_def_ptr = zone_def_arr;
  } else {
    zone_def_ptr = nullptr;
  }
  double noise, error;
  cpl::core::Error::throw_errors_with(cpl_flux_get_noise_window, diff.ptr(),
                                      zone_def_ptr, ron_hsize, ron_nsamp,
                                      &noise, &error);  // Outputs
  return std::make_tuple(noise, error);
}

std::tuple<double, double>
get_noise_ring(const cpl::core::ImageBase& diff,
               std::tuple<size, size, double, double> zone_def, size ron_hsize,
               size ron_nsamp)
{
  double* zone_def_ptr;
  // Note: the max value of long exceeds the max double value so potential
  // data loss? On the other hand I'm unsure if anyone is going to make a
  // image 9,223,372,036,854,775,807 wide or high
  zone_def_ptr = new double[4];
  // Convert first two elements as they represent x,y coords
  zone_def_ptr[0] = (double)std::get<0>(zone_def) + 1;
  zone_def_ptr[1] = (double)std::get<1>(zone_def) + 1;
  // but leave the others as they represent the ring radiuses
  zone_def_ptr[2] = std::get<2>(zone_def);
  zone_def_ptr[3] = std::get<3>(zone_def);

  double noise, error;
  cpl::core::Error::throw_errors_with(cpl_flux_get_noise_ring, diff.ptr(),
                                      zone_def_ptr, ron_hsize, ron_nsamp,
                                      &noise, &error);  // Outputs
  delete[] zone_def_ptr;

  return std::make_tuple(noise, error);
}

}  // namespace detector
}  // namespace drs
}  // namespace cpl
