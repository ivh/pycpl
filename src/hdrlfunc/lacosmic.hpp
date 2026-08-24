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

#ifndef PYHDRL_FUNC_LACOSMIC_HPP_
#define PYHDRL_FUNC_LACOSMIC_HPP_

#include <memory>

#include <hdrl_parameter.h>

#include "hdrlcore/image.hpp"
#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace func
{
class LaCosmic
{
 public:
  LaCosmic(double sigma_lim, double f_lim, int max_iter);
  hdrl::core::pycpl_mask edgedetect(std::shared_ptr<hdrl::core::Image> img_in);
  hdrl_parameter* ptr();
  double get_sigma_lim();
  double get_f_lim();
  int get_max_iter();

 protected:
  hdrl_parameter* m_interface;
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_LACOSMIC_HPP_