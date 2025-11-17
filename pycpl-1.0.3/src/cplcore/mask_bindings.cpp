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

#include "cplcore/mask_bindings.hpp"

#include <cstring>
#include <memory>

#include <pybind11/eval.h>
#include <pybind11/stl.h>

#include "cplcore/image.hpp"
#include "cplcore/mask.hpp"
#include "cplcore/window_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive
#include "dump_handler.hpp"
#include "path_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive


namespace py = pybind11;

using size = cpl::core::size;

void
bind_mask(py::module& m)
{
  /**
   * @brief Minimal C++ bindings required for all cpl::core::Mask functionality
   * to be usable from python, without niceties applied
   *
   * This is a thinner wrapper over the C++ cpl::core::Mask than what we'd like,
   * so it's a private class, that is wrapped by a more ergonomic python class
   * cpl.core.Mask. This only allows access/setting by conversion to/from
   * unsigned char*
   *
   * For performance concerns, we add the size & length parameter to setting/
   * getting bytes, so that the wrapping cpl.core.Mask can set multiple bytes at
   * once.
   *
   * This class is also 1-dimensional. converting to a 2d representation
   * requires a helper class for mutable row access, again easier done in python
   * than C++
   */

  py::class_<cpl::core::Mask, std::shared_ptr<cpl::core::Mask>> mask1d(
      m, "_Mask1D", py::buffer_protocol());

  mask1d.def(py::init<const cpl::core::ImageBase&, double, double>())
      .def("threshold_image", &cpl::core::Mask::threshold_image)
      .def(py::init(
               [](int width, int height, py::object data) -> cpl::core::Mask {
                 if (data.is_none()) {
                   return cpl::core::Mask(width, height, nullptr);
                 } else {
                   return cpl::core::Mask(width, height,
                                          std::string(data.cast<py::bytes>()));
                 }
               }),
           py::arg("width"), py::arg("height"), py::arg("data") = py::none())
      .def_static("load", cpl::core::load_mask, py::arg("filename"),
                  py::arg("extension") = 0, py::arg("plane") = 0,
                  py::arg("window") = py::none())
      .def("get_bytes",
           [](const cpl::core::Mask& self, size index,
              size length) -> py::bytes {
             size data_size = self.get_size();

             if (length < 0 || index < 0 || index + length > data_size) {
               throw std::out_of_range(
                   "get_bytes index or size is larger than this mask, or is "
                   "negative.");
             }

             // Avoids copy to std::string. Not sure if this makes a noticable
             // performance difference.
             return py::bytes(
                 reinterpret_cast<const char*>(&self.data()[index]), length);
           })
      .def(
          "set_bytes",
          [](cpl::core::Mask& self, size index, py::bytes input_bytes) -> void {
            size data_size = self.get_size();

            std::string input_data = static_cast<std::string>(input_bytes);
            size_t input_length = input_data.length();

            if ((index < 0) || (input_length + index > data_size)) {
              PyErr_SetString(PyExc_IndexError,
                              "get_bytes index or size is larger than this "
                              "mask, or is negative.");
              throw std::out_of_range(
                  "set_bytes data would run the masks' end, or is placed "
                  "before its beginning (negative index).");
            }

            std::memcpy(self.data() + index, input_data.data(), input_length);
          })
      .def_property_readonly(
          "width",
          [](const cpl::core::Mask& self) -> size { return self.get_width(); })
      .def_property_readonly(
          "height",
          [](const cpl::core::Mask& self) -> size { return self.get_height(); })
      .def("is_empty", &cpl::core::Mask::is_empty)
      .def("count", &cpl::core::Mask::count, py::arg("window") = py::none())
      //.def("__str__", &cpl::core::Mask::dump,py::arg("window") = py::none())
      .def("__repr__",
           [](cpl::core::Mask& self) -> py::str {
             py::str representation = "<cpl.core.Mask, {}x{} {} mask>";
             return representation.format(self.get_width(), self.get_height(),
                                          (self.is_empty()) ? "empty"
                                                            : "non-empty");
           })
      .def(
          "dump",
          [](cpl::core::Mask& self,
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
        Dump the mask contents to a file, stdout or a string.

        This function is intended just for debugging. It prints the contents of a mask 
        to the file path specified by `filename`. 
        If a `filename` is not specified, output goes to stdout (unless `show` is False). 
        In both cases the contents are also returned as a string.

        Parameters
        ----------
        filename : str, optional
            File to dump mask contents to
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
            Send mask contents to stdout. Defaults to True.

        Returns
        -------
        str 
            Multiline string containing the dump of the mask contents.

            
      )pydoc")
      .def("__and__", &cpl::core::Mask::operator&)
      .def("__or__", &cpl::core::Mask::operator|)
      .def("__xor__", &cpl::core::Mask::operator^)
      .def("__invert__", &cpl::core::Mask::operator~)
      .def("collapse_rows", &cpl::core::Mask::collapse_rows)
      .def("collapse_cols", &cpl::core::Mask::collapse_cols)
      .def("extract", &cpl::core::Mask::extract)
      .def("rotate", &cpl::core::Mask::rotate)
      .def("shift", &cpl::core::Mask::shift)
      .def("insert", &cpl::core::Mask::insert)
      .def("flip", &cpl::core::Mask::flip)
      .def("move",
           [](cpl::core::Mask& self, int nb_cut,
              py::list positions) -> cpl::core::Mask {
             std::vector<size> positions_cpp;
             for (auto item : positions) {
               positions_cpp.push_back(item.cast<size>() + 1);
             }
             return self.move(nb_cut, positions_cpp);
           })
      .def("subsample", &cpl::core::Mask::subsample)
      .def("filter", &cpl::core::Mask::filter)
      .def("save", &cpl::core::Mask::save)
      .def("__eq__",
           [](const cpl::core::Mask& self, py::object eq_arg) -> bool {
             try {
               return self == eq_arg.cast<cpl::core::Mask&>();
             }
             catch (const py::cast_error& wrong_type) {
               return false;
             }
           })
      .def(py::pickle(
          [](cpl::core::Mask& self) -> py::tuple {
            return py::make_tuple(self.get_width(), self.get_height(),
                                  py::bytes(std::string(self)));
          },
          [mask1d](py::tuple t) -> cpl::core::Mask {
            return cpl::core::Mask(t[0].cast<int>(), t[1].cast<int>(),
                                   t[2].cast<std::string>());
          }))
      .def_buffer([](cpl::core::Mask& self) -> py::buffer_info {
        return py::buffer_info(
            reinterpret_cast<void*>(self.data()),   // Pointer to buffer
            sizeof(unsigned char),                  // Size of a single element
            py::format_descriptor<bool>::format(),  // Python struct- style
                                                    // format descriptor
            2,                                      // Number of dimensions
            {static_cast<size_t>(self.get_height()),
             static_cast<size_t>(self.get_width())},  // Dimensions
            {// Strides (in bytes) in each dimension
             static_cast<size_t>(sizeof(unsigned char) * self.get_width()),
             static_cast<size_t>(sizeof(unsigned char))});
      });

  // Create the ergonomic cpl.core.Mask
  py::exec(
      R"(
            import numpy as np
            from collections.abc import Collection

            class Mask(np.ndarray):
                '''
                These masks are useful for object detection routines or bad pixel map handling. 
                
                Morphological routines (erosion, dilation, closing and opening) and logical operations are provided. 
                
                CPL masks are like a 2d binary array, with each pixel representing True or False, and can be set as such:

                .. code-block:: python

                    m = cpl.core.Mask(3,3)
                    m[0][0] = True

                PyCPL uses 0 indexing, in the sense that the lower left element in a CPL mask has index (0, 0).
                '''
                @classmethod
                def _2d_to_bytes(self, lists):
                    height = len(lists)
                    if len(lists) == 0:
                        raise ValueError("Mask expected a non-empty list of lists, empty list given")
                    width = len(lists[0])

                    maskbytes = bytearray()
                    for row in lists:
                        if len(row) != width:
                            raise ValueError("Mask from list of lists requires the lists be homogenous")
                        maskbytes += bytes(row)
                    return _Mask1D(width, height, bytes(maskbytes))

                @classmethod
                def load(cls, fitsfile, extension=0, plane=0 , window=None):
                    '''
                    Loads an bitmask from an INTEGER FITS file
                    
                    Parameters
                    ----------
                    fitsfile : str
                        filename of fits file
                    extension : int
                        Specifies the extension from which the image should be loaded
                        (Default) 0 is for the main data section (Files without extension)
                    plane : int
                        Specifies the plane to request from the data section. Default 0.
                    window : tuple(int, int, int, int)
                        The rectangle in the format (x1,y1, x2, y2) specifying the subset of the image to load. 
                        If None, load the entire window
                    
                    Raises
                    ------
                    cpl.core.FileIOError
                        if the file cannot be opened or does not exist
                    cpl.core.BadFileFormatError
                        if the data cannot be loaded from the file
                    cpl.core.IllegalInputError
                        if the pased extension number is negative
                    cpl.core.DataNotFoundError
                        if the specified extension has no mask data
                    '''
                    if(window):
                        return cls(_Mask1D.load(*args))
                    else:
                        return cls(_Mask1D.load(fitsfile, extension, plane))
                
                def __new__(cls,*args):
                    '''
                    Generate a new Mask with the following formats:
                    Mask(Collection) : Pass a non-empty list of homogeneous lists
                    Mask(width, height, bytes) : Build a 2d mask from a bytestring with given dimensions
                    
                    Raises
                    ------
                    cpl.core.IllegalInputError
                        if given width or height is negative
                    '''
                    if len(args) == 1 and isinstance(args[0], Collection):
                        new_mask = Mask._2d_to_bytes(args[0]) #Non-empty list of homogenous lists
                    elif len(args) == 3 and isinstance(args[2], bytes):
                        new_mask = _Mask1D(*args) #Optional bytes present
                    elif len(args) == 3 and isinstance(args[0], Image):
                        new_mask = _Mask1D(*args) #Create new mask using image thresholding
                    elif len(args) == 2:
                        new_mask = _Mask1D(*args) #Optional bytes not present
                    elif len(args) == 1 and isinstance(args[0], _Mask1D):
                        new_mask = args[0] # Internally used wrapper
                    else:
                        raise TypeError("Mask expected width, height & (optional) bytes, or a non-empty list of homogeneous lists")

                    obj=np.array(new_mask,copy=False).view(cls)

                    obj._mask= new_mask
                    return obj
                def rotate(self, turns):
                    '''
                    Rotate this mask by a multiple of 90 degrees clockwise
                    
                    Parameters
                    ----------
                    turns : int
                        Integral amount of 90 degree turns to execute.
                    
                    Notes
                    -----
                    `turns` can be any value, its modulo 4 determines rotation:
                    
                    - -3 to turn 270 degrees counterclockwise.
                    - -2 to turn 180 degrees counterclockwise.
                    - -1 to turn  90 degrees counterclockwise.
                    -  0 to not turn
                    - +1 to turn  90 degrees clockwise (same as -3)
                    - +2 to turn 180 degrees clockwise (same as -2).
                    - +3 to turn 270 degrees clockwise (same as -1).
                    
                    The lower left corner of the image is at (0,0), 
                    x increasing from left to right, y increasing from bottom to top.
                    '''
                    self._mask.rotate(turns)
                    self.shape=(self._mask.height, self._mask.width)
                
                @property
                def width(self):
                    """ Number of columns wide this mask is"""
                    return self._mask.width

                @property
                def height(self):
                    """ Number of rows high this mask is"""
                    return self._mask.height

                #Checks equality by xoring the two masks
                def __eq__(self, other):
                    '''
                    Checks equality by xoring the two masks
                    '''
                    if not isinstance(other, self.__class__):
                        return False

                    return Mask(self._mask.__xor__(other._mask)).is_empty()

                def copy(self):
                    '''
                    Duplicate the mask
                    '''
                    newCopy=Mask(self.width, self.height)
                    newCopy.insert(self,0,0)
                    return newCopy
                
                def __deepcopy__(self):
                    return self.copy()
                
                def is_empty(self):
                    return self._mask.is_empty()

                def count(self, window = None):
                    '''
                    Determines number of occurrences of '1' bit in the given window of this bitmask

                    Parameters
                    ----------
                    window : int
                        Rectangle to count bits in the format (x1,y1,x2,y2)

                    '''
                    return self._mask.count() if window == None else self._mask.count(window)

                def __and__(self, other):
                    return Mask(self._mask.__and__(other._mask))

                def __or__(self, other):
                    return Mask(self._mask.__or__(other._mask))

                def __xor__(self, other):
                    return Mask(self._mask.__xor__(other._mask))

                def __invert__(self):
                    return Mask(self._mask.__invert__())

                def collapse_rows(self):
                    '''
                    Create a 1-row mask, all elements are the logical AND of each cell in its 
                    corresponding column. Width is kept the same
                    '''
                    return Mask(self._mask.collapse_rows())

                def collapse_cols(self):
                    '''
                    Create a 1-column mask, all elements are the logical AND of each cell in its 
                    corresponding column. Height is kept the same.
                    '''
                    return Mask(self._mask.collapse_cols())

                def extract(self, window):
                    '''
                    Copies out a window of this mask to a new mask.

                    Parameters
                    ----------
                    window : tuple(int, int, int, int)
                        rectangle to extract from this mask in the format (x1,y1, x2, y2)

                    Raises
                    ------
                    cpl.core.IllegalInputError
                        if the zone falls outside the mask
                    '''
                    return Mask(self._mask.extract(window))

                def shift(self, yshift, xshift):
                    '''
                    shift a mask

                    The 'empty zone' in the shifted mask is set to True. 
                    
                    The shift values have to be valid: -nx < dx < nx and -ny < dy < ny

                    Parameters
                    ----------
                    yshift : int
                        shift in y
                    xshift : int
                        shift in x
                    
                    Raises
                    ------
                    cpl.core.IllegalInputError
                        if the offsets are too big
                    '''
                    self._mask.shift(yshift, xshift)

                def insert(self, to_insert, ypos, xpos):
                    '''
                    insert a mask into self
                    
                    Parameters
                    ----------
                    to_insert : cpl.core.Mask
                        mask to insert into self
                    ypos : int
                        the y pixel position in self where the lower left pixel of to_insert should go 
                        (from 0 to the y size of self)
                    xpos : int
                        the x pixel position in self where the lower left pixel of to_insert should go 
                        (from 0 to the x size of in self)
                    
                    Raises
                    ------
                    cpl.core.IllegalInputError
                        if xpos or ypos is outside self
                    '''
                    self._mask.insert(to_insert._mask, ypos, xpos)

                def flip(self, axis):
                    """
                    Flip a mask on a given mirror line. 

                    Parameters
                    ----------
                    axis : int
                        angle to mirror line in polar coord. is theta = (PI/4) * angle 
                        - 0 (ϑ=0) to flip the image around the horizontal
                        - 1 (ϑ=π∕4) to flip the image around y=x
                        - 2 (ϑ=π∕2) to flip the image around the vertical
                        - 3 (ϑ=3π∕4) to flip the image around y=-x
                    
                    Raises
                    ------
                    cpl.core.IllegalInputError
                        if angle is not as specified
                    """
                    self._mask.flip(axis)

                def move(self, nb_cut, positions):
                    '''
                    Reorganize the pixels in a mask. 

                    nb_cut must be positive and divide the size of the input mask in x and y.
                
                    Parameters
                    ----------
                    nb_cut : int
                        the number of cut in x and y 
                    new_pos : list of integers
                        array with the nb_cut^2 new positions

                    Raises
                    ------
                    cpl.core.IllegalInputError
                        if nb_cut is not as requested.
                    '''
                    self._mask.move(nb_cut, positions)

                def subsample(self, ystep, xstep):
                    '''
                    Subsample a mask.
                    

                    Parameters
                    ----------
                    ystep : int
                        Take every ystep pixel in y
                    xstep : int
                        Take every xstep pixel in x  

                    Raises
                    ------
                    cpl.core.IllegalInputError
                        if xstep and ystep are not greater than zero
                    '''
                    return Mask(self._mask.subsample(ystep, xstep))

                def filter(self, kernel, filter, border):
                    '''
                    Filter the mask using a binary kernel. 

                    The kernel must have an odd number of rows and an odd number of columns, with at least one pixel set to 1

                    Parameters
                    ----------
                    kernel : cpl.core.Mask
                        Mask of elements to use (for each pixel set to 1)
                    filter : cpl.core.Filter
                        cpl.core.Filter.EROSION, cpl.core.Filter.DILATION, cpl.core.Filter.OPENING, cpl.core.Filter.CLOSING 
                    border : cpl.core.Border
                        cpl.core.Border.NOP, cpl.core.Border.ZERO or cpl.core.Border.COPY 

                    Raises
                    ------
                    cpl.core.DataNotFoundError 
                        If the kernel is empty.
                    cpl.core.AccessOutOfRangeError
                        If the kernel has a side longer than the input mask.
                    cpl.core.UnsupportedModeError 
                        if the border/filter mode is unsupported.

                    Returns
                    ------
                    Mask(self._mask.filter(kernel._mask, filter, border))
                        filter_output mask same size as input

                    Notes
                    -----
                    For erosion and dilation: In-place filtering is not supported, but the input buffer may overlap all but the 
                    1+h first rows of the output buffer, where 1+2*h is the number of rows in the kernel.

                    For opening and closing: Opening is implemented as an erosion followed by a dilation, and closing is implemented 
                    as a dilation followed by an erosion. As such a temporary, internal buffer the size of self is allocated and used. 
                    Consequently, in-place opening and closing is supported with no additional overhead, it is achieved by passing 
                    the same mask as both self and other.

                    Duality and idempotency: Erosion and Dilation have the duality relations: not(dil(A,B)) = er(not(A), B) and 
                    not(er(A,B)) = dil(not(A), B).
                    
                    Opening and closing have similar duality relations: not(open(A,B)) = close(not(A), B) and not(close(A,B)) = open(not(A), B).
                    
                    Opening and closing are both idempotent, i.e. open(A,B) = open(open(A,B),B) and close(A,B) = close(close(A,B),B).
                    
                    The above duality and idempotency relations do not hold on the mask border (with the currently supported border modes).
                    
                    Unnecessary large kernels: Adding an empty border to a given kernel should not change the outcome of the filtering. However
                    doing so widens the border of the mask to be filtered and therefore has an effect on the filtering of the mask border. Since 
                    an unnecessary large kernel is also more costly to apply, such kernels should be avoided.

                    1x3 erosion example:

                    .. code-block:: python
                        
                        kernel = ~cpl.core.Mask(1,3)
                        filtered.filter(kernel,cpl.core.Filter.EROSION,cpl.core.Border.NOP)
                    '''
                    return Mask(self._mask.filter(kernel._mask, filter, border))

                def save(self,filename,pl, mode):
                    '''
                    Save a mask to a FITS file. 

                    Parameters
                    ----------
                    filename : str
                        Name of the file to write 
                    pl : cpl.core.PropertyList
                        Property list for the output header (Default None)
                    mode : unsigned int 
                        Desired output options, determined by bitwise or of cpl.core.io enums

                    Raises
                    ------
                    cpl.core.IllegalInputError
                        if the mode is unsupported
                    cpl.core.NotCreatedError
                        if the output file cannot be created
                    cpl.core.FileIOError
                        if the data cannot be written to the file

                    Notes
                    -----
                    This function saves a mask to a FITS file. If a property list is provided, it is written to the header where the mask is written.

                    The type used in the file is cpl.core.Type.UCHAR (8 bit unsigned).

                    Supported output modes are cpl.core.io.CREATE (create a new file) and cpl.core.io.EXTEND (append a new extension to an existing file)

                    The output mode cpl.core.io.EXTEND can be combined (via bit-wise or) with an option for tile-compression. This compression is lossless. 
                    The options are: cpl.core.io.COMPRESS_GZIP, cpl.core.io.COMPRESS_RICE, cpl.core.io.COMPRESS_HCOMPRESS, cpl.core.io.COMPRESS_PLIO.

                    Note that in append mode the file must be writable (and do not take for granted that a file is writable just because it was created 
                    by the same application, as this depends from the system umask)
                    '''
                    self._mask.save(filename,pl,mode)

                def __str__(self):
                    return self.dump(show=False)
                def __repr__(self):
                    return self._mask.__repr__()
                def print(self):
                    return '\n'.join([
                        ''.join([
                            '█' if cell else '·' for cell in row
                        ])
                        for row in reversed(self)
                    ])
                def dump(self,filename="",mode="w",window=None,show=True):
                    '''
                    Dump the mask contents to a file, stdout or a string.
                
                    This function is intended just for debugging. It prints the contents of a mask 
                    to the file path specified by `filename`. 
                    If a `filename` is not specified, output goes to stdout (unless `show` is False). 
                    In both cases the contents are also returned as a string.
                
                    Parameters
                    ----------
                    filename : str, optional
                        File to dump mask contents to
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
                      Defaults to entire image.
                    show : bool, optional
                        Send mask contents to stdout. Defaults to True.
                
                    Returns
                    -------
                    str 
                        Multiline string containing the dump of the mask contents.
                    '''    

                    return self._mask.dump(filename,mode,window,show)

                @classmethod
                def threshold_image(cls, image, lo_cut, hi_cut, inval):
                    '''
                    Create a new Mask by applying the given thresholds to a `cpl.core.Image`.

                    Parameters
                    ----------
                    image : cpl.core.Image
                        Image to threshold
                    lo_cut : float
                        Lower bound for threshold
                    hi_cut : float
                        Uppper bound for threshold
                    inval : bool
                        This value (0 or 1, False or True) is assigned where
                        the pixel value is not marked as rejected and is strictly
                        inside the provided interval. The other positions are assigned
                        the other value.

                    Raises
                    ------
                    cpl.core.UnsupportedModeError
                        if the image data type is unsupported
                    cpl.core.IllegalInputError
                        if inval is not binary

                    Notes
                    -----
                    The input image type can be cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT or cpl.core.Type.INT.

                    If `lo_cut` is greater than or equal to `hi_cut`, then the mask is filled with
                    outval (opposite of `inval`).
                    '''
                    threshold_mask = cls(image.width, image.height)
                    threshold_mask._mask.threshold_image(image, lo_cut, hi_cut, inval)
                    return threshold_mask
        )",
      m.attr("__dict__"));
}
