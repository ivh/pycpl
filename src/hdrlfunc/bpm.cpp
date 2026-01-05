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

#include "hdrlfunc/bpm.hpp"

#include <cpl_error.h>
#include <cpl_image_io.h>
#include <cpl_imagelist.h>
#include <cpl_mask.h>
#include <hdrl_bpm_2d.h>
#include <hdrl_bpm_3d.h>
#include <hdrl_bpm_fit.h>
#include <hdrl_bpm_utils.h>

#include "hdrlcore/error.hpp"
#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace func
{

using hdrl::core::Error;
using hdrl::core::IllegalInputError;
using hdrl::core::InvalidTypeError;

BPM::BPM() {}

hdrl_parameter*
BPM::ptr()
{
  return m_interface;
}

pycpl_mask
BPM::filter(pycpl_mask input_mask, cpl_size kernel_nx, cpl_size kernel_ny,
            cpl_filter_mode filter)
{
  cpl_mask* m = Error::throw_errors_with(hdrl_bpm_filter, input_mask.m,
                                         kernel_nx, kernel_ny, filter);
  return pycpl_mask(m);
}

pycpl_imagelist
BPM::filter_list(pycpl_imagelist inlist, cpl_size kernel_nx, cpl_size kernel_ny,
                 cpl_filter_mode filter)
{
  cpl_imagelist* il = Error::throw_errors_with(hdrl_bpm_filter_list, inlist.il,
                                               kernel_nx, kernel_ny, filter);
  return pycpl_imagelist(il);
}

// hdrl_bpm_to_mask is in a private class
// pycpl_mask
// BPM::to_mask(pycpl_image bpm, uint64_t selection)
// {
//     cpl_mask* m = Error::throw_errors_with(hdrl_bpm_to_mask, bpm.im,
//     selection); return pycpl_mask(m);
// }

// BPM2D
BPM2D::BPM2D(double kappa_low, double kappa_high, int maxiter,
             cpl_filter_mode filter, cpl_border_mode border, int smooth_x,
             int smooth_y)
{
  m_interface = Error::throw_errors_with(
      hdrl_bpm_2d_parameter_create_filtersmooth, kappa_low, kappa_high, maxiter,
      filter, border, smooth_x, smooth_y);
  // verify that the parameters make sense
  cpl_error_code err = hdrl::core::Error::throw_errors_with(
      hdrl_bpm_2d_parameter_verify, m_interface);
}

BPM2D::BPM2D(double kappa_low, double kappa_high, int maxiter, int steps_x,
             int steps_y, int filter_size_x, int filter_size_y, int order_x,
             int order_y)
{
  m_interface =
      Error::throw_errors_with(hdrl_bpm_2d_parameter_create_legendresmooth,
                               kappa_low, kappa_high, maxiter, steps_x, steps_y,
                               filter_size_x, filter_size_y, order_x, order_y);
  // verify that the parameters make sense
  cpl_error_code err = hdrl::core::Error::throw_errors_with(
      hdrl_bpm_2d_parameter_verify, m_interface);
}

hdrl::core::pycpl_mask
BPM2D::compute(std::shared_ptr<hdrl::core::Image> img_in)
{
  if (img_in == nullptr) {
    throw InvalidTypeError(HDRL_ERROR_LOCATION,
                           "Image provided to BPM2D compute must not be None");
  }
  cpl_mask* mask = Error::throw_errors_with(hdrl_bpm_2d_compute,
                                            img_in.get()->ptr(), m_interface);
  return hdrl::core::pycpl_mask(mask);
}

cpl_filter_mode
BPM2D::get_filter()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_filter,
                                  m_interface);
}

cpl_border_mode
BPM2D::get_border()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_border,
                                  m_interface);
}

double
BPM2D::get_kappa_low()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_kappa_low,
                                  m_interface);
}

double
BPM2D::get_kappa_high()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_kappa_high,
                                  m_interface);
}

int
BPM2D::get_maxiter()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_maxiter,
                                  m_interface);
}

int
BPM2D::get_steps_x()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_steps_x,
                                  m_interface);
}

int
BPM2D::get_steps_y()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_steps_y,
                                  m_interface);
}

int
BPM2D::get_filter_size_x()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_filter_size_x,
                                  m_interface);
}

int
BPM2D::get_filter_size_y()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_filter_size_y,
                                  m_interface);
}

int
BPM2D::get_order_x()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_order_x,
                                  m_interface);
}

int
BPM2D::get_order_y()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_order_y,
                                  m_interface);
}

int
BPM2D::get_smooth_x()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_smooth_x,
                                  m_interface);
}

int
BPM2D::get_smooth_y()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_smooth_y,
                                  m_interface);
}

hdrl_bpm_2d_method
BPM2D::get_method()
{
  return Error::throw_errors_with(hdrl_bpm_2d_parameter_get_method,
                                  m_interface);
}

// BPM3D
BPM3D::BPM3D(double kappa_low, double kappa_high, hdrl_bpm_3d_method method)
{
  m_interface = Error::throw_errors_with(hdrl_bpm_3d_parameter_create,
                                         kappa_low, kappa_high, method);
  // verify that the parameters make sense
  cpl_error_code err = hdrl::core::Error::throw_errors_with(
      hdrl_bpm_3d_parameter_verify, m_interface);
}

pycpl_imagelist
BPM3D::compute(std::shared_ptr<ImageList> imglist_in)
{
  if (imglist_in == nullptr) {
    throw InvalidTypeError(
        HDRL_ERROR_LOCATION,
        "ImageList provided to BPM3D compute must not be None");
  }
  cpl_imagelist* imlist = Error::throw_errors_with(
      hdrl_bpm_3d_compute, imglist_in.get()->ptr(), m_interface);
  return pycpl_imagelist(imlist);
}

double
BPM3D::get_kappa_low()
{
  return Error::throw_errors_with(hdrl_bpm_3d_parameter_get_kappa_low,
                                  m_interface);
}

double
BPM3D::get_kappa_high()
{
  return Error::throw_errors_with(hdrl_bpm_3d_parameter_get_kappa_high,
                                  m_interface);
}

hdrl_bpm_3d_method
BPM3D::get_method()
{
  return Error::throw_errors_with(hdrl_bpm_3d_parameter_get_method,
                                  m_interface);
}

// BPMFit
BPMFit::BPMFit(int degree, double pval)
{
  m_interface = Error::throw_errors_with(hdrl_bpm_fit_parameter_create_pval,
                                         degree, pval);
  // verify that the parameters make sense
  cpl_error_code err = hdrl::core::Error::throw_errors_with(
      hdrl_bpm_fit_parameter_verify, m_interface);
}

BPMFit::BPMFit(std::string rel, int degree, double low, double high)
{
  if (rel == "coef") {
    m_interface = Error::throw_errors_with(
        hdrl_bpm_fit_parameter_create_rel_coef, degree, low, high);
  } else if (rel == "chi") {
    m_interface = Error::throw_errors_with(
        hdrl_bpm_fit_parameter_create_rel_chi, degree, low, high);
  } else {
    throw IllegalInputError(
        HDRL_ERROR_LOCATION,
        "Invalid rel type for hdrl.func.BPMFit. Expected coef or chi.");
  }
  // verify that the parameters make sense
  cpl_error_code err = hdrl::core::Error::throw_errors_with(
      hdrl_bpm_fit_parameter_verify, m_interface);
}

pycpl_image
BPMFit::compute(std::shared_ptr<ImageList> imglist_in,
                pycpl_vector sample_position)
{
  if (imglist_in == nullptr) {
    throw InvalidTypeError(
        HDRL_ERROR_LOCATION,
        "ImageList provided to BPMFit compute must not be None");
  }
  cpl_image* out_mask;
  cpl_error_code err = Error::throw_errors_with(
      hdrl_bpm_fit_compute, m_interface, imglist_in.get()->ptr(),
      sample_position.v, &out_mask);
  return pycpl_image(out_mask);
}

int
BPMFit::get_degree()
{
  return Error::throw_errors_with(hdrl_bpm_fit_parameter_get_degree,
                                  m_interface);
}

double
BPMFit::get_pval()
{
  return Error::throw_errors_with(hdrl_bpm_fit_parameter_get_pval, m_interface);
}

double
BPMFit::get_rel_chi_low()
{
  return Error::throw_errors_with(hdrl_bpm_fit_parameter_get_rel_chi_low,
                                  m_interface);
}

double
BPMFit::get_rel_chi_high()
{
  return Error::throw_errors_with(hdrl_bpm_fit_parameter_get_rel_chi_high,
                                  m_interface);
}

double
BPMFit::get_rel_coef_low()
{
  return Error::throw_errors_with(hdrl_bpm_fit_parameter_get_rel_coef_low,
                                  m_interface);
}

double
BPMFit::get_rel_coef_high()
{
  return Error::throw_errors_with(hdrl_bpm_fit_parameter_get_rel_coef_high,
                                  m_interface);
}

}  // namespace func
}  // namespace hdrl
