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

#ifndef PYHDRL_FUNC_BPM_HPP_
#define PYHDRL_FUNC_BPM_HPP_

#include <memory>
#include <string>

#include <cpl_filter.h>
#include <cpl_type.h>
#include <hdrl_bpm_2d.h>
#include <hdrl_bpm_3d.h>
#include <hdrl_parameter.h>

#include "hdrlcore/image.hpp"
#include "hdrlcore/imagelist.hpp"
#include "hdrlcore/pycpl_types.hpp"

namespace hdrl
{
namespace func
{

using hdrl::core::Image;
using hdrl::core::ImageList;
using hdrl::core::pycpl_image;
using hdrl::core::pycpl_imagelist;
using hdrl::core::pycpl_mask;
using hdrl::core::pycpl_vector;

// Base class to hold static functions and other common aspects for the other
// inherited classes
class BPM
{
 public:
  BPM();
  hdrl_parameter* ptr();
  // Utils
  /*
  @brief Allows the growing and shrinking of bad pixel masks. It can be used to
  e.g. set pixels to bad if the pixel is surrounded by other bad pixels.
  @param input_mask input mask
  @param kernel_nx  size in x-direction of the filtering kernel
  @param kernel_ny  size in y-direction of the filtering kernel
  Supported modes:
  cpl.core.Filter.EROSION, cpl.core.Filter.DILATION, cpl.core.Filter.OPENING,
  cpl.core.Filter.CLOSING

  @see filter_list()
  The algorithm assumes, that all pixels outside the mask are good, i.e. it
  enlarges the mask by the kernel size and marks this border as good. It
  applies on the enlarged mask the operation and extract the original-size
  mask at the very end.

   */
  static pycpl_mask filter(pycpl_mask input_mask, cpl_size kernel_nx,
                           cpl_size kernel_ny, cpl_filter_mode filter);

  /*
  @brief Wrapper around bpm filter() to filter list of images
  @param inlist     input image list
  @param kernel_nx  size in x-direction of the filtering kernel
  @param kernel_ny  size in y-direction of the filtering kernel
  @return   The filtered image list
  @see filter()
  */
  static pycpl_imagelist
  filter_list(pycpl_imagelist inlist, cpl_size kernel_nx, cpl_size kernel_ny,
              cpl_filter_mode filter);

  // static pycpl_mask to_mask(pycpl_image bpm, uint64_t selection);

 protected:
  hdrl_parameter* m_interface;
};

class BPM2D : public BPM
{
 public:
  // filtersmooth
  BPM2D(double kappa_low, double kappa_high, int maxiter,
        cpl_filter_mode filter, cpl_border_mode border, int smooth_x,
        int smooth_y);
  // legendresmooth
  BPM2D(double kappa_low, double kappa_high, int maxiter, int steps_x,
        int steps_y, int filter_size_x, int filter_size_y, int order_x,
        int order_y);
  pycpl_mask compute(std::shared_ptr<Image> img_in);
  // Accessors
  cpl_filter_mode get_filter();
  cpl_border_mode get_border();
  double get_kappa_low();
  double get_kappa_high();
  int get_maxiter();
  int get_steps_x();
  int get_steps_y();
  int get_filter_size_x();
  int get_filter_size_y();
  int get_order_x();
  int get_order_y();
  int get_smooth_x();
  int get_smooth_y();
  hdrl_bpm_2d_method get_method();
};

class BPM3D : public BPM
{
 public:
  /*
  @brief    Creates BPM object for the imagelist method
  @param    kappa_low       Low kappa factor for thresholding algorithm
  @param    kappa_high      High kappa factor for thresholding algorithm
  @param    method          used method
  @return   The BPM_3D object.

  @see      bpm3d.compute()
  */
  BPM3D(double kappa_low, double kappa_high, hdrl_bpm_3d_method method);

  /*
    @brief detect bad pixels on a stack of identical images
    @param imglist   input hdrl imagelist
    @return pycpl_imagelist where the newly rejected pixels are marked as unity.

    @note We assume that the images are already scaled outside this routine,
    i.e. their absolute levels match.

    @details For a Gaussian distribution the Median Absolute Deviation (MAD) is
    a robust and consistent estimate of the Standard Deviation (STD) in the
    sense that the STD is approximately K * MAD, where K is a constant equal to
    approximately 1.4826
  */
  pycpl_imagelist compute(std::shared_ptr<ImageList> imglist_in);
  // Accessors
  double get_kappa_low();
  double get_kappa_high();
  hdrl_bpm_3d_method get_method();
};

class BPMFit : public BPM
{
 public:
  // pval
  /*
   * @brief create bpm_fit parameter with p-value bpm treshold
   * @param degree  degree of fit
   * @param pval    p-value of bpm threshold
   * @return Instances of the BPMFit objects
   * @see BPMFit.compute()
   */
  BPMFit(int degree, double pval);

  // rel_chi and rel_coef
  /*
   * @brief create bpm_fit parameter with relative chi bpm treshold
   * @param degree  degree of fit
   * @param low relative chi distribution bpm lower threshold
   * @param high relative chi distribution bpm upper threshold
   * @return Instances of the BPMFit objects
   * @see BPMFit.compute()
   */
  BPMFit(std::string rel, int degree, double low, double high);

  /*
    @brief compute bad pixel map based on fitting a stack of images
    @param imglist_in    hdrl_imagelist to fit
    @param sample_position  cpl_vector of sampling position of the images in
    data, e.g. exposure time
    @return out_mask    output bad pixel mask as integer image

    The function fits a polynomial of given degree to
    the imagelist at the sampling positions defined in sample_position.
  */
  pycpl_image
  compute(std::shared_ptr<ImageList> imglist_in, pycpl_vector sample_position);

  // Accessors
  int get_degree();
  double get_pval();
  double get_rel_chi_low();
  double get_rel_chi_high();
  double get_rel_coef_low();
  double get_rel_coef_high();
};

}  // namespace func
}  // namespace hdrl

#endif  // PYHDRL_FUNC_BPM_HPP_
