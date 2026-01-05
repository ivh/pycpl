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

#ifndef PYHDRL_FUNC_CATALOGUE_HPP_
#define PYHDRL_FUNC_CATALOGUE_HPP_

#include <hdrl_parameter.h>

#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace func
{

class Catalogue
{
 public:
  // Constructor that creates the parameter
  Catalogue(int obj_min_pixels, double obj_threshold, bool obj_deblending,
            double obj_core_radius, bool bkg_estimate, int bkg_mesh_size,
            double bkg_smooth_fwhm, double det_eff_gain, double det_saturation,
            int resulttype);

  struct Result
  {
    hdrl::core::pycpl_table catalogue;
    hdrl::core::pycpl_image segmentation_map;
    hdrl::core::pycpl_image background;
    hdrl::core::pycpl_propertylist qclist;
  };

  // Compute method
  Result
  compute(hdrl::core::pycpl_image image,
          hdrl::core::pycpl_image confidence_map = hdrl::core::pycpl_image(),
          hdrl::core::pycpl_wcs wcs = hdrl::core::pycpl_wcs());

  // Accessor methods
  int get_obj_min_pixels();
  double get_obj_threshold();
  bool get_obj_deblending();
  double get_obj_core_radius();
  bool get_bkg_estimate();
  int get_bkg_mesh_size();
  double get_bkg_smooth_fwhm();
  double get_det_eff_gain();
  double get_det_saturation();
  int get_resulttype();

  // Get the underlying parameter pointer
  hdrl_parameter* ptr();

 protected:
  hdrl_parameter* m_interface;

  // Store parameter values for getter methods
  int m_obj_min_pixels;
  double m_obj_threshold;
  bool m_obj_deblending;
  double m_obj_core_radius;
  bool m_bkg_estimate;
  int m_bkg_mesh_size;
  double m_bkg_smooth_fwhm;
  double m_det_eff_gain;
  double m_det_saturation;
  int m_resulttype;
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_CATALOGUE_HPP_
