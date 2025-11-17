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


/*
 * This file is for declarations related to cpl_type, and
 * We aren't actually binding a class/enum for cpl_type, since that's already
 * a C/C++ enum. But there are some conversions that should be implemented,
 * and other 'type' definitions that are used throughout the C++ code.
 */

#ifndef PYCPL_TYPES_HPP
#define PYCPL_TYPES_HPP

#include <complex>
#include <type_traits>

#include <cpl_type.h>

#include "cplcore/error.hpp"

namespace cpl
{
namespace core
{
template <class T>
constexpr cpl_type
type_to_cpl_default()
{
  static_assert(!std::is_same_v<T, T>,
                "You instantiated a type_to_cpl with an invalid C++ type");
  return CPL_TYPE_INVALID;
}

template <class T>
// Unless the type is one of the below types,
// The implementation here will fail a static assert.
constexpr inline cpl_type type_to_cpl = type_to_cpl_default<T>();

template <>
constexpr inline cpl_type type_to_cpl<std::string> = CPL_TYPE_STRING;
template <>
constexpr inline cpl_type type_to_cpl<char> = CPL_TYPE_CHAR;
template <>
constexpr inline cpl_type type_to_cpl<unsigned char> = CPL_TYPE_UCHAR;
template <>
constexpr inline cpl_type type_to_cpl<bool> = CPL_TYPE_BOOL;
template <>
constexpr inline cpl_type type_to_cpl<cpl_boolean> = CPL_TYPE_BOOL;
template <>
constexpr inline cpl_type type_to_cpl<short> = CPL_TYPE_SHORT;
template <>
constexpr inline cpl_type type_to_cpl<unsigned short> = CPL_TYPE_USHORT;
template <>
constexpr inline cpl_type type_to_cpl<int> = CPL_TYPE_INT;
template <>
constexpr inline cpl_type type_to_cpl<unsigned int> = CPL_TYPE_UINT;
template <>
constexpr inline cpl_type type_to_cpl<long int> = CPL_TYPE_LONG;
template <>
constexpr inline cpl_type type_to_cpl<unsigned long int> = CPL_TYPE_ULONG;
template <>
constexpr inline cpl_type type_to_cpl<long long int> = CPL_TYPE_LONG_LONG;
template <>
constexpr inline cpl_type type_to_cpl<float> = CPL_TYPE_FLOAT;
template <>
constexpr inline cpl_type type_to_cpl<double> = CPL_TYPE_DOUBLE;
template <>
constexpr inline cpl_type type_to_cpl<void*> = CPL_TYPE_POINTER;
template <>
constexpr inline cpl_type type_to_cpl<std::complex<float>> =
    CPL_TYPE_FLOAT_COMPLEX;
template <>
constexpr inline cpl_type type_to_cpl<std::complex<double>> =
    CPL_TYPE_DOUBLE_COMPLEX;
template <>
constexpr inline cpl_type type_to_cpl<float _Complex> = CPL_TYPE_FLOAT_COMPLEX;
template <>
constexpr inline cpl_type type_to_cpl<double _Complex> =
    CPL_TYPE_DOUBLE_COMPLEX;

// The following cannot be defined due to it being the same as a long long
// template<> constexpr cpl_type type_to_cpl<cpl_size> = CPL_TYPE_SIZE;

/**
 * @brief Helper for run_func_for_type: A single case, assuming Instance is of
 * type CppType
 */
template <class CppType, template <class> class Instance,
          template <class> class StaticCallable, class Return, class Base,
          class... Args>
Return
run_func_for_single_type(Base* inst, Args... args)
{
  static_assert(
      !std::is_member_pointer_v<decltype(&StaticCallable<CppType>::enabled)>);

  if constexpr (!StaticCallable<CppType>::enabled) {
    throw InvalidTypeError(PYCPL_ERROR_LOCATION,
                           "Unsupported CPL Type for this object/class");
  } else {
    // This would be helpful EXCEPT it doesn't work if StaticCallable::run is
    // overloaded, specifically I encountered the issue when void * overloaded
    // with const std::string &.
    // static_assert(!std::is_member_pointer_v<decltype(&StaticCallable<CppType>::run)>);
    // I also tried this, but it gives invalid static_cast from unresolved
    // overloaded function type.
    // static_assert(!std::is_member_pointer_v<decltype(static_cast<Return
    // (*)(Base *, Args...)>(&StaticCallable<CppType>::run))>);

    return StaticCallable<CppType>::run(
        reinterpret_cast<Instance<CppType>*>(inst), args...);
  }
}

/**
 * @brief Helper for run_func_for_type: A single case, assuming Instance is of
 * type CppType, Specialisation for const instances
 */
template <class CppType, template <class> class Instance,
          template <class> class StaticCallable, class Return, class Base,
          class... Args>
Return
run_func_for_single_type(const Base* inst, Args... args)
{
  static_assert(
      !std::is_member_pointer_v<decltype(&StaticCallable<CppType>::enabled)>);

  if constexpr (!StaticCallable<CppType>::enabled) {
    throw InvalidTypeError(PYCPL_ERROR_LOCATION,
                           "Unsupported CPL Type for this object/class");
  } else {
    // This would be helpful EXCEPT it doesn't work if StaticCallable::run is
    // overloaded, specifically I encountered the issue when void * overloaded
    // with const std::string &.
    // static_assert(!std::is_member_pointer_v<decltype(&StaticCallable<CppType>::run)>);
    // I also tried this, but it gives invalid static_cast from unresolved
    // overloaded function type.
    // static_assert(!std::is_member_pointer_v<decltype(static_cast<Return
    // (*)(Base *, Args...)>(&StaticCallable<CppType>::run))>);

    return StaticCallable<CppType>::run(
        reinterpret_cast<const Instance<CppType>*>(inst), args...);
  }
}

namespace
{
template <class>
struct always_true
{
  static constexpr bool value = true;
};
}  // namespace

/**
 * @brief An inverse of type_to_cpl, except that this is for calling functions
 *        with the type as the argument (instead of returning a 'type')
 *
 * @tparam (Required) Instance Is the main templated class that the given
 * instance downcasts into
 * @tparam (Required) StaticCallable A struct with the static method
 * "run(Instance<Anything>, ...Args)", and static constexpr bool "enabled".
 *         StaticCallable<CppType>::run(...) is called with base, args as
 * arguments. StaticCallable<CppType>::run(...) won't be instantiated unless
 *         StaticCallable<CppType>::enabled is true, allowing for
 * StaticCallable<T> to be a deleted function, enabled = false, then for
 * specialisations to exist with enabled = true. If a cpl_type encountered at
 * runtime, has equivalent CppType where StaticCallable<CppType>::enabled is
 * false, then  InvalidTypeError is thrown.
 * @tparam (Required) StaticCallable<Anything>::run(...)'s return type
 * @tparam Base Superclass of all Instance<Anything>s, to hold the pointer*,
 *         Instance<Anything>s should be downcastable from this Base.
 * @tparam Args arguments (after the Instance<Any>* pointer) passed to
 * StaticCallable<CppType>::run
 *
 * @note If possible, StaticCallable would have been a templated function, but
 * that it isn't possible to specify a templated function as a template
 * argument. It would have been passed in as a runtime argument, as an object
 * with an operator(). This solution is done with the help of
 * https://stackoverflow.com/a/4697356
 * @note Return type parameter is required because the alternative is this:
 *       auto run_func_for_type(...) -> decltype(StaticCallable<WHAT
 * HERE?>::run(args)) Because using decltype(...::run(args)) instantiates the
 * run function for said template argument, if the user hasn't enabled said run
 * function, said function could be deleted. Alternative to a Return type
 * parameter, we could required that the user give at least 1 CPP type that is
 * enabled.
 *
 *
 * Example
 * @code
 * template<Pixel> struct image_maker {
 *     static void run(Image<Pixel> *my_image, file_to_save_to) = delete;
 *     static constexpr bool enabled = is_image_pix<TPixel>;
 * }
 *
 * template<> image_maker<double>::run(Image<double> *my_image, file_to_save_to)
 * { double *pixels = my_image.pixels(); //This might return depending on pixel
 * type
 *     ... do stuff ...
 * }
 *
 * template<> image_maker<char>::run(Image<char> *my_image, file_to_save_to) {
 *     char *pixels = my_image.pixels(); //This might return depending on pixel
 * type
 *     ... do stuff ...
 * }
 *
 * void ImageBase::save_rgba(cpl_type ty, file_to_save_to) {
 *     run_func_for_type<
 *         Image, //Instance
 *         image_maker, //StaticCallable: The thing to actually run the code
 *         void
 *     >(ty, this, file_to_save_to)
 * }
 * @endcode
 *
 * @param ty The type of *inst. only pass inst->get_type(), or something to that
 * effect, as this MUST match the inst's type at runtime.
 * @param inst The class to call the function with. This will be downcasted
 *             to different classes based on ty's value at runtime.
 *             This can be null (passed to StaticCallable::run as null)
 * @param args Rest of the arguments to StaticCallable::run(inst, args)
 * @throws InvalidTypeError if the given ty isn't supported.
 */
template <template <class> class Instance,
          template <class> class StaticCallable, class Return,
          // These are deducable in normal circumstances
          class Base, class... Args>
Return
run_func_for_type(cpl_type ty, Base* inst, Args... args)
{
  switch (ty) {
    case CPL_TYPE_CHAR:
      return run_func_for_single_type<char, Instance, StaticCallable, Return>(
          inst, args...);
    case CPL_TYPE_UCHAR:
      return run_func_for_single_type<unsigned char, Instance, StaticCallable,
                                      Return>(inst, args...);
    case CPL_TYPE_BOOL:
      return run_func_for_single_type<cpl_boolean, Instance, StaticCallable,
                                      Return>(inst, args...);
    case CPL_TYPE_SHORT:
      return run_func_for_single_type<short, Instance, StaticCallable, Return>(
          inst, args...);
    case CPL_TYPE_USHORT:
      return run_func_for_single_type<unsigned short, Instance, StaticCallable,
                                      Return>(inst, args...);
    case CPL_TYPE_INT:
      return run_func_for_single_type<int, Instance, StaticCallable, Return>(
          inst, args...);
    case CPL_TYPE_UINT:
      return run_func_for_single_type<unsigned int, Instance, StaticCallable,
                                      Return>(inst, args...);
    case CPL_TYPE_LONG:
      return run_func_for_single_type<long int, Instance, StaticCallable,
                                      Return>(inst, args...);
    case CPL_TYPE_ULONG:
      return run_func_for_single_type<unsigned long int, Instance,
                                      StaticCallable, Return>(inst, args...);
    case CPL_TYPE_LONG_LONG:
      return run_func_for_single_type<long long int, Instance, StaticCallable,
                                      Return>(inst, args...);
    case CPL_TYPE_SIZE:
      return run_func_for_single_type<cpl_size, Instance, StaticCallable,
                                      Return>(inst, args...);
    case CPL_TYPE_FLOAT:
      return run_func_for_single_type<float, Instance, StaticCallable, Return>(
          inst, args...);
    case CPL_TYPE_DOUBLE:
      return run_func_for_single_type<double, Instance, StaticCallable, Return>(
          inst, args...);
    case CPL_TYPE_POINTER:
      return run_func_for_single_type<void*, Instance, StaticCallable, Return>(
          inst, args...);
    case CPL_TYPE_FLOAT_COMPLEX:
      return run_func_for_single_type<std::complex<float>, Instance,
                                      StaticCallable, Return>(inst, args...);
    case CPL_TYPE_DOUBLE_COMPLEX:
      return run_func_for_single_type<std::complex<double>, Instance,
                                      StaticCallable, Return>(inst, args...);
    default:
      throw InvalidTypeError(PYCPL_ERROR_LOCATION,
                             "Given cpl_type is not known");
  }
}

template <class T>
constexpr bool is_complex_v = false;
template <class T>
constexpr bool is_complex_v<std::complex<T>> = true;

std::complex<float> complexf_to_cpp(float _Complex value);
std::complex<double> complexd_to_cpp(double _Complex value);
float _Complex complex_to_c(std::complex<float> value);
double _Complex complex_to_c(std::complex<double> value);

using size = cpl_size;

}  // namespace core
}  // namespace cpl

#endif  // PYCPL_TYPES_HPP