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

#include "hdrlfunc/fpn.hpp"

#include <cpl_image_io.h>
#include <hdrl_fpn.h>

namespace hdrl
{
namespace func
{

Fpn::Result
Fpn::compute(hdrl::core::pycpl_image image, hdrl::core::pycpl_mask mask,
             cpl_size dc_mask_x, cpl_size dc_mask_y)
{
  cpl_image* power_spectrum = nullptr;
  double std_val = 0.0;
  double std_mad_val = 0.0;

  // Use Error::throw_errors_with pattern for automatic error handling
  hdrl::core::Error::throw_errors_with(hdrl_fpn_compute, image.im, mask.m,
                                       dc_mask_x, dc_mask_y, &power_spectrum,
                                       &std_val, &std_mad_val);

  // Create the result struct
  Result result;
  result.power_spectrum = hdrl::core::pycpl_image(power_spectrum);
  result.std = std_val;
  result.std_mad = std_mad_val;

  return result;
}

}  // namespace func
}  // namespace hdrl
