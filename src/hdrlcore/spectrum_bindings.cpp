// This file is part of the PyHDRL Python language bindings
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

#include "spectrum_bindings.hpp"
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <memory>
//#include <csignal>
//#include <csetjmp>
#include <stdexcept>
#include <iostream>
#include <algorithm>  // For std::transform

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "spectrum.hpp"
#include "error.hpp"

namespace py = pybind11;

// Global variables for signal handling
//static sigjmp_buf segfault_buffer;

// Signal handler for segmentation faults
//static void segfault_handler(int signal) {
//    std::cerr << "Segmentation fault caught! Attempting recovery..." << std::endl;
//    siglongjmp(segfault_buffer, 1);
//}

// Function to initialize environment variables
//static void init_environment() {
//    // Set environment variables for memory safety
//    setenv("OMP_NUM_THREADS", "1", 1);
//    setenv("GOMP_CPU_AFFINITY", "0", 1);
//    setenv("MALLOC_CHECK_", "2", 1);
//    setenv("MALLOC_PERTURB_", "123", 1);

//    // Set up signal handler for segmentation faults
//    signal(SIGSEGV, segfault_handler);
//
//    std::cout << "Environment initialized with memory safety settings" << std::endl;
//}

// Function to configure GSL error handling
//static void configure_gsl() {
//    std::cout << "GSL configuration skipped (not available)" << std::endl;
//}

// Helper function to validate input arrays
static void validate_input_arrays(py::buffer_info& flux_buf,
                                 py::buffer_info& flux_e_buf,
                                 py::buffer_info& wav_buf) {
    // Check sizes match and are non-zero
    if (flux_buf.shape[0] == 0 || flux_e_buf.shape[0] == 0 || wav_buf.shape[0] == 0) {
        throw py::value_error("Input arrays must be non-empty");
    }

    if (flux_buf.shape[0] != flux_e_buf.shape[0] || flux_buf.shape[0] != wav_buf.shape[0]) {
        throw py::value_error("Input arrays must have the same size");
    }

    // Check for NaN or Inf values in the arrays
    double* flux_data = static_cast<double*>(flux_buf.ptr);
    double* flux_e_data = static_cast<double*>(flux_e_buf.ptr);
    double* wav_data = static_cast<double*>(wav_buf.ptr);

    size_t size = flux_buf.shape[0];
    for (size_t i = 0; i < size; ++i) {
        if (std::isnan(flux_data[i]) || std::isnan(flux_e_data[i]) || std::isnan(wav_data[i]) ||
            std::isinf(flux_data[i]) || std::isinf(flux_e_data[i]) || std::isinf(wav_data[i])) {
            throw py::value_error("Invalid value (NaN or Inf) in input data at index " + std::to_string(i));
        }
    }

    // Check if wavelengths are sorted
    for (size_t i = 1; i < size; ++i) {
        if (wav_data[i] <= wav_data[i-1]) {
            throw py::value_error("Wavelengths must be strictly increasing");
        }
    }
}

// Helper function to validate wavelength array
static void validate_wavelength_array(py::buffer_info& wav_buf) {
    if (wav_buf.shape[0] == 0) {
        throw py::value_error("Wavelength array must be non-empty");
    }

    double* wav_data = static_cast<double*>(wav_buf.ptr);
    size_t size = wav_buf.shape[0];

    for (size_t i = 0; i < size; ++i) {
        if (std::isnan(wav_data[i]) || std::isinf(wav_data[i])) {
            throw py::value_error("Invalid wavelength value (NaN or Inf) at index " + std::to_string(i));
        }
    }

    // Check if wavelengths are sorted
    for (size_t i = 1; i < size; ++i) {
        if (wav_data[i] <= wav_data[i-1]) {
            throw py::value_error("Wavelengths must be strictly increasing");
        }
    }
}

// Helper function to convert numpy array to vector
static std::vector<double> numpy_to_vector(py::array_t<double>& array) {
    py::buffer_info buf = array.request();
    std::vector<double> vec(buf.size);
    std::memcpy(vec.data(), buf.ptr, buf.size * sizeof(double));
    return vec;
}

// Safe wrapper for HDRL operations that might segfault
//template<typename Func, typename... Args>
//auto safe_hdrl_call(Func func, Args... args)
//    -> decltype(func(std::forward<Args>(args)...))
//{
//    volatile int jump_val = sigsetjmp(segfault_buffer, 1);
//    if (jump_val != 0) {
//        throw py::value_error("Segmentation fault occurred during HDRL operation");
//    }
//
//    return func(std::forward<Args>(args)...);
//}

// Helper function to create a shared_ptr from a raw pointer with proper ownership
//template<typename T>
//static std::shared_ptr<T> make_shared_from_raw(T* ptr) {
//    if (!ptr) {
//        throw py::value_error("Null pointer provided");
//    }
//    return std::shared_ptr<T>(ptr, [](T* p) { delete p; });
//}

void bind_spectrum1d(py::module_& m) {
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
        .def(py::init<double, double, double>(), py::arg("shift"), py::arg("error"), py::arg("quality"))
        .def("get_shift", &hdrl::core::XCorrelationResult::get_shift)
        .def("get_error", &hdrl::core::XCorrelationResult::get_error)
        .def("get_quality", &hdrl::core::XCorrelationResult::get_quality)
        .def_property_readonly("shift", &hdrl::core::XCorrelationResult::get_shift)
        .def_property_readonly("error", &hdrl::core::XCorrelationResult::get_error)
        .def_property_readonly("quality", &hdrl::core::XCorrelationResult::get_quality)
        .def("__repr__", [](const hdrl::core::XCorrelationResult& self) -> std::string {
            return "XCorrelationResult(shift=" + std::to_string(self.get_shift()) +
                   ", error=" + std::to_string(self.get_error()) +
                   ", quality=" + std::to_string(self.get_quality()) + ")";
        });

    // Bind Parameter class
    py::class_<hdrl::core::Parameter>(m, "Parameter")
        .def(py::init<>())
        .def("__repr__", [](const hdrl::core::Parameter& self) -> std::string {
            return "Parameter(ptr=" +
                   std::to_string(reinterpret_cast<uintptr_t>(
                       const_cast<hdrl::core::Parameter&>(self).ptr())) +
                   ")";
        });

    // Bind Spectrum1D with improved constructors and methods
    py::class_<hdrl::core::Spectrum1D, std::shared_ptr<hdrl::core::Spectrum1D>>(m, "Spectrum1D")
        // Environment configuration methods
//        .def_static("init_environment", &init_environment,
//                   "Initialize environment variables for memory safety")
//        .def_static("configure_gsl", &configure_gsl,
//                   "Configure GSL to handle errors gracefully")

        // Constructors with enhanced validation
        .def(py::init<>())
        .def(py::init([](py::array_t<double> flux, py::array_t<double> flux_error,
                         py::array_t<double> wavelengths, std::string scale) {
                try {
                    // Extract buffers from NumPy arrays
                    py::buffer_info flux_buf = flux.request();
                    py::buffer_info flux_e_buf = flux_error.request();
                    py::buffer_info wav_buf = wavelengths.request();

                    // Validate input arrays
                    validate_input_arrays(flux_buf, flux_e_buf, wav_buf);

                    // Convert scale string to enum
                    hdrl::core::WaveScale wave_scale;
                    if (scale == "linear") {
                        wave_scale = hdrl::core::WaveScale::LINEAR;
                    } else if (scale == "log") {
                        wave_scale = hdrl::core::WaveScale::LOG;
                    } else {
                        throw py::value_error("Invalid scale. Must be 'linear' or 'log'.");
                    }

                    return std::make_shared<hdrl::core::Spectrum1D>(
                        hdrl::core::Spectrum1D::create(
                            static_cast<double*>(flux_buf.ptr),
                            static_cast<double*>(flux_e_buf.ptr),
                            static_cast<double*>(wav_buf.ptr),
                            flux_buf.shape[0],
                            wave_scale
                        )
                    );
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create spectrum: ") + e.what());
                }
            }),
            py::arg("flux"), py::arg("flux_error"), py::arg("wavelengths"),
            py::arg("scale") = "linear")

        .def_static("create",
            [](py::array_t<double> flux, py::array_t<double> flux_error,
               py::array_t<double> wavelengths, std::string scale) {
                try {
                    // Extract buffers from NumPy arrays
                    py::buffer_info flux_buf = flux.request();
                    py::buffer_info flux_e_buf = flux_error.request();
                    py::buffer_info wav_buf = wavelengths.request();

                    // Validate input arrays
                    validate_input_arrays(flux_buf, flux_e_buf, wav_buf);

                    // Convert scale string to enum
                    hdrl::core::WaveScale wave_scale;
                    if (scale == "linear") {
                        wave_scale = hdrl::core::WaveScale::LINEAR;
                    } else if (scale == "log") {
                        wave_scale = hdrl::core::WaveScale::LOG;
                    } else {
                        throw py::value_error("Invalid scale. Must be 'linear' or 'log'.");
                    }

                    // Create Spectrum1D
                    return std::make_shared<hdrl::core::Spectrum1D>(
                        hdrl::core::Spectrum1D::create(
                            static_cast<double*>(flux_buf.ptr),
                            static_cast<double*>(flux_e_buf.ptr),
                            static_cast<double*>(wav_buf.ptr),
                            flux_buf.shape[0],
                            wave_scale
                        )
                    );
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create spectrum: ") + e.what());
                }
            },
            py::arg("flux"), py::arg("flux_error"), py::arg("wavelengths"),
            py::arg("scale") = "linear")

        .def_static("create_error_free",
            [](py::array_t<double> flux, py::array_t<double> wavelengths, std::string scale) {
                try {
                    py::buffer_info flux_buf = flux.request();
                    py::buffer_info wav_buf = wavelengths.request();

                    if (flux_buf.shape[0] != wav_buf.shape[0]) {
                        throw py::value_error("Input arrays must have the same size");
                    }

                    if (flux_buf.shape[0] == 0) {
                        throw py::value_error("Input arrays must be non-empty");
                    }

                    // Validate wavelength array
                    validate_wavelength_array(wav_buf);

                    hdrl::core::WaveScale wave_scale = (scale == "log") ?
                        hdrl::core::WaveScale::LOG : hdrl::core::WaveScale::LINEAR;

                    return std::make_shared<hdrl::core::Spectrum1D>(
                        hdrl::core::Spectrum1D::create_error_free(
                            static_cast<double*>(flux_buf.ptr),
                            static_cast<double*>(wav_buf.ptr),
                            flux_buf.shape[0],
                            wave_scale
                        )
                    );
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create error-free spectrum: ") + e.what());
                }
            },
            py::arg("flux"), py::arg("wavelengths"), py::arg("scale") = "linear")

        .def_static("create_error_DER_SNR",
            [](py::array_t<double> flux, size_t half_window,
               py::array_t<double> wavelengths, std::string scale) {
                try {
                    py::buffer_info flux_buf = flux.request();
                    py::buffer_info wav_buf = wavelengths.request();

                    if (flux_buf.shape[0] != wav_buf.shape[0]) {
                        throw py::value_error("Input arrays must have the same size");
                    }

                    if (flux_buf.shape[0] == 0) {
                        throw py::value_error("Input arrays must be non-empty");
                    }

                    // Validate wavelength array
                    validate_wavelength_array(wav_buf);

                    hdrl::core::WaveScale wave_scale = (scale == "log") ?
                        hdrl::core::WaveScale::LOG : hdrl::core::WaveScale::LINEAR;

                    return std::make_shared<hdrl::core::Spectrum1D>(
                        hdrl::core::Spectrum1D::create_error_DER_SNR(
                            static_cast<double*>(flux_buf.ptr),
                            half_window,
                            static_cast<double*>(wav_buf.ptr),
                            flux_buf.shape[0],
                            wave_scale
                        )
                    );
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create DER_SNR spectrum: ") + e.what());
                }
            },
            py::arg("flux"), py::arg("half_window"), py::arg("wavelengths"),
            py::arg("scale") = "linear")

        // Core methods
        .def("size", &hdrl::core::Spectrum1D::size)
        .def("get_scale", &hdrl::core::Spectrum1D::get_scale)

        // Data access methods with numpy array return
        .def("get_flux",
            [](const hdrl::core::Spectrum1D& self) {
                try {
                    std::vector<double> data = self.get_flux_vector();
                    py::array_t<double> array(data.size());
                    auto buf = array.request();
                    double* ptr = static_cast<double*>(buf.ptr);
                    std::memcpy(ptr, data.data(), data.size() * sizeof(double));
                    return array;
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to get flux vector: ") + e.what());
                }
            })
        .def("get_wavelengths",
            [](const hdrl::core::Spectrum1D& self) {
                try {
                    std::vector<double> data = self.get_wavelength_vector();
                    py::array_t<double> array(data.size());
                    auto buf = array.request();
                    double* ptr = static_cast<double*>(buf.ptr);
                    std::memcpy(ptr, data.data(), data.size() * sizeof(double));
                    return array;
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to get wavelength vector: ") + e.what());
                }
            })
        .def("get_flux_error",
            [](const hdrl::core::Spectrum1D& self) {
                try {
                    std::vector<double> data = self.get_flux_error_vector();
                    py::array_t<double> array(data.size());
                    auto buf = array.request();
                    double* ptr = static_cast<double*>(buf.ptr);
                    std::memcpy(ptr, data.data(), data.size() * sizeof(double));
                    return array;
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to get flux error vector: ") + e.what());
                }
            })

        // Arithmetic operations with error handling
        .def("mul_scalar",
            [](hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    self.mul_scalar(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to multiply by scalar: ") + e.what());
                }
            },
            py::arg("scalar"))
        .def("mul_scalar_create",
            [](const hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    return self.mul_scalar_create(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create scaled spectrum: ") + e.what());
                }
            },
            py::arg("scalar"))
        .def("div_scalar",
            [](hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    if (scalar == 0.0) {
                        throw py::value_error("Cannot divide by zero");
                    }
                    self.div_scalar(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to divide by scalar: ") + e.what());
                }
            },
            py::arg("scalar"))
        .def("div_scalar_create",
            [](const hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    if (scalar == 0.0) {
                        throw py::value_error("Cannot divide by zero");
                    }
                    return self.div_scalar_create(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create divided spectrum: ") + e.what());
                }
            },
            py::arg("scalar"))
        .def("add_scalar",
            [](hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    self.add_scalar(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to add scalar: ") + e.what());
                }
            },
            py::arg("scalar"))
        .def("add_scalar_create",
            [](const hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    return self.add_scalar_create(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create spectrum with added scalar: ") + e.what());
                }
            },
            py::arg("scalar"))
        .def("sub_scalar",
            [](hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    self.sub_scalar(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to subtract scalar: ") + e.what());
                }
            },
            py::arg("scalar"))
        .def("sub_scalar_create",
            [](const hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    return self.sub_scalar_create(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create spectrum with subtracted scalar: ") + e.what());
                }
            },
            py::arg("scalar"))
        .def("pow_scalar",
            [](hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    self.pow_scalar(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to raise to power: ") + e.what());
                }
            },
            py::arg("scalar"))
        .def("pow_scalar_create",
            [](const hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    return self.pow_scalar_create(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create spectrum raised to power: ") + e.what());
                }
            },
            py::arg("scalar"))
        .def("exp_scalar",
            [](hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    self.exp_scalar(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to exponentiate: ") + e.what());
                }
            },
            py::arg("scalar"))
        .def("exp_scalar_create",
            [](const hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    return self.exp_scalar_create(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create exponentiated spectrum: ") + e.what());
                }
            },
            py::arg("scalar"))

        // Spectrum-Spectrum operations with error handling
        .def("div_spectrum_create",
            [](const hdrl::core::Spectrum1D& self,
               const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
                try {
                    if (!other) {
                        throw py::value_error("Other spectrum is null");
                    }
                    return self.div_spectrum_create(*other);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to divide spectra: ") + e.what());
                }
            },
            py::arg("other"))
        .def("mul_spectrum_create",
            [](const hdrl::core::Spectrum1D& self,
               const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
                try {
                    if (!other) {
                        throw py::value_error("Other spectrum is null");
                    }
                    return self.mul_spectrum_create(*other);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to multiply spectra: ") + e.what());
                }
            },
            py::arg("other"))
        .def("add_spectrum_create",
            [](const hdrl::core::Spectrum1D& self,
               const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
                try {
                    if (!other) {
                        throw py::value_error("Other spectrum is null");
                    }
                    return self.add_spectrum_create(*other);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to add spectra: ") + e.what());
                }
            },
            py::arg("other"))
        .def("sub_spectrum_create",
            [](const hdrl::core::Spectrum1D& self,
               const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
                try {
                    if (!other) {
                        throw py::value_error("Other spectrum is null");
                    }
                    return self.sub_spectrum_create(*other);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to subtract spectra: ") + e.what());
                }
            },
            py::arg("other"))
        .def("div_spectrum",
            [](hdrl::core::Spectrum1D& self,
               const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
                try {
                    if (!other) {
                        throw py::value_error("Other spectrum is null");
                    }
                    self.div_spectrum(*other);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to divide spectra: ") + e.what());
                }
            },
            py::arg("other"))
        .def("mul_spectrum",
            [](hdrl::core::Spectrum1D& self,
               const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
                try {
                    if (!other) {
                        throw py::value_error("Other spectrum is null");
                    }
                    self.mul_spectrum(*other);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to multiply spectra: ") + e.what());
                }
            },
            py::arg("other"))
        .def("add_spectrum",
            [](hdrl::core::Spectrum1D& self,
               const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
                try {
                    if (!other) {
                        throw py::value_error("Other spectrum is null");
                    }
                    self.add_spectrum(*other);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to add spectra: ") + e.what());
                }
            },
            py::arg("other"))
        .def("sub_spectrum",
            [](hdrl::core::Spectrum1D& self,
               const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
                try {
                    if (!other) {
                        throw py::value_error("Other spectrum is null");
                    }
                    self.sub_spectrum(*other);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to subtract spectra: ") + e.what());
                }
            },
            py::arg("other"))

        // Spectrum Shift Operations with error handling
        .def("compute_shift_xcorrelation",
            [](const hdrl::core::Spectrum1D& self,
               const std::shared_ptr<hdrl::core::Spectrum1D>& other,
               size_t half_win, bool normalize) {
                try {
                    if (!other) {
                        throw py::value_error("Other spectrum is null");
                    }
                    return self.compute_shift_xcorrelation(*other, half_win, normalize);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to compute shift xcorrelation: ") + e.what());
                }
            },
            py::arg("other"), py::arg("half_win"), py::arg("normalize") = true)

        .def_static("create_shift_fit_parameter",
            &hdrl::core::Spectrum1D::create_shift_fit_parameter,
            py::arg("wguess"), py::arg("range_wmin"), py::arg("range_wmax"),
            py::arg("fit_wmin"), py::arg("fit_wmax"), py::arg("fit_half_win"))

        .def("compute_shift_fit",
            [](const hdrl::core::Spectrum1D& self,
               const std::shared_ptr<hdrl::core::Parameter>& par) {
                try {
                    if (!par) {
                        throw py::value_error("Parameter is null");
                    }
                    return self.compute_shift_fit(*par);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to compute shift fit: ") + e.what());
                }
            },
            py::arg("par"))

        // Wavelength Operations with error handling
        .def("wavelength_shift",
            [](hdrl::core::Spectrum1D& self, double shift) {
                try {
                    self.wavelength_shift(shift);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to shift wavelengths: ") + e.what());
                }
            },
            py::arg("shift"))
        .def("wavelength_shift_create",
            [](const hdrl::core::Spectrum1D& self, double shift) {
                try {
                    return self.wavelength_shift_create(shift);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create shifted spectrum: ") + e.what());
                }
            },
            py::arg("shift"))
        .def("wavelength_mult_scalar_linear",
            [](hdrl::core::Spectrum1D& self, double scale) {
                try {
                    if (scale <= 0.0) {
                        throw py::value_error("Scale must be positive");
                    }
                    self.wavelength_mult_scalar_linear(scale);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to multiply wavelengths: ") + e.what());
                }
            },
            py::arg("scale"))
        .def("wavelength_mult_scalar_linear_create",
            [](const hdrl::core::Spectrum1D& self, double scale) {
                try {
                    if (scale <= 0.0) {
                        throw py::value_error("Scale must be positive");
                    }
                    return self.wavelength_mult_scalar_linear_create(scale);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create scaled wavelength spectrum: ") + e.what());
                }
            },
            py::arg("scale"))
        .def("wavelength_convert_to_linear",
            [](hdrl::core::Spectrum1D& self) {
                try {
                    self.wavelength_convert_to_linear();
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to convert to linear scale: ") + e.what());
                }
            })
        .def("wavelength_convert_to_linear_create",
            [](const hdrl::core::Spectrum1D& self) {
                try {
                    return self.wavelength_convert_to_linear_create();
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create linear scale spectrum: ") + e.what());
                }
            })
        .def("wavelength_convert_to_log",
            [](hdrl::core::Spectrum1D& self) {
                try {
                    self.wavelength_convert_to_log();
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to convert to log scale: ") + e.what());
                }
            })
        .def("wavelength_convert_to_log_create",
            [](const hdrl::core::Spectrum1D& self) {
                try {
                    return self.wavelength_convert_to_log_create();
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create log scale spectrum: ") + e.what());
                }
            })

        // Spectrum Selection with error handling
        .def("select_window",
            [](const hdrl::core::Spectrum1D& self, double lambda_min, double lambda_max, bool is_internal) {
                try {
                    if (lambda_min >= lambda_max) {
                        throw py::value_error("lambda_min must be less than lambda_max");
                    }
                    return self.select_window(lambda_min, lambda_max, is_internal);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to select wavelength window: ") + e.what());
                }
            },
            py::arg("lambda_min"), py::arg("lambda_max"), py::arg("is_internal") = false)

        // Spectrum Resample Operations with enhanced safety
        .def("resample",
            [](const hdrl::core::Spectrum1D& self,
               const std::shared_ptr<hdrl::core::Spectrum1D>& other,
               hdrl::core::InterpolationMethod method) {
                try {
                    if (!other) {
                        throw py::value_error("Other spectrum is null");
                    }
                    return self.resample(*other, method);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    // Try with a simpler interpolation method as fallback
                    try {
                        return self.resample(*other, hdrl::core::InterpolationMethod::LINEAR);
                    } catch (const std::exception& e2) {
                        throw py::value_error(std::string("Resampling failed with both original and fallback methods: ") + e2.what());
                    }
                }
            },
            py::arg("other"),
            py::arg("method") = hdrl::core::InterpolationMethod::AKIMA)

        .def("resample_to_wavelengths",
            [](const hdrl::core::Spectrum1D& self,
               py::array_t<double> wavelengths,
               hdrl::core::InterpolationMethod method) {
                try {
                    std::vector<double> wav_vec = numpy_to_vector(wavelengths);
                    return self.resample_to_wavelengths(wav_vec, method);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    // Try with linear interpolation as fallback
                    try {
                        std::vector<double> wav_vec = numpy_to_vector(wavelengths);
                        return self.resample_to_wavelengths(wav_vec, hdrl::core::InterpolationMethod::LINEAR);
                    } catch (const std::exception& e2) {
                        throw py::value_error(std::string("Resampling to wavelengths failed with both original and fallback methods: ") + e2.what());
                    }
                }
            },
            py::arg("wavelengths"),
            py::arg("method") = hdrl::core::InterpolationMethod::AKIMA)

        .def("resample_fit",
            [](const hdrl::core::Spectrum1D& self,
               py::array_t<double> wavelengths, int k, int nCoeff) {
                try {
                    std::vector<double> wav_vec = numpy_to_vector(wavelengths);

                    if (k <= 0 || nCoeff <= 0) {
                        throw py::value_error("k and nCoeff must be positive");
                    }

                    return self.resample_fit(wav_vec, k, nCoeff);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    // Try with linear interpolation as fallback
                    try {
                        std::vector<double> wav_vec = numpy_to_vector(wavelengths);
                        return self.resample_to_wavelengths(wav_vec, hdrl::core::InterpolationMethod::LINEAR);
                    } catch (const std::exception& e2) {
                        throw py::value_error(std::string("Resampling fit failed with both original and fallback methods: ") + e2.what());
                    }
                }
            },
            py::arg("wavelengths"), py::arg("k"), py::arg("nCoeff"))

        .def("resample_windowed_fit",
            [](const hdrl::core::Spectrum1D& self,
               py::array_t<double> wavelengths, int k, int nCoeff, long window,
               double factor) {
                try {
                    std::vector<double> wav_vec = numpy_to_vector(wavelengths);

                    if (k <= 0 || nCoeff <= 0) {
                        throw py::value_error("k and nCoeff must be positive");
                    }

                    if (window <= 0) {
                        throw py::value_error("window must be positive");
                    }

                    return self.resample_windowed_fit(wav_vec, k, nCoeff, window, factor);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    // Try with linear interpolation as fallback
                    try {
                        std::vector<double> wav_vec = numpy_to_vector(wavelengths);
                        return self.resample_to_wavelengths(wav_vec, hdrl::core::InterpolationMethod::LINEAR);
                    } catch (const std::exception& e2) {
                        throw py::value_error(std::string("Resampling windowed fit failed with both original and fallback methods: ") + e2.what());
                    }
                }
            },
            py::arg("wavelengths"), py::arg("k"), py::arg("nCoeff"),
            py::arg("window"), py::arg("factor"))

        .def("resample_integrate",
            [](const hdrl::core::Spectrum1D& self,
               py::array_t<double> wavelengths) {
                try {
                    std::vector<double> wav_vec = numpy_to_vector(wavelengths);
                    return self.resample_integrate(wav_vec);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Resampling integrate failed: ") + e.what());
                }
            },
            py::arg("wavelengths"))

        // I/O with error handling
        .def("save",
            [](const hdrl::core::Spectrum1D& self, const std::string& filename) {
                try {
                    if (filename.empty()) {
                        throw py::value_error("Filename cannot be empty");
                    }
                    self.save(filename);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to save spectrum: ") + e.what());
                }
            },
            py::arg("filename"))

        // Utility methods with error handling
        .def("is_compatible_with",
            [](const hdrl::core::Spectrum1D& self,
               const std::shared_ptr<hdrl::core::Spectrum1D>& other) {
                try {
                    if (!other) {
                        throw py::value_error("Other spectrum is null");
                    }
                    return self.is_compatible_with(*other);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to check spectrum compatibility: ") + e.what());
                }
            },
            py::arg("other"))

        .def("duplicate",
            [](const hdrl::core::Spectrum1D& self) {
                try {
                    return self.duplicate();
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to duplicate spectrum: ") + e.what());
                }
            })

        // Operator overloads with error handling
        .def(
            "__mul__",
            [](const hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    return self.mul_scalar_create(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Multiplication failed: ") + e.what());
                }
            },
            py::is_operator())
        .def(
            "__imul__",
            [](hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    self.mul_scalar(scalar);
                    return py::cast(&self);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("In-place multiplication failed: ") + e.what());
                }
            },
            py::is_operator())
        .def(
            "__truediv__",
            [](const hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    if (scalar == 0.0) {
                        throw py::value_error("Cannot divide by zero");
                    }
                    return self.div_scalar_create(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Division failed: ") + e.what());
                }
            },
            py::is_operator())
        .def(
            "__itruediv__",
            [](hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    if (scalar == 0.0) {
                        throw py::value_error("Cannot divide by zero");
                    }
                    self.div_scalar(scalar);
                    return py::cast(&self);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("In-place division failed: ") + e.what());
                }
            },
            py::is_operator())
        .def(
            "__add__",
            [](const hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    return self.add_scalar_create(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Addition failed: ") + e.what());
                }
            },
            py::is_operator())
        .def(
            "__iadd__",
            [](hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    self.add_scalar(scalar);
                    return py::cast(&self);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("In-place addition failed: ") + e.what());
                }
            },
            py::is_operator())
        .def(
            "__sub__",
            [](const hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    return self.sub_scalar_create(scalar);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Subtraction failed: ") + e.what());
                }
            },
            py::is_operator())
        .def(
            "__isub__",
            [](hdrl::core::Spectrum1D& self, double scalar) {
                try {
                    self.sub_scalar(scalar);
                    return py::cast(&self);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("In-place subtraction failed: ") + e.what());
                }
            },
            py::is_operator())

        .def("__repr__", [](const hdrl::core::Spectrum1D& self) -> std::string {
            try {
                return "Spectrum1D(size=" + std::to_string(self.size()) + ", scale=" +
                       (self.get_scale() == hdrl::core::WaveScale::LINEAR ? "linear" : "log") +
                       ")";
            } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                return "Spectrum1D(invalid)";
            }
        });

    // Bind Spectrum1DList with proper initialization and error handling
    py::class_<hdrl::core::Spectrum1DList, std::shared_ptr<hdrl::core::Spectrum1DList>>(m, "Spectrum1DList")
        .def(py::init([](py::args args) {
            try {
                return std::make_shared<hdrl::core::Spectrum1DList>(hdrl::core::Spectrum1DList::create());
            } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                throw py::value_error(std::string("Failed to create spectrum list: ") + e.what());
            }
        }), "Create an empty spectrum list")

        .def_static("create", []() {
            try {
                return std::make_shared<hdrl::core::Spectrum1DList>(hdrl::core::Spectrum1DList::create());
            } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                throw py::value_error(std::string("Failed to create spectrum list: ") + e.what());
            }
        }, "Create a new spectrum list")

        .def_static("create_from_array",
            [](py::list spectra_list) {
                try {
                    auto list = std::make_shared<hdrl::core::Spectrum1DList>(hdrl::core::Spectrum1DList::create());

                    for (size_t i = 0; i < py::len(spectra_list); i++) {
                        auto spec = spectra_list[i].cast<std::shared_ptr<hdrl::core::Spectrum1D>>();
                        if (!spec) {
                            throw py::value_error("Null spectrum in list at index " + std::to_string(i));
                        }
                        list->set(i, *spec);
                    }

                    return list;
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to create spectrum list from array: ") + e.what());
                }
            },
            py::arg("spectra"),
            "Create a spectrum list from an array of spectra")

        .def("size",
            [](const std::shared_ptr<hdrl::core::Spectrum1DList>& list) {
                try {
                    return list->size();
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to get spectrum list size: ") + e.what());
                }
            },
            "Get the number of spectra in the list")

        .def("get",
            [](const std::shared_ptr<hdrl::core::Spectrum1DList>& list, size_t index) {
                try {
                    if (index >= list->size()) {
                        throw py::value_error("Index out of range");
                    }
                    return std::make_shared<hdrl::core::Spectrum1D>(list->get(index));
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to get spectrum from list: ") + e.what());
                }
            },
            py::arg("index"),
            "Get a spectrum from the list")

        .def("set",
            [](std::shared_ptr<hdrl::core::Spectrum1DList>& list, size_t index,
               const std::shared_ptr<hdrl::core::Spectrum1D>& spectrum) {
                try {
                    if (!spectrum) {
                        throw py::value_error("Null spectrum provided");
                    }
                    if (index >= list->size()) {
                        throw py::value_error("Index out of range");
                    }
                    list->set(index, *spectrum);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to set spectrum in list: ") + e.what());
                }
            },
            py::arg("index"), py::arg("spectrum"),
            "Set a spectrum in the list")

        .def("unset",
            [](std::shared_ptr<hdrl::core::Spectrum1DList>& list, size_t index) {
                try {
                    if (index >= list->size()) {
                        throw py::value_error("Index out of range");
                    }
                    return std::make_shared<hdrl::core::Spectrum1D>(list->unset(index));
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to unset spectrum from list: ") + e.what());
                }
            },
            py::arg("index"),
            "Remove and return a spectrum from the list")

        .def("duplicate",
            [](const std::shared_ptr<hdrl::core::Spectrum1DList>& list) {
                try {
                    return std::make_shared<hdrl::core::Spectrum1DList>(list->duplicate());
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to duplicate spectrum list: ") + e.what());
                }
            },
            "Create a duplicate of the spectrum list")

        .def("__len__",
            [](const std::shared_ptr<hdrl::core::Spectrum1DList>& list) {
                try {
                    return list->size();
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to get spectrum list length: ") + e.what());
                }
            })
        .def("__getitem__",
            [](const std::shared_ptr<hdrl::core::Spectrum1DList>& list, size_t index) {
                try {
                    if (index >= list->size()) {
                        throw py::value_error("Index out of range");
                    }
                    return std::make_shared<hdrl::core::Spectrum1D>(list->get(index));
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to get item from spectrum list: ") + e.what());
                }
            })
        .def("__setitem__",
            [](std::shared_ptr<hdrl::core::Spectrum1DList>& list, size_t index,
               const std::shared_ptr<hdrl::core::Spectrum1D>& spectrum) {
                try {
                    if (!spectrum) {
                        throw py::value_error("Null spectrum provided");
                    }
                    if (index >= list->size()) {
                        throw py::value_error("Index out of range");
                    }
                    list->set(index, *spectrum);
                } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                    throw py::value_error(std::string("Failed to set item in spectrum list: ") + e.what());
                }
            });

    #if defined HDRL_USE_EXPERIMENTAL || defined HDRL_USE_PRIVATE
    // Bind CollapseResult struct with proper ImageList handling
    py::class_<hdrl::core::CollapseResult>(m, "CollapseResult")
        .def_readonly("result", &hdrl::core::CollapseResult::result)
        .def_readonly("aligned_images", &hdrl::core::CollapseResult::aligned_images)
        .def("__repr__", [](const hdrl::core::CollapseResult& self) -> std::string {
            try {
                std::string result_size = std::to_string(self.result.size());
                return "CollapseResult(result_size=" + result_size + ", aligned_images=ImageList)";
            } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                return "CollapseResult(invalid)";
            }
        });

    // Add collapse method to Spectrum1DList with error handling
    m.def("spectrum1dlist_collapse",
        [](const std::shared_ptr<hdrl::core::Spectrum1DList>& list,
           const std::shared_ptr<hdrl::core::Parameter>& stacking_par,
           py::array_t<double> wavelengths,
           const std::shared_ptr<hdrl::core::Parameter>& resample_par,
           bool mark_bpm_in_interpolation) {
            try {
                if (!list) {
                    throw py::value_error("Spectrum list is null");
                }
                if (!stacking_par) {
                    throw py::value_error("Stacking parameter is null");
                }
                if (!resample_par) {
                    throw py::value_error("Resample parameter is null");
                }

                std::vector<double> wav_vec = numpy_to_vector(wavelengths);
                return list->collapse(*stacking_par, wav_vec, *resample_par, mark_bpm_in_interpolation);
            } catch (const hdrl::core::Error&) { throw; }
                catch (const std::exception& e) {
                throw py::value_error(std::string("Failed to collapse spectrum list: ") + e.what());
            }
        },
        py::arg("list"), py::arg("stacking_par"), py::arg("wavelengths"),
        py::arg("resample_par"), py::arg("mark_bpm_in_interpolation") = false);
    #endif

    // Add module-level environment initialization function
//    m.def("init_environment", &init_environment,
//         "Initialize environment variables for memory safety");
//    m.def("configure_gsl", &configure_gsl,
//         "Configure GSL to handle errors gracefully");
}
