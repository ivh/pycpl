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

#ifndef PYHDRL_FUNC_DAR_HPP_
#define PYHDRL_FUNC_DAR_HPP_

#include <hdrl_parameter.h>

#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace func
{

class Dar
{
 public:
  // Constructor that creates the parameter
  Dar(hdrl::core::Value airmass, hdrl::core::Value parang,
      hdrl::core::Value posang, hdrl::core::Value temp, hdrl::core::Value rhum,
      hdrl::core::Value pres, hdrl::core::pycpl_wcs wcs);

  struct Result
  {
    hdrl::core::pycpl_vector xShift;
    hdrl::core::pycpl_vector yShift;
    hdrl::core::pycpl_vector xShiftErr;
    hdrl::core::pycpl_vector yShiftErr;
  };

  // Compute method
  Result
  compute(hdrl::core::Value lambdaRef, hdrl::core::pycpl_vector lambdaIn);

  // Accessor methods
  hdrl::core::Value get_airmass() const { return airmass_; }

  hdrl::core::Value get_parang() const { return parang_; }

  hdrl::core::Value get_posang() const { return posang_; }

  hdrl::core::Value get_temp() const { return temp_; }

  hdrl::core::Value get_rhum() const { return rhum_; }

  hdrl::core::Value get_pres() const { return pres_; }

  hdrl::core::pycpl_wcs get_wcs() const { return wcs_; }

  // Get the underlying parameter pointer
  hdrl_parameter* ptr() { return m_interface; }

 protected:
  hdrl_parameter* m_interface;

  // Store parameter values for getter methods
  hdrl::core::Value airmass_;
  hdrl::core::Value parang_;
  hdrl::core::Value posang_;
  hdrl::core::Value temp_;
  hdrl::core::Value rhum_;
  hdrl::core::Value pres_;
  hdrl::core::pycpl_wcs wcs_;
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_DAR_HPP_