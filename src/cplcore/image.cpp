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


// Aims to wrap cpl_image structs as a C++ class.
// This wrapper will contain a pointer to the underlying CPL struct, using the
// appropriate CPL method for applying/retrieving information.

#include "cplcore/image.hpp"

#include <cstdio>
#include <cstring>
#include <sstream>

#include <cpl_memory.h>

namespace cpl
{
namespace core
{
using Mask = cpl::core::Mask;

std::shared_ptr<ImageBase>
load_fits_image(const std::filesystem::path& filename, cpl_type dtype,
                size extension, size plane, Window area)
{
  cpl_image* loaded;

  if (area == Window::All) {
    loaded = Error::throw_errors_with(cpl_image_load, filename.c_str(), dtype,
                                      plane, extension);
  } else {
    loaded =
        Error::throw_errors_with(cpl_image_load_window, filename.c_str(), dtype,
                                 plane, extension, EXPAND_WINDOW(area));
  }

  switch (Error::throw_errors_with(cpl_image_get_type, loaded)) {
    case CPL_TYPE_DOUBLE:
      return static_cast<std::shared_ptr<ImageBase>>(
          std::make_shared<Image<double>>(loaded));
    case CPL_TYPE_FLOAT:
      return static_cast<std::shared_ptr<ImageBase>>(
          std::make_shared<Image<float>>(loaded));
    case CPL_TYPE_INT:
      return static_cast<std::shared_ptr<ImageBase>>(
          std::make_shared<Image<int>>(loaded));
    default:
      throw InvalidTypeError(
          PYCPL_ERROR_LOCATION,
          "cpl_image_load returned an unexpected Image type");
  }
}

std::pair<std::shared_ptr<ImageBase>, int>
labelise_mask(const Mask& from)
{
  size n_regions;
  std::shared_ptr<ImageBase> labelled =
      cpl::core::ImageBase::make_image(Error::throw_errors_with(
          cpl_image_labelise_mask_create, from.ptr(), &n_regions));
  return std::make_pair(labelled, n_regions);
}

ImageBase::ImageBase(cpl_image* to_steal) : m_interface(to_steal) {}

ImageBase::ImageBase(cpl::core::size width, cpl::core::size height,
                     cpl_type type, size_t pixel_size, void* pixbuf)
    : m_interface(Error::throw_errors_with(cpl_image_new, width, height, type))
{
  if (pixbuf == nullptr) {
    return;
  }
  std::memcpy(Error::throw_errors_with(cpl_image_get_data, m_interface), pixbuf,
              width * height * pixel_size);
}

ImageBase::ImageBase(cpl::core::size width, cpl::core::size height,
                     cpl_type type, size_t pixel_size,
                     const std::string& pixbuf)
    : m_interface(Error::throw_errors_with(cpl_image_new, width, height, type))
{
  if (pixbuf.length() != get_size() * pixel_size) {
    throw IllegalInputError(
        PYCPL_ERROR_LOCATION,
        "Image input string size doesn't match width * height * bpp");
  }

  void* data_ptr = data();
  std::memcpy(data_ptr, pixbuf.c_str(), pixbuf.length());
}

ImageBase::~ImageBase() { cpl_image_delete(m_interface); }

size
ImageBase::get_width() const
{
  return Error::throw_errors_with(cpl_image_get_size_x, m_interface);
}

size
ImageBase::get_height() const
{
  return Error::throw_errors_with(cpl_image_get_size_y, m_interface);
}

size
ImageBase::get_size() const
{
  return Error::throw_errors_with(cpl_image_get_size_x, m_interface) *
         Error::throw_errors_with(cpl_image_get_size_y, m_interface);
}

void*
ImageBase::data()
{
  return reinterpret_cast<void*>(
      Error::throw_errors_with(cpl_image_get_data, m_interface));
}

const void*
ImageBase::data() const
{
  return reinterpret_cast<const void*>(
      Error::throw_errors_with(cpl_image_get_data_const, m_interface));
}

std::optional<double>
ImageBase::get_double(size y, size x) const
{
  std::pair<size, size> coords = cpl::core::cpl_coord(x, y);

  int is_rejected;
  double pixel = Error::throw_errors_with(
      cpl_image_get, m_interface, coords.first, coords.second, &is_rejected);

  if (is_rejected) {
    // TODO: Below commented out returns NaN. Got to make this universal for
    //       other data types though so going to just leave comment out until
    //       a decision can be reached
    // return std::numeric_limits<double>::quiet_NaN();
    return {};
  }
  return {pixel};
}

std::optional<std::complex<double>>
ImageBase::get_complex(size y, size x) const
{
  std::pair<size, size> coords = cpl::core::cpl_coord(x, y);

  int is_rejected;
  double _Complex pixel =
      Error::throw_errors_with(cpl_image_get_complex, m_interface, coords.first,
                               coords.second, &is_rejected);

  if (is_rejected) {
    return {};
  }
  return {complexd_to_cpp(pixel)};
}

std::optional<
    std::variant<double, int, float, std::complex<float>, std::complex<double>>>
ImageBase::get_either(size y, size x) const
{
  if (is_complex()) {
    std::optional<std::complex<double>> result = get_complex(y, x);
    if (result.has_value()) {
      return {result};
    }
  } else {
    std::optional<double> result = get_double(y, x);
    if (result.has_value()) {
      return {result};
    }
  }
  return {};
}

std::optional<Mask>
ImageBase::unset_bpm()
{
  cpl_mask* old_mask =
      Error::throw_errors_with(cpl_image_unset_bpm, m_interface);
  if (old_mask) {
    return {old_mask};
  } else {
    return {};
  }
}

std::optional<Mask>
ImageBase::set_bpm(Mask& shared_mask)
{
  if (shared_mask.m_borrows) {
    // Duplicate the input mask, since it can't be shraed to 2 images.
    shared_mask = Mask(shared_mask);
  }

  shared_mask.m_borrows = true;
  cpl_mask* old_mask = Error::throw_errors_with(cpl_image_set_bpm, m_interface,
                                                shared_mask.ptr());

  if (old_mask) {
    return {old_mask};
  } else {
    return {};
  }
}

Mask
ImageBase::get_bpm()
{
  Mask retval(Error::throw_errors_with(cpl_image_get_bpm, m_interface));
  retval.m_borrows = true;  // Borrows the cpl_mask*
  return retval;
}

std::optional<const Mask>
ImageBase::get_bpm() const
{
  const cpl_mask* bpm =
      Error::throw_errors_with(cpl_image_get_bpm_const, m_interface);
  if (!bpm) {
    return {};
  } else {
    Mask retval(const_cast<cpl_mask*>(bpm));
    retval.m_borrows = true;  // Borrows the cpl_mask*
    return {retval};
  }
}

void
ImageBase::set_double(size y, size x, double value)
{
  std::pair<size, size> coords = cpl::core::cpl_coord(x, y);

  Error::throw_errors_with(cpl_image_set, m_interface, coords.first,
                           coords.second, value);
}

void
ImageBase::set_complex(size y, size x, std::complex<double> value)
{
  std::pair<size, size> coords = cpl::core::cpl_coord(x, y);

  Error::throw_errors_with(cpl_image_set_complex, m_interface, coords.first,
                           coords.second, complex_to_c(value));
}

void
ImageBase::set_either(
    size y, size x,
    std::variant<double, int, float, std::complex<float>, std::complex<double>>
        value)
{
  if (double* dvalue = std::get_if<double>(&value)) {
    if (get_type() & CPL_TYPE_COMPLEX) {
      // The image type is complex but we only have a double.
      // Assign to the real part of the complex double pixel value
      return set_complex(
          y, x, std::complex<double>(static_cast<double>(*dvalue), 0.0));
    } else {
      // Set a pixel of a scalar image to a double value
      return set_double(y, x, *dvalue);
    }
  } else if (float* fvalue = std::get_if<float>(&value)) {
    if (get_type() & CPL_TYPE_COMPLEX) {
      // The image type is complex but we only have a float.
      // Assign to the real part of the complex double pixel value
      return set_complex(
          y, x, std::complex<double>(static_cast<double>(*fvalue), 0.0));
    } else {
      // Set a pixel of a scalar image to a double value
      return set_double(y, x, static_cast<double>(*fvalue));
    }
  } else if (int* ivalue = std::get_if<int>(&value)) {
    if (get_type() & CPL_TYPE_COMPLEX) {
      // The image type is complex but we only have a int.
      // Assign to the real part of the complex double pixel value
      return set_complex(
          y, x, std::complex<double>(static_cast<double>(*ivalue), 0.0));
    } else {
      // Set a pixel of a scalar image to a double value
      return set_double(y, x, static_cast<double>(*ivalue));
    }
  } else if (std::complex<double>* cdvalue =
                 std::get_if<std::complex<double>>(&value)) {
    if (get_type() & CPL_TYPE_COMPLEX) {
      // Set a pixel of a complex image to a complex value
      return set_complex(y, x, *cdvalue);
    } else {
      throw cpl::core::InvalidTypeError(
          PYCPL_ERROR_LOCATION,
          "This image only accepts scalar values, not complex");
    }
  } else if (std::complex<float>* cfvalue =
                 std::get_if<std::complex<float>>(&value)) {
    if (get_type() & CPL_TYPE_COMPLEX) {
      // Set a pixel of a complex image to a complex value
      // Since we have std::complex<float>, also convert it to
      // std::complex<double>
      return set_complex(y, x, static_cast<std::complex<double>>(*cfvalue));
    } else {
      throw cpl::core::InvalidTypeError(
          PYCPL_ERROR_LOCATION,
          "This image only accepts scalar values, not complex");
    }
  } else {
    throw std::logic_error("set_either was given unaccounted-for variant");
  }
}

void
ImageBase::conjugate()
{
  Error::throw_errors_with(cpl_image_conjugate, m_interface, m_interface);
}

namespace
{
template <class TPixel>
struct pair_maker
{
  static ImageBase::ImagePair run(const Image<TPixel>* self) = delete;
  static constexpr bool enabled = false;
};

// pair_maker only runs for converting complex to 2 non-complex
template <class TPixel>
struct pair_maker<std::complex<TPixel>>
{
  static ImageBase::ImagePair run(const Image<std::complex<TPixel>>* self)
  {
    return std::make_pair(
        std::make_shared<Image<TPixel>>(self->get_width(), self->get_height()),
        std::make_shared<Image<TPixel>>(self->get_width(), self->get_height()));
  }

  static constexpr bool enabled =
      is_image_pix<TPixel>::value && is_image_pix<std::complex<TPixel>>::value;
};
}  // namespace

ImageBase::ImagePair
ImageBase::fill_re_im() const
{
  ImageBase::ImagePair result =
      run_func_for_type<Image, pair_maker, ImageBase::ImagePair>(get_type(),
                                                                 this);
  Error::throw_errors_with(cpl_image_fill_re_im, result.first->ptr(),
                           result.second->ptr(), m_interface);
  return result;
}

ImageBase::ImagePair
ImageBase::fill_abs_arg() const
{
  ImageBase::ImagePair result =
      run_func_for_type<Image, pair_maker, ImageBase::ImagePair>(get_type(),
                                                                 this);
  Error::throw_errors_with(cpl_image_fill_abs_arg, result.first->ptr(),
                           result.second->ptr(), m_interface);
  return result;
}

void
ImageBase::fill_rejected(double a)
{
  Error::throw_errors_with(cpl_image_fill_rejected, m_interface, a);
}

void
ImageBase::fill_window(Window to_fill, double value)
{
  Error::throw_errors_with(cpl_image_fill_window, m_interface,
                           EXPAND_WINDOW(to_fill), value);
}

void
ImageBase::fill_noise_uniform(double min_pix, double max_pix)
{
  Error::throw_errors_with(cpl_image_fill_noise_uniform, m_interface, min_pix,
                           max_pix);
}

void
ImageBase::fill_gaussian(double xcen, double ycen, double norm, double sig_x,
                         double sig_y)
{
  std::pair<size, size> coords = cpl::core::cpl_coord(xcen, ycen);
  Error::throw_errors_with(cpl_image_fill_gaussian, m_interface, coords.first,
                           coords.second, norm, sig_x, sig_y);
}

std::string
ImageBase::dump_structure() const
{
  char* charBuff;
  size_t len;

  // Open char pointer as stream
  FILE* stream = open_memstream(&charBuff, &len);
  Error::throw_errors_with(cpl_image_dump_structure, m_interface, stream);
  fflush(stream);
  std::string returnString(charBuff);  // Cast to std::string
  fclose(stream);
  free(charBuff);

  return returnString;
}

std::string
ImageBase::dump(std::optional<Window> window) const
{
  cpl::core::Window w = window.value_or(cpl::core::Window::All);
  if (w.llx == 0 && w.lly == 0 && w.urx == 0 && w.ury == 0) {
    w = cpl::core::Window::All;
  }
  if (w != cpl::core::Window::All) {
    int dw = this->get_width();
    int dh = this->get_height();
    std::ostringstream oss;
    if ((w.llx < 0) || (w.lly < 0) || (w.urx >= dw) || (w.ury >= dh)) {
      oss << "Window(" << w.llx << "," << w.lly << "," << w.urx << "," << w.ury
          << ")";
      oss << " exceeds image bounds (0,0," << dw - 1 << "," << dh - 1 << ")";
      throw cpl::core::AccessOutOfRangeError(PYCPL_ERROR_LOCATION, oss.str());
    }
    if ((w.llx > w.urx) || (w.lly > w.ury)) {
      oss << "Invalid image window definition: ";
      oss << "Window(" << w.llx << "," << w.lly << "," << w.urx << "," << w.ury
          << ")";
      throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION, oss.str());
    }
  }
  if (w.llx == 0 && w.lly == 0 && w.urx == 0 && w.ury == 0) {
    w = cpl::core::Window::All;
  }
  if (w == cpl::core::Window::All) {
    w = {0, 0, this->get_width() - 1, this->get_height() - 1};
  }

  char* charBuff;
  size_t len;

  // Open char pointer as stream
  FILE* stream = open_memstream(&charBuff, &len);
  Error::throw_errors_with(cpl_image_dump_window, m_interface, EXPAND_WINDOW(w),
                           stream);
  fflush(stream);
  std::string returnString(charBuff);  // Cast to std::string
  fclose(stream);
  free(charBuff);

  return returnString;
}

namespace
{
template <class TPixel>
struct duplicate_runner
{
  static std::shared_ptr<ImageBase> run(const Image<TPixel>* self)
  {
    return std::make_shared<Image<TPixel>>(
        Error::throw_errors_with(cpl_image_duplicate, self->ptr()));
  }

  static constexpr bool enabled = is_image_pix<TPixel>::value;
};

template <class TPixel>
struct cast_runner
{
  static std::shared_ptr<ImageBase>
  run(const Image<TPixel>* /* nothing */, cpl_image* from,
      const cpl_type new_type)
  {
    return std::make_shared<Image<TPixel>>(
        Error::throw_errors_with(cpl_image_cast, from, new_type));
  }

  static constexpr bool enabled = is_image_pix<TPixel>::value;
};
}  // namespace

std::shared_ptr<ImageBase>
ImageBase::duplicate() const
{
  return cpl::core::run_func_for_type<Image, duplicate_runner,
                                      std::shared_ptr<ImageBase>>(get_type(),
                                                                  this);
}

std::shared_ptr<ImageBase>
ImageBase::cast(cpl_type type) const
{
  return cpl::core::run_func_for_type<Image, cast_runner,
                                      std::shared_ptr<ImageBase>, ImageBase>(
      type, nullptr, m_interface, type);
}

void
ImageBase::save(const char* filename, const PropertyList& pl, unsigned mode,
                cpl_type dtype) const
{
  cpl_type pixel_type =
      (dtype == CPL_TYPE_UNSPECIFIED) ? cpl_image_get_type(m_interface) : dtype;
  Error::throw_errors_with(cpl_image_save, m_interface, filename, pixel_type,
                           pl.ptr().get(), mode);
}

bool
ImageBase::is_complex() const
{
  return get_type() & CPL_TYPE_COMPLEX;
}

const cpl_image*
ImageBase::ptr() const
{
  return m_interface;
}

cpl_image*
ImageBase::ptr()
{
  return m_interface;
}

cpl_image*
ImageBase::unwrap(ImageBase&& self)
{
  cpl_image* interface = self.m_interface;
  self.m_interface = nullptr;
  return interface;
}

ImageBase::operator std::string() const
{
  return std::string(reinterpret_cast<const char*>(data()));
}

namespace
{
template <class TPixel>
struct image_maker
{
  static std::shared_ptr<ImageBase>
  run(Image<TPixel>*, size width, size height, void* pixbuf)
  {
    return std::make_shared<Image<TPixel>>(width, height,
                                           reinterpret_cast<TPixel*>(pixbuf));
  }

  static std::shared_ptr<ImageBase>
  run(Image<TPixel>*, size width, size height, const std::string& pixbuf)
  {
    return std::make_shared<Image<TPixel>>(width, height, pixbuf);
  }

  static std::shared_ptr<ImageBase> run(Image<TPixel>*, cpl_image* to_steal)
  {
    return std::make_shared<Image<TPixel>>(to_steal);
  }

  static constexpr bool enabled = is_image_pix<TPixel>::value;
};
}  // namespace

std::shared_ptr<ImageBase>
ImageBase::make_image(size width, size height, cpl_type type, void* pixbuf)
{
  // Throws InvalidTypeError if is_image_pix is not true for the given cpl_type
  try {
    return cpl::core::run_func_for_type<
        Image, image_maker, std::shared_ptr<ImageBase> /*Return*/, ImageBase>(
        type, nullptr, width, height, pixbuf);
  }
  catch (const cpl::core::InvalidTypeError& error) {
    throw cpl::core::InvalidTypeError(PYCPL_ERROR_LOCATION,
                                      "make_image makes INT, FLOAT, DOUBLE, "
                                      "FLOAT_COMPLEX and DOUBLE_COMPLEX.",
                                      error);
  }
}

std::shared_ptr<ImageBase>
ImageBase::make_image(cpl_image* input)
{
  return cpl::core::run_func_for_type<
      Image, image_maker, std::shared_ptr<ImageBase> /*Return*/, ImageBase,
      /* Var args can't be deduced properly here */
      cpl_image*>(cpl_image_get_type(input), nullptr, input);
}

std::shared_ptr<ImageBase>
ImageBase::make_image(size width, size height, cpl_type type,
                      const std::string& pixbuf)
{
  // Throws InvalidTypeError if is_image_pix is not true for the given cpl_type
  try {
    return cpl::core::run_func_for_type<
        Image, image_maker, std::shared_ptr<ImageBase> /*Return*/, ImageBase,
        size, size, const std::string&>(type, nullptr, width, height, pixbuf);
  }
  catch (const cpl::core::InvalidTypeError& error) {
    throw cpl::core::InvalidTypeError(
        PYCPL_ERROR_LOCATION,
        "make_image takes INT, FLOAT, DOUBLE, FLOAT_COMPLEX or DOUBLE_COMPLEX",
        error);
  }
}

void
ImageBase::add(const ImageBase& im2)
{
  Error::throw_errors_with(cpl_image_add, m_interface, im2.m_interface);
}

void
ImageBase::subtract(const ImageBase& im2)
{
  Error::throw_errors_with(cpl_image_subtract, m_interface, im2.m_interface);
}

void
ImageBase::multiply(const ImageBase& im2)
{
  Error::throw_errors_with(cpl_image_multiply, m_interface, im2.m_interface);
}

void
ImageBase::divide(const ImageBase& im2)
{
  Error::throw_errors_with(cpl_image_divide, m_interface, im2.m_interface);
}

void
ImageBase::add_scalar(double scalar)
{
  Error::throw_errors_with(cpl_image_add_scalar, m_interface, scalar);
}

void
ImageBase::subtract_scalar(double scalar)
{
  Error::throw_errors_with(cpl_image_subtract_scalar, m_interface, scalar);
}

void
ImageBase::multiply_scalar(double scalar)
{
  Error::throw_errors_with(cpl_image_multiply_scalar, m_interface, scalar);
}

void
ImageBase::divide_scalar(double scalar)
{
  Error::throw_errors_with(cpl_image_divide_scalar, m_interface, scalar);
}

void
ImageBase::power(double exponent)
{
  Error::throw_errors_with(cpl_image_power, m_interface, exponent);
}

void
ImageBase::exponential(double base)
{
  Error::throw_errors_with(cpl_image_exponential, m_interface, base);
}

void
ImageBase::logarithm(double base)
{
  Error::throw_errors_with(cpl_image_logarithm, m_interface, base);
}

void
ImageBase::normalise(cpl_norm mode)
{
  Error::throw_errors_with(cpl_image_normalise, m_interface, mode);
}

void
ImageBase::abs()
{
  Error::throw_errors_with(cpl_image_abs, m_interface);
}

void
ImageBase::hypot(const ImageBase& first, const ImageBase& second)
{
  Error::throw_errors_with(cpl_image_hypot, m_interface, first.m_interface,
                           second.m_interface);
}

void
ImageBase::and_with(const ImageBase& second)
{
  Error::throw_errors_with(cpl_image_and, m_interface, nullptr,
                           second.m_interface);
}

void
ImageBase::or_with(const ImageBase& second)
{
  Error::throw_errors_with(cpl_image_or, m_interface, nullptr,
                           second.m_interface);
}

void
ImageBase::xor_with(const ImageBase& second)
{
  Error::throw_errors_with(cpl_image_xor, m_interface, nullptr,
                           second.m_interface);
}

void
ImageBase::negate()
{
  Error::throw_errors_with(cpl_image_not, m_interface, nullptr);
}

void
ImageBase::and_scalar(cpl_bitmask second)
{
  Error::throw_errors_with(cpl_image_and_scalar, m_interface, nullptr, second);
}

void
ImageBase::or_scalar(cpl_bitmask second)
{
  Error::throw_errors_with(cpl_image_or_scalar, m_interface, nullptr, second);
}

void
ImageBase::xor_scalar(cpl_bitmask second)
{
  Error::throw_errors_with(cpl_image_xor_scalar, m_interface, nullptr, second);
}

namespace
{
template <class TPixel>
struct image_extractor
{
  static std::shared_ptr<ImageBase> run(const Image<TPixel>* self, Window area)
  {
    return std::make_shared<Image<TPixel>>(Error::throw_errors_with(
        cpl_image_extract, self->ptr(), EXPAND_WINDOW(area)));
  }

  static constexpr bool enabled = is_image_pix<TPixel>::value;
};
}  // namespace

std::shared_ptr<ImageBase>
ImageBase::extract(Window area) const
{
  return cpl::core::run_func_for_type<Image, image_extractor,
                                      std::shared_ptr<ImageBase> /*Return*/
                                      >(get_type(), (ImageBase*)this, area);
}

void
ImageBase::rotate(int rot)
{
  Error::throw_errors_with(cpl_image_turn, m_interface, rot);
}

void
ImageBase::shift(size dy, size dx)
{
  Error::throw_errors_with(cpl_image_shift, m_interface, dx, dy);
}

void
ImageBase::copy_into(const ImageBase& im2, size ypos, size xpos)
{
  std::pair<size, size> coords = cpl::core::cpl_coord(xpos, ypos);

  Error::throw_errors_with(cpl_image_copy, m_interface, im2.m_interface,
                           coords.first, coords.second);
}

void
ImageBase::flip(int angle)
{
  Error::throw_errors_with(cpl_image_flip, m_interface, angle);
}

void
ImageBase::move(size nb_cut, const std::vector<size>& positions)
{
  if (static_cast<std::vector<size>::size_type>(nb_cut * nb_cut) !=
      positions.size()) {
    throw IllegalInputError(PYCPL_ERROR_LOCATION,
                            "positions not equal to nb_cut^2");
  }
  if (this->get_width() % nb_cut != 0 || this->get_height() % nb_cut != 0 ||
      nb_cut < 1) {
    throw IllegalInputError(PYCPL_ERROR_LOCATION,
                            "nb_cut of " + std::to_string(nb_cut) +
                                " cant slice image of shape" +
                                std::to_string(this->get_width()) + "x" +
                                std::to_string(this->get_height()));
  }
  Error::throw_errors_with(cpl_image_move, m_interface, nb_cut, &positions[0]);
}

std::pair<double, double>
ImageBase::get_fwhm(size ypos, size xpos) const
{
  std::pair<size, size> coords = cpl::core::cpl_coord(xpos, ypos);

  double fwhm_x;
  double fwhm_y;
  Error::throw_errors_with(cpl_image_get_fwhm, m_interface, coords.first,
                           coords.second, &fwhm_x, &fwhm_y);
  return std::make_pair(fwhm_y, fwhm_x);
}

cpl::core::Bivector
ImageBase::iqe(Window area) const
{
  return cpl::core::Bivector(Error::throw_errors_with(
      cpl_image_iqe, m_interface, EXPAND_WINDOW(area)));
}

std::shared_ptr<ImageBase>
ImageBase::warp_polynomial(const Polynomial& poly_y, const Polynomial& poly_x,
                           const cpl::core::Vector& yprofile, double yradius,
                           const cpl::core::Vector& xprofile, double xradius,
                           std::tuple<size, size> out_dim, cpl_type out_type)
{
  auto [out_width, out_height] = out_dim;
  cpl_image* out =
      Error::throw_errors_with(cpl_image_new, out_width, out_height, out_type);
  Error::throw_errors_with(cpl_image_warp_polynomial, out, m_interface,
                           poly_x.ptr(), poly_y.ptr(), xprofile.ptr(), xradius,
                           yprofile.ptr(), yradius);
  return ImageBase::make_image(out);
}

std::shared_ptr<ImageBase>
ImageBase::warp(const ImageBase& deltay, const ImageBase& deltax,
                const cpl::core::Vector& yprofile, double yradius,
                const cpl::core::Vector& xprofile, double xradius)
{
  // New image needs same dimensions as deltax and deltay, so we just base it
  // off deltax's dimensions Keep same type as self
  cpl_image* new_image = Error::throw_errors_with(
      cpl_image_new, deltax.get_width(), deltax.get_height(), get_type());
  Error::throw_errors_with(cpl_image_warp, new_image, m_interface,
                           deltax.m_interface, deltay.m_interface,
                           xprofile.ptr(), xradius, yprofile.ptr(), yradius);
  return ImageBase::make_image(new_image);
}

void
ImageBase::fill_jacobian_polynomial(const Polynomial& poly_x,
                                    const Polynomial& poly_y)
{
  Error::throw_errors_with(cpl_image_fill_jacobian_polynomial, m_interface,
                           poly_x.ptr(), poly_y.ptr());
}

void
ImageBase::fill_jacobian(const ImageBase& deltax, const ImageBase& deltay)
{
  Error::throw_errors_with(cpl_image_fill_jacobian, m_interface,
                           deltax.m_interface, deltay.m_interface);
}

namespace
{
template <class TPixel>
struct extract_subsample_runner
{
  static std::shared_ptr<ImageBase>
  run(const Image<TPixel>* self, size ystep, size xstep)
  {
    return std::make_shared<Image<TPixel>>(Error::throw_errors_with(
        cpl_image_extract_subsample, self->ptr(), xstep, ystep));
  }

  static constexpr bool enabled = is_image_pix<TPixel>::value;
};

template <class TPixel>
struct rebin_runner
{
  static std::shared_ptr<ImageBase> run(const Image<TPixel>* self, size ystart,
                                        size xstart, size ystep, size xstep)
  {
    return std::make_shared<Image<TPixel>>(Error::throw_errors_with(
        cpl_image_rebin, self->ptr(), xstart, ystart, xstep, ystep));
  }

  static constexpr bool enabled = is_image_pix<TPixel>::value;
};
}  // namespace

std::shared_ptr<ImageBase>
ImageBase::extract_subsample(size ystep, size xstep) const
{
  return cpl::core::run_func_for_type<Image, extract_subsample_runner,
                                      std::shared_ptr<ImageBase>>(
      get_type(), (ImageBase*)this, ystep, xstep);
}

std::shared_ptr<ImageBase>
ImageBase::rebin(size ystart, size xstart, size ystep, size xstep) const
{
  return cpl::core::run_func_for_type<Image, rebin_runner,
                                      std::shared_ptr<ImageBase>>(
      get_type(), (ImageBase*)this, ystart, xstart, ystep, xstep);
}

std::tuple<double, double>
ImageBase::get_interpolated(double ypos, double xpos,
                            const cpl::core::Vector& yprofile, double yradius,
                            const cpl::core::Vector& xprofile,
                            double xradius) const
{
  double pconfid;  // Output variable from cpl_image_get_interpolated, store
                   // both the result and confidence value in the tuple
  double interpolated = Error::throw_errors_with(
      cpl_image_get_interpolated, m_interface, xpos, ypos, xprofile.ptr(),
      xradius, yprofile.ptr(), yradius, &pconfid);
  return std::make_tuple(interpolated, pconfid);
}

size
ImageBase::count_rejected() const
{
  return Error::throw_errors_with(cpl_image_count_rejected, m_interface);
}

bool
ImageBase::is_rejected(size y, size x) const
{
  std::pair<size, size> coords = cpl::core::cpl_coord(x, y);

  return Error::throw_errors_with(cpl_image_is_rejected, m_interface,
                                  coords.first, coords.second);
}

void
ImageBase::reject(size y, size x)
{
  std::pair<size, size> coords = cpl::core::cpl_coord(x, y);

  Error::throw_errors_with(cpl_image_reject, m_interface, coords.first,
                           coords.second);
}

void
ImageBase::accept(size y, size x)
{
  std::pair<size, size> coords = cpl::core::cpl_coord(x, y);

  Error::throw_errors_with(cpl_image_accept, m_interface, coords.first,
                           coords.second);
}

void
ImageBase::accept_all()
{
  Error::throw_errors_with(cpl_image_accept_all, m_interface);
}

void
ImageBase::reject_from_mask(const Mask& map)
{
  Error::throw_errors_with(cpl_image_reject_from_mask, m_interface, map.ptr());
}

void
ImageBase::reject_value(cpl_value mode)
{
  Error::throw_errors_with(cpl_image_reject_value, m_interface, mode);
}

double
ImageBase::get_min(std::optional<Window> area) const
{
  if (!area.has_value()) {
    return Error::throw_errors_with(cpl_image_get_min, m_interface);
  } else {
    Window selected_window = area.value();
    return Error::throw_errors_with(cpl_image_get_min_window, m_interface,
                                    EXPAND_WINDOW(selected_window));
  }
}

double
ImageBase::get_max(std::optional<Window> area) const
{
  if (!area.has_value()) {
    return Error::throw_errors_with(cpl_image_get_max, m_interface);
  } else {
    Window selected_window = area.value();
    return Error::throw_errors_with(cpl_image_get_max_window, m_interface,
                                    EXPAND_WINDOW(selected_window));
  }
}

double
ImageBase::get_mean(std::optional<Window> area) const
{
  if (!area.has_value()) {
    return Error::throw_errors_with(cpl_image_get_mean, m_interface);
  } else {
    Window selected_window = area.value();
    return Error::throw_errors_with(cpl_image_get_mean_window, m_interface,
                                    EXPAND_WINDOW(selected_window));
  }
}

double
ImageBase::get_median(std::optional<Window> area) const
{
  if (!area.has_value()) {
    return Error::throw_errors_with(cpl_image_get_median, m_interface);
  } else {
    Window selected_window = area.value();
    return Error::throw_errors_with(cpl_image_get_median_window, m_interface,
                                    EXPAND_WINDOW(selected_window));
  }
}

double
ImageBase::get_stdev(std::optional<Window> area) const
{
  if (!area.has_value()) {
    return Error::throw_errors_with(cpl_image_get_stdev, m_interface);
  } else {
    Window selected_window = area.value();
    return Error::throw_errors_with(cpl_image_get_stdev_window, m_interface,
                                    EXPAND_WINDOW(selected_window));
  }
}

double
ImageBase::get_flux(std::optional<Window> area) const
{
  if (!area.has_value()) {
    return Error::throw_errors_with(cpl_image_get_flux, m_interface);
  } else {
    Window selected_window = area.value();
    return Error::throw_errors_with(cpl_image_get_flux_window, m_interface,
                                    EXPAND_WINDOW(selected_window));
  }
}

double
ImageBase::get_absflux(std::optional<Window> area) const
{
  if (!area.has_value()) {
    return Error::throw_errors_with(cpl_image_get_absflux, m_interface);
  } else {
    Window selected_window = area.value();
    return Error::throw_errors_with(cpl_image_get_absflux_window, m_interface,
                                    EXPAND_WINDOW(selected_window));
  }
}

double
ImageBase::get_sqflux(std::optional<Window> area) const
{
  if (!area.has_value()) {
    return Error::throw_errors_with(cpl_image_get_sqflux, m_interface);
  } else {
    Window selected_window = area.value();
    return Error::throw_errors_with(cpl_image_get_sqflux_window, m_interface,
                                    EXPAND_WINDOW(selected_window));
  }
}

double
ImageBase::get_centroid_x(std::optional<Window> area) const
{
  if (!area.has_value()) {
    return Error::throw_errors_with(cpl_image_get_centroid_x, m_interface);
  } else {
    Window selected_window = area.value();
    return Error::throw_errors_with(cpl_image_get_centroid_x_window,
                                    m_interface,
                                    EXPAND_WINDOW(selected_window));
  }
}

double
ImageBase::get_centroid_y(std::optional<Window> area) const
{
  if (!area.has_value()) {
    return Error::throw_errors_with(cpl_image_get_centroid_y, m_interface);
  } else {
    Window selected_window = area.value();
    return Error::throw_errors_with(cpl_image_get_centroid_y_window,
                                    m_interface,
                                    EXPAND_WINDOW(selected_window));
  }
}

std::pair<size, size>
ImageBase::get_minpos(std::optional<Window> area) const
{
  std::pair<size, size> retval;
  if (!area.has_value()) {
    Error::throw_errors_with(cpl_image_get_minpos, m_interface, &retval.first,
                             &retval.second);
  } else {
    Window selected_window = area.value();
    Error::throw_errors_with(cpl_image_get_minpos_window, m_interface,
                             EXPAND_WINDOW(selected_window), &retval.first,
                             &retval.second);
  }
  return cpl_to_coord(retval.second, retval.first);
}

std::pair<size, size>
ImageBase::get_maxpos(std::optional<Window> area) const
{
  std::pair<size, size> retval;
  if (!area.has_value()) {
    Error::throw_errors_with(cpl_image_get_maxpos, m_interface, &retval.first,
                             &retval.second);
  } else {
    Window selected_window = area.value();
    Error::throw_errors_with(cpl_image_get_maxpos_window, m_interface,
                             EXPAND_WINDOW(selected_window), &retval.first,
                             &retval.second);
  }
  return cpl_to_coord(retval.second, retval.first);
}

std::pair<double, double>
ImageBase::get_median_dev(std::optional<Window> area) const
{
  if (!area.has_value()) {
    double median_dev;
    double median = Error::throw_errors_with(cpl_image_get_median_dev,
                                             m_interface, &median_dev);
    return std::make_pair(median, median_dev);
  } else {
    Window selected_window = area.value();
    double median_dev;
    double median =
        Error::throw_errors_with(cpl_image_get_median_dev_window, m_interface,
                                 EXPAND_WINDOW(selected_window), &median_dev);
    return std::make_pair(median, median_dev);
  }
}

std::pair<double, double>
ImageBase::get_mad(std::optional<Window> area) const
{
  if (!area.has_value()) {
    double mad;
    double median =
        Error::throw_errors_with(cpl_image_get_mad, m_interface, &mad);

    return std::make_pair(median, mad);
  } else {
    Window selected_window = area.value();
    double mad;
    double median =
        Error::throw_errors_with(cpl_image_get_mad_window, m_interface,
                                 EXPAND_WINDOW(selected_window), &mad);

    return std::make_pair(median, mad);
  }
}

std::shared_ptr<ImageBase>
ImageBase::filter_mask(const Mask& kernel, cpl_filter_mode filter,
                       cpl_border_mode border, cpl_type dtype)
{
  size width = get_width();
  size height = get_height();
  if (border == CPL_BORDER_CROP) {
    // In this mode the output image must be smaller than the original image.
    width -= (kernel.get_width() - 1);
    height -= (kernel.get_height() - 1);
  }
  auto filtered_image = ImageBase::make_image(width, height, dtype);
  Error::throw_errors_with(cpl_image_filter_mask, (*filtered_image).m_interface,
                           m_interface, kernel.ptr(), filter, border);
  return filtered_image;
}

std::shared_ptr<ImageBase>
ImageBase::filter(const Matrix& kernel, cpl_filter_mode filter,
                  cpl_border_mode border, cpl_type dtype)
{
  auto filtered_image = ImageBase::make_image(get_width(), get_height(), dtype);
  Error::throw_errors_with(cpl_image_filter, (*filtered_image).m_interface,
                           m_interface, kernel.ptr(), filter, border);
  return filtered_image;
}

void
ImageBase::threshold(double lo_cut, double hi_cut, double assign_lo_cut,
                     double assign_hi_cut) const
{
  if (is_complex()) {
    throw InvalidTypeError(PYCPL_ERROR_LOCATION,
                           "Image.threshold be used complex images");
  }

  // Mask newmask(get_width(), get_height());
  Error::throw_errors_with(cpl_image_threshold, m_interface, lo_cut, hi_cut,
                           assign_lo_cut, assign_hi_cut);
}

std::shared_ptr<ImageBase>
ImageBase::fft(std::optional<std::shared_ptr<ImageBase>> img_imag,
               unsigned mode)
{
  std::shared_ptr<ImageBase> real;
  std::shared_ptr<ImageBase> imag;
  // Check if the calling image is complex, if it is extract individual
  // components using. Otherwise duplicate m_interface (real) and img_imag
  if (get_type() == CPL_TYPE_DOUBLE_COMPLEX) {
    // Extract individual component types to use
    std::tie(real, imag) = fill_re_im();
  } else {  // If not complex, then duplicate directly. If either input is not
            // double then the cpl function itself should be able to catch it
    real = duplicate();
    if (img_imag.has_value()) {
      imag = img_imag.value()->duplicate();
    } else {
      imag = ImageBase::make_image(get_width(), get_height(), CPL_TYPE_DOUBLE);
    }
  }
  Error::throw_errors_with(cpl_image_fft, real->ptr(), imag->ptr(), mode);

  size image_size = get_width() * get_height();

  double* real_data = cpl_image_get_data_double(real->ptr());
  double* imag_data = cpl_image_get_data_double(imag->ptr());

  // Merge the two into a single complex image
  // Allocate data for the return image
  double _Complex* output_data =
      (double _Complex*)cpl_calloc(image_size, sizeof(double _Complex));

  for (int i = 0; i < image_size; i++) {
    double(&z)[2] = reinterpret_cast<double(&)[2]>(output_data[i]);
    z[0] = real_data[i];
    // TODO: Document details regarding the cpl fft convention to help explain
    // the sign flip.
    z[1] = imag_data[i];  // The imaginary component when comparing to numpy is
                          // sign flipped.
  }
  cpl_image* output =
      cpl_image_wrap_double_complex(get_width(), get_height(), output_data);
  return ImageBase::make_image(output);
}

ImageBase::ImageBase(const ImageBase& other)
    : m_interface(
          Error::throw_errors_with(cpl_image_duplicate, other.m_interface))
{
}

ImageBase::ImageBase(ImageBase&& other) noexcept
    : m_interface(other.m_interface)
{
  other.m_interface = nullptr;
}

ImageBase&
ImageBase::operator=(const ImageBase& other)
{
  cpl_image_delete(m_interface);  // Doesn't throw
  m_interface = nullptr;
  // If the below does throw, we don't want m_interface to point anywhere
  m_interface =
      Error::throw_errors_with(cpl_image_duplicate, other.m_interface);
  return *this;
}

ImageBase&
ImageBase::operator=(ImageBase&& other) noexcept
{
  cpl_image_delete(m_interface);  // Doesn't throw
  m_interface = other.m_interface;
  other.m_interface = nullptr;
  return *this;
}

bool
ImageBase::operator==(const ImageBase& other) const
{
  if (get_width() != other.get_width() || get_height() != other.get_height() ||
      get_type() != other.get_type()) {
    return false;
  }
  return std::memcmp(data(), other.data(), get_size() * pixel_size()) == 0;
}

bool
ImageBase::operator!=(const ImageBase& other) const
{
  return !operator==(other);
}

Vector
ImageBase::vector_from_row(int pos)
{
  cpl_vector* output = Error::throw_errors_with(
      cpl_vector_new_from_image_row, m_interface,
      pos - 1);  // convert pos to PyCPL coords (0 for bottom)
  return Vector(output);
}

Vector
ImageBase::vector_from_column(int pos)
{
  cpl_vector* output = Error::throw_errors_with(
      cpl_vector_new_from_image_column, m_interface,
      pos - 1);  // convert pos to PyCPL coords (0 for leftmost)
  return Vector(output);
}

/*
    The following 5 specialisations of get_pixel and 5 set_pixel
    would have hopefully been collapsed to a partial template specialisation,
    then only needing 2 specialisations (1 for complex, 1 for not complex).
    But C++ doesn't let us do that on functions
*/

template <>
std::optional<double>
Image<double>::get_pixel(size y, size x) const
{
  return get_double(y, x);
}

template <>
std::optional<float>
Image<float>::get_pixel(size y, size x) const
{
  std::optional<double> max_precision = get_double(y, x);
  if (!max_precision.has_value()) {
    return {};
  }
  return {static_cast<float>(*max_precision)};
}

template <>
std::optional<int>
Image<int>::get_pixel(size y, size x) const
{
  std::optional<double> max_precision = get_double(y, x);
  if (!max_precision.has_value()) {
    return {};
  }
  return {static_cast<int>(*max_precision)};
}

template <>
std::optional<std::complex<float>>
Image<std::complex<float>>::get_pixel(size y, size x) const
{
  std::optional<std::complex<double>> max_precision = get_complex(y, x);
  if (!max_precision.has_value()) {
    return {};
  }
  return {{static_cast<float>(max_precision->real()),
           static_cast<float>(max_precision->imag())}};
}

template <>
std::optional<std::complex<double>>
Image<std::complex<double>>::get_pixel(size y, size x) const
{
  return get_complex(y, x);
}

template <>
Image<double>&
Image<double>::set_pixel(size y, size x, double value)
{
  set_double(y, x, value);
  return *this;
}

template <>
Image<float>&
Image<float>::set_pixel(size y, size x, float value)
{
  set_double(y, x, static_cast<double>(value));
  return *this;
}

template <>
Image<int>&
Image<int>::set_pixel(size y, size x, int value)
{
  set_double(y, x, static_cast<double>(value));
  return *this;
}

template <>
Image<std::complex<double>>&
Image<std::complex<double>>::set_pixel(size y, size x,
                                       std::complex<double> value)
{
  set_complex(y, x, value);
  return *this;
}

template <>
Image<std::complex<float>>&
Image<std::complex<float>>::set_pixel(size y, size x, std::complex<float> value)
{
  set_complex(y, x, static_cast<std::complex<double>>(value));
  return *this;
}

template <>
Image<int>::Image(const Mask& from)
    : Image(Error::throw_errors_with(cpl_image_new_from_mask, from.ptr()))
{
}

}  // namespace core
}  // namespace cpl
