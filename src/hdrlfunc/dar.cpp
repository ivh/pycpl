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

#include "hdrlfunc/dar.hpp"

#include <cpl_type.h>
#include <cpl_vector.h>
#include <hdrl_dar.h>

#include "hdrlcore/error.hpp"
#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace func
{

Dar::Dar(hdrl::core::Value airmass, hdrl::core::Value parang,
         hdrl::core::Value posang, hdrl::core::Value temp,
         hdrl::core::Value rhum, hdrl::core::Value pres,
         hdrl::core::pycpl_wcs wcs)
    : airmass_(airmass), parang_(parang), posang_(posang), temp_(temp),
      rhum_(rhum), pres_(pres), wcs_(wcs)
{
  // Create the parameter using the C function with proper error handling
  m_interface = hdrl::core::Error::throw_errors_with(
      hdrl_dar_parameter_create, airmass_.v, parang_.v, posang_.v, temp_.v,
      rhum_.v, pres_.v, wcs_.w);
}

Dar::Result
Dar::compute(hdrl::core::Value lambdaRef, hdrl::core::pycpl_vector lambdaIn)
{
  // Create output vectors with the same size as lambdaIn
  cpl_errorstate prestate = cpl_errorstate_get();
  cpl_size size = cpl_vector_get_size(lambdaIn.v);
  cpl_vector* xShift = cpl_vector_new(size);
  cpl_vector* yShift = cpl_vector_new(size);
  cpl_vector* xShiftErr = cpl_vector_new(size);
  cpl_vector* yShiftErr = cpl_vector_new(size);
  hdrl::core::Error::throw_errors_after(prestate);

  // Use Error::throw_errors_with pattern for automatic error handling
  hdrl::core::Error::throw_errors_with(hdrl_dar_compute, m_interface,
                                       lambdaRef.v, lambdaIn.v, xShift, yShift,
                                       xShiftErr, yShiftErr);

  // Create the result struct
  Result result;
  result.xShift = hdrl::core::pycpl_vector(xShift);
  result.yShift = hdrl::core::pycpl_vector(yShift);
  result.xShiftErr = hdrl::core::pycpl_vector(xShiftErr);
  result.yShiftErr = hdrl::core::pycpl_vector(yShiftErr);

  // Clean up
  prestate = cpl_errorstate_get();
  cpl_vector_delete(xShift);
  cpl_vector_delete(yShift);
  cpl_vector_delete(xShiftErr);
  cpl_vector_delete(yShiftErr);
  hdrl::core::Error::throw_errors_after(prestate);

  return result;
}

}  // namespace func
}  // namespace hdrl
