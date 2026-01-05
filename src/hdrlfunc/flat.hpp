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

#ifndef PYHDRL_FUNC_FLAT_HPP_
#define PYHDRL_FUNC_FLAT_HPP_

#include <memory>

#include <cpl_type.h>
#include <hdrl_flat.h>
#include <hdrl_parameter.h>
#include <pybind11/pytypes.h>

#include "hdrlcore/imagelist.hpp"
#include "hdrlcore/pycpl_types.hpp"
#include "hdrlfunc/collapse.hpp"

namespace hdrl
{
namespace func
{
using hdrl::core::ImageList;
using hdrl::core::pycpl_mask;
using hdrl::func::Collapse;

class Flat
{
 public:
  // Constructors
  Flat(cpl_size filter_size_x, cpl_size filter_size_y, hdrl_flat_method method);
  // Compute
  py::object compute(std::shared_ptr<ImageList> hdrl_data, Collapse collapse,
                     pycpl_mask stat_mask);
  // Accessors
  cpl_size get_filter_size_x();
  cpl_size get_filter_size_y();
  hdrl_flat_method get_method();
  hdrl_parameter* ptr();

 protected:
  hdrl_parameter* m_interface;
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_FLAT_HPP_