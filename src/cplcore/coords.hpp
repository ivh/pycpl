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


// Utility structs (not bindings) and functions for more C++-idiomatic usage
// of CPL's cpl_image, cpl_mask, and related functions.

#ifndef PYCPL_COORDS_HPP
#define PYCPL_COORDS_HPP

#include <utility>

#include "cplcore/types.hpp"

namespace cpl
{
namespace core
{

using size = cpl::core::size;

/**
 * @brief Converts C++/Python friendly image/mask coorindates
 *       to a CPL coordinate, by -1'ing both coordinates.
 *
 * This should be used when calling cpl_functions in the C++ code.
 * NOT to be used for sizes or dimensions
 *
 * @see cpl_to_coord Inverse of this
 *
 * @example
 *     int Image::get_pixel(size x, size y) {
 *         std::pair<size, size> converted = cpl_coord(x, y);
 *         int result = cpl_image_get_pixel(self, converted.first,
 * converted.second);
 *         ...
 *
 * @code
 * std::pair<size, size> initial;
 * std::pair<size, size> once = cpl_coord(initial.first, initial.second);
 * std::pair<size, size> twice = cpl_to_coord(once.first, once.second);
 * assert(twice.first == initial.first);
 * assert(twice.second == initial.second);
 * @endcode
 */
std::pair<size, size> cpl_coord(size x, size y);

/**
 * @brief Converts CPL coordinate for an image/mask to
 *        0-indexed C++/Python friendly coordinate.
 *
 * This should be used when cpl_functions return coordinates
 * NOT to be used for sizes or dimensions
 *
 * @see cpl_coord Inverse of this
 *
 * @code
 * std::pair<size, size> initial;
 * std::pair<size, size> once = cpl_coord(initial.first, initial.second);
 * std::pair<size, size> twice = cpl_to_coord(once.first, once.second);
 * assert(twice.first == initial.first);
 * assert(twice.second == initial.second);
 * @endcode
 */
std::pair<size, size> cpl_to_coord(size x, size y);

/**
 * @brief This expands to all properties of
 *        the given window, in order that they are mostly used by cpl.
 *        This also does conversion to CPL's 1-indexed coordinates
 *        This does NOT work for Window::All
 */
#define EXPAND_WINDOW(WINDOW) \
  WINDOW.llx + 1, WINDOW.lly + 1, WINDOW.urx + 1, WINDOW.ury + 1

/**
 * @brief represents the coordinates of a rectangle within a 2d pixel space.
 *        Used as arguments to many functions.
 *
 * Aims to be a substitute for where cpl functions might take 4 cpl_size
 * arguments, C++ functions take this class. This represents the coordinates of
 * a rectangle within a 2d pixel space. A macro, EXPAND_WINDOW, expands to the 4
 * coordiantes in the order that they are most commonly used in cpl_*_window
 * functions
 *
 * Window::All expands to the full size of the image it is used with.
 */
struct Window
{
  size llx;
  size lly;
  size urx;
  size ury;

  /**
   * @brief A constant value that is converted to the largest a
   *        window could possibly be when this is used in a PyCPL function.
   *
   * This functionality is not part of CPL itself, however many functions
   * have a window version and a non-window version, which may be chosen
   * based on if window == Window::All.
   *
   * Because cpl functions don't accept inputs where llx > urx or lly > ury,
   * we can one of these invalid data points to denote this Window::All
   * whilst not imfringing on valid windows. (Also allows better debugging
   * when you realise you haven't accounted for ==Window::All possibility)
   *
   * Windows' coordinates should never be negative, and we try to avoid
   * something that might come up accidentally like MAX_INT or MIN_INT
   */
  static const Window All;

  bool operator==(const Window& other) const noexcept;
  bool operator!=(const Window& other) const noexcept;
};

std::ostream& operator<<(std::ostream& os, const Window& other) noexcept;

}  // namespace core
}  // namespace cpl

#endif  // PYCPL_COORDS_HPP