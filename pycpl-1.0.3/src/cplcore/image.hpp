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

// Aims to wrap cpl_image as a C++ class
// This warpper will contain a pointer to the underlying CPL struct, using the
// appropriate CPL method for applying/retrieving information.

#ifndef PYCPL_IMAGE_HPP
#define PYCPL_IMAGE_HPP

#include <complex>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include <cpl_image.h>

#include "cplcore/bivector.hpp"
#include "cplcore/error.hpp"
#include "cplcore/mask.hpp"
#include "cplcore/polynomial.hpp"
#include "cplcore/types.hpp"
#include "cplcore/vector.hpp"

namespace cpl
{
namespace core
{
template <class TPixel>
class Image;
class ImageBase;

template <class TPixel>
struct is_image_pix
{
  static constexpr bool value = false;
};

template <>
struct is_image_pix<int>
{
  static constexpr bool value = true;
};

template <>
struct is_image_pix<float>
{
  static constexpr bool value = true;
};

template <>
struct is_image_pix<double>
{
  static constexpr bool value = true;
};

template <>
struct is_image_pix<std::complex<float>>
{
  static constexpr bool value = true;
};

template <>
struct is_image_pix<std::complex<double>>
{
  static constexpr bool value = true;
};

/**
 * @brief (NOT AVAILABLE FOR COMPLEX) Loads an image from a FITS file,
 *        Type of the returned image being determined by what's in the
 *        file (No type conversion is performed).
 *
 * The returned image has no bad pixels.
 *
 * @param extension Specifies the extension from which the image should be
 * loaded (Default) 0 is for the main data section (Files without extension)
 * @param plane Specifies the plane to request from the data section. Default 0.
 * @param area The rectangle specifying the subset of the image to load.
 *             Window::All to load whole fits file.
 */
std::shared_ptr<ImageBase>
load_fits_image(const std::filesystem::path& filename,
                cpl_type dtype = CPL_TYPE_UNSPECIFIED, size extension = 0,
                size plane = 0, Window area = Window::All);

/**
 * @brief Separate the given mask into contiguous regions, labelling each region
 *        a different number. 0 Doesn't count as a region.
 * @returns The image making up the labelled regions, and the amount of regions.
 *
 * Labelises all blobs: 4-neighbor connected zones set to 1 in this mask
 * will end up in the image as zones where all pixels are the same, unique
 * integer.
 *
 * Non-recursive flood-fill is applied to label the zones, dimensioned by the
 * number of lines in the image, and the maximal number of lines possibly
 * covered by a blob.
 *
 * @note I would like to have this be a member of Mask, however that would
 * require a mutual dependency of image.hpp and mask.hpp, complicating things.
 */
std::pair<std::shared_ptr<ImageBase>, int> labelise_mask(const Mask& from);

/**
 * A 2-dimensional data structure with a pixel type
 * (One of int, float, double, float complex, double complex)
 * And an optional bad pixel map
 *
 * Pixel indexing follows Astropys convention of 0-indexed,
 * Row-then-column (TBD direction). Pixelwise access should
 * be done row-wise for optimal performance, as this is how
 * the pixel buffer is stored.
 *
 * Utilities include:
 * FITS File Input/Output
 * ImageBase arithmetic, casting, extraction, thresholding, filtering,
 * resampling Bad pixel handling Generation of test images Special functions,
 * such as image quality estimator.
 */
class ImageBase
{
  friend class ImageList;

 public:
  /**
   * @throws InvalidTypeError if to_steal's type doesn't match this template
   * class.
   */
  ImageBase(cpl_image* to_steal);

  /**
   * @brief How much data each pixel of this image takes up,
   *        in the data array. Width * Height * pixel_size()
   *        guaranteed to be the size of data()
   *
   * This forces ImageBase to be pure virtual class
   * Users are required to use ImageBase<type>.
   */
  virtual size_t pixel_size() const = 0;

  /**
   * @brief Construct an image of the given dimensions,
   * optionally taking ownership of an existing pixel buffer.
   *
   * @param type CPL_TYPE_{FLOAT,DOUBLE}_COMPLEX, CPL_TYPE_{INT,FLOAT,DOUBLE}
   * @param pixel_size Size of each pixel in the given pixel buffer. MUST MATCH
   * cpl_type_get_sizeof
   *
   * If the sizes do not match the pixel buffer, or are not
   * positive, an IllegalInputError is thrown.
   */
  ImageBase(cpl::core::size width, cpl::core::size height, cpl_type type,
            size_t pixel_size, void* pixbuf = nullptr);
  /**
   * @brief Construct an image of the given dimensions,
   *        copying the given buffer of pixels (size determined by subclass)
   *
   * @param type CPL_TYPE_{FLOAT,DOUBLE}_COMPLEX, CPL_TYPE_{INT,FLOAT,DOUBLE}
   * @param pixel_size Size of each pixel in the given pixel buffer. MUST MATCH
   * cpl_type_get_sizeof
   *
   * If the size of the bitmask doesn't match width * height,
   * an IllegalInputError is thrown.
   */
  ImageBase(size width, size height, cpl_type type, size_t pixel_size,
            const std::string& pixbuf);

  virtual ~ImageBase();

  virtual cpl_type get_type() const = 0;

  /**
   * @param    real   The image real part to be transformed in place
   * @param    imag   The image imaginary part to be transformed in place
   * @param    mode   The desired FFT options (combined with bitwise or)
   *
   * The input images must be of double type.
   *
   * If the second passed image is NULL, the resulting imaginary part
   * cannot be returned. This can be useful if the input is real and the
   * output is known to also be real. But if the output has a significant
   * imaginary part, you might want to pass a 0-valued image as the second
   * parameter.
   *
   * Any rejected pixel is used as if it were a good pixel.
   *
   * The image must be square with a size that is a power of two.
   *
   * These are the supported FFT modes:
   * CPL_FFT_DEFAULT: Default, forward FFT transform
   * CPL_FFT_INVERSE: Inverse FFT transform
   * CPL_FFT_UNNORMALIZED: Do not normalize (with N*N for N-by-N image) on
   * inverse. Has no effect on forward transform. CPL_FFT_SWAP_HALVES: Swap the
   * four quadrants of the result image.
   *
   * @throws IllegalInputError if the image is not square or if the image size
   * is not a power of 2.
   * @throws InvalidTypeError if mode is 1, e.g. due to a logical or (||) of the
   *     allowed FFT options.
   * @throws UnsupportedModeError if mode is otherwise different from the
   * allowed FFT options.
   *
   */
  std::shared_ptr<ImageBase>
  fft(std::optional<std::shared_ptr<ImageBase>> imag, unsigned mode);

  /**
   * @brief Number of pixels wide this image is
   */
  size get_width() const;

  /**
   * @brief Number of pixels high this image is
   */
  size get_height() const;

  /**
   * @brief Returns width * height of this image
   * NOT the number of bytes in this image. Multiply by pixel_size for that
   */
  size get_size() const;

  /**
   * @brief Mutable access to the underlying data, untyped.
   *
   * The data is of a format you'd expect of a C 2D homogenous array,
   * TPixel[width*height] width×height values of the concrete pixel type
   * (determined by Image<> template arg) x=0,y=0 being the first element, x=1,
   * y=0 being the next, x=0,y=1 being the element at data()[width]
   *
   * @see cpl::core::Image::pixels
   */
  void* data();
  /**
   * @brief Const access to the underlying data, untyped.
   *
   * The data is of a format you'd expect of a C 2D homogenous array,
   * TPixel[width*height] width×height values of the concrete pixel type
   * (determined by Image<> template arg) x=0,y=0 being the first element, x=1,
   * y=0 being the next, x=0,y=1 being the element at data()[width]
   *
   * @see cpl::core::Image::pixels
   */
  const void* data() const;

  /**
   * @brief If this image is of int, float, or double type, returns double
   *        (max precision) of the pixel value at the given location
   *
   * y is the row, 0 being the BOTTOMmost row of the image
   * x is the column, 0 being the leftmost column of image
   *
   * This follows the FITS convention, except for starting at 0,
   * which follows from astropy, and reduces confusion for C++/Python
   * programmers.
   *
   * @throws IllegalInputError for invalid coordinate
   * @throws InvalidTypeError If this isn't an int, float, or double type
   * @returns null when the pixel at said location is rejected, otherwise
   *          the pixel at the location
   */
  std::optional<double> get_double(size y, size x) const;

  /**
   * @brief If this image is a complex type image, returns complex double
   *        (max precision) of the pixel value at the given location
   *
   * y is the row, 0 being the BOTTOMmost row of the image
   * x is the column, 0 being the leftmost column of image
   *
   * This follows the FITS convention, except for starting at 0,
   * which follows from astropy, and reduces confusion for C++/Python
   * programmers.
   *
   * @throws IllegalInputError for invalid coordinate
   * @throws InvalidTypeError If this isn't a a complex image
   * @returns null when the pixel at said location is rejected, otherwise
   *          the pixel at the location
   */
  std::optional<std::complex<double>> get_complex(size y, size x) const;

  /**
   * @brief Gets either a double if this is a normal image (int, float, double)
   *        or a std::complex<double> if this is a complex image (float or
   * double). equivalent to calling get_complex or get_double based on
   * is_complex()
   *
   * y is the row, 0 being the BOTTOMmost row of the image
   * x is the column, 0 being the leftmost column of image
   *
   *
   * This follows the FITS convention, except for starting at 0,
   * which follows from astropy, and reduces confusion for C++/Python
   * programmers.
   *
   * @throws IllegalInputError for invalid coordinate
   * @returns null when the pixel at said location is rejected, otherwise
   *          the pixel at the location
   */
  std::optional<std::variant<double, int, float, std::complex<float>,
                             std::complex<double>>>
  get_either(size y, size x) const;

  /**
   * @brief    Removes this image's Bad Pixel Map (BPM), returning it if it
   * exists.
   * @return   Old mask of bad pixels, if this image had one
   */
  std::optional<Mask> unset_bpm();

  /**
   * @brief    Replaces this image's bad pixel map by (if possible) stealing the
   * given one. (The given Mask may itself be borrowed from an image, in which
   * case, it will be copied to this image's BPM)
   * @param    bpm   Bad Pixel Map (BPM) to set, replacing the old one.
   *                 This is turned from an owning Mask to a borrowing one if
   *                 it was an owning mask.
   *                 It's m_on_destruct is NOT set, so you may set it
   *                 after executing set_bpm.
   * @return   Old mask of bad pixels, if this image had one
   * @note NULL is returned if the image had no bad pixel map, while a non-NULL
   *     returned mask must be deallocated by the caller using
   * cpl_mask_delete().
   * @see cpl_image_get_bpm_const()
   */
  std::optional<Mask> set_bpm(Mask& new_mask);

  /**
   * @brief Mutable access to the bad pixel map.
   *        a new mask may be created when this function is called,
   *        if one did not exist yet.
   *
   * It is undefined behaviour to use the returned mask after this image is
   * destructed.
   *
   * @returns a mask with borrows=true
   */
  Mask get_bpm();

  /**
   * @brief Const access to the bad pixel map.
   *        If no mask is set, returned optional will have no value
   *
   * It is undefined behaviour to use the returned mask after this image is
   * destructed.
   */
  std::optional<const Mask> get_bpm() const;

  /**
   * @brief    Set the pixel at the given position to the given value
   *           Precision is lost when used on int/float images.
   * If the pixel is flagged as rejected, this flag is removed.
   *
   * y is the row, 0 being the BOTTOMmost row of the image
   * x is the column, 0 being the leftmost column of image
   *
   * This follows the FITS convention, except for starting at 0,
   * which follows from astropy, and reduces confusion for C++/Python
   * programmers.
   *
   * @throws AccessOutOfRangeError if the passed position is not in the image
   * @throws InvalidTypeError If this isn't an int, float, or double type
   */
  void set_double(size y, size x, double value);

  /**
   * @brief    Set the pixel at the given position to the given value
   *           Precision is lost when used on float images.
   * If the pixel is flagged as rejected, this flag is removed.
   *
   * y is the row, 0 being the BOTTOMmost row of the image
   * x is the column, 0 being the leftmost column of image
   *
   * This follows the FITS convention, except for starting at 0,
   * which follows from astropy, and reduces confusion for C++/Python
   * programmers.
   *
   * @throws AccessOutOfRangeError if the passed position is not in the image
   * @throws InvalidTypeError If this isn't a a complex image
   */
  void set_complex(size y, size x, std::complex<double> value);

  /**
   * @brief Set the pixel at the given position to the given value
   *        equivalent to callingset_complex or set_double based on is_complex()
   *
   * y is the row, 0 being the BOTTOMmost row of the image
   * x is the column, 0 being the leftmost column of image
   *
   *
   * This follows the FITS convention, except for starting at 0,
   * which follows from astropy, and reduces confusion for C++/Python
   * programmers.
   *
   * @return Reference to this, which has been modified
   * @throws AccessOutOfRangeError if the passed position is not in the image
   * @throws InvalidTypeError If the given variant doesn't match this image
   */
  void set_either(size y, size x,
                  std::variant<double, int, float, std::complex<float>,
                               std::complex<double>>);

  /**
   * @brief Complex conjugate the pixels in a complex image
   *
   * Any bad pixels are also conjugated.
   *
   * @throws InvalidTypeError If an input image is not of complex type
   */
  void conjugate();  // Conjugate on self

  using ImagePair =
      std::pair<std::shared_ptr<ImageBase>, std::shared_ptr<ImageBase>>;

  /**
   * @brief Split this complex image into its real and/or imaginary part(s)
   *
   * Any bad pixels are also processed.
   *
   * @return Real part, then imaginary part.
   * @throws InvalidTypeError If an input image is not of complex type
   */
  ImagePair fill_re_im() const;

  /**
   * @brief Split this complex image into its absolute and argument part(s)
   *
   * Any bad pixels are also processed.
   *
   * @return Real part, then imaginary part.
   * @throws InvalidTypeError If an input image is not of complex type
   */
  ImagePair fill_abs_arg() const;

  /**
  @brief    Generate an image with uniform random noise distribution.
  @param    min_pix     Minimum output pixel value.
  @param    max_pix     Maximum output pixel value.
  @return Reference to this, which has been modified.

  Generate an image with a uniform random noise distribution. Pixel values are
  within the provided bounds.
  This function expects an already allocated image.
  The input image type can be CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  @throws IllegalInpuError if min_pix is bigger than max_pix
  @throws InvalidTypeError if the passed image type is not supported
  */
  /*----------------------------------------------------------------------------*/
  void fill_noise_uniform(double min_pix, double max_pix);

  /*----------------------------------------------------------------------------*/
  /**
  @ingroup cpl_image
  @brief    Generate an image from a 2d gaussian function.
  @param    xcen        x position of the center (1 for the first pixel)
  @param    ycen        y position of the center (1 for the first pixel)
  @param    norm        norm of the gaussian.
  @param    sig_x       Sigma in x for the gaussian distribution.
  @param    sig_y       Sigma in y for the gaussian distribution.
  @return   Reference to this, which has been modified.

  This function expects an already allocated image.
  This function generates an image of a 2d gaussian. The gaussian is
  defined by the position of its centre, given in pixel coordinates inside
  the image with the FITS convention (x from 1 to nx, y from 1 to ny), its
  norm and the value of sigma in x and y.

  f(x, y) = (norm/(2*pi*sig_x*sig_y)) * exp(-(x-xcen)^2/(2*sig_x^2)) *
              exp(-(y-ycen)^2/(2*sig_y^2))

  The input image type can be CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.

  Possible #_cpl_error_code_ set in this function:
  @throws InvalidTypeError if the passed image type is not supported
  */
  /*----------------------------------------------------------------------------*/
  void fill_gaussian(double xcen, double ycen, double norm, double sig_x,
                     double sig_y);

  /**
   * @brief Set the bad pixels in an image to a fixed value
   * @param a The fixed value
   *
   * Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT, CPL_TYPE_DOUBLE.
   *
   * @throws IllegalInpuError if the image pixel type is not supported
   */
  void fill_rejected(double a);

  /**
   * @brief Fill an image window with a constant
   * @param to_fill Area of the image to do this operation on.
   * @param value The value to fill with
   */
  void fill_window(Window to_fill, double value);

  /**
   * @brief Dump structural information of a CPL image to a string.
   *
   * @return String with the dump of the image structure information.
   */
  std::string dump_structure() const;

  /**
   * @brief Dump pixel values in a CPL image to a string.
   * @param w Area of the image to do this operation on.
   *
   * @return String with the dump of the image contents.
   */
  std::string dump(std::optional<Window> window) const;

  /**
   * @brief Copy an image.
   *
   * Copy an image into a new image object. The pixels and the bad pixel map are
   * also copied. The returned image must be deallocated using
   * cpl_image_delete().
   *
   * @return 1 newly allocated image,.
   */
  std::shared_ptr<ImageBase> duplicate() const;

  /**
   * @brief Convert a cpl_image to a given type
   * @param type The destination type
   *
   * Casting to non-complex types is only supported for non-complex types.
   *
   * @return the newly allocated cpl_image
   * @throws IllegalInputError if the passed type is invalid
   * @throws TypeMismatchError if the passed image type is complex and requested
   */
  std::shared_ptr<ImageBase> cast(cpl_type type) const;

  /**
   * @brief Save an image to a FITS file
   * @param filename Name of the file to write to
   * @param type The type used to represent the data in the file
   * @param pl Property list for the output header or NULL
   * @param mode The desired output options
   *
   * This function saves an image to a FITS file. If a property list
   * is provided, it is written to the header where the image is written.
   * The image may be NULL, in this case only the propertylist is saved.
   *
   * Supported image types are CPL_TYPE_DOUBLE, CPL_TYPE_FLOAT, CPL_TYPE_INT.
   *
   * The type used in the file can be one of:
   * CPL_TYPE_UCHAR  (8 bit unsigned),
   * CPL_TYPE_SHORT  (16 bit signed),
   * CPL_TYPE_USHORT (16 bit unsigned),
   * CPL_TYPE_INT    (32 bit signed),
   * CPL_TYPE_FLOAT  (32 bit floating point), or
   * CPL_TYPE_DOUBLE (64 bit floating point). Additionally, the special value
   * CPL_TYPE_UNSPECIFIED is allowed. This value means that the type used
   * for saving is the pixel type of the input image. Using the image pixel type
   * as saving type ensures that the saving incurs no loss of information.
   *
   * Supported output modes are CPL_IO_CREATE (create a new file) and
   * CPL_IO_EXTEND (append a new extension to an existing file).
   *
   * Upon success the image will reside in a FITS data unit with NAXIS = 2.
   * Is it possible to save a single image in a FITS data unit with NAXIS = 3,
   * see cpl_imagelist_save().
   *
   * When the data written to disk are of an integer type, the output mode
   * CPL_IO_EXTEND can be combined (via bit-wise or) with an option for
   * tile-compression. This compression of integer data is lossless.
   * The options are:
   * CPL_IO_COMPRESS_GZIP, CPL_IO_COMPRESS_RICE, CPL_IO_COMPRESS_HCOMPRESS,
   * CPL_IO_COMPRESS_PLIO.
   * With compression the type must be CPL_TYPE_UNSPECIFIED or CPL_TYPE_INT.
   *
   * Note that in append mode the file must be writable (and do not take for
   * granted that a file is writable just because it was created by the same
   * application, as this depends from the system @em umask).
   *
   * @return Reference to this.
   * @throws IllegalInputError if the type or the mode is not supported
   * @throws InvalidTypeError if the passed pixel type is not supported
   * @throws FileNotCreatedError if the output file cannot be created
   */
  void save(const char* filename, const PropertyList& pl, unsigned mode,
            cpl_type dtype = CPL_TYPE_UNSPECIFIED) const;

  /**
   * @brief Returns true if this image requires the use of get_complex()
   *      as opposed to get_double.
   */
  bool is_complex() const;

  const cpl_image* ptr() const;
  cpl_image* ptr();

  /**
   * @brief Relieves self Image of ownership of the underlying
   *        cpl_image* pointer. This is a counterpart to Image(cpl_image
   * *to_steal); Make sure to use cpl_image_delete to delete the returned
   * cpl_image* when you're done with it.
   */
  static cpl_image* unwrap(ImageBase&& self);

  operator std::string() const;

  /**
   * @brief Constructs an Image<Something> instance based on the given cpl_type.
   * optionally taking ownership of an existing pixel buffer.
   *
   * If the sizes do not match the pixel buffer, or are not
   * positive, an IllegalInputError is thrown.
   *
   * @param type CPL_TYPE_{FLOAT,DOUBLE}_COMPLEX, CPL_TYPE_{INT,FLOAT,DOUBLE}
   * @throws InvalidTypeError if the given cpl_type is not supported by Image
   */
  static std::shared_ptr<ImageBase>
  make_image(size width, size height, cpl_type type, void* pixbuf = nullptr);
  /**
   * @brief Constructs an Image<Something> instance based on the given cpl_type.
   *        copying the given buffer of pixels (size determined by subclass)
   *
   * If the size of the bitmask doesn't match width * height,
   * an IllegalInputError is thrown.
   *
   * @param type CPL_TYPE_{FLOAT,DOUBLE}_COMPLEX, CPL_TYPE_{INT,FLOAT,DOUBLE}
   * @throws InvalidTypeError if the given cpl_type is not supported by Image
   */
  static std::shared_ptr<ImageBase>
  make_image(size width, size height, cpl_type type,
             const std::string& bitmask);

  static std::shared_ptr<ImageBase> make_image(cpl_image* input);


  /**
   * @brief Add two images, store the result in the first image.
   * @param im2 second operand.
   *
   * The first input image is modified to contain the result of the operation.
   *
   * The bad pixel map of the first image becomes the union of the bad pixel
   * maps of the input images.
   *
   * @throws IncompatibleInputError if the input images have different sizes
   * @throws TypeMismatchError if the second input image has complex type
   */
  void add(const ImageBase& im2);

  /**
   * @brief Subtract two images, store the result in the first image.
   * @param im2 second operand.
   */
  void subtract(const ImageBase& im2);

  /**
   * @brief Multiply two images, store the result in the first image.
   * @param im2 second operand.
   */
  void multiply(const ImageBase& im2);

  /**
   * @brief Divide two images, store the result in the first image.
   * @param im2 second operand.
   *
   * @throws IncompatibleInputError if the input images have different sizes
   * @throws TypeMismatchError if the second input image has complex type
   */
  void divide(const ImageBase& im2);

  /**
   * @brief Elementwise addition of a scalar to an image
   * @param scalar Number to add
   *
   * Modifies the image by adding a number to each of its pixels.
   *
   * The operation is always performed in double precision, with a final
   * cast of the result to the image pixel type.
   */
  void add_scalar(double scalar);

  /**
   * @brief Elementwise subtraction of a scalar from an image
   * @param scalar Number to subtract
   */
  void subtract_scalar(double scalar);

  /**
   * @brief Elementwise multiplication of an image with a scalar
   * @param scalar Number to multiply with
   */
  void multiply_scalar(double scalar);

  /**
   * @brief Elementwise division of an image with a scalar
   * @param scalar Non-zero number to divide with
   *
   * Modifies the image by dividing each of its pixels with a number.
   *
   * If the scalar is zero the image is not modified and an error is returned.
   */
  void divide_scalar(double scalar);

  /**
   * @brief Compute the elementwise power of the image.
   * @param exponent Scalar exponent.
   *
   * Modifies the image by lifting each of its pixels to exponent.
   *
   * Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
   *
   * Pixels for which the power to the given exponent is not defined are
   * rejected and set to zero.
   */
  void power(double exponent);

  /**
   * @brief Compute the elementwise exponential of the image.
   * @param base Base of the exponential.
   *
   * Modifies the image by computing the base-scalar exponential of each of its
   * pixels.
   *
   * Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
   *
   * Pixels for which the power of the given base is not defined are
   * rejected and set to zero.
   */
  void exponential(double base);

  /**
   * @brief Compute the elementwise logarithm of the image.
   * @param base Base of the logarithm.
   *
   * Modifies the image by computing the base-scalar logarithm of each of its
   * pixels.
   *
   * Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
   *
   * Pixels for which the logarithm is not defined are
   * rejected and set to zero.
   *
   * @throws InvalidTypeError if the passed image type is not supported
   * @throws IllegalInputError if base is non-positive
   */
  void logarithm(double base);

  /**
   * @brief Normalise pixels in an image.
   * @param mode Normalisation mode.
   *
   * Normalises an image according to a given criterion.
   *
   * Possible normalisations are:
   * - CPL_NORM_SCALE sets the pixel interval to [0,1].
   * - CPL_NORM_MEAN sets the mean value to 1.
   * - CPL_NORM_FLUX sets the flux to 1.
   * - CPL_NORM_ABSFLUX sets the absolute flux to 1.
   */
  void normalise(cpl_norm mode);

  /**
   * @brief Take the absolute value of an image.
   *
   * Set each pixel to its absolute value.
   */
  void abs();

  /**
   * @brief The pixel-wise Euclidean distance between two images
   * @param first First input image
   * @param second Second input image.
   *
   * The Euclidean distance function is useful for gaussian error propagation
   * on addition/subtraction operations.
   *
   * For pixel values a and b the Euclidean distance c is defined as:
   * $$c = sqrt{a^2 + b^2}$$
   *
   * The Euclidean distance result is stored in self.
   *
   * Images can be of type CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
   *
   * If both input operands are of type CPL_TYPE_FLOAT the distance is computed
   * in single precision (using hypotf()), otherwise in double precision
   * (using hypot()).
   *
   * @throws IncompatibleInputError if the images have different sizes
   * @throws InvalidTypeError if the passed image type is not supported
   */
  void hypot(const ImageBase& first, const ImageBase& second);

  /**
   * @brief The bit-wise and of two images with integer pixels
   * @param second Second operand
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  void and_with(const ImageBase& second);

  /**
   * @brief The bit-wise or of two images with integer pixels
   * @param second Second operand
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  void or_with(const ImageBase& second);

  /**
   * @brief The bit-wise xor of two images with integer pixels
   * @param second Second operand
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  void xor_with(const ImageBase& second);

  /**
   * @brief The bit-wise complement (not) of an image with integer pixels
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  void negate();

  /**
   * @brief The bit-wise and of a scalar and an image with integer pixels
   * @param second Second operand (scalar)
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  void and_scalar(cpl_bitmask second);

  /**
   * @brief The bit-wise or of a scalar and an image with integer pixels
   * @param second Second operand (scalar)
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  void or_scalar(cpl_bitmask second);

  /**
   * @brief The bit-wise xor of a scalar and an image with integer pixels
   * @param second Second operand (scalar)
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  void xor_scalar(cpl_bitmask second);

  /**
   * @brief Extract a rectangular zone from an image into another image.
   * @param area Defines the rectangle to extract
   *
   * The input coordinates define the extracted region by giving the coordinates
   * of the lower left and upper right corners (inclusive).
   *
   * Coordinates must be provided in the FITS convention: lower left
   * corner of the image is at (1,1), x increasing from left to right,
   * y increasing from bottom to top.
   * Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
   *
   * If the input image has a bad pixel map and if the extracted rectangle has
   * bad pixel(s), then the extracted image will have a bad pixel map, otherwise
   * it will not.
   *
   * @return 1 newly allocated image
   * @throws IllegalInputError if the window coordinates are not valid
   */
  std::shared_ptr<ImageBase> extract(Window area) const;

  /**
   * @brief Rotate an image by a multiple of 90 degrees clockwise.
   * @param rot The multiple: -1 is a rotation of 90 deg counterclockwise.
   *
   * Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
   *
   * The definition of the rotation relies on the FITS convention:
   * The lower left corner of the image is at (1,1), x increasing from left to
   * right, y increasing from bottom to top.
   *
   * For rotations of +90 or -90 degrees on rectangular non-1D-images,
   * the pixel buffer is temporarily duplicated.
   *
   * rot may be any integer value, its modulo 4 determines the rotation:
   * - -3 to turn 270 degrees counterclockwise.
   * - -2 to turn 180 degrees counterclockwise.
   * - -1 to turn  90 degrees counterclockwise.
   * -  0 to not turn
   * - +1 to turn  90 degrees clockwise (same as -3)
   * - +2 to turn 180 degrees clockwise (same as -2).
   * - +3 to turn 270 degrees clockwise (same as -1).
   */
  void rotate(int rot);

  /**
   * @brief Shift an image by integer offsets
   * @param dy The shift in Y
   * @param dx The shift in X
   *
   * The new zones (in the result image) where no new value is computed are set
   * to 0 and flagged as bad pixels.
   * The shift values have to be valid:
   * -nx < dx < nx and -ny < dy < ny
   *
   * @throws IllegalInputError if the requested shift is bigger than the
   */
  void shift(size dy, size dx);

  /**
   * @brief Copy one image into another
   * @param im2 the inserted image
   * @param ypos the y pixel position in im1 where the lower left pixel of
   *     im2 should go (from 1 to the y size of im1)
   * @param xpos the x pixel position in im1 where the lower left pixel of
   *     im2 should go (from 1 to the x size of im1)
   *
   * (ypos, xpos) must be a valid position in im1. If im2 is bigger than the
   * place left in im1, the part that falls outside of im1 is simply ignored, an
   * no error is raised. The bad pixels are inherited from im2 in the concerned
   * im1 zone.
   *
   * The two input images must be of the same type, namely one of
   * CPL_TYPE_INT, CPL_TYPE_FLOAT, CPL_TYPE_DOUBLE.
   *
   * @throws TypeMismatchError if the input images are of different types
   * @throws InvalidTypeError if the passed image type is not supported
   * @throws AccessOutOfRangeError if xpos or ypos are outside the
   */
  void copy_into(const ImageBase& im2, size ypos, size xpos);

  /**
   * @brief Flip an image on a given mirror line.
   * @param angle mirror line in polar coord. is theta = (PI/4) * angle
   *
   * This function operates locally on the pixel buffer.
   *
   * angle can take one of the following values:
   * - 0 (theta=0) to flip the image around the horizontal
   * - 1 (theta=pi/4) to flip the image around y=x
   * - 2 (theta=pi/2) to flip the image around the vertical
   * - 3 (theta=3pi/4) to flip the image around y=-x
   *
   * Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
   *
   * @throws IllegalInputError if the angle is different from the allowed values
   */
  void flip(int angle);

  /**
   * @brief Reorganize the pixels in an image
   * @param nb_cut the number of cut in x and y
   * @param new_pos array with the nb_cut^2 new positions
   *
   * nb_cut^2 defines in how many tiles the images will be moved. Each tile will
   * then be moved to an other place defined in new_pos. 1 will leave the image
   * unchanged, 2 is used to move the quadrants, etc..
   * new_pos contains nb_cut^2 values between 1 and nb_cut^2.
   * The zones positions are counted from the lower left part of the image.
   * It is not allowed to move two tiles to the same position (the relation
   * between th new tiles positions and the initial position is bijective !).
   *
   * The image x and y sizes have to be multiples of nb_cut.
   *
   * 16   17   18           6    5    4
   * 13   14   15           3    2    1
   *
   * 10   11   12   ---->  12   11   10
   * 7    8    9           9    8    7
   *
   * 4    5    6          18   17   16
   * 1    2    3          15   14   13
   *
   * image 3x6            cpl_image_move(image, 3, new_pos);
   * with new_pos = {9,8,7,6,5,4,3,2,1};
   *
   * The bad pixels are moved accordingly.
   *
   * @throws IllegalInputError if nb_cut is not strictly positive or cannot
   */
  void move(size nb_cut, const std::vector<size>& positions);

  /**
   * @brief Compute FWHM values in x and y for an object
   * @param ypos the y position of the object (1 for the first pixel)
   * @param xpos the x position of the object (1 for the first pixel)
   *
   * This function uses a basic method: start from the center of the object
   * and go away until the half maximum value is reached in x and y.
   *
   * For the FWHM in x (resp. y) to be computed, the image size in the x (resp.
   * y) direction should be at least of 5 pixels.
   *
   * If for any reason, one of the FHWMs cannot be computed, its returned value
   * is -1.0, but an error is not necessarily raised. For example, if a 4 column
   * image is passed, the fwhm_x would be -1.0, the fwhm_y would be correctly
   * computed, and no error would be raised.
   *
   * @return fwhm y, x: the computed FWHM in y or x directinos
   * @throws DataNotFoundError if (xpos, ypos) specifies a rejected pixel or a
   * @throws AccessOutOfRangeError if xpos or ypos is outside the image
   */
  std::pair<double, double> get_fwhm(size ypos, size xpos) const;

  /**
   * @brief Compute an image quality estimation for an object
   * @param area The zone, at least 4×4 pixels
   *
   * This function makes internal use of the iqe() MIDAS function (called
   * here cpl_iqe()) written by P. Grosbol. Refer to the MIDAS documentation
   * for more details. This function has proven to give good results over
   * the years when called from RTD. The goal is to provide the exact same
   * functionality in CPL as the one provided in RTD. The code is simply
   * copied from the MIDAS package, it is not maintained by the CPL team.
   *
   * The returned object must be deallocated with cpl_bivector_delete().
   * It contains in the first vector the computed values, and in the second
   * one, the associated errors.
   * The computed values are:
   * - x position of the object
   * - y position of the object
   * - FWHM along the major axis
   * - FWHM along the minor axis
   * - the angle of the major axis with the horizontal in degrees
   * - the peak value of the object
   * - the background computed
   *
   * The bad pixels map of the image is not taken into account.
   * The input image must be of type float.
   *
   * @return a newly allocated cpl_bivector containing the results or
   *     NULL in error case.
   * @throws IllegalInputError if the specified zone is not valid or if the
   * computation fails on the zone
   * @throws InvalidTypeError if the input image has the wrong type
   */
  cpl::core::Bivector iqe(Window area) const;

  /**
   * @brief Warp an image according to a 2D polynomial transformation.
   * @param poly_y Defines source y-pos corresponding to destination (u,v).
   * @param poly_x Defines source x-pos corresponding to destination (u,v).
   * @param yprofile Interpolation weight as a function of the distance in Y
   * @param yradius Positive inclusion radius in the Y-dimension
   * @param xprofile Interpolation weight as a function of the distance in X
   * @param xradius Positive inclusion radius in the X-dimension
   *
   * 'out' and 'in'  may have different dimensions and types.
   *
   * The pair of 2D polynomials are used internally like this
   *
   * where (u,v) are (integer) pixel positions in the destination image and
   * (y,x) are the corresponding pixel positions (typically non-integer) in the
   * source image.
   *
   * The identity transform (poly_x(u,v) = u, poly_y(u,v) = v) would thus
   * overwrite the 'out' image with the 'in' image, starting from the lower left
   * if the two images are of different sizes.
   *
   * Beware that extreme transformations may lead to blank images.
   *
   * The input image type may be CPL_TYPE_INT, CPL_TYPE_FLOAT or
   * CPL_TYPE_DOUBLE.
   *
   * In case a correction for flux conservation were required, please create
   * a correction map using the function @c
   * cpl_image_fill_jacobian_polynomial().
   *
   * @throws IllegalInputError if the polynomial dimensions are not 2
   */
  std::shared_ptr<ImageBase>
  warp_polynomial(const Polynomial& poly_y, const Polynomial& poly_x,
                  const cpl::core::Vector& yprofile, double yradius,
                  const cpl::core::Vector& xprofile, double xradius,
                  std::tuple<size, size> out_dim, cpl_type out_type);

  /**
   * @brief Warp an image
   * @param in Source image to warp
   * @param deltay The y shift of each pixel, same image size as out
   * @param deltax The x shift of each pixel, same image size as out
   * @param yprofile Interpolation weight as a function of the distance in Y
   * @param yradius Positive inclusion radius in the Y-dimension
   * @param xprofile Interpolation weight as a function of the distance in X
   * @param xradius Positive inclusion radius in the X-dimension
   *
   * The pixel value at the (integer) position (u, v) in the destination image
   * is interpolated from the (typically non-integer) pixel position (x, y) in
   * the source image, where
   *
   * The identity transform is thus given by deltax and deltay filled with
   * zeros.
   *
   * The first pixel is (1, 1), located in the lower left.
   * 'out' and 'in' may have different sizes, while deltax and deltay must have
   * the same size as 'out'. deltax and deltay must have pixel type
   * CPL_TYPE_DOUBLE.
   *
   * Beware that extreme transformations may lead to blank images.
   *
   * 'out' and 'in' may be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or
   * CPL_TYPE_DOUBLE.
   *
   * Examples of profiles and radius are:
   *
   * In case a correction for flux conservation were required, please create
   * a correction map using the function @c cpl_image_fill_jacobian().
   *
   * @throws IllegalInputError if the input images sizes are incompatible
   */
  std::shared_ptr<ImageBase>
  warp(const ImageBase& deltay, const ImageBase& deltax,
       const cpl::core::Vector& yprofile, double yradius,
       const cpl::core::Vector& xprofile, double xradius);

  /**
   * @brief Compute area change ratio for a 2D polynomial transformation.
   * @param poly_x Defines source x-pos corresponding to destination (u,v).
   * @param poly_y Defines source y-pos corresponding to destination (u,v).
   *
   * Given an input image with pixel coordinates (y, x) which is
   * mapped into an output image with pixel coordinates (u, v), and
   * the polynomial inverse transformation (u, v) to (y, x) as in
   *
   * This is trivially obtained by computing the absolute value of the
   * determinant of the Jacobian of the transformation for each pixel
   * of the (u, v) image @em out.
   *
   * Typically this function would be used to determine a flux-conservation
   * factor map for the target image specified in function
   *
   * where @em out_flux_corrected is the resampled image @em out after
   * correction for flux conservation.
   *
   * @throws IllegalInputError if the polynomial dimensions are not 2
   */
  void
  fill_jacobian_polynomial(const Polynomial& poly_x, const Polynomial& poly_y);

  /**
   * @brief Compute area change ratio for a transformation map.
   * @param deltay The y shifts for each pixel
   * @param deltax The x shifts for each pixel
   *
   * The shifts images @em deltax and @em deltay, describing the
   * transformation, must be of type CPL_TYPE_DOUBLE and of the
   * same size as @em out. For each pixel (u, v) of the @em out
   * image, the deltax and deltay code the following transformation:
   *
   * This function writes the density of the (u, v) coordinate
   * system relative to the (y, x) coordinates for each (u, v)
   * pixel of image @em out.
   *
   * This is trivially obtained by computing the absolute value of the
   * determinant of the Jacobian of the transformation for each pixel
   * of the (u, v) image @em out.
   *
   * The partial derivatives are estimated at the position (u, v)
   * in the following way:
   *
   * Typically this function would be used to determine a flux-conservation
   * factor map for the target image specified in function
   *
   * where @em out_flux_corrected is the resampled image @em out after
   * correction for flux conservation.
   *
   * @throws IllegalInputError if the polynomial dimensions are not 2
   */
  void fill_jacobian(const ImageBase& deltay, const ImageBase& deltax);

  /**
   * @brief Sub-sample an image
   * @param ystep Take every ystep pixel in y
   * @param xstep Take every xstep pixel in x
   *
   * step represents the sampling step in y and x: both steps = 2 will create an
   * image with a quarter of the pixels of the input image.
   *
   * image type can be CPL_TYPE_INT, CPL_TYPE_FLOAT and CPL_TYPE_DOUBLE.
   * If the image has bad pixels, they will be resampled in the same way.
   *
   * The flux of the sampled pixels will be preserved, while the flux of the
   * pixels not sampled will be lost. Using steps = 2 in each direction on a
   * uniform image will thus create an image with a quarter of the flux.
   *
   * The returned image must be deallocated using cpl_image_delete().
   *
   * @return The newly allocated sub-sampled image or NULL in error case
   * @throws IllegalInputError if ystep, xstep are not positive
   */
  std::shared_ptr<ImageBase> extract_subsample(size ystep, size xstep) const;

  /**
   * @brief Rebin an image
   * @param ystart start y position of binning (starting from 1...)
   * @param xstart start x position of binning (starting from 1...)
   * @param ystep Bin size in y.
   * @param xstep Bin size in x.
   *
   * If both bin sizes in x and y are = 2, an image with (about) a quarter
   * of the pixels of the input image will be created. Each new pixel
   * will be the sum of the values of all contributing input pixels.
   * If a bin is incomplete (i.e., the input image size is not a multiple
   * of the bin sizes), it is not computed.
   *
   * ystep and xstep must not be greater than the sizes of the rebinned
   * region.
   *
   * The input image type can be CPL_TYPE_INT, CPL_TYPE_FLOAT and
   * CPL_TYPE_DOUBLE. If the image has bad pixels, they will be propagated to
   * the rebinned image "pessimistically", i.e., if at least one of the
   * contributing input pixels is bad, then the corresponding output pixel will
   * also be flagged "bad". If you need an image of "weights" for each rebinned
   * pixel, just cast the input image bpm into a CPL_TYPE_INT image, and
   * apply cpl_image_rebin() to it too.
   *
   * The returned image must be deallocated using cpl_image_delete().
   *
   * @return The newly allocated rebinned image or NULL in case of error
   * @throws IllegalInputError if ystep, xstep, ystart, xstart are not positive
   */
  std::shared_ptr<ImageBase>
  rebin(size ystart, size xstart, size ystep, size xstep) const;

  /**
   * @brief Interpolate a pixel
   * @param ypos Pixel y floating-point position (FITS convention)
   * @param xpos Pixel x floating-point position (FITS convention)
   * @param yprofile Interpolation weight as a function of the distance in Y
   * @param yradius Positive inclusion radius in the Y-dimension
   * @param xprofile Interpolation weight as a function of the distance in X
   * @param xradius Positive inclusion radius in the X-dimension
   * @param pconfid Confidence level of the interpolated value (range 0 to 1)

   *
   * If the X- and Y-radii are identical the area of inclusion is a circle,
   * otherwise it is an ellipse, with the larger of the two radii as the
   * semimajor axis and the other as the semiminor axis.
   *
   * The radii are only required to be positive. However, for small radii,
   * especially radii less than 1/sqrt(2), (xpos, ypos) may be located such that
   * no source pixels are included in the interpolation, causing the
   * interpolated pixel value to be undefined.
   *
   * The X- and Y-profiles can be generated with
   * cpl_vector_fill_kernel_profile(profile, radius).
   * For profiles generated with cpl_vector_fill_kernel_profile() it is
   * important to use the same radius both there and in
   * cpl_image_get_interpolated().
   *
   * A good profile length is CPL_KERNEL_DEF_SAMPLES,
   * using radius CPL_KERNEL_DEF_WIDTH.
   *
   * On error *pconfid is negative (unless pconfid is NULL).
   * Otherwise, if *pconfid is zero, the interpolated pixel-value is undefined.
   * Otherwise, if *pconfid is less than 1, the area of inclusion is close to
   * the image border or contains rejected pixels.
   *
   * The input image type can be CPL_TYPE_INT, CPL_TYPE_FLOAT and
   * CPL_TYPE_DOUBLE.
   *
   * Here is an example of a simple image unwarping (with error-checking omitted
   * for brevity):
   *
   * const double xyradius = CPL_KERNEL_DEF_WIDTH;
   *
   * cpl_vector * xyprofile = cpl_vector_new(CPL_KERNEL_DEF_SAMPLES);
   * cpl_image  * unwarped  = cpl_image_new(nx, ny, CPL_TYPE_DOUBLE);
   *
   * cpl_vector_fill_kernel_profile(xyprofile, CPL_KERNEL_DEFAULT, xyradius);
   *
   * for (iv = 1; iv <= ny; iv++) {
   * for (iu = 1; iu <= nx; iu++) {
   * double confidence;
   * const double x = my_unwarped_x();
   * const double y = my_unwarped_y();
   *
   * const double value = cpl_image_get_interpolated(warped, x, y,
   * xyprofile, xyradius,
   * xyprofile, xyradius,
   * &confidence);
   *
   * if (confidence > 0)
   * cpl_image_set(unwarped, iu, iv, value);
   * else
   * cpl_image_reject(unwarped, iu, iv);
   * }
   * }
   *
   * cpl_vector_delete(xyprofile);
   *
   * @return The interpolated pixel value, or undefined on error
   * @throws IllegalInputError if xradius, xprofile, yprofile and yradius are
   * not as requested
   * @throws InvalidTypeError if the passed image type is not supported
   */
  std::tuple<double, double>
  get_interpolated(double ypos, double xpos, const cpl::core::Vector& yprofile,
                   double yradius, const cpl::core::Vector& xprofile,
                   double xradius) const;

  /**
   * @brief Count the number of bad pixels declared in an image
   *
   * @return the number of bad pixels or -1 if the input image is NULL
   */
  size count_rejected() const;

  /**
   * @brief Test if a pixel is good or bad
   * @param y the y pixel position in the image (first pixel is 0)
   * @param x the x pixel position in the image (first pixel is 0)
   *
   * @return true if the pixel is bad, false if the pixel is good
   * @throws AccessOutOfRangeError if the specified position is out of
   */
  bool is_rejected(size y, size x) const;

  /**
   * @brief Set a pixel as bad in an image
   * @param y the y pixel position in the image (first pixel is 0)
   * @param x the x pixel position in the image (first pixel is 0)
   *
   * @throws AccessOutOfRangeError if the specified position is out of
   */
  void reject(size y, size x);

  /**
   * @brief Set a pixel as good in an image
   * @param y the y pixel position in the image (first pixel is 0)
   * @param x the x pixel position in the image (first pixel is 0)
   *
   * @throws AccessOutOfRangeError if the specified position is out of
   */
  void accept(size y, size x);

  /**
   * @brief Set all pixels in the image as good
   */
  void accept_all();

  /**
   * @brief Set the bad pixels in an image as defined in a mask
   * @param map the mask defining the bad pixels
   *
   * If the input image has a bad pixel map prior to the call, it is
   * overwritten.
   */
  void reject_from_mask(const Mask& map);

  /**
   * @brief Reject pixels with the specified special value(s)
   * @param mode Bit field specifying which special value(s) to reject
   *
   * @throws InvalidTypeError if mode is 1, e.g. due to a logical or (||)
   * @throws UnsupportedModeError if mode is otherwise different from the
   */
  void reject_value(cpl_value mode);

  /**
   * @brief computes minimum pixel value over an image or sub window.
   * @param area The area of this image to opreate on.
   *
   * @return the minimum value
   */
  double get_min(std::optional<Window> area) const;

  /**
   * @brief computes maximum pixel value over an image or image sub-window.
   * @param area The area of this image to opreate on
   *
   * The specified bounds are included in the specified region.
   *
   * Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT or CPL_TYPE_DOUBLE.
   *
   * @return the maximum value, or undefined on error
   */
  double get_max(std::optional<Window> area) const;

  /**
   * @brief computes mean pixel value over an image or sub window.
   * @param area The area of this image to opreate on
   *
   * @return the mean value
   */
  double get_mean(std::optional<Window> area) const;

  /**
   * @brief computes median pixel value over an image or over an image
   *sub-window.
   ** @param area The area of this image to opreate on
   *
   * The specified bounds are included in the specified region.
   *
   * For a finite population or sample, the median is the middle value of an odd
   * number of values (arranged in ascending order) or any value between the two
   * middle values of an even number of values.
   * For an even number of elements in the array, the mean of the two central
   * values is returned. Note that in this case, the median might not belong
   * to the input array.
   *
   * @return The median value, or undefined on error
   * @throws IllegalInputError if the window is outside the image or if
   */
  double get_median(std::optional<Window> area) const;

  /**
   * @brief computes pixel standard deviation over an image or sub window.
   * @param area The area of this image to opreate on
   *
   * @return the standard deviation value
   */
  double get_stdev(std::optional<Window> area) const;

  /**
   * @brief Computes the sum of pixel values over an image or sub window.
   * @param area The area of this image to opreate on
   *
   * @return the flux value
   */
  double get_flux(std::optional<Window> area) const;

  /**
   * @brief Computes the sum of absolute values over an image or sub window.
   * @param area The area of this image to opreate on
   *
   * @return the absolute flux (sum of |pixels|) value
   */
  double get_absflux(std::optional<Window> area) const;

  /**
   * @brief Computes the sum of squared values over an image or sub window
   * @param area The area of this image to opreate on
   *
   * @return the sqaure flux
   */
  double get_sqflux(std::optional<Window> area) const;

  /**
   * @brief Computes the x centroid value over the whole image or sub window
   * @param area The area of this image to opreate on
   *
   * @return the x centroid value
   */
  double get_centroid_x(std::optional<Window> area) const;

  /**
   * @brief Computes the y centroid value over the whole image or sub window
   * @param area The area of this image to opreate on
   *
   * @return the y centroid value
   */
  double get_centroid_y(std::optional<Window> area) const;
  /**
   * @brief Computes minimum pixel value and position over an image or
   * sub-window.
   * @param area The area of this image to opreate on
   * @return the x coordinate and y coordinate of the minimum position
   *
   * The specified bounds are included in the specified region.
   *
   * Images can be CPL_TYPE_FLOAT, CPL_TYPE_INT or CPL_TYPE_DOUBLE.
   */
  std::pair<size, size> get_minpos(std::optional<Window> area) const;

  /**
   * @brief Computes maximum pixel value and position over an image or
   * sub-window.
   * @param area The area of this image to opreate on
   * @return the x coordinate and y coordinate of the maximum position
   */
  std::pair<size, size> get_maxpos(std::optional<Window> area) const;

  /**
   * @brief Computes median and mean absolute median deviation on an image
   * window or sub-window
   * @param area The area of this image to opreate on
   *
   * For each non-bad pixel in the window the absolute deviation from the median
   * is computed. The mean of these absolute deviations in returned via sigma,
   * while the median itself is returned by the function. The computed median
   * and sigma may be a robust estimate of the mean and standard deviation of
   * the pixels. The sigma is however still sensitive to outliers. See
   * cpl_image_get_mad_window() for a more robust estimator.
   *
   * @return The median of the non-bad pixels and The mean of the absolute
   * median deviation of the (good) pixels
   * @throws InvalidTypeError if the passed image type is not supported
   * @throws IllegalInputError if the specified window is illegal
   */
  std::pair<double, double> get_median_dev(std::optional<Window> area) const;

  /**
   * @brief Computes median and median absolute deviation (MAD) on an image or
   * sub-window.
   * @param area The area of this image to opreate on
   *
   * For each non-bad pixel in the window the absolute deviation from the median
   * is computed. The median of these absolute deviations in returned via sigma,
   * while the median itself is returned by the function.
   *
   * If the pixels are gaussian, the computed sigma is a robust and consistent
   * estimate of the standard deviation in the sense that the standard deviation
   * is approximately k * MAD, where k is a constant equal to
   * approximately 1.4826. CPL defines CPL_MATH_STD_MAD as this scaling
   * constant.
   *
   * @return The median of the non-bad pixels and the median absolute deviation
   * (MAD) of the non-bad pixels
   * @throws InvalidTypeError if the passed image type is not supported
   * @throws IllegalInputError if the specified window is illegal
   */
  std::pair<double, double> get_mad(std::optional<Window> area) const;

  /**
   * @brief Filter the image using a binary kernel
   * @param other Image to filter
   * @param kernel Pixels to use, if set to CPL_BINARY_1
   * @param filter CPL_FILTER_MEDIAN, CPL_FILTER_AVERAGE and more, see below
   * @param border CPL_BORDER_FILTER and more, see below
   * @param dtype Data type for the output image.
   * @return A newly allocated image containing the filtered image.
   *
   * The kernel must have an odd number of rows and an odd number of columns.
   *
   * The two images must have equal dimensions, except for the border mode
   * CPL_BORDER_CROP, where the input image must have 2 * hx columns more and
   * 2 * hy rows more than the output image, where the kernel has size
   * (1 + 2 * hx, 1 + 2 * hy).
   *
   * In standard deviation filtering the kernel must have at least two elements
   * set to CPL_BINARY_1, for others at least one element must be set to
   * CPL_BINARY_1.
   *
   * Supported pixel types are: CPL_TYPE_INT, CPL_TYPE_FLOAT and
   * CPL_TYPE_DOUBLE.
   *
   * In median filtering the two images must have the same pixel type.
   *
   * In standard deviation filtering a filtered pixel must be computed from at
   * least two input pixels, for other filters at least one input pixel must be
   * available. Output pixels where this is not the case are set to zero and
   * flagged as rejected.
   *
   * In-place filtering is not supported.
   *
   * Supported modes:
   *
   * CPL_FILTER_MEDIAN:
   * CPL_BORDER_FILTER, CPL_BORDER_COPY, CPL_BORDER_NOP, CPL_BORDER_CROP.
   *
   * CPL_FILTER_AVERAGE:
   * CPL_BORDER_FILTER
   *
   * CPL_FILTER_AVERAGE_FAST:
   * CPL_BORDER_FILTER
   *
   * CPL_FILTER_STDEV:
   * CPL_BORDER_FILTER
   *
   * CPL_FILTER_STDEV_FAST:
   * CPL_BORDER_FILTER
   *
   * To shift an image 1 pixel up and 1 pixel right with the CPL_FILTER_MEDIAN
   * filter and a 3 by 3 kernel, one should set to CPL_BINARY_1 the bottom
   * leftmost kernel element - at row 3, column 1, i.e.
   *
   * The kernel required to do a 5 x 5 median filtering is created like this:
   *
   * @throws IllegalInputError if the kernel has a side of even length.
   * @throws DataNotFoundError If the kernel is empty, or in case of
   CPL_FILTER_STDEV if the kernel has only one element set to CPL_BINARY_1.
   * @throws AccessOutOfRangeError If the kernel has a side longer than the
   input image.
   * @throws InvalidTypeError if the passed image type is not supported.
   * @throws TypeMismatchError if in median filtering the input and output pixel
   types differ.
   * @throws UnsupportedModeError If the output pixel buffer overlaps the input
   one (or the kernel), or the border/filter mode is unsupported.

   */
  std::shared_ptr<ImageBase>
  filter_mask(const Mask& kernel, cpl_filter_mode filter,
              cpl_border_mode border, cpl_type dtype);


  /**
   * @brief Filter the image using a floating-point kernel
   * @param other Image to filter
   * @param kernel Pixel weigths
   * @param filter CPL_FILTER_LINEAR, CPL_FILTER_MORPHO
   * @param border CPL_BORDER_FILTER
   * @param dtype Data type for the output image.
   * @return A newly allocated image containing the filtered image.
   *
   * The two images must have equal dimensions.
   *
   * The kernel must have an odd number of rows and an odd number of columns
   * and at least one non-zero element.
   *
   * For scaling filters (@c CPL_FILTER_LINEAR_SCALE and
   *
   * For flux-preserving filters (@c CPL_FILTER_LINEAR and @c CPL_FILTER_MORPHO)
   * the filtered pixel must have at least one input pixel with a non-zero
   * weight available. Output pixels where this is not the case are set to zero
   * and flagged as bad.
   *
   * In-place filtering is not supported.
   *
   * Supported filters: @c CPL_FILTER_LINEAR, @c CPL_FILTER_MORPHO,
   *
   * Supported borders modes: @c CPL_BORDER_FILTER
   *
   * Beware that the 1st pixel - at (1,1) - in an image is the lower left,
   * while the 1st element in a matrix  - at (0,0) - is the top left. Thus to
   * shift an image 1 pixel up and 1 pixel right with the CPL_FILTER_LINEAR and
   * a 3 by 3 kernel, one should set to 1.0 the bottom leftmost matrix element
   * which is at row 3, column 1, i.e.
   *
   * @throws IllegalInputError if the kernel has a side of even length.
   * @throws DivisionByZeroError If the kernel is a zero-matrix.
   * @throws AccessOutOfRangeError If the kernel has a side longer than the
   * input image.
   * @throws InvalidTypeError if the passed image type is not supported.
   * @throws TypeMismatchError if in median filtering the input and output pixel
   * types differ.
   * @throws IncompatibleInputError If the input and output images have
   * incompatible sizes.
   * @throws UnsupportedModeError If the output pixel buffer overlaps the input
   * CPL_ERROR_UNSUPPORTED_MODE If the output pixel buffer overlaps the input
   */
  std::shared_ptr<ImageBase>
  filter(const Matrix& kernel, cpl_filter_mode filter, cpl_border_mode border,
         cpl_type dtype);


  /**
   * @brief Threshold an image to a given interval.
   * @param lo_cut Strict Lower bounf of threhsold
   * @param hi_cut Strict Upper bound of threshold
   * @param assign_lo_cut Value to assign to pixels below low bound.
   * @param assign_hi_cut Value to assign to pixels above high bound.
   *
   * This function is only defined for non-complex Images
   *
   * The input image type can be double, float or int.
   * If lo_cut ≥ hi_cut, then the mask is filled with opposite of inval
   *
   * @throws InvalidTypeError if called on a complex-type image
   * @throws UnsupportedModeError if TPixel is not double, float or int
   */
  void threshold(double lo_cut, double hi_cut, double assign_lo_cut,
                 double assign_hi_cut) const;

  /*----------------------------------------------------------------------------*/
  /**
   @ingroup cpl_image
  @brief    Extract a column from an image
  @param    image_in    Input image
  @param    pos         Position of the column (1 for the left one)
  @return   1 newly allocated cpl_vector or NULL on error

  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
  The returned vector must be deallocated using cpl_vector_delete().

  The bad pixels map is not taken into account in this function.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if pos is not valid
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
  */
  /*----------------------------------------------------------------------------*/
  Vector vector_from_column(int pos);
  /*----------------------------------------------------------------------------*/
  /**
   @ingroup cpl_image
  @brief    Extract a column from an image
  @param    image_in    Input image
  @param    pos         Position of the column (1 for the left one)
  @return   1 newly allocated cpl_vector or NULL on error

  Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
  The returned vector must be deallocated using cpl_vector_delete().

  The bad pixels map is not taken into account in this function.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if pos is not valid
  - CPL_ERROR_INVALID_TYPE if the passed image type is not supported
  */
  /*----------------------------------------------------------------------------*/
  Vector vector_from_row(int pos);

  int iter_idx = 0;  // Used to keep track as an iterator for the python class
  ImageBase& operator=(const ImageBase& other);
  ImageBase& operator=(ImageBase&& other) noexcept;

  bool operator==(const ImageBase& other) const;
  bool operator!=(const ImageBase& other) const;

 protected:
  cpl_image* m_interface;

  /*
  Copy & Move can maybe? change the TPixel.
  Which is completley unwatned. But the functionality
  implemented in them is used by Image<TPixel>, so they
  are protected here
  */
  ImageBase(const ImageBase& other);
  ImageBase(ImageBase&& other) noexcept;
};

template <class TPixel>
class Image : public ImageBase
{
 public:
  /**
   * @throws InvalidTypeError if to_steal's type doesn't match this template
   * class.
   */
  Image(cpl_image* to_steal) : ImageBase(to_steal)
  {
    if (cpl_image_get_type(to_steal) != get_type()) {
      throw InvalidTypeError(PYCPL_ERROR_LOCATION,
                             "cpl::core::Image"
                             " constructed with cpl_image* of type not "
                             "matching template parameter."
                             " use cpl::core::make_image if you do not know "
                             "the type of the cpl_image*");
    }
  }

  Image(const Image<TPixel>& other) : ImageBase(other) {}

  Image(Image<TPixel>&& other) noexcept : ImageBase(other) {}

  ~Image() = default;

  Image<TPixel>& operator=(const Image<TPixel>& other)
  {
    *(static_cast<ImageBase*>(this)) = other;
  }

  Image<TPixel>& operator=(Image<TPixel>&& other) noexcept
  {
    *(static_cast<ImageBase*>(this)) = other;
  }

  /**
   * @brief How much data each pixel of this image takes up,
   *        in the data array. Width * Height * pixel_size()
   *        guaranteed to be the size of data()
   *
   * This forces ImageBase to be pure virtual class
   * Users are required to use ImageBase<type>.
   */
  virtual size_t pixel_size() const override
  {
    return cpl_type_get_sizeof(get_type());
  }

  /**
   * @brief  How much data each pixel of this image takes up,
   *        in the data array. Width * Height * pixel_size()
   *        guaranteed to be the size of data()
   */
  static size_t pixel_size_static()
  {
    return cpl_type_get_sizeof(cpl::core::type_to_cpl<TPixel>);
  }

  /**
   * @brief Construct an image of the given dimensions,
   * optionally taking ownership of an existing pixel buffer.
   *
   * If the sizes do not match the pixel buffer, or are not
   * positive, an IllegalInputError is thrown.
   */
  Image(cpl::core::size width, cpl::core::size height, TPixel* pixbuf = nullptr)
      :  // this->pixel_size() or this->get_type() calls aren't allowed due to
         // vtable being null at this point in construction
        ImageBase(width, height, cpl::core::type_to_cpl<TPixel>,
                  Image<TPixel>::pixel_size_static(), pixbuf)
  {
  }

  /**
   * @brief Construct an image of the given dimensions,
   *        copying the given bytes, reinterpreting them as the image
   *
   * If the size of the pixbuf doesn't match width * height * pixel_size(),
   * an IllegalInputError is thrown.
   */
  Image(size width, size height, const std::string& pixbuf)
      :  // this->pixel_size() or this->get_type() calls aren't allowed due to
         // vtable being null at this point in construction
        ImageBase(width, height, cpl::core::type_to_cpl<TPixel>,
                  Image<TPixel>::pixel_size_static(), pixbuf)
  {
  }

  virtual cpl_type get_type() const override
  {
    return cpl::core::type_to_cpl<TPixel>;
  }

  /**
   * @brief Mutable access to the underlying data, aligned & sized properly.
   *
   * The data is of a format you'd expect of a C 2D homogenous array,
   * TPixel[width*height] width×height values of our TPixel type, x=0,y=0 being
   * the first element, x=1, y=0 being the next, x=0,y=1 being the element at
   * data()[width]
   *
   * @see get_pixel
   */
  TPixel* pixels()
  {
    return reinterpret_cast<TPixel*>(
        Error::throw_errors_with(cpl_image_get_data, m_interface));
  }

  /**
   * @brief Const access to the underlying data, aligned & sized properly.
   *
   * The data is of a format you'd expect of a C 2D homogenous array,
   * TPixel[width*height] width×height values of our TPixel type, x=0,y=0 being
   * the first element, x=1, y=0 being the next, x=0,y=1 being the element at
   * data()[width]
   *
   * @see get_pixel
   */
  const TPixel* pixels() const
  {
    return reinterpret_cast<TPixel*>(
        Error::throw_errors_with(cpl_image_get_data_const, m_interface));
  }

  /**
   * @brief Get the value of a pixel at a given position
   * y is the row, 0 being the BOTTOMmost row of the image
   * x is the column, 0 being the leftmost column of image
   *
   * This follows the FITS convention, except for starting at 0,
   * which follows from astropy, and reduces confusion for C++/Python
   * programmers.
   *
   * @throws IllegalInputError for invalid coordinate
   * @returns null when the pixel at said location is rejected, otherwise
   *          the pixel at the location
   */
  std::optional<TPixel> get_pixel(size y, size x) const;

  /**
   * @brief    Set the pixel at the given position to the given value
   *           Precision is lost when used on int/float images.
   * If the pixel is flagged as rejected, this flag is removed.
   *
   * y is the row, 0 being the BOTTOMmost row of the image
   * x is the column, 0 being the leftmost column of image
   *
   * This follows the FITS convention, except for starting at 0,
   * which follows from astropy, and reduces confusion for C++/Python
   * programmers.
   *
   * @throws AccessOutOfRangeError if the passed position is not in the image
   * @throws InvalidTypeError If this isn't an int, float, or double type
   */
  Image<TPixel>& set_pixel(size y, size x, TPixel value);

  /*
      Functions & stuff specific to non-complex class instances
  */

  /**
   * @brief (NOT AVAILABLE FOR COMPLEX) Loads an image from a FITS file,
   *        If the type of the FITS file doesn't match the Template parameter
   *        TPixel, then type conversion is performed to return an ImageBase
   *
   * The returned image has no bad pixels.
   *
   * This function is only defined for non-complex Images
   *
   * @param extension Specifies the extension from which the image should be
   * loaded (Default) 0 is for the main data section (Files without extension)
   * @param plane Specifies the plane to request from the data section. Default
   * 0.
   * @param area The rectangle specifying the subset of the image to load.
   *             Window::All to load whole fits file.
   * @throws InvalidTypeError if this is called on a complex-type image
   */
  static Image<TPixel>
  load_fits_image(const std::filesystem::path& filename, int extension = 0,
                  int plane = 0, Window area = Window::All)
  {
    if (cpl::core::type_to_cpl<TPixel> & CPL_TYPE_COMPLEX) {
      throw InvalidTypeError(
          PYCPL_ERROR_LOCATION,
          "Image::load_fits_image cannot consume FITS files to a complex "
          "image");
    }

    if (area == Window::All) {
      return Image(Error::throw_errors_with(cpl_image_load, filename.c_str(),
                                            CPL_TYPE_UNSPECIFIED, plane,
                                            extension));
    } else {
      return Image(Error::throw_errors_with(
          cpl_image_load_window, filename.c_str(), CPL_TYPE_UNSPECIFIED, plane,
          extension, EXPAND_WINDOW(area)));
    }
  }

  /**
   * @brief Interpret the given mask as an Integer image
   *
   * @note This function is only template instantiated for ImageBase<int>
   */
  explicit Image(const Mask& from);

  /**
   * @param area Defines the rectangle to extract
   * @see ImageBase::extract for non-pixel-specific version.
   * @return   1 newly allocated image
   *
   * The input coordinates define the extracted region by giving the coordinates
   * of the lower left and upper right corners (inclusive).
   *
   * Coordinates must be provided in the FITS convention: lower left
   * corner of the image is at (1,1), x increasing from left to right,
   * y increasing from bottom to top.
   * Images can be of type CPL_TYPE_INT, CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE.
   *
   * If the input image has a bad pixel map and if the extracted rectangle has
   * bad pixel(s), then the extracted image will have a bad pixel map, otherwise
   * it will not.
   *
   * @throws IllegalInputError if the window coordinates are not valid
   * @throws InvalidTypeError if the passed image type is not supported
   */
  Image<TPixel> extract(Window area) const
  {
    Image<TPixel>(
        Error::throw_errors_with(cpl_image_extract, EXPAND_WINDOW(area)));
  }

  /**
   * @param    second   Second operand
   * @note This function is only template instantiated for ImageBase<int>
   * @see cpl_mask_and() for the equivalent logical operation
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  Image<TPixel>& and_with(const Image<TPixel>& second)
  {
    Error::throw_errors_with(cpl_image_and, m_interface, nullptr, second.ptr());
    return *this;
  }

  /**
   * @param    second   Second operand
   * @note This function is only template instantiated for Image<int>
   * @see cpl_mask_or() for the equivalent logical operation
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  Image<TPixel>& or_with(const Image<TPixel>& second)
  {
    Error::throw_errors_with(cpl_image_or, m_interface, nullptr, second.ptr());
    return *this;
  }

  /**
   * @param    second   Second operand
   * @note This function is only template instantiated for Image<int>
   * @see cpl_mask_or() for the equivalent logical operation
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  Image<TPixel>& xor_with(const Image<TPixel>& second)
  {
    Error::throw_errors_with(cpl_image_xor, m_interface, nullptr, second.ptr());
    return *this;
  }

  /**
   * @param    second   Second operand
   * @note This function is only template instantiated for Image<int>
   * @see cpl_mask_or() for the equivalent logical operation
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  Image<TPixel>& negate()
  {
    Error::throw_errors_with(cpl_image_not, m_interface, nullptr);
    return *this;
  }

  /**
   * @param    second   Second operand (scalar)
   * @note This function is only template instantiated for Image<int>
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  Image<TPixel>& and_scalar(uint64_t second)
  {
    Error::throw_errors_with(cpl_image_and_scalar, m_interface, nullptr,
                             second);
    return *this;
  }

  /**
   * @param    second   Second operand (scalar)
   * @note This function is only template instantiated for Image<int>
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  Image<TPixel>& or_scalar(uint64_t second)
  {
    Error::throw_errors_with(cpl_image_or_scalar, m_interface, nullptr, second);
    return *this;
  }

  /**
   * @param    second   Second operand (scalar)
   * @note This function is only template instantiated for Image<int>
   *
   * @throws IncompatibleInputError if the images have different sizes
   */
  Image<TPixel>& xor_scalar(uint64_t second)
  {
    Error::throw_errors_with(cpl_image_xor_scalar, m_interface, nullptr,
                             second);
    return *this;
  }

  /**
   * @brief Convert a cpl_image to this type of image
   * @param other The image to cast from
   *
   * Casting to non-complex types is only supported for non-complex types.
   *
   * @return the newly allocated cpl_image
   * @throws IllegalInputError if the passed type is invalid
   * @throws TypeMismatchError if the passed image type is complex and requested
   */
  static Image<TPixel> cast(ImageBase& other)
  {
    return Image(Error::throw_errors_with(cpl_image_cast, other.ptr(),
                                          cpl::core::type_to_cpl<TPixel>));
  }

  /**
   * @brief Sub-sample an image
   * @param xstep Take every xstep pixel in x
   * @param ystep Take every ystep pixel in y
   *
   * step represents the sampling step in x and y: both steps = 2 will create an
   * image with a quarter of the pixels of the input image.
   *
   * image type can be CPL_TYPE_INT, CPL_TYPE_FLOAT and CPL_TYPE_DOUBLE.
   * If the image has bad pixels, they will be resampled in the same way.
   *
   * The flux of the sampled pixels will be preserved, while the flux of the
   * pixels not sampled will be lost. Using steps = 2 in each direction on a
   * uniform image will thus create an image with a quarter of the flux.
   *
   * The returned image must be deallocated using cpl_image_delete().
   *
   * @return The newly allocated sub-sampled image or NULL in error case
   * @throws IllegalInputError if xstep, ystep are not positive
   */
  Image<TPixel> extract_subsample(size xstep, size ystep) const
  {
    return Error::throw_errors_with(cpl_image_extract_subsample, m_interface,
                                    xstep, ystep);
  }

  /**
   * @brief Rebin an image
   * @param xstart start x position of binning (starting from 1...)
   * @param ystart start y position of binning (starting from 1...)
   * @param xstep Bin size in x.
   * @param ystep Bin size in y.
   *
   * If both bin sizes in x and y are = 2, an image with (about) a quarter
   * of the pixels of the input image will be created. Each new pixel
   * will be the sum of the values of all contributing input pixels.
   * If a bin is incomplete (i.e., the input image size is not a multiple
   * of the bin sizes), it is not computed.
   *
   * xstep and ystep must not be greater than the sizes of the rebinned
   * region.
   *
   * The input image type can be CPL_TYPE_INT, CPL_TYPE_FLOAT and
   * CPL_TYPE_DOUBLE. If the image has bad pixels, they will be propagated to
   * the rebinned image "pessimistically", i.e., if at least one of the
   * contributing input pixels is bad, then the corresponding output pixel will
   * also be flagged "bad". If you need an image of "weights" for each rebinned
   * pixel, just cast the input image bpm into a CPL_TYPE_INT image, and
   * apply cpl_image_rebin() to it too.
   *
   * The returned image must be deallocated using cpl_image_delete().
   *
   * @return The newly allocated rebinned image or NULL in case of error
   * @throws IllegalInputError if xstep, ystep, xstart, ystart are not positive
   */
  Image<TPixel> rebin(size xstart, size ystart, size xstep, size ystep) const
  {
    return Error::throw_errors_with(cpl_image_rebin, m_interface, xstart,
                                    ystart, xstep, ystep);
  }

  bool operator==(const Image<TPixel>& other) const
  {
    return static_cast<ImageBase*>(this) == other;
  }

  bool operator!=(const Image<TPixel>& other) const
  {
    return !operator==(other);
  }
};

template <>
std::optional<double> Image<double>::get_pixel(size x, size y) const;
template <>
std::optional<float> Image<float>::get_pixel(size x, size y) const;
template <>
std::optional<int> Image<int>::get_pixel(size x, size y) const;
template <>
std::optional<std::complex<float>>
Image<std::complex<float>>::get_pixel(size x, size y) const;
template <>
std::optional<std::complex<double>>
Image<std::complex<double>>::get_pixel(size x, size y) const;

}  // namespace core
}  // namespace cpl

#endif  // PYCPL_IMAGE_HPP