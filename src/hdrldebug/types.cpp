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

#include "hdrldebug/types.hpp"

namespace hdrl
{
namespace debug
{

Types::Types() {}

// These simple functions exercise the custom type casters

hdrl::core::pycpl_table
Types::table(hdrl::core::pycpl_table tab)
{
  return tab;
}

hdrl::core::pycpl_propertylist
Types::propertylist(hdrl::core::pycpl_propertylist plist)
{
  return plist;
}

hdrl::core::pycpl_image
Types::image(hdrl::core::pycpl_image im)
{
  return im;
}

hdrl::core::pycpl_mask
Types::mask(hdrl::core::pycpl_mask m)
{
  return m;
}

hdrl::core::pycpl_vector
Types::vector(hdrl::core::pycpl_vector v)
{
  return v;
}

hdrl::core::pycpl_imagelist
Types::imagelist(hdrl::core::pycpl_imagelist imlist)
{
  return imlist;
}

hdrl::core::pycpl_wcs
Types::wcs(hdrl::core::pycpl_wcs w)
{
  return w;
}

}  // namespace debug
}  // namespace hdrl
