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


#include "spectrum.hpp"

#include <cstddef>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cmath>
//#include <csignal>
//#include <csetjmp>

#include <cpl_bivector.h>
#include <cpl_error.h>
#include <cpl_memory.h>
#include <cpl_table.h>
#include <cpl_vector.h>
#include <hdrl_image.h>
#include <hdrl_imagelist.h>
#include <hdrl_parameter.h>
#include <hdrl_spectrum_shift.h>
#include <hdrl_types.h>

#include "error.hpp"

// Helper functions to access xcorrelation result values
double get_xcorrelation_shift(hdrl_xcorrelation_result* result) {
    if (!result) {
        throw hdrl::core::NullInputError(
            HDRL_ERROR_LOCATION,
            "Null xcorrelation result provided to get_xcorrelation_shift");
    }
    return result->peakpos;
}

double get_xcorrelation_error(hdrl_xcorrelation_result* result) {
    if (!result) {
        throw hdrl::core::NullInputError(
            HDRL_ERROR_LOCATION,
            "Null xcorrelation result provided to get_xcorrelation_error");
    }
    return result->sigma;
}

double get_xcorrelation_quality(hdrl_xcorrelation_result* result) {
    if (!result) {
        throw hdrl::core::NullInputError(
            HDRL_ERROR_LOCATION,
            "Null xcorrelation result provided to get_xcorrelation_quality");
    }
    return result->mse;
}

// RAII wrapper for CPL images
class CPLImageGuard {
    cpl_image* img_;
public:
    explicit CPLImageGuard(cpl_image* img) : img_(img) {}
    ~CPLImageGuard() { if (img_) cpl_image_delete(img_); }
    cpl_image* get() const { return img_; }
    cpl_image* release() { cpl_image* tmp = img_; img_ = nullptr; return tmp; }
};

// RAII wrapper for CPL arrays
class CPLArrayGuard {
    cpl_array* arr_;
public:
    explicit CPLArrayGuard(cpl_array* arr) : arr_(arr) {}
    ~CPLArrayGuard() { if (arr_) cpl_array_delete(arr_); }
    cpl_array* get() const { return arr_; }
    cpl_array* release() { cpl_array* tmp = arr_; arr_ = nullptr; return tmp; }
};

// RAII wrapper for CPL bivectors
class CPLBivectorGuard {
    cpl_bivector* bivec_;
public:
    explicit CPLBivectorGuard(cpl_bivector* bivec) : bivec_(bivec) {}
    ~CPLBivectorGuard() { if (bivec_) cpl_bivector_delete(bivec_); }
    cpl_bivector* get() const { return bivec_; }
    cpl_bivector* release() { cpl_bivector* tmp = bivec_; bivec_ = nullptr; return tmp; }
};

// Helper function to create CPL images safely
static CPLImageGuard create_safe_image(size_t size, const double* data) {
    cpl_image* img = hdrl::core::Error::throw_errors_with(cpl_image_new, size, 1, CPL_TYPE_DOUBLE);
    if (!img) {
        throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION, "Failed to create image");
    }

    for (size_t i = 0; i < size; ++i) {
        hdrl::core::Error::throw_errors_with(cpl_image_set, img, i + 1, 1, data[i]);
    }

    return CPLImageGuard(img);
}

// Helper function to create CPL arrays safely
static CPLArrayGuard create_safe_array(size_t size, const double* data) {
    cpl_array* arr = hdrl::core::Error::throw_errors_with(cpl_array_new, size, CPL_TYPE_DOUBLE);
    if (!arr) {
        throw hdrl::core::NullInputError(HDRL_ERROR_LOCATION, "Failed to create array");
    }

    for (size_t i = 0; i < size; ++i) {
        hdrl::core::Error::throw_errors_with(cpl_array_set, arr, i, data[i]);
    }

    return CPLArrayGuard(arr);
}

// Helper function to check if interpolation is possible
static bool can_interpolate(size_t size, hdrl_spectrum1D_interpolation_method method) {
    // For spline-based methods, we need at least 2 points
    if (method == hdrl_spectrum1D_interp_akima || method == hdrl_spectrum1D_interp_cspline) {
        return size >= 2;
    }
    // For linear interpolation, we need at least 2 points
    if (method == hdrl_spectrum1D_interp_linear) {
        return size >= 2;
    }
    // For other methods, we might have different requirements
    return size >= 1;
}

namespace hdrl {
namespace core {

// --- XCorrelationResult Implementation ---
XCorrelationResult::XCorrelationResult(double shift, double error, double quality)
    : m_shift(shift), m_error(error), m_quality(quality), m_result(nullptr) {}

XCorrelationResult::XCorrelationResult(hdrl_xcorrelation_result* result)
    : m_result(result) {
    if (!m_result) {
        throw NullInputError(
            HDRL_ERROR_LOCATION,
            "Null xcorrelation result provided to XCorrelationResult");
    }

    // Extract values using the helper functions
    m_shift = get_xcorrelation_shift(m_result);
    m_error = get_xcorrelation_error(m_result);
    m_quality = get_xcorrelation_quality(m_result);
}

XCorrelationResult::~XCorrelationResult() {}

XCorrelationResult::XCorrelationResult(XCorrelationResult&& other) noexcept
    : m_shift(other.m_shift), m_error(other.m_error),
      m_quality(other.m_quality), m_result(other.m_result) {
    other.m_result = nullptr;
}

XCorrelationResult& XCorrelationResult::operator=(XCorrelationResult&& other) noexcept {
    if (this != &other) {
        m_shift = other.m_shift;
        m_error = other.m_error;
        m_quality = other.m_quality;
        m_result = other.m_result;
        other.m_result = nullptr;
    }
    return *this;
}

// --- Spectrum1D Implementation ---

// --- Constructors/Destructors ---
Spectrum1D::Spectrum1D() : m_interface(nullptr) {}

Spectrum1D::Spectrum1D(hdrl_spectrum1D* spectrum) : m_interface(spectrum) {
    if (!m_interface) {
        throw NullInputError(HDRL_ERROR_LOCATION,
                           "Null spectrum provided to owning Spectrum1D");
    }
}

Spectrum1D::Spectrum1D(const hdrl_spectrum1D* spectrum)
    : m_interface(const_cast<hdrl_spectrum1D*>(spectrum)) {
    if (!m_interface) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Null spectrum provided to Spectrum1D");
    }
}

Spectrum1D::~Spectrum1D() {
    if (m_interface) {
        hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_delete, &m_interface);
    }
}

Spectrum1D::Spectrum1D(Spectrum1D&& other) noexcept
    : m_interface(other.m_interface) {
    other.m_interface = nullptr;
}

Spectrum1D& Spectrum1D::operator=(Spectrum1D&& other) noexcept {
    if (this != &other) {
        if (m_interface) {
            hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_delete, &m_interface);
        }
        m_interface = other.m_interface;
        other.m_interface = nullptr;
    }
    return *this;
}

// --- Static Constructors ---
Spectrum1D Spectrum1D::create(const double* flux, const double* flux_e,
                            const double* wavelengths, size_t size, WaveScale scale) {
    if (size == 0) {
        throw NullInputError(HDRL_ERROR_LOCATION,"Cannot create spectrum with zero size");
    }

    // Check for null pointers
    if (!flux || !flux_e || !wavelengths) {
        throw NullInputError(HDRL_ERROR_LOCATION,"Null pointer provided to spectrum creation");
    }

    try {
        CPLImageGuard flux_img = create_safe_image(size, flux);
        CPLImageGuard flux_e_img = create_safe_image(size, flux_e);
        CPLArrayGuard wavelength_arr = create_safe_array(size, wavelengths);

        hdrl_spectrum1D* spec = hdrl::core::Error::throw_errors_with(
            hdrl_spectrum1D_create,
            flux_img.get(), flux_e_img.get(), wavelength_arr.get(),
            static_cast<hdrl_spectrum1D_wave_scale>(scale));

        if (!spec) {
            throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create spectrum");
        }

        return Spectrum1D(spec);
    } catch (...) {
        throw;
    }
}

Spectrum1D Spectrum1D::create_error_free(const double* flux,
                                       const double* wavelengths, size_t size, WaveScale scale) {
    if (size == 0) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Cannot create spectrum with zero size");
    }

    try {
        CPLImageGuard flux_img = create_safe_image(size, flux);
        CPLArrayGuard wavelength_arr = create_safe_array(size, wavelengths);

        hdrl_spectrum1D* spec = hdrl::core::Error::throw_errors_with(
            hdrl_spectrum1D_create_error_free,
            flux_img.get(), wavelength_arr.get(),
            static_cast<hdrl_spectrum1D_wave_scale>(scale));

        if (!spec) {
            throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create error-free spectrum");
        }

        return Spectrum1D(spec);
    } catch (...) {
        throw;
    }
}

Spectrum1D Spectrum1D::create_error_DER_SNR(const double* flux, size_t half_window,
                                          const double* wavelengths, size_t size, WaveScale scale) {
    if (size == 0) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Cannot create spectrum with zero size");
    }

    try {
        CPLImageGuard flux_img = create_safe_image(size, flux);
        CPLArrayGuard wavelength_arr = create_safe_array(size, wavelengths);

        hdrl_spectrum1D* spec = hdrl::core::Error::throw_errors_with(
            hdrl_spectrum1D_create_error_DER_SNR,
            flux_img.get(), half_window, wavelength_arr.get(),
            static_cast<hdrl_spectrum1D_wave_scale>(scale));

        if (!spec) {
            throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create DER_SNR spectrum");
        }

        return Spectrum1D(spec);
    } catch (...) {
        throw;
    }
}

Spectrum1D Spectrum1D::create_analytic(
    std::function<std::pair<double, double>(double)> func,
    const double* wavelengths, size_t size, WaveScale scale) {
    // Create a wrapper function for the C API
    auto wrapper = [](hdrl_data_t lambda) -> hdrl_value {
        static std::function<std::pair<double, double>(double)>* stored_func = nullptr;
        static std::pair<double, double> default_result{0.0, 0.0};

        if (!stored_func) {
            return {default_result.first, default_result.second};
        }

        auto result = (*stored_func)(lambda);
        return {result.first, result.second};
    };

    // Store the function pointer temporarily
    static std::function<std::pair<double, double>(double)>* func_ptr = &func;
    wrapper(0.0);  // Just to set the pointer

    cpl_array* wavelength_arr = hdrl::core::Error::throw_errors_with(cpl_array_new, size, CPL_TYPE_DOUBLE);
    for (size_t i = 0; i < size; ++i) {
        hdrl::core::Error::throw_errors_with(cpl_array_set, wavelength_arr, i, wavelengths[i]);
    }

    hdrl_spectrum1D* spec = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_create_analytic,
        wrapper, wavelength_arr, static_cast<hdrl_spectrum1D_wave_scale>(scale));

    hdrl::core::Error::throw_errors_with(cpl_array_delete, wavelength_arr);

    if (!spec) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create analytic spectrum");
    }
    return Spectrum1D(spec);
}

// --- Core Methods ---
size_t Spectrum1D::size() const {
    if (!m_interface) return 0;
    return hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_size, m_interface);
}

WaveScale Spectrum1D::get_scale() const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    return static_cast<WaveScale>(hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_scale, m_interface));
}

const hdrl_image* Spectrum1D::get_flux() const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    return hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_flux, m_interface);
}

const cpl_array* Spectrum1D::get_wavelengths() const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_spectrum1D_wavelength spec_wav = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_wavelength, m_interface);
    return spec_wav.wavelength;
}

// --- Arithmetic Operations ---
void Spectrum1D::mul_scalar(double scalar) {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_value value = {scalar, 0.0};
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_mul_scalar, m_interface, value);
}

Spectrum1D Spectrum1D::mul_scalar_create(double scalar) const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_value value = {scalar, 0.0};
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_mul_scalar_create, m_interface, value);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create scaled spectrum");
    }
    return Spectrum1D(result);
}

void Spectrum1D::div_scalar(double scalar) {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_value value = {scalar, 0.0};
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_div_scalar, m_interface, value);
}

Spectrum1D Spectrum1D::div_scalar_create(double scalar) const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_value value = {scalar, 0.0};
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_div_scalar_create, m_interface, value);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create divided spectrum");
    }
    return Spectrum1D(result);
}

void Spectrum1D::add_scalar(double scalar) {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_value value = {scalar, 0.0};
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_add_scalar, m_interface, value);
}

Spectrum1D Spectrum1D::add_scalar_create(double scalar) const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_value value = {scalar, 0.0};
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_add_scalar_create, m_interface, value);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create spectrum with added scalar");
    }
    return Spectrum1D(result);
}

void Spectrum1D::sub_scalar(double scalar) {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_value value = {scalar, 0.0};
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_sub_scalar, m_interface, value);
}

Spectrum1D Spectrum1D::sub_scalar_create(double scalar) const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_value value = {scalar, 0.0};
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_sub_scalar_create, m_interface, value);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create spectrum with subtracted scalar");
    }
    return Spectrum1D(result);
}

void Spectrum1D::pow_scalar(double scalar) {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_value value = {scalar, 0.0};
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_pow_scalar, m_interface, value);
}

Spectrum1D Spectrum1D::pow_scalar_create(double scalar) const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_value value = {scalar, 0.0};
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_pow_scalar_create, m_interface, value);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create spectrum raised to power");
    }
    return Spectrum1D(result);
}

void Spectrum1D::exp_scalar(double scalar) {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_value value = {scalar, 0.0};
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_exp_scalar, m_interface, value);
}

Spectrum1D Spectrum1D::exp_scalar_create(double scalar) const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_value value = {scalar, 0.0};
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_exp_scalar_create, m_interface, value);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create exponentiated spectrum");
    }
    return Spectrum1D(result);
}

// --- Spectrum-Spectrum Operations ---
Spectrum1D Spectrum1D::div_spectrum_create(const Spectrum1D& other) const {
    if (!m_interface || !other.m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum in division");
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_div_spectrum_create, m_interface, other.m_interface);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to divide spectra");
    }
    return Spectrum1D(result);
}

Spectrum1D Spectrum1D::mul_spectrum_create(const Spectrum1D& other) const {
    if (!m_interface || !other.m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum in multiplication");
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_mul_spectrum_create, m_interface, other.m_interface);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to multiply spectra");
    }
    return Spectrum1D(result);
}

Spectrum1D Spectrum1D::add_spectrum_create(const Spectrum1D& other) const {
    if (!m_interface || !other.m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum in addition");
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_add_spectrum_create, m_interface, other.m_interface);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to add spectra");
    }
    return Spectrum1D(result);
}

Spectrum1D Spectrum1D::sub_spectrum_create(const Spectrum1D& other) const {
    if (!m_interface || !other.m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum in subtraction");
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_sub_spectrum_create, m_interface, other.m_interface);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to subtract spectra");
    }
    return Spectrum1D(result);
}

void Spectrum1D::div_spectrum(const Spectrum1D& other) {
    if (!m_interface || !other.m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum in division");
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_div_spectrum, m_interface, other.m_interface);
}

void Spectrum1D::mul_spectrum(const Spectrum1D& other) {
    if (!m_interface || !other.m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum in multiplication");
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_mul_spectrum, m_interface, other.m_interface);
}

void Spectrum1D::add_spectrum(const Spectrum1D& other) {
    if (!m_interface || !other.m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum in addition");
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_add_spectrum, m_interface, other.m_interface);
}

void Spectrum1D::sub_spectrum(const Spectrum1D& other) {
    if (!m_interface || !other.m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum in subtraction");
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_sub_spectrum, m_interface, other.m_interface);
}

// --- Spectrum Shift Operations ---
XCorrelationResult Spectrum1D::compute_shift_xcorrelation(const Spectrum1D& other, size_t half_win,
                                                          bool normalize) const {
    if (!m_interface || !other.m_interface) {
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum in shift computation");
    }

    hdrl_xcorrelation_result* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_compute_shift_xcorrelation,
        m_interface, other.m_interface, half_win, normalize ? CPL_TRUE : CPL_FALSE);

    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to compute shift");
    }

    // Extract values from the result structure
    double shift = result->peakpos;
    double error = result->sigma;
    double quality = result->mse;

    // Free the result structure
    hdrl::core::Error::throw_errors_with(hdrl_xcorrelation_result_delete, result);

    return XCorrelationResult(shift, error, quality);
}

Parameter Spectrum1D::create_shift_fit_parameter(double wguess, double range_wmin,
                                                double range_wmax, double fit_wmin,
                                                double fit_wmax, double fit_half_win) {
    hdrl_parameter* param = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_shift_fit_parameter_create,
        wguess, range_wmin, range_wmax, fit_wmin, fit_wmax, fit_half_win);
    return Parameter(param);
}

double Spectrum1D::compute_shift_fit(const Parameter& par) const {
    if (!m_interface) {
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum in shift fit computation");
    }

    double shift = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_compute_shift_fit,
        m_interface, const_cast<Parameter&>(par).ptr());

    return shift;
}

// --- Wavelength Operations ---
void Spectrum1D::wavelength_shift(double shift) {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_wavelength_shift, m_interface, shift);
}

Spectrum1D Spectrum1D::wavelength_shift_create(double shift) const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_wavelength_shift_create, m_interface, shift);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create shifted spectrum");
    }
    return Spectrum1D(result);
}

void Spectrum1D::wavelength_mult_scalar_linear(double scale) {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_wavelength_mult_scalar_linear, m_interface, scale);
}

Spectrum1D Spectrum1D::wavelength_mult_scalar_linear_create(double scale) const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_wavelength_mult_scalar_linear_create, m_interface, scale);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create scaled wavelength spectrum");
    }
    return Spectrum1D(result);
}

void Spectrum1D::wavelength_convert_to_linear() {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_wavelength_convert_to_linear, m_interface);
}

Spectrum1D Spectrum1D::wavelength_convert_to_linear_create() const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_wavelength_convert_to_linear_create, m_interface);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create linear scale spectrum");
    }
    return Spectrum1D(result);
}

void Spectrum1D::wavelength_convert_to_log() {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_wavelength_convert_to_log, m_interface);
}

Spectrum1D Spectrum1D::wavelength_convert_to_log_create() const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_wavelength_convert_to_log_create, m_interface);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create log scale spectrum");
    }
    return Spectrum1D(result);
}

// --- Spectrum Selection ---
Spectrum1D Spectrum1D::select_window(double lambda_min, double lambda_max, bool is_internal) const {
    if (!m_interface) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Null spectrum in select_window");
    }

    // Create a bivector for the window
    CPLBivectorGuard win_guard(cpl_bivector_new(1));
    if (!win_guard.get()) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create bivector");
    }

    hdrl::core::Error::throw_errors_with(cpl_vector_set, cpl_bivector_get_x(win_guard.get()), 0, lambda_min);
    hdrl::core::Error::throw_errors_with(cpl_vector_set, cpl_bivector_get_y(win_guard.get()), 0, lambda_max);

    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_select_wavelengths,
        m_interface, win_guard.get(), is_internal ? CPL_TRUE : CPL_FALSE);

    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to select wavelength window");
    }

    return Spectrum1D(result);
}

// --- Spectrum Resample Operations ---
hdrl_spectrum1D_interpolation_method Spectrum1D::to_c_interp_method(InterpolationMethod method) {
    switch (method) {
        case InterpolationMethod::LINEAR: return hdrl_spectrum1D_interp_linear;
        case InterpolationMethod::CSPLINE: return hdrl_spectrum1D_interp_cspline;
        case InterpolationMethod::AKIMA:
        default: return hdrl_spectrum1D_interp_akima;
    }
}

Parameter Spectrum1D::create_interpolation_parameter(InterpolationMethod method) {
    hdrl_spectrum1D_interpolation_method c_method = to_c_interp_method(method);
    hdrl_parameter* param = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_resample_interpolate_parameter_create, c_method);

    if (!param) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create interpolation parameter");
    }

    return Parameter(param);
}

Parameter Spectrum1D::create_fit_parameter(int k, int nCoeff) {
    hdrl_parameter* param = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_resample_fit_parameter_create, k, nCoeff);

    if (!param) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create fit parameter");
    }

    return Parameter(param);
}

Parameter Spectrum1D::create_windowed_fit_parameter(int k, int nCoeff, long window, double factor) {
    // Create a regular fit parameter first
    hdrl_parameter* param = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_resample_fit_parameter_create, k, nCoeff);

    if (!param) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create windowed fit parameter");
    }

    return Parameter(param);
}

Parameter Spectrum1D::create_integration_parameter() {
    hdrl_parameter* param = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_resample_integrate_parameter_create);

    if (!param) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create integration parameter");
    }

    return Parameter(param);
}

Spectrum1D Spectrum1D::resample(const Spectrum1D& other, InterpolationMethod method) const {
    if (!m_interface || !other.m_interface) {
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum in resample operation");
    }

    Parameter params = create_interpolation_parameter(method);

    hdrl_spectrum1D_wavelength spec_wav = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_wavelength, m_interface);
    hdrl_spectrum1D* resampled = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_resample, other.m_interface, &spec_wav, params.ptr());

    if (!resampled) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Resampling failed");
    }

    return Spectrum1D(resampled);
}

// Modified resample_to_wavelengths with enhanced safety
Spectrum1D Spectrum1D::resample_to_wavelengths(
    const std::vector<double>& wavelengths,
    InterpolationMethod method) const {

    if (!m_interface) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Null spectrum in resample operation");
    }

    if (wavelengths.empty()) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Empty wavelengths array");
    }

    // Convert method to C enum
    hdrl_spectrum1D_interpolation_method c_method = to_c_interp_method(method);

    // Check if we have enough points for the requested interpolation method
    size_t original_size = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_size, m_interface);
    if (!can_interpolate(original_size, c_method)) {
        // Fall back to a simpler method or return a copy of the original
        if (wavelengths.size() == 1) {
            return handle_single_point_resample(wavelengths[0]);
        } else {
            // Use linear interpolation as fallback
            return resample_to_wavelengths(wavelengths, InterpolationMethod::LINEAR);
        }
    }

    // Create wavelength array
    cpl_array* wav_array = hdrl::core::Error::throw_errors_with(cpl_array_new, wavelengths.size(), CPL_TYPE_DOUBLE);
    if (!wav_array) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create wavelength array");
    }

    for (size_t i = 0; i < wavelengths.size(); ++i) {
        hdrl::core::Error::throw_errors_with(cpl_array_set, wav_array, i, wavelengths[i]);
    }

    // Create interpolation parameter
    hdrl_parameter* param = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_resample_interpolate_parameter_create, c_method);
    if (!param) {
        hdrl::core::Error::throw_errors_with(cpl_array_delete, wav_array);
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create interpolation parameter");
    }

    // Prepare wavelength structure
    hdrl_spectrum1D_wavelength wl = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_wavelength, m_interface);
    wl.wavelength = wav_array;

    // Try the resampling operation
    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_resample, m_interface, &wl, param);

    if (!result) {
        // Try with linear interpolation as a fallback
        hdrl::core::Error::throw_errors_with(hdrl_parameter_delete, param);
        param = hdrl::core::Error::throw_errors_with(
            hdrl_spectrum1D_resample_interpolate_parameter_create,
            hdrl_spectrum1D_interp_linear);
        if (!param) {
            hdrl::core::Error::throw_errors_with(cpl_array_delete, wav_array);
            throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create fallback interpolation parameter");
        }

        result = hdrl::core::Error::throw_errors_with(
            hdrl_spectrum1D_resample, m_interface, &wl, param);
    }

    hdrl::core::Error::throw_errors_with(hdrl_parameter_delete, param);
    hdrl::core::Error::throw_errors_with(cpl_array_delete, wav_array);

    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Resampling failed");
    }

    return Spectrum1D(result);
}

Spectrum1D Spectrum1D::resample_fit(
    const std::vector<double>& wavelengths, int k, int nCoeff) const {

    if (!m_interface) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Null spectrum in resample operation");
    }

    if (wavelengths.empty()) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Empty wavelengths array");
    }

    size_t original_size = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_size, m_interface);
    if (original_size < 2) {
        // For small datasets, fall back to linear interpolation
        return resample_to_wavelengths(wavelengths, InterpolationMethod::LINEAR);
    }

    cpl_array* wav_array = hdrl::core::Error::throw_errors_with(cpl_array_new, wavelengths.size(), CPL_TYPE_DOUBLE);
    for (size_t i = 0; i < wavelengths.size(); ++i) {
        hdrl::core::Error::throw_errors_with(cpl_array_set, wav_array, i, wavelengths[i]);
    }

    hdrl_parameter* param = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_resample_fit_parameter_create, k, nCoeff);

    if (!param) {
        hdrl::core::Error::throw_errors_with(cpl_array_delete, wav_array);
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create fit parameter");
    }

    hdrl_spectrum1D_wavelength wl = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_wavelength, m_interface);
    wl.wavelength = wav_array;

    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_resample, m_interface, &wl, param);

    hdrl::core::Error::throw_errors_with(hdrl_parameter_delete, param);
    hdrl::core::Error::throw_errors_with(cpl_array_delete, wav_array);

    if (!result) {
        // Try with a simpler method if fit fails
        return resample_to_wavelengths(wavelengths, InterpolationMethod::LINEAR);
    }

    return Spectrum1D(result);
}

Spectrum1D Spectrum1D::resample_windowed_fit(
    const std::vector<double>& wavelengths, int k, int nCoeff, long window, double factor) const {

    if (wavelengths.empty()) {
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Empty wavelengths array");
    }

    cpl_array* wav_array = hdrl::core::Error::throw_errors_with(cpl_array_new, wavelengths.size(), CPL_TYPE_DOUBLE);
    for (size_t i = 0; i < wavelengths.size(); ++i) {
        hdrl::core::Error::throw_errors_with(cpl_array_set, wav_array, i, wavelengths[i]);
    }

    Parameter params = create_windowed_fit_parameter(k, nCoeff, window, factor);

    hdrl_spectrum1D* resampled = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_resample_on_array, m_interface, wav_array, params.ptr());

    hdrl::core::Error::throw_errors_with(cpl_array_delete, wav_array);

    if (!resampled) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Resampling failed");
    }

    return Spectrum1D(resampled);
}

Spectrum1D Spectrum1D::resample_integrate(const std::vector<double>& wavelengths) const {
    if (!m_interface) {
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum in resample operation");
    }

    if (wavelengths.empty()) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Empty wavelengths array");
    }

    cpl_array* wav_array = hdrl::core::Error::throw_errors_with(cpl_array_new, wavelengths.size(), CPL_TYPE_DOUBLE);
    for (size_t i = 0; i < wavelengths.size(); ++i) {
        hdrl::core::Error::throw_errors_with(cpl_array_set, wav_array, i, wavelengths[i]);
    }

    hdrl_parameter* param = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_resample_integrate_parameter_create);

    if (!param) {
        hdrl::core::Error::throw_errors_with(cpl_array_delete, wav_array);
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create integration parameter");
    }

    hdrl_spectrum1D_wavelength wl = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_wavelength, m_interface);
    wl.wavelength = wav_array;

    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_resample, m_interface, &wl, param);

    hdrl::core::Error::throw_errors_with(hdrl_parameter_delete, param);
    hdrl::core::Error::throw_errors_with(cpl_array_delete, wav_array);

    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Resampling failed");
    }

    return Spectrum1D(result);
}

// Helper method for single point resampling
Spectrum1D Spectrum1D::handle_single_point_resample(double target_wavelength) const {
    if (!m_interface) {
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum in single point resample");
    }

    // Find the closest wavelength in the original spectrum
    hdrl_spectrum1D_wavelength spec_wav = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_wavelength, m_interface);
    const cpl_array* orig_wav = spec_wav.wavelength;
    size_t orig_size = cpl_array_get_size(orig_wav);

    if (orig_size == 0) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Original spectrum has no data points");
    }

    // Find the index of the closest wavelength
    size_t closest_idx = 0;
    double min_diff = std::abs(hdrl::core::Error::throw_errors_with(cpl_array_get, orig_wav, 0, nullptr) - target_wavelength);

    for (size_t i = 1; i < orig_size; ++i) {
        double diff = std::abs(hdrl::core::Error::throw_errors_with(cpl_array_get, orig_wav, i, nullptr) - target_wavelength);
        if (diff < min_diff) {
            min_diff = diff;
            closest_idx = i;
        }
    }

    // Get the flux value at that index
    int rej;
    hdrl_value val = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_flux_value, m_interface, closest_idx, &rej);

    // Create a new spectrum with a single point
    double flux[] = {val.data};
    double flux_e[] = {val.error};
    double wav[] = {target_wavelength};

    return Spectrum1D::create(flux, flux_e, wav, 1, get_scale());
}

// --- I/O ---
void Spectrum1D::save(const std::string& filename) const {
    if (!m_interface)
        throw InvalidTypeError(HDRL_ERROR_LOCATION, "Null spectrum");

    cpl_table* table = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_convert_to_table,
        m_interface, "FLUX", "WAVELENGTH", "FLUX_ERROR", nullptr);

    if (!table) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to convert spectrum to table");
    }

    hdrl::core::Error::throw_errors_with(
        cpl_table_save, table, nullptr, nullptr, filename.c_str(), CPL_IO_CREATE);
    hdrl::core::Error::throw_errors_with(cpl_table_delete, table);
}

// --- Data Access ---
std::vector<double> Spectrum1D::get_flux_vector() const {
    if (!m_interface) return {};

    const hdrl_image* img = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_flux, m_interface);
    size_t size = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_size, m_interface);

    std::vector<double> data(size);
    for (size_t i = 0; i < size; ++i) {
        int rej;
        hdrl_value val = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_flux_value, m_interface, i, &rej);
        data[i] = val.data;
    }

    return data;
}

std::vector<double> Spectrum1D::get_wavelength_vector() const {
    if (!m_interface) return {};

    hdrl_spectrum1D_wavelength spec_wav = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_wavelength, m_interface);
    const cpl_array* wav_arr = spec_wav.wavelength;
    if (!wav_arr) return {};

    size_t sz = hdrl::core::Error::throw_errors_with(cpl_array_get_size, wav_arr);
    std::vector<double> data(sz);

    for (size_t i = 0; i < sz; ++i) {
        data[i] = hdrl::core::Error::throw_errors_with(cpl_array_get, wav_arr, i, nullptr);
    }

    return data;
}

std::vector<double> Spectrum1D::get_flux_error_vector() const {
    if (!m_interface) return {};

    size_t sz = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_size, m_interface);
    std::vector<double> errors(sz);

    for (size_t i = 0; i < sz; ++i) {
        int rej;
        hdrl_value val = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_flux_value, m_interface, i, &rej);
        errors[i] = val.error;
    }

    return errors;
}

// --- Utility Methods ---
bool Spectrum1D::is_compatible_with(const Spectrum1D& other) const {
    if (!m_interface || !other.m_interface) return false;

    hdrl_spectrum1D_wavelength w1 = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_wavelength, m_interface);
    hdrl_spectrum1D_wavelength w2 = hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_get_wavelength, other.m_interface);

    return hdrl::core::Error::throw_errors_with(hdrl_spectrum1D_are_spectra_compatible, &w1, &w2) == CPL_TRUE;
}

Spectrum1D Spectrum1D::duplicate() const {
    if (!m_interface) return Spectrum1D();

    hdrl_spectrum1D* result = hdrl::core::Error::throw_errors_with(
        hdrl_spectrum1D_duplicate, m_interface);
    if (!result) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to duplicate spectrum");
    }

    return Spectrum1D(result);
}

// --- Spectrum1DList Implementation ---
Spectrum1DList::Spectrum1DList() : m_list(nullptr) {}

Spectrum1DList::Spectrum1DList(hdrl_spectrum1Dlist* list) : m_list(list) {
    if (!m_list) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Null spectrum list provided to Spectrum1DList");
    }
}

Spectrum1DList::~Spectrum1DList() {
    if (m_list) {
        Error::throw_errors_with(hdrl_spectrum1Dlist_delete, m_list);
    }
}

Spectrum1DList::Spectrum1DList(Spectrum1DList&& other) noexcept
    : m_list(other.m_list) {
    other.m_list = nullptr;
}

Spectrum1DList& Spectrum1DList::operator=(Spectrum1DList&& other) noexcept {
    if (this != &other) {
        if (m_list) {
            Error::throw_errors_with(hdrl_spectrum1Dlist_delete, m_list);
        }
        m_list = other.m_list;
        other.m_list = nullptr;
    }
    return *this;
}

Spectrum1DList Spectrum1DList::create() {
    hdrl_spectrum1Dlist* list = Error::throw_errors_with(hdrl_spectrum1Dlist_new);
    if (!list) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to create spectrum list");
    }
    return Spectrum1DList(list);
}

Spectrum1DList Spectrum1DList::create_from_array(const std::vector<Spectrum1D>& spectra) {
    Spectrum1DList list = create();

    for (size_t i = 0; i < spectra.size(); ++i) {
        const Spectrum1D& spectrum = spectra[i];

        if (!spectrum.ptr()) {
            throw NullInputError(HDRL_ERROR_LOCATION, "Null spectrum in create_from_array()");
        }

        hdrl_spectrum1D* duplicated = Error::throw_errors_with(
            hdrl_spectrum1D_duplicate,
            const_cast<hdrl_spectrum1D*>(spectrum.ptr())
        );

        Error::throw_errors_with(
            hdrl_spectrum1Dlist_set,
            list.m_list,
            duplicated,
            i
        );
    }

    return list;
}

size_t Spectrum1DList::size() const {
    if (!m_list) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Null spectrum list in size()");
    }
    return Error::throw_errors_with(hdrl_spectrum1Dlist_get_size, m_list);
}

Spectrum1D Spectrum1DList::get(size_t index) const {
    if (!m_list) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Null spectrum list in get()");
    }

    const hdrl_spectrum1D* spectrum = Error::throw_errors_with(
        hdrl_spectrum1Dlist_get_const,
        m_list,
        index
    );

    if (!spectrum) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to get spectrum from list");
    }

    hdrl_spectrum1D* duplicated = Error::throw_errors_with(
        hdrl_spectrum1D_duplicate,
        spectrum
    );

    return Spectrum1D(duplicated);
}

void Spectrum1DList::set(size_t index, const Spectrum1D& spectrum) {
    if (!m_list) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Null spectrum list in set()");
    }

    if (!spectrum.ptr()) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Null spectrum in set()");
    }

    hdrl_spectrum1D* duplicated = Error::throw_errors_with(
        hdrl_spectrum1D_duplicate,
        const_cast<hdrl_spectrum1D*>(spectrum.ptr())
    );

    Error::throw_errors_with(
        hdrl_spectrum1Dlist_set,
        m_list,
        duplicated,
        index
    );
}

Spectrum1D Spectrum1DList::unset(size_t index) {
    if (!m_list) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Null spectrum list in unset()");
    }

    hdrl_spectrum1D* spectrum = Error::throw_errors_with(
        hdrl_spectrum1Dlist_unset,
        m_list,
        index
    );

    if (!spectrum) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to unset spectrum from list");
    }

    return Spectrum1D(spectrum);
}

Spectrum1DList Spectrum1DList::duplicate() const {
    if (!m_list) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Null spectrum list in duplicate()");
    }

    hdrl_spectrum1Dlist* duplicated = Error::throw_errors_with(
        hdrl_spectrum1Dlist_duplicate,
        m_list
    );

    if (!duplicated) {
        throw IllegalOutputError(HDRL_ERROR_LOCATION, "Failed to duplicate spectrum list");
    }

    return Spectrum1DList(duplicated);
}

#if defined HDRL_USE_EXPERIMENTAL || defined HDRL_USE_PRIVATE
CollapseResult Spectrum1DList::collapse(
    const Parameter& stacking_par,
    const std::vector<double>& wavelengths,
    const Parameter& resample_par,
    bool mark_bpm_in_interpolation) const {

    if (!m_list) {
        throw NullInputError(HDRL_ERROR_LOCATION, "Null spectrum list in collapse()");
    }

    cpl_array* wav_array = Error::throw_errors_with(
        cpl_array_new,
        wavelengths.size(), CPL_TYPE_DOUBLE
    );

    for (size_t i = 0; i < wavelengths.size(); ++i) {
        Error::throw_errors_with(
            cpl_array_set,
            wav_array, i, wavelengths[i]
        );
    }

    hdrl_spectrum1D* result = nullptr;
    cpl_image* contrib = nullptr;
    hdrl_imagelist* resampled_and_aligned_fluxes = nullptr;

    hdrl_parameter* stacking_par_ptr = const_cast<hdrl_parameter*>(
        const_cast<Parameter*>(&stacking_par)->ptr()
    );
    hdrl_parameter* resample_par_ptr = const_cast<hdrl_parameter*>(
        const_cast<Parameter*>(&resample_par)->ptr()
    );

    Error::throw_errors_with(
        hdrl_spectrum1Dlist_collapse,
        m_list,
        stacking_par_ptr,
        wav_array,
        resample_par_ptr,
        mark_bpm_in_interpolation ? CPL_TRUE : CPL_FALSE,
        &result,
        &contrib,
        &resampled_and_aligned_fluxes
    );

    Spectrum1D result_spectrum(result);
    ImageList aligned_images;

    if (resampled_and_aligned_fluxes) {
        aligned_images = ImageList(resampled_and_aligned_fluxes);
    } else {
        aligned_images = ImageList();
    }

    Error::throw_errors_with(cpl_array_delete, wav_array);

    CollapseResult collapse_result;
    collapse_result.result = std::move(result_spectrum);
    collapse_result.aligned_images = std::move(aligned_images);
    return collapse_result;
}
#endif

}  // namespace core
}  // namespace hdrl

