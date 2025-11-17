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

#ifndef PYCPL_VECTOR_BINDINGS_HPP
#define PYCPL_VECTOR_BINDINGS_HPP

// Keep pybind11.h the first include, see pybind11 documentation for details:
// https://pybind11.readthedocs.io/en/stable/basics.html#header-and-namespace-conventions
#include <pybind11/pybind11.h>

#include <functional>
#include <optional>
#include <utility>

#include "cplcore/bivector.hpp"
#include "cplcore/vector.hpp"

namespace py = pybind11;

/**
 * @brief Binds the Vector and Bivector class (from vector.hpp and bivector.hpp)
 * @param m The Pybind Python module to which Vector is bound (usually named
 * 'cpl.core')
 *
 * The following is the list of Python objects that are bound when this function
 * is run:
 *     - cpl.ui.Bivector
 *     - cpl.ui.Vector
 * This function is intended to be called by the 'main' binding function in
 * src/bindings.cpp
 */
void bind_vector(py::module& m);

/*
 * @brief Constructs a cpl::core::Vector from any python iterable
 * with a length
 */
cpl::core::Vector py_vec_constructor(py::iterable iterable);

namespace as_cpl_vec_types
{
using deleter_ty = std::function<void(cpl::core::Vector*)>;
using unique_ty = std::unique_ptr<cpl::core::Vector, deleter_ty>;
using storage_ty = std::optional<unique_ty>;
using return_ty = std::optional<std::reference_wrapper<cpl::core::Vector>>;
}  // namespace as_cpl_vec_types

/**
 * Convert any compatible python object/None to a cpl::core::Vector (.second of
 * return value)
 *
 * If a Python list or other iterable+sized type is passed in, a CPL Vector is
 * created. Otherwise, the existing Vector (or None) is returned.
 *
 * The returned unique_ptr owns any created cpl::core::Vectors, if there was
 * one created. Keep it around for as long as you wish to use the
 * reference_wrapper<cpl::core::Vector>.
 *
 * TODO: Modifications to the vector aren't reflected to passed in python lists
 * or other MutableSequences, but that could be done in the custom deleter.
 */
std::pair<as_cpl_vec_types::storage_ty, as_cpl_vec_types::return_ty>
as_cpl_vec(py::object double_list);

/*
 * @brief Constructs a cpl::core::Biector from any python tuple
 * of 2 python objects convertable to cpl::core::Vector
 */
cpl::core::Bivector py_bivec_constructor(py::iterable tuple);

namespace as_cpl_bivec_types
{
using deleter_ty = std::function<void(cpl::core::Bivector*)>;
using unique_ty = std::unique_ptr<cpl::core::Bivector, deleter_ty>;
using storage_ty = std::optional<unique_ty>;
using return_ty = std::optional<std::reference_wrapper<cpl::core::Bivector>>;
}  // namespace as_cpl_bivec_types

/**
 * Convert any compatible python object/None to a cpl::core::Bivector (.second
 * of return value)
 *
 * If a Python tuple or other iterable  type is passed in, a CPL Bivector is
 * created. Otherwise, the existing Bivector (or None) is returned.
 *
 * If a tuple of cpl Vectors is passed in, those vectors are *copied* into a new
 * created Bivector.
 *
 * The returned unique_ptr owns any created cpl::core::Bivectors, if there was
 * one created. Keep it around for as long as you wish to use the
 * reference_wrapper<cpl::core::Bivector>.
 *
 * TODO: Modifications to the bivector aren't reflected to passed in python
 * tuples, lists or other MutableSequences, but that could be done in the custom
 * deleter.
 */
std::pair<as_cpl_bivec_types::storage_ty, as_cpl_bivec_types::return_ty>
as_cpl_bivec(py::object tuple);

#endif  // PYCPL_VECTOR_BINDINGS_HPP