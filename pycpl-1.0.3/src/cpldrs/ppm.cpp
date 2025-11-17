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

#include "cpldrs/ppm.hpp"

#include <cpl_array.h>
#include <cpl_bivector.h>
#include <cpl_matrix.h>
#include <cpl_ppm.h>

#include "cplcore/error.hpp"

namespace cpl
{
namespace drs
{
cpl::core::Bivector
match_positions(const cpl::core::Vector& peaks, const cpl::core::Vector& lines,
                double min_disp, double max_disp, double tolerance)

{
  cpl_bivector* res = cpl::core::Error::throw_errors_with(
      cpl_ppm_match_positions, peaks.ptr(), lines.ptr(), min_disp, max_disp,
      tolerance, (cpl_array**)NULL, (cpl_array**)NULL);
  if (res == NULL) {
    // Throw, explain that the function may have returned null
    if (peaks.get_size() < 4) {
      throw cpl::core::DataNotFoundError(
          PYCPL_ERROR_LOCATION,
          "no matching positions found as the size of peaks is <4");
    }
    throw cpl::core::DataNotFoundError(PYCPL_ERROR_LOCATION,
                                       "no matching positions found");
  }
  return cpl::core::Bivector(res);
}

std::tuple<std::vector<int>, cpl::core::Matrix, cpl::core::Matrix, double,
           double>
match_points(const cpl::core::Matrix& data, size use_data, double err_data,
             const cpl::core::Matrix& pattern, size use_pattern,
             double err_pattern, double tolerance, double radius)
{
  // Intialize return paramters
  cpl_matrix* mpattern = NULL;
  cpl_matrix* mdata = NULL;
  double lin_scale;
  double lin_angle;

  cpl_array* res = cpl::core::Error::throw_errors_with(
      cpl_ppm_match_points, data.ptr(), use_data, err_data, pattern.ptr(),
      use_pattern, err_pattern, tolerance, radius, &mdata, &mpattern,
      &lin_scale, &lin_angle);
  int* res_data = cpl_array_get_data_int(res);  // Store  array data in vector
  size array_size = cpl_array_get_size(res);
  std::vector<int> res_return(res_data, res_data + array_size);
  cpl_array_delete(res);  // delete array struct data
  return std::make_tuple(res_return, cpl::core::Matrix(mdata),
                         cpl::core::Matrix(mpattern), lin_scale, lin_angle);
}

}  // namespace drs
}  // namespace cpl