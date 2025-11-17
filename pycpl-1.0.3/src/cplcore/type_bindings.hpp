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

#ifndef PYCPL_TYPE_BINDINGS_HPP
#define PYCPL_TYPE_BINDINGS_HPP

// Keep pybind11.h the first include, see pybind11 documentation for details:
// https://pybind11.readthedocs.io/en/stable/basics.html#header-and-namespace-conventions
#include <pybind11/pybind11.h>

#include <optional>
#include <string_view>

#include <cpl_type.h>

namespace py = pybind11;

/**
 * @brief Binds the Type enum, from cpl's cpl_type.h
 * @param m The Pybind Python module to which Type is bound (usually named
 * 'cpl.core')
 *
 * The following is the list of Python objects that are bound when this function
 * is run:
 *     - cpl.ui.Type
 * This function is intended to be called by the 'main' binding function in
 * src/bindings.cpp
 */
void bind_types(py::module& m);

namespace cpl
{

/**
 * Given a struct-packing format string, as specified by
 * https://docs.python.org/3/library/struct.html#struct-alignment
 * This function will determine what, if any, cpl type corresponds to said
 * string.
 *
 * Its use is limited to numpy types from numpy arrays, with 1 element per type.
 * (A CPL_TYPE_STRING might also be returned)
 * The format descriptor might look like '@h' for a native short, for example,
 * or '@'. Returned would be CPL_TYPE_SHORT in this case.
 *
 * @note This is also used by pybind11's buffer_info.format and
 *       py::format_descriptor<T>::format()
 *
 * @throws InvalidTypeError: Multiple (not special/alignment specifying or 'x';
 * just ascii) characters in the format descriptor cause this function to throw.
 * @throws InvalidTypeError: An otherwise unrecognised format_descriptor.
 */
std::optional<cpl_type>
pystruct_type_to_cpl(std::string_view format_descriptor);

/**
 * Given a struct-packing format string, as specified by
 * https://docs.python.org/3/library/struct.html#struct-alignment
 * This function will determine if the (usually) buffer_info.ptr
 * can be used as a c pointer to the corresponding c type (return True)
 *
 * This is true only in the case that the format descriptor is native,
 * and has no extra padding bytes ('x's).
 * So any leading '<', '>', '!', '=' characters make this return false.
 * As will having more than 1 ascii character such as 'hh' or 'fi'
 *
 * @example
 *     buffer_info info = py::buffer(something).request();
 *     cpl_type image_type = pystruct_type_to_cpl(info.format)
 *
 *     if(pystruct_type_is_native(info.format)) {
 *         //Simple case that this allows for
 *         make_image_from_c_array(image_type, info.ptr);
 *
 *     } else {
 *         make_blank_image(image_type);
 *         // This code is example code, it doesn't take into account strides
 *         size_t pixels = <product of info.shape>;
 *         for(void *elem = info.ptr; elem < info.ptr+pixels; elem +=
 * info.itemsize) { put_pixel_into_image(elem)
 *         }
 *     }
 */
bool pystruct_type_is_native(std::string_view format_descriptor);

/**
 * @brief Given a Python class, this converts it (if it's a numpy type)
 *        from a numpy type, to a corresponding C type (as a cpl_type)
 */
std::optional<cpl_type> numpy_type_to_cpl(py::object numpy_type);

}  // namespace cpl

#endif  // PYCPL_TYPE_BINDINGS_HPP