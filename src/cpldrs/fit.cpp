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
 * Wraps the cpl drs fit functions as Python functions with type converison
 */

#include "cpldrs/fit.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <functional>
#include <iterator>
#include <utility>

#include <cpl_array.h>
#include <cpl_fit.h>
#include <cpl_matrix.h>
#include <cpl_type.h>

#include "cplcore/array.hpp"
#include "cplcore/error.hpp"

namespace cpl
{
namespace drs
{

// Global definitions (as to not have multiple instances compiled in)
/**
 * @internal
 * @brief Thread-local variable acting as a lambdas' captured scope, for the
 *        evaluate lambda passed into fit_lvmq, used from f_trampoline
 *
 * These global variables are required because cpl_fit_lvqm doesn't allow the
 * caller to pass in any user data. So the trampoline (above) does't have access
 * to  anything but globals and its arguments.
 * This means there is no way, except as a global, to pass in the std::function
 * (It cannot be converted to a function pointer, because it captures scope.)
 *
 * These are thread local to ensure any simulatenous executions of cpl_fit_lvmq
 * don't interfere with each other.
 *
 * A similar thing to this is done in error.cpp
 */
thread_local std::function<int(const double[], const double[], double*)>
    f_lambda;
/**
 * @internal
 * @brief Thread-local variable acting as a lambdas' captured scope, for the
 *        evaluate lambda passed into fit_lvmq, used from dfda_trampoline
 * @see f_lambda for explaination
 */
thread_local std::function<int(const double[], const double[], double[])>
    dfda_lambda;

/**
 * @internal
 * @brief Trampoline function that calls thread_local f_lambda from cpl_fit_lvmq
 */
int
f_trampoline(const double x[], const double a[], double* result)
{
  return f_lambda(x, a, result);
}

/**
 * @internal
 * @brief Trampoline function that calls thread_local dfda_lambda from
 * cpl_fit_lvmq
 */
int
dfda_trampoline(const double x[], const double a[], double result[])
{
  return dfda_lambda(x, a, result);
}

/** Previously using a primary definition in fit.hpp, and the above
 *trampoline/lamdba definitions was in fit.hpp. This template specialization
 *only approach replaces a single primary definition to resolve a
 *bad_function_call exception on Mac gcc>=11.
 *
 * It is theorised that previously primary definition included accross many
 *compilation units across pycpl may have caused the stored in f_lambda to
 *become NULL outside the scope of `fit_lvmq` due to the possible conflicts
 *between compilation units. This shouldn't nomally happen due to there being a
 *global declaration in fit.cpp, and it does not in most compilers but there may
 *be restrictions or quirks introduced with Mac gcc11.
 *
 * The consequences of this however is that despite fit_lvmq being a template
 *function, only one specialization type is defined and a new method will need
 *to be written for each new specialization type. This is fine for now as only
 *one template type is currently accessible from the python bindings but it
 *should be noted in future if attempting to bind other function signatures for
 *evaluate and evaluate_derivatives.
 *
 * FIXME: My theory being will need to be confirmed via a debugger (lldb most
 *likely) but this is very low on the priority list since it does not currently
 *impact the functionality or usability of pycpl.
 **/
template <>
std::tuple<cpl::core::Vector, double, double, cpl::core::Matrix>
fit_lvmq(
    const cpl::core::Matrix&
        x_positions,  // const std::optional<cpl::core::Matrix> &sigma_x,
    const cpl::core::Vector& y_positions,
    cpl::core::Vector starting_guess_params,
    const std::optional<std::vector<bool>>& participating_parameters,
    // Currently fit_lvmq only allows this function definition. Its fine to be
    // restrictive for now I suppose
    std::function<double(std::vector<double>, std::vector<double>)> evaluate,
    std::function<std::vector<double>(std::vector<double>, std::vector<double>)>
        evaluate_derivatives,
    const std::optional<cpl::core::Vector>& sigma_y, double rel_tol,
    int tol_count, int max_iterations)
{
  if (participating_parameters.has_value() &&
      starting_guess_params.get_size() != participating_parameters->size()) {
    throw cpl::core::IncompatibleInputError(
        PYCPL_ERROR_LOCATION,
        "participating_parameters (ia) must match size of "
        "starting_guess_params (a)");
  }

  size d = x_positions.get_ncol();
  size m = starting_guess_params.get_size();
  std::exception_ptr last_thrown;

  // The following lambda logic could have gone into the trampoline functions
  // but that would mean storing each of the captured variables (d, m,
  // last_thrown) as thread_local globals

  std::function<int(const double[], const double[], double*)> this_f_lambda =
      [d, m, &last_thrown, &this_f_lambda,
       &evaluate](const double x[], const double a[], double* result) -> int {
    try {
      // Replace with span here
      // The sizes of these are D and M from the param docs for 'f' in cpl
      std::vector<double> x_vec(x, x + d);
      std::vector<double> a_vec(a, a + m);
      // May throw anything:
      double result_returned = evaluate(std::move(x_vec), std::move(a_vec));

      // evaluate_derivatives may have called lvmq, which would reset
      // the thread_local dfda_lambda, so reset it to this f_lambda:
      *result = result_returned;  // Store result in output parameter
      f_lambda = this_f_lambda;   // Reset f_lambda

      return 0;
    }
    catch (...) {
      last_thrown = std::current_exception();
      std::rethrow_exception(last_thrown);
      return 1;
    }
  };
  f_lambda = this_f_lambda;

  std::function<int(const double[], const double[], double[])>
      this_dfda_lambda =
          [d, m, &last_thrown, &this_dfda_lambda, &evaluate_derivatives](
              const double x[], const double a[], double result[]) -> int {
    try {
      // Replace with span here
      // The sizes of these are D and M from the param docs for 'f' in cpl
      std::vector<double> x_vec(x, x + d);
      std::vector<double> a_vec(a, a + m);
      // May throw anything:
      std::vector<double> result_returned =
          evaluate_derivatives(std::move(x_vec), std::move(a_vec));
      if (result_returned.size() != m) {
        throw cpl::core::IncompatibleInputError(
            PYCPL_ERROR_LOCATION,
            "evaluate_derivatives must return vector of size M");
      }

      // evaluate_derivatives may have called lvmq, which would reset
      // the thread_local dfda_lambda, so reset it to this dfda_lambda:
      // Copy out the result
      std::memcpy(result, result_returned.data(),
                  m * sizeof(double));  // Store result in output parameter
      dfda_lambda = this_dfda_lambda;   // Reset dfda_lambda

      return 0;
    }
    catch (...) {
      last_thrown = std::current_exception();
      std::rethrow_exception(last_thrown);
      return 1;
    }
  };
  dfda_lambda = this_dfda_lambda;

  double mean_squared_error;
  double reduced_chi_square;
  cpl_matrix* formal_covariance_matrix;

  bool extra_outputs = sigma_y.has_value();

  std::vector<int> participating_params_ints;
  if (participating_parameters.has_value()) {
    std::transform(participating_parameters->begin(),
                   participating_parameters->end(),
                   std::back_inserter(participating_params_ints),
                   [](bool elem) -> int { return elem; });
  }

  cpl::core::Error::throw_errors_with(
      cpl_fit_lvmq, x_positions.ptr(),
      nullptr,  // Currently sigma_x is unsupported: must always be null
      y_positions.ptr(), sigma_y.has_value() ? sigma_y->ptr() : nullptr,
      starting_guess_params.ptr(),
      participating_parameters.has_value() ? &participating_params_ints[0]
                                           : nullptr,
      &f_trampoline, &dfda_trampoline, rel_tol, tol_count, max_iterations,
      &mean_squared_error, extra_outputs ? &reduced_chi_square : nullptr,
      extra_outputs ? &formal_covariance_matrix : nullptr);

  if (last_thrown) {
    std::rethrow_exception(last_thrown);
  }

  return {
      starting_guess_params.duplicate(), mean_squared_error, reduced_chi_square,
      cpl::core::Matrix(formal_covariance_matrix)  // steals
  };
}

std::shared_ptr<cpl::core::ImageList>
fit_imagelist_polynomial(
    const cpl::core::Vector& x_pos, const cpl::core::ImageList& values,
    size mindeg, size maxdeg, bool is_symsamp, cpl_type pixeltype,
    std::optional<std::shared_ptr<cpl::core::ImageBase>> fiterror,
    std::optional<cpl::core::Window> area)
{
  if (!area.has_value()) {
    return std::make_shared<cpl::core::ImageList>(
        cpl::core::Error::throw_errors_with(
            cpl_fit_imagelist_polynomial, x_pos.ptr(), values.ptr(), mindeg,
            maxdeg, static_cast<cpl_boolean>(is_symsamp), pixeltype,
            fiterror.has_value() ? fiterror.value()->ptr() : nullptr));
  } else {
    return std::make_shared<cpl::core::ImageList>(
        cpl::core::Error::throw_errors_with(
            cpl_fit_imagelist_polynomial_window, x_pos.ptr(), values.ptr(),
            EXPAND_WINDOW(area.value()), mindeg, maxdeg,
            static_cast<cpl_boolean>(is_symsamp), pixeltype,
            fiterror.has_value() ? fiterror.value()->ptr() : nullptr));
  }
}

fit_gaussian_output
fit_image_gaussian(
    const cpl::core::ImageBase& input, size xpos, size ypos, size xsize,
    size ysize, std::optional<std::shared_ptr<cpl::core::ImageBase>> errors,
    // First guesses are all optional for each individual guess
    cpl_array* parameters,  // While it would be nice to pass in the vector
                            // directly, not binding cpl_array prevents usage of
                            // invalid values this function relies on
    // Just convert at the bindings level so the user can pass in py::none for
    // invalid values
    std::optional<std::vector<bool>> frozen_params)
{
  // if the user gives us errors, 4 additional outputs
  // are able to be produced.

  cpl_array* err_params;
  cpl_array* fit_params;
  double rms;
  double redchisq;
  cpl_matrix* covariance;
  cpl_matrix* phys_conv;
  double major;
  double minor;
  double angle;

  bool has_errors = errors.has_value();

  if (frozen_params.has_value()) {
    int arr_size = frozen_params.value().size();
    if (arr_size != 7) {
      cpl_array_delete(
          parameters);  // Since the function flow is halted here, delete
                        // allocated array from the bindings layer
      throw cpl::core::IllegalInputError(
          PYCPL_ERROR_LOCATION,
          "Initial fit_params array is not exactly 7 elements");
    }
    fit_params = cpl_array_new(7, CPL_TYPE_INT);
    for (int i = 0; i < arr_size; i++) {
      cpl_array_set_int(fit_params, i, (int)frozen_params.value()[i]);
    }
  }


  // Pre-allocated, but only if im_err is set.
  err_params = has_errors ? cpl_array_new(7, CPL_TYPE_DOUBLE) : nullptr;

  cpl::core::Error::throw_errors_with(
      cpl_fit_image_gaussian, input.ptr(),
      has_errors ? errors->get()->ptr() : nullptr, xpos, ypos, xsize, ysize,
      parameters, has_errors ? err_params : nullptr,
      frozen_params.has_value() ? fit_params : nullptr, &rms,
      // Pointer to inside the optional
      has_errors ? &redchisq : nullptr, has_errors ? &covariance : nullptr,
      &major, &minor, &angle, has_errors ? &phys_conv : nullptr);
  std::vector<double> res_parameters =
      cpl::core::cpl_array_as_vector<double>(parameters);
  if (frozen_params.has_value()) {
    cpl_array_delete(fit_params);
  }
  if (has_errors) {
    double* err_data = cpl_array_get_data_double(err_params);
    std::vector<double> err_params_vector(err_data, err_data + 7);
    cpl_array_unwrap(err_params);


    // Wrap the matricies in cpl::core::Matrix's if they were outputted
    cpl::core::Matrix formal_covariance_matrix(covariance);
    cpl::core::Matrix formal_phys_conv(phys_conv);
    return std::make_tuple(err_params_vector, rms, redchisq,
                           formal_covariance_matrix, major, minor, angle,
                           formal_phys_conv, res_parameters);
  }

  return std::make_tuple(
      std::optional<std::vector<double>>(), rms, std::optional<double>(),
      std::optional<cpl::core::Matrix>(), major, minor, angle,
      std::optional<cpl::core::Matrix>(), res_parameters);
}

}  // namespace drs
}  // namespace cpl
