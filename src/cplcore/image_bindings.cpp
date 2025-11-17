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

#include "cplcore/image_bindings.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include <cpl_memory.h>
#include <pybind11/complex.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "cplcore/coords.hpp"
#include "cplcore/image.hpp"
#include "cplcore/imagelist.hpp"
#include "cplcore/mask.hpp"
#include "cplcore/type_bindings.hpp"
#include "cplcore/window_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive
#include "dump_handler.hpp"
#include "path_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive

namespace py = pybind11;

using size = cpl::core::size;

using generic_pixel =
    std::variant<double, int, float, std::complex<float>, std::complex<double>>;

struct ImageRowAccessor
{
  // Reference to ensure python doesn't erase the owning image before its
  // used
  py::object image_reference;

  // Quick access to the image
  std::shared_ptr<cpl::core::ImageBase> image;

  // The row of the above image that this row accessor is for
  size y;

  // Iterator index for python __next__
  int iter_idx = 0;
};

namespace
{

template <class TPixel>
struct image_iter_setter
{
  static void
  run(cpl::core::Image<TPixel>* instance, size x, size y, py::object& pixel)
  {
    instance->set_pixel(y, x, pixel.cast<TPixel>());
  }

  constexpr static bool enabled = cpl::core::is_image_pix<TPixel>::value;
};
}  // namespace

std::shared_ptr<cpl::core::ImageBase>
image_from_python_matrix(cpl_type image_type, py::object matrix)
{
  py::object builtins = py::module::import("builtins");
  py::object py_iter_builtin = builtins.attr("iter");
  py::object py_next_builtin = builtins.attr("next");
  std::function<size(const py::object&)> py_len_builtin =
      [&builtins](const py::object& o) {
        return builtins.attr("len")(o).cast<size>();
      };

  // Iterate through first arg at this part:
  py::object height_iter;
  // First object is used for width calculation
  // but has to be saved to be used later for setting pixels
  py::object row;
  py::object width_iter;
  size height;
  size width;
  try {
    height_iter = py_iter_builtin(matrix);
    height = py_len_builtin(matrix);

    row = py_next_builtin(height_iter);
    width = py_len_builtin(row);
  }
  catch (const py::type_error& /* unused */) {
    throw py::type_error(
        std::string(
            "expected sized iterable (len >0) of sized iterables, not ") +
        matrix.get_type().attr("__name__").cast<std::string>());
  }

  std::shared_ptr<cpl::core::ImageBase> new_image =
      cpl::core::ImageBase::make_image(width, height, image_type);

  // Set all pixels from the first iterable
  width_iter = py_iter_builtin(row);
  for (size x = 0; x < width; ++x) {
    py::object pixel = py_next_builtin(width_iter);

    cpl::core::run_func_for_type<cpl::core::Image, image_iter_setter, void>(
        image_type, new_image.get(), x, 0, pixel);
  }

  // Set all pixels from the rest of the iterables
  for (size y = 1; y < height; ++y) {
    row = py_next_builtin(height_iter);
    width_iter = py_iter_builtin(row);

    if (size check_width = py_len_builtin(row) != width) {
      std::ostringstream err_msg;
      err_msg << "expected all iterables have the same size: " << y
              << "expected iterable " << y << " to have size " << width
              << ", not " << check_width;
      throw py::value_error(err_msg.str().c_str());
    }

    for (size x = 0; x < width; ++x) {
      py::object pixel = py_next_builtin(width_iter);

      cpl::core::run_func_for_type<cpl::core::Image, image_iter_setter, void>(
          image_type, new_image.get(), x, y, pixel);
    }
  }

  return new_image;
}

std::shared_ptr<cpl::core::ImageBase>
image_from_arr(py::iterable obj)
{
  // Numpy array or other buffer first argument
  // If iteration of the ndarray is too slow,
  // then (given the numpy array is perfectly native & matches size)
  // a memcpy from the numpy array could be faster.

  py::array input_arr;
  try {
    input_arr = (py::array)obj;  // Convert arg to numpy array
  }
  catch (const py::cast_error& /* unused */) {
    throw py::type_error(std::string("expected numpy compatible array, not ") +
                         obj.get_type().attr("__name__").cast<std::string>());
  }

  py::buffer buf;
  try {
    buf = input_arr.cast<py::buffer>();
  }
  catch (const py::cast_error& /* unused */) {
    throw py::type_error(std::string("expected numpy array, or implementor of "
                                     "cpython buffer protocol, not ") +
                         obj.get_type().attr("__name__").cast<std::string>());
  }

  py::buffer_info info = buf.request();

  // Check shape is ok
  std::vector<ssize_t> shape = info.shape;
  size width, height;
  if (info.ndim == 2) {
    height = shape[0];
    width = shape[1];
  } else {
    std::ostringstream err_msg;
    err_msg << "expected 2-dimensional buffer, not " << info.shape.size()
            << "-dimensional buffer";
    throw py::value_error(err_msg.str());
  }

  std::optional<cpl_type> inferred_type =
      cpl::pystruct_type_to_cpl(info.format);

  if (
      // All padding/alignment is native c-style:
      cpl::pystruct_type_is_native(info.format) &&
      // Type is a good CPL type:
      inferred_type.has_value() && inferred_type != CPL_TYPE_STRING &&
      std::all_of(info.strides.begin(), info.strides.end(),
                  [](ssize_t stride) -> bool {
                    // All strides are 1.
                    return 1 == stride;
                  })) {
    assert(info.itemsize ==
           static_cast<ssize_t>(cpl_type_get_sizeof(*inferred_type)));
    // The numpy storage exactly matches C-style storage
    return cpl::core::ImageBase::make_image(width, height, *inferred_type,
                                            info.ptr);
  } else {
    // The numpy storage format doesn't match
    // This part of the code will iterate through each element
    // But first, a cpl_type is required

    if (!py::hasattr(input_arr, "dtype")) {
      throw py::type_error(
          std::string("expected numpy array, not ") +
          input_arr.get_type().attr("__name__").cast<std::string>());
    }

    py::object numpy_dtype = input_arr.attr("dtype");
    if (auto np_derived_type = cpl::numpy_type_to_cpl(numpy_dtype)) {
      if (np_derived_type == CPL_TYPE_LONG) {
        // Convert to int type from python int type (int64/int32)
        input_arr = (py::array_t<int>)input_arr;
        np_derived_type = CPL_TYPE_INT;
        // Casting a value too large for C int will result in pybind throwing a
        // casting error: RuntimeError: Unable to cast Python instance of type
        // <class 'int'> to C++ type 'int'

        // TODO: Is there a better way around this?
      }

      // TODO: Is it possible to just use the type from the struct?
      // If so, then that would be preferred, as it avoids the
      // dependency on numpy (any buffer implementor can use it)
      // An assertion fail here says that the above is not possible.
      // assert(inferred_type == np_derived_type);

      return image_from_python_matrix(*np_derived_type, obj);

    } else {
      throw py::type_error(
          std::string("numpy array is expected to be one of"
                      " the supported cpl types, not ") +
          numpy_dtype.get_type().attr("__name__").cast<std::string>());
    }
  }
}

namespace
{
template <class TPixel>
struct buffer_info_getter
{
  static py::buffer_info run(cpl::core::Image<TPixel>* inst)
  {
    return py::buffer_info(
        inst->pixels(), inst->pixel_size(),
        py::format_descriptor<TPixel>::format(), 2,
        {static_cast<size_t>(inst->get_height()),
         static_cast<size_t>(inst->get_width())},
        {// Strides (in bytes) in each dimension
         static_cast<size_t>(sizeof(TPixel) * inst->get_width()),
         static_cast<size_t>(sizeof(TPixel))});
  }

  static constexpr bool enabled = cpl::core::is_image_pix<TPixel>::value;
};
}  // namespace

void
bind_image(py::module& m)
{
  /**
        @brief Minimal C++ bindings required for all cpl::core::Image
   functionality to be usable from python, without niceties applied

        This is a thinner wrapper over the C++ cpl::core::Image than what we'd
        like, so it's a private class, that is wrapped by a more ergonomic
        python class cpl.core.Image.

        This class is also 1-dimensional. converting to a 2d representation
        requires a helper class for mutable row access, again easier done in
   python than C++
   */
  py::class_<cpl::core::ImageBase, std::shared_ptr<cpl::core::ImageBase>>
      image_class(m, "Image", py::buffer_protocol());

  py::enum_<cpl_norm>(
      image_class,
      "Normalise")  // Unsure if we need all aliases? Giraffe only sets CLI
      .value("SCALE", CPL_NORM_SCALE)
      .value("MEAN", CPL_NORM_MEAN)
      .value("FLUX", CPL_NORM_FLUX)
      .value("ABSFLUX", CPL_NORM_ABSFLUX);


  py::class_<ImageRowAccessor> image_row(m, "ImageRow");
  image_row.doc() = R"docstring(
        Returned from a Image's __getitem__ method or iterator. Used to access specific rows of the image.

        Not instantiatable on its own.
    )docstring";

  image_class.doc() = R"docstring(

    A cpl.core.Image is a 2-dimensional data structure with a pixel type and an optional bad pixel map.

    The pixel indexing follows 0-indexing with the lower left corner having index (0, 0). The pixel
    buffer is stored row-wise so for optimum performance any pixel-wise access should be done likewise.

    Functionality include: FITS I/O Image arithmetic, casting, extraction, thresholding, filtering,
    resampling Bad pixel handling Image statistics Generation of test images Special functions, such as
    the image quality estimator.

    Supported cpl.core.Types:

    - cpl.core.Type.INT (32-bit integer)
    - cpl.core.Type.FLOAT
    - cpl.core.Type.DOUBLE
    - cpl.core.FLOAT_COMPLEX
    - cpl.core.DOUBLE_COMPLEX

    Parameters
    ----------
    data : iterable
      A 1d or 2d iterable containing image data to copy from, and either infers or in the case of a numpy array
      copies its type. Any iterable should be compatible as long as it implements python's buffer protocol
      with a SINGLE c-type per element, and an appropriate .dtype.
      If a 1d iterable is given, width must also be given to properly split the data into image rows.
    dtype : cpl.core.Type, optional
      Cast all pixels (numbers) in the array to given type to create the image.
      List must be homogenous sized. If not given the type will be extracted directly in the case of a
      numpy array or inferred.
    width : int, optional
      Width of the new image. This will split `data` into `width` sized rows to initialise the rows of
      the new image. Should only be given if `data` is 1d, otherwise a ValueError exception is thrown.

    Raises
    ------
    cpl.core.InvalidTypeError
        dtype is not a supported image type.
    cpl.core.IllegalInputError
        `data` is in an invalid format, or could not be reshaped with `width` widthg
    )docstring";

  image_class
      .def(py::init([](py::iterable data, std::optional<cpl_type> dtype,
                       std::optional<size> width) {
             // Reshape the data according to width, if given
             if (width.has_value()) {
               // Get image type via value given, or inference
               py::array input_arr;
               try {
                 input_arr = (py::array)data;  // Convert arg to numpy array
               }
               catch (const py::cast_error& /* exception */) {
                 throw cpl::core::IllegalInputError(
                     PYCPL_ERROR_LOCATION,
                     std::string("expected numpy compatible array, not ") +
                         data.get_type().attr("__name__").cast<std::string>());
               }
               // Cast to buffer to get info
               py::buffer buf;
               try {
                 buf = input_arr.cast<py::buffer>();
               }
               catch (const py::cast_error& /* exception */) {
                 throw cpl::core::IllegalInputError(
                     PYCPL_ERROR_LOCATION,
                     std::string("expected numpy array, or implementor of "
                                 "cpython buffer protocol, not ") +
                         data.get_type().attr("__name__").cast<std::string>());
               }

               py::buffer_info info = buf.request();

               // Check number of dimensions is correct
               if (info.ndim != 1) {
                 std::ostringstream err_msg;
                 err_msg << "expected 1-dimensional buffer, not "
                         << info.shape.size() << "-dimensional buffer";
                 throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                                    err_msg.str());
               }
               // Get height for reshaping
               ssize_t height = input_arr.size() / width.value();
               try {
                 // Set new data reshaped to the required width
                 input_arr.resize(py::array::ShapeContainer(
                     {(ssize_t)width.value(), height}));
                 data = input_arr;
               }
               catch (const py::type_error& /* exception */) {
                 std::ostringstream err_msg;
                 err_msg << "Could not reshape data of length "
                         << input_arr.size() << " into a " << width.value()
                         << "x" << height << " image";
                 throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                                    err_msg.str());
               }
             }

             if (!dtype.has_value()) {
               return image_from_arr(data);
             } else {
               // CPL Type given to cast
               return image_from_python_matrix(dtype.value(), data);
             }
           }),
           py::arg("data"), py::arg("dtype").none(true) = py::none(),
           py::arg("width").none(true) = py::none())
      .def_static(
          "zeros",
          [](size width, size height,
             cpl_type dtype) -> std::shared_ptr<cpl::core::ImageBase> {
            return cpl::core::ImageBase::make_image(width, height, dtype);
          },
          py::arg("width"), py::arg("height"), py::arg("dtype"), R"docstring(
      Create an image of width × height dimensions, all 0’s, as type dtype

      Parameters
      ----------
      width : int
          width of the new image
      height : int
          height of the new image
      dtype : cpl.core.Type
          Type of the new image (see supported cpl.core.Types in class summary)

      Returns
      -------
      cpl.core.Image
          New width x height image of dtype initialised with all 0’s

      Raises
      ------
      cpl.core.InvalidTypeError
          dtype is not a supported image type.
      )docstring")
      .def_static(
          "zeros_like",
          [](const cpl::core::ImageBase& other)
              -> std::shared_ptr<cpl::core::ImageBase> {
            return cpl::core::ImageBase::make_image(
                other.get_width(), other.get_height(), other.get_type());
          },
          py::arg("other"),
          R"docstring(
      Create an image filled with 0's with width, height and type matching another image.

      Parameters
      ----------
      other : cpl.core.Image
          Other Image with the desired width, height and data type.

      Returns
      -------
      cpl.core.Image
          New Image initialised with all 0’s
      )docstring")

      .def_static("load", &cpl::core::load_fits_image, py::arg("filename"),
                  py::arg("dtype") = CPL_TYPE_DOUBLE, py::arg("extension") = 0,
                  py::arg("plane") = 0, py::arg("area") = py::none(),
                  R"docstring(
                    Load an image from a file.

                    Load image data from the extension `extension` of the FITS
                    file `filename`. The FITS extenstion may be an image
                    (``NAXIS`` = 2) or a data cube (``NAXIS`` = 3). By default
                    the image is read from the primary HDU of the FITS file.

                    When the specified extension is a data cube, the slice of
                    the data cube to load may be given by `plane`. By default
                    the first plane is loaded.

                    By default the whole image is loaded. If a particular
                    region of an image should be loaded, the region to load
                    may be provided by the argument `area`.

                    The argument `dtype` specifies the pixel data type of the
                    result image. When the image is loaded the pixel data type
                    in the input FITS file is converted into `dtype`. By default
                    the image data of the input file is converted to
                    cpl.core.Type.DOUBLE. To load the image without converting
                    the pixel data type use cpl.core.Type.UNSPECIFIED.

                    Valid pixel data types which may be used for `dtype` are:

                    - cpl.core.Type.INT (32-bit integer)
                    - cpl.core.Type.FLOAT
                    - cpl.core.Type.DOUBLE

                    Parameters
                    ----------
                    filename : str
                      Name of the input file
                    dtype : cpl.core.Type, default=cpl.core.Type.DOUBLE
                      Data type of the pixels of the returned image
                    extension : int, default=0
                      Index of the FITS extension to load (the primary data unit
                      has index 0)
                    plane : int, default=0
                      Index of the plane of a data cube to load (counting
                      starts from 0)
                    area : Tuple, default=None
                      Region of interest to load given as a tuple specifying
                      the lower left x, the lower left y, the upper right x (inclusive)
                      and the upper right y coordinate (inclusive) in this order.
                      Numbering of the pixel x and y positions starts at 0
                      (pycpl convention). If `None` then the entire image will be loaded.


                    Returns
                    -------
                    cpl.core.Image
                      New image instance of loaded data

                    Raises
                    ------
                    cpl.core.FileIOError
                      If the file cannot be opened, or does not exist.
                    cpl.core.BadFileFormatError
                      If the data cannot be loaded from the file.
                    cpl.core.InvalidTypeError
                      If the requested pixel data type is not supported.
                    cpl.core.IllegalInputError
                      If the requested extension number is invalid (negative),
                      the plane number is out of range, or if the given image region
                      is invalid.
                    cpl.core.DataNotFoundError
                      If the specified extension has no image data.
                  )docstring")
      .def_static(
          "labelise_create",
          [](py::object mask)
              -> std::pair<std::shared_ptr<cpl::core::ImageBase>, int> {
            if (py::hasattr(mask, "_mask")) {
              return cpl::core::labelise_mask(
                  mask.attr("_mask").cast<cpl::core::Mask&>());
            } else {
              throw cpl::core::IllegalInputError(
                  PYCPL_ERROR_LOCATION, "Must provide mask to labellize");
            }
          },
          py::arg("mask"), R"pydoc(
        Labelise a mask to differentiate different objects

        Separate the given mask into contiguous regions, labelling each region
        a different number. 0 Doesn't count as a region.

        Labelises all blobs: 4-neighbor connected zones set to 1 in this mask
        will end up in the image as zones where all pixels are the same, unique
        integer.

        Parameters
        ----------
        mask : cpl.core.Mask
            mask to labelise

        Returns
        -------
        tuple(cpl.core.Image, int)
            The image making up the labelled regions, and the amount of regions.

        Notes
        -----
        Non-recursive flood-fill is applied to label the zones, dimensioned by the
        number of lines in the image, and the maximal number of lines possibly
        covered by a blob.
      )pydoc")
      .def_static("from_accepted", &cpl::core::image_from_accepted,
                  py::arg("image_list"),
                  R"docstring(
        Create a contribution map from the bad pixel maps of the images.

        The returned map counts for each pixel the number of good pixels in the list.

        Parameters
        ----------
        imlist : cpl.core.ImageList
            Images to generate a contribution map from.

        Returns
        -------
        cpl.core.Image
            Output contribution map with the pixel type cpl.core.Type.INT

        Raises
        ------
        cpl.core.IllegalInputError
            If the input image list is not valid
    )docstring")
      .def_static(
          "create_gaussian",
          [](size width, size height, cpl_type dtype, double xcen, double ycen,
             double norm, double sig_x,
             double sig_y) -> std::shared_ptr<cpl::core::ImageBase> {
            auto image = cpl::core::ImageBase::make_image(width, height, dtype);
            (*image).fill_gaussian(xcen, ycen, norm, sig_x, sig_y);
            return image;
          },
          py::arg("width"), py::arg("height"), py::arg("dtype"),
          py::arg("xcen"), py::arg("ycen"), py::arg("norm"), py::arg("sig_x"),
          py::arg("sig_y"),
          R"docstring(
        Generate an image from a 2d gaussian function.

        This function generates an image of a 2d gaussian. The gaussian is
        defined by the position of its centre, given in pixel coordinates inside
        the image with the FITS convention (x from 0 to nx-1, y from 0 to ny-1), its
        norm and the value of sigma in x and y.

        .. math::
              f(x, y) = (norm/(2*pi*sig_x*sig_y)) * exp(-(x-xcen)^2/(2*sig_x^2)) * exp(-(y-ycen)^2/(2*sig_y^2))

        Parameters
        ----------
        width : int
            width of the new image
        height : int
            height of the new image
        dtype : cpl.core.Type
            type of the new image, must be cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE
        xcen : int, float
            x position of the center
        ycen : int, float
            y position of the center
        norm : int, float
            norm of the gaussian.
        sig_x : int, float
            sigma in x for the gaussian distribution.
        sig_y : int, float
            sigma in y for the gaussian distribution.

        Returns
        -------
        cpl.core.Image
            New Image containing the gaussian function.

        Raises
        ------
        cpl.core.InvalidTypeError
            If `self` is not of a supported image type.

        See Also
        --------
        cpl.core.Image.create_gaussian_like : Generate an image from a 2d gaussian function with width, height and type matching another image.
        )docstring")
      .def_static(
          "create_gaussian_like",
          [](const cpl::core::ImageBase& other, double xcen, double ycen,
             double norm, double sig_x,
             double sig_y) -> std::shared_ptr<cpl::core::ImageBase> {
            auto image = cpl::core::ImageBase::make_image(
                other.get_width(), other.get_height(), other.get_type());
            (*image).fill_gaussian(xcen, ycen, norm, sig_x, sig_y);
            return image;
          },
          py::arg("other"), py::arg("xcen"), py::arg("ycen"), py::arg("norm"),
          py::arg("sig_x"), py::arg("sig_y"),
          R"docstring(
        Generate an image from a 2d gaussian function with width, height and type matching another image.

        This function generates an image of a 2d gaussian. The gaussian is
        defined by the position of its centre, given in pixel coordinates inside
        the image with the FITS convention (x from 0 to nx-1, y from 0 to ny-1), its
        norm and the value of sigma in x and y.

        .. math::
              f(x, y) = (norm/(2*pi*sig_x*sig_y)) * exp(-(x-xcen)^2/(2*sig_x^2)) * exp(-(y-ycen)^2/(2*sig_y^2))

        Parameters
        ----------
        other : cpl.core.Image
            Other Image with the desired width, height and data type.
        xcen : int, float
            x position of the center
        ycen : int, float
            y position of the center
        norm : int, float
            norm of the gaussian.
        sig_x : int, float
            sigma in x for the gaussian distribution.
        sig_y : int, float
            sigma in y for the gaussian distribution.

        Returns
        -------
        cpl.core.Image
            New Image containing the gaussian function.

        See Also
        --------
        cpl.core.Image.create_gaussian :  Generate an image from a 2d gaussian function.
        )docstring")
      .def_static(
          "create_jacobian",
          [](const cpl::core::ImageBase& deltax,
             const cpl::core::ImageBase& deltay)
              -> std::shared_ptr<cpl::core::ImageBase> {
            // Need to match dimensions of deltax, deltay
            auto image = cpl::core::ImageBase::make_image(
                deltax.get_width(), deltax.get_height(), CPL_TYPE_DOUBLE);
            (*image).fill_jacobian(deltax, deltay);
            return image;
          },
          py::arg("deltax"), py::arg("deltay"),
          R"mydelim(
        Compute area change ratio for a transformation map.

        Parameters
        ----------
        deltax : cpl.core.Image
            x shift of each pixel
        deltay : cpl.core.Image
            y shift of each pixel

        Returns
        -------
        cpl.core.Image
            New Image containing the are change ratios

        Notes
        -----
        The shifts Images deltax and deltay, describing the transformation, must be of type cpl.core.Type.DOUBLE. For each pixel (u, v) of the
        output image, the deltax and deltay code the following transformation:

        u - deltax(u,v) = x
        v - deltay(u,v) = y

        This function writes the density of the (u, v) coordinate system relative to the (x, y) coordinates for each (u, v) pixel of image out.

        This is trivially obtained by computing the absolute value of the determinant of the Jacobian of the transformation for each pixel of
        the (u, v) image self.

        The partial derivatives are estimated at the position (u, v) in the following way:

            dx/du = 1 + 1/2 ( deltax(u-1, v) - deltax(u+1, v) )
            dx/dv =     1/2 ( deltax(u, v-1) - deltax(u, v+1) )
            dy/du =     1/2 ( deltay(u-1, v) - deltay(u+1, v) )
            dy/dv = 1 + 1/2 ( deltay(u, v-1) - deltay(u, v+1) )

        Typically this function would be used to determine a flux-conservation factor map for the target image specified in function warp().

        The map produced by this function is not applicable for flux conservation in case the transformation implies severe undersampling of the original signal.

        Raises
        ----------
        cpl.core.IllegalInputError
            if the shift Images are not 2 dimensional
        cpl.core.InvalidTypeError
            if the shift Images are not cpl.core.Type.DOUBLE type.
        )mydelim")
      .def_static(
          "create_jacobian_polynomial",
          [](size width, size height, cpl_type dtype,
             const cpl::core::Polynomial& poly_x,
             const cpl::core::Polynomial& poly_y)
              -> std::shared_ptr<cpl::core::ImageBase> {
            auto image = cpl::core::ImageBase::make_image(width, height, dtype);
            (*image).fill_jacobian_polynomial(poly_x, poly_y);
            return image;
          },
          py::arg("width"), py::arg("height"), py::arg("dtype"),
          py::arg("poly_x"), py::arg("poly_y"),
          R"mydelim(
        Compute area change ratio for a 2D polynomial transformation.

        Parameters
        ----------
        width : int
            width of the new image
        height : int
            height of the new image
        dtype : cpl.core.Type
            type of the new image, must be `cpl.core.Type.FLOAT` or `cpl.core.Type.DOUBLE`    
        poly_x : cpl.core.Polynomial
            defines source x-pos corresponding to destination (u,v).
        poly_y : cpl.core.Polynomial
            defines source y-pos corresponding to destination (u,v).

        Returns
        -------
        cpl.core.Image
            New Image containing the computed area change ratios.

        Notes
        -----
        For an image with pixel coordinates (x, y) which is mapped into an output image with pixel coordinates (u, v), and the
        polynomial inverse transformation (u, v) to (x, y) as in warp_polynomial(), this function writes the density of the (u, v)
        coordinate system relative to the (x, y) coordinates for each (u, v) pixel of the output image.

        This is trivially obtained by computing the absolute value of the determinant of the Jacobian of the transformation for each
        pixel of the (u, v) self.

        Typically this function would be used to determine a flux-conservation factor map for the target image specified in function warp_polynomial().

        The map produced by this function is not applicable for flux conservation in case the transformation implies severe undersampling of the original signal.

        Raises
        ----------
        cpl.core.IllegalInputError
          if the polynomial dimensions are not 2
        cpl.core.InvalidTypeError
          if the image type is not supported

        See Also
        --------
        cpl.core.Image.create_jacobian_polynomial_like : Compute area change ratio for a 2D polynomial transformation with width, height and type matching another image.
        )mydelim")
      .def_static(
          "create_jacobian_polynomial_like",
          [](const cpl::core::ImageBase& other,
             const cpl::core::Polynomial& poly_x,
             const cpl::core::Polynomial& poly_y)
              -> std::shared_ptr<cpl::core::ImageBase> {
            auto image = cpl::core::ImageBase::make_image(
                other.get_width(), other.get_height(), other.get_type());
            (*image).fill_jacobian_polynomial(poly_x, poly_y);
            return image;
          },
          py::arg("other"), py::arg("poly_x"), py::arg("poly_y"),
          R"mydelim(
        Compute area change ratio for a 2D polynomial transformation with width, height and type matching another image.

        Parameters
        ----------
        other : cpl.core.Image
            other Image with the desired width, height and data type. The type of `other` must be `cpl.core.Type.FLOAT` or `cpl.core.Type.DOUBLE` 
        poly_x : cpl.core.Polynomial
            defines source x-pos corresponding to destination (u,v).
        poly_y : cpl.core.Polynomial
            defines source y-pos corresponding to destination (u,v).

        Returns
        -------
        cpl.core.Image
            New Image containing the computed area change ratios.

        Notes
        -----
        For an image with pixel coordinates (x, y) which is mapped into an output image with pixel coordinates (u, v), and the
        polynomial inverse transformation (u, v) to (x, y) as in warp_polynomial(), this function writes the density of the (u, v)
        coordinate system relative to the (x, y) coordinates for each (u, v) pixel of the output image.

        This is trivially obtained by computing the absolute value of the determinant of the Jacobian of the transformation for each
        pixel of the (u, v) self.

        Typically this function would be used to determine a flux-conservation factor map for the target image specified in function warp_polynomial().

        The map produced by this function is not applicable for flux conservation in case the transformation implies severe undersampling of the original signal.

        Raises
        ----------
        cpl.core.IllegalInputError
          if the polynomial dimensions are not 2
        cpl.core.InvalidTypeError
          if the image type is not supported

        See Also
        --------
        cpl.core.image.create_jacobian_polynomial : Compute area change ratio for a 2D polynomial transformation.
        )mydelim")
      .def_static(
          "create_noise_uniform",
          [](size width, size height, cpl_type type, double min_pix,
             double max_pix) -> std::shared_ptr<cpl::core::ImageBase> {
            auto image = cpl::core::ImageBase::make_image(width, height, type);
            (*image).fill_noise_uniform(min_pix, max_pix);
            return image;
          },
          py::arg("width"), py::arg("height"), py::arg("type"),
          py::arg("min_pix"), py::arg("max_pix"),
          R"docstring(
        Create an image with uniform random noise distribution.

        Pixel values are within the provided bounds.

        Parameters
        ----------
        width : int
            width of the new image
        height : int
            height of the new image
        dtype : cpl.core.Type
            type of the new image, must be `cpl.core.Type.INT`, `cpl.core.Type.FLOAT` or `cpl.core.Type.DOUBLE`   
        min_pix : float
            minimum output pixel value.
        max_pix : float
            maximum output pixel value.

        Returns
        -------
        cpl.core.Image
            New image containing random noise.

        Raises
        ------
        cpl.core.IllegalInputError
            If `min_pix` > `max_pix`
        cpl.core.InvalidTypeError
            If the image is not of a supported image type.

        See Also
        --------
        cpl.core.Image.create_noise_uniform_like : Create an image with uniform random noise distribution with width, height and type matching another image.
        )docstring")
      .def_static(
          "create_noise_uniform_like",
          [](const cpl::core::ImageBase& other, double min_pix,
             double max_pix) -> std::shared_ptr<cpl::core::ImageBase> {
            auto image = cpl::core::ImageBase::make_image(
                other.get_width(), other.get_height(), other.get_type());
            (*image).fill_noise_uniform(min_pix, max_pix);
            return image;
          },
          py::arg("other"), py::arg("min_pix"), py::arg("max_pix"),
          R"docstring(
        Create an image with uniform random noise distribution with width, height and type matching another image.

        Pixel values are within the provided bounds.

        Parameters
        ----------
        other : cpl.core.Image
            other Image with the desired width, height and data type. The type of `other` must be
            `cpl.core.Type.INT`, `cpl.core.Type.FLOAT` or `cpl.core.Type.DOUBLE`   
        min_pix : float
            minimum output pixel value.
        max_pix : float
            maximum output pixel value.

        Returns
        -------
        cpl.core.Image
            New image containing random noise.

        Raises
        ------
        cpl.core.IllegalInputError
            If `min_pix` > `max_pix`
        cpl.core.InvalidTypeError
            If the image is not of a supported image type.

        See Also
        --------
        cpl.core.Image.create_noise_uniform : Create an image with uniform random noise distribution.
        )docstring")
      .def_static(
          "hypot",
          [](const cpl::core::ImageBase& first,
             const cpl::core::ImageBase& second)
              -> std::shared_ptr<cpl::core::ImageBase> {
            auto out = cpl::core::ImageBase::make_image(
                first.get_width(), first.get_height(), first.get_type());
            (*out).hypot(first, second);
            return out;
          },
          py::arg("first"), py::arg("second"),
          R"docstring(
        The pixel-wise Euclidean distance between two images.

        The Euclidean distance function is useful for gaussian error propagation
        on addition/subtraction operations.

        For pixel values a and b the Euclidean distance c is defined as:
        :math:'c = \sqrt{a^2 + b^2}'

        If both input operands are of type cpl.core.Type.FLOAT the distance is computed
        in single precision, otherwise in double precision.

        Parameters
        ----------
        first : cpl.core.Image
            First input image. Must be type `cpl.core.Type.CPL_TYPE_FLOAT` or
            `cpl.core.Type.CPL_TYPE_DOUBLE`.
        second : cpl.core.Image
            Second input image. Must be type `cpl.core.Type.CPL_TYPE_FLOAT` or
            `cpl.core.Type.CPL_TYPE_DOUBLE`.

        Returns
        -------
        cpl.core.Image
          A new Image containing the Euclidean distance between `first` and `second`.

        Raises
        ------
        cpl.core.IncompatibleInputError
            if the images have different sizes        
        cpl.core.InvalidType
            if the images are not both CPL_TYPE_FLOAT or CPL_TYPE_DOUBLE type.
        )docstring")
      .def_property_readonly("type", &cpl::core::ImageBase::get_type,
                             "cpl.core.Type : Pixel type of the image")
      .def_property_readonly("width", &cpl::core::ImageBase::get_width,
                             "int : width of the image")
      .def_property_readonly("height", &cpl::core::ImageBase::get_height,
                             "int : height of the image")
      .def_property_readonly(
          "size", &cpl::core::ImageBase::get_size,
          "int : Total number of pixels in the image (width*height)")
      .def_property_readonly(
          "shape",
          [](cpl::core::ImageBase& self) -> py::tuple {
            return py::make_tuple(self.get_height(), self.get_width());
          },
          "tuple(int, int) : tuple detailing the shape of the image in the "
          "format (height, width)")
      .def("__bytes__",
           [](std::shared_ptr<cpl::core::ImageBase>& self, size index,
              size length) -> py::bytes {
             size data_size = self->get_size() * self->pixel_size();
             const char* data_ptr = reinterpret_cast<const char*>(self->data());

             if (length < 0 || index < 0 || index + length > data_size) {
               throw std::out_of_range(
                   "get_bytes index or size is larger than this image, or is "
                   "negative");
             }

             // Avoids copy to std::string. Not sure if this makes a noticable
             // performance difference.
             return py::bytes(&data_ptr[index], length);
           })
      .def(
          "get_pixels",
          [](std::shared_ptr<cpl::core::ImageBase>& self, size index,
             size length) -> std::vector<std::optional<generic_pixel>> {
            /*
            Pixels must convert every individual pixel
            */
            size pixel_count = self->get_size();

            if (length < 0 || index < 0 || index + length > pixel_count) {
              throw cpl::core::AccessOutOfRangeError(
                  PYCPL_ERROR_LOCATION,
                  "get_pixels index or size is larger than this image, or is "
                  "negative");
            }
            std::vector<std::optional<generic_pixel>> out_pixs;
            out_pixs.reserve(length);

            int width = self->get_width();
            for (int i = index; i < index + length; ++i) {
              int x = i % width;
              int y = (int)(i / width);

              out_pixs.push_back(self->get_either(y, x));
            }
            return out_pixs;
          },
          py::arg("index"), py::arg("length"),
          R"docstring(
        Get a list of pixel data from the image from a given index along the image.

        Indices are in reference to a 1D representation of the image starting from 0. 
        When converting from 2D coordinates this is equal to (row*width+column)

        Parameters
        ----------
        index : int
            Zero-based index along the Image data to begin extracting pixel data. When converting from 2D coordinates this is equal to (row*width+column)
        length : int
            Number of values to extract starting from `index`

        Returns
        -------
        list
            `length` number of values in the image starting from pixel `index`

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            If the range specified using `index` and `length` is outside of the image.
    )docstring")
      .def(
          "set_pixels",
          [](std::shared_ptr<cpl::core::ImageBase>& self,
             std::vector<std::optional<generic_pixel>> pixels,
             size index) -> void {
            size self_image_count = self->get_size();

            size_t input_pixels = pixels.size();

            if ((index < 0) || (input_pixels + index > self_image_count)) {
      // If this error is in place, the AccessOutOfRangeError is not
      // raised. Commented out to be consistent with get_pixels
#if 0
              PyErr_SetString(PyExc_IndexError,
                              "get_bytes index or size is larger than this "
                              "mask, or is negative.");
#endif
              throw cpl::core::AccessOutOfRangeError(
                  PYCPL_ERROR_LOCATION,
                  "set_pixels data would run the masks' end, or is placed "
                  "before its beginning (negative index).");
            }
            int width = self->get_width();

            for (int ipix = 0; ipix < input_pixels; ipix++) {
              int idx = index + ipix;
              if (idx < self_image_count) {
                // This is the inverse of the idx = row*width + column
                // calculation Both have to include the width - this is not a
                // typo. Some (zero-indexed) PyCPL code to check this is:
                // width=2
                // height=6
                // #idx = row*width + column
                // for dy in range(0,height):
                //     for dx in range(0,width):
                //         idx = dy*width + dx
                //         print (idx,dx,dy)
                //
                // print ("Now go the other way")
                // for idx in range(0,width*height):
                //     x = idx % width
                //     y = int(idx / width )
                //     print (idx,x,y)
                int x = idx % width;
                int y = (int)(idx / width);

                if (pixels[ipix].has_value()) {
                  self->set_either(y, x, std::variant(*pixels[ipix]));
                } else {
                  self->reject(y, x);
                }
              }
            }
          },
          py::arg("pixels"), py::arg("index"),
          R"docstring(
        Set a list of pixel data from the image from a given index along the image.

        Indices are in reference to a 1D representation of the image starting from 0. When converting from 2D coordinates this is equal to (row*width+column)

        Some input `pixels` can be set to `None` to set as bad instead of setting a value. This will be reflected in the corresponding location
        in the bad pixel map.

        Parameters
        ----------
        pixels : int
            `length` number of values to set in the image starting from pixel `index`

        index : int
            Zero-based index along the Image data to begin setting pixel data. When converting from 2D coordinates this is equal to (row*width+column)

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            If the range specified using `index` and `length` is outside of the image.
    )docstring")
      .def("get_pixel", &cpl::core::ImageBase::get_either, py::arg("y"),
           py::arg("x"),
           R"docstring(
        Get a pixel at the specified coordinates.

        This is equivalent to getting the pixel via image index using im[y][x]

        Parameters
        ----------
        y : int
            Row to extract value,  0 being the BOTTOMmost row of the image
        x : int
            Column to extract value. 0 being the leftmost column of image

        Returns
        -------
        None, float, or complex
            Value at the specified index.
            - `None` if the value is invalid.
            - float if the image if the image is of a numerical type (cpl.core.Type.INT, cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE).
            - complex if the image is of a complex type (cpl.core.Type.FLOAT_COMPLEX or cpl.core.Type.DOUBLE_COMPLEX)

        Raises
        ------
        cpl.core.IllegalInputError
            Coordinates are invalid
    )docstring")
      .def_property(
          "bpm",
          [](py::object self) -> py::object {
            // ImageBase.get_bpm won't keep the ImageBase (or the python object)
            // alive. but we want to be safer with python. So to do that, we set
            // the m_on_destruct to hold a py::object
            cpl::core::Mask borrowed_mask =
                self.cast<cpl::core::ImageBase*>()->get_bpm();
            borrowed_mask.m_on_destruct = [self](cpl::core::Mask&) -> void {};
            return py::module::import("cpl.core")
                .attr("Mask")
                .operator()<py::return_value_policy::move>(borrowed_mask);
          },
          [](std::shared_ptr<cpl::core::ImageBase> self,
             py::object new_bpm) -> std::optional<cpl::core::Mask> {
            if (new_bpm.is_none()) {
              return self->unset_bpm();
            } else if (py::hasattr(new_bpm, "_mask")) {
              // Mask 2D Wrapper
              return self->set_bpm(
                  new_bpm.attr("_mask").cast<cpl::core::Mask&>());
            } else {
              return self->set_bpm(new_bpm.cast<cpl::core::Mask&>());
            }
          },
          py::return_value_policy::move,
          "cpl.core.Mask : Bad Pixel Mask of this image to mark locations of "
          "bad pixels often used during filtering")
      .def_buffer([](cpl::core::ImageBase* self) -> py::buffer_info {
        return cpl::core::run_func_for_type<
            cpl::core::Image, buffer_info_getter, py::buffer_info>(
            self->get_type(), self);
      })
      .def("set_pixel", &cpl::core::ImageBase::set_either, py::arg("y"),
           py::arg("x"), py::arg("value"),
           R"docstring(
        Set a pixel at the specified coordinates.

        This is equivalent to setting the pixel via image index using im[y][x] = value

        Parameters
        ----------
        y : int
            Row to extract value, 0 being the BOTTOMmost row of the image
        x : int
            Column to extract value. 0 being the leftmost column of image
        value : int, float, complex
            Value to set. Must be compatible with the image type (int, float for numerical, complex for complex)

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            if the passed position is not within the image.
        cpl.core.InvalidTypeError
            If the type `value` is not compatible with the image type of `self`
    )docstring")
      .def("conjugate", &cpl::core::ImageBase::conjugate,
           R"docstring(
        Complex conjugate the pixels in a complex image. Modified in place.

        Raises
        ------
        cpl.core.InvalidTypeError
            If the image is not of a complex type
    )docstring")
      .def("split_real_imag", &cpl::core::ImageBase::fill_re_im,
           R"docstring(
        Split this complex image into its real and/or imaginary part(s)

        Any bad pixels are also processed.

        The split creates new image instances and will not modify the original image

        Returns
        -------
        tuple(cpl.core.Image, cpl.core.Image)
            Real and Imaginary parts of the image in the format (real, imaginary).
            Images will be of type `cpl.core.Type.DOUBLE` if `self` is of type `cpl.core.Type.DOUBLE_COMPLEX`.
            Likewise images will be of type `cpl.core.Type.FLOAT` if `self` is of type `cpl.core.Type.FLOAT_COMPLEX`.

        Raises
        ------
        cpl.core.InvalidTypeError
            If the image is not of a complex type

        Notes
        -----
        This corresponds to the `cpl_image_fill_re_im` function in the CPL C API
    )docstring")
      .def("split_abs_arg", &cpl::core::ImageBase::fill_abs_arg,
           R"docstring(
        Split this complex image into its absolute and argument part(s)

        Any bad pixels are also processed.

        The split creates new image instances and will not modify the original image

        Returns
        -------
        tuple(cpl.core.Image, cpl.core.Image)
            absolute and argument parts of the image in the format (absolute, argument)

        Raises
        ------
        cpl.core.InvalidTypeError
            If the image is not of a complex type

        Notes
        -----
        This corresponds to the `cpl_image_fill_abs_arg` function in the CPL C API
    )docstring")
      .def("fill_rejected", &cpl::core::ImageBase::fill_rejected,
           py::arg("value"),
           R"docstring(
        Set the bad pixels in an image to a fixed value.

        Images can be of type cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE

        Parameters
        ----------
        value : int, float
            Value to replace bad pixels

        Raises
        ------
        cpl.core.InvalidTypeError
            If the image's type is not supported, i.e. `cpl.core.Type.FLOAT_COMPLEX` or `cpl.core.Type.DOUBLE_COMPLEX`
    )docstring")
      .def("fill_window", &cpl::core::ImageBase::fill_window, py::arg("window"),
           py::arg("value"),
           R"docstring(
        Fill an image window with a constant

        Any bad pixels in the window are accepted.

        Upper boundaries are inclusive and will also be filled with `value`.

        Images can be of type cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE

        Parameters
        ----------
        window : tuple(int,int,int,int)
          Window to fill with `value` in the format (llx, lly, urx, ury) where:
          - `llx` Lower left X coordinate
          - `lly` Lower left Y coordinate
          - `urx` Upper right X coordinate (inclusive)
          - `ury` Upper right Y coordinate (inclusive)
        value : float
            Value to fill with

        Raises
        ------
        cpl.core.IllegalInputError
            The specified window is not valid
    )docstring")
      .def("save", &cpl::core::ImageBase::save, py::arg("filename"),
           py::arg("pl"), py::arg("mode"),
           py::arg("dtype") = CPL_TYPE_UNSPECIFIED,
           R"docstring(
        Save an image to a FITS file

        This function saves an image to a FITS file. If a property list is provided, it is written to the header where the image is written.

        Supported image types are cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT.

        The type used in the file can be one of: cpl.core.Type.UCHAR (8 bit unsigned), cpl.core.Type.SHORT (16 bit signed), cpl.core.Type.USHORT
        (16 bit unsigned), cpl.core.Type.INT (32 bit signed), cpl.core.Type.FLOAT (32 bit floating point), or cpl.core.Type.DOUBLE (64 bit floating point).
        By default the saved type is cpl.core.Type.UNSPECIFIED. This value means that the type used for saving is the pixel type
        of the input image. Using the image pixel type as saving type ensures that the saving incurs no loss of information.

        Supported output modes are cpl.core.io.CREATE (create a new file) and cpl.core.io.EXTEND (append a new extension to an existing file)

        Note that in append mode the file must be writable (and do not take for granted that a file is writable just because it was created by the same
        application, as this depends from the system umask).

        The output mode cpl.core.io.EXTEND can be combined (via bit-wise OR) with an option for tile-compression. This compression is lossless.
        The options are: cpl.core.io.COMPRESS_GZIP, cpl.core.io.COMPRESS_RICE, cpl.core.io.COMPRESS_HCOMPRESS, cpl.core.io.COMPRESS_PLIO.

        Upon success the image will reside in a FITS data unit with NAXIS = 2. Is it possible to save a single image in a FITS data unit with NAXIS = 3

        Parameters
        ----------
        filename : str
            Name of the file to write
        pl : cpl.core.PropertyList, optional
            Property list for the output header. None by default.
        mode : unsigned int
            Desired output options, determined by bit-wise OR of cpl.core.io enums
        dtype : cpl.core.Type, optional
            The type used to represent the data in the file. By default it saves using the image's current dtype

        Raises
        ------
        cpl.core.IllegalInputError
            if the type or the mode is not supported
        cpl.core.InvalidTypeError
            if the passed pixel type is not supported
        cpl.core.FileNotCreatedError
            If the output file cannot be created
        cpl.core.FileIOError
            if the data cannot be written to the file

        See Also
        --------
        cpl.core.ImageList.save : for saving imagelists to a fits file
        )docstring")
      .def("duplicate", &cpl::core::ImageBase::duplicate,
           R"docstring(
        Copy the image.

        Copy the image into a new image object. The pixels and the bad pixel map are also copied.

        This method is also used when performing a deepcopy on an image.

        Returns
        -------
        cpl.core.Image
            New image object that is a copy of the original image.
    )docstring")
      .def("cast", &cpl::core::ImageBase::cast, py::arg("dtype"),
           R"docstring(
        Returns a copy of the image converted to a given data type.

        Casting to non-complex types is only supported for non-complex types.

        Parameters
        ----------
        dtype : cpl.core.Type
            The destination type

        Returns
        -------
        cpl.core.Image
            New image that is a copy of the original image converted to the given `dtype`

        Raises
        ------
        cpl.core.IllegalInputError
            If the passed type is invalid
        cpl.core.TypeMismatchError
            If the image type is complex and requested casting type is non-complex.

        See Also
        --------
        cpl.core.Image.to_type : Converts an image to a given type. Modified in place.
    )docstring")
      .def(
          "as_array",
          [image_class](cpl::core::ImageBase& self) {
            return image_class.attr("__array__")(self);
          },
          R"docstring(
        Returns a copy of the Image as a numpy array.

        Returns
        -------
        numpy.ndarray
            New numpy array containing the pixel values from the Image. The data type
            of the array will be the same as the data type of the Image.  
    )docstring")
      .def("to_type", &cpl::core::ImageBase::cast, py::arg("dtype"),
           R"docstring(
        Convert an image to a given type. Modified in place.

        Casting to non-complex types is only supported for non-complex types.

        Parameters
        ----------
        dtype : cpl.core.Type
            The destination type

        Returns
        -------
        cpl.core.Image
            New image that is a copy of the original image converted to the given `dtype`

        Raises
        ------
        cpl.core.IllegalInputError
            If the passed type is invalid
        cpl.core.TypeMismatchError
            If the image type is complex and requested casting type is non-complex.

        See Also
        --------
        cpl.core.Image.cast : Get the minimum pixel value over the entire image
    )docstring")
      .def_property_readonly(
          "is_complex", &cpl::core::ImageBase::is_complex,
          "bool : True if the image is of type cpl.core.Type.FLOAT_COMPLEX or "
          "cpl.core.Type.DOUBLE_COMPLEX")
      .def("__repr__", &cpl::core::ImageBase::dump_structure)
      .def("__str__",
           [](cpl::core::ImageBase& self) -> std::string {
             return self.dump(cpl::core::Window::All);
           })
      .def("add", &cpl::core::ImageBase::add, py::arg("other"), R"pydoc(
        Adds values from Image other to self. Modified in place.


        The bad pixel map of the `self` becomes the union of the bad pixel
        maps of the input images.

        Parameters
        ----------
        other : cpl.core.Image
            Image to add to `self`

        Raises
        ------
        cpl.core.IncompatibleInputError
            if the input images have different sizes
        cpl.core.TypeMismatchError
            if the `other` has complex type
        )pydoc")
      .def("subtract", &cpl::core::ImageBase::subtract, py::arg("other"),
           R"pydoc(
        Subtract image values from `self`

        Parameters
        ----------
        other : cpl.core.Image
            Image to subtract from `self`

        Raises
        ------
        cpl.core.IncompatibleInputError
            if the input images have different sizes
        cpl.core.TypeMismatchError
            if the `other` has complex type
        )pydoc")
      .def("multiply", &cpl::core::ImageBase::multiply, py::arg("other"),
           R"pydoc(
        Multiply `self` by another image

        Parameters
        ----------
        other : cpl.core.Image
            Image to multiply with `self`

        Raises
        ------
        cpl.core.IncompatibleInputError
            if the input images have different sizes
        cpl.core.TypeMismatchError
            if the `other` has complex type
        )pydoc")
      .def("divide", &cpl::core::ImageBase::divide, py::arg("other"), R"pydoc(
        Divide `self` by another image

        Parameters
        ----------
        other : cpl.core.Image
            image to divide with

        Raises
        ------
        cpl.core.IncompatibleInputError
            if the input images have different sizes
        cpl.core.TypeMismatchError
            if the second input image has complex type

        Notes
        -----
        The result of division with a zero-valued pixel is marked as a bad pixel.
        )pydoc")
      .def("add_scalar", &cpl::core::ImageBase::add_scalar, py::arg("scalar"),
           R"pydoc(
        Elementwise addition of a scalar to an image. Modified in place.

        Modifies the image by adding a number to each of its pixels.

        The operation is always performed in double precision, with a final
        cast of the result to the image pixel type.

        Parameters
        ----------
        scalar : float
            Number to add
        )pydoc")
      .def("subtract_scalar", &cpl::core::ImageBase::subtract_scalar,
           py::arg("scalar"), R"pydoc(
        Elementwise subtraction of a scalar from an image. Modified in place.

        Parameters
        ----------
        scalar : float
            Number to subtract
        )pydoc")
      .def("multiply_scalar", &cpl::core::ImageBase::multiply_scalar,
           py::arg("scalar"), R"pydoc(
        Elementwise multiplication of an image with a scalar. Modified in place.

        Parameters
        ----------
        scalar : float
            Number to multiply with
        )pydoc")
      .def("divide_scalar", &cpl::core::ImageBase::divide_scalar,
           py::arg("scalar"), R"pydoc(
        Elementwise division of an image with a scalar

        Modifies the image by dividing each of its pixels with a number.

        Parameters
        ----------
        scalar : float
            Non-zero number to divide with

        Raises
        ------
        cpl.core.DivsionByZeroError
            scalar is 0.0
        )pydoc")
      .def("power", &cpl::core::ImageBase::power, py::arg("exponent"), R"pydoc(
        Compute the elementwise power of the image.

        Modifies the image by lifting each of its pixels to exponent.

        Images can be of type cpl.core.Type.INT, cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE.

        Pixels for which the power to the given exponent is not defined are
        rejected and set to zero.

        Parameters
        ----------
        exponent : float
            Scalar exponent.
        )pydoc")
      .def("exponential", &cpl::core::ImageBase::exponential, py::arg("base"),
           R"pydoc(
      Compute the elementwise exponential of the image.

      Modifies the image by computing the base-scalar exponential of each of its
      pixels.

      Images can be of type cpl.core.Type.INT, cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE.

      Pixels for which the power of the given base is not defined are
      rejected and set to zero.

      Parameters
      ----------
      base : float
          Base of the exponential.
      )pydoc")
      .def("logarithm", &cpl::core::ImageBase::logarithm, py::arg("base"),
           R"pydoc(
        Compute the elementwise logarithm of the image. Modified in place.

        Modifies the image by computing the base-scalar logarithm of each of its
        pixels.

        Images can be of type cpl.core.Type.INT, cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE.

        Pixels for which the logarithm is not defined are
        rejected and set to zero.

        Parameters
        ----------
        base : float
            Base of the logarithm.

        Raises
        ------
        cpl.core.InvalidTypeError
            if the image type is not supported
        cpl.core.IllegalInputError
            if base is non-positive
        )pydoc")
      .def("normalise", &cpl::core::ImageBase::normalise, py::arg("mode"),
           R"pydoc(
        Normalise pixels in an image. Modified in place.

        Normalises an image according to a given criterion.

        Possible normalisations are:
        - cpl.core.Image.Normalise.SCALE sets the pixel interval to [0,1].
        - cpl.core.Image.Normalise.MEAN sets the mean value to 1.
        - cpl.core.Image.Normalise.FLUX sets the flux to 1.
        - cpl.core.Image.Normalise.ABSFLUX sets the absolute flux to 1.

        Parameters
        ----------
        mode : cpl.core.Image.Normalise
            Normalisation mode.
        )pydoc")
      .def("abs", &cpl::core::ImageBase::abs, R"pydoc(
        Take the absolute value of an image. Modified in place.

        Set each pixel to its absolute value.
        )pydoc")
      .def("and_with", &cpl::core::ImageBase::and_with, py::arg("other"),
           R"pydoc(
        Takes the bit-wise AND of the image with another image, pixel by pixel.

        Both images must be integer type. The AND is done in place, overwriting the
        original image.

        Parameters
        ----------
        other : cpl.core.Image
            Second operand

        Raises
        ------
        cpl.core.IncompatibleInputError
            if the images have different sizes
        cpl.core.InvalidTypeError
            If either image type is not cpl.core.Type.INT
        )pydoc")
      .def("or_with", &cpl::core::ImageBase::or_with, py::arg("other"), R"pydoc(
        Takes the bit-wise OR of the image with another image, pixel by pixel.

        Both images must be integer type. The OR is done in place, overwriting the
        original image.

        Parameters
        ----------
        other : cpl.core.Image
            Second operand

        Raises
        ------
        cpl.core.IncompatibleInputError
            if the images have different sizes
        cpl.core.InvalidTypeError
            If either image type is not cpl.core.Type.INT
        )pydoc")
      .def("xor_with", &cpl::core::ImageBase::xor_with, py::arg("other"),
           R"pydoc(
        Takes the bit-wise XOR of the image with another image, pixel by pixel.

        Both images must be integer type. The XOR is done in place, overwriting the
        original image.

        Parameters
        ----------
        other : cpl.core.Image
            Second operand

        Raises
        ------
        cpl.core.IncompatibleInputError
            if the images have different sizes
        cpl.core.InvalidTypeError
            If either image type is not cpl.core.Type.INT
        )pydoc")
      .def("negate", &cpl::core::ImageBase::negate, R"pydoc(
        Takes the bit-wise complement (NOT) of the image, pixel by pixel.

        The image must be integer type. The NOT is doen in place, overwriting the original image.

        Raises
        ------
        cpl.core.InvalidTypeError
            If the image's type is not `cpl.core.Type.INT`
        )pydoc")
      .def("and_scalar", &cpl::core::ImageBase::and_scalar, py::arg("value"),
           R"pydoc(
        The bit-wise AND of a scalar and an image with integer pixels. Modified in place.

        Parameters
        ----------
        value : int
            scalar value to bitwise AND with the pixels

        Notes
        -----
        cpl.core.Type.INT is required
        )pydoc")
      .def("or_scalar", &cpl::core::ImageBase::or_scalar, py::arg("value"),
           R"pydoc(
        The bit-wise OR of a scalar and an image with integer pixels. Modified in place.

        Parameters
        ----------
        value : int
            scalar value to bit-wise OR with the pixels

        Notes
        -----
        cpl.core.Type.INT is required
        )pydoc")
      .def("xor_scalar", &cpl::core::ImageBase::xor_scalar, py::arg("value"),
           R"pydoc(
        The bit-wise XOR of a scalar and an image with integer pixels. Modified in place.

        Parameters
        ----------
        value : int
            scalar value to bit-wise XOR with the pixels

        Notes
        -----
        cpl.core.Type.INT is required
        )pydoc")
      .def("extract", &cpl::core::ImageBase::extract, py::arg("window"),
           R"pydoc(
        Extract a rectangular zone from an image into another image.

        The input coordinates define the extracted region by giving the coordinates
        of the lower left and upper right corners (inclusive).

        Coordinates must be provided in the FITS convention: lower left
        corner of the image is at (1,1), x increasing from left to right,
        y increasing from bottom to top.
        Images can be of type cpl.core.Type.INT, cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE.

        If the input image has a bad pixel map and if the extracted rectangle has
        bad pixel(s), then the extracted image will have a bad pixel map, otherwise
        it will not.

        Parameters
        ----------
        window : tuple(int,int,int,int)
          Window in the format (llx, lly, urx, ury) where:
          - `llx` Lower left X coordinate
          - `lly` Lower left Y coordinate
          - `urx` Upper right X coordinate
          - `ury` Upper right Y coordinate

        Returns
        -------
        cpl.core.Image
            New image instance of the extracted area.

        Raises
        ------
        cpl.core.IllegalInputError
            if the window coordinates are not valid
        )pydoc")
      .def("turn", &cpl::core::ImageBase::rotate, py::arg("rot"), R"pydoc(
        Rotate an image by a multiple of 90 degrees clockwise.

        Images can be of type cpl.core.Type.INT, cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE.

        The definition of the rotation relies on the PyCPL convention: The lower left corner of the image is at (0,0), x increasing from left to right, y increasing from bottom to top.

        For rotations of +90 or -90 degrees on rectangular non-1D-images, the pixel buffer is temporarily duplicated.

        rot may be any integer value, its modulo 4 determines the rotation:

            -3 to turn 270 degrees counterclockwise.
            -2 to turn 180 degrees counterclockwise.
            -1 to turn 90 degrees counterclockwise.
            0 to not turn
            +1 to turn 90 degrees clockwise (same as -3)
            +2 to turn 180 degrees clockwise (same as -2).
            +3 to turn 270 degrees clockwise (same as -1).

        Parameters
        ----------
        rot : int
            Number of clockwise rotations. -1 is a rotation of 90 deg counterclockwise.

        Raises
        ------
        cpl.core.InvalidTypeError
            if the image type is not supported e
        )pydoc")
      .def("shift", &cpl::core::ImageBase::shift, py::arg("dy"), py::arg("dx"),
           R"pydoc(
        Shift an image by integer offsets

        The new zones (in the result image) where no new value is computed are set
        to 0 and flagged as bad pixels.
        The shift values have to be valid:
        -nx < dx < nx and -ny < dy < ny

        Parameters
        ----------
        dy : int
            The shift in Y
        dx : int
            The shift in X

        Raises
        ------
        cpl.core.IllegalInputError
            if the requested shift is bigger than the
        )pydoc")
      .def("copy_into", &cpl::core::ImageBase::copy_into, py::arg("other"),
           py::arg("ypos"), py::arg("xpos"), R"pydoc(
        Copy one image into another

        (ypos, xpos) must be a valid position in `self`. If `other` is bigger than the place
        left in `self`, the part that falls outside of `self` is simply ignored, an no
        error is raised.
        The bad pixels are inherited from `other` in the concerned `self` zone.

        The two input images must be of the same type, namely one of
        cpl.core.Type.INT, cpl.core.Type.FLOAT, cpl.core.Type.DOUBLE.

        Parameters
        ----------
        other : cpl.core.Image
            the inserted image
        ypos : int
            the y pixel position in `self` where the lower left pixel of
            `other` should go (from 0 to the y-1 size of `self`)
        xpos : int
            the x pixel position in `self` where the lower left pixel of
            `other` should go (from 0 to the x-1 size of `self`)

        Raises
        ------
        cpl.core.TypeMismatchError
            if the input images are of different types
        cpl.core.InvalidTypeError
            if the image type is not supported
        cpl.core.AccessOutOfRangeError
            if xpos or ypos are outside the specified range

        Notes
        -----
        The two pixel buffers may not overlap
        )pydoc")
      .def("flip", &cpl::core::ImageBase::flip, py::arg("angle"), R"pydoc(
        Flip an image on a given mirror line.

        This function operates locally on the pixel buffer.

        angle can take one of the following values:
        - 0 (theta=0) to flip the image around the horizontal
        - 1 (theta=pi/4) to flip the image around y=x
        - 2 (theta=pi/2) to flip the image around the vertical
        - 3 (theta=3pi/4) to flip the image around y=-x

        Images can be of type cpl.core.Type.INT, cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        angle : int
            mirror line in polar coord. is theta = (PI/4) * angle

        Raises
        ------
        cpl.core.IllegalInputError
            if the angle is different from the allowed values
        cpl.core.InvalidTypeError
            if the image type is not supported
        )pydoc")
      .def("move", &cpl::core::ImageBase::move, py::arg("nb_cut"),
           py::arg("new_pos"), R"pydoc(
        Permute tiles in an image

        nb_cut^2 defines in how many tiles the images will be permuted. Each tile will
        then be moved to another place defined in new_pos. nb_cut equal 1 will leave
        the image unchanged, 2 is used to permute the four image quadrants, etc..
        new_pos contains nb_cut^2 values between 1 and nb_cut^2, i.e. a permutation
        of the values from 1 to nb_cut^2.
        The zone positions are counted from the lower left part of the image.
        It is not allowed to move two tiles to the same position (the relation
        between th new tiles positions and the initial position is bijective !).
        The array with the permuted positions must contain nb_cut^2 values, the
        function is unable to verify this.

        The image x and y sizes have to be multiples of nb_cut.

        16   17   18           6    5    4
        13   14   15           3    2    1

        10   11   12   ---->  12   11   10
        7    8    9           9    8    7

        4    5    6          18   17   16
        1    2    3          15   14   13

        image 3x6            image.move(3, new_pos);
        with new_pos = [9,8,7,6,5,4,3,2,1];

        The bad pixels are moved in the same way.

        Parameters
        ----------
        nb_cut : int
            The number of cuts in x and y
        new_pos : list of ints
            Array with the nb_cut^2 permuted positions

        Raises
        ------
        cpl.core.IllegalInputError
            if `nb_cut` is not strictly positive or cannot divide one of the image sizes or
            if the new_pos array specifies to move two tiles to the same position.
        cpl.core.InvalidTypeError
            if the image type is not supported

        Notes
        -----
        The permutation array _must_ contain `nb_cut`-squared elements
        )pydoc")
      .def(
          "get_fwhm",
          [](cpl::core::ImageBase& self, size ypos, size xpos) -> py::tuple {
            std::pair<double, double> fwhm_res = self.get_fwhm(ypos, xpos);
            return py::make_tuple(
                fwhm_res.first == -1 ? py::none() : py::cast(fwhm_res.first),
                fwhm_res.second == -1 ? py::none() : py::cast(fwhm_res.second));
          },
          py::arg("ypos"), py::arg("xpos"), R"pydoc(
        Compute the FWHM of an object in a cpl.core.Vector

        For the FWHM in x (resp. y) to be computed, the image size in the x (resp.
        y) direction should be at least of 5 pixels.

        If for any reason, one of the FHWMs cannot be computed, its returned value
        is None with no exception raised. For example, if a 4 column image is passed,
        the x component of the return tuple would be None, while the y component
        would be correctly computed, and no exception would be raised.

        Parameters
        ----------
        ypos : int
            the y position of the object (0 for the first pixel)
        xpos : int
            the x position of the object (0 for the first pixel)
        Returns
        -------
        tuple(float, float)
            fwhm y, x, which are the computed FWHM in y or x directions

        Raises
        ------
        cpl.core.DataNotFoundError
            if (`ypos`, `xpos`) specifies a rejected pixel or a pixel with a non-positive value
        cpl.core.AccessOutOfRangeError
            if (`ypos` or `xpos`) is outside the image size range

        Notes
        -----
        The return value may be None with no error condition

        This function uses a basic method: start from the center of the object
        and go away until the half maximum value is reached in x and y.
        )pydoc")
      .def("image_quality_est", &cpl::core::ImageBase::iqe, py::arg("window"),
           R"mydelim(
    Compute an image quality estimation for an object

    Parameters
    ----------
    window: tuple(int, int, int, int)
        The zone window in the format (x1, y1, x2, y2)

    Returns
    -------
    cpl.core.BiVector
      The IQE result, which contains in the first vector (x) the computed values, and in the second
      one (y), the associated errors.

      The computed values are:
      - x position of the object
      - y position of the object
      - FWHM along the major axis
      - FWHM along the minor axis
      - the angle of the major axis with the horizontal in degrees
      - the peak value of the object
      - the background computed

    Raises
    ------
    cpl.core.IllegalInputError
      if the input zone is not valid or if the computation fails on the zone
    cpl.core.InvalidTypeError
      if the input image has the wrong type

    Notes
    -----
    This function makes internal use of the iqe() MIDAS function (called
    here cpl_iqe()) written by P. Grosbol. Refer to the MIDAS documentation
    for more details. This function has proven to give good results over
    the years when called from RTD. The goal is to provide the exact same
    functionality in CPL as the one provided in RTD. The code is simply
    copied from the MIDAS package, it is not maintained by the CPL team.

    The bad pixels map of the image is not taken into account.
    The input image must be of type float.
    )mydelim")
      .def(
          "warp_polynomial",
          [](cpl::core::ImageBase& self, const cpl::core::Polynomial poly_y,
             const cpl::core::Polynomial poly_x,
             const cpl::core::Vector yprofile, double yradius,
             const cpl::core::Vector xprofile, double xradius,
             std::optional<std::tuple<size, size>> out_dim,
             std::optional<cpl_type> out_type)
              -> std::shared_ptr<cpl::core::ImageBase> {
            return self.warp_polynomial(
                poly_x, poly_y, xprofile, xradius, yprofile, yradius,
                out_dim.value_or(std::make_tuple((size)self.get_width(),
                                                 (size)self.get_height())),
                out_type.value_or(self.get_type()));
          },
          py::arg("poly_y"), py::arg("poly_x"), py::arg("yprofile"),
          py::arg("yradius"), py::arg("xprofile"), py::arg("xradius"),
          py::arg("out_dim").none(true) = py::none(),
          py::arg("out_type").none(true) = py::none(),
          R"mydelim(
    Warp an image according to a 2D polynomial transformation.

    Parameters
    ----------
    poly_y : cpl.dfs.Polynomial
        Polynomial defining source y-pos corresponding to destination (u,v).
    poly_x :  cpl.dfs.Polynomial
        Polynomial defining source x-pos corresponding to destination (u,v).
    yprofile :  cpl.dfs.Vector
        Interpolation weight vector as a function of the distance in Y
    yradius : float
        Positive inclusion radius in the Y-dimension
    xprofile : cpl.dfs.Vector
        Interpolation weight as a function of the distance in X
    xradius : float
        Positive inclusion radius in the X-dimension
    out_dim : (size, size)
        output dimensions. If not given then will default to the same dimensions of
        self
    out_type : cpl.core.Type
        Output type. If not given then will default to the same type of self. Will
        cause errors if output type is not compatible with input

    Returns
    -------
    cpl.core.Image
        New warped image

    Notes
    -----
    'out' and 'in'  may have different dimensions and types.

    The pair of 2D polynomials are used internally like this:

    .. code-block:: python

      x = poly_x.eval(cpl.core.Vector([u,v]))
      y = poly_y.eval(cpl.core.Vector([u,v]))

    where (u,v) are (integer) pixel positions in the destination image and (x,y)
    are the corresponding pixel positions (typically non-integer) in the source
    image.

    The identity transform (poly_x(u,v) = u, poly_y(u,v) = v) would thus
    overwrite the 'out' image with the 'in' image, starting from the lower left
    if the two images are of different sizes.

    Beware that extreme transformations may lead to blank images.

    The input image type can be cpl.core.Type.INT, cpl.core.Type.FLOAT and cpl.core.Type.DOUBLE.

    In case a correction for flux conservation were required, please create
    a correction map using the function `cpl.core.Image.create_jacobian_polynomial()`.

    Raises
    ------
    cpl.core.IllegalInputError
      if the polynomial dimensions are not 2
    cpl.core.InvalidTypeError
      if the output image type is incompatible with the input image
    )mydelim")
      .def("warp", &cpl::core::ImageBase::warp, py::arg("deltay"),
           py::arg("deltax"), py::arg("yprofile"), py::arg("yradius"),
           py::arg("xprofile"), py::arg("xradius"),
           R"mydelim(
        Generate a warped version of this image

        Parameters
        ----------
        deltax : int
            The x shift of each pixel, must be same size as `deltay` and type `cplcore.Type.DOUBLE`
        deltay : int
            The y shift of each pixel, must be same size as `deltax` and type `cplcore.Type.DOUBLE`
        xprofile : cpl.core.Vector
            Interpolation weight as a function of the distance in Y
        xradius : float
            Positive inclusion radius in the X-dimension
        yprofile : cpl.core.Vector
            Interpolation weight as a function of the distance in Y
        yradius : float
            Positive inclusion radius in the Y-dimension
        xprofile : cpl.core.Vector
            Interpolation weight as a function of the in X
        xradius: float
            Positive inclusion radius in the X-dimension

        Returns
        -------
        cpl.core.Image
            new warped image, same size as `deltax` and `deltay` and same type as self

        Raises
        ----------
        cpl.core.IllegalInputError
           if the input images sizes are incompatible or if the delta images are not of type cplcore.Type.DOUBLE
        cpl.core.InvalidTypeError
           if the image type is not supported

        See Also
        --------
        cpl.core.Image.create_jacobian : Compute area change ratio for a transformation map.

        Notes
        -----
        The pixel value at the (integer) position (u, v) in the destination image is interpolated
        from the (typically non-integer) pixel position (x, y) in the source image, where:

        x = u - deltax(u, v),
        y = v - deltay(u, v).

        The identity transform is thus given by `deltax` and `deltay` filled with zeros.

        `deltax` and `deltay` may be a different size than self, but must be the same size
        as each other. 

        self may be of the type cplcore.Type.INT, cplcore.Type.FLOAT or cplcore.Type.DOUBLE

        If case a correction for flux conservation is required please create a correction map using
        the function `cpl.core.Image.create_jacobian()`.
        )mydelim")
      .def(
          "fft",
          [](std::shared_ptr<cpl::core::ImageBase> self,
             std::optional<std::shared_ptr<cpl::core::ImageBase>> imag,
             bool inverse, bool unnormalized,
             bool swapHalves) -> std::shared_ptr<cpl::core::ImageBase> {
            if (imag.has_value() &&
                (self->get_type() == CPL_TYPE_DOUBLE_COMPLEX)) {
              py::object warn =
                  py::module::import("warnings");  // Only import when necessary
              warn(
                  "`self` has a complex data type and thus `imag` will not be "
                  "used");
            }
            unsigned mode =
                CPL_FFT_DEFAULT;  // CPL_FFT_DEFAULT is defined as 0 to add
                                  // options onto. Will leave here anyway
                                  // incase this ever changes.
            if (inverse) {
              mode |= CPL_FFT_INVERSE;
            }
            if (unnormalized) {
              mode |= CPL_FFT_UNNORMALIZED;
            }
            if (swapHalves) {
              mode |= CPL_FFT_SWAP_HALVES;
            }
            return self->fft(imag, mode);
          },
          py::arg("imag").none(true) = py::none(), py::arg("inverse") = false,
          py::arg("unnormalized") = false, py::arg("swap_halves") = false,
          R"docstring(
        Fast Fourier Transform a square, power-of-two sized image. Modified in place.

        `self` must be either of type cpl.core.Type.DOUBLE_COMPLEX or cpl.core.Type.DOUBLE.
        If `self` is passed as cpl.core.Type.DOUBLE, the imaginary component can be passed via `imag`, which must also be
        cpl.core.Type.DOUBLE. `imag` is unused otherwise.

        Any rejected pixel is used as if it were a good pixel.

        The image must be square with a size that is a power of two.

        Different FFT options can be set via the kwargs (see Parameters).

        Parameters
        ----------
        imag : cpl.core.Image, optional
            The imaginary part of the image. Only used when the image's type is cpl.core.Type.DOUBLE
            If not given, a 0 value image will be set in its place.
        inverse : bool, optional
            True to perform Inverse FFT transform
        unnormalized : bool, optional
            True to not normalize (with N*N for N-by-N image) on `inverse` = False. Has no effect on forward transform (`inverse` = True).
        swap_halves : bool, optional
            Swap the four quadrants of the result image.

        Raises
        ------
        cpl.core.IllegalInputError
            if the image is not square or if the image size is not a power of 2.
        cpl.core.UnsupportedModeError
            if mode is otherwise different from the allowed FFT options.
        cpl.core.InvalidTypeError
            if the passed image type is not supported.

        Warning
        -------
        When comparing to cpl.drs.fft.fft_image and numpy's fft, with any image
        dimensions greater than or equal to 4x4 the values of the imaginary
        component of the resulting image is sign flipped. This is due to the differing implementation of
        the fft algorithm which is being looked into.

        If possible is recommended to use cpl.drs.fft.fft_image as its a more up to date
        and well more maintained implementation of fft using fftw.

    )docstring")
      .def("extract_subsample", &cpl::core::ImageBase::extract_subsample,
           py::arg("xstep"), py::arg("ystep"), R"pydoc(
        Sub-sample an image

        step represents the sampling step in x and y: both steps = 2 will create an
        image with a quarter of the pixels of the input image.

        image type can be cpl.core.Type.INT, cpl.core.Type.FLOAT and cpl.core.Type.DOUBLE.
        If the image has bad pixels, they will be resampled in the same way.

        The flux of the sampled pixels will be preserved, while the flux of the
        pixels not sampled will be lost. Using steps = 2 in each direction on a
        uniform image will thus create an image with a quarter of the flux.

        Parameters
        ----------
        ystep : int
            Take every xstep pixel in y
        xstep : int
            Take every ystep pixel in x

        Returns
        -------
        cpl.core.Image
            New sub-sampled image

        Raises
        ------
        cpl.core.IllegalInputError
            if xstep, ystep are not positive
        cpl.core.InvalidTypeError
            if the image type is not supported
        )pydoc")
      .def("rebin", &cpl::core::ImageBase::rebin, py::arg("ystart"),
           py::arg("xstart"), py::arg("ystep"), py::arg("xstep"), R"pydoc(
        Rebin an image

        If both bin sizes in x and y are = 2, an image with (about) a quarter
        of the pixels of the input image will be created. Each new pixel
        will be the sum of the values of all contributing input pixels.
        If a bin is incomplete (i.e., the input image size is not a multiple
        of the bin sizes), it is not computed.

        xstep and ystep must not be greater than the sizes of the rebinned
        region.

        The input image type can be cpl.core.Type.INT, cpl.core.Type.FLOAT and cpl.core.Type.DOUBLE.
        If the image has bad pixels, they will be propagated to the rebinned
        image "pessimistically", i.e., if at least one of the contributing
        input pixels is bad, then the corresponding output pixel will also
        be flagged "bad". If you need an image of "weights" for each rebinned
        pixel, just cast the input image bpm into a cpl.core.Type.INT image, and
        apply cpl.core.Image.rebin() to it too.

        Parameters
        ----------
        ystart : int
            start y position of binning (starting from 0...)
        xstart : int
            start x position of binning (starting from 0...)
        ystep : int
            Bin size in y.
        xstep : int
            Bin size in x.

        Returns
        -------
        cpl.core.Image
            New rebinned image

        Raises
        ------
        cpl.core.IllegalInputError
            if xstep, ystep, xstart, ystart are not positive
        cpl.core.InvalidTypeError
            if the image type is not supported
        )pydoc")
      .def("get_interpolated", &cpl::core::ImageBase::get_interpolated,
           py::arg("ypos"), py::arg("xpos"), py::arg("yprofile"),
           py::arg("yradius"), py::arg("xprofile"), py::arg("xradius"),
           R"mydelim(
    Interpolate a pixel

    Parameters
    ----------
    ypos : int
      Pixel y floating-point position (FITS convention)
    xpos : int
      Pixel x floating-point position (FITS convention)
    yprofile : cpl.core.Vector
      Interpolation weight vector as a function of the distance in Y
    yradius : float
      Positive inclusion radius in the Y-dimension
    xprofile :cpl.core.Vector
      Interpolation weight as a function of the distance in X
    xradius : float
      Positive inclusion radius in the X-dimension

    Returns
    -------
    tuple(float, float)
      Tuple of (interpolated, confidence), where `interpolated` represents the
      interpolated pixel value and `confidence` represents the confidence level
      of the interpolated value (range 0 to 1)

    Raises
    ------
    cpl.core.IllegalInputError
      If xradius, xprofile, yprofile and yradius are not as requested
    cpl.core.InvalidTypeError
      If the image type is not supported

    Notes
    -----
    If the X- and Y-radii are identical the area of inclusion is a circle,
    otherwise it is an ellipse, with the larger of the two radii as the
    semimajor axis and the other as the semiminor axis.

    A good profile length is 2001, using radius 2.0.

    The radii are only required to be positive. However, for small radii,
    especially radii less than 1/sqrt(2), (xpos, ypos) may be located such that
    no source pixels are included in the interpolation, causing the interpolated
    pixel value to be undefined.

    The X- and Y-profiles can be generated with
    cpl.core.Vector.fill_kernel_profile(profile, radius).
    For profiles generated with cpl_vector_fill_kernel_profile() it is
    important to use the same radius both there and in
    cpl.core.Image.get_interpolated().

    On error *pconfid* is negative (unless pconfid is NULL).
    Otherwise, if *pconfid* is zero, the interpolated pixel-value is undefined.
    Otherwise, if *pconfid* is less than 1, the area of inclusion is close to the
    image border or contains rejected pixels.

    The input image type can be cpl.core.Type.INT, cpl.core.Type.FLOAT and cpl.core.Type.DOUBLE.
    )mydelim")
      .def("count_rejected", &cpl::core::ImageBase::count_rejected, R"pydoc(
        Count the number of bad pixels declared in an image

        Returns
        -------
        int
            the number of bad pixels

        )pydoc")
      .def("is_rejected", &cpl::core::ImageBase::is_rejected, py::arg("y"),
           py::arg("x"), R"pydoc(
        Test if a pixel is good or bad

        Parameters
        ----------
        y : int
            the y pixel position in the image (first pixel is 0)
        x : int
            the x pixel position in the image (first pixel is 0)

        Returns
        -------
        bool
            True if the pixel is bad, False if the pixel is good

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            if the specified position is outside of image `self`
        )pydoc")
      .def("reject", &cpl::core::ImageBase::reject, py::arg("y"), py::arg("x"),
           R"pydoc(
        Set a pixel as bad in an image

        Parameters
        ----------
        y : int
            the y pixel position in the image (first pixel is 0)
        x : int
            the x pixel position in the image (first pixel is 0)

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            if the specified position is outside of the image
        )pydoc")
      .def(
          "reject_from_mask",
          [](const cpl::core::ImageBase& self, py::object map) -> void {
            if (py::hasattr(map, "_mask")) {
              const_cast<cpl::core::ImageBase&>(self).reject_from_mask(
                  map.attr("_mask").cast<cpl::core::Mask&>());
            } else {
              throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                                 "map must be mask");
            }
          },
          py::arg("map"),
          R"mydelim(
    Set the bad pixels in an image as defined in a mask

    If the input image has a bad pixel map prior to the call, it is overwritten.

    Parameters
    ----------
    map : cpl.core.Mask
        the mask defining the bad pixels

    Raises
    ------
    cpl.core.IncompatibleInputError
      if the image and the map have different sizes
    )mydelim")
      .def(
          "reject_value",
          [](cpl::core::ImageBase& self, py::set values) -> void {
            int mode = 0;
            // Need to iterate through the set of values instead of using
            // .contains() so that we can use std::isnan().
            py::iterator iter = values.begin();
            while (iter != py::iterator::sentinel()) {
              // Get a value from the set by dereferencing the iterator and
              // explicitly casting to double.
              auto value = (*iter).cast<double>();
              // For each value check against the supported special values and
              // make sure the corresponding bits are set.
              if (value == 0.0) {
                mode = mode | CPL_VALUE_ZERO;
              } else if (std::isinf(value)) {
                if (value > 0) {
                  mode = mode | CPL_VALUE_PLUSINF;
                } else {
                  mode = mode | CPL_VALUE_MINUSINF;
                }
              } else if (std::isnan(value)) {
                mode = mode | CPL_VALUE_NAN;
              } else {
                throw cpl::core::UnsupportedModeError(
                    PYCPL_ERROR_LOCATION,
                    "Reject values must be 0, -Inf, +Inf or NaN");
              }
              ++iter;
            }
            self.reject_value(static_cast<cpl_value>(mode));
          },
          py::arg("values"),
          R"pydoc(
        Reject pixels with the specified special value(s)

        Parameters
        ----------
        values: set
          The set of special values that should be marked as rejected pixels.
          The supported special values are 0, math.inf, -math.inf, math.nan
          and their numpy equivalents, and any combination is allowed.

        Raises
        ------
        cpl.core.UnsupportedModeError
          If something other than one of the supported special values is in
          the values parameter.
        cpl.core.InvalidTypeError
          If the image is a complex type.
      )pydoc")
      .def("accept", &cpl::core::ImageBase::accept, py::arg("y"), py::arg("x"),
           R"pydoc(
        Set a pixel as good in an image

        Parameters
        ----------
        y : int
            the y pixel position in the image (first pixel is 0)
        y : int
            the x pixel position in the image (first pixel is 0)

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            if the specified position is outside of image `self`
        )pydoc")
      .def("accept_all", &cpl::core::ImageBase::accept_all,
           "Set all pixels in the image as good")
      .def("get_min", &cpl::core::ImageBase::get_min,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes the minimum pixel value over an entire image or image sub window.
        
        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate (inclusive)
            - `ury` Upper right Y coordinate (inclusive)

        Returns
        -------
        float
            the minimum value

        Notes
        -----
        Does not work on complex images.

        Raises
        ------
        cpl.core.IllegalInputError
            If the specified window is illegal

        See Also
        --------
        cpl.core.Image.get_max : Get the maximum pixel value over an image or image sub window.
        )pydoc")
      .def("get_max", &cpl::core::ImageBase::get_max,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes the maximum pixel value over an entire image or image sub window
        
        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate (inclusive)
            - `ury` Upper right Y coordinate (inclusive)

        Returns
        -------
        float
            the maximum value

        Notes
        -----
        Does not work on complex images.
        
        Raises
        ------
        cpl.core.IllegalInputError
            If the specified window is illegal

        See Also
        --------
        cpl.core.Image.get_min : Get the minimum pixel value over the entire image or image sub window.
        )pydoc")
      .def("get_mean", &cpl::core::ImageBase::get_mean,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes the mean pixel value over an entire image or sub-window.
        
        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate
            - `ury` Upper right Y coordinate

        Returns
        -------
        float
            the mean value

        Notes
        -----
        Does not work on complex images.

        Raises
        ------
        cpl.core.IllegalInputError
            If the specified window is illegal
        )pydoc")
      .def("get_median", &cpl::core::ImageBase::get_median,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes the median pixel value over an entire image or sub-window.
        
        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate
            - `ury` Upper right Y coordinate

        Returns
        -------
        float
            the median value

        Raises
        ------
        cpl.core.IllegalInputError
            If the specified window is illegal

        Notes
        -----
        The median value is calculated using integer arithmetic if the image has integer data type,
        in which case the median value may differ from some other Python libraries such as numpy.
        For integer images the behaviour of get_median is equivalent to `np.floor(np.median(int_image))`. 
        )pydoc")
      .def("get_stdev", &cpl::core::ImageBase::get_stdev,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes the pixel standard deviation over an image or sub window.
        
        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate
            - `ury` Upper right Y coordinate

        Returns
        -------
        float
            the standard deviation value

        Raises
        ------
        cpl.core.IllegalInputError
            If the specified window is illegal
        Notes
        -----
        get_stdev calculates the "sample standard deviation" rather than the "ensemble standard
        deviation", i.e. the divisor in calculations is N - 1, where N is the number of pixels.
        This is equivalent to `np.std(image, ddof=1)` in numpy. 
        )pydoc")
      .def("get_flux", &cpl::core::ImageBase::get_flux,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes the sum of pixel values over an entire image or sub window
        
        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate
            - `ury` Upper right Y coordinate

        Returns
        -------
        float
            the flux (sum of pixels) value

        Raises
        ------
        cpl.core.IllegalInputError
            If the specified window is illegal

        Notes
        -----
        Does not work on complex images.
        )pydoc")
      .def("get_absflux", &cpl::core::ImageBase::get_absflux,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes the sum of absolute values over an entire image or sub window.
        
        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate
            - `ury` Upper right Y coordinate

        Returns
        -------
        float
            the absolute flux (sum of pixels) value

        Raises
        ------
        cpl.core.IllegalInputError
            If the specified window is illegal

        cpl.core.InvalidTypeError
            If the image they are called on has complex data type.
        )pydoc")
      .def("get_sqflux", &cpl::core::ImageBase::get_sqflux,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes the sum of squared values over an entire image or sub-window
        
        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate
            - `ury` Upper right Y coordinate

        Returns
        -------
        float
            the square flux

        Raises
        ------
        cpl.core.IllegalInputError
            If the specified window is illegal

        cpl.core.InvalidTypeError
            If the image they are called on has complex data type.
        )pydoc")
      .def("get_centroid_x", &cpl::core::ImageBase::get_centroid_x,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes the x centroid value over the whole image or sub-window.
        
        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate
            - `ury` Upper right Y coordinate


        Returns
        -------
        float
            the x centroid value

        Raises
        ------
        cpl.core.IllegalInputError
            If the specified window is illegal

        cpl.core.InvalidTypeError
            If the image they are called on has complex data type.

        See Also
        --------
        cpl.core.Image.get_centroid_y : Compute the y centroid value over the whole image or sub-window.
        )pydoc")
      .def("get_centroid_y", &cpl::core::ImageBase::get_centroid_y,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes the y centroid value over the whole image or sub-window.
        
        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate
            - `ury` Upper right Y coordinate


        Returns
        -------
        float
            the y centroid value

        Raises
        ------
        cpl.core.IllegalInputError
            If the specified window is illegal

        cpl.core.InvalidTypeError
            If the image they are called on has complex data type.
        
        See Also
        --------
        cpl.core.Image.get_centroid_x : Compute the x centroid value over the whole image or sub-window.
        )pydoc")
      .def("get_minpos", &cpl::core::ImageBase::get_minpos,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes minimum pixel value position over an image or sub-window.

        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate
            - `ury` Upper right Y coordinate

        Returns
        -------
        tuple(int, int)
            the x coordinate and y coordinate of the minimum position in the format (x,y)

        Raises
        ------
        cpl.core.IllegalInputError
            If the specified window is illegal
        cpl.core.InvalidTypeError
            If `self`'s pixel type is invalid

        See Also
        --------
        cpl.core.Image.get_maxpos : get the position of the maximum value in the image
        )pydoc")
      .def("get_maxpos", &cpl::core::ImageBase::get_maxpos,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes maximum pixel value and position over an image or sub-window.
        
        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate
            - `ury` Upper right Y coordinate

        Returns
        -------
        tuple(int, int)
            the x coordinate and y coordinate of the maximum position in the format (x,y)
        
        Raises
        ------
        cpl.core.IllegalInputError
            If the specified window is illegal
        cpl.core.InvalidTypeError
            If `self`'s pixel type is invalid

        See Also
        --------
        cpl.core.Image.get_minpos : get the position of the minimum value in the image
        )pydoc")
      .def("get_median_dev", &cpl::core::ImageBase::get_median_dev,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes median and mean absolute median deviation on an image or sub-window.

        For each non-bad pixel in the window the absolute deviation from the median is computed.
        The mean absolute median deviation is however still sensitive to outliers.
        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate
            - `ury` Upper right Y coordinate

        Returns
        -------
        tuple(float,float)
            The median of the non-bad pixels and the mean absolute median deviation

        Raises
        ------
        cpl.core.IllegalInputError
            If the specified window is illegal
        cpl.core.DataNotFoundError
            If all pixels in the specified window are bad

        See Also
        --------
        cpl.core.Image.get_mad : for calculating median and median absolute median deviation (MAD) on the image
        cpl.core.Image.get_median: for calculating median on all pixels
        )pydoc")
      .def("get_mad", &cpl::core::ImageBase::get_mad,
           py::arg("window").none(true) = py::none(), R"pydoc(
        Computes median and median absolute deviation (MAD) on an image or sub-window.
        
        Images can be cpl.core.Type.FLOAT, cpl.core.Type.INT or cpl.core.Type.DOUBLE.

        Parameters
        ----------
        window : tuple(int,int,int,int), optional
            Window to operate on in the format (llx, lly, urx, ury) where:
            - `llx` Lower left X coordinate (0 for leftmost)
            - `lly` Lower left Y coordinate (0 for lowest)
            - `urx` Upper right X coordinate
            - `ury` Upper right Y coordinate

        Returns
        -------
        tuple(float,float)
            The median of the non-bad pixels and the median absolute deviation of the good pixels in the format (median, MAD)

        Raises
        ------
        cpl.core.InvalidTypeError
            if the image type is not supported
        cpl.core.DataNotFoundError
            If all pixels in the image are bad
        
        See Also
        --------
        cpl.core.Image.get_median_dev : for calculating median and mean absolute median deviation on an image or sub-window.
        cpl.core.Image.get_median : for calculating median on all pixels or sub-window.
        )pydoc")
      .def(
          "filter_mask",
          [](const cpl::core::ImageBase& self, py::object kernel,
             cpl_filter_mode filter, cpl_border_mode border,
             cpl_type dtype) -> std::shared_ptr<cpl::core::ImageBase> {
            if (py::hasattr(kernel, "_mask")) {
              if (border == CPL_BORDER_NOP) {
                // This implemention can't do CPL_BORDER_NOP.
                throw cpl::core::UnsupportedModeError(
                    PYCPL_ERROR_LOCATION,
                    "cpl.core.Border.NOP border mode is not supported in "
                    "PyCPL");
              }
              if (border == CPL_BORDER_ZERO) {
                // Without a pre-allocated results image CPL_BORDER_NOP is
                // equivalent to CPL_BORDER_ZERO
                border = CPL_BORDER_NOP;
              }
              return const_cast<cpl::core::ImageBase&>(self).filter_mask(
                  kernel.attr("_mask").cast<cpl::core::Mask&>(), filter, border,
                  dtype);
            } else {
              throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                                 "kernel must be mask");
            }
          },
          py::arg("kernel"), py::arg("filter"), py::arg("border"),
          py::arg("dtype"),
          R"mydelim(
        Filter an image using a binary kernel

        Parameters
        ----------
        kernel : cpl.core.Mask
            Mask of Pixels to use
        filter : cpl.core.Filter
            Filter to use, can be:
            cpl.core.Filter.MEDIAN, cpl.core.Filter.AVERAGE and more, see notes
        border :
            border to use, can be cpl.core.Border.FILTER and more, see Notes
        dtype : cpl.core.Type
            Data type to use for the output image. Can be cpl.core.Type.INT, cpl.core.Type.FLOAT
            or cpl.core.Type.DOUBLE but see Notes for restrictions.

        Returns
        -------
        cpl.core.Image
            The filtered image.

        Notes
        -----
        The kernel must have an odd number of rows and an odd number of columns.

        The output image will have equal dimensions to the original image, except
        for the border mode CPL_BORDER_CROP, where the output image must have 
        2 * hx columns fewer and 2 * hy rows fewer than the original image,
        where the kernel has size (1 + 2 * hx, 1 + 2 * hy).

        In standard deviation filtering the kernel must have at least two elements
        set to True, for others at least one element must be set to
        True.

        Supported pixel types are: cpl.core.Type.INT, cpl.core.Type.FLOAT and cpl.core.Type.DOUBLE.

        In median filtering the two images must have the same pixel type.

        In standard deviation filtering a filtered pixel must be computed from at
        least two input pixels, for other filters at least one input pixel must be
        available. Output pixels where this is not the case are set to zero and
        flagged as rejected.

        In-place filtering is not supported.

        Supported modes:

        cpl.core.Filter.MEDIAN:
        cpl.core.Border.FILTER, cpl.core.Border.ZERO, cpl.core.Border.COPY, cpl.core.Border.CROP.

        cpl.core.Filter.AVERAGE:
        cpl.core.Border.FILTER

        cpl.core.Filter.AVERAGE_FAST:
        cpl.core.Border.FILTER

        cpl.core.Filter.STDEV:
        cpl.core.Border.FILTER

        cpl.core.Filter.STDEV_FAST:
        cpl.core.Border.FILTER

        Note that in PyCPL the supported border modes for median filtering includes
        `ZERO` but not `NOP` as in CPL's `cpl_image_filter_mask`. This is because the
        `NOP` mode preserves pixel values from the border regions of a pre-allocated
        results Image, but this method uses a new Image to store the results so there
        are no pre-existing pixel values to preserve. See PIPE-11042 for more details.

        To shift an image 1 pixel up and 1 pixel right with the cpl.core.Filter.MEDIAN
        filter and a 3 by 3 kernel, one should set to CPL_BINARY_1 the bottom
        leftmost kernel element - at row 3, column 1, i.e.

        .. code-block:: python

          kernel=cpl.core.Mask(3,3)
          kernel[0][0] = True

        The kernel required to do a 5 x 5 median filtering is created like this:

        .. code-block:: python

          kernel=~cpl.core.Mask(5,5)

        Raises
        ------
        cpl.core.IllegalInputError
            if the kernel has a side of even length.
        cpl.core.DataNotFoundError
            If the kernel is empty, or in case of cpl.core.Filter.STDEV if the kernel has only one element set to True.
        cpl.core.AccessOutOfRangeError
            If the kernel has a side longer than the input image.
        cpl.core.InvalidTypeError
            if the image type is not supported.
        cpl.core.TypeMismatchError
            if in median filtering the input and output pixel types differ.
        cpl.core.UnsupportedModeError
            If the output pixel buffer overlaps the input one (or the kernel), or the border/filter mode is unsupported.
        )mydelim")
      .def("filter", &cpl::core::ImageBase::filter, py::arg("kernel"),
           py::arg("filter"), py::arg("border") = CPL_BORDER_FILTER,
           py::arg("dtype"),
           R"pydoc(
        Filter the image using a floating-point kernel

        The kernel must have an odd number of rows and an odd number of columns and at least one non-zero element.

        For scaling filters (cpl.core.Filter.LINEAR_SCALE and cpl.core.Filter.MORPHO_SCALE) the flux of the filtered image will be
        scaled with the sum of the weights of the kernel. If for a given input pixel location the kernel covers only bad
        pixels, the filtered pixel value is flagged as bad and set to zero.

        For flux-preserving filters (cpl.core.Filter.LINEAR and cpl.core.Filter.MORPHO) the filtered pixel must have at least one input
        pixel with a non-zero weight available. Output pixels where this is not the case are set to zero and flagged as bad.

        Supported pixel types are: cpl.core.Type.INT, cpl.core.Type.FLOAT and cpl.core.Type.DOUBLE.

        Supported filters: cpl.core.Filter.LINEAR, cpl.core.Filter.MORPHO, cpl.core.Filter.LINEAR_SCALE and cpl.core.Filter.MORPHO_SCALE

        The result is returned in a new Image.

        Parameters
        ----------
        kernel : cpl.core.Matrix
            Pixel weights
        filter : cpl.core.Filter
            cpl.core.Filter.LINEAR or cpl.core.Filter.MORPHO, cpl.core.Filter.LINEAR_SCALE and cpl.core.Filter.MORPHO_SCALE
        border : cpl.core.Border, optional
            Filtering border mode. Currently only supports cpl.core.Border.FILTER and thus is set to that by default
        dtype : cpl.core.Type
            Data type to use for the output image, can be cpl.core.Type.INT, cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE.

        Returns
        -------
        cpl.core.Image
            The filtered image.

        Raises
        ------
        cpl.core.IllegalInputError
            if the kernel has a side of even length.
        cpl.core.DivisionByZeroError
            If the kernel is a zero-matrix.
        cpl.core.AccessOutOfRangeError
            If the kernel has a side longer than the input image.
        cpl.core.InvalidTypeError
            if the image type is not supported.
        cpl.core.TypeMismatchError
            if in median filtering the input and output pixel types differ.
        cpl.core.UnsupportedModeError
            If the output pixel buffer overlaps the input one (or the kernel), or the border/filter mode is unsupported.

        See Also
        --------
        cpl.core.Image.filter_mask : For filtering using a binary kernel (cpl.core.Mask)
        )pydoc")
      .def(
          "dump",
          [](cpl::core::ImageBase& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<cpl::core::Window> window,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(),
                                self.dump(window), show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("window") = py::none(), py::arg("show") = true,
          R"pydoc(
            Dump the image contents to a file, stdout or a string.

            This function is intended just for debugging. It prints the contents of an image
            to the file path specified by `filename`. 
            If a `filename` is not specified, output goes to stdout (unless `show` is False). 
            In both cases the contents are also returned as a string.

            Parameters
            ----------
            filename : str, optional
                File to dump image contents to
            mode : str, optional
                Mode to open the file with. Defaults to "w" (write, overwriting the contents of the file if it already exists),
                but can also be set to "a" (append, creating the file if it does not already exist or appending to the end of
                it if it does).
            window : tuple(int,int,int,int), optional
              Window to dump with `value` in the format (llx, lly, urx, ury) where:
              - `llx` Lower left X coordinate
              - `lly` Lower left Y coordinate
              - `urx` Upper right X coordinate 
              - `ury` Upper right Y coordinate
            show : bool, optional
                Send image contents to stdout. Defaults to True.

            Returns
            -------
            str 
                Multiline string containing the dump of the image contents.

                
          )pydoc")
      .def(
          "threshold",
          [](const cpl::core::ImageBase& self, double assign_lo_cut,
             double assign_hi_cut, std::optional<double> lo_cut,
             std::optional<double> hi_cut) -> void {
            if (!lo_cut.has_value() || !hi_cut.has_value()) {
              switch (self.get_type()) {
                case CPL_TYPE_INT:
                  lo_cut = lo_cut.value_or(INT_MIN);
                  hi_cut = hi_cut.value_or(INT_MAX);
                  break;
                case CPL_TYPE_FLOAT:
                  lo_cut = lo_cut.value_or(FLT_MIN);
                  hi_cut = hi_cut.value_or(FLT_MAX);
                  break;
                case CPL_TYPE_DOUBLE:
                  lo_cut = lo_cut.value_or(DBL_MIN);
                  hi_cut = hi_cut.value_or(DBL_MAX);
                  break;
                default:
                  throw cpl::core::InvalidTypeError(
                      PYCPL_ERROR_LOCATION,
                      "Image is not a numerical type (cpl.core.Type.INT, "
                      "cpl.core.Type.FLOAT, or cpl.core.Type.DOUBLE");
                  break;
              }
            }
            self.threshold(lo_cut.value(), hi_cut.value(), assign_lo_cut,
                           assign_hi_cut);
          },
          py::arg("assign_lo_cut"), py::arg("assign_hi_cut"),
          py::arg("lo_cut").none(true) = py::none(),
          py::arg("hi_cut").none(true) = py::none(),
          R"pydoc(
    Threshold an image to a given interval. Thresholding is performed inplace.

    Pixels outside of the provided interval are assigned the given values.

    By default `lo_cut` and `hi_cut` are set to the minimum and maximum value of the image data type.
    Therefore `assign_lo_cut` will not be applied to any pixels if `lo_cut` is also not set,
    and `assign_hi_cut` will not be applied to any pixels if `hi_cut` is also not set.

    Images can be of type cpl.core.Type.INT, cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE.
    lo_cut must be smaller than or equal to hi_cut.

    Parameters
    ----------
    assign_lo_cut : float
        Value to assign to pixels below low bound.
    assign_hi_cut : float
        Value to assign to pixels above high bound.
    lo_cut : float, optional
        Lower bound.
    hi_cut : float, optional
        Higher bound.

    Raises
    ------
    cpl.core.InvalidTypeError
        if the image type is not supported
      )pydoc")
      .def("equals", &cpl::core::ImageBase::operator==, py::arg("other"),
           R"mydelim(
    Check if one image is equivalent to another.

    Two images are considered equal if they share the same dimensions, type, and values (in the same positions as each other).

    Can also be called using the equality operator i.e. `self`==`other`

    Parameters
    ----------
    - other : cpl.core.Image
        Image to compare to `self`.

    Return
    ------
    bool
        True if `self` is equal to `other`. False otherwise.

    Notes
    -----
    In comparison to numpy array equality, this function is more strict in that the properties of the array (type and dimensions)
    need to be equal, contrary to numpy which does elementwise comparisons and does not require the arrays to be of the same
    data type. Numpy functions are still however compatible with images as input arguments for numpy and thus equality functions
    can be used with the images (e.g. `np.array_equals(im1, im2)`)
    )mydelim")
      .def("__deepcopy__",
           [](cpl::core::ImageBase& self,
              py::dict /* memo */) -> std::shared_ptr<cpl::core::ImageBase> {
             return self.duplicate();
           })
      .def("__eq__", &cpl::core::ImageBase::operator==)
      .def("__contains__",
           [](std::shared_ptr<cpl::core::ImageBase> self,
              py::object iterable_comparable) -> bool {
             py::object builtins = py::module::import("builtins");
             py::object py_iter_builtin = builtins.attr("iter");
             py::object py_next_builtin = builtins.attr("next");

             size height = self->get_height();
             size width = self->get_width();

             for (size y = 0; y < height; ++y) {
               // Compare each row by iterating over the comparable row and
               // image row simultaneously:

               py::object compare_row_with =
                   py_iter_builtin(iterable_comparable);
               bool row_equal = true;

               for (size x = 0; x < width; ++x) {
                 auto compare_with = py_next_builtin(compare_row_with)
                                         .cast<std::optional<generic_pixel>>();

                 if (self->get_either(y, x) != compare_with) {
                   row_equal = false;
                   break;
                 }
               }

               if (row_equal) {
                 return true;
               }
             }
             return false;
           })
      .def("__len__",
           [](std::shared_ptr<cpl::core::ImageBase> self) -> size {
             return self->get_height();
           })
      .def("__getitem__",
           [](py::object self, size index) -> ImageRowAccessor {
             auto self_casted =
                 self.cast<std::shared_ptr<cpl::core::ImageBase>>();

             if (index < 0) {
               index += self_casted->get_height();
             }

             if (index < 0 || index >= self_casted->get_height()) {
               throw py::index_error("image row index out of range");
             }

             return ImageRowAccessor{self, self_casted, index};
           })
      .def("__next__",
           [](py::object self) -> ImageRowAccessor {
             auto self_casted =
                 self.cast<std::shared_ptr<cpl::core::ImageBase>>();
             if (self_casted->iter_idx >= self_casted->get_height()) {
               self_casted->iter_idx = 0;
               throw py::stop_iteration();
             }
             int index = self_casted->iter_idx++;
             return ImageRowAccessor{self, self_casted, index};
           })
      .def("__iter__", [](py::object self) -> py::object { return self; })
      .def(
          "__reversed__",
          [](py::object /* self */) -> void {
            PyErr_SetString(PyExc_RuntimeError,
                            "Reverse operation is unsupported on images");
          },
          "Unsupported")
      .def("vector_from_column", &cpl::core::ImageBase::vector_from_column,
           py::arg("pos"), R"pydoc(
        Extract a column from an image.

        Images can be of type cpl.core.Type.INT, cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE.

        The bad pixels map is not taken into account in this function.

        Parameters
        ----------
        pos : int
            Position of the column (0 for leftmost column)

        Returns
        -------
        cpl.core.Vector
            Vector of values from column `pos` of the image

        Raises
        ------
        cpl.core.IllegalInputError
            `pos` is not valid
        cpl.core.IllegalTypeError
            If the image is a type that is not supported
      )pydoc")
      .def("vector_from_row", &cpl::core::ImageBase::vector_from_row,
           py::arg("pos"), R"pydoc(
        Extract a row from an image.

        Images can be of type cpl.core.Type.INT, cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE.

        The bad pixels map is not taken into account in this function.

        Parameters
        ----------
        pos : int
            Position of the row (0 for bottom row)

        Returns
        -------
        cpl.core.Vector
            Vector of values from row `pos` of the image

        Raises
        ------
        cpl.core.IllegalInputError
            `pos` is not valid
        cpl.core.IllegalTypeError
            If the image is a type that is not supported
      )pydoc")
      // conversion to numpy array via np.array or np.asarray
      .def(
          "__array__",
          [](const cpl::core::ImageBase& self,
             const py::kwargs& /* unused */) -> py::array {
            switch (self.get_type()) {
              case CPL_TYPE_INT: {
                return py::array(py::buffer_info(
                    (int*)(self.data()),
                    sizeof(int),  // itemsize
                    py::format_descriptor<int>::format(),
                    2,  // ndim
                    std::vector<size>{self.get_height(),
                                      self.get_width()},  // shape
                    std::vector<size>{(size)sizeof(int) * self.get_width(),
                                      sizeof(int)}  // strides
                    ));
              }
              case CPL_TYPE_FLOAT: {
                return py::array(py::buffer_info(
                    (float*)(self.data()),
                    sizeof(float),  // itemsize
                    py::format_descriptor<float>::format(),
                    2,  // ndim
                    std::vector<size>{self.get_height(),
                                      self.get_width()},  // shape
                    std::vector<size>{(size)sizeof(float) * self.get_width(),
                                      sizeof(float)}  // strides
                    ));
              }
              case CPL_TYPE_LONG: {
                return py::array(py::buffer_info(
                    (long*)(self.data()),
                    sizeof(long),  // itemsize
                    py::format_descriptor<long>::format(),
                    2,  // ndim
                    std::vector<size>{self.get_height(),
                                      self.get_width()},  // shape
                    std::vector<size>{(size)sizeof(long) * self.get_width(),
                                      sizeof(long)}  // strides
                    ));
              }

              case CPL_TYPE_FLOAT_COMPLEX: {
                float _Complex* data = (float _Complex*)(self.data());

                std::complex<float>* std_data =
                    (std::complex<float>*)cpl_calloc(
                        self.get_width() * self.get_height(),
                        sizeof(std::complex<float>));
                for (int i = 0; i < self.get_width() * self.get_height(); i++) {
                  std_data[i] = cpl::core::complexf_to_cpp(data[i]);
                }
                py::array returnValue(py::buffer_info(
                    std_data,
                    sizeof(std::complex<float>),  // itemsize
                    py::format_descriptor<std::complex<float>>::format(),
                    2,  // ndim
                    std::vector<size>{self.get_height(),
                                      self.get_width()},  // shape
                    std::vector<size>{(size)sizeof(std::complex<float>) *
                                          self.get_width(),
                                      sizeof(std::complex<float>)}  // strides
                    ));
                return returnValue;
              }

              case CPL_TYPE_DOUBLE_COMPLEX: {
                double _Complex* data = (double _Complex*)(self.data());
                std::complex<double>* std_data =
                    (std::complex<double>*)cpl_calloc(
                        self.get_width() * self.get_height(),
                        sizeof(std::complex<double>));
                for (int i = 0; i < self.get_width() * self.get_height(); i++) {
                  std_data[i] = cpl::core::complexd_to_cpp(data[i]);
                }
                py::array returnValue(py::buffer_info(
                    std_data,
                    sizeof(std::complex<double>),  // itemsize
                    py::format_descriptor<std::complex<double>>::format(),
                    2,  // ndim
                    std::vector<size>{self.get_height(),
                                      self.get_width()},  // shape
                    std::vector<size>{(size)sizeof(std::complex<double>) *
                                          self.get_width(),
                                      sizeof(std::complex<double>)}  // strides
                    ));
                return returnValue;
              }
              case CPL_TYPE_LONG_LONG: {
                return py::array(py::buffer_info(
                    (long long*)(self.data()),
                    sizeof(long long),  // itemsize
                    py::format_descriptor<long long>::format(),
                    2,  // ndim
                    std::vector<size>{self.get_height(),
                                      self.get_width()},  // shape
                    std::vector<size>{(size)sizeof(long long) *
                                          self.get_width(),
                                      sizeof(long long)}  // strides
                    ));
                break;
              }

              case CPL_TYPE_DOUBLE: {
                return py::array(py::buffer_info(
                    (double*)(self.data()),
                    sizeof(double),  // itemsize
                    py::format_descriptor<double>::format(),
                    2,  // ndim
                    std::vector<size>{self.get_height(),
                                      self.get_width()},  // shape
                    std::vector<size>{(size)sizeof(double) * self.get_width(),
                                      sizeof(double)}  // strides
                    ));
              }
              default: {
                throw cpl::core::InvalidTypeError(
                    PYCPL_ERROR_LOCATION,
                    "column is of invalid type, cannot be cast to numpy array");
              }
            }
          });

  image_row
      .def("__contains__",
           [](ImageRowAccessor& self,
              std::optional<generic_pixel> to_find) -> bool {
             size width = self.image->get_width();
             for (size x = 0; x < width; ++x) {
               if (self.image->get_either(self.y, x) == to_find) {
                 return true;
               }
             }
             return false;
           })
      .def("__len__",
           [](ImageRowAccessor& self) -> size {
             return self.image->get_width();
           })
      .def("__getitem__",
           [](ImageRowAccessor& self, size index) {
             if (index < 0) {
               index += self.image->get_width();
             }

             if (index < 0 || index >= self.image->get_width()) {
               throw py::index_error("image row index out of range");
             }


             return self.image->get_either(self.y, index);
           })
      .def("__setitem__",
           [](ImageRowAccessor& self, size index,
              std::optional<generic_pixel> to_set) -> void {
             if (to_set.has_value()) {
               self.image->set_either(self.y, index, *to_set);
             } else {
               self.image->reject(self.y, index);
             }
           })
      .def("__next__",
           [](ImageRowAccessor& self) {
             if (self.iter_idx >= self.image->get_width()) {
               self.iter_idx = 0;
               throw py::stop_iteration();
             }
             return self.image->get_either(self.y, self.iter_idx++);
           })
      .def(
          "__reversed__",
          [](py::object /* self */) -> void {
            PyErr_SetString(PyExc_RuntimeError,
                            "Reverse operation is unsupported on images");
          },
          "Unsupported")
      .def(
          "__array__",
          [](ImageRowAccessor& self,
             const py::kwargs& /* unused */) -> py::array {
            void* image_data = self.image->data();
            size rowLen = self.image->get_width();
            switch (self.image->get_type()) {
              case CPL_TYPE_INT: {
                return py::array(py::buffer_info(
                    ((int*)image_data) + (self.image->get_width() * self.y),
                    sizeof(int),  // itemsize
                    py::format_descriptor<int>::format(),
                    1,                                           // ndim
                    std::vector<size>{self.image->get_width()},  // shape
                    std::vector<size>{sizeof(int)}               // strides
                    ));
              }
              case CPL_TYPE_FLOAT: {
                return py::array(py::buffer_info(
                    ((float*)image_data) + (self.image->get_width() * self.y),
                    sizeof(float),  // itemsize
                    py::format_descriptor<float>::format(),
                    1,                                           // ndim
                    std::vector<size>{self.image->get_width()},  // shape
                    std::vector<size>{sizeof(float)}             // strides
                    ));
              }
              case CPL_TYPE_LONG: {
                return py::array(py::buffer_info(
                    ((long*)image_data) + (self.image->get_width() * self.y),
                    sizeof(long),  // itemsize
                    py::format_descriptor<long>::format(),
                    1,                               // ndim
                    std::vector<size>{rowLen},       // shape
                    std::vector<size>{sizeof(long)}  // strides
                    ));
              }

              case CPL_TYPE_FLOAT_COMPLEX: {
                float _Complex* data =
                    ((float _Complex*)image_data) + (rowLen * self.y);

                std::complex<float>* std_data =
                    (std::complex<float>*)cpl_calloc(
                        rowLen, sizeof(std::complex<float>));
                for (int i = 0; i < rowLen; i++) {
                  std_data[i] = cpl::core::complexf_to_cpp(data[i]);
                }
                py::array returnValue(py::buffer_info(
                    std_data,
                    sizeof(std::complex<float>),  // itemsize
                    py::format_descriptor<std::complex<float>>::format(),
                    1,                                              // ndim
                    std::vector<size>{rowLen},                      // shape
                    std::vector<size>{sizeof(std::complex<float>)}  // strides
                    ));
                return returnValue;
              }

              case CPL_TYPE_DOUBLE_COMPLEX: {
                double _Complex* data =
                    ((double _Complex*)image_data) + (rowLen * self.y);
                std::complex<double>* std_data =
                    (std::complex<double>*)cpl_calloc(
                        rowLen, sizeof(std::complex<double>));
                for (int i = 0; i < self.image->get_width(); i++) {
                  std_data[i] = cpl::core::complexd_to_cpp(data[i]);
                }
                py::array returnValue(py::buffer_info(
                    std_data,
                    sizeof(std::complex<double>),  // itemsize
                    py::format_descriptor<std::complex<double>>::format(),
                    1,                                               // ndim
                    std::vector<size>{rowLen},                       // shape
                    std::vector<size>{sizeof(std::complex<double>)}  // strides
                    ));
                return returnValue;
              }
              case CPL_TYPE_LONG_LONG: {
                return py::array(py::buffer_info(
                    ((long long*)image_data) +
                        (self.image->get_width() * self.y),
                    sizeof(long long),  // itemsize
                    py::format_descriptor<long long>::format(),
                    1,                                           // ndim
                    std::vector<size>{self.image->get_width()},  // shape
                    std::vector<size>{sizeof(long long)}         // strides
                    ));
              }

              case CPL_TYPE_DOUBLE: {
                return py::array(py::buffer_info(
                    ((double*)image_data) + (self.image->get_width() * self.y),
                    sizeof(double),  // itemsize
                    py::format_descriptor<double>::format(),
                    1,                                           // ndim
                    std::vector<size>{self.image->get_width()},  // shape
                    std::vector<size>{sizeof(double)}            // strides
                    ));
              }
              default: {
                throw cpl::core::InvalidTypeError(
                    PYCPL_ERROR_LOCATION,
                    "column is of invalid type, cannot be cast to numpy array");
              }
            }
          })
      .def("__str__", [](py::object& self) -> py::object {
        // Delegated to the numpy array to string conversion
        return self.attr("__array__")().attr("__str__")();
      });

  py::class_<cpl::core::ImageList, std::shared_ptr<cpl::core::ImageList>>
      imagelist(m, "ImageList");

  imagelist.doc() = R"pydoc(
          This module provides functions to create and use a cpl_imagelist.

          A CPL ImageList is an ordered list of CPL Images. All images in a list must have the same pixel-type and the same dimensions.

          It is allowed to insert the same image into different positions in the list. Different images in the list are allowed to share the same bad pixel map.

          Parameters
          ----------
          from : iterable of cpl.core.Image, optional
              Images to store in `self` on init. If not given the ImageList is initialised withou any images.

          Raises
          ------
          cpl.core.TypeMismatchError
              images in `from` are of varying types
          cpl.core.IncompatibleInputError
              images in `from` are of varying sizes
    )pydoc";

  py::enum_<cpl_collapse_mode>(imagelist, "Collapse")
      .value("MEAN", CPL_COLLAPSE_MEAN)
      .value("MEDIAN", CPL_COLLAPSE_MEDIAN)
      .value("MEDIAN_MEAN", CPL_COLLAPSE_MEDIAN);

  py::enum_<cpl_swap_axis>(
      imagelist,
      "SwapAxis")  // Unsure if we need all aliases? Giraffe only sets CLI
      .value("XZ", CPL_SWAP_AXIS_XZ)
      .value("YZ", CPL_SWAP_AXIS_YZ);

  imagelist
      .def(py::init())
      // List of Images first since that would be faster in comparison to the
      // from traditional iterable
      .def(py::init<std::vector<std::shared_ptr<cpl::core::ImageBase>>>(),
           py::arg("from"))
      .def(py::init(
               [](py::iterable from) -> std::shared_ptr<cpl::core::ImageList> {
                 std::shared_ptr<cpl::core::ImageList> self =
                     std::make_shared<cpl::core::ImageList>();

                 for (py::handle it : from) {
                   std::shared_ptr<cpl::core::ImageBase> input_im;
                   py::object obj = py::cast<py::object>(it);  // Resolve to
                                                               // python object

                   try {
                     input_im =
                         py::cast<std::shared_ptr<cpl::core::ImageBase>>(obj);
                   }
                   catch (const std::exception& /* unused */) {
                     // Ignore it: try to convert to array instead
                   }

                   py::array input_arr;
                   try {
                     input_arr = py::cast<py::array>(
                         obj);  // Try to convert it to a array type
                     input_im = image_from_arr(input_arr);
                   }
                   catch (const py::cast_error& /* unused */) {
                     throw py::type_error(
                         std::string("expected numpy compatible array, not ") +
                         it.get_type().attr("__name__").cast<std::string>());
                   }

                   self->append(input_im);
                 }
                 return self;
               }),
           py::arg("from"))
      .def("__deepcopy__",
           [](cpl::core::ImageList& self,
              py::dict /* memo */) -> std::shared_ptr<cpl::core::ImageList> {
             return self.duplicate();
           })
      .def("append", &cpl::core::ImageList::append, py::arg("to_append"),
           R"pydoc(
        Append an image to the end of `self`

        It is allowed to insert the same image into two different positions in a list.

        To insert an image a specific position then set via index (e.g. self[i] = new_image)

        It is not allowed to insert images of different sizes or types into a list.

        Parameters
        ----------
        to_append : cpl.core.Image
            The image to append

        Raises
        ------
        cpl.core.TypeMismatchError
            if `to_append` and `self` are of different types
        cpl.core.IncompatibleInputError
            if `to_append` and `self` have different sizes
        )pydoc")
      .def("__getitem__",
           [](cpl::core::ImageList& self,
              long position) -> std::shared_ptr<cpl::core::ImageBase> {
             if (position < 0) {
               position += self.size();
             }

             if (position >= self.size() || position < 0) {
               throw py::index_error("ImageList index out of range");
             }
             return self.get_at(position);
           })
      .def("__len__",
           [](std::shared_ptr<cpl::core::ImageList> self) -> size {
             return self->size();
           })
      .def("__str__",
           [](const cpl::core::ImageList& self) -> std::string {
             return self.dump(cpl::core::Window::All);
           })
// FIXME: The original design provides a keyword argument "window",
//        which is only useful if __str__() is called explicitly.
//        Right now the fixed version of the original implementation
//        is following. Decission to taken which version should be
//        implemented, the one dumping the whole images, consistent
//        with the same method for images, or the one accepting a
//        keyword argument in addition
#if 0
      .def("__str__",
           [](const cpl::core::ImageList& self,
              py::kwargs& kwargs) -> std::string {
             if (kwargs && kwargs.contains("window")) {
               return self.dump(kwargs["window"].cast<cpl::core::Window>());
             } else {
               return self.dump(cpl::core::Window::All);
             }
           })
#endif
      .def("__repr__", &cpl::core::ImageList::dump_structure)
      .def(
          "dump",
          [](const cpl::core::ImageList& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<cpl::core::Window> window,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(),
                                self.dump(window), show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("window") = py::none(), py::arg("show") = true,
          R"pydoc(
            Dump the contents of each image in the ImageList to a file, stdout or a string.

            This function is intended just for debugging. It prints the contents of an image
            to the file path specified by `filename`. 
            If a `filename` is not specified, output goes to stdout (unless `show` is False). 
            In both cases the contents are also returned as a string.

            Parameters
            ----------
            filename : str, optional
                File to dump file image contents to
            mode : str, optional
                Mode to open the file with. Defaults to "w" (write, overwriting the contents of the file if it already exists),
                but can also be set to "a" (append, creating the file if it does not already exist or appending to the end of
                it if it does).
            window : tuple(int,int,int,int), optional
              Window to dump with `value` in the format (llx, lly, urx, ury) where:
              - `llx` Lower left X coordinate
              - `lly` Lower left Y coordinate
              - `urx` Upper right X coordinate 
              - `ury` Upper right Y coordinate
            show : bool, optional
                Send image contents to stdout. Defaults to True.

            Returns
            -------
            str 
                Multiline string containing the dump of the image contents in the ImageList.

                
          )pydoc")
      .def("multiply_image", &cpl::core::ImageList::multiply_image,
           py::arg("img"),
           R"pydoc(
        Multiply an image list by an image.

        Parameters
        ----------
        img : cpl.core.Image
            image to multiply
        )pydoc")
      .def("add_image", &cpl::core::ImageList::add_image, py::arg("img"),
           R"pydoc(
        Add an image list by an image.

        Parameters
        ----------
        img : cpl.core.Image
            image to add
        )pydoc")
      .def("subtract_image", &cpl::core::ImageList::subtract_image,
           py::arg("img"),
           R"pydoc(
        Subtract an image list by an image.

        Parameters
        ----------
        img : cpl.core.Image
            image to subtract
        )pydoc")
      .def("divide_image", &cpl::core::ImageList::divide_image, py::arg("img"),
           R"pydoc(
        Divide an image list by an image.

        Parameters
        ----------
        img : cpl.core.Image
            image to divide
        )pydoc")
      .def("power", &cpl::core::ImageList::power, py::arg("exponent"),
           R"pydoc(
        Compute the elementwise power of each image in the imlist.

        Parameters
        ----------
        exponent : float
            Scalar exponent
        )pydoc")
      .def("add_scalar", &cpl::core::ImageList::add_scalar, py::arg("value"),
           R"pydoc(
        Elementwise addition of a scalar to each image in the imlist. Modified in place

        Parameters
        ----------
        value : float
            Number to add

        Returns
        -------
        None
        )pydoc")
      .def("subtract_scalar", &cpl::core::ImageList::subtract_scalar,
           py::arg("value"),
           R"pydoc(
        Elementwise subtraction of a scalar to each image in the imlist. Modified in place.

        Parameters
        ----------
        value : float
            Number to subtract
        )pydoc")
      .def("multiply_scalar", &cpl::core::ImageList::multiply_scalar,
           py::arg("value"),
           R"pydoc(
        Elementwise multiplication of a scalar to each image in the imlist.

        Parameters
        ----------
        value : float
            Number to multiply with

        Returns
        -------
        None
        )pydoc")
      .def("divide_scalar", &cpl::core::ImageList::divide_scalar,
           py::arg("value"),
           R"pydoc(
        Elementwise division of each image in the imlist with a scalar.

        Parameters
        ----------
        value : float
            Non-zero number to divide with
        )pydoc")
      .def("exponential", &cpl::core::ImageList::exponential, py::arg("base"),
           R"pydoc(
        Compute the elementwise exponential of each image in `self`. Modified in place.

        Parameters
        ----------
        base : float
            Base of the exponential.
        )pydoc")
      .def("collapse_create", &cpl::core::ImageList::collapse_create,
           R"pydoc(
        Average an imagelist to a single image.

        The bad pixel maps of the images in the input list are taken into account, the result image pixels are flagged as rejected for
        those where there were no good pixel at the same position in the input image list.

        For integer pixel types, the averaging is performed using integer division.

        Returns
        -------
        cpl.core.Image
          The average Image
        )pydoc")
      .def("collapse_minmax_create",
           &cpl::core::ImageList::collapse_minmax_create, py::arg("nlow"),
           py::arg("nhigh"), R"pydoc(
        Average with rejection an imagelist to a single image

        The input images are averaged, for each pixel position the nlow lowest pixels
        and the nhigh highest pixels are discarded for the average computation.

        The input image list can be of type cpl.core.Type.INT, cpl.core.Type.FLOAT and
        cpl.core.Type.DOUBLE. The created image will be of the same type.

        On success each pixel in the created image is the mean of the non-rejected
        values on the pixel position in the input image list.

        For a given pixel position any bad pixels (i.e. values) are handled as
        follows:
        Given n bad values on a given pixel position, n/2 of those values are assumed
        to be low outliers and n/2 of those values are assumed to be high outliers.
        Any low or high rejection will first reject up to n/2 bad values and if more
        values need to be rejected that rejection will take place on the good values.
        This rationale behind this is to allow the rejection of outliers to include
        bad pixels without introducing a bias.
        If for a given pixel all values in the input image list are rejected, the
        resulting pixel is set to zero and flagged as rejected.

        Parameters
        ----------
        nlow : int
            Number of low rejected values
        nhigh : int
            Number of high rejected values

        Returns
        -------
        cpl.core.Image
            The average image


        Raises
        ------
        cpl.core.IllegalInputError
            if the input image list is not valid or if the sum of the rejections is
            not lower than the number of images or if nlow or nhigh is negative
        cpl.core.InvalidTypeError
            if the passed image list type is not supported
        )pydoc")
      .def("multiply", &cpl::core::ImageList::multiply, py::arg("other"),
           R"pydoc(
        Elementwise multiply this ImageList with another. Modified in place.

        The two input lists must have the same size, the image number n in the list other is multiplyed with the image number n in this list.

        Parameters
        ----------
        other : cpl.core.ImageList
            ImageList to multiply
        )pydoc")
      .def("add", &cpl::core::ImageList::add, py::arg("other"), R"pydoc(
        Add this ImageList with another. Modified in place.

        The two input lists must have the same size, the image number n in the list other is added to the image number n in this list.

        Parameters
        ----------
        other : cpl.core.ImageList
            ImageList to add
        )pydoc")
      .def("subtract", &cpl::core::ImageList::subtract, py::arg("other"),
           R"pydoc(
        Elementwise subtract this ImageList with another. Modified in place.

        The two input lists must have the same size, the image number n in the list other is subtracted from the image number n in this list.

        Parameters
        ----------
        other : cpl.core.ImageList
            ImageList to subtract with
        )pydoc")
      .def("divide", &cpl::core::ImageList::divide, py::arg("other"), R"pydoc(
        Divide this ImageList with another. Modified in place.

        The two input lists must have the same size, the image number n in the list other is divides the image number n in this list.

        Parameters
        ----------
        other : cpl.core.ImageList
            ImageList to divide with
        )pydoc")
      .def("normalise", &cpl::core::ImageList::normalise, py::arg("mode"),
           R"pydoc(
        Normalize each image in the list. Modified in place.

        The list may be partly modified if an error occurs.

        Possible normalisations are:
        - cpl.core.Image.Normalise.SCALE sets the pixel interval to [0,1].
        - cpl.core.Image.Normalise.MEAN sets the mean value to 1.
        - cpl.core.Image.Normalise.FLUX sets the flux to 1.
        - cpl.core.Image.Normalise.ABSFLUX sets the absolute flux to 1.

        Parameters
        ----------
        mode : cpl.core.Image.Normalise
            Normalization mode.

        )pydoc")
      .def("threshold", &cpl::core::ImageList::threshold, py::arg("lo_cut"),
           py::arg("hi_cut"), py::arg("assign_lo_cut"),
           py::arg("assign_hi_cut"), R"pydoc(
        Threshold all pixel values to an interval.

        Threshold the images of the list using cpl_image_threshold()
        The input image list is modified.

        Pixels outside of the provided interval are assigned the given values.

        Parameters
        ----------
        lo_cut : float
            Lower bound.
        hi_cut : float
            Higher bound.
        assign_lo_cut : float
            Value to assign to pixels below low bound.
        assign_hi_cut : float
            Value to assign to pixels above high bound.
        )pydoc")
      .def("collapse_sigclip_create",
           &cpl::core::ImageList::collapse_sigclip_create, py::arg("kappalow"),
           py::arg("kappahigh"), py::arg("keepfrac"), py::arg("mode"), R"pydoc(
        Collapse an imagelist with kappa-sigma-clipping rejection

        The collapsing is an iterative process which will stop when it converges
        (i.e. an iteration did not reject any values for a given pixel) or
        when the next iteration would reduce the fraction of values to keep
        to less than or equal to keepfrac.

        A call with keepfrac == 1.0 will thus perform no clipping.

        Supported modes:
        cpl.core.ImageList.Collapse.MEAN:
        The center value of the acceptance range will be the mean.
        cpl.core.ImageList.Collapse.MEDIAN:
        The center value of the acceptance range will be the median.
        cpl.core.ImageList.Collapse.MEDIAN_MEAN:
        The center value of the acceptance range will be the median in
        the first iteration and in subsequent iterations it will be the
        mean.

        For each pixel position the pixels whose value is higher than
        center + kappahigh * stdev or lower than center - kappalow * stdev
        are discarded for the subsequent center and stdev computation, where center
        is defined according to the clipping mode, and stdev is the standard
        deviation of the values at that pixel position. Since the acceptance
        interval must be non-empty, the sum of kappalow and kappahigh must be
        positive. A typical call has both kappalow and kappahigh positive.

        The minimum number of values that the clipping can select is 2. This is
        because the clipping criterion is based on the sample standard deviation,
        which needs at least two values to be defined. This means that all calls
        with (positive) values of keepfrac less than 2/n will behave the same. To
        ensure that the values in (at least) i planes out of n are kept, keepfrac
        can be set to (i - 0.5) / n, e.g. to keep at least 50 out of 100 values,
        keepfrac can be set to 0.495.

        The output pixel is set to the mean of the non-clipped values, regardless
        of which clipping mode is used. Regardless of the input pixel type, the
        mean is computed in double precision. The result is then cast to the
        output-pixel type, which is identical to the input pixel type.

        Bad pixels are ignored from the start. This means that with a sufficient
        number of bad pixels, the fraction of good values will be less than keepfrac.
        In this case no iteration is performed at all. If there is at least one
        good value available, then the mean will be based on the good value(s). If
        for a given pixel position there are no good values, then that pixel is
        set to zero, rejected as bad and if available the value in the
        contribution map is set to zero.

        The input imagelist can be of type cpl.core.Type.INT, cpl.core.Type.FLOAT and
        cpl.core.Type.DOUBLE.

        Parameters
        ----------
        kappalow : float
            kappa-factor for lower clipping threshold
        kappahigh : float
            kappa-factor for upper clipping threshold
        keepfrac : float
            The fraction of values to keep (0.0 < keepfrac <= 1.0)
        mode : cpl.core.ImageList.Collapse
            Clipping mode, cpl.core.ImageList.Collapse.MEAN or cpl.core.ImageList.Collapse.MEDIAN

        Returns
        -------
        tuple(cpl.core.Image, cpl.core.Image)
            The collapsed image and the contribution map as an integer image, i.e. the number
            of kept (non-clipped) values after the iterative process on every pixel.
            In the format (collapsed, contribution)


        Raises
        ------
        cpl.core.DataNotFoundError
            if there are less than 2 images in the list
        cpl.core.IllegalInputError
            if the sum of `kappalow` and `kappahigh` is non-positive,
        cpl.core.AccessOutOfRangeError
            if keepfrac is outside the required interval which is 0.0 < keepfrac <= 1.0
        cpl.core.InvalidTypeError
            if the type of the input imagelist is unsupported
        cpl.core.UnsupportedModeError
            if the passed mode is none of the above listed
        )pydoc")
      .def("collapse_median_create",
           &cpl::core::ImageList::collapse_median_create, R"pydoc(
        Create a median image from the Imagelist

        The image list can be of type cpl.core.Type.INT, cpl.core.Type.FLOAT and
        cpl.core.Type.DOUBLE.

        On success each pixel in the created image is the median of the values on
        the same pixel position in the images in the list. If for a given pixel all
        values in the input image list are rejected the resulting pixel is set to
        zero and flagged as rejected.

        The median is defined here as the middle value of an odd number of sorted
        samples and for an even number of samples as the mean of the two central
        values. Note that with an even number of samples the median may not be
        among the input samples.

        Also note that in the case of an even number of integer images the mean
        value will be computed using integer arithmetic. Cast your integer data
        to a floating point pixel type if that is not the desired behavior.

        Returns
        -------
        cpl.core.Image
            The median image of the input pixel type


        Raises
        ------
        cpl.core.IllegalInputError
            if the image list is not valid
        )pydoc")
      .def("swap_axis_create", &cpl::core::ImageList::swap_axis_create,
           py::arg("mode"),
           R"pydoc(
        This function is intended for users that want to use the ImageList object as a cube.

        Swapping the axis would give them access to the usual functions in the 3 dimensions. This has the cost that it duplicates the memory
        consumption, which can be a problem for big amounts of data.

        Image list can be cpl.core.Type.INT, cpl.core.Type.FLOAT or cpl.core.Type.DOUBLE

        Parameters
        ----------
        mode : cpl.core.ImageList.SwapAxis
            The swapping mode. The mode can be either cpl.core.ImageList.SwapAxis.XZ or cpl.core.ImageList.SwapAxis.YZ

        Returns
        -------
        New image list of the given axis
        )pydoc")
      .def("save", &cpl::core::ImageList::save, py::arg("filename"),
           py::arg("pl"), py::arg("mode"),
           py::arg("dtype") = CPL_TYPE_UNSPECIFIED,
           R"docstring(
        Save an imagelist to a FITS file

        This function saves an image list to a FITS file. If a property list is provided, it is written to the named file before the pixels are written.

        Image lists are saved as a 3 dimensional data cube.

        Supported image types are cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT.

        The type used in the file can be one of: cpl.core.Type.UCHAR (8 bit unsigned), cpl.core.Type.SHORT (16 bit signed), cpl.core.Type.USHORT
        (16 bit unsigned), cpl.core.Type.INT (32 bit signed), cpl.core.Type.FLOAT (32 bit floating point), or cpl.core.Type.DOUBLE (64 bit floating point).
        By default the saved type is cpl.core.Type.UNSPECIFIED. This value means that the type used for saving is the pixel type
        of the input image. Using the image pixel type as saving type ensures that the saving incurs no loss of information.

        Supported output modes are cpl.core.io.CREATE (create a new file) and cpl.core.io.EXTEND (append a new extension to an existing file)

        Note that in append mode the file must be writable (and do not take for granted that a file is writable just because it was created by the same
        application, as this depends from the system umask).

        The output mode cpl.core.io.EXTEND can be combined (via bit-wise OR) with an option for tile-compression. This compression is lossless.
        The options are: cpl.core.io.COMPRESS_GZIP, cpl.core.io.COMPRESS_RICE, cpl.core.io.COMPRESS_HCOMPRESS, cpl.core.io.COMPRESS_PLIO. With compression
        the type must be cpl.core.Type.UNSPECIFIED or cpl.core.Type.INT.

        In extend and append mode, make sure that the file has write permissions. You may have problems if you create a file in your application and
        append something to it with the umask set to 222. In this case, the file created by your application would not be writable, and the append would fail.

        Parameters
        ----------
        filename : str
            Name of the file to write
        pl : cpl.core.PropertyList, optional
            Property list for the output header. None by default.
        mode : unsigned int
            Desired output options, determined by bit-wise OR of cpl.core.io enums
        dtype : cpl.core.Type, optional
            The type used to represent the data in the file. By default it saves using the image's current dtype

        Raises
        ------
        cpl.core.IllegalInputError
            if the type or the mode is not supported
        cpl.core.InvalidTypeError
            if the passed pixel type is not supported
        cpl.core.FileNotCreatedError
            If the output file cannot be created
        cpl.core.FileIOError
            if the data cannot be written to the file

        See Also
        --------
        cpl.core.Image.save : for saving individual images to a fits file
        )docstring")
      .def_static("load", &cpl::core::load_imagelist, py::arg("filename"),
                  py::arg("dtype") = CPL_TYPE_DOUBLE, py::arg("extension") = 0,
                  py::arg("area") = py::none(), R"pydoc(
                    Load an image list from a file.

                    Load data from the extension `extension` of the FITS file
                    `filename` into a list of images. The FITS extension may be
                    an image (``NAXIS`` = 2) or a data cube (``NAXIS`` =  3). Each
                    image plane in the input data is loaded as a separate image.
                    By default the data is read from the primary HDU of the FITS
                    file.

                    By default the full area (extent in x and y) of the data is
                    loaded. This may be restricted to a particular region of
                    the data by providing an appropriate argument `area`.

                    The argument `dtype` specifies the pixel data type of the result
                    image list. When the data is loaded the pixel data type in the
                    input FITS file is converted into `dtype`. By default the data in
                    the input extension is converted to cpl.core.Type.DOUBLE. To load
                    the data without converting the pixel data type use
                    cpl.core.Type.UNSPECIFIED.

                    Valid pixel data types which may be used for `dtype` are:

                    - cpl.core.Type.INT  (32-bit integer)
                    - cpl.core.Type.FLOAT
                    - cpl.core.Type.DOUBLE

                    Parameters
                    ----------

                    filename : str
                      Name of the input file
                    dtype : cpl.core.Type, optional
                      Data type of the pixels in the returend list of images. Is cpl.core.Type.DOUBLE by default.
                    extension : int, default=0
                      Index of the FITS extension to load (the primary data
                      unit has index 0)
                    Area : Tuple, default=None
                      Region of interest to load given as a tuple specifying
                      the lower left x, the lower left y, the upper right x (inclusive)
                      and the upper right y coordinate (inclusive) in this order.
                      Numbering of the pixel x and y positions starts at 1
                      (FITS convention)

                    Returns
                    -------
                    cpl.core.ImageList
                      New image list instance of loaded data

                    Raises
                    ------
                    cpl.core.FileIOError
                      If the file cannot be opened, or does not exist.
                    cpl.core.BadFileFormatError
                      If the data cannot be loaded from the file.
                    cpl.core.InvalidTypeError
                      If the requested pixel data type is not supported.
                    cpl.core.IllegalInputError
                      If the requested extension number is invalid (negative),
                      the plane number is out of range, or if the given image region
                      is invalid.
                    cpl.core.DataNotFoundError
                      If the specified extension has no image data.
                  )pydoc")
      .def(
          "__setitem__",
          [](cpl::core::ImageList& self, long position,
             std::shared_ptr<cpl::core::ImageBase> item) -> void {
            if (position >= self.size() || position < 0) {
              throw py::index_error("ImageList index out of range");
            }
            self.set(item, position);
          },
          py::arg("index"), py::arg("image"), "Set image at index position")
      .def(
          "insert",
          [](cpl::core::ImageList& self, long position,
             std::shared_ptr<cpl::core::ImageBase> item) -> void {
            if (position > self.size() || position < 0) {
              throw py::index_error("ImageList index out of range");
            }
            self.insert(item, position);
          },
          py::arg("position"), py::arg("item"), R"pydoc(
        Insert an image into the index `position`. This will increase the imagelist size by 1

        Parameters
        ----------
        position : int
            index to insert Image
        item : cpl.core.Image
            Image to insert
        )pydoc")
      .def("astype", &cpl::core::ImageList::cast, py::arg("dtype"), R"pydoc(
        Cast an imagelist to a different CPL type

        Parameters
        ----------
        dtype : cpl.core.Type
            Type to cast the imagelist to

        Returns
        -------
        New ImageList, containing images cast to the specified type
        )pydoc")
      .def(
          "pop",
          [](cpl::core::ImageList& self, std::optional<size> PyPosition)
              -> std::shared_ptr<cpl::core::ImageBase> {
            size position = PyPosition.value_or(
                self.size() - 1);  // Use given value or use the last index
            if (position >= self.size() || position < 0) {
              throw py::index_error("ImageList index out of range");
            }
            return self.pop(position);
          },
          py::arg("position") = py::none(),
          R"pydoc(
        Remove and return the image at `position`

        Parameters
        ----------
        position : int, optional
            Index to pop image from the image list. Defaults to the last image.

        Raises
        ------
        IndexError
            If `position` is out of range
        )pydoc")
      .def("__delitem__",
           [](cpl::core::ImageList& self, long position) -> void {
             if (position >= self.size() || position < 0) {
               throw py::index_error("ImageList index out of range");
             }
             self.pop(position);
           })
      .def("empty", &cpl::core::ImageList::empty, R"pydoc(
        Empty an imagelist and deallocate all its images

        After the call the image list can be populated again.
        )pydoc")
      .def("is_uniform", &cpl::core::ImageList::is_uniform, R"pydoc(
        Determine if an imagelist contains images of equal size and type

        The function raises an error if the imagelist is empty (see Raises)

        Returns
        -------
        bool
          True if uniform, otherwise false

        Raises
        ------
        cpl.core.IllegalInputError
          If the imagelist is empty
        )pydoc");
}
