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

#ifndef PYHDRL_FUNC_MAGLIM_HPP_
#define PYHDRL_FUNC_MAGLIM_HPP_

#include <cpl_type.h>

#include <memory>

#include <hdrl_utils.h>

#include "hdrlcore/parameter.hpp"
#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace func
{

class Maglim
{
 public:
  Maglim(double zeropoint, double fwhm, cpl_size kernel_size_x,
         cpl_size kernel_size_y, hdrl_image_extend_method extend_method,
         std::shared_ptr<hdrl::core::Parameter> mode_param);
  double compute(hdrl::core::pycpl_image image);

  // Accessors for parameters
  double get_zeropoint() const;
  double get_fwhm() const;
  cpl_size get_kernel_size_x() const;
  cpl_size get_kernel_size_y() const;
  hdrl_image_extend_method get_extend_method() const;
  std::shared_ptr<hdrl::core::Parameter> get_mode_param() const;

 private:
  double zeropoint_;
  double fwhm_;
  cpl_size kernel_size_x_;
  cpl_size kernel_size_y_;
  hdrl_image_extend_method extend_method_;
  std::shared_ptr<hdrl::core::Parameter> mode_param_;
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_MAGLIM_HPP_