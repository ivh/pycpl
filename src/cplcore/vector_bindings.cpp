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

#include "cplcore/vector_bindings.hpp"

#include <filesystem>

#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "cplcore/bivector.hpp"
#include "cplcore/vector.hpp"
#include "dump_handler.hpp"
#include "path_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive

namespace py = pybind11;

using size = cpl::core::size;

/**
 * An iterator over vector (not the C++ STL kind of iterator)
 * that is bound to python as an iterator (has __next__, __iter__)
 *
 * I did not create a C++ iterator for the C++ cpl::core::Vector class because:
 *  - It  would NOT keep around the cpl::core::Vector class (unless we force
 *    every cpl::core::Vector to be wrapped in a shared_ptr)
 *  - It's quite a lot of boilerplate.
 *  - It would require accessing the cpl::core::Vector::data
 *    instead of cpl::core::Vector::get() (since it doesn't return a pointer,
 *    and STL iterators need pointers)
 * However
 *  - This is not really usable to any C++ code.
 */
struct VectorIterator
{
  py::object vector;
  size index;
};

void
bind_vector(py::module& m)
{
  py::enum_<cpl_kernel> kernel_class(m, "Kernel");

  kernel_class
      .value("DEFAULT", CPL_KERNEL_DEFAULT,
             "default kernel, currently cpl.core.Kernel.TANH")
      .value("TANH", CPL_KERNEL_TANH, "Hyperbolic tangent")
      .value("SINC", CPL_KERNEL_SINC, "Sinus cardinal")
      .value("SINC2", CPL_KERNEL_SINC2, "Square sinus cardinal")
      .value("LANCZOS", CPL_KERNEL_LANCZOS, "Lanczos2 kernel")
      .value("HAMMING", CPL_KERNEL_HAMMING, "Hamming kernel")
      .value("HANN", CPL_KERNEL_HANN, "Hann kernel")
      .value("NEAREST", CPL_KERNEL_NEAREST,
             "Nearest neighbor kernel (1 when dist < 0.5, else 0)");

  py::enum_<cpl_fit_mode> fit_mode_class(m, "FitMode", py::arithmetic());

  fit_mode_class.value("CENTROID", CPL_FIT_CENTROID)
      .value("STDEV", CPL_FIT_STDEV)
      .value("AREA", CPL_FIT_AREA)
      .value("OFFSET", CPL_FIT_OFFSET)
      .value("ALL", CPL_FIT_ALL);

  py::enum_<cpl_sort_direction> sort_class(m, "Sort");

  sort_class
      .value("DESCENDING", CPL_SORT_DESCENDING,
             "For use with cpl.core.Vector.sort() for descending order sort")
      .value("ASCENDING", CPL_SORT_ASCENDING,
             "For use with cpl.core.Vector.sort() for ascending order sort");

  py::enum_<cpl_sort_mode> sort_mode_class(m, "SortMode");

  sort_mode_class.value("BY_X", CPL_SORT_BY_X).value("BY_Y", CPL_SORT_BY_Y);

  py::class_<cpl::core::Vector, std::shared_ptr<cpl::core::Vector>> vector(
      m, "Vector");
  vector.doc() = R"mydelim(
        Class for ordered sequences of numbers.

        A `cpl.core.Vector` contains an ordered list of double precision floating point numbers.
        It has methods for sorting, statistics, and other simple operations. Two Vectors may
        be combined into a `cpl.core.Bivector` to represent sequences of x and y values.

        A Vector can also be created using the zeros class method.

        Parameters
        ----------
        data : iterable of floats
            An iterable object which yields floating point values.

        See Also
        --------
        cpl.core.Bivector: Class for pairs of ordered sequences of numbers.
        cpl.core.Vector.zeros: Create a Vector of given length, initialised with 0's.

        Examples
        --------
        >>> vector_list = cpl.core.Vector([1, 2, 3])
        ... vector_tuple = cpl.core.Vector((4, 5, 6))
        ... vector_copy = cpl.core.Vector(vector_list)
        ... vector_zeros = cpl.core.Vector.zeros(5)
    )mydelim";

  py::enum_<cpl_lowpass> lowpass(
      vector, "LowPass",
      "Filter type for cpl.core.Vector.filter_lowpass_create");

  lowpass.value("LINEAR", CPL_LOWPASS_LINEAR)
      .value("GAUSSIAN", CPL_LOWPASS_GAUSSIAN);

  vector
      // Since this Vector is itself iterable, this copy constructor isn't
      // necessary (it can be passed as argument to iterable constructor)
      // but it's more performant to do it in C++. This copy constructor
      // has to be .def'd before the iterable one, or the iterable one will
      // be used for cpl.core.Vectors. In the docs this can still be defined as
      // data
      .def(py::init<const cpl::core::Vector&>(), py::arg("data"))
      .def(py::init(&py_vec_constructor), py::arg("data"))
      .def_static(
          "zeros",
          [](size vector_size) -> cpl::core::Vector {
            return cpl::core::Vector(vector_size);
          },
          py::arg("size"), R"pydoc(
        Create a Vector of given length, initialised with 0's.

        Parameters
        ----------
        size : int
            size of the new Vector

        Returns
        -------
        cpl.core.Vector
            New cpl.core.Vector, length `size`, initialised with 0's

        Raises
        ------
        cpl.core.IllegalInputError
            size is non-positive
        )pydoc")
      .def_static("read", &cpl::core::Vector::read, py::arg("filename"),
                  R"pydoc(
        Read a list of values from an ASCII file and create a Vector

        Parse an input ASCII file values and create a Vector from it
        Lines beginning with a hash are ignored, blank lines also.
        In valid lines the value is preceeded by an integer, which is ignored.

        In addition to normal files, FIFO (see man mknod) are also supported.

        Parameters
        ----------
        filename : cpl.core.std::string
            Name of the input ASCII file

        Returns
        -------
        cpl.core.Vector
            A new Vector with the parsed ASCII file values

        Raises
        ------
        cpl.core.FileIOError
            if the file cannot be read
        cpl.core.BadFileFormatError
            if the file contains no valid lines
        )pydoc")
      .def(
          "dump",
          [](cpl::core::Vector& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(), self.dump(),
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("show") = true,
          R"pydoc(
        Dump a vector contents to a file, stdout or a string.

        Each element is preceded by its index number (starting with 1!) and
        written on a single line.

        Comment lines start with the hash character.

        Parameters
        ----------
        filename : str, optional
            File to dump vector contents to
        mode : str, optional
            Mode to open the file with. Defaults to "w" (write, overwriting the contents of the file if it already exists),
            but can also be set to "a" (append, creating the file if it does not already exist or appending to the end of
            it if it does).
        show : bool, optional
            Send vector contents to stdout. Defaults to True.

        Returns
        -------
        str 
            Multiline string containing the dump of the vector contents.

        Notes
        -----
        In principle a vector can be saved using :py:meth:`dump` re-read using :py:meth:`read`.
        This will however introduce significant precision loss due to the limited
        accuracy of the ASCII representation.
        )pydoc")
      .def_static("load", &cpl::core::Vector::load, py::arg("filename"),
                  py::arg("xtnum"), R"pydoc(
        Load a list of values from a FITS file

        This function loads a vector from a FITS file (``NAXIS`` = 1).

        `xtnum` specifies from which extension the vector should be loaded.
        This could be 0 for the main data section or any number between 1 and N,
        where N is the number of extensions present in the file.

        Parameters
        ----------
        filename : str
            Name of the input file
        xtnum : int
            Extension number in the file (0 for primary HDU)

        Returns
        -------
        cpl.core.Vector
            The loaded vector from the file, at extension xtnum

        Raises
        ------
        cpl.core.IllegalInputError
            if the extension is not valid
        cpl.core.FileIOError
            if the file cannot be read
        cpl.core.UnsupportedModeError
             if the file is too large to be read
        )pydoc")
      .def("save", &cpl::core::Vector::save, py::arg("filename"),
           py::arg("type"), py::arg("plist"), py::arg("mode"), R"pydoc(
        Save a vector to a FITS file

        This function saves a vector to a FITS file (``NAXIS`` = 1). If a property list
        is provided, it is written to the named file before the pixels are written.

        If the image is not provided, the created file will only contain the
        primary header. This can be useful to create multiple extension files.

        The type used in the file can be one of:
        cpl.core.Type.UCHAR  (8 bit unsigned),
        cpl.core.Type.SHORT  (16 bit signed),
        cpl.core.Type.USHORT (16 bit unsigned),
        cpl.core.Type.INT    (32 bit signed),
        cpl.core.Type.FLOAT  (32 bit floating point), or
        cpl.core.Type.DOUBLE (64 bit floating point).
        Use cpl.core.Type.DOUBLE when no loss of information is required.

        Supported output modes are cpl.core.IO.CREATE (create a new file) and
        cpl.core.IO.EXTEND  (append to an existing file)

        If you are in append mode, make sure that the file has writing
        permissions. You may have problems if you create a file in your
        application and append something to it with the umask set to 222. In
        this case, the file created by your application would not be writable,
        and the append would fail.

        Parameters
        ----------
        filename : cpl.core.str
            Name of the file to write
        type : cpl.core.Type
            The type used to represent the data in the file
        plist : cpl.core.Propertylist
            Property list for the output header or NULL
        mode : cpl.core.IO
            The desired output options. Can combine with bitwise or (e.g. cpl.core.IO.CREATE|cpl.core.IO.GZIP)

        Raises
        ------
        cpl.core.IllegalInputError
            if the type or the mode is not supported
        cpl.core.FileNotCreatedError
            if the output file cannot be created
        cpl.core.FileIOError
            if the data cannot be written to the file
        )pydoc")
      .def(
          "copy",
          [](cpl::core::Vector& self) -> cpl::core::Vector {
            auto duplicate = cpl::core::Vector(self.get_size());
            duplicate.copy(self);
            return duplicate;
          },
          R"pydoc(
        Copy the contents of the Vector into a new Vector objects.

        Vectors can also be copied by passing a Vector to the Vector
        constructor.

        Returns
        -------
        cpl.core.Vector
            New Vector containing a copy of the contents of the original.

        See Also
        --------
        cpl.core.Vector: Class for ordered sequences of numbers.
      )pydoc")
      .def("__len__", &cpl::core::Vector::get_size)
      .def("__str__", &cpl::core::Vector::dump)
      .def_property("size", &cpl::core::Vector::get_size,
                    &cpl::core::Vector::set_size,
                    "Number of elements in the vector. Is resizable")
      .def("__iter__",
           [](py::object& self) -> VectorIterator {
             // Instead of trying to call data() and unwrap() from python,
             // use either the iterators, or
             // (If this is implemented, which I'm not sure we need)
             // use the buffer protocol?
             return VectorIterator{self, 0};
           })
      .def("__getitem__",
           [](cpl::core::Vector& self, long index) -> double {
             long real_index = index < 0 ? index + self.get_size() : index;
             if (real_index < 0 || real_index >= self.get_size()) {
               throw py::index_error(std::to_string(index));
             }
             return self.get(real_index);  // Get item at index
           })
      .def("__getitem__",
           [](cpl::core::Vector& self, py::slice slice) -> cpl::core::Vector {
             size_t start, stop, step, slicelength;
             // If slice fails to pass to variables then a python error occured.
             // Throw existing error.
             if (!slice.compute(self.get_size(), &start, &stop, &step,
                                &slicelength)) {
               throw py::error_already_set();
             }
             return self.extract(start, stop - 1, step);
           })
      .def("__setitem__",
           [](cpl::core::Vector& self, long index, double el) -> void {
             long real_index = index < 0 ? index + self.get_size() : index;
             if (real_index < 0 || real_index >= self.get_size()) {
               throw py::index_error(std::to_string(index));
             }
             self.set(real_index, el);
           })
      .def("__setitem__",
           [](cpl::core::Vector& self, py::slice slice,
              std::vector<double> values) -> void {
             size_t start, stop, step, slicelength;
             // If slice fails to pass to variables then a python error occured.
             // Throw existing error.
             if (!slice.compute(self.get_size(), &start, &stop, &step,
                                &slicelength))
               throw py::error_already_set();


             if (slicelength != values.size())
               throw std::out_of_range(
                   "Left and right hand size of slice assignment have "
                   "different sizes");  // Is cast to a python ValueError

             size_t pos = start;
             // Manually set variables using slice information as there is no
             // cpl equivalent
             for (size_t i = 0; i < slicelength; ++i) {
               self.set(pos, values[i]);
               pos += step;
             }
           })
      .def("add", &cpl::core::Vector::add, py::arg("other"), R"pydoc(
        Add a cpl.core.Vector to self

        The other vector must have the same size as the calling vector

        Parameters
        ----------
        other : cpl.core.Vector
            Vector to add
        )pydoc")
      .def("subtract", &cpl::core::Vector::subtract, py::arg("other"), R"pydoc(
        Subtract a cpl.core.Vector from self

        The other vector must have the same size as the calling vector

        Parameters
        ----------
        other : cpl.core.Vector
            Vector to subtract
        )pydoc")
      .def("multiply", &cpl::core::Vector::multiply, py::arg("other"), R"pydoc(
        Multiply another vector with the calling vector, component-wise

        Parameters
        ----------
        other : cpl.core.Vector
            Vector to multiply with
        )pydoc")
      .def("divide", &cpl::core::Vector::divide, py::arg("other"), R"pydoc(
        Divide the calling vector by another vector, element-wise

        If an element in vector `other` is zero, a cpl.core.DivisionByZeroError is thrown.

        Parameters
        ----------
        other : cpl.core.Vector
            Vector to divide by

        Raises
        ------
        cpl.core.IncompatibleInputError
            if the calling vector and `other` have different sizes
        cpl.core.DivisionByZeroError
            if `other` contains an element equal to zero.

        )pydoc")
      .def("cycle", &cpl::core::Vector::cycle, py::arg("shift"), R"pydoc(
        Perform a cyclic shift to the right of the elements of the vector

        A shift of +1 will move the last element to the first, a shift of -1 will
        move the first element to the last, a zero-shift will perform a copy (or
        do nothing in case of an in-place operation).

        A non-integer shift will perform the shift in the Fourier domain. Large
        discontinuities in the vector to shift will thus lead to FFT artifacts
        around each discontinuity.

        Parameters
        ----------
        shift : float
            The number of positions to cyclic right-shift

        Raises
        ------
        cpl.core.UnsupportedModeError
            if the shift is non-integer and FFTW is unavailable
        )pydoc")
      .def("product", &cpl::core::Vector::product, py::arg("other"), R"pydoc(
        Compute the vector dot product of the caller vector and `other`

        Parameters
        ----------
        other : cpl.core.Vector
            Another vector of the same size

        Returns
        -------
        float
            The (non-negative) product

        Raises
        ------
        cpl.core.IncompatibleInputError
            if `other` has a different size from the calling vector
        )pydoc")
      .def(
          "sort",
          [](cpl::core::Vector& self, bool reverse) -> void {
            cpl_sort_direction dir = CPL_SORT_ASCENDING;
            if (reverse) {
              dir = CPL_SORT_DESCENDING;
            }
            self.sort(dir);
          },
          py::arg("reverse") = false, R"pydoc(
        Sort the Vector in place.

        The values are sorted in either ascending or descending order. The sorting
        is done in place, modifying the Vector.

        Parameters
        ----------
        reverse : bool, default False
            If `True` values will be sorted in descending order, otherwise they will
            be sorted in ascending order.

        See Also
        --------
        cpl.core.Vector.sorted : Return a sorted copy of the Vector.

        Notes
        -----
        If two members compare as equal their order in the sorted Vector is undefined.
      )pydoc")
      .def(
          "sorted",
          [](cpl::core::Vector& self, bool reverse) -> cpl::core::Vector {
            cpl_sort_direction dir = CPL_SORT_ASCENDING;
            if (reverse) {
              dir = CPL_SORT_DESCENDING;
            }
            auto output = cpl::core::Vector(self.get_size());
            output.copy(self);
            output.sort(dir);
            return output;
          },
          py::arg("reverse") = false, R"pydoc(
        Return a sorted copy of the Vector.

        The values are sorted in either ascending of descending order. The result
        is returned in a new `cpl.core.Vector`, the original is not modified.

        Parameters
        ----------
        reverse : bool, default False
            If `True` values will be sorted in descending order, otherwise they will
            be sorted in ascending order.

        See Also
        --------
        cpl.core.Vector.sort : Sort the Vector in place.

        Notes
        -----
        If two members compare as equal their order in the sorted Vector is undefined.
      )pydoc")
      .def("fill", &cpl::core::Vector::fill, R"pydoc(
        Fill the Vector with a given value

        Parameters
        ----------
        val : float
            Value used to fill the cpl_vector
        )pydoc")
      .def("sqrt", &cpl::core::Vector::sqrt, R"pydoc(
        Compute the sqrt of a Vector

        The sqrt of each data element is computed and modified.

        Raises
        ------
        cpl.core.IllegalInputError
          An element is negative
        )pydoc")
      .def("binary_search", &cpl::core::Vector::bisect, py::arg("value"),
           R"pydoc(
        In a sorted (ascending) vector find the element closest to the given value

        Bisection is used to find the element.

        If two (neighboring) elements with different values both minimize
        fabs(sorted[index] - key) the index of the larger element is returned.

        If the vector contains identical elements that minimize
        fabs(sorted[index] - key) then it is undefined which element has its index
        returned.

        Use cpl.core.Vector.sort(cpl.core.Sort.ASCENDING) before calling this function
        to ensure the vector is sorted correctly

        Parameters
        ----------
        value : float
            Value to find

        Returns
        -------
        int
          The index that minimizes fabs(sorted[index] - value)

        Raises
        ------
        cpl.core.IllegalInputError
          If the vector is not correctly sorted in ascending order.

        )pydoc")
      .def("extract", &cpl::core::Vector::extract, py::arg("istart"),
           py::arg("istop"), py::arg("istep") = 1,
           R"pydoc(
        Extract a sub-vector from a vector

        Parameters
        ----------
        istart : int
            Start index (from 0 to number of elements - 1), must be less than istop
        istop : int
            Stop  index (from 0 to number of elements - 1), must be greater than istart
        istep : int, optional
            Extract every step element (Currently does not support any value other than the default)

        Returns
        -------
        cpl.core.Vector
            New sub-vector with the values of the requested range

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            if istart is less than 0 or istop is greater than the size of the vector
        cpl.core.IllegalInputError
            if istep is not 1, or istart is not less than istop

        Notes
        -----
        istep only supporting a value of 1 is to be fixed, as is allowing istop to be greater than istart.
        )pydoc")
      .def("minpos", &cpl::core::Vector::get_minpos,
           R"pydoc(
        Get the index of the minimum element of the vector

        Returns
        -------
        int
            The index (0 for first) of the minimum value
        )pydoc")
      .def("maxpos", &cpl::core::Vector::get_maxpos,
           R"pydoc(
        Get the index of the maximum element of the vector

        Returns
        -------
        int
            The index (0 for first) of the maximum value
        )pydoc")
      .def("min", &cpl::core::Vector::get_min,
           R"pydoc(
        Get the minimum of the vector

        Returns
        -------
        float
            The minimum value of the vector
        )pydoc")
      .def("max", &cpl::core::Vector::get_max,
           R"pydoc(
        Get the maximum of the vector

        Returns
        -------
        float
            The maximum value of the vector
        )pydoc")
      .def("sum", &cpl::core::Vector::get_sum,
           R"pydoc(
        Get the sum of the elements of the vector

        Returns
        -------
        float
            The sum of the elements value of the vector
        )pydoc")
      .def("mean", &cpl::core::Vector::get_mean,
           R"pydoc(
        Get the mean of the elements of the vector

        Returns
        -------
        float
            The mean of the elements value of the vector
        )pydoc")
      .def("median", &cpl::core::Vector::get_median,
           R"pydoc(
        Get the median of the elements of the vector

        Returns
        -------
        float
            The median of the elements value of the vector
        )pydoc")
      .def("stdev", &cpl::core::Vector::get_stdev,
           R"pydoc(
        Get the standard deviation of the elements of the vector

        Returns
        -------
        float
            The standard deviation of the elements value of the vector
        )pydoc")
      .def_static(
          "correlate",
          [](const cpl::core::Vector& v1, const cpl::core::Vector& v2,
             size max_shift) -> std::pair<cpl::core::Vector, size> {
            auto xcorr = cpl::core::Vector(2 * max_shift + 1);
            size max_index = xcorr.correlate(v1, v2);
            return std::pair(xcorr, max_index);
          },
          py::arg("v1"), py::arg("v2"), py::arg("max_shift"), R"pydoc(
        Return cross-correlation of two vectors.

        The length of `v2` may not exceed that of `v1`. If the difference in length
        between `v1` and `v2` is less than `max_shift` then this difference must be
        even (if the difference is odd resampling of `v2` may be useful).

        The cross-correlation is in fact the dot product of two unit vectors and
        therefore ranges from -1 to 1.

        The cross-correlation is computed with shifts ranging from `-max_shift`
        to `+max_shift`.

        On success, element i (starting with 0) of the returned `cpl.core.Vector` contains
        the cross-correlation at offset `i - max_shift`.

        If `v1` is longer than `v2`, the first element in `v1` used for the resulting
        cross-correlation is `max(0, shift + (len(v1) - len(v2)) / 2)`.

        Parameters
        ----------
        v1 : cpl.core.Vector
            1st vector to correlate
        v2 : cpl.core.Vector
            2nd vector to correlate
        max_shift : int
            Maximum size of shift to be used when calculating cross correlation.

        Returns
        -------
        cpl.core.Vector
            New Vector of size `2 * max_shift + 1` containing the cross correlation of
            `v1` and `v2` for shifts ranging from `-max_shift` to `+max_shift`. 
        int
            Index of output Vector at which the maximum cross correlation value occurs.

        Raises
        ------
        cpl.core.IllegalInputError
            if `v1` and `v2` have incompatible sizes.

        Notes
        -----
        The cross-correlation is, in absence of rounding errors, commutative only for
        equal-sized vectors, i.e. changing the order of `v1` and `v2` will move element `j`
        in the returned Vector to `2 * max_shift - j` and thus change the index of maximum
        cross correlation from `i` to `2 * max_shift - i`.

        If, in absence of rounding errors, more than one shift would give the maximum
        cross-correlation, rounding errors may cause any one of those shifts to be
        returned. If rounding errors have no effect the index corresponding to the
        shift with the smallest absolute value is returned (with preference given to
        the smaller of two indices that correspond to the same absolute shift).

        Cross-correlation with `max_shift == 0` requires about 8n FLOPs, where
        `n` is the number of elements of `v2`.

        Each increase of `max_shift` by 1 requires about 4n FLOPs more, when all 
        elements of `v2` can be cross-correlated, otherwise the extra cost is about 4m,
        where `m` is the number of elements in `v2` that can be cross-correlated,
        `n - max_shift <= m < n`.

        Example of 1D-wavelength calibration (error handling omitted for brevity):

        .. code-block:: python

            # Dispersion is of type cpl.core.Polynomial
            # The return type of mymodel() and myobservation() is cpl.core.Vector
            model = mymodel(dispersion)
            observed = myobservation()
            vxc, max_index = cpl.core.Vector.correlate(model, observed, max_shift)
            dispersion.shift_1d(0, max_index - max_shift)
        )pydoc")
      .def("filter_lowpass_create", &cpl::core::Vector::filter_lowpass_create,
           py::arg("filter_type"), py::arg("hw"), R"pydoc(
        Apply a low-pass filter to a vector

        This type of low-pass filtering consists in a convolution with a given
        kernel. The chosen filter type determines the kind of kernel to apply for
        convolution.

        Supported kernels are cpl.core.Vector.LowPass.LINEAR and cpl.core.Vector.LowPass.GAUSSIAN.

        In the case of cpl.core.Vector.LowPass.GAUSSIAN, the gaussian sigma used is
        1/sqrt(2). As this function is not meant to be general and cover all
        possible cases, this sigma is hardcoded and cannot be changed.

        The returned signal has exactly as many samples as the input signal.

        Parameters
        ----------
        filter_type : cpl.core.Vector.LowPass
            Type of filter to use
        hw : int
            Filter half-width

        Returns
        -------
        cpl.core.Vector
            The resulting signal

        Raises
        ------
        cpl.core.IllegalInputError
            if filter_type is not supported or if hw is bigger than half the vector size
        )pydoc")
      .def("filter_median_create", &cpl::core::Vector::filter_median_create,
           py::arg("hw"), R"pydoc(
        Apply a 1D median filter of given half-width to a Vector

        This function applies a median smoothing to the caller Vector and returns a
        new Vector containing a median-smoothed version of the input.

        The returned Vector has exactly as many samples as the input one. The
        outermost hw values are copies of the input, each of the others is set to
        the median of its surrounding 1 + 2 * hw values.

        For historical reasons twice the half-width is allowed to equal the
        Vector length, although in this case the returned Vector is simply a
        duplicate of the input one.

        If different processing of the outer values is needed or if a more general
        kernel is needed, then :py:meth:`cpl.core.Image.filter_mask` can be called instead with
        cpl.core.Filter.MEDIAN and the 1D-image input wrapped around self.

        Parameters
        ----------
        hw : int
            Filter half-width

        Returns
        -------
        cpl.core.Vector
            The filtered vector.

        Raises
        ------
        cpl.core.IllegalInputError
            if hw is negative or bigger than half the vector

        )pydoc")
      .def("add_scalar", &cpl::core::Vector::add_scalar, py::arg("value"),
           R"pydoc(
        Elementwise addition of a scalar to a vector

        Add a number to each element of the vector.

        Parameters
        ----------
        value : float
            Number to add
        )pydoc")
      .def("subtract_scalar", &cpl::core::Vector::subtract_scalar,
           py::arg("value"), R"pydoc(
        Elementwise subtraction of a scalar to a vector

        Subtract a number to each element of the vector.

        Parameters
        ----------
        value : float
            Number to subtract
        )pydoc")
      .def("multiply_scalar", &cpl::core::Vector::multiply_scalar,
           py::arg("factor"), R"pydoc(
        Elementwise multiplication of a vector with a scalar

        Multiply each element of the vector with a number.

        Parameters
        ----------
        factor : float
            Number to multiply with
        )pydoc")
      .def("divide_scalar", &cpl::core::Vector::divide_scalar,
           py::arg("divisor"), R"pydoc(
        Elementwise division of a vector with a scalar

        Divide each element of the vector with a number.

        Parameters
        ----------
        divisor : float
            Non-zero number to divide with
        )pydoc")
      .def("logarithm", &cpl::core::Vector::logarithm, py::arg("base"), R"pydoc(
        Compute the element-wise logarithm.

        The base and all the vector elements must be positive and the base must be
        different from 1.

        Parameters
        ----------
        base : float
            Logarithm base.

        Raises
        ------
        cpl.core.IllegalInputError
            if base is negative or zero or if one of the vector values is negative or zero
        cpl.core.DivisionByZeroError
            if a division by zero occurs
        )pydoc")
      .def("exponential", &cpl::core::Vector::exponential, py::arg("base"),
           R"pydoc(
        Compute the exponential of all vector elements.

        If the base is zero all vector elements must be positive and if the base is
        negative all vector elements must be integer.

        Parameters
        ----------
        base : float
            Exponential base.

        Raises
        ------
        cpl.core.IllegalInputError
            base and v are not as requested
        cpl.core.DivisionByZeroError
            if one of the values is negative or 0
        )pydoc")
      .def("power", &cpl::core::Vector::power, py::arg("exponent"), R"pydoc(
        Compute the power of all vector elements.

        If the exponent is negative all vector elements must be non-zero and if
        the exponent is non-integer all vector elements must be non-negative.

        Parameters
        ----------
        exponent : float
            Constant exponent.

        Raises
        ------
        cpl.core.IllegalInputError
            if v and exponent are not as requested
        cpl.core.DivisionByZeroError
            if one of the values is 0

        Notes
        -----
        Following the behaviour of C99 pow() function, this function sets 0^0 = 1.
        )pydoc")
      .def_static(
          "kernel_profile",
          [](cpl_kernel type, double radius, size ksize) -> cpl::core::Vector {
            auto kernel = cpl::core::Vector(ksize);
            kernel.fill_kernel_profile(type, radius);
            return kernel;
          },
          py::arg("type"), py::arg("radius"), py::arg("size"), R"pydoc(
        Return a Vector containing a kernel profile.

        A number of predefined kernel profiles are available:
        - cpl.core.Kernel.DEFAULT: default kernel, currently cpl.core.Kernel.TANH
        - cpl.core.Kernel.TANH: Hyperbolic tangent
        - cpl.core.Kernel.SINC: Sinus cardinal
        - cpl.core.Kernel.SINC2: Square sinus cardinal
        - cpl.core.Kernel.LANCZOS: Lanczos2 kernel
        - cpl.core.Kernel.HAMMING: Hamming kernel
        - cpl.core.Kernel.HANN: Hann kernel
        - cpl.core.Kernel.NEAREST: Nearest neighbor kernel (1 when dist < 0.5, else 0)

        Parameters
        ----------
        type : cpl.core.Kernel
            Type of kernel profile.
        radius : float
            Radius of the profile in pixels
        size : int
            Size of the kernel profile in pixels.

        Returns
        -------
        cpl.core.Vector
            Vector of length `size` containing the calculated kernel values.

        Raises
        ------
        cpl.core.IllegalInputError
            if `radius` is non-positive, or in case of the `cpl.core.Kernel.TANH` profile if `size` exceeds 32768
        )pydoc")
      .def_static(
          "fit_gaussian",
          [](
              // Pybind doesn't seem to want to work with
              // optional<reference_wrapper<const Vector>>
              py::object x, py::object y, py::object y_sigma, unsigned fit_pars,
              std::optional<double> x0, std::optional<double> sigma,
              std::optional<double> area,
              std::optional<double> offset) -> py::object {
            py::object NamedTuple =
                py::module::import("collections").attr("namedtuple");
            py::object fit_gaussian_tuple = NamedTuple(
                "FitGaussianResult", py::cast(std::vector<std::string>(
                                         {"x0", "sigma", "area", "offset",
                                          "mse", "red_chisq", "covariance"})));


            auto vec_x_opt = as_cpl_vec(x);
            auto vec_y_opt = as_cpl_vec(y);
    // FIXME: The commented out line below is to be used when CPL
    //        supports x_sigma
#if 0
            auto vec_x_sigma = as_cpl_vec(x_sigma);
#endif
            auto vec_y_sigma = as_cpl_vec(y_sigma);

            // X & Y Vectors aren't optional, so handle that case:
            if (!vec_x_opt.second.has_value() ||
                !vec_y_opt.second.has_value()) {
              throw py::value_error(
                  "Expected cpl.core.Vector or list of double, found None "
                  "(Argument x or y)");
            }
            auto vec_x = *(vec_x_opt.second);
            auto vec_y = *(vec_y_opt.second);

            py::tuple res = py::cast(cpl::core::Vector::fit_gaussian(
                vec_x, vec_y, (cpl_fit_mode)fit_pars, vec_y_sigma.second, x0,
                sigma, area, offset));
            return fit_gaussian_tuple(*res);
          },
          py::arg("x"), py::arg("y"), py::arg("y_sigma"), py::arg("fit_pars"),
          py::arg("x0").none(true) = py::none(),
          py::arg("sigma").none(true) = py::none(),
          py::arg("area").none(true) = py::none(),
          py::arg("offset").none(true) = py::none(),
          R"pydoc(
            Apply a 1d gaussian fit.

            This function fits to the input vectors a 1d gaussian function of the form

            .. math::

              f(x) =  \mathrm{area} / \sqrt{2 \pi \sigma^2} * \exp(-(x - x0)^2 / (2 \sigma^2)) + \mathrm{offset}

            where `area` > 0, by minimizing chi^2 using a Levenberg-Marquardt algorithm.

            The values to fit are read from the input vector `x`.

            The diagonal elements (the variances) are guaranteed to be positive.

            Occasionally, the Levenberg-Marquardt algorithm fails to converge to a set of
            sensible parameters. In this case (and only in this case), a
            cpl.core.ContinueError is set. To allow the caller to recover from this
            particular error.

            Parameters
            ----------
            x : cpl.core.Vector
                Positions to fit
            y : cpl.core.Vector
                The N values to fit.
            y_sigma : cpl.core.Vector
                Uncertainty (one sigma, gaussian errors assumed) associated with y
            fit_pars : cpl.core.FitMode
                Specifies which parameters participate in the fit (any other parameters will be held constant).
                Possible values are cpl.core.FitMode.CENTROID, cpl.core.FitMode.STDEV, cpl.core.FitMode.AREA,
                cpl.core.FitMode.OFFSET and cpl.core.FitMode.ALL, and any bitwise combination of these (using
                bitwise OR).
            x0 : double, optional
                Preset center of best fit gaussian if cpl.core.FitMode.CENTROID is not used in fit_pars.
                Value is unused otherwise.
            sigma : doublee, optional
                Width of best fit gaussian if cpl.core.FitMode.STDEV is not used in fit_pars.
                Value is unused otherwise.
            area : doublee, optional
                Area of gaussian if cpl.core.FitMode.AREA is not used in fit_pars.
                Value is unused otherwise.
            offset : doublee, optional
                Fitted background level if cpl.core.FitMode.OFFSET is not used in fit_pars.
                Value is unused otherwise.
            Returns
            -------
            NamedTuple(float, float, float, float, float, float, cpl.core.Matrix)
                A FitGaussianResult NamedTuple with the following elements:
                  x0 : float
                      Center of best fit gaussian.
                  sigma : float
                      Width of best fit gaussian. A positive number on success.
                  area : float
                      Area of gaussian. A positive number on succes.
                  offset : float
                      Fitted background level.
                  mse : float
                      the mean squared error of the best fit
                  red_chisq : float
                      the reduced chi-squared of the best fit. None if `y_sigma` is not passed
                  covariance : cpl.core.Matrix
                      The formal covariance matrix of the best fit, On success the diagonal
                      terms of the covariance matrix are guaranteed to be positive.
                      However, terms that involve a constant parameter (as defined by the input
                      array `evaluate_derivatives`) are always set to zero. None if `y_sigma`
                      is not passed

            Raises
            ------
            cpl.core.InvalidTypeError
                if the specified fit_pars is not a bitwise combination of the allowed values (e.g. 0 or 1).
            cpl.core.IncompatibleInputError
                if the sizes of any input vectors are different, or if the computation of reduced chi square or covariance is requested, but sigma_y is not provided.
            cpl.core.IllegalInputError
                if any input noise values, sigma or area is non-positive, or if chi square computation is requested and there are less than 5 data points to fit, or if an
                initial value is required for x0, sigma, area or offset when a fit_pars mode is not present.
            cpl.core.IllegalOutputError
                if memory allocation failed.
            cpl.core.ContinueError
                if the fitting algorithm failed.
            cpl.core.SingularMatrixError
                if the covariance matrix could not be calculated.
            )pydoc")
      .def("__eq__", &cpl::core::Vector::operator==)
      .def("__eq__",
           [](cpl::core::Vector& /* self */, py::object /* other */) -> bool {
             return false;
           })
      .def("__ne__", &cpl::core::Vector::operator!=)
      .def("__ne__",
           [](cpl::core::Vector& /* self */, py::object /* other */) -> bool {
             return true;
           })
      .def("__repr__",
           [](cpl::core::Vector& self) -> std::string {
             std::ostringstream ss;
             ss << "cpl.core.Vector([";
             size data_size = self.get_size();
             double* data = self.data();

             // In order to avoid inordinately long output
             // and crashes, truncate to 7 elements (middle
             // element becomes "..." when truncated)
             if (data_size > 7) {
               ss << data[0] << ", ";
               ss << data[1] << ", ";
               ss << data[2] << ", ";
               ss << "..., ";
               ss << data[data_size - 3] << ", ";
               ss << data[data_size - 2] << ", ";
               ss << data[data_size - 1];
             } else {
               for (size i = 0; i < data_size; ++i) {
                 if (i != 0) {
                   ss << ", ";
                 }
                 ss << data[i];
               }
             }
             ss << "])";
             return ss.str();
           })
      .def("__array__",
           [](cpl::core::Vector& self,
              const py::kwargs& /* unused */) -> py::array_t<double> {
             return py::array_t<double>(
                 py::buffer_info(self.data(),
                                 sizeof(double),  // itemsize
                                 py::format_descriptor<double>::format(),
                                 1,                                   // ndim
                                 std::vector<size>{self.get_size()},  // shape
                                 std::vector<size>{sizeof(double)}    // strides
                                 ));
           })
      // Due to being deprecated in CPL, these are not bound
      // .def_static("new_lss_kernel", &cpl::core::Vector::new_lss_kernel)
      // .def("convolve_symmetric", &cpl::core::Vector::convolve_symmetric)
      ;

  py::class_<VectorIterator> vector_iterator(m, "VectorIterator");
  vector_iterator
      .def("__next__",
           [](VectorIterator& self) -> double {
             auto container = py::cast<cpl::core::Vector&>(self.vector);
             if (self.index >= container.get_size()) {
               throw py::stop_iteration();
             }
             double next_item = container.get(self.index);
             ++self.index;
             return next_item;
           })
      .def("__iter__", [](VectorIterator& self) -> VectorIterator {
        return VectorIterator{self};
      });


  py::class_<cpl::core::Bivector, std::shared_ptr<cpl::core::Bivector>>
      bivector(m, "Bivector");

  bivector.doc() = R"mydelim(
        Class for pairs of ordered sequences of numbers.

        A `cpl.core.Bivector` is composed of two `cpl.core.Vectors` of the same size.
        It can be used to store 1d functions, with the x and y positions of the samples,
        offsets in x and y or simply positions in an image.
        
        These Vectors are stored in properties `x` and `y`, however they can also be
        accessed using 0 and 1 indexes (and by extension through `__iter__`) for x and y
        respectfully.

        A Bivector can be created from any iterable object that containins two
        equal length sequences of floating point numbers. Examples include tuples
        containing two lists of numbers, lists containing two Vectors, and existing
        Bivectors.

        A Bivector can also be created using the zeros class method.

        Parameters
        ----------
        data : iterable of iterables of floats
            An iterable object which yields two items, both of which are iterables
            yielding an equal number of floating point values. 

        See Also
        --------
        cpl.core.Vector: Class for ordered sequences of numbers.
        cpl.core.Bivector.zeros: Create a Bivector of given length, initialised with 0's.

        Examples
        --------
        >>> bivector_list_of_tuples = cpl.core.Bivector([(1, 3, 5), (2, 4, 6)])
        ... bivector_tuple_of_lists = cpl.core.Bivector(([1, 2, 3], [4, 6, 8]))
        ... bivector_copy = cpl.core.Bivector(bivector_tuple_of_lists)
        ... vector_x = cpl.core.Vector((1, 2, 3))
        ... vector_y = cpl.core.Vector.zeros(3)        
        ... bivector_vectors = cpl.core.Bivector((vector_x, vector_y))
        ... bivector_zeros = cpl.core.Bivector.zeros(5)
    )mydelim";

  bivector.def(py::init<const cpl::core::Bivector&>())
      .def(py::init(&py_bivec_constructor), py::arg("data"))
      .def_static(
          "zeros",
          [](size bivector_size) -> cpl::core::Bivector {
            return cpl::core::Bivector(bivector_size);
          },
          py::arg("size"), R"pydoc(
            Create a Bivector of given length, initialised with 0's.

            Parameters
            ----------
            size : int
                size of the new Bivector

            Returns
            -------
            cpl.core.Biector
                New cpl.core.Bivector, length `size`, initialised with 0's

            Raises
            ------
            cpl.core.IllegalInputError
                size is non-positive            
          )pydoc")
      .def(
          "copy",
          [](cpl::core::Bivector& self) -> cpl::core::Bivector {
            auto duplicate = cpl::core::Bivector(self.get_size());
            duplicate.copy(self);
            return duplicate;
          },
          R"pydoc(
        Copy the contents of the Bivector into a new Bivector object.

        Bivectors can also be copied by passing a Bivector to the
        Bivector constructor.

        Returns
        -------
        cpl.core.Bivector
            New Bivector containing a copy of the contents of the original.

        See Also
        --------
        cpl.core.Bivector : Class for pairs of ordered sequences of numbers.
      )pydoc")
      .def(
          "dump",
          [](cpl::core::Bivector& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(), self.dump(),
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("show") = true,
          R"pydoc(
        Dump a vector contents to a file, stdout or a string.

        Each element is preceded by its index number (starting with 1!) and
        written on a single line.

        Comment lines start with the hash character.

        Parameters
        ----------
        filename : str, optional
            File to dump bivector contents to
        mode : str, optional
            Mode to open the file with. Defaults to "w" (write, overwriting the contents of the file if it already exists),
            but can also be set to "a" (append, creating the file if it does not already exist or appending to the end of
            it if it does).
        show : bool, optional
            Send bivector contents to stdout. Defaults to True.

        Returns
        -------
        str 
            Multiline string containing the dump of the bivector contents.

        Notes
        -----
        In principle a bivector can be saved using :py:meth:`dump` re-read using :py:meth:`read`.
        This will however introduce significant precision loss due to the limited
        accuracy of the ASCII representation.
        )pydoc")
      .def_static("read", &cpl::core::Bivector::read, py::arg("filename"),
                  R"pydoc(
        Read a list of values from an ASCII file and create a cpl_bivector

        The input ASCII file must contain two values per line.

        Two columns of numbers are expected in the input file.

        In addition to normal files, FIFO (see man mknod) are also supported.

        Parameters
        ----------
        filename : cpl.core.std::string
            Name of the input ASCII file

        Returns
        -------
        cpl.core.Bivector
            New Bivector with the values writen in the input ASCII file
        Raises
        ------
        cpl.core.FileIOError
            if the file cannot be read
        )pydoc")
      .def_property_readonly(
          "size", &cpl::core::Bivector::get_size,
          "Length of the bivector and in turn the length of x and y")
      .def("__len__", [](cpl::core::Bivector& /* self */) -> size { return 2; })
      .def("__str__", &cpl::core::Bivector::dump)
      .def(
          "__getitem__",
          [](cpl::core::Bivector& self, int index) -> cpl::core::Vector* {
            switch (index) {
              case 0: return &self.get_x();
              case 1:
              case -1: return &self.get_y();
              default: throw py::index_error();
            }
          },
          py::return_value_policy::reference_internal)
      .def("__setitem__",
           [](cpl::core::Bivector& self, int index,
              py::object vec) -> cpl::core::Vector {
             switch (index) {
                 // These will accept python double iterables (which cpl vectors
                 // are) (There's no special case for cpl vectors) It copies the
                 // data to a new cpl_vector (because set_x/set_y require a
                 // newly &&moved vector)
               case 0: return self.set_x(py_vec_constructor(vec));
               case 1:
               case -1: return self.set_y(py_vec_constructor(vec));
               default: throw py::index_error();
             }
           })
      .def_property(
          "x",
          py::cpp_function(
              [](const cpl::core::Bivector& self) -> const cpl::core::Vector* {
                return &self.get_x();
              },
              py::return_value_policy::reference_internal),
          [](cpl::core::Bivector& self, py::object vec) -> cpl::core::Vector {
            return self.set_x(py_vec_constructor(vec));
          },
          "x vector")
      .def_property(
          "y",
          py::cpp_function(
              [](const cpl::core::Bivector& self) -> const cpl::core::Vector* {
                return &self.get_y();
              },
              py::return_value_policy::reference_internal),
          [](cpl::core::Bivector& self, py::object vec) -> cpl::core::Vector {
            return self.set_y(py_vec_constructor(vec));
          },
          "y vector")
      .def(
          "interpolate_linear",
          [](cpl::core::Bivector& self,
             cpl::core::Vector& xout) -> cpl::core::Bivector {
            auto yout = cpl::core::Vector(xout.get_size());
            auto fout = cpl::core::Bivector(std::move(xout), std::move(yout));
            fout.interpolate_linear(self);
            return fout;
          },
          py::arg("xout"), R"pydoc(
        Linear interpolation of a 1D-function

        Here `self` is interpreted as samples of a one dimensional function, with `x`
        containing abscissa values and `y` containing the corresponding ordinate values.
        The argument to this function, `xout`, is a `cpl.core.Vector` containing a set
        of abscissa values for which interpolated ordinate values are to be calculated.
        Linear interpolation is used to calculate the new ordinate values and the
        result is returned in a new `cpl.core.Bivector` object containing a copy of
        `xout` and the corresponding interpolated ordinate values.

        For each abscissa point in `xout`, `self.x` must either have two neigboring 
        abscissa points such that `self.x[i] < xout[j] < self.x[i+1]`, or a single
        identical abscissa point, such that `self.x[i] == xout[j]`. This is ensured
        by having monotonically increasing abscissa points in both `self.x` and `xout`,
        and by `min(self.x) <= min(xout)` and `max(xout) < max(self.x)`. However, for
        efficiency reasons (since `self.x` can be very long) the monotonicity is only
        verified to the extent necessary to actually perform the interpolation. This
        input requirement implies that extrapolation is not allowed.

        Parameters
        ----------
        xout : cpl.core.Vector
            abcissa points to interpolate the ordinate values from `self` to.

        Returns
        -------
        cpl.core.Bivector
              New Bivector containing the abscissa and ordinate values of the
              interpolated function as `x` and `y` attributes.

        Raises
        ------
        cpl.core.DataNotFoundError
            if xout has an endpoint which is out of range
        cpl.core.IllegalInputError
            if the monotonicity requirement on the 2 input abcissa Vectors is not met.
        )pydoc")
      .def(
          "sort",
          [](cpl::core::Bivector& self, bool reverse,
             cpl_sort_mode mode) -> void {
            cpl_sort_direction dir = CPL_SORT_ASCENDING;
            if (reverse) {
              dir = CPL_SORT_DESCENDING;
            }
            self.sort(self, dir, mode);
          },
          py::arg("reverse") = false, py::arg("mode") = CPL_SORT_BY_X, R"pydoc(
        Sort the Bivector in place.

        The values are sorted in either ascending or descending order, using either
        `x` or `y` as the key. The sorting is done in place, modifying the Bivector.

        Parameters
        ----------
        reverse : bool, default False
            If `True` values will be sorted in descending order, otherwise they will
            be sorted in ascending order.
        mode : cpl.core.SortMode
            `cpl.core.SortMode.BY_X` to sort by the values in `x`, or
            `cpl.core.SortMode.BY_Y` to sort by the values in `y`.

        Raises
        ------
        TypeError
            if `mode` is neither `cpl.core.SortMode.BY_X` or `cpl.core.SortMode.BY_Y`

        See Also
        --------
        cpl.core.Bivector.sorted : Return a sorted copy of the Bivector.

        Notes
        -----
        If two members compare as equal their order in the sorted Bivector is undefined.
      )pydoc")
      .def(
          "sorted",
          [](cpl::core::Bivector& self, bool reverse,
             cpl_sort_mode mode) -> cpl::core::Bivector {
            cpl_sort_direction dir = CPL_SORT_ASCENDING;
            if (reverse) {
              dir = CPL_SORT_DESCENDING;
            }
            auto output = cpl::core::Bivector(self.get_size());
            output.sort(self, dir, mode);
            return output;
          },
          py::arg("reverse") = false, py::arg("mode") = CPL_SORT_BY_X, R"pydoc(
        Return a sorted copy of the Bivector.

        The values are sorted in either ascending or descending order, using either
        `x` or `y` as the key. The result is returned in a new `cpl.core.Bivector`,
        the original is not modified.

        Parameters
        ----------
        reverse : bool, default False
            If `True` values will be sorted in descending order, otherwise they will
            be sorted in ascending order.
        mode : cpl.core.SortMode
            `cpl.core.SortMode.BY_X` to sort by the values in `x`, or
            `cpl.core.SortMode.BY_Y` to sort by the values in `y`.

        Raises
        ------
        TypeError
            if `mode` is neither `cpl.core.SortMode.BY_X` or `cpl.core.SortMode.BY_Y`

        See Also
        --------
        cpl.core.Bivector.sort : Sort the Bivector in place.

        Notes
        -----
        If two members compare as equal their order in the sorted Bivector is undefined.
      )pydoc")
      .def("__eq__", &cpl::core::Bivector::operator==)
      .def("__eq__",
           [](cpl::core::Bivector& /* self */, py::object /* other */) -> bool {
             return false;
           })
      .def("__repr__", [](cpl::core::Bivector& self) -> std::string {
        py::object py_repr_builtin =
            py::module::import("builtins").attr("repr");
        std::ostringstream ss;
        double* data = self.get_x_data();
        size data_size = self.get_size();
        ss << "cpl.core.Bivector([";

        // In order to avoid inordinately long output
        // and crashes, truncate to 7 elements (middle
        // element becomes "..." when truncated)
        if (data_size > 7) {
          ss << data[0] << ", ";
          ss << data[1] << ", ";
          ss << data[2] << ", ";
          ss << "..., ";
          ss << data[data_size - 3] << ", ";
          ss << data[data_size - 2] << ", ";
          ss << data[data_size - 1];
          ss << "], [";
          data = self.get_y_data();
          ss << data[0] << ", ";
          ss << data[1] << ", ";
          ss << data[2] << ", ";
          ss << "..., ";
          ss << data[data_size - 3] << ", ";
          ss << data[data_size - 2] << ", ";
          ss << data[data_size - 1] << "";
        } else {
          for (size i = 0; i < data_size; ++i) {
            if (i != 0) {
              ss << ", ";
            }
            ss << data[i];
          }
          ss << "], [";
          data = self.get_y_data();
          for (size i = 0; i < data_size; ++i) {
            if (i != 0) {
              ss << ", ";
            }
            ss << data[i];
          }
        }
        ss << "])";
        return ss.str();
      });
}

cpl::core::Vector
py_vec_constructor(py::iterable iterable)
{
  py::object builtins = py::module::import("builtins");
  py::object py_len_builtin = builtins.attr("len");

  size n;
  try {
    n = py::cast<size>(py_len_builtin(iterable));
  }
  catch (...) {
    throw py::type_error("expected type with .len(): int");
  }

  // Performance improvement: Is there a way to use Vector::wrap()
  // without the .data() of the STL vector being free'd?
  // i.e. can I move the data out of the STL vector?
  cpl::core::Vector v(n);
  size i = 0;
  for (auto elem : iterable) {
    try {
      v.set(i, py::cast<double>(elem));
    }
    catch (const py::cast_error& /* unused */) {
      throw py::type_error("expected iterable over floats");
    }
    ++i;
  }
  return v;
}

std::pair<as_cpl_vec_types::storage_ty, as_cpl_vec_types::return_ty>
as_cpl_vec(py::object double_list)
{
  using namespace as_cpl_vec_types;
  if (double_list.is_none()) {
    return std::make_pair<storage_ty, return_ty>({}, {});
  }

  try {
    // Case: The cpl::core::Vector already exists, wrapped by a
    // cpl.core.Vector class. Pybind can unwrap it for us:

    // Casting to a Vector* (as opposed to a Vector&) is required for pybind
    // to NOT create a temporary Vector.

    // TODO: Deleter that copies data from the vector back to the python obj

    cpl::core::Vector* already_created =
        py::cast<cpl::core::Vector*>(double_list);
    return std::make_pair<storage_ty, return_ty>({}, {*already_created});
  }
  catch (const py::cast_error& /*unused */) {
    try {
      // Case: The input isn't a cpl::core::Vector, so we create one
      // using the constructor that takes a  a sized iterable

      unique_ty storage(
          new cpl::core::Vector(std::move(py_vec_constructor(double_list))),
          [](cpl::core::Vector* to_delete) -> void { delete to_delete; });

      // With clang: as soon as this returns it deletes the created vector. Need
      // to fix
      return std::make_pair<as_cpl_vec_types::storage_ty,
                            as_cpl_vec_types::return_ty>(std::move(storage),
                                                         *(storage.get()));
    }
    catch (const py::cast_error& /* unused */) {
      throw py::type_error("Expected cpl.core.Vector or a sized Iterable");
    }
  }
}

cpl::core::Bivector
py_bivec_constructor(py::iterable tuple)
{
  py::object builtins = py::module::import("builtins");
  py::object py_next_builtin = builtins.attr("next");
  py::object py_iter_builtin = builtins.attr("iter");
  py::object py_len_builtin = builtins.attr("len");

  size n;
  try {
    n = py::cast<size>(py_len_builtin(tuple));
    if (n < 2) {
      throw std::runtime_error("Bivector construction requires 2 vectors");
    }
  }
  catch (...) {
    throw py::type_error("Expected tuple of size 2");
  }

  py::object iter = py_iter_builtin(tuple);

  auto x_vec = py_vec_constructor(py_next_builtin(iter));
  auto y_vec = py_vec_constructor(py_next_builtin(iter));

  return cpl::core::Bivector(std::move(x_vec), std::move(y_vec));
}

std::pair<as_cpl_bivec_types::storage_ty, as_cpl_bivec_types::return_ty>
as_cpl_bivec(py::object tuple)
{
  using namespace as_cpl_bivec_types;
  if (tuple.is_none()) {
    return std::make_pair<storage_ty, return_ty>({}, {});
  }

  try {
    // Case: The cpl::core::Bivector already exists, wrapped by a
    // cpl.core.Bivector class. Pybind can unwrap it for us:

    // Casting to a Bivector* (as opposed to a Bivector&) is required for pybind
    // to NOT create a temporary Bivector.

    // TODO: Deleter that copies data from the Bivector back to the python obj

    cpl::core::Bivector* already_created =
        py::cast<cpl::core::Bivector*>(tuple);
    return std::make_pair<storage_ty, return_ty>({}, {*already_created});
  }
  catch (const py::cast_error& /*unused */) {
    try {
      // Case: The input isn't a cpl::core::Bivector, so we create one
      // using the constructor that takes a  a sized iterable
      unique_ty storage(
          new cpl::core::Bivector(std::move(py_bivec_constructor(tuple))),
          [](cpl::core::Bivector* to_delete) -> void { delete to_delete; });

      return std::make_pair<storage_ty, return_ty>(std::move(storage),
                                                   *(storage.get()));
    }
    catch (const py::cast_error& /* unused */) {
      throw py::type_error(
          "Expected cpl.core.Bivector or a 2-tuple of sized Iterable");
    }
  }
}
