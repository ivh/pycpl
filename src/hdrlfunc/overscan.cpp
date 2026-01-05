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

#include "hdrlfunc/overscan.hpp"

#include <stdexcept>

#include <cpl_image.h>
#include <hdrl_collapse.h>
#include <hdrl_overscan.h>
#include <hdrl_parameter.h>

#include "hdrlcore/error.hpp"
#include "hdrlcore/pycpl_image.hpp"  // IWYU pragma: keep
#include "hdrlcore/pycpl_types.hpp"
#include "hdrlcore/pycpl_window.hpp"  // IWYU pragma: keep

namespace hdrl
{
namespace func
{

Overscan::Overscan(const std::string& direction, double ccd_ron, int box_hsize,
                   Collapse collapse,
                   const hdrl::core::pycpl_window& overscan_region)
    : m_direction(direction), m_ccd_ron(ccd_ron), m_box_hsize(box_hsize),
      m_overscan_region(overscan_region)  // Save region

{
  if (direction == "x") {
    m_corr_dir = HDRL_X_AXIS;
  } else if (direction == "y") {
    m_corr_dir = HDRL_Y_AXIS;
  } else {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "Invalid correction direction");
  }

  // Use struct fields directly
  hdrl_parameter* region_param = hdrl_rect_region_parameter_create(
      overscan_region.llx, overscan_region.lly, overscan_region.urx,
      overscan_region.ury);

  // Create Overscan interface parameter
  m_interface = hdrl::core::Error::throw_errors_with(
      hdrl_overscan_parameter_create, m_corr_dir, ccd_ron, box_hsize,
      collapse.ptr(), region_param);
}

py::object
Overscan::compute(std::shared_ptr<hdrl::core::Image> input_image)
{
  if (!input_image) {
    throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION,
                                     "Received null input image!");
  }

  if (m_interface == nullptr) {
    throw std::runtime_error("Overscan interface is null (m_interface).");
  }

  hdrl_image* source = input_image->ptr();

  if (!source) {
    throw hdrl::core::NullInputError(
        HDRL_ERROR_LOCATION,
        "Input hdrl_image* is null. Did you initialize the image correctly?");
  }

  cpl_image* cpl_src = hdrl_image_get_image(source);

  if (!cpl_src) {
    throw hdrl::core::NullInputError(
        HDRL_ERROR_LOCATION, "Cannot retrieve cpl_image* from hdrl_image.");
  }

  hdrl_overscan_compute_result* result = hdrl::core::Error::throw_errors_with(
      hdrl_overscan_compute, cpl_src, m_interface);

  if (!result) {
    throw std::runtime_error("hdrl_overscan_compute returned null result.");
  }

  m_result = result;

  hdrl_image* correction_img =
      hdrl_overscan_compute_result_get_correction(result);
  cpl_image* contribution =
      hdrl_overscan_compute_result_get_contribution(result);

  if (!correction_img || !contribution) {
    throw std::runtime_error(
        "Overscan result missing correction or contribution image.");
  }

  // Get additional diagnostic images
  cpl_image* chi2_img = hdrl_overscan_compute_result_get_chi2(result);
  cpl_image* red_chi2_img = hdrl_overscan_compute_result_get_red_chi2(result);

  hdrl_parameter* collapse = hdrl_overscan_parameter_get_collapse(m_interface);

  cpl_image* sigclip_reject_low = nullptr;
  cpl_image* sigclip_reject_high = nullptr;
  if (hdrl_collapse_parameter_is_sigclip(collapse)) {
    sigclip_reject_low =
        hdrl_overscan_compute_result_get_sigclip_reject_low(result);
    sigclip_reject_high =
        hdrl_overscan_compute_result_get_sigclip_reject_high(result);
  }
  cpl_image* minmax_reject_low = nullptr;
  cpl_image* minmax_reject_high = nullptr;
  if (hdrl_collapse_parameter_is_minmax(collapse)) {
    minmax_reject_low =
        hdrl_overscan_compute_result_get_minmax_reject_low(result);
    minmax_reject_high =
        hdrl_overscan_compute_result_get_minmax_reject_high(result);
  }

  py::module_ col = py::module_::import("collections");
  py::object ntup =
      col.attr("namedtuple")("OverscanResult",
                             "correction contribution chi2 red_chi2 "
                             "sigclip_reject_low sigclip_reject_high "
                             "minmax_reject_low minmax_reject_high");

  return ntup(std::make_shared<hdrl::core::Image>(
                  hdrl::core::Image(hdrl_image_duplicate(correction_img))),
              hdrl::core::pycpl_image(contribution),
              hdrl::core::pycpl_image(chi2_img),
              hdrl::core::pycpl_image(red_chi2_img),
              hdrl::core::pycpl_image(sigclip_reject_low),
              hdrl::core::pycpl_image(sigclip_reject_high),
              hdrl::core::pycpl_image(minmax_reject_low),
              hdrl::core::pycpl_image(minmax_reject_high));
}

py::object
Overscan::correct(std::shared_ptr<hdrl::core::Image> input_image,
                  std::optional<hdrl::core::pycpl_window> region)
{
  if (!input_image) {
    throw std::runtime_error("Input image is null (shared_ptr).");
  }

  if (input_image->ptr() == nullptr) {
    throw std::runtime_error("Image pointer is null.");
  }

  if (m_result == nullptr) {
    throw core::NullInputError(
        HDRL_ERROR_LOCATION,
        "Overscan result not available. Call compute() first.");
  }

  hdrl_image* himg = input_image->ptr();  // Access internal hdrl_image*

  hdrl::core::pycpl_window r;

  if (region.has_value()) {
    r = region.value();
  } else {
    r = m_overscan_region;  // or hdrl::core::pycpl_window::All if you prefer a
                            // default region
  }

  hdrl_parameter* region_param =
      hdrl_rect_region_parameter_create(r.llx, r.lly, r.urx, r.ury);

  hdrl_overscan_correct_result* correct_result =
      hdrl::core::Error::throw_errors_with(hdrl_overscan_correct, himg,
                                           region_param, m_result);

  hdrl_image* corrected =
      hdrl_overscan_correct_result_get_corrected(correct_result);

  cpl_image* badmask = hdrl_overscan_correct_result_get_badmask(correct_result);

  if (!corrected || !badmask) {
    throw std::runtime_error(
        "Overscan correction failed: missing corrected image or badmask.");
  }

  if (region_param != nullptr) {
    hdrl_parameter_delete(region_param);
  }

  py::module_ col = py::module_::import("collections");
  py::object ntup =
      col.attr("namedtuple")("OverscanCorrectResult", "corrected badmask");
  py::object tresult =
      ntup(std::make_shared<hdrl::core::Image>(
               corrected),                     // already takes hdrl_image*
           hdrl::core::pycpl_image(badmask));  // wraps raw cpl_image*

  return tresult;
}

hdrl_parameter*
Overscan::ptr()
{
  return m_interface;
}

hdrl::core::pycpl_window
Overscan::get_overscan_region() const
{
  return m_overscan_region;
}

}  // namespace func
}  // namespace hdrl
