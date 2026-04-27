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

#include "hdrlfunc/catalogue.hpp"

#include <cpl_image.h>
#include <cpl_type.h>
#include <cpl_wcs.h>

#include <hdrl_catalogue.h>

namespace hdrl
{
namespace func
{

Catalogue::Catalogue(int obj_min_pixels, double obj_threshold,
                     bool obj_deblending, double obj_core_radius,
                     bool bkg_estimate, int bkg_mesh_size,
                     double bkg_smooth_fwhm, double det_eff_gain,
                     double det_saturation, int resulttype)
    : m_obj_min_pixels(obj_min_pixels), m_obj_threshold(obj_threshold),
      m_obj_deblending(obj_deblending), m_obj_core_radius(obj_core_radius),
      m_bkg_estimate(bkg_estimate), m_bkg_mesh_size(bkg_mesh_size),
      m_bkg_smooth_fwhm(bkg_smooth_fwhm), m_det_eff_gain(det_eff_gain),
      m_det_saturation(det_saturation), m_resulttype(resulttype)
{
  // Convert bool to cpl_boolean
  cpl_boolean obj_deblending_cpl = obj_deblending ? CPL_TRUE : CPL_FALSE;
  cpl_boolean bkg_estimate_cpl = bkg_estimate ? CPL_TRUE : CPL_FALSE;

  // Convert int to hdrl_catalogue_options enum
  hdrl_catalogue_options resulttype_enum =
      static_cast<hdrl_catalogue_options>(resulttype);

  // Use Error::throw_errors_with pattern for automatic error handling
  m_interface = hdrl::core::Error::throw_errors_with(
      hdrl_catalogue_parameter_create, obj_min_pixels, obj_threshold,
      obj_deblending_cpl, obj_core_radius, bkg_estimate_cpl, bkg_mesh_size,
      bkg_smooth_fwhm, det_eff_gain, det_saturation, resulttype_enum);
}

Catalogue::Result
Catalogue::compute(hdrl::core::pycpl_image image,
                   hdrl::core::pycpl_image confidence_map,
                   hdrl::core::pycpl_wcs wcs)
{
  hdrl_catalogue_result* result = nullptr;

  // Convert optional parameters to C pointers
  cpl_image* conf_map_ptr = nullptr;
  cpl_wcs* wcs_ptr = nullptr;

  if (confidence_map.im != nullptr) {
    conf_map_ptr = confidence_map.im;
  }

  if (wcs.w != nullptr) {
    wcs_ptr = wcs.w;
  }

  // Use Error::throw_errors_with pattern for automatic error handling
  result = hdrl::core::Error::throw_errors_with(
      hdrl_catalogue_compute, image.im, conf_map_ptr, wcs_ptr, m_interface);

  // Create the result struct with explicit default initialization
  Result catalogue_result = {
      hdrl::core::pycpl_table(),  // catalogue - default constructor sets t =
                                  // nullptr
      hdrl::core::pycpl_image(),  // segmentation_map - default constructor sets
                                  // im = nullptr
      hdrl::core::pycpl_image(),  // background - default constructor sets im =
                                  // nullptr
      hdrl::core::pycpl_propertylist()  // qclist - default constructor sets pl
                                        // = nullptr
  };

  // Only assign catalogue if it was requested in resulttype
  // HDRL_CATALOGUE_CAT_COMPLETE = 4
  if (result->catalogue && (m_resulttype & 4)) {
    catalogue_result.catalogue = hdrl::core::pycpl_table(result->catalogue);
  }
  // If result->catalogue is nullptr or not requested,
  // catalogue_result.catalogue will be default-constructed with t = nullptr

  if (result->segmentation_map) {
    catalogue_result.segmentation_map =
        hdrl::core::pycpl_image(result->segmentation_map);
  }
  // If result->segmentation_map is nullptr, catalogue_result.segmentation_map
  // will be default-constructed with im = nullptr

  if (result->background) {
    catalogue_result.background = hdrl::core::pycpl_image(result->background);
  }
  // If result->background is nullptr, catalogue_result.background will be
  // default-constructed with im = nullptr

  if (result->qclist) {
    catalogue_result.qclist = hdrl::core::pycpl_propertylist(result->qclist);
  }
  // If result->qclist is nullptr, catalogue_result.qclist will be
  // default-constructed with pl = nullptr

  // Clean up the C result structure
  hdrl::core::Error::throw_errors_with(hdrl_catalogue_result_delete, result);

  return catalogue_result;
}

// Accessor methods - return stored parameter values
int
Catalogue::get_obj_min_pixels()
{
  return m_obj_min_pixels;
}

double
Catalogue::get_obj_threshold()
{
  return m_obj_threshold;
}

bool
Catalogue::get_obj_deblending()
{
  return m_obj_deblending;
}

double
Catalogue::get_obj_core_radius()
{
  return m_obj_core_radius;
}

bool
Catalogue::get_bkg_estimate()
{
  return m_bkg_estimate;
}

int
Catalogue::get_bkg_mesh_size()
{
  return m_bkg_mesh_size;
}

double
Catalogue::get_bkg_smooth_fwhm()
{
  return m_bkg_smooth_fwhm;
}

double
Catalogue::get_det_eff_gain()
{
  return m_det_eff_gain;
}

double
Catalogue::get_det_saturation()
{
  return m_det_saturation;
}

int
Catalogue::get_resulttype()
{
  return m_resulttype;
}

hdrl_parameter*
Catalogue::ptr()
{
  return m_interface;
}

}  // namespace func
}  // namespace hdrl
