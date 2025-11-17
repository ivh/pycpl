
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

#ifndef PYCPL_APERTURES_HPP
#define PYCPL_APERTURES_HPP

#include <memory>
#include <string>
#include <tuple>

#include <cpl_apertures.h>

#include "cplcore/coords.hpp"
#include "cplcore/image.hpp"
#include "cplcore/mask.hpp"
#include "cplcore/types.hpp"
#include "cplcore/vector.hpp"

namespace cpl
{
namespace drs
{

using size = cpl::core::size;

class Apertures
{
 public:
  Apertures(cpl_apertures* to_steal);

  /**
   * @brief Destructor for cpl_apertures
   *
   * This function deallocates all memory allocated for the object.
   *

   */
  ~Apertures();

  /**
   * @brief Dump a cpl_apertures to a string.
   *
   * This function dumps all information in a cpl_apertures to a string.
   * If the object is unallocated or contains nothing,
   * this function does nothing.
   *
   * @return A string with the apertures contents.
   */
  std::string dump() const;

  /**
   * @brief Compute statistics on selected apertures
   * @param inImage Reference image
   * @param lab Labelized image (of type CPL_TYPE_INT)
   *
   * The labelized image must contain at least one pixel for each value from 1
   * to the maximum value in the image.
   *
   * For the centroiding computation of an aperture, if some pixels have
   * values lower or equal to 0, all the values of the aperture are locally
   * shifted such as the minimum value of the aperture has a value of
   * epsilon. The centroid is then computed on these positive values. In
   * principle, centroid should always be computed on positive values, this
   * is done to avoid raising an error in case the caller of the function
   * wants to use it on negative values images without caring about the
   * centroid results. In such cases, the centroid result would be
   * meaningful, but slightly depend on the hardcoded value chosen for
   * epsilon (1e-10).
   *
   * @return An CPL apertures object or @em NULL on error
   * @throws TypeMismatchError if lab is not of type CPL_TYPE_INT or
   * @throws IllegalInputError if lab has a negative value or zero maximum
   * @throws IncompatibleInputError if lab and inImage have different sizes.
   */
  Apertures(const cpl::core::ImageBase& inImage,
            const cpl::core::ImageBase& lab);

  /**
   * @brief Get the number of apertures
   *
   * @return The number of apertures or -1 on error
   */
  size get_size() const;

  /**
   * @brief Get the average X-position of an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return The average X-position of the aperture or negative on error
   */
  double get_pos_x(size ind) const;

  /**
   * @brief Get the average Y-position of an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return The average Y-position of the aperture or negative on error
   */
  double get_pos_y(size ind) const;

  /**
   * @brief Get the X-centroid of an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * For a concave aperture the centroid may not belong to the aperture.
   *
   * @return The X-centroid position of the aperture or negative on error
   * @throws IllegalInputError if ind is non-positive
   */
  double get_centroid_x(size ind) const;

  /**
   * @brief Get the Y-centroid of an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return The X-centroid position of the aperture or negative on error
   */
  double get_centroid_y(size ind) const;

  /**
   * @brief Get the X-position of the aperture maximum value
   * @param ind The aperture index (1 for the first one)
   *
   * @return The X-position of the aperture maximum value or negative on error
   */
  size get_maxpos_x(size ind) const;

  /**
   * @brief Get the Y-position of the aperture maximum value
   * @param ind The aperture index (1 for the first one)
   *
   * @return The Y-position of the aperture maximum value or negative on error
   */
  size get_maxpos_y(size ind) const;

  /**
   * @brief Get the X-position of the aperture minimum value
   * @param ind The aperture index (1 for the first one)
   *
   * @return The X-position of the aperture minimum value or negative on error
   */
  size get_minpos_x(size ind) const;

  /**
   * @brief Get the Y-position of the aperture minimum value
   * @param ind The aperture index (1 for the first one)
   *
   * @return The Y-position of the aperture minimum value or negative on error
   */
  size get_minpos_y(size ind) const;

  /**
   * @brief Get the number of pixels of an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return The number of pixels of the aperture or negative on error
   * @throws IllegalInputError if ind is non-positive
   */
  size get_npix(size ind) const;

  /**
   * @brief Get the leftmost x position in an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return the leftmost x position of the aperture or negative on error
   */
  size get_left(size ind) const;

  /**
   * @brief Get the y position of the leftmost x position in an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return the y position of the leftmost x position or negative on error
   */
  size get_left_y(size ind) const;

  /**
   * @brief Get the rightmost x position in an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return the rightmost x position in an aperture or negative on error
   */
  size get_right(size ind) const;

  /**
   * @brief Get the y position of the rightmost x position in an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return the y position of the rightmost x position or negative on error
   */
  size get_right_y(size ind) const;

  /**
   * @brief Get the x position of the topmost y position in an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return the x position of the topmost y position or negative on error
   */
  size get_top_x(size ind) const;

  /**
   * @brief Get the topmost y position in an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return the topmost y position in the aperture or negative on error
   */
  size get_top(size ind) const;

  /**
   * @brief Get the x position of the bottommost y position in an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return the bottommost x position of the aperture or negative on error
   */
  size get_bottom_x(size ind) const;

  /**
   * @brief Get the bottommost y position in an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return the bottommost y position in the aperture or negative on error
   */
  size get_bottom(size ind) const;

  /**
   * @brief Get the maximum value of an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return The maximum value of the aperture or undefined on error
   * @throws IllegalInputError if ind is non-positive
   */
  double get_max(size ind) const;

  /**
   * @brief Get the minimum value of an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return The minimum value of the aperture or undefined on error
   */
  double get_min(size ind) const;

  /**
   * @brief Get the mean value of an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return The mean value of the aperture or undefined on error
   */
  double get_mean(size ind) const;

  /**
   * @brief Get the median value of an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return The median value of the aperture or undefined on error
   */
  double get_median(size ind) const;

  /**
   * @brief Get the standard deviation of an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return The standard deviation of the aperture or negative on error
   * @throws IllegalInputError if ind is non-positive
   * @throws AccessOutOfRangeError if ind exceeds the number of apertures in
   * inImage
   */
  double get_stdev(size ind) const;

  /**
   * @brief Get the flux of an aperture
   * @param ind The aperture index (1 for the first one)
   *
   * @return The flux of the aperture or undefined on error
   */
  double get_flux(size ind) const;

  /**
   * @brief Sort by decreasing aperture size and apply changes
   *
   */
  void sort_by_npix();

  /**
   * @brief Sort by decreasing aperture peak value and apply changes
   *

   */
  void sort_by_max();

  /**
   * @brief Sort by decreasing aperture flux and apply changes
   *
   */
  void sort_by_flux();

  /**
   * @brief Simple detection of apertures in an image
   * @param sigmas Positive, decreasing sigmas to apply
   * @param pisigma  or unchanged on error
   *
   * @return Tuple of the detected apertures and Index of the sigma that was
   * used
   */
  static std::tuple<std::shared_ptr<Apertures>, size>
  extract(const cpl::core::ImageBase& inImage,
          const cpl::core::Vector& sigmas);  //, size& pisigma) const;

  /**
   * @brief Simple detection of apertures in an image window
   * @param sigmas Positive, decreasing sigmas to apply
   * @param area Rectangle of the window in the format (llx, lly, urx, ury)
   * where:
   * - llx Lower left x position (FITS convention)
   * - lly Lower left y position (FITS convention)
   * - urx Upper right x position (FITS convention)
   * - ury Upper right y position (FITS convention)
   *
   * @return Tuple of detected apertures and Index of the sigma that was used
   */
  static std::tuple<std::shared_ptr<Apertures>, size>
  extract_window(const cpl::core::ImageBase& inImage,
                 const cpl::core::Vector& sigmas,
                 cpl::core::Window area);  //, size& pisigma) const;

  /**
   * @brief Simple apertures creation from a user supplied selection mask
   * @param selection The mask of selected pixels
   *
   * The values selected for inclusion in the apertures must have the non-zero
   * value in the selection mask, and must not be flagged as bad in the bad
   * pixel map of the image.
   *
   * The input image type can be CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT or
   * CPL_TYPE_INT.
   *
   * @return The list of detected apertures
   * @throws IncompatibleInputError if inImage and selection have different
   * sizes.
   * @throws TypeMismatchError if inImage is of a complex type
   */
  static std::shared_ptr<Apertures>
  extract_mask(const cpl::core::ImageBase& inImage,
               const cpl::core::Mask& selection);

  /**
   * @brief Simple apertures detection in an image using a provided sigma
   * @param sigma Detection level
   *
   * The threshold used for the detection is the median plus the average
   * distance to the median times sigma.
   *
   * The input image type can be CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT or
   * CPL_TYPE_INT.
   *
   * @return The list of detected apertures or NULL on error
   * @throws IllegalInputError if sigma is non-positive
   * @throws TypeMismatchError if inImage is of a complex type
   */
  static std::shared_ptr<Apertures>
  extract_sigma(const cpl::core::ImageBase& inImage, double sigma);

  size iter_idx = 0;  // Used to keep track as an iterator for the python class
                      // to iterate over apertures
 private:
  cpl_apertures* m_interface;
};

}  // namespace drs
}  // namespace cpl

#endif  // PYCPL_APERTURES_HPP