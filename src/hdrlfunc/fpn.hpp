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

#ifndef PYHDRL_FUNC_FPN_HPP_
#define PYHDRL_FUNC_FPN_HPP_

#include <cpl_type.h>

#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace func
{

class Fpn
{
 public:
  Fpn() = default;

  struct Result
  {
    hdrl::core::pycpl_image power_spectrum;
    double std;
    double std_mad;
  };

  static Result
  compute(hdrl::core::pycpl_image image, hdrl::core::pycpl_mask mask,
          cpl_size dc_mask_x, cpl_size dc_mask_y);
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_FPN_HPP_
