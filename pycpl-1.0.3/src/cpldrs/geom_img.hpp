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

#ifndef PYCPL_GEOM_IMG_HPP
#define PYCPL_GEOM_IMG_HPP

#include <memory>
#include <optional>
#include <tuple>

// FIXME: cpl_geom_img.h is not self contained. It lacks the include
//        directive for cpl_imagelist.h. The following include works
//        around this issue, but this include must be kept before
//        cpl_geom_img.h is included for CPL versions prior to
//        version 7.3.2!
#include <cpl_version.h>
#if CPL_VERSION_CODE <= CPL_VERSION(7, 3, 3)
#include <cpl_imagelist.h>
#endif
#include <cpl_geom_img.h>
#include <cpl_vector.h>

#include "cplcore/bivector.hpp"
#include "cplcore/image.hpp"
#include "cplcore/imagelist.hpp"
#include "cplcore/vector.hpp"

namespace cpl
{
namespace drs
{
namespace geom
{

using size = cpl::core::size;

/**
 * @brief Get the offsets by correlating the images
 * @param ilist Input image list
 * @param estimates First-guess estimation of the offsets
 * @param anchors List of anchor points
 * @param s_hx Half-width of search area
 * @param s_hy Half-height of search area
 * @param m_hx Half-width of measurement area
 * @param m_hy Half-height of measurement area
 *
 * The matching is performed using a 2d cross-correlation, using a minimal
 * squared differences criterion. One measurement is performed per input anchor
 * point, and the median offset is returned together with a measure of
 * similarity for each plane.
 *
 * The images in the input list must only differ from a shift. In order
 * from the correlation to work, they must have the same level (check the
 * average values of your input images if the correlation does not work).
 *
 * The supported types are CPL_TYPE_DOUBLE and CPL_TYPE_FLOAT.
 * The bad pixel maps are ignored by this function.
 *
 * The ith offset (offsx, offsy) in the returned offsets is the one that have
 * to be used to shift the ith image to align it on the reference image (the
 * first one).
 *
 * If not NULL, the returned cpl_bivector must be deallocated with
 * cpl_bivector_delete().
 *
 * @return tuple of List of offsets and the list of cross-correlation quality
 * factors
 */
std::tuple<cpl::core::Bivector, cpl::core::Vector>
img_offset_fine(const cpl::core::ImageList& ilist,
                const cpl::core::Bivector& estimates,
                const cpl::core::Bivector& anchors, size s_hx, size s_hy,
                size m_hx, size m_hy);

/**
 * @brief Shift and add an images list to a single image
 * @param ilist Input image list
 * @param offs List of offsets in x and y
 * @param kernel Interpolation kernel to use for resampling
 * @param rejmin Number of minimum value pixels to reject in stacking
 * @param rejmax Number of maximum value pixels to reject in stacking
 * @param union_flag Combination mode, CPL_GEOM_UNION, CPL_GEOM_INTERSECT or
 *     CPL_GEOM_FIRST
 * @param ppos_x If non-NULL, *ppos_x is the X-position of the first
 *     image in the combined image
 * @param ppos_y If non-NULL, *ppos_y is the Y- position of the first
 *     image in the combined image
 *
 * The supported types are CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT.
 *
 * The number of provided offsets shall be equal to the number of input images.
 * The ith offset (offs_x, offs_y) is the offset that has to be used to shift
 * the ith image to align it on the first one.
 *
 * Provide the name of the kernel you want to generate. Supported kernel
 * types are:
 * - CPL_KERNEL_DEFAULT: default kernel, currently CPL_KERNEL_TANH
 * - CPL_KERNEL_TANH: Hyperbolic tangent
 * - CPL_KERNEL_SINC: Sinus cardinal
 * - CPL_KERNEL_SINC2: Square sinus cardinal
 * - CPL_KERNEL_LANCZOS: Lanczos2 kernel
 * - CPL_KERNEL_HAMMING: Hamming kernel
 * - CPL_KERNEL_HANN: Hann kernel
 * - CPL_KERNEL_NEAREST: Nearest neighbor kernel (1 when dist < 0.5, else 0)
 *
 * If the number of input images is lower or equal to 3, the rejection
 * parameters are ignored.
 * If the number of input images is lower or equal to 2*(rejmin+rejmax), the
 * rejection parameters are ignored.
 *
 * On success the returned image array contains 2 images:
 * - the combined image
 * - the contribution map
 *
 * Pixels with a zero in the contribution map are flagged as bad in the
 * combined image.
 *
 * If the call is successful (*ppos_x, *ppos_y) is the pixel coordinate in the
 * created output image pair where the lower-leftmost pixel of the first input
 * image is located, where the lower-leftmost pixel of the output image is at
 * (0, 0). Note that this differs from the corresponding CPL function, where the
 * lower-leftmost pixel of the output image is at (1, 1).
 *
 * @return Pointer to newly allocated images array, or NULL on error.
 * @throws IllegalInputError if ilist is invalid or if rejmin or rejmax is
 * @throws IncompatibleInputError if ilist and offs have different sizes
 * @throws IllegalOutputError if the CPL_GEOM_INTERSECT method is used with
 * non-overlapping images.
 * @throws InvalidTypeError if the passed image list type is not supported
 * @throws UnsupportedModeError if the union_flag is not one of the supported
 */
std::tuple<std::shared_ptr<cpl::core::ImageBase>,
           std::shared_ptr<cpl::core::ImageBase>, double, double>
img_offset_saa(const cpl::core::ImageList& ilist,
               const cpl::core::Bivector& offs, cpl_kernel kernel, size rejmin,
               size rejmax, cpl_geom_combine union_flag);

/**
 * @brief Images list recombination
 * @param imlist Input imagelist
 * @param offs List of offsets in x and y
 * @param refine Iff non-zero, the offsets will be refined
 * @param aperts List of correlation apertures or NULL if unknown
 * @param sigmas Positive, decreasing sigmas to apply
 * @param pisigma Index of the sigma that was used or undefined on error
 * @param s_hx Search area half-width.
 * @param s_hy Search area half-height.
 * @param m_hx Measurement area half-width.
 * @param m_hy Measurement area half-height.
 * @param min_rej number of low values to reject in stacking
 * @param max_rej number of high values to reject in stacking
 * @param union_flag Combination mode (CPL_GEOM_UNION or CPL_GEOM_INTERSECT)
 *
 * With offset refinement enabled:
 * This function detects cross correlation points in the first image (if not
 * provided by the user), use them to refine the provided offsets with a cross
 * correlation method, and then apply the shift and add to recombine the images
 * together. Non-correlating images are removed from self.
 *
 * Without offset refinement self is not modified.
 *
 * The supported types are CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT.
 *
 * The number of provided offsets shall be equal to the number of input images.
 * The ith offset (offs_x, offs_y) is the offset that has to be used to shift
 * the ith image to align it on the first one.
 *
 * sigmas may be NULL if offset refinement is disabled or if aperts is non-NULL.
 *
 * On success the returned image array contains 2 images:
 * - the combined image
 * - the contribution map
 *
 *
 * @return Pointer to newly allocated images array, or NULL on error.
 * @throws IllegalInputError if self is not uniform
 * @throws IncompatibleInputError if self and offs have different sizes
 */
std::tuple<std::shared_ptr<cpl::core::ImageBase>,
           std::shared_ptr<cpl::core::ImageBase>, std::optional<size>>
img_offset_combine(const cpl::core::ImageList& ilist,
                   const cpl::core::Bivector& offs, size s_hx, size s_hy,
                   size m_hx, size m_hy, size min_rej, size max_rej,
                   cpl_geom_combine union_flag, bool refine,
                   std::optional<cpl::core::Bivector> aperts,
                   std::optional<cpl::core::Vector> sigmas);

}  // namespace geom
}  // namespace drs
}  // namespace cpl

#endif  // PYCPL_GEOM_IMG_HPP