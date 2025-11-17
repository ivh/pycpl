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
 * Wraps the cpl_matrix struct as a C++ class cpl::core::Matrix
 * Implementing all operations that a cpl_matrix can do.
 *
 * This class is optional from the Python programmer's perspective, as they can
 * use a python list, of which there should be an automatic conversion to this
 * matrix (see matrix_conversion.hpp, TODO)
 */

#ifndef PYCPL_MATRIX_HPP
#define PYCPL_MATRIX_HPP

#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <cpl_matrix.h>

#include "cplcore/types.hpp"

namespace cpl
{
namespace core
{
class Matrix_iterator;
class Matrix_const_iterator;
class Matrix_iterator_iterator;
class Matrix_const_iterator_iterator;

using size = cpl::core::size;

/**
 * The elements of a @em cpl_matrix with M rows and N columns are counted
 * from 0,0 to M-1,N-1. The matrix element 0,0 is the one at the upper left
 * corner of a matrix. The CPL matrix functions work properly only in the
 * case the matrices elements do not contain garbage (such as @c NaN or
 * infinity).
 */
class Matrix
{
 public:
  Matrix(cpl_matrix* to_steal) noexcept;
  Matrix& operator=(const Matrix& other);

  /**
   * @brief
   *   Create a new matrix from existing data.
   *
   * @param data     Existing data buffer.
   * @param rows     Number of matrix rows.
   * @param columns  Number of matrix columns.
   *
   * @return Pointer to new matrix, or @c NULL in case of error.
   *
   * This function creates a new matrix that will encapsulate the given
   * data. The size of the input data must exactly match rows * columns,
   * The input  array is supposed to contain in sequence all the new @em Matrix
   * rows. For instance, in the case of a 3x4 matrix, the input array
   * should contain 12 elements
   * @code
   *            0 1 2 3 4 5 6 7 8 9 10 11
   * @endcode
   * that would correspond to the matrix elements
   *            0  1  2  3
   * @code
   *            4  5  6  7
   *            8  9 10 11
   * @endcode
   * The data buffer is copied into this matrix, so it is safe to destruct
   * the buffer after calling this function. Any modifications to the matrix
   * are NOT reflected to the data buffer, and visa versa.
   * @throws IllegalInputError if rows * columns != data.size()
   */
  Matrix(size rows, size columns, const std::vector<double>& data);

  /**
   * @brief Create a zero matrix of given size.
   * @param rows Number of matrix rows.
   * @param columns Number of matrix columns.
   *
   * This function allocates and initialises to zero a matrix of given size.
   * To destroy this matrix the function @c cpl_matrix_delete() should be used.
   *
   * @return Pointer to new matrix, or @c NULL in case of error.
   */
  Matrix(size rows, size columns);

  /**
   * @brief Create a new matrix from existing data.
   * @param rows Number of matrix rows.
   * @param columns Number of matrix columns.
   * @param data Existing data buffer.
   *
   * This function creates a new matrix that will encapsulate the given
   * data. At any error condition, a @c NULL pointer would be returned.
   * Note that the size of the input data array is not checked in any way,
   * and it is expected to match the specified matrix sizes. The input
   * array is supposed to contain in sequence all the new @em cpl_matrix
   * rows. For instance, in the case of a 3x4 matrix, the input array
   * should contain 12 elements
   *
   * @return Pointer to new matrix, or @c NULL in case of error.
   */
  static Matrix wrap(size rows, size columns, double* data);

  /**
   * @brief Delete a matrix.
   *
   * This function frees all the memory associated to a matrix.
   * If @em matrix is @c NULL, nothing is done.
   *
   */
  ~Matrix();

  /**
   * @brief Delete a matrix, but not its data buffer.
   *
   * This function deallocates all the memory associated to a matrix,
   * with the exception of its data buffer. This type of destructor
   * should be used on matrices created with the @c cpl_matrix_wrap()
   * constructor, if the data buffer specified then was not allocated
   * using the functions of the @c cpl_memory module. In such a case, the
   * data buffer should be deallocated separately. See the documentation
   * of the function @c cpl_matrix_wrap(). If @em matrix is @c NULL,
   * nothing is done, and a @c NULL pointer is returned.
   *
   * @return Pointer to the internal data buffer.
   */
  void* unwrap();

  /**
   * @brief Dump a matrix contents into a string.
   *
   * This function is intended just for debugging. It returns a multiline string
   * with the elements of a matrix, ordered in rows and columns.
   *
   * @return String with the matrix contents.
   */
  std::string dump() const;

  /**
   * @brief Get the number of rows of a matrix.
   *
   * Determine the number of rows in a matrix.
   *
   * @return Number of matrix rows, or zero in case of failure.
   */
  size get_nrow() const;

  /**
   * @brief Get the number of columns of a matrix.
   *
   * Determine the number of columns in a matrix.
   *
   * @return Number of matrix columns, or zero in case of failure.
   */
  size get_ncol() const;

  /**
   * @brief Get the pointer to a matrix data buffer, or @c NULL in case of
   * error.
   *
   * A @em cpl_matrix object includes an array of values of type @em double.
   * This function returns a pointer to this internal array, whose first
   * element corresponds to the @em cpl_matrix element 0,0. The internal
   * array contains in sequence all the @em cpl_matrix rows. For instance,
   * in the case of a 3x4 matrix, the array elements
   *
   * @return Pointer to the matrix data buffer.
   */
  double* get_data();

  /**
   * @brief Get the pointer to a matrix data buffer, or @c NULL in case of
   * error.
   *
   * @return Pointer to the matrix data buffer.
   */
  const double* get_data() const;

  /**
   * @brief Get the value of a matrix element.
   * @param row Matrix element row.
   * @param column Matrix element column.
   *
   * Get the value of a matrix element. The matrix rows and columns are
   * counted from 0,0.
   *
   * @return Value of the specified matrix element, or 0.0 in case of error.
   */
  double get(size row, size column) const;

  /**
   * @brief Write a value to a matrix element.
   * @param row Matrix element row.
   * @param column Matrix element column.
   * @param value Value to write.
   *
   * Write a value to a matrix element. The matrix rows and columns are
   * counted from 0,0.
   *
   */
  void set(size row, size column, double value);

  /**
   * @brief Make a copy of a matrix.
   *
   * A copy of the input matrix is created. To destroy the duplicated matrix
   * the function @c cpl_matrix_delete() should be used.
   *
   * @return Pointer to the new matrix, or @c NULL in case of error.
   */
  Matrix(const Matrix& matrix);

  /**
   * @brief Extract a submatrix from a matrix.
   * @param start_row Matrix row where to begin extraction.
   * @param start_column Matrix column where to begin extraction.
   * @param step_row Step between extracted rows.
   * @param step_column Step between extracted columns.
   * @param nrows Number of rows to extract.
   * @param ncolumns Number of columns to extract.
   *
   * The new matrix will include the @em nrows x @em ncolumns values read
   * from the input matrix elements starting from position (@em start_row,
   *
   * @return Pointer to the new matrix, or @c NULL in case of error.
   */
  Matrix extract(size start_row, size start_column, size step_row,
                 size step_column, size nrows, size ncolumns) const;

  /**
   * @brief Extract a matrix row.
   * @param row Sequence number of row to copy.
   *
   * If a MxN matrix is given in input, the extracted row is a new 1xN matrix.
   * The row number is counted from 0. To destroy the new matrix the function
   *
   * @return Pointer to new matrix, or @c NULL in case of error.
   */
  Matrix extract_row(size row) const;

  /**
   * @brief Copy a matrix column.
   * @param column Sequence number of column to copy.
   *
   * If a MxN matrix is given in input, the extracted row is a new Mx1 matrix.
   * The column number is counted from 0. To destroy the new matrix the
   * function @c cpl_matrix_delete() should be used.
   *
   * @return Pointer to new matrix, or @c NULL in case of error.
   */
  Matrix extract_column(size column) const;

  /**
   * @brief Extract a matrix diagonal.
   * @param diagonal Sequence number of the diagonal to copy.
   *
   * If a MxN matrix is given in input, the extracted diagonal is a Mx1
   * matrix if N >= M, or a 1xN matrix if N < M. The diagonal number is
   * counted from 0, corresponding to the matrix diagonal starting at
   * element (0,0). A square matrix has just one diagonal; if M != N,
   * the number of diagonals in the matrix is |M - N| + 1. To specify
   * a @em diagonal sequence number outside this range would set an
   * error condition, and a @c NULL pointer would be returned. To destroy
   * the new matrix the function @c cpl_matrix_delete() should be used.
   *
   * @return Pointer to the new matrix, or @c NULL in case of error.
   */
  Matrix extract_diagonal(size diagonal) const;

  /**
   * @brief Write the values of a matrix into another matrix.
   * @param submatrix Pointer to matrix to get the values from.
   * @param row Position of row 0 of @em submatrix in @em matrix.
   * @param col Position of column 0 of @em submatrix in @em matrix.
   *
   * The values of @em submatrix are written to @em matrix starting at the
   * indicated row and column. There are no restrictions on the sizes of
   *
   */
  void copy(const Matrix& submatrix, size row, size col);

  /**
   * @brief Write the same value to all matrix elements.
   * @param value Value to write
   *
   * Write the same value to all matrix elements.
   *
   */
  void fill(double value);

  /**
   * @brief Write the same value to a matrix row.
   * @param value Value to write
   * @param row Sequence number of row to overwrite.
   *
   * Write the same value to a matrix row. Rows are counted starting from 0.
   *
   */
  void fill_row(double value, size row);

  /**
   * @brief Write the same value to a matrix column.
   * @param value Value to write
   * @param column Sequence number of column to overwrite
   *
   * Write the same value to a matrix column. Columns are counted starting
   * from 0.
   *
   */
  void fill_column(double value, size column);

  /**
   * @brief Write a given value to all elements of a given matrix diagonal.
   * @param value Value to write to diagonal
   * @param diagonal Number of diagonal to overwrite, 0 for main, positive for
   *     above main, negative for below main
   *
   *
   *
   */
  void fill_diagonal(double value, size diagonal);

  /**
   * @brief Write the same value into a submatrix of a matrix.
   * @param value Value to write.
   * @param row Start row of matrix submatrix.
   * @param col Start column of matrix submatrix.
   * @param nrow Number of rows of matrix submatrix.
   * @param ncol Number of columns of matrix submatrix.
   *
   * The specified value is written to @em matrix starting at the indicated
   * row and column; @em nrow and @em ncol can exceed the input matrix
   * boundaries, just the range overlapping the @em matrix is used in that
   * case.
   *
   */
  void fill_window(double value, size row, size col, size nrow, size ncol);

  /**
   * @brief Check for zero matrix.
   * @param tolerance Max tolerated rounding to zero.
   *
   * After specific manipulations of a matrix some of its elements
   * may theoretically be expected to be zero. However, because of
   * numerical noise, such elements may turn out not to be exactly
   * zero. In this specific case, if any of the matrix element is
   * not exactly zero, the matrix would not be classified as a null
   * matrix. A threshold may be specified to consider zero any number
   * that is close enough to zero. If the specified @em tolerance is
   * negative, a default of @c DBL_EPSILON is used. A zero tolerance
   * may also be specified.
   *
   * @return 1 in case of zero matrix, 0 otherwise. If a @c NULL pointer
   *     is passed, -1 is returned.
   */
  bool is_zero(double tolerance) const;

  /**
   * @brief Check if a matrix is diagonal.
   * @param tolerance Max tolerated rounding to zero.
   *
   * A threshold may be specified to consider zero any number that
   * is close enough to zero. If the specified @em tolerance is
   * negative, a default of @c DBL_EPSILON is used. A zero tolerance
   * may also be specified. No error is set if the input matrix is
   * not square.
   *
   * @return 1 in case of diagonal matrix, 0 otherwise. If a @c NULL pointer
   *     is passed, or the input matrix is not square, -1 is returned.
   */
  bool is_diagonal(double tolerance) const;

  /**
   * @brief Check for identity matrix.
   * @param tolerance Max tolerated rounding to zero, or to one.
   *
   * A threshold may be specified to consider zero any number that is
   * close enough to zero, and 1 any number that is close enough to 1.
   * If the specified @em tolerance is negative, a default of @c DBL_EPSILON
   * is used. A zero tolerance may also be specified. No error is set if the
   * input matrix is not square.
   *
   * @return 1 in case of identity matrix, 0 otherwise. If a @c NULL pointer
   *     is passed, or the input matrix is not square, -1 is returned.
   */
  bool is_identity(double tolerance) const;

  /**
   * @brief Sort matrix by rows.
   * @param by_absolute True to sort by absolute value. Default False.
   *
   * The matrix elements of the leftmost column are used as reference for
   * the row sorting, if there are identical the values of the second
   * column are considered, etc. Rows with the greater values go on top.
   * If @em mode is equal to zero, the rows are sorted according to their
   * absolute values (zeroes at bottom).
   *
   */
  void sort_rows(bool by_absolute);

  /**
   * @brief Sort matrix by columns.
   * @param by_absolute True to sort by absolute value. Default False.
   *
   * The matrix elements of the top row are used as reference for
   * the column sorting, if there are identical the values of the second
   * row are considered, etc. Columns with the largest values go on the
   * right. If @em mode is equal to zero, the columns are sorted according
   * to their absolute values (zeroes at left).
   *
   */
  void sort_columns(bool by_absolute);

  /**
   * @brief Rounding to zero very small numbers in matrix.
   * @param tolerance Max tolerated rounding to zero.
   *
   * After specific manipulations of a matrix some of its elements
   * may theoretically be expected to be zero (for instance, as a result
   * of multiplying a matrix by its inverse). However, because of
   * numerical noise, such elements may turn out not to be exactly
   * zero. With this function any very small number in the matrix is
   * turned to exactly zero. If the @em tolerance is zero or negative,
   * a default threshold of @c DBL_EPSILON is used.
   *
   */
  void threshold_small(double tolerance);

  /**
   * @brief Swap two matrix rows.
   * @param row1 One matrix row.
   * @param row2 Another matrix row.
   *
   * The values of two given matrix rows are exchanged. Rows are counted
   * starting from 0. If the same row number is given twice, nothing is
   * done and no error is set.
   *
   */
  void swap_rows(size row1, size row2);

  /**
   * @brief Swap two matrix columns.
   * @param column1 One matrix column.
   * @param column2 Another matrix column.
   *
   * The values of two given matrix columns are exchanged. Columns are
   * counted starting from 0. If the same column number is given twice,
   * nothing is done and no error is set.
   *
   */
  void swap_columns(size column1, size column2);

  /**
   * @brief Swap a matrix column with a matrix row.
   * @param row Matrix row.
   *
   * The values of the indicated row are exchanged with the column having
   * the same sequence number. Rows and columns are counted starting from 0.
   *
   */
  void swap_rowcolumn(size row);

  /**
   * @brief Reverse order of rows in matrix.
   *
   * The order of the rows in the matrix is reversed in place.
   *
   */
  void flip_rows();

  /**
   * @brief Reverse order of columns in matrix.
   *
   * The order of the columns in the matrix is reversed in place.
   *
   */
  void flip_columns();

  /**
   * @brief Delete rows from a matrix.
   * @param start First row to delete.
   * @param count Number of rows to delete.
   *
   * A portion of the matrix data is physically removed. The pointer
   * to matrix data may change, therefore pointers previously retrieved
   * by calling @c cpl_matrix_get_data() should be discarded. The specified
   * segment can extend beyond the end of the matrix, but the attempt to
   * remove all matrix rows is flagged as an error because zero length
   * matrices are illegal. Rows are counted starting from 0.
   *
   */
  void erase_rows(size start, size count);

  /**
   * @brief Delete columns from a matrix.
   * @param start First column to delete.
   * @param count Number of columns to delete.
   *
   * A portion of the matrix data is physically removed. The pointer
   * to matrix data may change, therefore pointers previously retrieved
   * by calling @c cpl_matrix_get_data() should be discarded. The specified
   * segment can extend beyond the end of the matrix, but the attempt to
   * remove all matrix columns is flagged as an error because zero length
   * matrices are illegal. Columns are counted starting from 0.
   *
   */
  void erase_columns(size start, size count);

  /**
   * @brief Resize a matrix.
   * @param rows New number of rows.
   * @param columns New number of columns.
   *
   * The input matrix is resized according to specifications. The old
   * matrix elements contained in the resized matrix are left unchanged,
   * and new matrix elements added by an increase of the matrix number of
   * rows and/or columns are initialised to zero.
   *
   */
  void set_size(size rows, size columns);

  /**
   * @brief Reframe a matrix.
   * @param top Extra rows on top.
   * @param bottom Extra rows on bottom.
   * @param left Extra columns on left.
   * @param right Extra columns on right.
   *
   * The input matrix is reframed according to specifications. Extra rows
   * and column on the sides might also be negative, as long as they are
   * compatible with the matrix sizes: the input matrix would be reduced
   * in size accordingly, but an attempt to remove all matrix columns
   * and/or rows is flagged as an error because zero length matrices are
   * illegal. The old matrix elements contained in the new shape are left
   * unchanged, and new matrix elements added by the reshaping are initialised
   * to zero. No reshaping (i.e., all the extra rows set to zero) would
   * not be flagged as an error.
   *
   */
  void resize(size top, size bottom, size left, size right);

  /**
   * @brief Append a matrix to another.
   * @param matrix2 Pointer to second matrix.
   * @param mode Matrices connected horizontally (0) or vertically (1).
   *
   * If @em mode is set to 0, the matrices must have the same number
   * of rows, and are connected horizontally with the first matrix
   * on the left. If @em mode is set to 1, the matrices must have the
   * same number of columns, and are connected vertically with the
   * first matrix on top. The first matrix is expanded to include the
   * values from the second matrix, while the second matrix is left
   * untouched.
   *
   */
  void append(const Matrix& matrix2, int mode);

  /**
   * @brief Shift matrix elements.
   * @param rshift Shift in the vertical direction.
   * @param cshift Shift in the horizontal direction.
   *
   * The performed shift operation is cyclical (toroidal), i.e., matrix
   * elements shifted out of one side of the matrix get shifted in from
   * its opposite side. There are no restrictions on the values of the
   * shift. Positive shifts are always in the direction of increasing
   * row/column indexes.
   *
   */
  void shift(size rshift, size cshift);

  /**
   * @brief Add two matrices.
   * @param matrix2 Pointer to second matrix.
   *
   * Add two matrices element by element. The two matrices must have
   * identical sizes. The result is written to the first matrix.
   *
   */
  void add(const Matrix& matrix2);

  /**
   * @brief Subtract a matrix from another.
   * @param matrix2 Pointer to second matrix.
   *
   * Subtract the second matrix from the first one element by element.
   * The two matrices must have identical sizes. The result is written
   * to the first matrix.
   *
   */
  void subtract(const Matrix& matrix2);

  /**
   * @brief Multiply two matrices element by element.
   * @param matrix2 Pointer to second matrix.
   *
   * Multiply the two matrices element by element. The two matrices must
   * have identical sizes. The result is written to the first matrix.
   *
   */
  void multiply(const Matrix& matrix2);

  /**
   * @brief Divide a matrix by another element by element.
   * @param matrix2 Pointer to second matrix.
   *
   * Divide each element of the first matrix by the corresponding
   * element of the second one. The two matrices must have the same
   * number of rows and columns. The result is written to the first
   * matrix. No check is made against a division by zero.
   *
   */
  void divide(const Matrix& matrix2);

  /**
   * @brief Add a scalar to a matrix.
   * @param value Value to add.
   *
   * Add the same value to each matrix element.
   *
   */
  void add_scalar(double value);

  /**
   * @brief Subtract a scalar to a matrix.
   * @param value Value to subtract.
   *
   * Subtract the same value to each matrix element.
   *
   */
  void subtract_scalar(double value);

  /**
   * @brief Multiply a matrix by a scalar.
   * @param value Multiplication factor.
   *
   * Multiply each matrix element by the same factor.
   *
   */
  void multiply_scalar(double value);

  /**
   * @brief Divide a matrix by a scalar.
   * @param value Divisor.
   *
   * Divide each matrix element by the same value.
   *
   */
  void divide_scalar(double value);

  /**
   * @brief Compute the logarithm of matrix elements.
   * @param base Logarithm base.
   *
   * Each matrix element is replaced by its logarithm in the specified base.
   * The base and all matrix elements must be positive. If this is not the
   * case, the matrix would not be modified.
   *
   */
  void logarithm(double base);

  /**
   * @brief Compute the exponential of matrix elements.
   * @param base Exponential base.
   *
   * Each matrix element is replaced by its exponential in the specified base.
   * The base must be positive.
   *
   */
  void exponential(double base);

  /**
   * @brief Compute a power of matrix elements.
   * @param exponent Constant exponent.
   *
   * Each matrix element is replaced by its power to the specified
   * exponent. If the specified exponent is not negative, all matrix
   * elements must be not negative; if the specified exponent is
   * negative, all matrix elements must be positive; otherwise, an
   * error condition is set and the matrix will be left unchanged.
   * If the exponent is exactly 0.5 the (faster) @c sqrt() will be
   * applied instead of @c pow(). If the exponent is zero, then any
   * (non negative) matrix element would be assigned the value 1.0.
   *
   */
  void power(double exponent);

  /**
   * @brief Rows-by-columns product of two matrices.
   * @param matrix2 Pointer to right side matrix.
   *
   * Rows-by-columns product of two matrices. The number of columns of the
   * first matrix must be equal to the number of rows of the second matrix.
   * To destroy the new matrix the function @c cpl_matrix_delete() should
   * be used.
   *
   * @return Pointer to product matrix, or @c NULL in case of error.
   */
  Matrix product_create(const Matrix& matrix2) const;

  /**
   * @brief Create transposed matrix.
   *
   * The transposed of the input matrix is created. To destroy the new matrix
   * the function @c cpl_matrix_delete() should be used.
   *
   * @return Pointer to transposed matrix. If a @c NULL pointer is passed,
   *     a @c NULL pointer is returned.
   */
  Matrix transpose_create() const;

  /**
   * @brief Compute A = B * transpose(B)
   * @param other M x N Matrix to multiply with its transpose
   *
   * </table>
   *
   * @return The newly created Matrix
   */
  Matrix product_normal();

  /**
   * @brief Fill a matrix with the product of A * B'
   * @param ma The matrix A, of size M x K
   * @param mb The matrix B, of size N x K
   *
   */
  void product_transpose(const Matrix& ma, const Matrix& mb);

  /**
   * @brief Compute the determinant of a matrix.
   *
   * The input matrix must be a square matrix. In case of a 1x1 matrix,
   * the matrix single element value is returned.
   *
   * @return Matrix determinant. In case of error, 0.0 is returned.
   */
  double get_determinant() const;

  /**
   * @brief Solution of a linear system.
   * @param coeff non-singlar N by N matrix
   * @param rhs A matrix containing one or more right-hand-sides.
   *
   * Compute the solution of a system of N equations with N unknowns:
   *
   * coeff * X = rhs
   *
   * @return A newly allocated solution matrix with the size as rhs,
   *     or @c NULL on error.
   */
  static Matrix solve(const Matrix& coeff, const Matrix& rhs);

  /**
   * @brief Solution of overdetermined linear equations in a least squares
   * sense.
   * @param coeff An N by M matrix of linear system coefficients, where N >= M
   * @param rhs An N by K matrix containing K right-hand-sides.
   *
   * The following linear system of N equations and M unknowns
   * is given:
   *
   * where @em coeff is the NxM matrix of the coefficients, @em X is the
   * MxK matrix of the unknowns, and @em rhs the NxK matrix containing
   * the K right hand side(s).
   *
   * The solution to the normal equations is known to be a least-squares
   * solution, i.e. the 2-norm of coeff * X - rhs is minimized by the
   * solution to
   * transpose(coeff) * coeff * X = transpose(coeff) * rhs.
   *
   * In the case that coeff is square (N is equal to M) it gives a faster
   * and more accurate result to use cpl_matrix_solve().
   *
   * The solution matrix should be deallocated with the function
   *
   * @return A newly allocated M by K solution matrix,
   *     or @c NULL on error.
   */
  static Matrix solve_normal(const Matrix& coeff, const Matrix& rhs);

  /**
   * @brief Solve a linear system in a least square sense using an SVD
   * factorization.
   * @param coeff  An N by M matrix of linear system coefficients, where N >= M
   * @param rhs An N by 1 matrix with the right hand side of the system
   * @param mode       Cutoff mode selector for small singular values. Optional
   *                   and will not threshold if not given
   * @param tolerance  Factor used to compute the cutoff value if mode is
   *                   set to @c 2.
   *
   * The function solves a linear system of the form Ax = b for the solution
   * vector x, where @c A is represented by the argument @em coeff and @c b
   * by the argument @em rhs. The linear system is solved using the singular
   * value decomposition (SVD) of the coefficient matrix, based on a one-sided
   * Jacobi orthogonalization.
   *
   * The returned solution matrix should be deallocated using
   *
   * @return A newly allocated M by 1 matrix containing the solution vector or
   */
  static Matrix solve_svd(const Matrix& coeff, const Matrix& rhs,
                          std::optional<int> mode, double tolerance = 0);

  /**
   * @brief Find a matrix inverse.
   *
   * The input must be a square matrix. To destroy the new matrix the
   * function @c cpl_matrix_delete() should be used.
   *
   * @return Inverse matrix. In case of error a @c NULL is returned.
   */
  Matrix invert_create() const;

  /**
   * @brief Replace a matrix by its LU-decomposition
   * @param perm n-integer array to be filled with the row permutations
   * @param psig On success set to 1/-1 for an even/odd number of permutations
   *
   * @return Pair of n-integeter array filled with row permutations, true for
   * even number of permutations
   */
  std::pair<std::vector<int>, bool> decomp_lu();

  /**
   * @brief Solve a LU-system
   * @param rhs m right-hand-sides to be duplicated and have the solution
   * applied to
   * @param perm n-integer array filled with the row permutations, or NULL
   *
   * @return The solution of `self` as applied to `rhs`
   */
  Matrix solve_lu(Matrix& rhs, std::optional<std::vector<int>> perm) const;

  /**
   * @brief Replace a matrix by its Cholesky-decomposition, L * transpose(L) = A
   *
   */
  void decomp_chol();

  /**
   * @brief Solve a L*transpose(L)-system
   * @param rhs M right-hand-sides to be replaced by their solution
   *
   */
  void solve_chol(Matrix& rhs) const;

  /**
   * @brief Find the mean of all matrix elements.
   *
   * The mean value of all matrix elements is calculated.
   *
   * @return Mean. In case of error 0.0 is returned.
   */
  double get_mean() const;

  /**
   * @brief Find the median of matrix elements.
   *
   * The median value of all matrix elements is calculated.
   *
   * @return Median. In case of error 0.0 is returned.
   */
  double get_median() const;

  /**
   * @brief Find the standard deviation of matrix elements.
   *
   * The standard deviation of all matrix elements is calculated.
   *
   * @return Standard deviation. In case of error, or if a matrix is
   *     1x1, 0.0 is returned.
   */
  double get_stdev() const;

  /**
   * @brief Find the minimum value of matrix elements.
   *
   * The minimum value of all matrix elements is found.
   *
   * @return Minimum value. In case of error, 0.0 is returned.
   */
  double get_min() const;

  /**
   * @brief Find the maximum value of matrix elements.
   *
   * The maximum value of all matrix elements is found.
   *
   * @return Maximum value. In case of error, 0.0 is returned.
   */
  double get_max() const;

  /**
   * @brief Find position of minimum value of matrix elements.
   * @return first: row position of minimum.
   *         second: column position of minimum.
   *
   * The position of the minimum value of all matrix elements is found.
   * If more than one matrix element have a value corresponding to
   * the minimum, the lowest element row number is returned in @em row.
   * If more than one minimum matrix elements have the same row number,
   * the lowest element column number is returned in @em column.
   */
  std::pair<size, size> get_minpos() const;

  Matrix duplicate();

  /**
   * @brief Find position of the maximum value of matrix elements.
   * @return first: row position of maximum.
   *         second: column position of maximum.
   *
   * The position of the maximum value of all matrix elements is found.
   * If more than one matrix element have a value corresponding to
   * the maximum, the lowest element row number is returned in @em row.
   * If more than one maximum matrix elements have the same row number,
   * the lowest element column number is returned in @em column.
   */
  std::pair<size, size> get_maxpos() const;

  bool operator==(const Matrix& other) const;
  bool operator!=(const Matrix& other) const;

  const cpl_matrix* ptr() const;
  cpl_matrix* ptr();

  /**
   * @brief Relieves self Matrix of ownership of the underlying cpl_matrix*
   * pointer, if it is owned.
   *
   * This is a counterpart to Matrix(cpl_matrix *to_steal);
   *
   * @note Make sure to use cpl_matrix_delete to delete the returned
   * cpl_matrix*, or turn it back into a Matrix with Matrix(cpl_matrix
   * *to_steal)
   */
  static cpl_matrix* unwrap(Matrix&& self);

  typedef class Matrix_iterator iterator;
  typedef class Matrix_const_iterator const_iterator;

  Matrix_iterator begin();
  Matrix_const_iterator begin() const;
  Matrix_iterator end();
  Matrix_const_iterator end() const;

 private:
  cpl_matrix* m_interface;
};

class Matrix_iterator
{
  double* m_data;
  size m_ncol;

 public:
  explicit Matrix_iterator(double* data, size ncol) : m_data(data), m_ncol(ncol)
  {
  }

  Matrix_iterator& operator++();
  Matrix_iterator operator++(int);
  Matrix_iterator operator+(size diff) const;
  size operator+(Matrix_iterator other) const;
  Matrix_iterator& operator--();
  Matrix_iterator operator--(int);
  Matrix_iterator operator-(size diff) const;
  size operator-(Matrix_iterator other) const;
  Matrix_iterator_iterator operator*() const;
  Matrix_iterator_iterator operator[](size index) const;
  bool operator==(Matrix_iterator other) const;
  bool operator!=(Matrix_iterator other) const;
  bool operator<(Matrix_iterator other) const;
  bool operator>(Matrix_iterator other) const;
  bool operator<=(Matrix_iterator other) const;
  bool operator>=(Matrix_iterator other) const;

  typedef class Matrix_iterator_iterator iterator;
  typedef class Matrix_const_iterator_iterator const_iterator;

  Matrix_iterator_iterator begin();
  Matrix_const_iterator_iterator begin() const;
  Matrix_iterator_iterator end();
  Matrix_const_iterator_iterator end() const;

  using difference_type = size;
  using value_type = Matrix_iterator_iterator;
  using pointer = void;  // Doesn't make sense here, so I do void -
                         // https://stackoverflow.com/a/44019327
  using reference = Matrix_iterator_iterator;
  using iterator_category = std::random_access_iterator_tag;
};

class Matrix_const_iterator
{
  const double* m_data;
  size m_ncol;

 public:
  explicit Matrix_const_iterator(const double* data, size ncol)
      : m_data(data), m_ncol(ncol)
  {
  }

  Matrix_const_iterator& operator++();
  Matrix_const_iterator operator++(int);
  Matrix_const_iterator operator+(size diff) const;
  size operator+(Matrix_const_iterator other) const;
  Matrix_const_iterator& operator--();
  Matrix_const_iterator operator--(int);
  Matrix_const_iterator operator-(size diff) const;
  size operator-(Matrix_const_iterator other) const;
  Matrix_const_iterator_iterator operator*() const;
  Matrix_const_iterator_iterator operator[](size index) const;
  bool operator==(Matrix_const_iterator other) const;
  bool operator!=(Matrix_const_iterator other) const;
  bool operator<(Matrix_const_iterator other) const;
  bool operator>(Matrix_const_iterator other) const;
  bool operator<=(Matrix_const_iterator other) const;
  bool operator>=(Matrix_const_iterator other) const;

  typedef class Matrix_const_iterator_iterator iterator;
  typedef class Matrix_const_iterator_iterator const_iterator;

  Matrix_const_iterator_iterator begin() const;
  Matrix_const_iterator_iterator end() const;

  using difference_type = const double*;
  using value_type = Matrix_iterator_iterator;
  using pointer = void;  // Doesn't make sense here, so I do void -
                         // https://stackoverflow.com/a/44019327
  using reference = Matrix_const_iterator_iterator;
  using iterator_category = std::random_access_iterator_tag;
};

class Matrix_iterator_iterator
{
  double* m_data;

 public:
  explicit Matrix_iterator_iterator(double* data) : m_data(data) {}

  Matrix_iterator_iterator& operator++();
  Matrix_iterator_iterator operator++(int);
  Matrix_iterator_iterator operator+(std::ptrdiff_t diff) const;
  std::ptrdiff_t operator+(Matrix_iterator_iterator other) const;
  Matrix_iterator_iterator& operator--();
  Matrix_iterator_iterator operator--(int);
  Matrix_iterator_iterator operator-(std::ptrdiff_t diff) const;
  std::ptrdiff_t operator-(Matrix_iterator_iterator other) const;
  double& operator*() const;
  double& operator[](std::ptrdiff_t index) const;
  bool operator==(Matrix_iterator_iterator other) const;
  bool operator!=(Matrix_iterator_iterator other) const;
  bool operator<(Matrix_iterator_iterator other) const;
  bool operator>(Matrix_iterator_iterator other) const;
  bool operator<=(Matrix_iterator_iterator other) const;
  bool operator>=(Matrix_iterator_iterator other) const;

  // iterator traits
  using difference_type = double;
  using value_type = double;
  using pointer = double*;
  using reference = double&;
  using iterator_category = std::random_access_iterator_tag;
};

class Matrix_const_iterator_iterator
{
  const double* m_data;

 public:
  explicit Matrix_const_iterator_iterator(const double* data) : m_data(data) {}

  Matrix_const_iterator_iterator& operator++();
  Matrix_const_iterator_iterator operator++(int);
  Matrix_const_iterator_iterator operator+(std::ptrdiff_t diff) const;
  std::ptrdiff_t operator+(Matrix_const_iterator_iterator other) const;
  Matrix_const_iterator_iterator& operator--();
  Matrix_const_iterator_iterator operator--(int);
  Matrix_const_iterator_iterator operator-(std::ptrdiff_t diff) const;
  std::ptrdiff_t operator-(Matrix_const_iterator_iterator other) const;
  const double& operator*() const;
  const double& operator[](std::ptrdiff_t index) const;
  bool operator==(Matrix_const_iterator_iterator other) const;
  bool operator!=(Matrix_const_iterator_iterator other) const;
  bool operator<(Matrix_const_iterator_iterator other) const;
  bool operator>(Matrix_const_iterator_iterator other) const;
  bool operator<=(Matrix_const_iterator_iterator other) const;
  bool operator>=(Matrix_const_iterator_iterator other) const;

  // iterator traits
  using difference_type = std::ptrdiff_t;
  using value_type = double;
  using pointer = const double*;
  using reference = const double&;
  using iterator_category = std::random_access_iterator_tag;
};

}  // namespace core
}  // namespace cpl

#endif  // PYCPL_MATRIX_HPP