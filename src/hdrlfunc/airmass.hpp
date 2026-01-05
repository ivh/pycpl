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

#ifndef PYHDRL_FUNC_AIRMASS_HPP_
#define PYHDRL_FUNC_AIRMASS_HPP_

#include <hdrl_utils.h>

#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace func
{

class Airmass
{
 public:
  // Constructor that takes all parameters
  Airmass(hdrl::core::Value ra, hdrl::core::Value dec, hdrl::core::Value lst,
          hdrl::core::Value exptime, hdrl::core::Value latitude,
          hdrl_airmass_approx type);

  // Compute method
  hdrl::core::Value compute();

  // Accessor methods
  hdrl::core::Value get_ra() const { return ra_; }

  hdrl::core::Value get_dec() const { return dec_; }

  hdrl::core::Value get_lst() const { return lst_; }

  hdrl::core::Value get_exptime() const { return exptime_; }

  hdrl::core::Value get_latitude() const { return latitude_; }

  hdrl_airmass_approx get_type() const { return type_; }

 protected:
  // Store parameter values for getter methods
  hdrl::core::Value ra_;
  hdrl::core::Value dec_;
  hdrl::core::Value lst_;
  hdrl::core::Value exptime_;
  hdrl::core::Value latitude_;
  hdrl_airmass_approx type_;
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_AIRMASS_HPP_
