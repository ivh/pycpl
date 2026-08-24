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

#ifndef PYHDRL_DEBUG_TYPES_HPP_
#define PYHDRL_DEBUG_TYPES_HPP_

#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace debug
{

class Types
{
 public:
  Types();
  static hdrl::core::pycpl_table table(hdrl::core::pycpl_table tab);
  static hdrl::core::pycpl_image image(hdrl::core::pycpl_image im);
  static hdrl::core::pycpl_mask mask(hdrl::core::pycpl_mask m);
  static hdrl::core::pycpl_wcs wcs(hdrl::core::pycpl_wcs w);
  static hdrl::core::pycpl_vector vector(hdrl::core::pycpl_vector v);
  static hdrl::core::pycpl_imagelist
  imagelist(hdrl::core::pycpl_imagelist imlist);
  static hdrl::core::pycpl_propertylist
  propertylist(hdrl::core::pycpl_propertylist plist);
};

}  // namespace debug
}  // namespace hdrl

#endif  // PYHDRL_DEBUG_TYPES_HPP_