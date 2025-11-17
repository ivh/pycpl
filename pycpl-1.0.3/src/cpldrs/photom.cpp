// This file is part of PyCPL the ESO CPL Python language bindings
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

#include "cpldrs/photom.hpp"

#include <cpl_vector.h>

#include "cplcore/error.hpp"

namespace cpl
{
namespace drs
{
namespace photom
{
cpl::core::Vector
fill_blackbody(cpl_unit out_unit, const cpl::core::Vector& evalpoints,
               cpl_unit in_unit, double temp)
{
  // Preallocate the output vector: must be the same size as evalpoints
  cpl_vector* spectrum_out = cpl_vector_new(evalpoints.get_size());
  cpl::core::Error::throw_errors_with(cpl_photom_fill_blackbody, spectrum_out,
                                      out_unit, evalpoints.ptr(), in_unit,
                                      temp);
  return cpl::core::Vector(spectrum_out);  // Return wrapped output vector
}

}  // namespace photom
}  // namespace drs
}  // namespace cpl