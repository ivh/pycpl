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

// Wrapper class for cpl_property

#ifndef PROPERTY_HPP
#define PROPERTY_HPP

#include <complex>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <variant>

#include <cpl_property.h>
#include <cpl_type.h>

#include "cplcore/error.hpp"
#include "cplcore/types.hpp"

// Commented out until cpl_property_dump is made public
// #include <cpl_property_impl.h>

namespace cpl
{
namespace core
{

using size = cpl::core::size;

// Friend class for retype()
class PropertyList;

/**
 * Whilst technically cpl_properties can be created
 * to be arrays of values, the only actual way to use such a property
 * is as a char* string, so vector<type> isn't supported in this
 * Property class, only string and scalar types.
 */
class Property
{
  friend class PropertyList;

  struct value_to_cpl_type_visitor
  {
    template <class T>
    cpl_type operator()(T /* unused */)
    {
      return cpl::core::type_to_cpl<T>;
    }
  };

  /**
   * @brief See try_upcast_visitor::operator()
   */
  template <class U>
  struct try_upcast_visitor
  {
    bool allow_narrowing;

    /**
     * @brief Attempts to cast, at runtime, from the T type
     *        to the U type. If the cast fails, an InvalidTypeError is raised
     *
     * A cast can fail if there is no implicit conversion chain from T -> U
     * It can also fail if there was an implicit conversion chain, BUT
     *     * There is also a reverse chain from U -> T
     *     * Putting the value through the reverse chain yields a different
     * value than the original i.e. It fails if the value has to be narrowed.
     * This is on a per-value runtime basis. A '1' in a long, when T=long,
     * U=char, will not throw, but a 300 in a long will throw.
     */
    template <class T>
    U operator()(T visited_val)
    {
      if constexpr (!std::is_convertible_v<T, U>) {
        std::ostringstream ss;
        ss << "It is not a valid conversion to use a ";
        ss << cpl_type_get_name(type_to_cpl<T>);
        ss << " as a ";
        ss << cpl_type_get_name(type_to_cpl<U>);
        throw InvalidTypeError(PYCPL_ERROR_LOCATION, ss.str());
      } else {
        U casted = static_cast<U>(visited_val);

        if (allow_narrowing) {
          return casted;
        }
        // If a narrowing check is possible, it is tried:
        if constexpr (std::is_convertible_v<U, T>) {
          T narrowed_check = static_cast<T>(casted);
          if (narrowed_check != visited_val) {
            std::ostringstream ss;
            ss << "Using a " << cpl_type_get_name(type_to_cpl<T>);
            ss << " as a stand-in for " << cpl_type_get_name(type_to_cpl<U>);
            ss << " would loose information (Casts into " << casted << ")";
            throw InvalidTypeError(PYCPL_ERROR_LOCATION, ss.str());
          } else {
            return casted;
          }
        } else {
          return casted;
        }
      }
    }
  };

 public:
  /**
   * Whilst not stored at any point in the property class,
   * This value type interfaces between external code (e.g. python) and the
   * cpl_property* interface.
   *
   * Due to a bug with pybind11, the 'bool' type must come before 'int',
   * complex<double> must come before complex<float>, double before float, etc.
   * Without this, pybind11 gives function arguments (e.g. set_value(value))
   * integers when the python code gave booleans.
   */
  using value_type =
      std::variant<std::complex<double>, double, bool, std::string, long,
                   std::complex<float>, long long, int, char, float

                   >;

  /**
   * @brief If possible, upcasts (without narrowing) to the templated param
   * value.
   *
   * If conversion is not possible, or when the conversion would narrow when
   * disallowed An InvalidTypeError is thrown
   */
  template <class U>
  static U try_upcast(value_type val, bool allow_narrowing = false)
  {
    return std::visit(try_upcast_visitor<U>{allow_narrowing}, val);
  }

  /**
   * @brief Given a value type, determines what the corresponding CPL cpl_type
   * is.
   */
  cpl_type value_to_cpl_type(value_type val) noexcept
  {
    return std::visit(value_to_cpl_type_visitor{}, val);
  }

  /**
   * @brief Take ownership of a cpl_propertylist struct ptr
   */
  Property(cpl_property* to_steal) noexcept;

  /**
   * @brief Take ownership of the given Property, leaving it in
   *        a state where the only defined
   *        operations are assignment to, copying, or destruction
   */
  Property(Property&& other) noexcept;

  /**
   * @brief Create an empty property of a given type.
   * @param name Property name.
   * @param type Property type flag.
   *
   * The function allocates memory for a property of type @em type and assigns
   * the identifier string @em name to the newly created property.
   *
   * The returned property must be destroyed using the property destructor
   *
   * @return The newly created property, or @c NULL if it could not be created.
   * In the latter case an appropriate error code is set.
   */
  Property(const std::string& name, cpl_type type);

  /**
   * @brief Create an already filled property of a given type.
   * @param name Property name.
   * @param type Property type flag.
   * @param initial_value Value to start the property with
   *
   * The function allocates memory for a property of type @em type and assigns
   * the identifier string @em name to the newly created property.
   *
   * The returned property must be destroyed using the property destructor
   *
   * @return The newly created property, or @c NULL if it could not be created.
   * In the latter case an appropriate error code is set.
   */
  Property(const std::string& name, cpl_type type, value_type initial_value);

  /**
   * @brief Create an already filled property of a given type.
   * @param name Property name.
   * @param type Property type flag.
   * @param initial_value Value to start the property with
   * @param comment Comment to start with
   *
   * The function allocates memory for a property of type @em type and assigns
   * the identifier string @em name to the newly created property.
   *
   * The returned property must be destroyed using the property destructor
   *
   * @return The newly created property, or @c NULL if it could not be created.
   * In the latter case an appropriate error code is set.
   */
  Property(const std::string& name, cpl_type type, value_type initial_value,
           const std::string& comment);

  /**
   * @brief Create a copy of a property.
   *
   * The function returns a copy of the property @em self. The copy is a
   * deep copy, i.e. all property members are copied.
   *
   * @return The copy of the given property, or @c NULL in case of an error.
   *     In the latter case an appropriate error code is set.
   */
  Property(const Property& other);

  /**
   * @brief Destroy a property.
   *
   * The function destroys a property of any kind. All property members
   * including their values are properly deallocated. If the property @em self
   * is @c NULL, nothing is done and no error is set.
   *
   */
  ~Property();

  /**
   * @brief Get the current number of elements a property contains.
   *
   * The function returns the current number of elements the property
   *
   * Testing whether the value of a string property equals a given string
   * can be done like this:
   *
   * When the length of the string to compare to is known at compile time,
   * the call to memcmp() will typically be inlined (and no calls to strlen()
   * et al are needed). Further, if the length of the property string value
   * is greater than 1, then it is not necessary to test whether its type is
   * CPL_TYPE_STRING, since all properties of a numerical type has size 1.
   *
   * @return The current number of elements or -1 in case of an error. If an
   *     error occurred an appropriate error code is set.
   */
  size get_size() const;

  /**
   * @brief Dump a cpl_property to a string
   *
   * @return String with the property contents.
   */

  std::string dump() const;

  /**
   * @brief Modify the name of a property.
   * @param name New name.
   *
   * The function replaces the current name of @em self with a copy
   * of the string @em name. The function returns an error if @em name is
   *
   */
  void set_name(const std::string& name);

  /**
   * @brief Modify a property's comment.
   * @param comment New comment.
   *
   * The function replaces the current comment field of @em self with a
   * copy of the string @em comment. The new comment may be @c NULL. In this
   * case the function effectively deletes the current comment.
   *
   */
  void set_comment(const std::string& comment);

  /**
   * @brief Modify a property's type
   * @param type New type.
   *
   * The function changes the type field of @em self with a
   * the given cpl_type @em type. The value of this property
   * becomes the same as if it were a newly created property
   * of said type, with name and comment set.
   *
   * set_type(type) where type == get_type() is a no-op.
   * Value is NOT changed in this case.
   *
   * Any array not of type CPL_TYPE_STRING is not supported,
   * as there is no size argument to this function
   */
  void set_type(cpl_type type);

  /**
   * @brief Set the value of a property. Casting is done if nessecary,
   *        but if, for example, you're storing int 1923 into a CPL_TYPE_SHORT
   *        property, no casting is done.
   * @param value New value.
   *
   * The type of this property may change.
   *
   */
  void set_value(const Property::value_type& value);

  /**
   * @brief Set the value of a property. Casting is done if nessecary,
   *        but if, for example, you're storing int 1923 into a CPL_TYPE_SHORT
   *        property, an exception is thrown.
   * @param value New value.
   *
   * The type of this property does not change.
   *
   * @throws InvalidTypeError if the type of value isn't upcastable to
   * forced_type
   */
  void set_typed_value(const Property::value_type& value);

  /**
   * @brief Get the property name.
   *
   * The function returns a handle for the read-only identifier string of @em
   * self.
   *
   * @return The name of the property. All properties have a name
   */
  std::string get_name() const;

  /**
   * @brief Get the property comment.
   *
   * The function returns a handle for the read-only comment string of @em self.
   *
   * @return The comment of the property if it is present.
   */
  std::optional<std::string> get_comment() const;

  /**
   * @brief Get the type of a property.
   *
   * This function returns the type of the data value stored in the
   * property @em self.
   *
   * @return The type code of this property. In case of an error the returned
   *     type code is @c CPL_TYPE_INVALID and an appropriate error code is
   *     set.
   */
  cpl_type get_type() const;

  /**
   * @brief Get the value of a property.
   *
   * The function retrieves the value currently stored in the
   * property @em self.
   *
   * @return The current property value.
   */
  Property::value_type get_value() const;

  const cpl_property* ptr() const;
  cpl_property* ptr();

  /**
   * @brief Relieves self Property of ownership of the underlying
   *        cpl_property* pointer. This is a counterpart to
   * Property(cpl_property *to_steal); Make sure to use cpl_property_delete to
   * delete the returned cpl_property* when you're done with it
   */
  static cpl_property* unwrap(Property&& self);

  bool operator==(const Property& other) const;
  bool operator!=(const Property& other) const;

  Property& operator=(const Property& other);
  Property& operator=(Property&& other);

 private:
  cpl_property* m_interface;
};

}  // namespace core
}  // namespace cpl

#endif  // PROPERTY_HPP