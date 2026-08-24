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

#include "hdrlfunc/maglim.hpp"

#include <hdrl_maglim.h>

#include "hdrlcore/error.hpp"

namespace hdrl
{
namespace func
{

Maglim::Maglim(double zeropoint, double fwhm, cpl_size kernel_size_x,
               cpl_size kernel_size_y, hdrl_image_extend_method extend_method,
               std::shared_ptr<hdrl::core::Parameter> mode_param)
    : zeropoint_(zeropoint), fwhm_(fwhm), kernel_size_x_(kernel_size_x),
      kernel_size_y_(kernel_size_y), extend_method_(extend_method),
      mode_param_(mode_param)
{
}

double
Maglim::compute(hdrl::core::pycpl_image image)
{
  double limiting_magnitude = 0.0;

  // Use Error::throw_errors_with pattern for automatic error handling
  hdrl::core::Error::throw_errors_with(
      hdrl_maglim_compute, image.im, zeropoint_, fwhm_, kernel_size_x_,
      kernel_size_y_, extend_method_,
      mode_param_ ? mode_param_->ptr() : nullptr, &limiting_magnitude);

  return limiting_magnitude;
}

double
Maglim::get_zeropoint() const
{
  return zeropoint_;
}

double
Maglim::get_fwhm() const
{
  return fwhm_;
}

cpl_size
Maglim::get_kernel_size_x() const
{
  return kernel_size_x_;
}

cpl_size
Maglim::get_kernel_size_y() const
{
  return kernel_size_y_;
}

hdrl_image_extend_method
Maglim::get_extend_method() const
{
  return extend_method_;
}

std::shared_ptr<hdrl::core::Parameter>
Maglim::get_mode_param() const
{
  return mode_param_;
}

}  // namespace func
}  // namespace hdrl