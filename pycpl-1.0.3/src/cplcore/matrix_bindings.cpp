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

#include "matrix_bindings.hpp"

#include <cstddef>
#include <filesystem>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <pybind11/eval.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "cplcore/matrix.hpp"
#include "dump_handler.hpp"
#include "path_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive

namespace py = pybind11;

using size = cpl::core::size;

/**
 * Adapted from image_bindings.cpp image_from_python_matrix
 * TODO: Use a common constructor function like this for Image, Mask & Matrix.
 */
cpl::core::Matrix
matrix_from_python_matrix(py::object matrix)
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
  cpl::core::Matrix new_matrix = cpl::core::Matrix(height, width);

  // Set all pixels from the first iterable
  width_iter = py_iter_builtin(row);
  for (size x = 0; x < width; ++x) {
    py::object pixel = py_next_builtin(width_iter);

    new_matrix.set(0, x, py::cast<double>(pixel));
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

      new_matrix.set(y, x, py::cast<double>(pixel));
    }
  }

  return new_matrix;
}

enum svd_threshold_mode
{
  SVD_THRESHOLD_MODE_EPSILON,
  SVD_THRESHOLD_MODE_SIZE,
  SVD_THRESHOLD_MODE_USER
};

void
bind_matrix(py::module& m)
{
  py::class_<cpl::core::Matrix, std::shared_ptr<cpl::core::Matrix>>
      matrix_class(m, "Matrix");
  // In C++ there is the Matrix::iterator class,
  // But in python there is an auto-generated iterator of Matrix
  // yielding these MatrixRow classes.
  py::class_<cpl::core::Matrix::iterator> matrix_iterator(m, "MatrixRow");

  py::enum_<svd_threshold_mode>(matrix_class, "ThresholdMode")
      .value("EPSILON", SVD_THRESHOLD_MODE_EPSILON,
             "use machine DBL_EPSILON as the cutoff factor")
      .value("SIZE", SVD_THRESHOLD_MODE_SIZE,
             "compute the cutoff factor as 10*DBL_EPSILON*max(N, M)")
      .value("USER", SVD_THRESHOLD_MODE_USER,
             "use user-defined value as the cutoff factor");

  matrix_class.doc() = R"pydoc(
        This class provides the ability to create and interface with cpl_matrix.
        The elements of a cpl_matrix with M rows and N columns are counted from
        0,0 to M-1,N-1. The matrix element 0,0 is the one at the upper left
        corner of a matrix.

        The CPL matrix functions work properly only in the case the matrices
        elements do not contain garbage (such as NaN or infinity).

        Parameters
        ----------
        data : iterable of floats
          A 1d or 2d iterable containing matrix data to copy from. Any iterable should be compatible
          as long as it implements python's buffer protocol and only contains values of type float
          If a 1d iterable is given, `rows` must also be given to properly split the data into matrix rows.
        rows : int, optional
          Width of the new matrix. This will split `data` into `rows` number of rows to initialise the
          the new matrix. Should only be given if `data` is 1d, otherwise a ValueError exception is thrown.
    )pydoc";

  matrix_class
      .def(py::init([](py::iterable data,
                       std::optional<size> rows) -> cpl::core::Matrix {
             py::array input_arr;
             try {
               // Convert arg to double numpy array
               input_arr = (py::array)data;
             }
             catch (const py::cast_error& /* unused */) {
               throw cpl::core::IllegalInputError(
                   PYCPL_ERROR_LOCATION,
                   std::string(
                       "expected numpy compatible iterable of doubles, not ") +
                       data.get_type().attr("__name__").cast<std::string>());
             }
             py::buffer_info info = input_arr.request();

             if (rows.has_value()) {
               // 1d data if rows given. Get number of columns based on arr size
               // and rows
               if (info.ndim != 1) {
                 std::ostringstream err_msg;
                 err_msg
                     << "expected 1-dimensional buffer if rows is given, not "
                     << info.shape.size() << "-dimensional buffer";
                 throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                                    err_msg.str());
               }
               if (rows.value() == 0) {
                 throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                                    "rows cannot be 0");
               }
               size columns = input_arr.size() / rows.value();
               return cpl::core::Matrix(rows.value(), columns,
                                        input_arr.cast<std::vector<double>>());

             } else {
               if (info.ndim != 2) {  // 2d data if rows not given
                 std::ostringstream err_msg;
                 err_msg
                     << "expected 2-dimensional buffer if rows not given, not "
                     << info.shape.size() << "-dimensional buffer";
                 throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                                    err_msg.str());
               }
               return matrix_from_python_matrix(data);
             }
           }),
           py::arg("data"), py::arg("rows") = py::none())
      .def_static(
          "zeros",
          [](size rows, size columns) -> cpl::core::Matrix {
            return cpl::core::Matrix(rows, columns);
          },
          py::arg("rows"), py::arg("columns"), R"docstring(
      Create an matrix of columns x rows dimensions, all 0’s

      Parameters
      ----------
      rows : int
          number of rows in the new matrix
      columns : int
          number of columns in the new matrix

      Returns
      -------
      cpl.core.Matrix
          New columns x rows matrix initialised with all 0’s
      )docstring")
      .def(
          "dump",
          [](cpl::core::Matrix& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(), self.dump(),
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("show") = true,
          R"pydoc(
            Dump the matrix contents to a file, stdout or a string.

            This function is intended just for debugging. It just prints the elements of a matrix, ordered in rows and columns
            to the file path specified by `filename`. 
            If a `filename` is not specified, output goes to stdout (unless `show` is False).

            Parameters
            ----------
            filename : str, optional
                File to dump matrix contents to
            mode : str, optional
                Mode to open the file with. Defaults to "w" (write, overwriting the contents of the file if it already exists),
                but can also be set to "a" (append, creating the file if it does not already exist or appending to the end of
                it if it does).
            show : bool, optional
                Send matrix contents to stdout. Defaults to True.

            Returns
            -------
            str 
                Multiline string containing the dump of the matrix contents.

          )pydoc")
      .def_property_readonly("height", &cpl::core::Matrix::get_nrow)
      .def_property_readonly("width", &cpl::core::Matrix::get_ncol)
      .def_property_readonly(
          "shape",
          [](const cpl::core::Matrix& self) -> py::tuple {
            return py::make_tuple(self.get_ncol(), self.get_nrow());
          },
          "tuple(int,int) : Matrix shape in the format (columns, rows)")
      .def(py::init<const cpl::core::Matrix&>())
      .def("extract_diagonal", &cpl::core::Matrix::extract_diagonal,
           py::arg("diagonal"), R"pydoc(
        Extract a matrix diagonal.

        If a MxN matrix is given in input, the extracted diagonal is a Mx1
        matrix if :math:`N >= M`, or a 1xN matrix if :math:`N < M`. The diagonal number is
        counted from 0, corresponding to the matrix diagonal starting at
        element (0,0). A square matrix has just one diagonal; if M != N,
        the number of diagonals in the matrix is :math:`|M - N|` + 1. To specify
        a  diagonal sequence number outside this range raises a
        `cpl.core.AccessOutOfRangeError`

        Parameters
        ----------
        diagonal : int
            Sequence number of the diagonal to copy.

        Returns
        -------
        cpl.core.Matrix
            matrix with either 1xN or Mx1 dimensions containing the extracted diagonal.

        Raises
        ------
        cpl.core.AccessOutOfRangeError:
            If the `diagonal` is outside the matrix boundaries
        )pydoc")
      .def("extract_row", &cpl::core::Matrix::extract_row, py::arg("row"),
           R"pydoc(
        Extract a matrix row.

        If a MxN matrix is given in input, the extracted row is a new 1xN matrix.
        The row number is counted from 0.

        Parameters
        ----------
        row : int
            Sequence number of row to copy.

        Returns
        -------
        cpl.core.Matrix
            New matrix representing the row.


        Raises
        ------
        cpl.core.AccessOutOfRangeError
            The row is outside the matrix boundaries.
        )pydoc")
      .def("extract_column", &cpl::core::Matrix::extract_column,
           py::arg("column"), R"pydoc(
        Copy a matrix column.

        If a MxN matrix is given in input, the extracted row is a new Mx1 matrix.
        The column number is counted from 0.

        Parameters
        ----------
        column : int
            Sequence number of column to copy.

        Returns
        -------
        cpl.core.Matrix
            Mx1 Matrix containing the extracted column values.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            The column is outside the matrix boundaries.
        )pydoc")
      .def("fill", &cpl::core::Matrix::fill, py::arg("value"), R"pydoc(
        Write the same value to all matrix elements.

        Parameters
        ----------
        value : float
            Value to write
        )pydoc")
      .def("fill_row", &cpl::core::Matrix::fill_row, py::arg("value"),
           py::arg("row"), R"pydoc(
        Write the same value to a matrix row.

        Write the same value to a matrix row. Rows are counted starting from 0.

        Parameters
        ----------
        value : float
            Value to write
        row : int
            Sequence number of row to overwrite.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            The specified row is outside the matrix boundaries.
        )pydoc")
      .def("fill_column", &cpl::core::Matrix::fill_column, py::arg("value"),
           py::arg("column"), R"pydoc(
        Write the same value to a matrix column.

        Write the same value to a matrix column. Columns are counted starting
        from 0.

        Parameters
        ----------
        value : float
            Value to write
        column : int
            Sequence number of column to overwrite

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            The specified column is outside the matrix boundaries.
        )pydoc")
      .def("fill_diagonal", &cpl::core::Matrix::fill_diagonal, py::arg("value"),
           py::arg("diagonal"), R"pydoc(
        Write a given value to all elements of a given matrix diagonal.

        Parameters
        ----------
        value : float
            Value to write to diagonal
        diagonal : int
            Number of diagonal to overwrite, 0 for main, positive for
            above main, negative for below main.
            main is the diagonal starting from (0, 0) on the matrix.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            The specified diagonal is outside the matrix boundaries.
        )pydoc")
      .def("copy_values_from", &cpl::core::Matrix::copy, py::arg("submatrix"),
           py::arg("row"), py::arg("col"), R"pydoc(
        Copy the values from another matrix into `self`

        The values of `submatrix` are written to `self` starting at the
        indicated row and column. There are no restrictions on the sizes of
        `submatrix`: just the parts of submatrix overlapping matrix are copied.
        There are no restrictions on row and col either, that can also be
        negative. If the two matrices do not overlap, nothing is done, but an
        error is raised.

        Parameters
        ----------
        submatrix : cpl.core.Matrix
            Pointer to matrix to get the values from.
        row : int
            Position of row 0 of `submatrix` in `self`.
        col : int
            Position of column 0 of `submatrix` in `self`.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            No overlap exists between the two matrices.
        )pydoc")
      .def("fill_window", &cpl::core::Matrix::fill_window, py::arg("value"),
           py::arg("row"), py::arg("col"), py::arg("nrow"), py::arg("ncol"),
           R"pydoc(
        Write the same value into a submatrix of a matrix.

        The specified value is written to `self` starting at the indicated
        row and column; `nrow` and `ncol` can exceed `self`
        boundaries, just the range overlapping the `self` is used in that
        case.

        Parameters
        ----------
        value : float
            Value to write.
        row : int
            Start row of matrix submatrix.
        col : int
            Start column of matrix submatrix.
        nrow : int
            Number of rows of matrix submatrix.
        ncol : int
            Number of columns of matrix submatrix.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            The specified start position is outside the matrix boundaries.
        cpl.core.IllegalInputError
            nrow or ncol are not positive.
        )pydoc")
      .def("shift", &cpl::core::Matrix::shift, py::arg("rshift"),
           py::arg("cshift"), R"pydoc(
        Shift matrix elements.

        The performed shift operation is cyclical (toroidal), i.e., matrix
        elements shifted out of one side of the matrix get shifted in from
        its opposite side. There are no restrictions on the values of the
        shift. Positive shifts are always in the direction of increasing
        row/column indexes.

        Parameters
        ----------
        rshift : int
            Shift in the vertical direction.
        cshift : int
            Shift in the horizontal direction.
        )pydoc")
      .def(
          "threshold_small",
          [](cpl::core::Matrix& self, std::optional<double> tolerance) -> void {
            self.threshold_small(tolerance.value_or(-1.0));
          },
          py::arg("tolerance").none(true) = py::none(), R"pydoc(
        Rounding to zero very small numbers in matrix.

        After specific manipulations of a matrix some of its elements
        may theoretically be expected to be zero (for instance, as a result
        of multiplying a matrix by its inverse). However, because of
        numerical noise, such elements may turn out not to be exactly
        zero. With this function any very small number in the matrix is
        turned to exactly zero.

        If no `tolerance` is given then the default value is used, equal to the
        machine double epsilon.

        Parameters
        ----------
        tolerance : float, optional
            Max tolerated rounding to zero. If not given the machine
            double epsilon is used.

        Notes
        -----
        If tolerance is given a negative value, the default value for tolerance
        will be used (machine double epsilon).
        )pydoc")
      .def(
          "is_zero",
          [](cpl::core::Matrix& self, std::optional<double> tolerance) -> bool {
            return self.is_zero(tolerance.value_or(-1.0));
          },
          py::arg("tolerance").none(true) = py::none(), R"pydoc(
        Check for zero matrix.

        After specific manipulations of a matrix some of its elements
        may theoretically be expected to be zero. However, because of
        numerical noise, such elements may turn out not to be exactly
        zero. In this specific case, if any of the matrix element is
        not exactly zero, the matrix would not be classified as a null
        matrix. A threshold may be specified to consider zero any number
        that is close enough to zero.

        If no `tolerance` is given then the default value is used, equal to the
        machine double epsilon.

        Parameters
        ----------
        tolerance : float
            Max tolerated rounding to zero. If not given the machine
            double epsilon is used.

        Returns
        -------
        bool
            True if `self` is a zero matrix. False otherwise

        Notes
        -----
        If tolerance is given a negative value, the default value for tolerance
        will be used (machine double epsilon).
        )pydoc")
      .def(
          "is_diagonal",
          [](cpl::core::Matrix& self, std::optional<double> tolerance) -> bool {
            return self.is_diagonal(tolerance.value_or(-1.0));
          },
          py::arg("tolerance").none(true) = py::none(), R"pydoc(
        Check if a matrix is diagonal.

        A threshold may be specified to consider zero any number that
        is close enough to zero. If the specified `tolerance` is
        negative (default), the default value is used, equal to the
        machine double epsilon. A zero tolerance may also be specified.

        No error is raised if the `self` is not square.

        Parameters
        ----------
        tolerance : float
            Max tolerated rounding to zero. If not given the machine
            double epsilon is used.

        Returns
        -------
        bool
            True if `self` is a diagonal matrix. False otherwise

        Raises
        ------
        cpl.core.IllegalInputError
            if the matrix is not square

        Notes
        -----
        If tolerance is given a negative value, the default value for tolerance
        will be used (machine double epsilon).
        )pydoc")
      .def(
          "is_identity",
          [](cpl::core::Matrix& self, std::optional<double> tolerance) -> bool {
            return self.is_identity(tolerance.value_or(-1.0));
          },
          py::arg("tolerance").none(true) = py::none(), R"pydoc(
        Check for identity matrix.

        A threshold may be specified to consider zero any number that is
        close enough to zero, and 1 any number that is close enough to 1.
        If `tolerance` is not given, the default
        value is used, equal to the machine double epsilon. A zero
        tolerance may also be specified.

        No error is raised if the `self` is not square.

        Parameters
        ----------
        tolerance : float, optional
            Max tolerated rounding to zero, or to one. If not given the machine
            double epsilon is used.

        Returns
        -------
        bool
            True if `self` is a identity matrix. False otherwise

        Raises
        ------
        cpl.core.IllegalInputError
            if the matrix is not square

        Notes
        -----
        If tolerance is given a negative value, the default value for tolerance
        will be used (machine double epsilon).
        )pydoc")
      .def("swap_rows", &cpl::core::Matrix::swap_rows, py::arg("row1"),
           py::arg("row2"), R"pydoc(
        Swap two matrix rows.

        The values of two given matrix rows are exchanged. Rows are counted
        starting from 0. If the same row number is given twice, nothing is
        done and no exception is raised.

        Parameters
        ----------
        row1 : int
            One matrix row.
        row2 : int
            Another matrix row.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            Either of the specified rows is outside the matrix boundaries.
        )pydoc")
      .def("swap_columns", &cpl::core::Matrix::swap_columns, py::arg("column1"),
           py::arg("column2"), R"pydoc(
        Swap two matrix columns.

        The values of two given matrix columns are exchanged. Columns are
        counted starting from 0. If the same column number is given twice,
        nothing is done and no exception is raised.

        Parameters
        ----------
        column1 : int
            One matrix column.
        column2 : int
            Another matrix column.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            Either of the specified columns is outside the matrix boundaries.
        )pydoc")
      .def("swap_rowcolumn", &cpl::core::Matrix::swap_rowcolumn, py::arg("row"),
           R"pydoc(
        Swap a matrix column with a matrix row.

        The values of the indicated row are exchanged with the column having
        the same sequence number. Rows and columns are counted starting from 0.

        Parameters
        ----------
        row : int
            Matrix row.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            The specified row is outside the matrix boundaries.
        cpl.core.IllegalInputError
            `self` is not square.
        )pydoc")
      .def("flip_rows", &cpl::core::Matrix::flip_rows, R"pydoc(
        Reverse order of rows in matrix.

        The order of the rows in the matrix is reversed in place.
        )pydoc")
      .def("flip_columns", &cpl::core::Matrix::flip_columns, R"pydoc(
        Reverse order of columns in matrix.

        The order of the columns in the matrix is reversed in place.
        )pydoc")
      .def("transpose_create", &cpl::core::Matrix::transpose_create, R"pydoc(
        Returns the transpose of the matrix in a new matrix.

        Returns
        -------
        cpl.core.Matrix
            New transposed matrix.
        )pydoc")
      .def("sort_rows", &cpl::core::Matrix::sort_rows,
           py::arg("by_absolute") = false, R"pydoc(
        Sort matrix by rows.

        The matrix elements of the leftmost column are used as reference for the row sorting,
        if there are identical the values of the second column are considered, etc.
        Rows with the greater values go on top.

        Parameters
        ----------
        by_absolute : bool, optional
            True to sort by absolute value. Default False.
        )pydoc")
      .def("sort_columns", &cpl::core::Matrix::sort_columns,
           py::arg("by_absolute") = false, R"pydoc(
        Sort matrix by columns.

        The matrix elements of the top row are used as reference for the column sorting,
        if there are identical the values of the second row are considered, etc.
        Columns with the greater values go to left.

        Parameters
        ----------
        by_absolute : bool, optional
            True to sort by absolute value. Default False.
        )pydoc")
      .def("erase_rows", &cpl::core::Matrix::erase_rows, py::arg("start"),
           py::arg("count"), R"pydoc(
        Delete rows from a matrix.

        A select number of rows will be completely removed from the object,
        reducing the total number of rows by 1. The specified segment can
        extend beyond the end of the matrix, but an attempt to remove all
        matrix rows will raise an exception because zero length matrices
        are illegal. Rows are counted starting from 0.

        Parameters
        ----------
        start : int
            First row to delete.
        count : int
            Number of rows to delete.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            The specified start is outside the matrix boundaries.
        cpl.core.IllegalInputError
            count is not positive.
        cpl.core.IllegalOutputError
            Attempt to delete all the rows of matrix.
        )pydoc")
      .def("erase_columns", &cpl::core::Matrix::erase_columns, py::arg("start"),
           py::arg("count"), R"pydoc(
        Delete columns from a matrix.

        A portion of the matrix data is removed. The specified
        segment can extend beyond the end of the matrix, but an attempt to
        remove all matrix columns will raise an exception because zero length
        matrices are illegal. Columns are counted starting from 0.

        Parameters
        ----------
        start : int
            First column to delete.
        count : int
            Number of columns to delete.

        Raises
        ------
        cpl.core.AccessOutOfRangeError
            The specified start is outside the matrix boundaries.
        cpl.core.IllegalInputError
            count is not positive.
        cpl.core.IllegalOutputError
            Attempt to delete all the columns of matrix.
        )pydoc")
      .def("resize", &cpl::core::Matrix::resize, py::arg("top"),
           py::arg("bottom"), py::arg("left"), py::arg("right"), R"pydoc(
        Resize a matrix by adding or removing rows and/or columns from the edges.

        `self` is reframed according to specifications. Extra rows
        and column on the sides might also be negative, as long as they are
        compatible with the matrix sizes: `self` would be reduced
        in size accordingly, but an attempt to remove all matrix columns
        and/or rows raises an exception because zero length matrices are
        illegal. The old matrix elements contained in the new shape are left
        unchanged, and new matrix elements added by the reshaping are initialised
        to zero. No reshaping (i.e., all the extra rows set to zero) would
        not raise an exception.

        Parameters
        ----------
        top : int
            Extra rows on top.
        bottom : int
            Extra rows on bottom.
        left : int
            Extra columns on left.
        right : int
            Extra columns on right.

        Raises
        ------
        cpl.core.IllegalOutputError
            Attempt to shrink `self` to zero size (or less).
        )pydoc")
      .def("set_size", &cpl::core::Matrix::set_size, py::arg("rows"),
           py::arg("columns"), R"pydoc(
        Resize a matrix.

        `self` is resized according to specifications. The old
        matrix elements contained in the resized matrix are left unchanged,
        and new matrix elements will be added by an increase of the matrix number of
        rows and/or columns are initialised to zero. New rows and/or columns
        will be added to the right/bottom of `self`. Likewise when
        shrinking the matrix by one of the dimensions, the rows/columns will be
        removed from the right/bottom of `self`.

        Parameters
        ----------
        rows : int
            New number of rows.
        columns : int
            New number of columns.

        Raises
        ------
        cpl.core.IllegalOutputError
            Attempt to shrink matrix to zero size (or less).
        )pydoc")
      .def("append", &cpl::core::Matrix::append, py::arg("other"),
           py::arg("mode"), R"pydoc(
        Append a matrix to another.

        If mode is set to 0, the matrices must have the same number of rows, and are
        connected horizontally with the `self` on the left. If mode is set to 1,
        the matrices must have the same number of columns, and are connected vertically
        with `self` on top. The `self` is expanded to include the values from the
        `other`, while `other` is left untouched.

        Parameters
        ----------
        other : cpl.core.Matrix
            matrix to append to `self`
        mode : int
            Matrices connected horizontally (0) or vertically (1).

        Raises
        ------
        cpl.core.IllegalInputError
            `mode` is neither 0 nor 1
        cpl.core.IncompatibleInputError
            	Matrices cannot be joined as indicated by `mode`.
        )pydoc")
      .def("add", &cpl::core::Matrix::add, py::arg("other"), R"pydoc(
        Perform matrix addition with `other` and `self`

        Add matrices `self` and `other` element by element. The two matrices must have
        identical sizes. The result is written to the `self`.

        Parameters
        ----------
        other : cpl.core.Matrix
            matrix to add with


        Raises
        ------
        cpl.core.IncompatibleInputError
            if `other` does not have the same size as `self`
        )pydoc")
      .def("subtract", &cpl::core::Matrix::subtract, py::arg("other"), R"pydoc(
        Subtract matrix `other` from `self`

        Subtract `other` from `self` element by element.
        The two matrices must have identical sizes. The result is written
        to `self`.

        Parameters
        ----------
        other : cpl.core.Matrix
            matrix to subtract with

        Raises
        ------
        cpl.core.IncompatibleInputError
            if `other` does not have the same size as `self`
        )pydoc")
      .def("multiply", &cpl::core::Matrix::multiply, py::arg("other"), R"pydoc(
        Multiply `self` by `other`, element by element. The two matrices must
        have identical sizes. The result is written to `self`.

        Parameters
        ----------
        other : cpl.core.Matrix
            matrix to multiply with

        Raises
        ------
        cpl.core.IncompatibleInputError
            if `other` does not have the same size as `self`

        Notes
        -----
        To obtain the rows-by-columns product between two matrices, use product_create()

        See Also
        --------
        cpl.core.matrix.product_create : Rows-by-columns product of two matrices
        cpl.core.matrix.multiply_scalar : Multiply `self` by a scalar.
        )pydoc")
      .def("divide", &cpl::core::Matrix::divide, py::arg("other"), R"pydoc(
        Divide `self` by `other`, element by element.

        Divide each element of `self` by the corresponding
        element of the second one. The two matrices must have the same
        number of rows and columns. The result is written to the first
        matrix. No check is made against a division by zero.

        Parameters
        ----------
        other : cpl.core.Matrix
            matrix to divide with

        Raises
        ------
        cpl.core.IncompatibleInputError
            if `other` does not have the same size as `self`
        )pydoc")
      .def("add_scalar", &cpl::core::Matrix::add_scalar, py::arg("value"),
           R"pydoc(
        Add a scalar to `self`.

        Add the same value to each matrix element.

        Parameters
        ----------
        value : float
            Value to add.
        )pydoc")
      .def("subtract_scalar", &cpl::core::Matrix::subtract_scalar,
           py::arg("value"), R"pydoc(
        Subtract a scalar to `self`.

        Subtract the same value to each matrix element.

        Parameters
        ----------
        value : float
            Value to subtract.
        )pydoc")
      .def("multiply_scalar", &cpl::core::Matrix::multiply_scalar,
           py::arg("value"), R"pydoc(
        Multiply `self` by a scalar.

        Multiply each matrix element by the same factor.

        Parameters
        ----------
        value : float
            Multiplication factor.

        See Also
        ----------
        cpl.core.matrix.multiply : Multiply `self` by `other`, element by element.
        cpl.core.matrix.product_create : Rows-by-columns product of two matrices.
        )pydoc")
      .def("divide_scalar", &cpl::core::Matrix::divide_scalar, py::arg("value"),
           R"pydoc(
        Divide `self` by a scalar.

        Divide each matrix element by the same value.

        Parameters
        ----------
        value : float
            Divisor.

        Raises
        ------
        cpl.core.DivisionByZeroError
            `value` is 0.0
        )pydoc")
      .def("logarithm", &cpl::core::Matrix::logarithm, py::arg("base"), R"pydoc(
        Compute the logarithm of matrix elements.

        Each matrix element is replaced by its logarithm in the specified base.
        The base and all matrix elements must be positive.

        Parameters
        ----------
        base : float
            Logarithm base.

        Raises
        ------
        cpl.core.IllegalInputError
            `base` or any element of `self` is not positive
        )pydoc")
      .def("exponential", &cpl::core::Matrix::exponential, py::arg("base"),
           R"pydoc(
        Compute the exponential of matrix elements.

        Each matrix element is replaced by its exponential in the specified base.
        The base must be positive.

        Parameters
        ----------
        base : float
            Exponential base.

        Raises
        ------
        cpl.core.IllegalInputError
            `base` is not positive
        )pydoc")
      .def("power", &cpl::core::Matrix::power, py::arg("exponent"), R"pydoc(
        Compute a power of matrix elements.

        Each matrix element is replaced by its power to the specified
        exponent. If the specified exponent is not negative, all matrix
        elements must be not negative; if the specified exponent is
        negative, all matrix elements must be positive; otherwise, an
        error condition is set and the matrix will be left unchanged.
        If the exponent is exactly 0.5 the (faster)  sqrt() will be
        applied instead of  pow(). If the exponent is zero, then any
        (non negative) matrix element would be assigned the value 1.0.

        Parameters
        ----------
        exponent : float
            Constant exponent.

        Raises
        ------
        cpl.core.IllegalInputError
            Any element of `self` is not compatible with `exponent` (see extended summary)
        )pydoc")
      .def("decomp_lu", &cpl::core::Matrix::decomp_lu, R"pydoc(
        Replace a matrix by its LU-decomposition

        `self` must be a n X n non-singular matrix to decompose. `self` will be modified
        inplace in which its values will be replaced with it's LU-decomposed values.

        The resulting LU decomposition can be solved with `cpl.core.Matrix.solve_lu`.

        Returns
        -------
        tuple(List[int], bool)
            The pair of n-integer list filled with row permutations (perm) and True/False for even number of
            permutations (psig). In the format (perm, psig).

        Raises
        ------
        cpl.core.IllegalInputError
            `self` is not an n by n matrix.
        cpl.core.SingularMatrixError
            `self` is singular.
        cpl.core.IncompatibleInputError
            `self` and `perm` have incompatible sizes.
        cpl.core.TypeMismatchError
            `perm` is not a list of ints

        Notes
        -----
        Algorithm reference: Golub & Van Loan, Matrix Computations, Algorithms 3.2.1 (Outer Product
        Gaussian Elimination) and 3.4.1 (Gauss Elimination with Partial Pivoting).

        See Also
        --------
        cpl.core.Matrix.solve_lu : Used to solve the LU-decomposition
        )pydoc")
      .def("solve_lu", &cpl::core::Matrix::solve_lu, py::arg("rhs"),
           py::arg("perm").none(true) = py::none(), R"pydoc(
        Solve a LU-system

        `self` should be a n x n LU-matrix that has been decomposed using
        `self.decomp_lu()`

        Parameters
        ----------
        rhs : cpl.core.Matrix
            m right-hand-sides. This is duplicated and replaced by the solution of `self` to generate the
            return matrix.
        perm : list of ints
            n-integer array filled with the row permutations

        Returns
        -------
        cpl.core.Matrix
            The solution of `self` as applied to `rhs`

        Raises
        ------
        cpl.core.IllegalInputError
            `self` is not an n by n matrix
        cpl.core.IncompatibleInputError
            The array or matrices not have the same number of rows.
        cpl.core.DivisionByZeroError
            The main diagonal of U contains a zero. This error can only occur
            if the LU-matrix does not come from a successful call to
            `cpl.core.Matrix.decomp_lu`.

        See Also
        --------
        cpl.core.Matrix.decomp_lu : Used to generate an LU-system which can then be solved using this method.
        )pydoc")
      .def("decomp_chol", &cpl::core::Matrix::decomp_chol, R"pydoc(
        Replace a matrix by its Cholesky-decomposition, L * transpose(L) = A

        Notes
        -----
        Only the upper triangle of self is read, L is written in the lower triangle
        If the matrix is singular the elements of self become undefined
        )pydoc")
      .def("solve_chol", &cpl::core::Matrix::solve_chol, py::arg("rhs"),
           R"pydoc(
        Solve a L*transpose(L)-system

        Parameters
        ----------
        rhs : cpl.core.Matrix
            M right-hand-sides to be replaced by their solution

        Notes
        -----
        Only the lower triangle of self is accessed
        )pydoc")
      .def("determinant", &cpl::core::Matrix::get_determinant, R"pydoc(
        Compute the determinant of a matrix.

        `self` must be a square matrix. In case of a 1x1 matrix, the matrix
        single element value is returned.

        Returns
        -------
        float
            Matrix determinant

        Raises
        ------
        cpl.core.IllegalInputError
            `self` is not square.
        cpl.core.UnspecifiedError
            `self` is near-singular with a determinant so close to zero that it cannot be
            represented by a double.
        )pydoc")
      .def("invert_create", &cpl::core::Matrix::invert_create, R"pydoc(
        Find a matrix inverse of `self`

        `self` must be a square matrix.

        Returns
        -------
        cpl.core.Matrix
            Inverse matrix.


        Notes
        -----
        When calling  invert_create() with a nearly singular
        matrix, it is possible to get a result containin NaN values without
        any error code being set.
        )pydoc")
      .def("mean", &cpl::core::Matrix::get_mean, R"pydoc(
        Find the mean of all matrix elements.

        The mean of all matrix elements is calculated

        Returns
        -------
        float
            Mean of all matrix elements

        Notes
        -----
        This function works properly only if all elements of the matrix have
        finite values (not NaN or Infinity).
        )pydoc")
      .def("stdev", &cpl::core::Matrix::get_stdev, R"pydoc(
        Find the standard deviation of all matrix elements.

        The standard deviation of all matrix elements is calculated

        Returns
        -------
        float
            Standard deviation of all matrix elements

        Notes
        -----
        This function works properly only if all elements of the matrix have
        finite values (not NaN or Infinity).
        )pydoc")
      .def("median", &cpl::core::Matrix::get_median, R"pydoc(
        Find the median of all matrix elements.

        The median of all matrix elements is calculated

        Returns
        -------
        float
            Median of all matrix elements
        )pydoc")
      .def("min", &cpl::core::Matrix::get_min, R"pydoc(
        Find the minimum value of all matrix elements.

        The minimum value of matrix elements is found.

        Returns
        -------
        float
            Minimum value in the matrix
        )pydoc")
      .def("max", &cpl::core::Matrix::get_max, R"pydoc(
        Find the maximum value of all matrix elements.

        The maximum value of matrix elements is found.

        Returns
        -------
        float
            Maximum value in the matrix
        )pydoc")
      .def("minpos", &cpl::core::Matrix::get_minpos, R"pydoc(
        Find position of minimum value of matrix elements.

        The position of the minimum value of all matrix elements is found.
        If more than one matrix element have a value corresponding to
        the minimum, the lowest element row number is returned in  row.
        If more than one minimum matrix elements have the same row number,
        the lowest element column number is returned in column.

        Returns
        -------
        tuple(int, int)
            tuple in the format (row, column), where:
            - row is the returned row position of minimum.
            - column is the returned column position of minimum
        )pydoc")
      .def("maxpos", &cpl::core::Matrix::get_maxpos, R"pydoc(
        Find position of maximum value of matrix elements.

        The position of the maximum value of all matrix elements is found.
        If more than one matrix element have a value corresponding to
        the maximum, the lowest element row number is returned in  row.
        If more than one maximum matrix elements have the same row number,
        the lowest element column number is returned in column.

        Returns
        -------
        tuple(int, int)
            tuple in the format (row, column), where:
            - row is the returned row position of maximum.
            - column is the returned column position of maximum
        )pydoc")
      .def("__matmul__", &cpl::core::Matrix::product_create)
      .def("product_create", &cpl::core::Matrix::product_create, py::arg("rhs"),
           R"pydoc(
        Rows-by-columns product of two matrices.

        The number of columns of the first matrix must be equal to the
        number of rows of the second matrix.

        Can also use the ``@`` operator to call this function for example with
        ``lhs`` as the calling object:

        .. code-block:: python
        
          product = lhs @ rhs

        Parameters
        ----------
        rhs : cpl.core.Matrix
            Right side matrix to get the product with the calling object

        Returns
        -------
        cpl.core.Matrix
            The rows-by-columns product of calling matrix and rhs matrix

        Raises
        ------
        cpl.core.IncompatibleInputError
            The number of columns of the calling matrix is not equal to
            the number of rows of the rhs matrix.

        See Also
        --------
        cpl.core.matrix.multiply : Multiply `self` by `other`, element by element.
        cpl.core.matrix.multiply_scalar : Multiply `self` by a scalar.
        cpl.core.matrix.product_normal : Compute A = B * transpose(B)
      )pydoc")
      .def("product_normal", &cpl::core::Matrix::product_normal, R"pydoc(
        Compute A = B * transpose(B)

        self * transpose(self)
        Matrix multiplication results in a matrix of the size
        [rows of left] * [columns of right]
        Here, left = self, right = transpose(self)
        and the rows/columns of a transpose(self) are flipped from a self
        so the result of the multiplication is [rows of self] * [columns of
        transpose(self)], simplifies into  [rows of self] * [rows of self]

        Parameters
        ----------
        other : cpl.core.Matrix
            M x N Matrix to multiply with its transpose

        Returns
        -------
        cpl.core.Matrix
            Resulting matrix

        Notes
        -----
        Only the upper triangle of A is computed, while the elements below the main diagonal have undefined values.

        See Also
        --------
        cpl.core.product_create : Rows-by-columns product of two matrices.
        cpl.core.multiply : Multiply `self` by `other`, element by element.
        cpl.core.matrix.multiply_scalar : Multiply `self` by a scalar.
        )pydoc")
      .def("__deepcopy__",
           [](cpl::core::Matrix& self, py::dict /* memo */)
               -> cpl::core::Matrix { return self.duplicate(); })
      .def("product_transpose", &cpl::core::Matrix::product_transpose,
           py::arg("ma"), py::arg("mb"), R"pydoc(
        Fill a matrix with the product of A * B'

        Parameters
        ----------
        ma : cpl.core.Matrix
            The matrix A, of size M x K
        mb : cpl.core.Matrix
            The matrix B, of size N x K

        Notes
        -----
        The use of the transpose of B causes a more efficient memory access
        Changing the order of A and B is allowed, it transposes the result
        )pydoc")
      .def_static("solve_normal", &cpl::core::Matrix::solve_normal,
                  py::arg("coefficients"), py::arg("rhs"),
                  R"pydoc(
        Solution of overdetermined linear equations in a least squares sense.

        rhs may contain more than one column, which each represent an independent right-hand-side.

        Parameters
        ----------
        coefficients : cpl.core.Matrix
            The N by M matrix of coefficients, where N >= M.
        rhs : cpl.core.Matrix
            An N by K matrix containing K right-hand-sides.

        Return
        ------
        cpl.core.Matrix
            A newly allocated M by K solution matrix

        Raises
        ----------
        cpl.core.IllegalInputError
            if coefficients is not a square matrix
        cpl.core.IncompatibleInputError
            if coefficients and rhs do not have the same number of rows
        cpl.core.SingularMatrixError
            if coeff is singular (to working precision)

        Notes
        -----
        The following linear system of N equations and M unknowns is given:

        coeff * X = rhs

        where coeff is the NxM matrix of the coefficients, X is the MxK matrix of the unknowns, and rhs the NxK matrix
        containing the K right hand side(s).

        The solution to the normal equations is known to be a least-squares solution, i.e. the 2-norm of coeff * X - rhs
        is minimized by the solution to transpose(coeff) * coeff * X = transpose(coeff) * rhs.

        In the case that coeff is square (N is equal to M) it gives a faster and more accurate result to use
        cpl.core.Matrix.solve().

        )pydoc")
      .def_static("solve", &cpl::core::Matrix::solve, py::arg("coefficients"),
                  py::arg("rhs"), R"pydoc(
        Solution of a linear system.

        Compute the solution of a system of N equations with N unknowns:

        coefficients * X = rhs

        coefficients must be an NxN matrix, and rhs a NxM matrix. M greater than 1 means that multiple independent
        right-hand-sides are solved for.

        rhs must have N rows and may contain more than one column, which each represent an independent
        right-hand-side.

        Parameters
        ----------
        coefficients : cpl.core.Matrix
            The N x N matrix of coefficients
        rhs : cpl.core.Matrix
            An N by M matrix containing one or more right-hand sides

        Returns
        -------
        cpl.core.Matrix
            New solution cpl.core.Matrix with the same size as rhs

        Raises
        ------
        cpl.core.IllegalInputError
            if coefficients is not a square matrix
        cpl.core.IncompatibleInputError
            if coefficients and rhs do not have the same number of rows
        cpl.core.SingularMatrixError
            if coefficients is singular (to working precision)
        )pydoc")
      .def_static("solve_svd", &cpl::core::Matrix::solve_svd,
                  py::arg("coefficients"), py::arg("rhs"),
                  py::arg("threshold_mode").none(true) = py::none(),
                  py::arg("threshold_tol") =
                      0,  // Default to 0 as that would be the behaviour if
                          // cpl_matrix_solve_svd is called instead of
                          // cpl_matrix_solve_svd_threshold
                  R"pydoc(
        Solve a linear system in a least square sense using an SVD factorization, optionally discarding
        singular values below a given threshold.

        The function solves a linear system of the form Ax = b for the solution vector x, where A is
        represented by the argument `coefficients` and b by the argument `rhs`. 
        
        If `threshold_mode` and `threshold_tol` are passed, singular values which are less or equal than a
        given cutoff value are treated as zero. Otherwise all singular values are taken into account,
        regardless of their magnitude. This latter case is equivalent to setting `threshold_mode` to
        cpl.core.Matrix.ThresholdMode.USER and `threshold_tol` to 0.

        The argument `threshold_mode` is used to select the computation of the cutoff value for small
        singular values. If `threshold_mode` is set to cpl.core.Matrix.ThresholdMode.EPSILON the machine
        precision DBL_EPSILON is used as the cutoff factor. If `threshold_mode` is cpl.core.Matrix.ThresholdMode.SIZE,
        the cutoff factor is computed as 10*DBL_EPSILON*max(N, M), and if `threshold_mode`
        is cpl.core.Matrix.ThresholdMode.USER the argument `threshold_tol`, a value in the range [0,1]
        is used as the cutoff factor. The actual cutoff value, is then given by the cutoff factor times
        the biggest singular value obtained from the SVD of the matrix coefficients of`self`.

        Parameters
        ----------
        coefficients : cpl.core.Matrix
            An N by M matrix of linear system coefficients, where N >= M
        rhs : cpl.core.Matrix
            An N by 1 matrix with the right hand side of the system
        threshold_mode : cpl.core.Matrix.ThresholdMode, optional
            Optional cutoff mode selector. used to select the computation of the cutoff value for small singular values.
            Options:
            - cpl.core.Matrix.ThresholdMode.EPSILON to use machine DBL_EPSILON as the cutoff factor
            - cpl.core.Matrix.ThresholdMode.SIZE, where the cutoff factor is computed as 10*DBL_EPSILON*max(N, M)
            - cpl.core.Matrix.ThresholdMode.USER, where the cutoff factor is set as the value passed to `threshold_tol`
            For consistency with CPL, the integer values 0, 1 or 2 can also be passed instead of the symbolic
            contants.
        threshold_tol : float, optional
            Factor used to compute the cutoff value if `threshold_mode` is set to cpl.core.Matrix.ThresholdMode.USER.
            Must be a value between 0. and 1. Defaults to 0. if not set, but is not used unless `threshold_mode`
            is set to cpl.core.Matrix.ThresholdMode.USER. See Notes for more details.

        Return
        ------
        cpl.core.Matrix
            A newly allocated M by 1 solution matrix

        Raises
        ------
        cpl.core.IncompatibleInputError
            if `coefficients` and `rhs` do not have the same number of rows
        cpl.core.IllegalInputError
            if matrix `rhs` has more than one column, an illegal mode
            (not one of cpl.core.Matrix.ThresholdMode.EPSILON, cpl.core.Matrix.ThresholdMode.SIZE
            or cpl.core.Matrix.ThresholdMode.USER, or their integer equivalents
            0, 1, or 2), or an illegal tolerance (not between 0. and 1.) was given.
        
        Notes
        -----
        The linear system is solved using the singular value decomposition (SVD) of the coefficient matrix,
        based on a one-sided Jacobi orthogonalization.
        )pydoc")
      .def_property_readonly(
          "shape",
          [](const cpl::core::Matrix& self) -> std::pair<size, size> {
            return std::make_pair(self.get_nrow(), self.get_ncol());
          })
      .def("__eq__", &cpl::core::Matrix::operator==)
      // Fallback if the above overload doesn't work:
      // Matricies are only ever equal to other matricies
      .def("__eq__",
           [](cpl::core::Matrix&, py::object) -> bool { return false; })
      .def(
          "__iter__",
          [](cpl::core::Matrix& self) -> py::iterator {
            // See MatrixIterWrap class for info
            return py::make_iterator(MatrixIterWrap(self.begin()),
                                     MatrixIterWrap(self.end()));
          },
          py::keep_alive<0, 1>(), "Iterate through the matrix rows")
      .def("__len__", &cpl::core::Matrix::get_nrow)
      .def("__str__", &cpl::core::Matrix::dump)
      .def(
          "__getitem__",
          [](cpl::core::Matrix& self,
             size index) -> cpl::core::Matrix_iterator {
            auto at_index =
                index < 0 ? (self.end() + index) : (self.begin() + index);
            if (at_index >= self.end() || at_index < self.begin()) {
              throw py::index_error(std::to_string(index));
            } else {
              return at_index;
            }
          },
          py::keep_alive<0, 1>(), "Get matrix row via index")
      // row then col slices
      .def(
          "__getitem__",
          [](cpl::core::Matrix& self,
             std::pair<py::object, py::object> spec) -> cpl::core::Matrix {
            // The objects are expected to be either a slice of an integer
            try {
              size row_is_idx = spec.first.cast<size>();
              spec.first = py::slice(row_is_idx, row_is_idx + 1, 1);
            }
            catch (const py::cast_error& /* exception */) {
              // Assume it's already a slice from now on
            }

            try {
              size row_is_idx = spec.second.cast<size>();
              spec.second = py::slice(row_is_idx, row_is_idx + 1, 1);
            }
            catch (const py::cast_error& /*exception */) {
              // Assume it's already a slice from now on
            }

            std::optional<size> start_row, start_col, stop_row, stop_col,
                step_row, step_col;
            try {
              start_row = spec.first.attr("start").cast<std::optional<size>>();
              start_col = spec.second.attr("start").cast<std::optional<size>>();
              stop_row = spec.first.attr("stop").cast<std::optional<size>>();
              stop_col = spec.second.attr("stop").cast<std::optional<size>>();
              step_row = spec.first.attr("step").cast<std::optional<size>>();
              step_col = spec.second.attr("step").cast<std::optional<size>>();
            }
            catch (...) {
              throw py::type_error("Expected index: 2 slices or indicies");
            }

            if (!step_row.has_value()) {
              step_row = {1};
            }
            if (!step_col.has_value()) {
              step_col = {1};
            }
            // steps are now guaranteed to have a value

            if (step_row == 0 || step_col == 0) {
              throw py::value_error("Step size cannot be 0");
            }

            // Startingpoint is based on if the step is >0 or <0
            if (!start_row.has_value()) {
              start_row = {*step_row > 0 ? 0 : self.get_nrow() - 1};
            }
            if (!start_col.has_value()) {
              start_col = {*step_col > 0 ? 0 : self.get_ncol() - 1};
            }
            // start indicies are now guaranteed to have a value

            if (!stop_row.has_value()) {
              stop_row = {*step_row > 0 ? self.get_nrow() : -1};
            }

            if (!stop_col.has_value()) {
              stop_col = {*step_col > 0 ? self.get_ncol() : -1};
            }
            // all indicies are now guaranteed to have a value


            // Negative indexing
            if (*start_row < 0)
              start_row = {*start_row + self.get_nrow()};
            if (*start_col < 0)
              start_col = {*start_col + self.get_ncol()};
            if (*stop_row < 0)
              stop_row = {*stop_row + self.get_nrow()};
            if (*stop_col < 0)
              stop_col = {*stop_col + self.get_ncol()};

            if (*start_row < 0 || *start_row > self.get_nrow() ||
                *start_col < 0 || *start_col > self.get_ncol() ||
                *stop_row < 0 || *stop_row > self.get_nrow() || *stop_col < 0 ||
                *stop_col > self.get_ncol()) {
              throw py::index_error();
            }

            // Get a number of outputs
            // contiguous = number of elements if only had step size of 1

            // There should always be at least one element returned, so
            // +1. Any further elements therefore require a multiple of
            // stepsize +1 (hence -1) between *stop_row and *start_row.
            // (hence rounded down / division)
            size nrow = (abs(*stop_row - *start_row) - 1) / abs(*step_row) + 1;
            size ncol = (abs(*stop_col - *start_col) - 1) / abs(*step_col) + 1;

            return self.extract(*start_row, *start_col, *step_row, *step_col,
                                nrow, ncol);
          },
          "Extract matrix values with slice")
      .def("__repr__",
           [](const cpl::core::Matrix& self) -> std::string {
             std::ostringstream ss;
             ss << "cpl.core.Matrix(" << self.get_nrow();
             ss << ", " << self.get_ncol();
             ss << ", [";
             size data_size = self.get_nrow() * self.get_ncol();
             for (size i = 0; i < data_size; ++i) {
               ss << self.get_data()[i];
               if (i + 1 != data_size) {
                 ss << ", ";
               }
             }
             ss << "])";
             return ss.str();
           })
      .def("__deepcopy__",
           [](const cpl::core::Matrix& self, py::object /* memo */)
               -> cpl::core::Matrix { return cpl::core::Matrix(self); });

  matrix_iterator.doc() = R"pydoc(
        Returned from a Matrix's __getitem__ method or iterator. Used to access specific rows of the matrix.

        Not instantiatable on its own.
    )pydoc";

  matrix_iterator
      .def(py::init(
          [](cpl::core::Matrix& of, size row) -> cpl::core::Matrix_iterator {
            return of.begin() + row;
          }))
      .def(
          "__iter__",
          [](const cpl::core::Matrix::iterator& self) -> py::iterator {
            return py::make_iterator(self.begin(), self.end());
          },
          py::keep_alive<0, 1>())
      .def("__len__",
           [](const cpl::core::Matrix::iterator& self) -> std::ptrdiff_t {
             return self.end() - self.begin();
           })
      .def("__getitem__",
           [](const cpl::core::Matrix::iterator& self, size index) -> double {
             auto at_index =
                 index < 0 ? (self.end() + index) : (self.begin() + index);
             if (at_index >= self.end() || at_index < self.begin()) {
               throw py::index_error(std::to_string(index));
             } else {
               return *at_index;
             }
           })
      .def("__setitem__",
           [](cpl::core::Matrix::iterator& self, size index,
              double value) -> void {
             auto at_index =
                 index < 0 ? (self.end() + index) : (self.begin() + index);
             if (at_index >= self.end() || at_index < self.begin()) {
               throw py::index_error(std::to_string(index));
             } else {
               *at_index = value;
             }
           })
      .def("index",
           [](const cpl::core::Matrix::iterator& self,
              double contains) -> std::ptrdiff_t {
             for (cpl::core::Matrix::const_iterator::iterator elem =
                      self.begin();
                  elem != self.end(); ++elem) {
               if (*elem == contains) {
                 return elem - self.begin();
               }
             }
             std::ostringstream ss;
             ss << contains << " is not in the Matrix row";
             throw py::value_error(ss.str());
           })
      .def("__repr__",
           [](const cpl::core::Matrix::iterator& self) -> std::string {
             std::ostringstream ss;
             ss << "<cpl.core.MatrixIterator values=[";
             for (auto elem : self) {
               ss << elem << ", ";
             }
             ss << "]>";
             return ss.str();
           });
}

using Matrix_iterator = cpl::core::Matrix::iterator;

MatrixIterWrap&
MatrixIterWrap::operator++()
{
  m_self++;
  return *this;
}

MatrixIterWrap
MatrixIterWrap::operator++(int)
{
  MatrixIterWrap retval(m_self);
  m_self++;
  return retval;
}

MatrixIterWrap
MatrixIterWrap::operator+(size diff) const
{
  return MatrixIterWrap(m_self + diff);
}

size
MatrixIterWrap::operator+(MatrixIterWrap other) const
{
  return m_self - other.m_self;
}

MatrixIterWrap&
MatrixIterWrap::operator--()
{
  m_self--;
  return *this;
}

MatrixIterWrap
MatrixIterWrap::operator--(int)
{
  MatrixIterWrap retval(m_self);
  m_self--;
  return retval;
}

MatrixIterWrap
MatrixIterWrap::operator-(size diff) const
{
  return MatrixIterWrap(m_self - diff);
}

size
MatrixIterWrap::operator-(MatrixIterWrap other) const
{
  return m_self - other.m_self;
}

Matrix_iterator
MatrixIterWrap::operator*() const
{
  return Matrix_iterator(m_self);
}

Matrix_iterator
MatrixIterWrap::operator[](size index) const
{
  return Matrix_iterator(m_self + (index));
}

// compares the matrix operator with other and returns bool value
bool
MatrixIterWrap::operator==(MatrixIterWrap other) const
{
  return m_self == other.m_self;
}

bool
MatrixIterWrap::operator!=(MatrixIterWrap other) const
{
  return !operator==(other);
}

bool
MatrixIterWrap::operator<(MatrixIterWrap other) const
{
  return m_self < other.m_self;
}

bool
MatrixIterWrap::operator>(MatrixIterWrap other) const
{
  return m_self > other.m_self;
}

bool
MatrixIterWrap::operator<=(MatrixIterWrap other) const
{
  return m_self <= other.m_self;
}

bool
MatrixIterWrap::operator>=(MatrixIterWrap other) const
{
  return m_self >= other.m_self;
}
