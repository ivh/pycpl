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

#include "cplcore/matrix.hpp"

#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

#include <cpl_memory.h>

#include "cplcore/array.hpp"
#include "cplcore/error.hpp"

namespace cpl
{
namespace core
{

Matrix::Matrix(cpl_matrix* to_steal) noexcept : m_interface(to_steal) {}

Matrix&
Matrix::operator=(const Matrix& other)
{
  cpl_matrix_delete(m_interface);
  m_interface =
      Error::throw_errors_with(cpl_matrix_duplicate, other.m_interface);
  return *this;
}

Matrix::Matrix(size rows, size columns)
    : m_interface(Error::throw_errors_with(cpl_matrix_new, rows, columns))
{
}

Matrix::Matrix(size rows, size columns, const std::vector<double>& data)
{
  using size_type = std::vector<double>::size_type;

  size_type n_elements = static_cast<size_type>(rows * columns);
  if (data.size() == n_elements) {
    // Copy data buffer so that the caller does not have to keep the vector
    // allocated. Calloc copied from cpl_matrix.c

    // FIXME: Returned pointer is unchecked!
    double* copied_data =
        reinterpret_cast<double*>(cpl_calloc(n_elements, sizeof(double)));
    memcpy(copied_data, &data[0], data.size() * sizeof(double));
    m_interface =
        Error::throw_errors_with(cpl_matrix_wrap, rows, columns, copied_data);
  } else {
    throw cpl::core::IllegalInputError(
        PYCPL_ERROR_LOCATION, "Matrix data size must match rows * columns");
  }
}

Matrix::~Matrix() { Error::throw_errors_with(cpl_matrix_delete, m_interface); }

std::string
Matrix::dump() const
{
  char* charBuff;
  size_t len;
  FILE* stream = open_memstream(&charBuff, &len);  // Open char pointer as
                                                   // stream
  Error::throw_errors_with(cpl_matrix_dump, m_interface, stream);
  fflush(stream);                      // Flush to char pointer
  std::string returnString(charBuff);  // Cast to std::string
  fclose(stream);
  free(charBuff);

  return returnString;
}

size
Matrix::get_nrow() const
{
  return Error::throw_errors_with(cpl_matrix_get_nrow, m_interface);
}

size
Matrix::get_ncol() const
{
  return Error::throw_errors_with(cpl_matrix_get_ncol, m_interface);
}

double*
Matrix::get_data()
{
  return Error::throw_errors_with(cpl_matrix_get_data, m_interface);
}

const double*
Matrix::get_data() const
{
  return Error::throw_errors_with(cpl_matrix_get_data_const, m_interface);
}

double
Matrix::get(size row, size column) const
{
  return Error::throw_errors_with(cpl_matrix_get, m_interface, row, column);
}

void
Matrix::set(size row, size column, double value)
{
  Error::throw_errors_with(cpl_matrix_set, m_interface, row, column, value);
}

Matrix::Matrix(const Matrix& matrix)
    : m_interface(Error::throw_errors_with(cpl_matrix_duplicate, matrix.ptr()))
{
}

Matrix
Matrix::extract(size start_row, size start_column, size step_row,
                size step_column, size nrows, size ncolumns) const
{
  return Error::throw_errors_with(cpl_matrix_extract, m_interface, start_row,
                                  start_column, step_row, step_column, nrows,
                                  ncolumns);
}

Matrix
Matrix::extract_row(size row) const
{
  return Error::throw_errors_with(cpl_matrix_extract_row, m_interface, row);
}

Matrix
Matrix::extract_column(size column) const
{
  return Error::throw_errors_with(cpl_matrix_extract_column, m_interface,
                                  column);
}

Matrix
Matrix::extract_diagonal(size diagonal) const
{
  return Error::throw_errors_with(cpl_matrix_extract_diagonal, m_interface,
                                  diagonal);
}

void
Matrix::copy(const Matrix& submatrix, size row, size col)
{
  Error::throw_errors_with(cpl_matrix_copy, m_interface, submatrix.ptr(), row,
                           col);
}

void
Matrix::fill(double value)
{
  Error::throw_errors_with(cpl_matrix_fill, m_interface, value);
}

void
Matrix::fill_row(double value, size row)
{
  Error::throw_errors_with(cpl_matrix_fill_row, m_interface, value, row);
}

void
Matrix::fill_column(double value, size column)
{
  Error::throw_errors_with(cpl_matrix_fill_column, m_interface, value, column);
}

void
Matrix::fill_diagonal(double value, size diagonal)
{
  Error::throw_errors_with(cpl_matrix_fill_diagonal, m_interface, value,
                           diagonal);
}

void
Matrix::fill_window(double value, size row, size col, size nrow, size ncol)
{
  Error::throw_errors_with(cpl_matrix_fill_window, m_interface, value, row, col,
                           nrow, ncol);
}

bool
Matrix::is_zero(double tolerance) const
{
  // Check if 1 since function can return -1. Unlikely as it'll probably throw
  // and error but just in case.
  int res =
      Error::throw_errors_with(cpl_matrix_is_zero, m_interface, tolerance);
  if (res == 1) {
    return true;
  } else if (res == 0) {
    return false;
  } else {
    // Returns -1 for null input. Probably wont happen but to be safe
    throw cpl::core::NullInputError(PYCPL_ERROR_LOCATION,
                                    "Input matrix is a null pointer");
  }
}

bool
Matrix::is_diagonal(double tolerance) const
{
  int res =
      Error::throw_errors_with(cpl_matrix_is_diagonal, m_interface, tolerance);
  if (res == 1) {
    return true;
  } else if (res == 0) {
    return false;
  } else {
    // Returns -1 if matrix is not square
    throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                       "Matrix is not square");
  }
}

bool
Matrix::is_identity(double tolerance) const
{
  int res =
      Error::throw_errors_with(cpl_matrix_is_identity, m_interface, tolerance);
  if (res == 1) {
    return true;
  } else if (res == 0) {
    return false;
  } else {
    // Returns -1 if matrix is not square
    throw cpl::core::IllegalInputError(PYCPL_ERROR_LOCATION,
                                       "Matrix is not square");
  }
}

void
Matrix::sort_rows(bool by_absolute)
{
  // Python API takes boolean. Convert to CPL function mode (0 to sort by
  // absolute, otherwise sort by value)
  int mode = by_absolute ? 0 : 1;
  Error::throw_errors_with(cpl_matrix_sort_rows, m_interface, mode);
}

void
Matrix::sort_columns(bool by_absolute)
{
  // Python API takes boolean. Convert to CPL function mode (0 to sort by
  // absolute, otherwise sort by value)
  int mode = by_absolute ? 0 : 1;
  Error::throw_errors_with(cpl_matrix_sort_columns, m_interface, mode);
}

void
Matrix::threshold_small(double tolerance)
{
  Error::throw_errors_with(cpl_matrix_threshold_small, m_interface, tolerance);
}

void
Matrix::swap_rows(size row1, size row2)
{
  Error::throw_errors_with(cpl_matrix_swap_rows, m_interface, row1, row2);
}

void
Matrix::swap_columns(size column1, size column2)
{
  Error::throw_errors_with(cpl_matrix_swap_columns, m_interface, column1,
                           column2);
}

void
Matrix::swap_rowcolumn(size row)
{
  Error::throw_errors_with(cpl_matrix_swap_rowcolumn, m_interface, row);
}

void
Matrix::flip_rows()
{
  Error::throw_errors_with(cpl_matrix_flip_rows, m_interface);
}

void
Matrix::flip_columns()
{
  Error::throw_errors_with(cpl_matrix_flip_columns, m_interface);
}

void
Matrix::erase_rows(size start, size count)
{
  Error::throw_errors_with(cpl_matrix_erase_rows, m_interface, start, count);
}

void
Matrix::erase_columns(size start, size count)
{
  Error::throw_errors_with(cpl_matrix_erase_columns, m_interface, start, count);
}

void
Matrix::set_size(size rows, size columns)
{
  Error::throw_errors_with(cpl_matrix_set_size, m_interface, rows, columns);
}

void
Matrix::resize(size top, size bottom, size left, size right)
{
  Error::throw_errors_with(cpl_matrix_resize, m_interface, top, bottom, left,
                           right);
}

void
Matrix::append(const Matrix& matrix2, int mode)
{
  Error::throw_errors_with(cpl_matrix_append, m_interface, matrix2.ptr(), mode);
}

void
Matrix::shift(size rshift, size cshift)
{
  Error::throw_errors_with(cpl_matrix_shift, m_interface, rshift, cshift);
}

void
Matrix::add(const Matrix& matrix2)
{
  Error::throw_errors_with(cpl_matrix_add, m_interface, matrix2.ptr());
}

void
Matrix::subtract(const Matrix& matrix2)
{
  Error::throw_errors_with(cpl_matrix_subtract, m_interface, matrix2.ptr());
}

void
Matrix::multiply(const Matrix& matrix2)
{
  Error::throw_errors_with(cpl_matrix_multiply, m_interface, matrix2.ptr());
}

void
Matrix::divide(const Matrix& matrix2)
{
  Error::throw_errors_with(cpl_matrix_divide, m_interface, matrix2.ptr());
}

void
Matrix::add_scalar(double value)
{
  Error::throw_errors_with(cpl_matrix_add_scalar, m_interface, value);
}

void
Matrix::subtract_scalar(double value)
{
  Error::throw_errors_with(cpl_matrix_subtract_scalar, m_interface, value);
}

void
Matrix::multiply_scalar(double value)
{
  Error::throw_errors_with(cpl_matrix_multiply_scalar, m_interface, value);
}

void
Matrix::divide_scalar(double value)
{
  Error::throw_errors_with(cpl_matrix_divide_scalar, m_interface, value);
}

void
Matrix::logarithm(double base)
{
  Error::throw_errors_with(cpl_matrix_logarithm, m_interface, base);
}

void
Matrix::exponential(double base)
{
  Error::throw_errors_with(cpl_matrix_exponential, m_interface, base);
}

void
Matrix::power(double exponent)
{
  Error::throw_errors_with(cpl_matrix_power, m_interface, exponent);
}

Matrix
Matrix::product_create(const Matrix& matrix2) const
{
  return Error::throw_errors_with(cpl_matrix_product_create, m_interface,
                                  matrix2.ptr());
}

Matrix
Matrix::transpose_create() const
{
  return Error::throw_errors_with(cpl_matrix_transpose_create, m_interface);
}

Matrix
Matrix::product_normal()
{
  /*
      self × transpose(self)
      Matrix multiplication results in a matrix of the size
      [rows of left] × [columns of right]
      Here, left = self, right = transpose(self)
      and the rows/columns of a transpose(self) are flipped from a self
      so the result of the multiplication is [rows of self] × [columns of
     transpose(self)], simplifies into  [rows of self] × [rows of self]
  */
  size self_rows = get_nrow();
  Matrix result(self_rows, self_rows);
  Error::throw_errors_with(cpl_matrix_product_normal, result.ptr(),
                           m_interface);
  return result;
}

void
Matrix::product_transpose(const Matrix& ma, const Matrix& mb)
{
  Error::throw_errors_with(cpl_matrix_product_transpose, m_interface, ma.ptr(),
                           mb.ptr());
}

double
Matrix::get_determinant() const
{
  return Error::throw_errors_with(cpl_matrix_get_determinant, m_interface);
}

Matrix
Matrix::solve(const Matrix& coeff, const Matrix& rhs)
{
  return Error::throw_errors_with(cpl_matrix_solve, coeff.ptr(), rhs.ptr());
}

Matrix
Matrix::solve_normal(const Matrix& coeff, const Matrix& rhs)
{
  return Error::throw_errors_with(cpl_matrix_solve_normal, coeff.ptr(),
                                  rhs.ptr());
}

Matrix
Matrix::solve_svd(const Matrix& coeff, const Matrix& rhs,
                  std::optional<int> mode, double tolerance)
{
  if (mode.has_value()) {
    return Error::throw_errors_with(cpl_matrix_solve_svd_threshold, coeff.ptr(),
                                    rhs.ptr(), mode.value(), tolerance);
  }
  return Error::throw_errors_with(cpl_matrix_solve_svd, coeff.ptr(), rhs.ptr());
}

Matrix
Matrix::invert_create() const
{
  return Error::throw_errors_with(cpl_matrix_invert_create, m_interface);
}

std::pair<std::vector<int>, bool>
Matrix::decomp_lu()
{
  if (get_ncol() != get_nrow()) {
    throw IllegalInputError(PYCPL_ERROR_LOCATION,
                            "decomp_lu requires a square matrix");
  }
  size n = get_ncol();
  std::pair<std::vector<int>, bool> retval;
  // Output vector's values don't matter, but size does.
  retval.first = std::vector<int>(n, 0);
  int psig;

  // Wrap the output vector in a cpl_array
  cpl_array* perm = cpl_array_wrap_int(&retval.first[0], n);

  Error::throw_errors_with(cpl_matrix_decomp_lu, m_interface, perm, &psig);

  cpl_array_unwrap(perm);
  retval.second = psig == 1;  // even then true.

  return retval;
}

Matrix
Matrix::solve_lu(Matrix& rhs, std::optional<std::vector<int>> perm) const
{
  cpl::core::Matrix rhs_dup = rhs.duplicate();
  if (perm.has_value()) {
    auto perm_arr = vector_as_temp_array_int(perm.value());
    Error::throw_errors_with(cpl_matrix_solve_lu, m_interface, rhs_dup.ptr(),
                             perm_arr.get());
  } else {
    Error::throw_errors_with(cpl_matrix_solve_lu, m_interface, rhs_dup.ptr(),
                             (cpl_array*)NULL);
  }
  return rhs_dup;
}

void
Matrix::decomp_chol()
{
  Error::throw_errors_with(cpl_matrix_decomp_chol, m_interface);
}

void
Matrix::solve_chol(Matrix& rhs) const
{
  Error::throw_errors_with(cpl_matrix_solve_chol, m_interface, rhs.ptr());
}

double
Matrix::get_mean() const
{
  return Error::throw_errors_with(cpl_matrix_get_mean, m_interface);
}

double
Matrix::get_median() const
{
  return Error::throw_errors_with(cpl_matrix_get_median, m_interface);
}

double
Matrix::get_stdev() const
{
  return Error::throw_errors_with(cpl_matrix_get_stdev, m_interface);
}

double
Matrix::get_min() const
{
  return Error::throw_errors_with(cpl_matrix_get_min, m_interface);
}

double
Matrix::get_max() const
{
  return Error::throw_errors_with(cpl_matrix_get_max, m_interface);
}

std::pair<size, size>
Matrix::get_minpos() const
{
  std::pair<size, size> retval;
  Error::throw_errors_with(cpl_matrix_get_minpos, m_interface, &retval.first,
                           &retval.second);
  return retval;
}

std::pair<size, size>
Matrix::get_maxpos() const
{
  std::pair<size, size> retval;
  Error::throw_errors_with(cpl_matrix_get_maxpos, m_interface, &retval.first,
                           &retval.second);
  return retval;
}

// Checking if rows and cols for equal length and identi in LHS and RHS
bool
Matrix::operator==(const Matrix& other) const
{
  size nrow = other.get_nrow();
  size ncol = other.get_ncol();
  return get_nrow() == nrow && get_ncol() == ncol &&
         std::memcmp(get_data(), other.get_data(),
                     nrow * ncol * sizeof(double)) == 0;
}

Matrix
Matrix::duplicate()
{
  return Matrix(Error::throw_errors_with(cpl_matrix_duplicate, m_interface));
}

bool
Matrix::operator!=(const Matrix& other) const
{
  return !operator==(other);
}

const cpl_matrix*
Matrix::ptr() const
{
  return m_interface;
}

cpl_matrix*
Matrix::ptr()
{
  return m_interface;
}

cpl_matrix*
Matrix::unwrap(Matrix&& self)
{
  cpl_matrix* interface = self.m_interface;
  self.m_interface = nullptr;
  return interface;
}

Matrix_iterator&
Matrix_iterator::operator++()
{
  m_data += m_ncol;
  return *this;
}

Matrix_iterator
Matrix_iterator::operator++(int)
{
  Matrix_iterator retval(m_data, m_ncol);
  m_data += m_ncol;
  return retval;
}

Matrix_iterator
Matrix_iterator::operator+(size diff) const
{
  return Matrix_iterator(m_data + m_ncol * diff, m_ncol);
}

size
Matrix_iterator::operator+(Matrix_iterator other) const
{
  return m_data - other.m_data;
}

Matrix_iterator&
Matrix_iterator::operator--()
{
  m_data -= m_ncol;
  return *this;
}

Matrix_iterator
Matrix_iterator::operator--(int)
{
  Matrix_iterator retval(m_data, m_ncol);
  m_data -= m_ncol;
  return retval;
}

Matrix_iterator
Matrix_iterator::operator-(size diff) const
{
  return Matrix_iterator(m_data - m_ncol * diff, m_ncol);
}

size
Matrix_iterator::operator-(Matrix_iterator other) const
{
  return m_data - other.m_data;
}

Matrix_iterator_iterator
Matrix_iterator::operator*() const
{
  return Matrix_iterator_iterator(m_data);
}

Matrix_iterator_iterator
Matrix_iterator::operator[](size index) const
{
  return Matrix_iterator_iterator(m_data + (m_ncol * index));
}

// Checking  matrix iterator operator with other and returning bool
bool
Matrix_iterator::operator==(Matrix_iterator other) const
{
  return m_data == other.m_data;
}

bool
Matrix_iterator::operator!=(Matrix_iterator other) const
{
  return !operator==(other);
}

bool
Matrix_iterator::operator<(Matrix_iterator other) const
{
  return m_data < other.m_data;
}

bool
Matrix_iterator::operator>(Matrix_iterator other) const
{
  return m_data > other.m_data;
}

bool
Matrix_iterator::operator<=(Matrix_iterator other) const
{
  return m_data <= other.m_data;
}

bool
Matrix_iterator::operator>=(Matrix_iterator other) const
{
  return m_data >= other.m_data;
}

Matrix_iterator_iterator
Matrix_iterator::begin()
{
  return Matrix_iterator_iterator(m_data);
}

Matrix_const_iterator_iterator
Matrix_iterator::begin() const
{
  return Matrix_const_iterator_iterator(m_data);
}

Matrix_iterator_iterator
Matrix_iterator::end()
{
  return Matrix_iterator_iterator(m_data + m_ncol);
}

Matrix_const_iterator_iterator
Matrix_iterator::end() const
{
  return Matrix_const_iterator_iterator(m_data + m_ncol);
}

Matrix_const_iterator&
Matrix_const_iterator::operator++()
{
  m_data += m_ncol;
  return *this;
}

Matrix_const_iterator
Matrix_const_iterator::operator++(int)
{
  Matrix_const_iterator retval(m_data, m_ncol);
  m_data += m_ncol;
  return retval;
}

Matrix_const_iterator
Matrix_const_iterator::operator+(size diff) const
{
  return Matrix_const_iterator(m_data + m_ncol * diff, m_ncol);
}

size
Matrix_const_iterator::operator+(Matrix_const_iterator other) const
{
  return m_data - other.m_data;
}

Matrix_const_iterator&
Matrix_const_iterator::operator--()
{
  m_data -= m_ncol;
  return *this;
}

Matrix_const_iterator
Matrix_const_iterator::operator--(int)
{
  Matrix_const_iterator retval(m_data, m_ncol);
  m_data -= m_ncol;
  return retval;
}

Matrix_const_iterator
Matrix_const_iterator::operator-(size diff) const
{
  return Matrix_const_iterator(m_data - m_ncol * diff, m_ncol);
}

size
Matrix_const_iterator::operator-(Matrix_const_iterator other) const
{
  return m_data - other.m_data;
}

Matrix_const_iterator_iterator
Matrix_const_iterator::operator*() const
{
  return Matrix_const_iterator_iterator(m_data);
}

Matrix_const_iterator_iterator
Matrix_const_iterator::operator[](size index) const
{
  return Matrix_const_iterator_iterator(m_data + (m_ncol * index));
}

// Checking Arithmetically operator mdata with other and returning bool
bool
Matrix_const_iterator::operator==(Matrix_const_iterator other) const
{
  return m_data == other.m_data;
}

bool
Matrix_const_iterator::operator!=(Matrix_const_iterator other) const
{
  return !operator==(other);
}

bool
Matrix_const_iterator::operator<(Matrix_const_iterator other) const
{
  return m_data < other.m_data;
}

bool
Matrix_const_iterator::operator>(Matrix_const_iterator other) const
{
  return m_data > other.m_data;
}

bool
Matrix_const_iterator::operator<=(Matrix_const_iterator other) const
{
  return m_data <= other.m_data;
}

bool
Matrix_const_iterator::operator>=(Matrix_const_iterator other) const
{
  return m_data >= other.m_data;
}

Matrix_const_iterator_iterator
Matrix_const_iterator::begin() const
{
  return Matrix_const_iterator_iterator(m_data);
}

Matrix_const_iterator_iterator
Matrix_const_iterator::end() const
{
  return Matrix_const_iterator_iterator(m_data + m_ncol);
}

Matrix_iterator_iterator&
Matrix_iterator_iterator::operator++()
{
  ++m_data;
  return *this;
}

Matrix_iterator_iterator
Matrix_iterator_iterator::operator++(int)
{
  return Matrix_iterator_iterator(m_data++);
}

Matrix_iterator_iterator
Matrix_iterator_iterator::operator+(std::ptrdiff_t diff) const
{
  return Matrix_iterator_iterator(m_data + diff);
}

std::ptrdiff_t
Matrix_iterator_iterator::operator+(Matrix_iterator_iterator other) const
{
  return m_data - other.m_data;
}

Matrix_iterator_iterator&
Matrix_iterator_iterator::operator--()
{
  --m_data;
  return *this;
}

Matrix_iterator_iterator
Matrix_iterator_iterator::operator--(int)
{
  return Matrix_iterator_iterator(m_data--);
}

Matrix_iterator_iterator
Matrix_iterator_iterator::operator-(std::ptrdiff_t diff) const
{
  return Matrix_iterator_iterator(m_data - diff);
}

std::ptrdiff_t
Matrix_iterator_iterator::operator-(Matrix_iterator_iterator other) const
{
  return m_data - other.m_data;
}

double&
Matrix_iterator_iterator::operator*() const
{
  return *m_data;
}

double&
Matrix_iterator_iterator::operator[](std::ptrdiff_t index) const
{
  return m_data[index];
}

// Checking m_data with other and returning bool
bool
Matrix_iterator_iterator::operator==(Matrix_iterator_iterator other) const
{
  return m_data == other.m_data;
}

bool
Matrix_iterator_iterator::operator!=(Matrix_iterator_iterator other) const
{
  return m_data != other.m_data;
}

bool
Matrix_iterator_iterator::operator<(Matrix_iterator_iterator other) const
{
  return m_data < other.m_data;
}

bool
Matrix_iterator_iterator::operator>(Matrix_iterator_iterator other) const
{
  return m_data > other.m_data;
}

bool
Matrix_iterator_iterator::operator<=(Matrix_iterator_iterator other) const
{
  return m_data <= other.m_data;
}

bool
Matrix_iterator_iterator::operator>=(Matrix_iterator_iterator other) const
{
  return m_data >= other.m_data;
}

Matrix_const_iterator_iterator&
Matrix_const_iterator_iterator::operator++()
{
  ++m_data;
  return *this;
}

Matrix_const_iterator_iterator
Matrix_const_iterator_iterator::operator++(int)
{
  return Matrix_const_iterator_iterator(m_data++);
}

Matrix_const_iterator_iterator
Matrix_const_iterator_iterator::operator+(std::ptrdiff_t diff) const
{
  return Matrix_const_iterator_iterator(m_data + diff);
}

std::ptrdiff_t
Matrix_const_iterator_iterator::operator+(
    Matrix_const_iterator_iterator other) const
{
  return m_data - other.m_data;
}

Matrix_const_iterator_iterator&
Matrix_const_iterator_iterator::operator--()
{
  --m_data;
  return *this;
}

Matrix_const_iterator_iterator
Matrix_const_iterator_iterator::operator--(int)
{
  return Matrix_const_iterator_iterator(m_data--);
}

Matrix_const_iterator_iterator
Matrix_const_iterator_iterator::operator-(std::ptrdiff_t diff) const
{
  return Matrix_const_iterator_iterator(m_data - diff);
}

std::ptrdiff_t
Matrix_const_iterator_iterator::operator-(
    Matrix_const_iterator_iterator other) const
{
  return m_data - other.m_data;
}

const double&
Matrix_const_iterator_iterator::operator*() const
{
  return *m_data;
}

const double&
Matrix_const_iterator_iterator::operator[](std::ptrdiff_t index) const
{
  return m_data[index];
}

// Checking m_data arithematic conditions individually
bool
Matrix_const_iterator_iterator::operator==(
    Matrix_const_iterator_iterator other) const
{
  return m_data == other.m_data;
}

bool
Matrix_const_iterator_iterator::operator!=(
    Matrix_const_iterator_iterator other) const
{
  return m_data != other.m_data;
}

bool
Matrix_const_iterator_iterator::operator<(
    Matrix_const_iterator_iterator other) const
{
  return m_data < other.m_data;
}

bool
Matrix_const_iterator_iterator::operator>(
    Matrix_const_iterator_iterator other) const
{
  return m_data > other.m_data;
}

bool
Matrix_const_iterator_iterator::operator<=(
    Matrix_const_iterator_iterator other) const
{
  return m_data <= other.m_data;
}

bool
Matrix_const_iterator_iterator::operator>=(
    Matrix_const_iterator_iterator other) const
{
  return m_data >= other.m_data;
}

// Iterate through the matrix and return rows of matrix
Matrix_iterator
Matrix::begin()
{
  return Matrix_iterator(cpl_matrix_get_data(m_interface), get_ncol());
}

Matrix_const_iterator
Matrix::begin() const
{
  return Matrix_const_iterator(cpl_matrix_get_data_const(m_interface),
                               get_ncol());
}

Matrix_iterator
Matrix::end()
{
  return Matrix_iterator(
      cpl_matrix_get_data(m_interface) + (get_ncol() * get_nrow()), get_ncol());
}

Matrix_const_iterator
Matrix::end() const
{
  return Matrix_const_iterator(cpl_matrix_get_data_const(m_interface) +
                                   (get_ncol() * get_nrow()),
                               get_ncol());
}

}  // namespace core
}  // namespace cpl
