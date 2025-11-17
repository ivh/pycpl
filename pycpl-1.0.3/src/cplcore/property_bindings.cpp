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

#include "cplcore/property_bindings.hpp"

#include <algorithm>
#include <complex>
#include <filesystem>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <cpl_type.h>
#include <pybind11/complex.h>
#include <pybind11/stl.h>

#include "cplcore/property.hpp"
#include "cplcore/propertylist.hpp"
#include "cpldfs/dfs.hpp"
#include "cplui/frame.hpp"
#include "dump_handler.hpp"
#include "path_conversion.hpp"  // IWYU pragma: keep, avoid clangd false positive

namespace py = pybind11;

using size = cpl::core::size;

void
propertylist_set_slice(
    cpl::core::PropertyList& self, py::slice slice,
    std::vector<std::reference_wrapper<const cpl::core::Property>> items)
{
  size_t start, stop, step, slicelength;
  if (!slice.compute(self.get_size(), &start, &stop, &step, &slicelength))
    throw py::error_already_set();

  if (step == 1) {
    // Special case allowing LHS and RHS to have different sizes only for step
    // size == 1
    for (size_t i = stop - 1; i >= start; --i) {
      self.erase(i);
    }
    for (auto item : items) {
      self.insert(start, item);
    }
    return;
  }

  if (slicelength != items.size())
    throw std::runtime_error(
        "Left and right hand size of slice assignment have different sizes!");

  for (size_t i = 0; i < slicelength; ++i) {
    self.get(start) = items[i].get();
    start += step;
  }
}

void
bind_propertylist(py::module& m)
{
  py::class_<cpl::core::Property, std::shared_ptr<cpl::core::Property>>
      property(m, "Property");

  property.doc() = R"pydoc(
    Properties are basically a variable container which consists of a name, a type identifier and a specific value of that type. 
    
    The type identifier always determines the type of the associated value. A property is similar to an ordinary variable and its 
    current value can be set or retrieved through its name. In addition a property may have a descriptive comment associated.
    
    The following types are supported:
        - cpl.core.Type.BOOL
        - cpl.core.Type.FLOAT
        - cpl.core.Type.INT
        - cpl.core.Type.CHAR
        - cpl.core.Type.STRING
        - cpl.core.Type.DOUBLE
        - cpl.core.Type.LONG
        - cpl.core.Type.LONG_LONG
        - cpl.core.Type.FLOAT_COMPLEX
        - cpl.core.Type.DOUBLE_COMPLEX
    
    Support for arrays in general is currently not available.
    )pydoc";

  property
      .def(py::init<const std::string&, cpl_type>(), py::arg("name"),
           py::arg("type"), "Initialise a property without setting value")
      .def(py::init(
               [](const std::string& name, cpl_type ty,
                  cpl::core::Property::value_type initial_value,
                  std::optional<std::string> comment) -> cpl::core::Property {
                 if (comment.has_value()) {
                   return cpl::core::Property(name, ty, initial_value,
                                              *comment);
                 } else {
                   return cpl::core::Property(name, ty, initial_value);
                 }
               }),
           py::arg("name"), py::arg("type"), py::arg("initial_value"),
           py::arg("comment") = py::none(),
           R"pydoc(
            Create a new property, manually provide type. Comment is optional. The following types are supported:
            - cpl.core.Type.BOOL
            - cpl.core.Type.FLOAT
            - cpl.core.Type.INT
            - cpl.core.Type.CHAR
            - cpl.core.Type.STRING
            - cpl.core.Type.DOUBLE
            - cpl.core.Type.LONG
            - cpl.core.Type.LONG_LONG
            - cpl.core.Type.FLOAT_COMPLEX
            - cpl.core.Type.DOUBLE_COMPLEX
        )pydoc")
      .def(
          py::init([](const std::string& name,
                      cpl::core::Property::value_type initial_value,
                      std::optional<std::string> comment) {
            // Check variant
            std::optional<cpl_type> inferred = std::visit(
                [](auto&& arg) -> std::optional<cpl_type> {
                  using T = std::decay_t<decltype(arg)>;
                  std::optional<cpl_type> type_id;

                  if constexpr (std::is_same_v<T, bool>) {
                    type_id = CPL_TYPE_BOOL;
                  } else if constexpr (std::is_same_v<T, int>) {
                    type_id = CPL_TYPE_INT;
                  } else if constexpr (std::is_same_v<T, float>) {
                    type_id = CPL_TYPE_FLOAT;
                  } else if constexpr (std::is_same_v<T, char>) {
                    type_id = CPL_TYPE_CHAR;
                  } else if constexpr (std::is_same_v<T, std::string>) {
                    type_id = CPL_TYPE_STRING;
                  } else if constexpr (std::is_same_v<T, double>) {
                    type_id = CPL_TYPE_DOUBLE;
                  } else if constexpr (std::is_same_v<T, long>) {
                    type_id = CPL_TYPE_LONG;
                  } else if constexpr (std::is_same_v<T, long long>) {
                    type_id = CPL_TYPE_LONG_LONG;
                  } else if constexpr (std::is_same_v<T, std::complex<float>>) {
                    type_id = CPL_TYPE_FLOAT_COMPLEX;
                  } else if constexpr (std::is_same_v<T,
                                                      std::complex<double>>) {
                    type_id = CPL_TYPE_DOUBLE_COMPLEX;
                  }
                  return type_id;
                },
                initial_value);

            if (!inferred.has_value()) {
              throw cpl::core::InvalidTypeError(
                  PYCPL_ERROR_LOCATION,
                  "Given initial value is not of a compatible type");
            }
            if (comment.has_value()) {
              return cpl::core::Property(name, inferred.value(), initial_value,
                                         *comment);
            } else {
              return cpl::core::Property(name, inferred.value(), initial_value);
            }
          }),
          py::arg("name"), py::arg("initial_value"),
          py::arg("comment") = py::none(),
          R"pydoc(
            Create a new property with automatic type inference. Comment is optional. The following types are supported:

            If the value provided fails to be inferred to one of the supported types, an InvalidTypeError is thrown. 
        )pydoc")
      .def_property_readonly(
          "__len__", &cpl::core::Property::get_size)  // Seems to be only for
                                                      // strings anyway?
      .def_property_readonly("type", &cpl::core::Property::get_type,
                             "CPL type of property. See ")
      .def_property("name", &cpl::core::Property::get_name,
                    &cpl::core::Property::set_name, "name of property")
      .def_property("comment", &cpl::core::Property::get_comment,
                    &cpl::core::Property::set_comment, "property description")
      .def("__eq__",
           [](cpl::core::Property& self, py::object eq_arg) -> bool {
             /*
                 If eq_arg were to be a cpl::core::Property (avoiding
                complication here), then running Parameter == NotAParameter
                would raise a type error in Python, So instead, it mustb e cast
                manually, here, to catch said type error.
             */
             try {
               cpl::core::Property* casted =
                   eq_arg.cast<cpl::core::Property*>();
               return self == *casted;
             }
             catch (const py::cast_error& wrong_type) {
               return false;  // Type mismatch should return False in python
             }
           })
      .def_property("value", &cpl::core::Property::get_value,
                    [](cpl::core::Property& self, py::object value) -> void {
                      if (value.is_none()) {
                        throw py::value_error("None is not allowed as a value");
                      } else {
                        self.set_value(
                            value.cast<cpl::core::Property::value_type>());
                      }
                    })
      .def("__str__", &cpl::core::Property::dump)
      .def("__repr__",
           [](py::object self) -> std::string {
             // Repr the tuple, which gives brackets for the  call as well as
             // commas
             return std::string("Property") +
                    py::repr(self.attr("__getstate__")()).cast<std::string>();
           })
      .def(
          "dump",
          [](cpl::core::Property& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(), self.dump(),
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("show") = true,
          R"pydoc(
        Dump a property contents to a file, stdout or a string.

        Comment lines start with the hash character.

        Parameters
        ----------
        filename : str, optional
            File to dump property contents to
        mode : str, optional
            Mode to open the file with. Defaults to "w" (write, overwriting the contents of the file if it already exists),
            but can also be set to "a" (append, creating the file if it does not already exist or appending to the end of
            it if it does).
        show : bool, optional
            Send property contents to stdout. Defaults to True.

        Returns
        -------
        str 
            Multiline string containing the dump of the property contents.
        )pydoc")
      .def(py::pickle(
          [](cpl::core::Property& self) -> py::tuple {
            return py::make_tuple(self.get_name(), self.get_type(),
                                  self.get_value(), self.get_comment());
          },
          [](py::tuple t) -> cpl::core::Property {
            cpl::core::Property self(
                t[0].cast<std::string>(), t[1].cast<cpl_type>(),
                t[2].cast<cpl::core::Property::value_type>());

            if (auto comment = t[3].cast<std::optional<std::string>>();
                comment.has_value()) {
              self.set_comment(*comment);
            }
            return self;
          }));

  py::class_<cpl::core::PropertyList, std::shared_ptr<cpl::core::PropertyList>>
      propertylist(m, "PropertyList");
  propertylist.doc() = R"pydoc(
    The opaque property list data type. 

    Was designed for supporting the FITS header information. Indeed, it is possible, using a
    single function, to load a header file into a property list, given the filename and the 
    number of the extension using the `load()` function. 
    )pydoc";
  propertylist.def(py::init(), "Initialize an empty property list")
      .def(py::init([](py::iterable from) -> cpl::core::PropertyList {
             cpl::core::PropertyList self;
             std::transform(from.begin(), from.end(), std::back_inserter(self),
                            [](py::handle elem) -> cpl::core::Property {
                              return elem.cast<cpl::core::Property>();
                            });
             return self;
           }),
           py::arg("from"),
           "Build a PropertyList from an iterable of properties. If iterable "
           "contains any non-properties a cast error will be thrown")
      .def("__str__", &cpl::core::PropertyList::dump)
      .def("__repr__",
           [](py::object self) -> std::string {
             // Repr the tuple, which gives brackets for the  call as well as
             // commas
             return std::string("PropertyList(") +
                    py::repr(self.attr("__getstate__")()).cast<std::string>() +
                    ")";
           })
      .def(
          "dump",
          [](cpl::core::PropertyList& self,
             std::optional<std::filesystem::path> filename,
             std::optional<std::string> mode,
             std::optional<bool> show) -> std::string {
            return dump_handler(filename.value(), mode.value(), self.dump(),
                                show.value());
          },
          py::arg("filename") = "", py::arg("mode") = "w",
          py::arg("show") = true,
          R"pydoc(
        Dump a property list contents to a file, stdout or a string.

        Each element is preceded by its index number (starting with 1!) and
        written on a single line.

        Comment lines start with the hash character.

        Parameters
        ----------
        filename : str, optional
            File to dump property list contents to
        mode : str, optional
            Mode to open the file with. Defaults to "w" (write, overwriting the contents of the file if it already exists),
            but can also be set to "a" (append, creating the file if it does not already exist or appending to the end of
            it if it does).
        show : bool, optional
            Send property list contents to stdout. Defaults to True.

        Returns
        -------
        str 
            Multiline string containing the dump of the property list contents.
        )pydoc")
      .def(py::pickle(
          [](cpl::core::PropertyList& self)
              -> std::vector<cpl::core::Property> {
            std::vector<cpl::core::Property> output;
            std::copy(self.begin(), self.end(), std::back_inserter(output));
            return output;
          },
          [](std::vector<cpl::core::Property> props)
              -> cpl::core::PropertyList {
            return cpl::core::PropertyList(props.begin(), props.end());
          }))
      .def("__len__", &cpl::core::PropertyList::get_size)
      .def("__contains__", &cpl::core::PropertyList::has)
      .def("__contains__",
           [](const cpl::core::PropertyList& self,
              const cpl::core::Property& find) -> bool {
             for (size_t i = 0; i < self.get_size(); ++i) {
               if (self.get(i) == find) {
                 return true;
               }
             }
             return false;
           })
      .def(
          "__getitem__",
          [](const cpl::core::PropertyList& self,
             long position) -> cpl::core::Property {
            if (position >= self.get_size() || position < 0) {
              throw py::index_error("PropertyList index out of range");
            }
            return self.get(position);
          },
          py::arg("index"),
          "Retrieve property from list using it's index position")
      .def(
          "__getitem__",
          [](const cpl::core::PropertyList& self, std::string_view name) {
            auto optional_value = self.get(name);
            if (!optional_value.has_value()) {
              throw py::key_error(std::string(name));
            }
            return *optional_value;
          },
          py::arg("prop_name"), "Retrieve property from list using it's name")
      .def("__getitem__",
           [](const cpl::core::PropertyList& self,
              py::slice slice) -> cpl::core::PropertyList {
             size_t start, stop, step, slicelength;
             if (!slice.compute(self.get_size(), &start, &stop, &step,
                                &slicelength))
               throw py::error_already_set();

             cpl::core::PropertyList seq;

             for (size_t i = 0; i < slicelength; ++i) {
               seq.append(self.get(start));
               start += step;
             }
             return seq;
           })
      .def(
          "__setitem__",
          [](cpl::core::PropertyList& self, long position,
             const cpl::core::Property& item) -> void {
            if (position < 0) {
              position += self.get_size();
            }
            if (position < 0 || position >= self.get_size()) {
              throw py::index_error("PropertyList index out of range");
            }
            // Should call the Property::operator= of the property in the deque.
            self.get(position) = item;
          },
          py::arg("index"), py::arg("property"),
          "Insert a property at the given index position")
      .def("__setitem__",
           [](cpl::core::PropertyList& self, std::string_view name,
              const cpl::core::Property& item) -> void {
             auto optional_to_set = self.get(name);
             if (!optional_to_set.has_value()) {
               throw py::key_error(std::string(name));
             }
             // Should call the Property::operator= of the property in the
             // deque.
             optional_to_set->get() = item;
           })
      .def("__setitem__", &propertylist_set_slice, py::arg("slice"),
           py::arg("properties"),
           "Insert an iterable of Properties into `this`, within the slice "
           "range. Slice must be the same size as the iterable")
      .def(
          "insert",
          [](cpl::core::PropertyList& self, long position,
             const cpl::core::Property& item) -> void {
            if (position > self.get_size() ||
                position < 0) {  // Equality to size is allowed
              throw py::index_error("PropertyList index out of range");
            } else if (position == self.get_size()) {
              self.append(item);
            } else {
              self.insert(position, item);
            }
          },
          py::arg("index"), py::arg("property"),
          "Insert a property at index. PropertyList will increase in size by "
          "1.")
      .def("insert",
           [](cpl::core::PropertyList& self, std::string_view name,
              const cpl::core::Property& item) -> void {
             if (!self.insert(name, item)) {
               throw py::key_error(std::string(name));
             }
           })
      .def(
          "__delitem__",
          [](cpl::core::PropertyList& self, long position) -> void {
            if (position >= self.get_size() || position < 0) {
              throw py::index_error("PropertyList index out of range");
            } else {
              self.erase(position);
            }
          },
          py::arg("index"),
          "Remove a Property from the PropertyList from position `index`")
      .def(
          "__delitem__",
          [](cpl::core::PropertyList& self, std::string_view name) -> void {
            if (!self.erase(name)) {
              throw py::key_error(std::string(name));
            }
          },
          py::arg("name"),
          "Remove a Property from the PropertyList with a specific name")
      // FIXME: Using a parameter sections for the following overloadend
      // functions causes Sphinx to issue a critical warning regarding an
      // unexpected section title. As a consequence manual formatting is used
      // here as a workaround. Once Sphinx is able to deal with this correctly
      // the 'automatic' formatting should be put back.
      .def("append",
           py::overload_cast<const cpl::core::Property&>(
               &cpl::core::PropertyList::append),
           py::arg("property"), R"pydoc(
          Append a property value to a property list.

          This function creates a new property and appends it to the end of a property list. It will not check if the property already exists.

          :Parameters:
            **property** (*cpl.core.Property*) -- Property to append
          )pydoc")
      .def("append",
           py::overload_cast<const cpl::core::PropertyList&>(
               &cpl::core::PropertyList::append),
           py::arg("other"), R"pydoc(
          Append a propertylist

          This function appends the properties from the property list `other` to `self`.

          :Parameters:
            **other** (*cpl.core.PropertyList*) -- Propertylist to append
          )pydoc")
      .def("append",
           py::overload_cast<std::string, cpl::core::Property::value_type>(
               &cpl::core::PropertyList::append),
           py::arg("name"), py::arg("value"), R"pydoc(
          Append a new property using a name and value

          This function appends a new property with `name` and inital value `value`. The type will be infered by `value`' s type

          :Parameters:
            - **name** (*str*) -- Name for the new property
            - **value** (*str, char, float, complex, bool, int*) -- Initial value of the new property
          )pydoc")
      .def("__eq__",
           [](const cpl::core::PropertyList& self, py::object eq_arg) -> bool {
             /*
                 If eq_arg were to be a cpl::core::PropertyList (avoiding
                complication here), then running Parameter == NotAParameter
                would raise a type error in Python, So instead, it mustb e cast
                manually, here, to catch said type error.
             */
             try {
               cpl::core::PropertyList* casted =
                   eq_arg.cast<cpl::core::PropertyList*>();
               return self.get_size() == casted->get_size() &&
                      std::equal(self.begin(), self.end(), casted->begin());
             }
             catch (const py::cast_error& wrong_type) {
               return false;  // Type mismatch should return False in python
             }
           })
      .def("del_regexp", &cpl::core::PropertyList::erase_regexp,
           py::arg("regexp"), py::arg("invert"),
           R"pydoc(
            Erase all properties with name matching a given regular expression.

            The function searches for all the properties matching in the list.

            The function expects POSIX 1003.2 compliant extended regular expressions.

            Parameters
            ----------
            regexp : str
                Regular expression.
            invert : bool
                Flag inverting the sense of matching.

            Returns
            -------
            int
                The number of erased entries
        )pydoc")
      // .def("copy_property", &cpl::core::PropertyList::copy_property)
      // .def("copy_property_regexp",
      // &cpl::core::PropertyList::copy_property_regexp)
      .def(
          "sort",
          [](cpl::core::PropertyList& self, py::function sort_func) -> void {
            auto sort_trampoline =
                [&sort_func](const cpl::core::Property& first,
                             const cpl::core::Property& second) -> int {
              return sort_func(first, second).cast<int>();
            };

            return self.sort(sort_trampoline);
          },
          py::arg("compare"),
          R"pydoc(
            Sort a property list using a passed function.

            Sort is done in place

            Parameters
            ----------
            compare : function(cpl.core.Property, cpl.core.Property) -> int
                The function used to compare two properties.  This function compares to determine whether a property is less, 
                equal or greater than another one.

            Returns
            -------
            None
        )pydoc")
      .def("save", &cpl::core::PropertyList::save, py::arg("name"),
           py::arg("mode"),
           R"pydoc(
            Save a property list to a FITS file. 
            
            Parameters
            ----------
            name : str 
                Name of the input file.
            position : int 
                Index of the data set to read.
            mode : unsigned
                The desired output options (combined with bitwise or of cpl.core.io enums)

            Notes
            -----
            This function saves a property list to a FITS file, using cfitsio. The data unit is empty.

            Supported output modes are cpl.core.io.CREATE (create a new file) and cpl.core.io.EXTEND 
            (append to an existing file) 
        )pydoc")

      .def_static("load_regexp", &cpl::core::load_propertylist_regexp,
                  py::arg("name"), py::arg("position"), py::arg("regexp"),
                  py::arg("invert"),
                  R"pydoc(
            Create a filtered property list from a file.
            
            Parameters
            ----------
            name : str 
                Name of the input file.
            position : int 
                Index of the data set to read.
            regexp : str
                Regular expression used to filter properties.
            invert : bool
                Flag inverting the sense of matching property names.

            Returns
            -------
            cpl.core.PropertyList
                The loaded propertylist from the input file at index `position`, with properties matching the `regexp` filter

            Notes
            -----
            The function reads all properties of the data set with index `position`
            with matching names from the file `name`. If the flag `invert` is False,
            all properties whose names match the regular expression `regexp` are
            read. If `invert` is set to True, all properties with
            names not matching `regexp` are read rather. The function expects
            POSIX 1003.2 compliant extended regular expressions.
            
            Currently only the FITS file format is supported. The property list is
            created by reading the FITS keywords from extension `position`.
            
            The numbering of the data sections starts from 0.
            
            When creating the property list from a FITS header, any keyword without
            a value such as undefined keywords, are not transformed into
            a property. In the case of float or double (complex) keywords, there is no
            way to identify the type returned by CFITSIO, therefore this function will
            always load them as double (complex).
            
            FITS format specific keyword prefixes (e.g. ``HIERARCH``) must
            not be part of the given pattern string `regexp`, but only the actual
            FITS keyword name may be given.
        )pydoc")
      .def_static("load", &cpl::core::load_propertylist, py::arg("name"),
                  py::arg("position"),
                  R"pydoc(
            Create a filtered property list from a file.
            
            Parameters
            ----------
            name : str 
                Name of the input file.
            position : int 
                Index of the data set to read.

            Returns
            -------
            cpl.core.PropertyList
                The loaded propertylist from the input file at index `position`

            Notes
            -----
            The function reads the properties of the data set with index position from the file name.

            Currently only the FITS file format is supported. The property list is created by reading 
            the FITS keywords from extension position. The numbering of the data sections starts from 
            0. When creating the property list from a FITS header, any keyword without a value such 
            as undefined keywords, are not transformed into a property. In the case of float or double 
            (complex) keywords, there is no way to identify the type returned by CFITSIO, therefore 
            this function will always load them as double (complex).
        )pydoc")
      .def(
          "setup_product_header",
          [](std::shared_ptr<cpl::core::PropertyList> self,
             std::shared_ptr<cpl::ui::Frame> product_frame,
             std::shared_ptr<cpl::ui::FrameSet> framelist,
             std::shared_ptr<cpl::ui::ParameterList> parlist,
             const std::string& recid, const std::string& pipeline_id,
             const std::string& dictionary_id,
             py::object inherit_frame) -> void {
            if (inherit_frame.is_none()) {
              cpl::dfs::setup_product_header(self, product_frame, framelist,
                                             parlist, recid, pipeline_id,
                                             dictionary_id, NULL);
            } else {
              cpl::dfs::setup_product_header(
                  self, product_frame, framelist, parlist, recid, pipeline_id,
                  dictionary_id,
                  inherit_frame.cast<std::shared_ptr<cpl::ui::Frame>>());
            }
          },
          py::arg("product_frame"), py::arg("framelist"), py::arg("parlist"),
          py::arg("recid"), py::arg("pipeline_id"), py::arg("dictionary_id"),
          py::arg("inherit_frame") = py::none(),
          R"mydelim(
    Add product keywords to a pipeline product property list.

    Parameters
    ----------
    product_frame : cpl.ui.Frame
      Frame describing the product
    framelist : cpl.ui.FrameSet
      List of frames including all input frames
    parlist : cpl.ui.ParameterList
      Recipe parameter list
    recid : str
      Recipe name
    pipeline_id : str
      Pipeline package (unique) identifier
    dictionary_id : str
      PRO dictionary identifier
    inherit_frame : cpl.ui.Frame, optional
      Frame from which header information is inherited

    Returns
    -------
    None

    Raises
    ------
    cpl.core.DataNotFoundError
      If the input framelist contains no input frames or
      a frame in the input framelist does not specify a file.
      In the former case the string "Empty set-of-frames" is appended
      to the error message.
    cpl.core.IllegalInputError
      If the product frame is not tagged or not grouped
      as cpl.ui.Frame.FrameGroup.PRODUCT. A specified `inherit_frame`
      doesn't belong to the input frame list, or it is not in FITS format.
    cpl.core.FileNotFoundError
      If a frame in the input framelist specifies a non-existing file.
    cpl.core.BadFileFormatError
      If a frame in the input framelist specifies an invalid file.

    Notes
    -----
    This function updates and validates that the property list `self` is DICB
    compliant. In particular, this function does the following:

    1. Selects a reference frame from which the primary and secondary
       keyword information is inherited. The primary information is
       contained in the FITS keywords ``ORIGIN``, ``TELESCOPE``, ``INSTRUME``,
       ``OBJECT``, ``RA``, ``DEC``, ``EPOCH``, ``EQUINOX``, ``RADESYS``,
       ``DATE-OBS``, ``MJD-OBS``, ``UTC``, ``LST``, ``PI-COI``, ``OBSERVER``,
       while the secondary information is contained in all the other keywords.
       If the `inherit_frame` is None, both primary and secondary information
       is inherited from the first frame in the input framelist with
       group cpl.ui.Frame.FrameGroup.RAW, or if no such frames are present
       the first frame with group cpl.ui.Frame.FrameGroup.CALIB.
       If `inherit_frame` is not None, the secondary information
       is inherited from `inherit_frame` instead.

    2. Copy to `self`, if they are present, the following primary
       FITS keywords from the first input frame in the `framelist`:
       ``ORIGIN``, ``TELESCOPE``, ``INSTRUME``, ``OBJECT``, ``RA``,
       ``DEC``, ``EPOCH``, ``EQUINOX``, ``RADESYS``, ``DATE-OBS``,
       ``MJD-OBS``, ``UTC``, ``LST``, ``PI-COI``, ``OBSERVER``. If those
       keywords are already present in the `self` property list, they
       are overwritten only in case they have the same type. If any of
       these keywords are present with an unexpected type, a warning is
       issued, but the keywords are copied anyway (provided that the
       above conditions are fulfilled), and no error is set.

    3. Copy all the ``HIERARCH ESO *`` keywords from the primary FITS header
       of the `inherit_frame` in `framelist`, with the exception of
       the ``HIERARCH ESO DPR *``, and of the ``HIERARCH ESO PRO *`` and
       ``HIERARCH ESO DRS *`` keywords if the `inherit_frame` is a calibration.
       If those keywords are already present in `self`, they are overwritten.

    4. If found, remove the ``HIERARCH ESO DPR *`` keywords from `self`.

    5. If found, remove the ``ARCFILE`` and ``ORIGFILE`` keywords from `self`.

    6. Add to `self` the following mandatory keywords from the PRO
       dictionary: ``PIPEFILE``, ``ESO PRO DID``, ``ESO PRO REC1 ID``,
       ``ESO PRO REC1 DRS ID``, ``ESO PRO REC1 PIPE ID``, and
       ``ESO PRO CATG``. If those keywords are already present in
       `self`, they are overwritten. The keyword ``ESO PRO CATG`` is
       always set identical to the tag in `product_frame`.

    7. Only if missing, add to `self` the following mandatory keywords
       from the PRO dictionary: ``ESO PRO TYPE``, ``ESO PRO TECH``, and
       ``ESO PRO SCIENCE``. The keyword ``ESO PRO TYPE`` will be set to
       ``REDUCED``. If the keyword ``ESO DPR TECH`` is found in the header
       of the first frame, ``ESO PRO TECH`` is given its value, alternatively
       if the keyword ``ESO PRO TECH`` is found it is copied instead, and
       if all fails the value ``UNDEFINED`` is set. Finally, if the keyword
       ``ESO DPR CATG`` is found in the header of the first frame and is set
       to ``SCIENCE``, the boolean keyword ``ESO PRO SCIENCE`` will be set to
       `true`, otherwise it will be copied from an existing ``ESO PRO SCIENCE``
       keyword, while it will be set to `false` in all other cases.

    8. Check the existence of the keyword ``ESO PRO DATANCOM`` in `self`. If
       this keyword is missing, one is added, with the value of the total
       number of raw input frames.

    9. Add to `self` the keywords ``ESO PRO REC1 RAW1 NAME``,
       ``ESO PRO REC1 RAW1 CATG``, ``ESO PRO REC1 CAL1 NAME``, ``ESO PRO REC1 CAL1 CATG``,
       to describe the content of the input set-of-frames.

    See the DICB PRO dictionary for details on the mentioned PRO keywords.

    Non-FITS files are handled as files with an empty FITS header.
    
    The pipeline identifier string `pipe_id` is composed of the pipeline package
    name and its version number in the form PACKAGE "/" PACKAGE_VERSION.
    )mydelim");
}
