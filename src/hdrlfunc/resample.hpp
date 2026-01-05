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

#ifndef PYHDRL_FUNC_RESAMPLE_HPP_
#define PYHDRL_FUNC_RESAMPLE_HPP_

#include <memory>
#include <string>

#include <cpl_propertylist.h>
#include <hdrl_imagelist.h>
#include <hdrl_parameter.h>
#include <hdrl_resample.h>

#include "hdrlcore/image.hpp"
#include "hdrlcore/imagelist.hpp"
#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace func
{

using hdrl::core::Image;
using hdrl::core::ImageList;
using hdrl::core::pycpl_propertylist;
using hdrl::core::pycpl_table;
using hdrl::core::pycpl_wcs;

class ResampleResult;
class ResampleMethod;
class ResampleOutgrid;

class Resample
{
 public:
  Resample();
  hdrl_parameter* ptr();
  static ResampleResult compute(pycpl_table restable, ResampleMethod method,
                                ResampleOutgrid outputgrid, pycpl_wcs wcs);
  // restable related functions
  static pycpl_table image_to_table(std::shared_ptr<Image> hima, pycpl_wcs wcs);
  static pycpl_table
  imagelist_to_table(std::shared_ptr<ImageList> himlist, pycpl_wcs wcs);
  static pycpl_table restable_template(int nrows);

 protected:
  hdrl_parameter* m_interface;
};

class ResampleMethod : public Resample
{
 public:
  // Nearest
  ResampleMethod();
  // Linear or Quadratic
  ResampleMethod(std::string mode, int loop_distance, bool use_errorweights);
  // Renka
  ResampleMethod(int loop_distance, bool use_errorweights,
                 double critical_radius);
  // Drizzle
  ResampleMethod(int loop_distance, bool use_errorweights, double pix_frac_x,
                 double pix_frac_y, double pix_frac_lambda);
  // Lanczos
  ResampleMethod(int loop_distance, bool use_errorweights, int kernel_size);
};

class ResampleOutgrid : public Resample
{
 public:
  // user defined constructors
  // 2D
  ResampleOutgrid(double delta_ra, double delta_dec, double ra_min,
                  double ra_max, double dec_min, double dec_max,
                  double fieldmargin);
  // 3D
  ResampleOutgrid(double delta_ra, double delta_dec, double delta_lambda,
                  double ra_min, double ra_max, double dec_min, double dec_max,
                  double lambda_min, double lambda_max, double fieldmargin);

  // helper constructors
  // 2D
  ResampleOutgrid(double delta_ra, double delta_dec);
  // 3D
  ResampleOutgrid(double delta_ra, double delta_dec, double delta_lambda);
};

class ResampleResult
{
 public:
  ResampleResult();
  ResampleResult(hdrl_resample_result* res);

  // Direct access to wrapped objects (like catalogue module)
  hdrl::core::pycpl_propertylist hdr;
  std::shared_ptr<ImageList> imlist;
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_RESAMPLE_HPP_