// This file is part of the PyHDRL Python language bindings
// Copyright (C) 2023-2026 European Southern Observatory
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

#include "hdrlcore/spectrum_bindings.hpp"

#include <cfloat>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "cpl_array.h"
#include "cpl_type.h"
#include "hdrl_spectrum_resample.h"
#include "pybind11/attr.h"
#include "pybind11/buffer_info.h"
#include "pybind11/detail/common.h"
#include "pybind11/numpy.h"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"

#include "hdrlcore/error.hpp"
#include "hdrlcore/pycpl_image.hpp"  // IWYU pragma: keep
#include "hdrlcore/spectrum.hpp"
#include "hdrlfunc/collapse.hpp"
#include "path_conversion.hpp"  // IWYU pragma: keep

namespace py = pybind11;

using hdrl::core::Spectrum1D;

namespace
{
class Spectrum1DResampleMethod
{
 public:
  explicit Spectrum1DResampleMethod(
      std::shared_ptr<hdrl::core::Parameter> parameter)
      : parameter_(std::move(parameter))
  {
    if (!parameter_ || parameter_->ptr() == nullptr) {
      throw py::value_error("Spectrum1DResampleMethod cannot be null");
    }
  }

  hdrl_parameter* ptr() const
  {
    return const_cast<hdrl::core::Parameter&>(*parameter_).ptr();
  }

  static std::shared_ptr<Spectrum1DResampleMethod>
  Interpolate(hdrl::core::InterpolationMethod method)
  {
    hdrl_spectrum1D_interpolation_method c_method =
        hdrl_spectrum1D_interp_akima;
    if (method == hdrl::core::InterpolationMethod::LINEAR) {
      c_method = hdrl_spectrum1D_interp_linear;
    } else if (method == hdrl::core::InterpolationMethod::CSPLINE) {
      c_method = hdrl_spectrum1D_interp_cspline;
    } else if (method == hdrl::core::InterpolationMethod::AKIMA) {
      c_method = hdrl_spectrum1D_interp_akima;
    }
    auto parameter = std::make_shared<hdrl::core::Parameter>(
        hdrl::core::Error::throw_errors_with(
            hdrl_spectrum1D_resample_interpolate_parameter_create, c_method));
    return std::make_shared<Spectrum1DResampleMethod>(parameter);
  }

  static std::shared_ptr<Spectrum1DResampleMethod> Integrate()
  {
    auto parameter = std::make_shared<hdrl::core::Parameter>(
        hdrl::core::Error::throw_errors_with(
            hdrl_spectrum1D_resample_integrate_parameter_create));
    return std::make_shared<Spectrum1DResampleMethod>(parameter);
  }

  static std::shared_ptr<Spectrum1DResampleMethod> Fit(int k, int n_coeff)
  {
    auto parameter = std::make_shared<hdrl::core::Parameter>(
        hdrl::core::Error::throw_errors_with(
            hdrl_spectrum1D_resample_fit_parameter_create, k, n_coeff));
    return std::make_shared<Spectrum1DResampleMethod>(parameter);
  }

  static std::shared_ptr<Spectrum1DResampleMethod>
  FitWindowed(int k, int n_coeff, long window, double factor)
  {
    auto parameter = std::make_shared<hdrl::core::Parameter>(
        hdrl::core::Error::throw_errors_with(
            hdrl_spectrum1D_resample_fit_windowed_parameter_create, k, n_coeff,
            window, factor));
    return std::make_shared<Spectrum1DResampleMethod>(parameter);
  }

 private:
  std::shared_ptr<hdrl::core::Parameter> parameter_;
};

// Helper function to convert numpy array to vector
std::vector<double>
numpy_to_vector(py::array_t<double>& array)
{
  py::buffer_info buf = array.request();
  std::vector<double> vec(buf.size);
  std::memcpy(vec.data(), buf.ptr, buf.size * sizeof(double));
  return vec;
}

// Helper functions to validate input arrays
void
validate_buffer(py::buffer_info& buffer)
{
  if (buffer.ndim != 1) {
    throw py::value_error("Array number of dimensions must be one");
  }
  if (buffer.shape[0] == 0) {
    throw py::value_error("Array must not be empty");
  }

  double* buffer_data = static_cast<double*>(buffer.ptr);
  for (ssize_t i = 0; i < buffer.size; ++i) {
    if (std::isnan(buffer_data[i]) || std::isinf(buffer_data[i])) {
      throw py::value_error(
          "Array contains invalid values (NaN or Inf) at index " +
          std::to_string(i));
    }
  }
}

bool
is_strictly_monotonic_increasing(cpl_array* array)
{
  double* array_data = cpl_array_get_data_double(array);
  for (cpl_size i = 1; i < cpl_array_get_size(array); ++i) {
    if (array_data[i] <= array_data[i - 1]) {
      return false;
    }
  }
  return true;
}

using array_view = std::unique_ptr<cpl_array, decltype(cpl_array_unwrap)*>;

array_view
make_array_view(py::array_t<double>& array)
{
  py::buffer_info buffer = array.request();
  validate_buffer(buffer);
  double* buffer_data = static_cast<double*>(buffer.ptr);
  return array_view(cpl_array_wrap_double(buffer_data, buffer.size),
                    cpl_array_unwrap);
}

using image_view = std::unique_ptr<cpl_image, decltype(cpl_image_unwrap)*>;

image_view
make_image_view(py::array_t<double>& array)
{
  py::buffer_info buffer = array.request();
  validate_buffer(buffer);
  double* buffer_data = static_cast<double*>(buffer.ptr);
  return image_view(cpl_image_wrap_double(buffer.size, 1, buffer_data),
                    cpl_image_unwrap);
}
}  // namespace

void
bind_spectrum1d(py::module_& m)
{
  // Bind enums
  py::enum_<hdrl::core::WaveScale>(m, "WaveScale")
      .value("LINEAR", hdrl::core::WaveScale::LINEAR)
      .value("LOG", hdrl::core::WaveScale::LOG)
      .export_values();

  py::enum_<hdrl::core::InterpolationMethod>(m, "InterpolationMethod")
      .value("LINEAR", hdrl::core::InterpolationMethod::LINEAR)
      .value("CSPLINE", hdrl::core::InterpolationMethod::CSPLINE)
      .value("AKIMA", hdrl::core::InterpolationMethod::AKIMA)
      .export_values();

  // Bind XCorrelationResult with improved representation
  py::class_<hdrl::core::XCorrelationResult>(m, "XCorrelationResult")
      .def(py::init<double, double, double>(), py::arg("shift"),
           py::arg("error"), py::arg("quality"))
      .def_property_readonly("shift",
                             &hdrl::core::XCorrelationResult::get_shift,
                             "float: Index where the cross correlation reaches "
                             "its maximum, with sub-pixel precision")
      .def_property_readonly(
          "error", &hdrl::core::XCorrelationResult::get_error,
          "float: Estimated standard deviation of the correlation")
      .def_property_readonly("quality",
                             &hdrl::core::XCorrelationResult::get_quality,
                             "float: Mean squared error of the best fit")
      .def("__repr__",
           [](const hdrl::core::XCorrelationResult& self) -> std::string {
             return "XCorrelationResult(shift=" +
                    std::to_string(self.get_shift()) +
                    ", error=" + std::to_string(self.get_error()) +
                    ", quality=" + std::to_string(self.get_quality()) + ")";
           });

  py::class_<Spectrum1DResampleMethod,
             std::shared_ptr<Spectrum1DResampleMethod>>(
      m, "Spectrum1DResampleMethod")
      .def_static(
          "Interpolate",
          [](hdrl::core::InterpolationMethod method)
              -> std::shared_ptr<Spectrum1DResampleMethod> {
            hdrl_spectrum1D_interpolation_method c_method =
                hdrl_spectrum1D_interp_akima;
            if (method == hdrl::core::InterpolationMethod::LINEAR) {
              c_method = hdrl_spectrum1D_interp_linear;
            } else if (method == hdrl::core::InterpolationMethod::CSPLINE) {
              c_method = hdrl_spectrum1D_interp_cspline;
            } else if (method == hdrl::core::InterpolationMethod::AKIMA) {
              c_method = hdrl_spectrum1D_interp_akima;
            }
            auto parameter = std::make_shared<hdrl::core::Parameter>(
                hdrl::core::Error::throw_errors_with(
                    hdrl_spectrum1D_resample_interpolate_parameter_create,
                    c_method));
            return std::make_shared<Spectrum1DResampleMethod>(parameter);
          },
          py::arg("method"),
          R"docstring(
          Constructor for the hdrl_parameter in the case of interpolation.

          Parameters
          ----------
          method : hdrl.core.InterpolationMethod
              The interpolation methods.
          )docstring")
      .def_static(
          "Integrate",
          []() -> std::shared_ptr<Spectrum1DResampleMethod> {
            auto parameter = std::make_shared<hdrl::core::Parameter>(
                hdrl::core::Error::throw_errors_with(
                    hdrl_spectrum1D_resample_integrate_parameter_create));
            return std::make_shared<Spectrum1DResampleMethod>(parameter);
          },
          R"docstring(
          Constructor for the hdrl_parameter in the case of integration.
          )docstring")
      .def_static(
          "Fit",
          [](int k, int n_coeff) -> std::shared_ptr<Spectrum1DResampleMethod> {
            auto parameter = std::make_shared<hdrl::core::Parameter>(
                hdrl::core::Error::throw_errors_with(
                    hdrl_spectrum1D_resample_fit_parameter_create, k, n_coeff));
            return std::make_shared<Spectrum1DResampleMethod>(parameter);
          },
          py::arg("k"), py::arg("n_coeff"),
          R"docstring(
          Constructor for the hdrl_parameter in the case of interpolation.

          Parameters
          ----------
          k : int
              The order of the B-spline.
          n_coeff : int
              The number of coefficients used for the fit.
          )docstring")
      .def_static(
          "FitWindowed",
          [](int k, int n_coeff, long window,
             double factor) -> std::shared_ptr<Spectrum1DResampleMethod> {
            auto parameter = std::make_shared<hdrl::core::Parameter>(
                hdrl::core::Error::throw_errors_with(
                    hdrl_spectrum1D_resample_fit_windowed_parameter_create, k,
                    n_coeff, window, factor));
            return std::make_shared<Spectrum1DResampleMethod>(parameter);
          },
          py::arg("k"), py::arg("n_coeff"), py::arg("window"),
          py::arg("factor"),
          R"docstring(
          Constructor for the hdrl_parameter in the case of interpolation.

          Parameters
          ----------
          k : int
              The order of the B-spline.
          n_coeff : int
              The number of coefficients used for the fit.
          window : int
              The number of destination wavelengths whose flux values are
              computed using the same model.
          factor : double
              The given window2 = window * factor. window2 is the number of
              source wavelengths used to compute the fit model.
          )docstring")
      .def("__repr__", [](const Spectrum1DResampleMethod& self) {
        return "Spectrum1DResampleMethod(ptr=" +
               std::to_string(reinterpret_cast<uintptr_t>(self.ptr())) + ")";
      });

  // Bind Spectrum1D with improved constructors and methods
  py::class_<hdrl::core::Spectrum1D, std::shared_ptr<hdrl::core::Spectrum1D>>
      spectrum1d_class(m, "Spectrum1D", py::buffer_protocol());

  spectrum1d_class.doc() = R"docstring(
      A hdrl.core.Spectrum1D is an HDRL spectrum1D containing the wavelengths, the fluxes at
      each wavelength together with their errors, and a wavelength scale.
  )docstring";

  spectrum1d_class
      .def(py::init([](py::array_t<double> flux, py::int_ half_window,
                       py::array_t<double> wavelengths, std::string scale) {
             image_view flux_view = make_image_view(flux);
             array_view wavelength_view = make_array_view(wavelengths);
             if (!is_strictly_monotonic_increasing(wavelength_view.get())) {
               throw py::value_error(
                   "Wavelength data must be sorted in ascending order");
             }
             // Convert scale string to enum
             hdrl::core::WaveScale wavelength_scale;
             if (scale == "linear") {
               wavelength_scale = hdrl::core::WaveScale::LINEAR;
             } else if (scale == "log") {
               wavelength_scale = hdrl::core::WaveScale::LOG;
             } else {
               throw py::value_error(
                   "Invalid wavelength scale. Must be 'linear' or 'log'.");
             }
             return std::make_shared<Spectrum1D>(
                 Spectrum1D(flux_view.get(), half_window, wavelength_view.get(),
                            wavelength_scale));
           }),
           py::arg("flux"), py::arg("half_window").noconvert(),
           py::arg("wavelengths"), py::arg("scale") = "linear", py::prepend(),
           R"docstring(
           Constructor for the hdrl.core.Spectrum1D class when no error information is available, in this case we use DER_SNR to esimate the error.

           Parameters
           ----------
           flux : array of float
               The flux.
           half_window : int
               The half window the DER_SNR is calculated on.
           wavelengths : array of float
               The frequencies.
           scale : string
               The scale of the spectrum (logarithmic `log` or linear `linear`).

           Returns
           -------
           hdrl.core.Spectrum1D
               A newly alocated spectrum.

           See Also
           --------
           The documentation of the function estimate_noise_DER_SNR().
           )docstring")
      .def(py::init([](py::function func, py::array_t<double> wavelengths,
                       std::string scale) {
             array_view wavelength_view = make_array_view(wavelengths);
             if (!is_strictly_monotonic_increasing(wavelength_view.get())) {
               throw py::value_error(
                   "Wavelength data must be sorted in ascending order");
             }
             // Convert scale string to enum
             hdrl::core::WaveScale wavelength_scale;
             if (scale == "linear") {
               wavelength_scale = hdrl::core::WaveScale::LINEAR;
             } else if (scale == "log") {
               wavelength_scale = hdrl::core::WaveScale::LOG;
             } else {
               throw py::value_error(
                   "Invalid wavelength scale. Must be 'linear' or 'log'.");
             }

             auto wrapped = [func](double lambda) -> hdrl_value {
               py::object out = func(lambda);
               py::tuple result = out.cast<py::tuple>();
               if (result.size() != 2) {
                 throw py::value_error(
                     "Analytic function must return a "
                     "tuple(value, error)");
               }
               return {result[0].cast<double>(), result[1].cast<double>()};
             };

             return std::make_shared<Spectrum1D>(
                 Spectrum1D(wrapped, wavelength_view.get(), wavelength_scale));
           }),
           py::arg("func").noconvert(), py::arg("wavelengths"),
           py::arg("scale") = "linear", py::prepend(),
           R"docstring(
           Constructor for the hdrl.core.Spectrum1D class in the case of a spectrum defined by an analytical function.

           Parameters
           ----------
           func : function
               The analytical function defining the spectrum.
           wavelengths : array of float
               The frequencies.
           scale : string
               The scale of the spectrum (logarithmic `log` or linear `linear`).

           Returns
           -------
           hdrl.core.Spectrum1D
               A newly alocated spectrum.
           )docstring")
      .def(py::init([](py::array_t<double> flux, py::object flux_error,
                       py::array_t<double> wavelengths, std::string scale) {
             image_view flux_view = make_image_view(flux);
             array_view wavelength_view = make_array_view(wavelengths);
             if (!is_strictly_monotonic_increasing(wavelength_view.get())) {
               throw py::value_error(
                   "Wavelength data must be sorted in ascending order");
             }
             // Convert scale string to enum
             hdrl::core::WaveScale wavelength_scale;
             if (scale == "linear") {
               wavelength_scale = hdrl::core::WaveScale::LINEAR;
             } else if (scale == "log") {
               wavelength_scale = hdrl::core::WaveScale::LOG;
             } else {
               throw py::value_error(
                   "Invalid wavelength scale. Must be 'linear' or 'log'.");
             }
             if (!flux_error.is_none()) {
               py::array_t<double> errors;
               try {
                 errors = flux_error.cast<py::array_t<double>>();
               }
               catch (const py::cast_error& /*unused */) {
                 throw py::type_error(
                     std::string("expected numpy compatible array, not ") +
                     py::type::of(flux_error)
                         .attr("__name__")
                         .cast<std::string>());
               }
               image_view flux_error_view = make_image_view(errors);
               return std::make_shared<Spectrum1D>(
                   Spectrum1D(flux_view.get(), flux_error_view.get(),
                              wavelength_view.get(), wavelength_scale));
             } else {
               return std::make_shared<Spectrum1D>(
                   Spectrum1D(flux_view.get(), nullptr, wavelength_view.get(),
                              wavelength_scale));
             }
           }),
           py::arg("flux"), py::arg("flux_error"), py::arg("wavelengths"),
           py::arg("scale") = "linear",
           R"docstring(
           Constructor for the hdrl.core.Spectrum1D class when error information is available.

           Parameters
           ----------
           flux : array of float
               The flux.
           flux_error : object
               The error for the flux.
           wavelengths : array of float
               The frequencies.
           scale : string
               The scale of the spectrum (logarithmic `log` or linear `linear`).

           Returns
           -------
           hdrl.core.Spectrum1D
               A newly alocated spectrum.
           )docstring")
      .def_property_readonly(
          "size", &hdrl::core::Spectrum1D::get_size,
          "int: Number of samples the 1D spectrum is made of")
      .def_property_readonly("scale", &hdrl::core::Spectrum1D::get_scale,
                             "string: Scale")
      // Data access methods with numpy array return
      .def_property_readonly(
          "wavelengths",
          [](const hdrl::core::Spectrum1D& self) {
            std::vector<double> data = self.get_wavelength_vector();
            py::array_t<double> array(data.size());
            auto buf = array.request();
            double* ptr = static_cast<double*>(buf.ptr);
            std::memcpy(ptr, data.data(), data.size() * sizeof(double));
            return array;
          },
          "array of float: Wavelengths the spectrum is defined on")
      .def_property_readonly(
          "flux",
          [](const hdrl::core::Spectrum1D& self) {
            std::vector<double> data = self.get_flux_vector();
            py::array_t<double> array(data.size());
            auto buf = array.request();
            double* ptr = static_cast<double*>(buf.ptr);
            std::memcpy(ptr, data.data(), data.size() * sizeof(double));
            return array;
          },
          "array of float: Flux")
      .def_property_readonly(
          "flux_error",
          [](const hdrl::core::Spectrum1D& self) {
            std::vector<double> data = self.get_flux_error_vector();
            py::array_t<double> array(data.size());
            auto buf = array.request();
            double* ptr = static_cast<double*>(buf.ptr);
            std::memcpy(ptr, data.data(), data.size() * sizeof(double));
            return array;
          },
          "array of float: Error of flux")
      .def_property_readonly(
          "bad_pixel_map",
          [](const hdrl::core::Spectrum1D& self) {
            std::vector<int> data = self.get_bad_pixel_map();
            py::array_t<int> array(data.size());
            auto buf = array.request();
            int* ptr = static_cast<int*>(buf.ptr);
            std::memcpy(ptr, data.data(), data.size() * sizeof(int));
            return array;
          },
          "array of float: Bad pixel map")
      .def("__deepcopy__",
           [](hdrl::core::Spectrum1D& self, py::dict /* unused */)
               -> std::shared_ptr<hdrl::core::Spectrum1D> {
             return self.duplicate();
           })
      // Arithmetic operations
      .def(
          "mul_scalar",
          [](hdrl::core::Spectrum1D& self, double scalar) {
            self.mul_scalar(scalar);
          },
          py::arg("scalar"),
          R"docstring(
          Computes the elementwise multiplication of a spectrum by a scalar.
          Spectrum is modified.

          Parameters
          ----------
          scalar : float
              The scalar factor.
          )docstring")
      .def(
          "div_scalar",
          [](hdrl::core::Spectrum1D& self, double scalar) {
            if (std::fabs(scalar) < DBL_EPSILON) {
              throw py::value_error("Division by zero");
            }
            self.div_scalar(scalar);
          },
          py::arg("scalar"),
          R"docstring(
          Computes the elementwise division of a spectrum by a scalar.
          Spectrum is modified.

          Parameters
          ----------
          scalar : float
              The scalar factor.
          )docstring")
      .def(
          "add_scalar",
          [](hdrl::core::Spectrum1D& self, double scalar) {
            self.add_scalar(scalar);
          },
          py::arg("scalar"),
          R"docstring(
          Computes the elementwise addition of a spectrum by a scalar.
          Spectrum is modified.

          Parameters
          ----------
          scalar : float
              The scalar factor.
          )docstring")
      .def(
          "sub_scalar",
          [](hdrl::core::Spectrum1D& self, double scalar) {
            self.sub_scalar(scalar);
          },
          py::arg("scalar"),
          R"docstring(
          Computes the elementwise subtraction of a spectrum by a scalar.
          Spectrum is modified.

          Parameters
          ----------
          scalar : float
              The scalar factor.
          )docstring")
      .def(
          "pow_scalar",
          [](hdrl::core::Spectrum1D& self, double scalar) {
            self.pow_scalar(scalar);
          },
          py::arg("scalar"),
          R"docstring(
          Computes the elementwise power of of the flux to the scalar.
          Spectrum is modified.

          Parameters
          ----------
          scalar : float
              The scalar factor.
          )docstring")
      .def(
          "exp_scalar",
          [](hdrl::core::Spectrum1D& self, double scalar) {
            self.exp_scalar(scalar);
          },
          py::arg("scalar"),
          R"docstring(
          Computes the elementwise power of the scalar to the flux.
          Spectrum is modified.

          Parameters
          ----------
          scalar : float
              The scalar factor.
          )docstring")
  // FIXME: Leave out the arithmetic operations with scalars that create a
  //        new Spectrum1D instance. This kind of operations is not (yet)
  //        implemented in other classes like Image and ImageList. To
  //        keep the overall API design consistent they are not made
  //        available for now.
#if 0
      .def(
          "mul_scalar_create",
          [](const hdrl::core::Spectrum1D& self, double scalar) {
            return self.mul_scalar_create(scalar);
          },
          py::arg("scalar"))
      .def(
          "div_scalar_create",
          [](const hdrl::core::Spectrum1D& self, double scalar) {
            if (std::fabs(scalar) < DBL_EPSILON) {
              throw py::value_error("Division by zero");
            }
            return self.div_scalar_create(scalar);
          },
          py::arg("scalar"))
      .def(
          "add_scalar_create",
          [](const hdrl::core::Spectrum1D& self, double scalar) {
            return self.add_scalar_create(scalar);
          },
          py::arg("scalar"))
      .def(
          "sub_scalar_create",
          [](const hdrl::core::Spectrum1D& self, double scalar) {
            return self.sub_scalar_create(scalar);
          },
          py::arg("scalar"))
      .def(
          "pow_scalar_create",
          [](const hdrl::core::Spectrum1D& self, double scalar) {
            return self.pow_scalar_create(scalar);
          },
          py::arg("scalar"))
      .def(
          "exp_scalar_create",
          [](const hdrl::core::Spectrum1D& self, double scalar) {
            return self.exp_scalar_create(scalar);
          },
          py::arg("scalar"))
#endif
      // Spectrum-Spectrum operations
      .def(
          "div_spectrum_create",
          [](const hdrl::core::Spectrum1D& self,
             const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
            if (!other) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `other`, not "
                  "None");
            }
            return self.div_spectrum_create(*other);
          },
          py::arg("other"),
          R"docstring(
          Divide one spectrum by another spectrum.

          Parameters
          ----------
          other : hdrl.core.Spectrum1D
              The denumerator.

          Returns
          -------
          hdrl.core.Spectrum1D
              The modified copy of spectrum.
          )docstring")
      .def(
          "mul_spectrum_create",
          [](const hdrl::core::Spectrum1D& self,
             const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
            if (!other) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `other`, not "
                  "None");
            }
            return self.mul_spectrum_create(*other);
          },
          py::arg("other"),
          R"docstring(
          Multiply one spectrum by another spectrum.

          Parameters
          ----------
          other : hdrl.core.Spectrum1D
              The other second factor.

          Returns
          -------
          hdrl.core.Spectrum1D
              The modified copy of spectrum.
          )docstring")
      .def(
          "add_spectrum_create",
          [](const hdrl::core::Spectrum1D& self,
             const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
            if (!other) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `other`, not "
                  "None");
            }
            return self.add_spectrum_create(*other);
          },
          py::arg("other"),
          R"docstring(
          Sum two spectra.

          Parameters
          ----------
          other : hdrl.core.Spectrum1D
              The other second factor.

          Returns
          -------
          hdrl.core.Spectrum1D
              The modified copy of spectrum.
          )docstring")
      .def(
          "sub_spectrum_create",
          [](const hdrl::core::Spectrum1D& self,
             const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
            if (!other) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `other`, not "
                  "None");
            }
            return self.sub_spectrum_create(*other);
          },
          py::arg("other"),
          R"docstring(
          Subtract two spectra.

          Parameters
          ----------
          other : hdrl.core.Spectrum1D
              The other second factor.

          Returns
          -------
          hdrl.core.Spectrum1D
              The modified copy of spectrum.
          )docstring")
      .def(
          "div_spectrum",
          [](hdrl::core::Spectrum1D& self,
             const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
            if (!other) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `other`, not "
                  "None");
            }
            self.div_spectrum(*other);
          },
          py::arg("other"),
          R"docstring(
          Divide one spectrum by another spectrum.

          Parameters
          ----------
          other : hdrl.core.Spectrum1D
              The denumerator.
          )docstring")
      .def(
          "mul_spectrum",
          [](hdrl::core::Spectrum1D& self,
             const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
            if (!other) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `other`, not "
                  "None");
            }
            self.mul_spectrum(*other);
          },
          py::arg("other"),
          R"docstring(
          Multiply one spectrum by another spectrum.

          Parameters
          ----------
          other : hdrl.core.Spectrum1D
              The other second factor.
          )docstring")
      .def(
          "add_spectrum",
          [](hdrl::core::Spectrum1D& self,
             const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
            if (!other) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `other`, not "
                  "None");
            }
            self.add_spectrum(*other);
          },
          py::arg("other"),
          R"docstring(
          Sum two spectra.

          Parameters
          ----------
          other : hdrl.core.Spectrum1D
              The other second factor.
          )docstring")
      .def(
          "sub_spectrum",
          [](hdrl::core::Spectrum1D& self,
             const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
            if (!other) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `other`, not "
                  "None");
            }
            self.sub_spectrum(*other);
          },
          py::arg("other"),
          R"docstring(
          Subtract two spectra.

          Parameters
          ----------
          other : hdrl.core.Spectrum1D
              The other second factor.
          )docstring")
      // Spectrum Shift Operations
      .def(
          "compute_shift_xcorrelation",
          [](const hdrl::core::Spectrum1D& self,
             const std::shared_ptr<hdrl::core::Spectrum1D>& other,
             size_t half_win, bool normalize) {
            if (!other) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `other`, not "
                  "None");
            }
            return self.compute_shift_xcorrelation(*other, half_win, normalize);
          },
          py::arg("other"), py::arg("half_win"), py::arg("normalize") = true,
          R"docstring(
          Calculate cross-correlation.

          Parameters
          ----------
          other : hdrl.core.Spectrum1D
              The other spectrum.
          half_win : int
              The half search window where the correlation is calculated.
          normalize : boolean
              Flag, `true` if normalize correlation in mean and rms.

          Returns
          -------
          XCorrelationResult
              Object with cross-correlation results.
          )docstring")
      .def("compute_shift_fit", &hdrl::core::Spectrum1D::compute_shift_fit,
           py::arg("wguess"), py::arg("wrange"), py::arg("fitrange"),
           py::arg("halfsize"),
           R"docstring(
           Compute the spectral shift of the spectrum with respect to
           an expected position of a spectral line.

           Parameters
           ----------
           wguess : float
               Expected wavelength of the spectral line.
           wrange : tuple(float, float)
               Tuple of the minimum and maximum wavelength defining the
               wavelength range of the spectrum to use.
           fitrange : tuple(float, float)
               Tuple of the minimum and maximum wavelength defining the
               wavelength range that is ignored when fitting the ratio of
               the spectrum and the fitted model with a polynomial.
           halfsize : float
               Window half size defining the wavelength limits for
               the polynomial fit.

           Returns
           -------
           float
               The relative shift of the spectrum with respect to the reference
               wavelength.
           )docstring")
      .def(
          "wavelength_shift",
          [](hdrl::core::Spectrum1D& self, double shift) {
            self.wavelength_shift(shift);
          },
          py::arg("shift"),
          R"docstring(
          Computes the elementwise shift of the wavelength by the shift
          parameter.

          Parameters
          ----------
          shift : float
              The shift scalar factor.
          )docstring")
      .def(
          "wavelength_shift_create",
          [](const hdrl::core::Spectrum1D& self, double shift) {
            return self.wavelength_shift_create(shift);
          },
          py::arg("shift"),
          R"docstring(
          Computes the elementwise shift of the wavelength by the shift
          parameter.

          Parameters
          ----------
          shift : float
              The shift scalar factor.

          Returns
          -------
          hdrl.core.Spectrum1D
              The modified copy of spectrum.
          )docstring")
      .def(
          "wavelength_mult_scalar_linear",
          [](hdrl::core::Spectrum1D& self, double scale) {
            if (scale <= 0.0) {
              throw py::value_error("Scale must be positive");
            }
            self.wavelength_mult_scalar_linear(scale);
          },
          py::arg("scale"),
          R"docstring(
          Computes the elementwise multiplication of the scalar for the
          wavelength.

          Parameters
          ----------
          scale : float
              The scalar factor.
          )docstring")
      .def(
          "wavelength_mult_scalar_linear_create",
          [](const hdrl::core::Spectrum1D& self, double scale) {
            if (scale <= 0.0) {
              throw py::value_error("Scale must be positive");
            }
            return self.wavelength_mult_scalar_linear_create(scale);
          },
          py::arg("scale"),
          R"docstring(
          Computes the elementwise multiplication of the scalar for the
          wavelength.

          Parameters
          ----------
          scale : float
              The scalar factor.

          Returns
          -------
          hdrl.core.Spectrum1D
              The modified copy of spectrum.
          )docstring")
      .def("wavelength_convert_to_linear",
           &hdrl::core::Spectrum1D::wavelength_convert_to_linear,
           R"docstring(Converts the wavelength scale to linear.)docstring")
      .def("wavelength_convert_to_linear_create",
           &hdrl::core::Spectrum1D::wavelength_convert_to_linear_create,
           R"docstring(
           Converts the wavelength scale to linear.

           Returns
           -------
           hdrl.core.Spectrum1D
               The modified copy of spectrum.
           )docstring")
      .def("wavelength_convert_to_log",
           &hdrl::core::Spectrum1D::wavelength_convert_to_log,
           R"docstring(Converts the wavelength scale to log.)docstring")
      .def("wavelength_convert_to_log_create",
           &hdrl::core::Spectrum1D::wavelength_convert_to_log_create,
           R"docstring(
           Converts the wavelength scale to log.

           Returns
           -------
           hdrl.core.Spectrum1D
               The modified copy of spectrum.
           )docstring")
      // Spectrum Selection
      .def(
          "select_window",
          [](const hdrl::core::Spectrum1D& self, double lambda_min,
             double lambda_max, bool is_internal) {
            if (lambda_min >= lambda_max) {
              throw py::value_error("lambda_min must be less than lambda_max");
            }
            return self.select_window(lambda_min, lambda_max, is_internal);
          },
          py::arg("lambda_min"), py::arg("lambda_max"),
          py::arg("is_internal") = false,
          R"docstring(
          Selects or discards flux values according to whether the value of the
          corresponding wavelength belongs to the interval [lambda_min,
          lambda_max].

          Parameters
          ----------
          lambda_min : double
              The lower limit of the interval required for selection.
          lambda_max : double
              The upper limit of the interval required for selection.
          is_internal : boolean
              Specify if selection is internal to the interval
              or external to the interval.

          Returns
          -------
          hdrl.core.Spectrum1D
              The selected subset of spectrum.
          )docstring")
      // Spectrum Resample Operations with enhanced safety
      .def(
          "resample",
          [](const hdrl::core::Spectrum1D& self,
             const std::shared_ptr<hdrl::core::Spectrum1D>& other,
             hdrl::core::InterpolationMethod method) {
            if (!other) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `other`, not "
                  "None");
            }
            return self.resample(*other, method);
          },
          py::arg("other"),
          py::arg("method") = hdrl::core::InterpolationMethod::AKIMA,
          R"docstring(
          Resample a spectrum with a provided method.

          Parameters
          ----------
          other : hdrl.core.Spectrum1D
              The spectrum to be resampled.
          method : hdrl.core.InterpolationMethod
              The interpolation method used in resampling.

          Returns
          -------
          hdrl.core.Spectrum1D
              The resampled spectrum.
          )docstring")
      .def(
          "resample_to_wavelengths",
          [](const hdrl::core::Spectrum1D& self,
             py::array_t<double> wavelengths,
             hdrl::core::InterpolationMethod method) {
            std::vector<double> wav_vec = numpy_to_vector(wavelengths);
            return self.resample_to_wavelengths(wav_vec, method);
          },
          py::arg("wavelengths"),
          py::arg("method") = hdrl::core::InterpolationMethod::AKIMA,
          R"docstring(
          Resample a spectrum on the wavelengths with a provided method.

          Parameters
          ----------
          wavelengths: vector of float
              The wavelengths the spectrum has to be resampled on.
          method : hdrl.core.InterpolationMethod
              The interpolation method used in resampling.

          Returns
          -------
          hdrl.core.Spectrum1D
              The resampled spectrum.
          )docstring")
      .def(
          "resample_fit",
          [](const hdrl::core::Spectrum1D& self,
             py::array_t<double> wavelengths, int k, int nCoeff) {
            std::vector<double> wav_vec = numpy_to_vector(wavelengths);

            if (k <= 0 || nCoeff <= 0) {
              throw py::value_error("k and nCoeff must be positive");
            }

            return self.resample_fit(wav_vec, k, nCoeff);
          },
          py::arg("wavelengths"), py::arg("k"), py::arg("nCoeff"),
          R"docstring(
          Resample a spectrum on the wavelengths with B-spline fit.

          Parameters
          ----------
          wavelengths: vector of float
              The wavelengths the spectrum has to be resampled on.
          k : int
              The order of the B-spline.
          nCoeff : int
              The number of coefficients used for the fit.

          Returns
          -------
          hdrl.core.Spectrum1D
              The resampled spectrum.
          )docstring")
      .def(
          "resample_windowed_fit",
          [](const hdrl::core::Spectrum1D& self,
             py::array_t<double> wavelengths, int k, int nCoeff, long window,
             double factor) {
            std::vector<double> wav_vec = numpy_to_vector(wavelengths);

            if (k <= 0 || nCoeff <= 0) {
              throw py::value_error("k and nCoeff must be positive");
            }

            if (window <= 0) {
              throw py::value_error("window must be positive");
            }

            return self.resample_windowed_fit(wav_vec, k, nCoeff, window,
                                              factor);
          },
          py::arg("wavelengths"), py::arg("k"), py::arg("nCoeff"),
          py::arg("window"), py::arg("factor"),
          R"docstring(
          Resample a spectrum on the wavelengths with B-spline fit.

          Parameters
          ----------
          wavelengths: vector of float
              The wavelengths the spectrum has to be resampled on.
          k : int
              The order of the B-spline.
          nCoeff : int
              The number of coefficients used for the fit.
          window : int
              The number of destination wavelengths whose flux values
              are computed using the same model.
          factor : float
              Given window2 = window * factor. window2 is the number of source
              wavelengths used to compute the fit model.

          Returns
          -------
          hdrl.core.Spectrum1D
              The resampled spectrum.
          )docstring")
      .def(
          "resample_integrate",
          [](const hdrl::core::Spectrum1D& self,
             py::array_t<double> wavelengths) {
            std::vector<double> wav_vec = numpy_to_vector(wavelengths);
            return self.resample_integrate(wav_vec);
          },
          py::arg("wavelengths"),
          R"docstring(
          Resample a spectrum on the wavelengths with integration.

          Parameters
          ----------
          wavelengths : vector of float
              The wavelengths the spectrum has to be resampled on.

          Returns
          -------
          hdrl.core.Spectrum1D
              The resampled spectrum.
          )docstring")
      // I/O
      .def(
          "save",
          [](const hdrl::core::Spectrum1D& self,
             const std::filesystem::path& filename) {
            if (filename.empty()) {
              throw py::value_error("Filename cannot be empty");
            }
            self.save(filename);
          },
          py::arg("filename"),
          R"docstring(
          Save the spectrum to file.

          Parameters
          ----------
          filename : std.filesystem.path
              The filename where spectrum will be saved.
          )docstring")
      // Utility methods
      .def(
          "is_compatible_with",
          [](const hdrl::core::Spectrum1D& self,
             const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
            if (!other) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `other`, not "
                  "None");
            }
            return self.is_compatible_with(*other);
          },
          py::arg("other"),
          R"docstring(
          Checks if two spectrum wavelengths are equal.

          Parameters
          ----------
          other : hdrl.core.Spectrum1D
              The spectrum to be compared with.

          Returns
          -------
          boolean
              The flag, true if compatible.
          )docstring")
      .def("duplicate", &hdrl::core::Spectrum1D::duplicate,
           R"docstring(
          Create a duplicate of the spectrum.

          Returns
          -------
          hdrl.core.Spectrum1D
              A new copy of the Spectrum1D.
          )docstring")
      .def(
          "reject_pixels",
          [](const hdrl::core::Spectrum1D& self, py::array_t<int> bad_samples) {
            py::buffer_info buf = bad_samples.request();
            if (buf.ndim != 1) {
              throw py::value_error(
                  "Argument `bad_samples` array number of dimensions must be "
                  "one");
            }
            const int* ptr = static_cast<const int*>(buf.ptr);
            std::vector<int> flags(ptr, ptr + buf.size);
            return self.reject_pixels(flags);
          },
          py::arg("bad_samples"),
          R"docstring(
          For every i-th element in bad_samples having value true,
          the i-th pixel in the 1D spectrum is marked as bad.

          Parameters
          ----------
          bad_samples : array of int
              The flags indicating whether the pixel is bad.

          Returns
          -------
          hdrl.core.Spectrum1D
              The spectrum having the appropriate bad pixels selected.
          )docstring")
      .def("is_uniformly_sampled",
           &hdrl::core::Spectrum1D::is_uniformly_sampled,
           R"docstring(
           Checks if the spectrum is defined on uniformly sampled wavelengths.

           Returns
           -------
           std.pair
               The flag if the spectrum is defined on uniformly sampled
               wavelengths and bin width.

           )docstring")
      // Operator overloads
      .def(
          "__mul__",
          [](const hdrl::core::Spectrum1D& self, double scalar) {
            return self.mul_scalar_create(scalar);
          },
          py::is_operator())
      .def(
          "__imul__",
          [](hdrl::core::Spectrum1D& self, double scalar) {
            self.mul_scalar(scalar);
            return py::cast(&self);
          },
          py::is_operator())
      .def(
          "__truediv__",
          [](const hdrl::core::Spectrum1D& self, double scalar) {
            if (std::fabs(scalar) < DBL_EPSILON) {
              throw py::value_error("Division by zero");
            }
            return self.div_scalar_create(scalar);
          },
          py::is_operator())
      .def(
          "__itruediv__",
          [](hdrl::core::Spectrum1D& self, double scalar) {
            if (std::fabs(scalar) < DBL_EPSILON) {
              throw py::value_error("Division by zero");
            }
            self.div_scalar(scalar);
            return py::cast(&self);
          },
          py::is_operator())
      .def(
          "__add__",
          [](const hdrl::core::Spectrum1D& self, double scalar) {
            return self.add_scalar_create(scalar);
          },
          py::is_operator())
      .def(
          "__iadd__",
          [](hdrl::core::Spectrum1D& self, double scalar) {
            self.add_scalar(scalar);
            return py::cast(&self);
          },
          py::is_operator())
      .def(
          "__sub__",
          [](const hdrl::core::Spectrum1D& self, double scalar) {
            return self.sub_scalar_create(scalar);
          },
          py::is_operator())
      .def(
          "__isub__",
          [](hdrl::core::Spectrum1D& self, double scalar) {
            self.sub_scalar(scalar);
            return py::cast(&self);
          },
          py::is_operator())
      .def("__len__", &hdrl::core::Spectrum1D::get_size)
      .def("__repr__", [](const hdrl::core::Spectrum1D& self) -> std::string {
        return "Spectrum1D(size=" + std::to_string(self.get_size()) +
               ", scale=" +
               (self.get_scale() == hdrl::core::WaveScale::LINEAR ? "linear"
                                                                  : "log") +
               ")";
      });

  // Bind Spectrum1DList
  py::class_<hdrl::core::Spectrum1DList,
             std::shared_ptr<hdrl::core::Spectrum1DList>>
      spectrum1dlist_class(m, "Spectrum1DList");

  spectrum1dlist_class.doc() = R"docstring(
      A hdrl.core.Spectrum1DList is a container for storing hdrl.core.Spectrum1D objects. It provides basic
      list management features. It corresponds to the HDRL spectrum1Dlist.
    )docstring";

  spectrum1dlist_class.def(py::init<>(), "Create an empty spectrum list")
      .def(py::init<std::vector<std::shared_ptr<hdrl::core::Spectrum1D>>>(),
           "Create a spectrum list")
      .def(py::init([](py::list spectra)
                        -> std::shared_ptr<hdrl::core::Spectrum1DList> {
             std::shared_ptr<hdrl::core::Spectrum1DList> self =
                 std::make_shared<hdrl::core::Spectrum1DList>();
             for (auto item : spectra) {
               if (!py::isinstance<hdrl::core::Spectrum1D>(item)) {
                 std::string msg =
                     "expected an hdrl.core.Spectrum1D, not " +
                     py::type::of(item).attr("__name__").cast<std::string>();
                 throw py::type_error(msg);
               }
               std::shared_ptr<hdrl::core::Spectrum1D> spectrum =
                   item.cast<std::shared_ptr<hdrl::core::Spectrum1D>>();
               self->append(spectrum);
             }
             return self;
           }),
           py::arg("spectra"),
           "Create a spectrum list from an array of spectra")
      .def(
          "pop",
          [](hdrl::core::Spectrum1DList& self,
             std::optional<hdrl::core::Spectrum1DList::size_type> index)
              -> std::shared_ptr<hdrl::core::Spectrum1D> {
            hdrl::core::Spectrum1DList::size_type pos =
                index.value_or(self.get_size() - 1);
            if (pos >= self.get_size()) {
              throw py::index_error("Spectrum1DList index out of range");
            }
            return self.pop(pos);
          },
          py::arg("index") = py::none(),
          R"docstring(
          Remove and return the spectrum at the `index`.

          Parameters
          ----------
          index : int, optional
              Index of spectrum to remove from the list. If no index is given
              the last spectrum in the list is removed.

          Returns
          -------
          hdrl.core.Spectrum1D
              Spectrum at `index`.

          Raises
          ------
          IndexError
              If the `index` is out of range.
          )docstring")
      .def("duplicate", &hdrl::core::Spectrum1DList::duplicate,
           R"docstring(
           Create a duplicate of the spectrum list.

           Returns
           -------
           hdrl.core.Spectrum1DList
               A new copy of the Spectrum1DList.
           )docstring")
      .def("__len__", &hdrl::core::Spectrum1DList::get_size,
           "int: number of spectra in the list")
      .def("__deepcopy__",
           [](hdrl::core::Spectrum1DList& self, py::dict /* unused */)
               -> std::shared_ptr<hdrl::core::Spectrum1DList> {
             return self.duplicate();
           })
      .def("__delitem__",
           [](hdrl::core::Spectrum1DList& self,
              hdrl::core::Spectrum1DList::size_type index) -> void {
             if (index >= self.get_size()) {
               throw py::index_error("Spectrum1DList index out of range");
             }
             self.pop(index);
           })
      .def("__getitem__",
           [](const hdrl::core::Spectrum1DList& self,
              hdrl::core::Spectrum1DList::size_type index)
               -> std::shared_ptr<hdrl::core::Spectrum1D> {
             if (index >= self.get_size()) {
               throw py::index_error("Spectrum1DList index out of range");
             }
             return self.get_at(index);
           })
      .def(
          "__setitem__",
          [](hdrl::core::Spectrum1DList& self,
             hdrl::core::Spectrum1DList::size_type index,
             const std::shared_ptr<hdrl::core::Spectrum1D> spectrum) -> void {
            if (index >= self.get_size()) {
              throw py::index_error("Spectrum1DList index out of range");
            }
            self.set(spectrum, index);
          },
          py::arg("index"), py::arg("spectrum"),
          "Assign a spectrum to a list element")
      .def(
          "collapse",
          [](const std::shared_ptr<hdrl::core::Spectrum1DList>& self,
             const std::shared_ptr<hdrl::func::Collapse>& stacking_par,
             py::array_t<double> wavelengths,
             const std::shared_ptr<Spectrum1DResampleMethod>& resample_par,
             bool mark_bpm_in_interpolation) {
            if (!self) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `self`, not "
                  "None");
            }
            if (!stacking_par) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `stacking_par`, "
                  "not "
                  "None");
            }
            if (!resample_par) {
              throw py::type_error(
                  "expected hdrl.core.Spectrum1D as argument `resample_par`, "
                  "not "
                  "None");
            }
            hdrl_parameter* stacking_par_ptr = stacking_par->ptr();
            hdrl_parameter* resample_par_ptr = resample_par->ptr();

            std::vector<double> wav_vec = numpy_to_vector(wavelengths);
            cpl_array* wav_array = hdrl::core::Error::throw_errors_with(
                cpl_array_new, wav_vec.size(), CPL_TYPE_DOUBLE);
            for (std::size_t i = 0; i < wav_vec.size(); ++i) {
              hdrl::core::Error::throw_errors_with(cpl_array_set, wav_array, i,
                                                   wav_vec[i]);
            }

            hdrl_spectrum1D* result = nullptr;
            cpl_image* contrib = nullptr;
            hdrl_imagelist* aligned_fluxes = nullptr;

            hdrl::core::Error::throw_errors_with(
                hdrl_spectrum1Dlist_collapse,
                const_cast<hdrl_spectrum1Dlist*>(self->ptr()), stacking_par_ptr,
                wav_array, resample_par_ptr,
                mark_bpm_in_interpolation ? CPL_TRUE : CPL_FALSE, &result,
                &contrib, &aligned_fluxes);
            hdrl::core::Error::throw_errors_with(cpl_array_delete, wav_array);

            hdrl::core::CollapseResult collapse_result;
            collapse_result.result =
                std::make_shared<hdrl::core::Spectrum1D>(result);
            collapse_result.contrib = hdrl::core::pycpl_image(contrib);
            // Keep API field present but avoid exposing ownership-sensitive
            // native aligned image list in this binding path.
            collapse_result.aligned_images =
                std::make_shared<hdrl::core::ImageList>();
            return collapse_result;
          },
          py::arg("stacking_par"), py::arg("wavelengths"),
          py::arg("resample_par"), py::arg("mark_bpm_in_interpolation") = false,
          R"docstring(
          Collapsing a hdrl.core.Spectrum1DList.

          Parameters
          ----------
          stacking_par : hdrl.func.Collapse
              Parameter regulating the stacking.
          wavelengths : array of float
              Wavelengths the resulting spectrum is defined on.
          resample_par : Spectrum1DResampleMethod
              Parameter regulating the resampling.
          mark_bpm_in_interpolation : boolean, default = False
              If true interpolated pixels whose neighbors (in the original
              spectrum) are rejected, are not considered during collapsing.

          Returns
          -------
          hdrl.core.CollapseResult
              The collapse result object, containing the resulting spectrum,
              output contribution mask, and resampled and aligned fluxes to be
              collapsed.
          )docstring");


  // Bind CollapseResult struct
  py::class_<hdrl::core::CollapseResult>(m, "CollapseResult")
      .def_readonly("result", &hdrl::core::CollapseResult::result,
                    "hdrl.core.CollapseResult.result: The resulting spectrum")
      .def_readonly("aligned_images",
                    &hdrl::core::CollapseResult::aligned_images,
                    "hdrl.core.CollapseResult.aligned_images : The aligned "
                    "fluxes to be collapsed")
      .def_readonly(
          "contrib", &hdrl::core::CollapseResult::contrib,
          "hdrl.core.CollapseResult.contrib : The output contribution mask")
      .def("__repr__",
           [](const hdrl::core::CollapseResult& self) -> std::string {
             std::string result_size =
                 self.result ? std::to_string(self.result->get_size()) : "0";
             return "CollapseResult(result_size=" + result_size +
                    ", aligned_images=ImageList)";
           });
}
