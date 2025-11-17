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

/**
 * Wraps the cpl_mask struct as a C++ class cpl::core::Mask
 * Implementing all operations that a cpl_struct can do, except FITS file
 * operations. (These are left to python/user choice of C++ FITS library)
 */

#ifndef PYCPL_MASK_HPP
#define PYCPL_MASK_HPP

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <cpl_mask.h>

#include "cplcore/coords.hpp"
#include "cplcore/propertylist.hpp"
#include "cplcore/types.hpp"

namespace cpl
{
namespace core
{

using size = cpl::core::size;

class ImageBase;  // forward declaration

/**
 * @brief a bitmask for a 2-dimensional image.
 *
 * The null state of this class raises NullInputError when
 * most functions are called on it, except assignment to it.
 *
 * There are 2 'states' that a Mask object can be in:
 * 1. Owning state:
 *    In this state, lifetime of the underlying cpl_mask* is directly tied to
 * that of the C++ Mask object.
 * 2. Borrowing state:
 *    In this state, the lifetime of the cpl_mask* is left up to someone else.
 *    It is required to be of a larger lifetime than this C++ class, or
 * undefined behaviour occurs. This usually happens when this mask is added
 * to/retrieved from an Image. (Since images own their masks, there is no way to
 * have multiple owners)
 *
 *    A custom destructor function allows the user of this class to keep an
 * arbitrary object attached to the lifetime of this object, e.g. a
 * shared_ptr<Image> of the owning image.
 */
class Mask
{
 public:
  Mask(cpl_mask* to_steal) noexcept;
  Mask(const Mask& other);
  Mask(Mask&& other) noexcept;
  Mask& operator=(const Mask& other);
  Mask& operator=(Mask&& other) noexcept;

  /**
   * @brief Construct a bitmask of the given dimensions,
   * optionally taking ownership of an existing bit buffer.
   *
   * If the sizes do not match the bit buffer, or are not
   * positive, an IllegalInputError is thrown.
   */
  Mask(size width, size height, unsigned char* bitmask = nullptr);

  /**
   * @brief Construct a bitmask of the given dimensions,
   *        copying the given buffer of bytes.
   *
   * The bitmask should be a string of 0x00s and 0x01s ONLY.
   *
   * If the size of the bitmask doesn't match width * height,
   * an IllegalInputError is thrown.
   */
  Mask(size width, size height, std::string bitmask);

  ~Mask();

  /*----------------------------------------------------------------------------*/
  /**
   *
    @brief    Creates a new mask using image thresholds
    @param    in      Image to threshold.
    @param    lo_cut  Lower bound for threshold.
    @param    hi_cut  Higher bound for threshold.
    @return   1 newly allocated mask or NULL on error
    @note The returned mask must be deallocated with cpl_mask_delete()

  */
  /*----------------------------------------------------------------------------*/
  Mask(const ImageBase& in, double lo_cut, double hi_cut);

  /*----------------------------------------------------------------------------*/
  /**
   *
    @brief    Select parts of an image with provided thresholds.
    @param    image   Image to threshold.
    @param    lo_cut  Lower bound for threshold.
    @param    hi_cut  Higher bound for threshold.
    @param    inval this value (0 or 1, false or true) is assigned where
                    the pixel value is not marked as rejected and is strictly
                    inside the provided interval.
                    The other positions are assigned the other value.
    The input image type can be CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT or CPL_TYPE_INT.
    If lo_cut is greater than or equal to hi_cut, then the mask is filled with
    outval.

    Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_UNSUPPORTED_MODE if the pixel type is unsupported
  - CPL_ERROR_INCOMPATIBLE_INPUT if the mask and image have different sizes
  - CPL_ERROR_ILLEGAL_INPUT if inval is not one of CPL_BINARY_1 or CPL_BINARY_0
  */
  /*----------------------------------------------------------------------------*/
  void threshold_image(const ImageBase& image, double lo_cut, double hi_cut,
                       bool inval);


  /**
   * @brief Dump CPL mask contents to a string.
   *
   * @param    w       optional Window
   *
   * Output format is as follows:
   * A header, with tab separation
   *     #----- mask: LLX <= x <= URX, LLY <= y <= URY -----
   *         X   Y   value
   * Values separated by tabs, then the value as an integer (all 8 bits in 1
   * int) 10  0   254 Rowwise first 10  1   254 10  2   254 10  3   254 10  4
   * 254
   *
   * @return String with the dump of the mask contents.
   */
  std::string dump(std::optional<Window> window) const;

  /**
   *  @brief    Save a mask to a FITS file
   *  @param    filename   Name of the file to write
   *  @param    pl      Property list for the output header or NULL
   *  @param    mode       The desired output options
   *
   *  This function saves a mask to a FITS file. If a property list
   *  is provided, it is written to the header where the mask is written.
   *
   *  The type used in the file is CPL_TYPE_UCHAR (8 bit unsigned).
   *
   * Supported output modes are CPL_IO_CREATE (create a new file) and
   *  CPL_IO_EXTEND (append a new extension to an existing file)
   *
   *  The output mode CPL_IO_EXTEND can be combined (via bit-wise or) with an
   *  option for tile-compression. This compression is lossless.
   *  The options are:
   *  CPL_IO_COMPRESS_GZIP, CPL_IO_COMPRESS_RICE, CPL_IO_COMPRESS_HCOMPRESS,
   *  CPL_IO_COMPRESS_PLIO.
   *
   *  Note that in append mode the file must be writable (and do not take for
   *  granted that a file is writable just because it was created by the same
   *  application, as this depends from the system @em umask).
   *
   * @return Reference to this.
   **/
  const Mask& save(const std::filesystem::path& filename,
                   const PropertyList& pl, unsigned mode) const;

  /**
   * @brief Mutable access to the underlying data
   *
   * The data is of a format you'd expect of a C 2D homogenous array:
   * width×height values of our TPixel type,
   * x=0,y=0 being the first element, x=1, y=0 being the next,
   * x=0,y=1 being the element at data()[width]
   *
   * Each element is 1 byte wide, containing either 1 or 0.
   * @see get_bit()
   */
  unsigned char* data();
  /**
   * @brief Const access to the underlying data:
   *
   * The data is of a format you'd expect of a C 2D homogenous array:
   * width×height values of our TPixel type,
   * x=0,y=0 being the first element, x=1, y=0 being the next,
   * x=0,y=1 being the element at data()[width]
   *
   * Each element is 1 byte wide, containing either 1 or 0.
   * @see get_bit
   */
  const unsigned char* data() const;
  /**
   * @brief Get an individual bit (1 or 0 returned)
   *
   * y is the row, 0 being the BOTTOMmost row of the image
   * x is the column, 0 being the leftmost column of image
   *
   * This follows the FITS convention, except for starting at 0,
   * which follows from astropy, and reduces confusion for C++/Python
   * programmers.
   *
   * @throws IllegalInputError for invalid coordinate
   */
  bool get_bit(size y, size x) const;

  /**
   * @brief How many pixels wide this image is
   */
  size get_width() const;

  /**
   * @brief How many pixels high this image is
   */
  size get_height() const;

  /**
   * @brief Returns width * height of this bitmask
   * W      Which is also the number of bytes in the data()
   */
  size get_size() const;

  /**
   * @brief Returns true iff this mask has nothing set to '1'
   */
  bool is_empty() const;
  /**
   * @brief Determiens number of occurrences of '1' bit in the
   *        given area of this bitmask
   * @throws IllegalInputError
   */
  size count(Window area = Window::All) const;

  // In-place operations are named OPERATOR_with
  // Since C++ &, |, ^ normally assume a copy is returned
  Mask& and_with(const Mask& other);
  Mask operator&(const Mask& other) const;

  Mask& or_with(const Mask& other);
  Mask operator|(const Mask& other) const;

  Mask& xor_with(const Mask& other);
  Mask operator^(const Mask& other) const;

  Mask& negate();
  Mask operator~() const;

  /**
   * @brief Create a 1-row mask, all elements are the logical AND
   *        of each cell in its corresponding column. Width is kept the same
   */
  Mask collapse_rows() const;
  /**
   * @brief Create a 1-column mask, all elements are the logical AND
   *        of each cell in its corresponding column. Height is kept the same
   */
  Mask collapse_cols() const;

  /**
   * @brief Copies out a window of this mask to a new mask.
   * @throws IllegalInputError
   */
  Mask extract(Window window) const;

  /**
   * @brief Rotate this mask by a multiple of 90 degrees clockwise
   * @param right_angle_turns Integral amount of turns to execute,
   *     Can by any value, its modulo 4 determines rotation:
   *
   *     - -3 to turn 270 degrees counterclockwise.
   *     - -2 to turn 180 degrees counterclockwise.
   *     - -1 to turn  90 degrees counterclockwise.
   *     -  0 to not turn
   *     - +1 to turn  90 degrees clockwise (same as -3)
   *     - +2 to turn 180 degrees clockwise (same as -2).
   *     - +3 to turn 270 degrees clockwise (same as -1).
   *
   * TODO: Determine if this is still accurate
   * The definition of the rotation relies on the FITS convention:
   * The lower left corner of the image is at (1,1), x increasing from left to
   * right, y increasing from bottom to top.
   * @returns this
   */
  Mask& rotate(int right_angle_turns);

  /**
   * @brief 2D bit shift. Empty values are set to '1'. This is modified
   * @throws IllegalInputError if shifts are bigger/equal to this mask
   * @returns this
   */
  Mask& shift(size y_shift, size x_shift);

  /**
   * @brief Inserts the bits of the given mask over the ones
   *        in this mask at the given location, for all bits
   *        in to_insert.
   * @throws IllegalInputError
   */
  Mask& insert(const Mask& to_insert, size y, size x);

  /**
   * @brief Flips this image along the given axis.
   * Values for axis:
   *  - 0 (theta=0) to flip the image around the horizontal
   *  - 1 (theta=pi/4) to flip the image around y=x
   *  - 2 (theta=pi/2) to flip the image around the vertical
   *  - 3 (theta=3pi/4) to flip the image around y=-x
   */
  Mask& flip(int axis);

  /**
   * @brief Rearrange pixels of this mask
   * @param    nb_cut  the number of cut in x and y
   *                   nb_cut must be positive and divide the size
   *                   of this mask in x and y evenly.
   * @param    new_pos array with the nb_cut^2 new positions
   *
   * This mask is divided into nb_cut * nb_cut tiles evenly
   * These tiles are then shuffled around according to new_pos array
   */
  Mask& move(size nb_cut, const std::vector<size>& positions);

  /**
   * @brief Downscales this image by sampling.
   * Samples are taken from the bottom left, going in steps.
   * There is no averaging or blending.
   * Returned image has the dimensions:
   *  - Width (⌊width - 1⌋ ÷ xstep) + 1
   *  - Height (⌊height - 1⌋ ÷ ystep) + 1
   */
  Mask subsample(size ystep, size xstep) const;

  /**
   * @brief  Filter a mask using a binary kernel
   * @param  kernel Elements to use, if set to CPL_BINARY_1
   * @param  filter CPL_FILTER_EROSION, CPL_FILTER_DILATION, CPL_FILTER_OPENING,
   *                or CPL_FILTER_CLOSING
   * @param  border CPL_BORDER_NOP, CPL_BORDER_ZERO or CPL_BORDER_COPY
   * @return CPL_ERROR_NONE Output of filtering result
   *
   * The kernel must have an odd number of rows and an odd number of columns.
   *
   * At least one kernel element must be set to CPL_BINARY_1.
   *
   * For erosion and dilation:
   * In-place filtering is not supported, but the input buffer may overlap all
   * but the 1+h first rows of the output buffer, where 1+2*h is the number of
   * rows in the kernel.
   *
   * For opening and closing:
   * Opening is implemented as an erosion followed by a dilation, and closing
   * is implemented as a dilation followed by an erosion. As such a temporary,
   * internal buffer the size of self is allocated and used. Consequently,
   * in-place opening and closing is supported with no additional overhead,
   * it is achieved by passing the same mask as both self and other.
   *
   * Duality and idempotency:
   *   Erosion and Dilation have the duality relations:
   *   not(dil(A,B)) = er(not(A), B) and
   *   not(er(A,B)) = dil(not(A), B).
   *
   *   Opening and closing have similar duality relations:
   *   not(open(A,B)) = close(not(A), B) and
   *   not(close(A,B)) = open(not(A), B).
   *
   *   Opening and closing are both idempotent, i.e.
   *   open(A,B) = open(open(A,B),B) and
   *   close(A,B) = close(close(A,B),B).
   *
   * The above duality and idempotency relations do _not_ hold on the mask
   * border (with the currently supported border modes).
   *
   * Unnecessary large kernels:
   * Adding an empty border to a given kernel should not change the outcome of
   * the filtering. However doing so widens the border of the mask to be
   * filtered and therefore has an effect on the filtering of the mask border.
   * Since an unnecessary large kernel is also more costly to apply, such
   * kernels should be avoided.
   *
   *  @par A 1 x 3 erosion filtering example
   *  @code
   *    Mask kernel = Mask(1, 3);
   *    kernel.negate();
   *    Mask filtered = raw.filter(kernel, CPL_FILTER_EROSION, CPL_BORDER_NOP);
   *  @endcode
   *
   * @throws IllegalInputError if the kernel has a side of even length.
   * @throws DataNotFoundError If the kernel is empty.
   * @throws AccessOutOfRangeError If the kernel has a side longer than the
   *                               input mask.
   * @throws UnsupportedModeError If the output pixel buffer overlaps the input
   *                              one (or the kernel), or the border/filter mode
   *                              is unsupported.
   */
  Mask filter(const Mask& kernel, cpl_filter_mode filter,
              cpl_border_mode border) const;

  const cpl_mask* ptr() const;
  cpl_mask* ptr();

  /**
   * @brief Relieves self Mask of ownership of the underlying cpl_mask* pointer,
   *        if it is owned.
   *
   * This is a counterpart to Mask(cpl_mask *to_steal);
   *
   * @note Make sure to use cpl_image_delete to delete the returned cpl_mask*
   * @returns empty optional if this mask doesn't own its underlying cpl_mask*,
   * OR (caller responsible for memory management) cpl_mask* when this did own
   * it.
   */
  static std::optional<cpl_mask*> unwrap(Mask&& self);


  /**
   * @brief Arbitrary code to run before ~Mask's own logic.
   *
   * This function is used when e.g. Image::get_bpm() is called. In this case,
   * Image is an 'owner', and want to return a Mask that borrows a cpl_mask
   * instead of owning it.
   *
   * This also allows a reference to the Image
   * to be kept as e.g. a shared_ptr or a python py::object in this
   * function's stack (by copying a shared_ptr into the std::function)
   * ensuring that this mask is always valid when accessed, and that
   * it's smart-pointer destructor is called when needed.
   *
   * Perhaps Image::get_bpm can return a unique_ptr with custom deleter instead.
   *
   * @note this is NOT called when Mask::unwrap is used to 'destruct' a mask
   * @see borrows
   */
  std::function<void(Mask&)> m_on_destruct;

  /**
   * @brief Flags if there is a 'borrow' relationship of this cpl_mask*
   *        instead of the default (borrows=false) where this Mask
   *        owns the underlying cpl_mask*
   *
   * This is used in the destructor, where cpl_mask_delete is only
   * called if borrows == false.
   *
   * This is also used in Mask::unwrap() to return not return if
   * this cannot be 'unwrapped' safely.
   *
   * This doesn't say anything about the semantics of the borrowing:
   * it doesn't say if this Mask will keep the thing it borrowed from
   * around or not
   */
  bool m_borrows = false;

  operator std::string() const;

  bool operator==(const Mask& other) const;
  bool operator!=(const Mask& other) const;

 private:
  cpl_mask* m_interface;

  static std::pair<size, size> coords_from_cpl(size x, size y) noexcept;
  static std::pair<size, size> coords_to_cpl(size x, size y) noexcept;
};

/**
 * @brief Loads an bitmask from an INTEGER FITS file,
 *
 * @param extension Specifies the extension from which the image should be
 * loaded (Default) 0 is for the main data section (Files without extension)
 * @param plane Specifies the plane to request from the data section. Default 0.
 * @param area The rectangle specifying the subset of the image to load.
 *             Window::All to load whole fits file.
 */
Mask load_mask(const std::filesystem::path& fitsfile, size plane = 0,
               size extension = 0, Window area = Window::All);

}  // namespace core
}  // namespace cpl

#endif  // PYCPL_MASK_HPP