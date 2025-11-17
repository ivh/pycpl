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

#ifndef PYCPL_MATRIX_BINDINGS_HPP
#define PYCPL_MATRIX_BINDINGS_HPP

// Keep pybind11.h the first include, see pybind11 documentation for details:
// https://pybind11.readthedocs.io/en/stable/basics.html#header-and-namespace-conventions
#include <pybind11/pybind11.h>

#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <utility>

#include "cplcore/matrix.hpp"

namespace py = pybind11;

/**
 * @brief Binds the Matrix class (from matrix.hpp)
 * @param m The Pybind Python module to which Matrix is bound (usually named
 * 'cpl.core')
 *
 * The following is the list of Python objects that are bound when this function
 * is run:
 *     - cpl.ui.Matrix
 * This function is intended to be called by the 'main' binding function in
 * src/bindings.cpp
 */
void bind_matrix(py::module& m);

// FIXME: The following declaration has no corresponding definition.
//        It is not clear if this is abandoned or unfinished code.
//        Keep it for now but guard it, until the status is clarified.
#if 0
/*
 * @brief Constructs a cpl::core::Matrix from any python iterable
 * with a length
 */
cpl::core::Matrix py_matrix_constructor(py::iterable iterable);
#endif

namespace as_cpl_matrix_types
{
using deleter_ty = std::function<void(cpl::core::Matrix*)>;
using unique_ty = std::unique_ptr<cpl::core::Matrix, deleter_ty>;
using storage_ty = std::optional<unique_ty>;
using return_ty = std::optional<std::reference_wrapper<cpl::core::Matrix>>;
}  // namespace as_cpl_matrix_types

/**
 * Convert any compatible python object/None to a cpl::core::Matrix (.second of
 * return value)
 *
 * If a Python list or other iterable+sized type is passed in, a CPL Matrix is
 * created. Otherwise, the existing Matrix (or None) is returned.
 *
 * The returned unique_ptr owns any created cpl::core::Matrixs, if there was
 * one created. Keep it around for as long as you wish to use the
 * reference_wrapper<cpl::core::Matrix>.
 *
 * TODO: Modifications to the matricies aren't reflected to passed in python
 * lists or other MutableSequences, but that could be done in the custom
 * deleter.
 */
std::pair<as_cpl_matrix_types::storage_ty, as_cpl_matrix_types::return_ty>
as_cpl_matrix(py::object double_list);

/**
 * Due to pybind not working well with the 2-dimensional iteration that matrix
 * has, This is used as py::make_iterator(this) to yield matrix Matrix::iterator
 * as matrix rows visible to python
 *
 * It is literally a very thin wrapper around Matrix::iterator:
 * All iterator movement operations delegate to the wrapped Iterator, and
 * importantly for py::make_iterator, dereferences deref to Matrix::iterator.
 *
 * This is all because python expects an intermediary 'container' that the
 * Matrix iterator yields, which then you can begin() and end() to iterate over
 * the row elements. Using this wrapper, the MatrixIterWrap iterates over
 * 'rows', where rows are Matrix::iterators. These then can be iterated over
 * again, yielding the double elements.
 *
 * Another solution might be to instead create a MatrixRow class, which the
 * Matrix::begin() and Matrix::end() iterator *derefs* into.
 */
class MatrixIterWrap
{
  using Matrix_iterator = cpl::core::Matrix::iterator;
  cpl::core::Matrix::iterator m_self;

 public:
  explicit MatrixIterWrap(cpl::core::Matrix::iterator self) : m_self(self) {}

  MatrixIterWrap& operator++();
  MatrixIterWrap operator++(int);
  MatrixIterWrap operator+(cpl::core::size diff) const;
  cpl::core::size operator+(MatrixIterWrap other) const;
  MatrixIterWrap& operator--();
  MatrixIterWrap operator--(int);
  MatrixIterWrap operator-(cpl::core::size diff) const;
  cpl::core::size operator-(MatrixIterWrap other) const;
  Matrix_iterator operator*() const;
  Matrix_iterator operator[](cpl::core::size index) const;
  bool operator==(MatrixIterWrap other) const;
  bool operator!=(MatrixIterWrap other) const;
  bool operator<(MatrixIterWrap other) const;
  bool operator>(MatrixIterWrap other) const;
  bool operator<=(MatrixIterWrap other) const;
  bool operator>=(MatrixIterWrap other) const;

  // iterator traits
  using difference_type = cpl::core::size;
  using value_type = cpl::core::Matrix::iterator;
  using pointer = void;
  using reference = cpl::core::Matrix::iterator;
  using iterator_category = std::random_access_iterator_tag;
};

#endif  // PYCPL_MATRIX_BINDINGS_HPP