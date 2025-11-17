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


// Container class ParameterList which is similar in functionality to
// cpl_parameterlist, using vectors instead

#ifndef PYCPL_IMAGELIST_HPP
#define PYCPL_IMAGELIST_HPP

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <cpl_imagelist.h>

#include "cplcore/coords.hpp"
#include "cplcore/image.hpp"
#include "cplcore/types.hpp"
#include "cplcore/vector.hpp"

namespace cpl
{
namespace core
{
class ImageList
{
 public:
  ImageList();

  ImageList(cpl_imagelist* im_list);

  ImageList(std::vector<std::shared_ptr<ImageBase>> images);

  ~ImageList();  // Default call destructor of vector, which in turn calls
                 // destructor of the parameters

  //   bool operator==(ImageList &other);
  //   bool operator!=(ImageList &other);

  const cpl_imagelist* ptr() const;

  /**
   * DO NOT change the ordering, or amount of images in this imagelist,
   * through the pointer.
   */
  cpl_imagelist* ptr();

  cpl_size size() const;

  std::shared_ptr<ImageBase> operator[](int index) const;

  std::shared_ptr<ImageBase> get_at(int index) const;

  /*----------------------------------------------------------------------------*/
  /**
    @brief    Get structural information of images in an imagelist
    @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

    Possible #_cpl_error_code_ set in this function:
    @throws FileIOError if a write operation fails

    Outputs stream from cpl function to string
  */
  /*----------------------------------------------------------------------------*/
  std::string dump_structure() const;

  std::string dump(std::optional<cpl::core::Window> window) const;

  /*----------------------------------------------------------------------------*/
  /**
    @brief    Insert an image into an imagelist
    @param    image    The image to insert
    @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

    It is allowed to specify the position equal to the number of images in the
    list. This will increment the size of the imagelist.

    It is not allowed to insert images of different size into a list.

    Uses cpl_imagelist_set to insert an image at the end of the list at index
    size()

    Possible #_cpl_error_code_ set in this function:
    @throws IllegalInputError if pos is negative
    @throws TypeMismatchError if im and self are of different types
    @throws IncompatibleInputError if im and self have different sizes
    @throws AccessOutOfRangeErrorif pos is bigger than the number of
      images in self
  */
  /*----------------------------------------------------------------------------*/
  void append(std::shared_ptr<ImageBase> image);

  // TODO: adapt below

  /**
   * @brief Swap the axis of an image list
   * @param mode The swapping mode
   *
   * This function is intended for users that want to use the cpl_imagelist
   * object as a cube. Swapping the axis would give them access to the
   * usual functions in the 3 dimensions. This has the cost that it
   * duplicates the memory consumption, which can be a problem for big
   * amounts of data.
   *
   * Image list can be CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
   * The mode can be either CPL_SWAP_AXIS_XZ or CPL_SWAP_AXIS_YZ
   *
   * @return The swapped image list or NULL in error case
   * @throws IllegalInputError if mode is not equal to one of the possible
   */
  std::shared_ptr<ImageList> swap_axis_create(cpl_swap_axis mode) const;

  /**
   * @brief Average an imagelist to a single image
   *
   * The returned image has to be deallocated with cpl_image_delete().
   *
   * The bad pixel maps of the images in the input list are taken into
   * account, the result image pixels are flagged as rejected for those
   * where there were no good pixel at the same position in the input image
   * list.
   *
   * For integer pixel types, the averaging is performed using integer division.
   *
   * @return the average image or NULL on error case.
   */
  std::shared_ptr<ImageBase> collapse_create() const;

  /**
   * @brief Add two image lists, the first one is replaced by the result.
   * @param in2 image list to add
   *
   * The two input lists must have the same size, the image number n in the
   * list in2 is added to the image number n in the list in1.
   *
   */
  void add(const ImageList& in2);

  /**
   * @brief Elementwise addition of a scalar to each image in the imlist
   * @param addend Number to add
   *
   */
  void add_scalar(double addend);

  /**
   * @brief Add an image to an image list.
   * @param img image to add
   *
   * The passed image is added to each image of the passed image list.
   *
   */
  void add_image(const ImageBase* img);

  /**
   * @brief Divide two image lists, the first one is replaced by the result.
   * @param in2 image list to divide
   *
   */
  void divide(const ImageList& in2);

  /**
   * @brief Subtract two image lists, the first one is replaced by the result.
   * @param in2 image list to subtract
   *
   */
  void subtract(const ImageList& in2);

  /**
   * @brief Multiply two image lists, the first one is replaced by the result.
   * @param in2 image list to multiply
   *
   */
  void multiply(const ImageList& in2);

  /**
   * @brief Threshold all pixel values to an interval.
   * @param lo_cut Lower bound.
   * @param hi_cut Higher bound.
   * @param assign_lo_cut Value to assign to pixels below low bound.
   * @param assign_hi_cut Value to assign to pixels above high bound.
   *
   * Threshold the images of the list using cpl_image_threshold()
   * The input image list is modified.
   *
   */
  void threshold(double lo_cut, double hi_cut, double assign_lo_cut,
                 double assign_hi_cut);

  /**
   * @brief Normalize each image in the list.
   * @param mode Normalization mode.
   *
   * The list may be partly modified if an error occurs.
   *
   */
  void normalise(cpl_norm mode);

  /**
   * @brief Elementwise division of each image in the imlist with a scalar
   * @param divisor Non-zero number to divide with
   *
   */
  void divide_scalar(double divisor);

  /**
   * @brief Compute the elementwise exponential of each image in the imlist
   * @param base Base of the exponential.
   *
   */
  void exponential(double base);

  /**
   * @brief Collapse an imagelist with kappa-sigma-clipping rejection
   * @param kappalow kappa-factor for lower clipping threshold
   * @param kappahigh kappa-factor for upper clipping threshold
   * @param keepfrac The fraction of values to keep (0.0 < keepfrac <= 1.0)
   * @param mode Clipping mode, CPL_COLLAPSE_MEAN or CPL_COLLAPSE_MEDIAN
   *
   * The collapsing is an iterative process which will stop when it converges
   * (i.e. an iteration did not reject any values for a given pixel) or
   * when the next iteration would reduce the fraction of values to keep
   * to less than or equal to keepfrac.
   *
   * A call with keepfrac == 1.0 will thus perform no clipping.
   *
   * Supported modes:
   * CPL_COLLAPSE_MEAN:
   * The center value of the acceptance range will be the mean.
   * CPL_COLLAPSE_MEDIAN:
   * The center value of the acceptance range will be the median.
   * CPL_COLLAPSE_MEDIAN_MEAN:
   * The center value of the acceptance range will be the median in
   * the first iteration and in subsequent iterations it will be the
   * mean.
   *
   * For each pixel position the pixels whose value is higher than
   * center + kappahigh * stdev or lower than center - kappalow * stdev
   * are discarded for the subsequent center and stdev computation, where center
   * is defined according to the clipping mode, and stdev is the standard
   * deviation of the values at that pixel position. Since the acceptance
   * interval must be non-empty, the sum of kappalow and kappahigh must be
   * positive. A typical call has both kappalow and kappahigh positive.
   *
   * The minimum number of values that the clipping can select is 2. This is
   * because the clipping criterion is based on the sample standard deviation,
   * which needs at least two values to be defined. This means that all calls
   * with (positive) values of keepfrac less than 2/n will behave the same. To
   * ensure that the values in (at least) i planes out of n are kept, keepfrac
   * can be set to (i - 0.5) / n, e.g. to keep at least 50 out of 100 values,
   * keepfrac can be set to 0.495.
   *
   * The output pixel is set to the mean of the non-clipped values, also in the
   * median mode. Regardless of the input pixel type, the mean is computed in
   * double precision. The result is then cast to the output-pixel type, which
   * is identical to the input pixel type.
   *
   * The input parameter contrib is optional. It must be either NULL or point to
   * a pre-allocated image of type CPL_TYPE_INT and size equal to the images in
   * the imagelist. On success, it will contain the contribution map, i.e.  the
   * number of kept (non-clipped) values after the iterative process on every
   * pixel.
   *
   * Bad pixels are ignored from the start. This means that with a sufficient
   * number of bad pixels, the fraction of good values will be less than
   * keepfrac. In this case no iteration is performed at all. If there is at
   * least one good value available, then the mean will be based on the good
   * value(s). If for a given pixel position there are no good values, then that
   * pixel is set to zero, rejected as bad and if available the value in the
   * contribution map is set to zero.
   *
   * The input imagelist can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT and
   * CPL_TYPE_DOUBLE.
   *
   * @return The collapsed image and an integer-image for contribution map
   * @throws DataNotFoundError if there are less than 2 images in the list
   * @throws IllegalInputError if the sum of kappalow and kappahigh is
   * non-positive,
   * @throws AccessOutOfRangeError if keepfrac is outside the required interval
   * which is 0.0 < keepfrac <= 1.0
   * @throws TypeMismatchError if contrib is non-NULL but not of type
   * CPL_TYPE_INT
   * @throws IncompatibleInputError if contrib is non-NULL but
   * @throws InvalidTypeError if the type of the input imagelist is unsupported
   */
  std::pair<std::shared_ptr<ImageBase>, std::shared_ptr<ImageBase>>
  collapse_sigclip_create(double kappalow, double kappahigh, double keepfrac,
                          cpl_collapse_mode mode);

  /**
   * @brief Divide an image list by an image.
   * @param img image for division
   *
   */
  void divide_image(const ImageBase* img);

  /**
   * @brief Subtract an image from an image list.
   * @param img image to subtract
   *
   */
  void subtract_image(const ImageBase* img);

  /**
   * @brief Compute the elementwise power of each image in the imlist
   * @param exponent Scalar exponent
   *
   */
  void power(double exponent);

  /**
   * @brief Elementwise multiplication of the imlist with a scalar
   * @param factor Number to multiply with
   *
   */
  void multiply_scalar(double factor);

  /**
   * @brief Create a median image from the input imagelist
   *
   * The input image list can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT and
   * CPL_TYPE_DOUBLE.
   *
   * On success each pixel in the created image is the median of the values on
   * the same pixel position in the input image list. If for a given pixel all
   * values in the input image list are rejected, the resulting pixel is set to
   * zero and flagged as rejected.
   *
   * The median is defined here as the middle value of an odd number of sorted
   * samples and for an even number of samples as the mean of the two central
   * values. Note that with an even number of samples the median may not be
   * among the input samples.
   *
   * Also, note that in the case of an even number of integer data, the mean
   * value will be computed using integer arithmetic. Cast your integer data
   * to a floating point pixel type if that is not the desired behavior.
   *
   * @return The median image of the input pixel type or NULL on error
   * @throws IllegalInputError if the input image list is not valid
   */
  std::shared_ptr<ImageBase> collapse_median_create() const;

  /**
   * @brief Average with rejection an imagelist to a single image
   * @param nlow Number of low rejected values
   * @param nhigh Number of high rejected values
   *
   * The input images are averaged, for each pixel position the nlow lowest
   * pixels and the nhigh highest pixels are discarded for the average
   * computation.
   *
   * The input image list can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT and
   * CPL_TYPE_DOUBLE. The created image will be of the same type.
   *
   * On success each pixel in the created image is the average of the
   * non-rejected values on the pixel position in the input image list.
   *
   * For a given pixel position any bad pixels (i.e. values) are handled as
   * follows:
   * Given n bad values on a given pixel position, n/2 of those values are
   * assumed to be low outliers and n/2 of those values are assumed to be high
   * outliers. Any low or high rejection will first reject up to n/2 bad values
   * and if more values need to be rejected that rejection will take place on
   * the good values. This rationale behind this is to allow the rejection of
   * outliers to include bad pixels without introducing a bias. If for a given
   * pixel all values in the input image list are rejected, the resulting pixel
   * is set to zero and flagged as rejected.
   *
   * @return The average image or NULL on error
   * @throws IllegalInputError if the input image list is not valid or if
   */
  std::shared_ptr<ImageBase>
  collapse_minmax_create(cpl_size nlow, cpl_size nhigh) const;

  /**
   * @brief Compute the elementwise logarithm of each image in the imlist
   * @param base Base of the logarithm.
   *
   */
  void logarithm(double base);

  /**
   * @brief Elementwise subtraction of a scalar from each image in the imlist
   * @param subtrahend Number to subtract
   *
   */
  void subtract_scalar(double subtrahend);

  /**
   * @brief Multiply an image list by an image.
   * @param img image to multiply
   *
   */
  void multiply_image(const ImageBase* img);

  /*----------------------------------------------------------------------------*/
  /**
    @brief    Save an imagelist to disk in FITS format.
    @param    self      Imagelist to save
    @param    filename  Name of the FITS file to write
    @param    type      The type used to represent the data in the file
    @param    plist     Property list for the output header or NULL
    @param    mode      The desired output options (combined with bitwise or)
    @return   the #_cpl_error_code_ or CPL_ERROR_NONE
    @see cpl_image_save()

    This function saves an image list to a FITS file. If a
    property list is provided, it is written to the named file before the
    pixels are written.

    Supported image lists types are CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT,
    CPL_TYPE_INT.

    The type used in the file can be one of:
      CPL_TYPE_UCHAR  (8 bit unsigned),
      CPL_TYPE_SHORT  (16 bit signed),
      CPL_TYPE_USHORT (16 bit unsigned),
      CPL_TYPE_INT    (32 bit signed),
      CPL_TYPE_FLOAT  (32 bit floating point), or
      CPL_TYPE_DOUBLE (64 bit floating point). Additionally, the special value
      CPL_TYPE_UNSPECIFIED is allowed. This value means that the type used
    for saving is the pixel type of the input image. Using the image pixel type
    as saving type ensures that the saving incurs no loss of information.

    Supported output modes are CPL_IO_CREATE (create a new file),
    CPL_IO_EXTEND (extend an existing file with a new extension) and
    CPL_IO_APPEND (append a list of images to the last data unit, which
    must already contain compatible image(s)).

    For the CPL_IO_APPEND mode it is recommended to pass a NULL pointer for the
    output header, since updating the already existing header incurs significant
    overhead.

    When the data written to disk are of an integer type, the output mode
    CPL_IO_EXTEND can be combined (via bit-wise or) with an option for
    tile-compression. This compression of integer data is lossless.
    The options are:
    CPL_IO_COMPRESS_GZIP, CPL_IO_COMPRESS_RICE, CPL_IO_COMPRESS_HCOMPRESS,
    CPL_IO_COMPRESS_PLIO.
    With compression the type must be CPL_TYPE_UNSPECIFIED or CPL_TYPE_INT.

    In extend and append mode, make sure that the file has write permissions.
    You may have problems if you create a file in your application and append
    something to it with the umask set to 222. In this case, the file created
    by your application would not be writable, and the append would fail.

    @throws IllegalInputError if the type or the mode is not supported
    @throws FileIOError if the file cannot be written
    @throws InvalidTypeError if the passed image list type is not supported
  */
  /*----------------------------------------------------------------------------*/
  void save(const std::filesystem::path& filename, const PropertyList& pl,
            unsigned mode, cpl_type type) const;

  /*----------------------------------------------------------------------------*/
  /**
    @brief    Determine if an imagelist contains images of equal size and type
    @param    imlist    The imagelist to check
    @return   Zero if uniform, positive if non-uniform and negative on error.

    The function returns 1 if the list is empty.

  */
  /*----------------------------------------------------------------------------*/
  bool is_uniform();


  /*----------------------------------------------------------------------------*/
  /**
    @brief    Remove an image from an imagelist
    @param    self   The imagelist
    @param    pos      The list position (from 0 to number of images-1)
    @return   The pointer to the removed image or NULL in error case

    The specified image is not deallocated, it is simply removed from the
    list. The pointer to the image is returned to let the user decide to
    deallocate it or not.
    Eventually, the image will have to be deallocated with cpl_image_delete().

    @throws IllegalInputError if pos is negative
    @throws AccessOutOfRangeErrorif pos is bigger than the number of
      images in self
  */
  /*----------------------------------------------------------------------------*/
  std::shared_ptr<ImageBase> pop(long pos);

  /*----------------------------------------------------------------------------*/
  /**
    @brief    sets an image at a specified position in the imagelist
    @param    self   The imagelist
    @param    im     The image to insert
    @param    pos    The list position (from 0 to number of images)
    @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

    It is allowed to specify the position equal to the number of images in the
    list. This will increment the size of the imagelist.

    It is not allowed to insert images of different size into a list.

    The added image is owned by the imagelist object, which deallocates it when
    cpl_imagelist_delete is called. Another option is to use cpl_imagelist_unset
    to recover ownership of the image, in which case the cpl_imagelist object is
    not longer responsible for deallocating it.

    @throws IllegalInputError if pos is negative
    @throws TypeMismatchError if im and self are of different types
    @throws IncompatibleInputError if im and self have different sizes
    @throws AccessOutOfRangeErrorif pos is bigger than the number of
      images in self
  */
  /*----------------------------------------------------------------------------*/
  void set(std::shared_ptr<ImageBase> img, long pos);

  /*----------------------------------------------------------------------------*/
  /**
    @brief    Insert an image into an imagelist
    @param    self   The imagelist
    @param    im     The image to insert
    @param    pos    The list position (from 0 to number of images)
    @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error

    This will increment the size of the imagelist.

    No action occurs if an image is inserted more than once into the same
    position. It is allowed to insert the same image into two different
    positions in a list.

    The image is inserted at the position pos in the image list.

    It is not allowed to insert images of different size into a list.

    The added image is owned by the imagelist object, which deallocates it when
    cpl_imagelist_delete is called. Another option is to use cpl_imagelist_unset
    to recover ownership of the image, in which case the cpl_imagelist object is
    not longer responsible for deallocating it.

    @throws IllegalInputError if pos is negative
    @throws TypeMismatchError if im and self are of different types
    @throws IncompatibleInputError if im and self have different sizes
    @throws AccessOutOfRangeErrorif pos is bigger than the number of
      images in self
  */
  /*----------------------------------------------------------------------------*/
  void insert(std::shared_ptr<ImageBase> img, long pos);

  /*----------------------------------------------------------------------------*/
  /**
    @brief    Cast an imagelist, optionally in-place
    @param    self   Destination imagelist
    @param    other  Source imagelist, or NULL to cast in-place
    @param    type   If called with empty self, cast to this pixel-type
    @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
    @see cpl_image_cast()
    @note    If called with a non-empty self in an out-of-place cast,
    the input images are cast to the type already present in self and appended
    to the output list. In this case the parameter type is ignored.

    @throws IncompatibleInputError if the same pointer is passed twice
    @throws IllegalInputError if the passed type is invalid
    @throws TypeMismatchError if the passed image type is complex and requested
    casting type is non-complex.
    @throws InvalidTypeError if the passed pixel type is not supported
  */
  /*----------------------------------------------------------------------------*/
  std::shared_ptr<ImageList> cast(cpl_type type);

  /*----------------------------------------------------------------------------*/
  /**
    @brief    Empty an imagelist and deallocate all its images
    @param    self  The image list or NULL
    @return   Nothing
    @see  cpl_imagelist_empty(), cpl_imagelist_delete()
    @note If @em self is @c NULL nothing is done and no error is set.

    After the call the image list can be populated again. It must eventually
    be deallocted with a call to cpl_imagelist_delete().

  */
  /*----------------------------------------------------------------------------*/
  void empty();
  /*----------------------------------------------------------------------------*/
  /**
    @brief    Copy an image list
    @return   1 newly allocated image list, or NULL on error.

    Copy an image list into a new image list object.
    The returned image list must be deallocated using cpl_imagelist_delete().

    Possible #_cpl_error_code_ set in this function:
    - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  */
  /*----------------------------------------------------------------------------*/
  std::shared_ptr<ImageList> duplicate();

 private:
  std::vector<std::shared_ptr<ImageBase>> m_images;
  cpl_imagelist* m_interface;
};

/*----------------------------------------------------------------------------*/
/**
  @brief    Load a FITS file extension into a list of images
  @param    filename   The FITS file name
  @param    type    Type of the images in the created image list
  @param    xtnum      The extension number (0 for primary HDU)
  @return   The loaded list of images or NULL on error.
  @see      cpl_image_load()

  This function loads all the images of a specified extension (NAXIS=2
  or 3) into an image list.

  Type can be CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT or CPL_TYPE_INT.
  The loaded images have an empty bad pixel map.

  The returned cpl_imagelist must be deallocated using cpl_imagelist_delete()

  Possible #_cpl_error_code_ set in this function:
  @throws IllegalInputError if xtnum is negative
  @throws InvalidTypeError if the passed type is not supported
  @throws FileIOError If the file cannot be opened or read, or if xtnum is
                      bigger than the number of extensions in the FITS file
  @throws BadFileFormatError if the file cannot be parsed
  @throws DataNotFoundError if the data cannot be read from the file
 */
/*----------------------------------------------------------------------------*/
std::shared_ptr<ImageList>
load_imagelist(const std::filesystem::path& name, cpl_type type, size position,
               Window area = Window::All);

/*----------------------------------------------------------------------------*/
/**
  @brief    Create a contribution map from the bad pixel maps of the images.
  @param    imlist  The imagelist
  @return   The contributions map (a CPL_TYPE_INT cpl_image) or NULL on error
  @see cpl_imagelist_is_uniform()

  The returned map counts for each pixel the number of good pixels in the list.
  The returned map has to be deallocated with cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  @throws IllegalInputError if the input image list is not valid
 */
/*----------------------------------------------------------------------------*/
std::shared_ptr<ImageBase> image_from_accepted(const ImageList& in2);


}  // namespace core
}  // namespace cpl

#endif  // PYCPL_IMAGELIST_HPP