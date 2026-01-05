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

#include "hdrlfunc/airmass.hpp"
#include "hdrlcore/error.hpp"

#include <hdrl_types.h>
#include <hdrl_utils.h>

namespace hdrl
{
namespace func
{

Airmass::Airmass(hdrl::core::Value ra, hdrl::core::Value dec,
                 hdrl::core::Value lst, hdrl::core::Value exptime,
                 hdrl::core::Value latitude, hdrl_airmass_approx type)
    : ra_(ra), dec_(dec), lst_(lst), exptime_(exptime), latitude_(latitude),
      type_(type)
{
}

hdrl::core::Value
Airmass::compute()
{
  // Use Error::throw_errors_with pattern for automatic error handling
  hdrl_value result = hdrl::core::Error::throw_errors_with(
      hdrl_utils_airmass, ra_.v, dec_.v, lst_.v, exptime_.v, latitude_.v,
      type_);

  return hdrl::core::Value(result);
}

}  // namespace func
}  // namespace hdrl
