import math
import tempfile
from pathlib import Path

import cpl.core as cplcore
import hdrl.core as hdrlcore
import hdrl.func as hdrlfunc
import numpy as np
import pytest
from hdrl.core import (
    InterpolationMethod,
    Spectrum1D,
    Spectrum1DList,
    WaveScale,
    XCorrelationResult,
)

EPSILON = 1e-6
RELATIVE_TOLERANCE = 1e-2


def _calc_gauss(mean, sigma, x):
    exponent = -((x - mean) ** 2.0) / (2.0 * sigma * sigma)
    return 1.0 / (2.0 * np.pi * sigma * sigma) * np.exp(exponent)


def _create_gaussian_times_absorption_spectrum(abs_mean):
    wavelengths = np.arange(1000.0, 2000.0, 1.0, dtype=float)
    # Keep same shape/scale as C tests in hdrl_spectrum1d_shift-test.c.
    gaussian = _calc_gauss(1500.0, 250.0, wavelengths)
    absorption = np.exp(-_calc_gauss(abs_mean, 0.75, wavelengths))
    flux = gaussian * absorption
    flux_error = np.zeros_like(flux)
    return Spectrum1D(flux, flux_error, wavelengths, "linear")


def create_strictly_increasing_wavelengths(size, start=1.0, step=1.0):
    return start + np.arange(size, dtype=float) * step


def create_test_spectrum(size=40):
    rng = np.random.default_rng(42)
    flux = rng.uniform(1.0, 128.0, size)
    flux_error = rng.uniform(0.5, 2.0, size)
    wavelengths = create_strictly_increasing_wavelengths(size, start=500.0, step=0.5)
    return Spectrum1D(flux, flux_error, wavelengths, "linear")


def create_linear_spectrum(size=50):
    wavelengths = np.linspace(1, 10, size)
    flux = np.linspace(1, 10, size)
    flux_error = np.ones(size) * 0.1
    return Spectrum1D(flux, flux_error, wavelengths, "linear")


def _create_reference_spectrum_for_resample_shift(is_error_free):
    flux = np.array([0, 1, 2, 1, 0, -1, -2, -1, 0, 1, 2, 1, 0, -1], dtype=float)
    flux_error = (
        np.zeros_like(flux)
        if is_error_free
        else np.array([0.1, 0.2, 0.3, 0.2, 0.1, 0.2, 0.3, 0.2, 0.1, 0.2, 0.3, 0.2, 0.1, 0.2], dtype=float)
    )
    wavelengths = np.array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14], dtype=float)
    return Spectrum1D(flux, flux_error, wavelengths, "linear")


def _create_reference_spectrum_for_resample_error_interpolation():
    flux = np.array([0, 1, 2, 1, 0, -1, -2, -1, 0, 1, 2, 1, 0, -1], dtype=float)
    flux_error = np.array([0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4], dtype=float)
    wavelengths = np.array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14], dtype=float)
    return Spectrum1D(flux, flux_error, wavelengths, "linear")


def _windowed_fit_reference_function(t):
    x = np.sin(10.0 * t)
    return np.exp(x * x * x)


def _create_stair_spectrum(start, stop, start_wave, step_wave):
    size = stop - start + 1
    wavelengths = start_wave + np.arange(size, dtype=float) * step_wave
    flux = start + np.arange(size, dtype=float)
    flux_error = flux / 10.0
    return Spectrum1D(flux, flux_error, wavelengths, "linear")


def _create_bad_stair_spectrum(start, stop, start_wave, step_wave, bad_indices):
    spec = _create_stair_spectrum(start, stop, start_wave, step_wave)
    flags = np.zeros(spec.size, dtype=np.int32)
    flags[np.asarray(bad_indices, dtype=np.int32)] = 1
    return spec.reject_pixels(flags)


def _get_waves(start, count, step):
    return start + np.arange(count, dtype=float) * step


def _create_spectrum_long(values):
    values = np.asarray(values, dtype=float)
    wavelengths = np.abs(values)
    flux = np.abs(values)
    flux_error = np.zeros_like(values)
    spec = Spectrum1D(flux, flux_error, wavelengths, "linear")
    bad_flags = (values < 0).astype(np.int32)
    if np.any(bad_flags):
        spec = spec.reject_pixels(bad_flags)
    return spec


def _calc_total_flux_from_bins(spec):
    w = spec.wavelengths
    f = spec.flux
    if len(w) == 1:
        return float(f[0])
    widths = np.empty_like(w)
    widths[0] = (w[1] - w[0]) / 2.0
    widths[-1] = (w[-1] - w[-2]) / 2.0
    if len(w) > 2:
        widths[1:-1] = (w[2:] - w[:-2]) / 2.0
    return float(np.sum(f * widths))


def _between_bin_error(a, b):
    return np.sqrt(a * a + b * b) / (10.0 * np.sqrt(2.0))


def _create_single_bin_spectrum(flux_value):
    flux = np.array([float(flux_value)], dtype=float)
    flux_error = np.zeros(1, dtype=float)
    wavelengths = np.array([100.0], dtype=float)
    return Spectrum1D(flux, flux_error, wavelengths, "linear")


def _create_spectrum1d_sin_fixture_like_c_test(sz=17, start=2, add_peak=False):
    peak = 100.0
    delta = 2.0 * np.pi / sz
    wavelengths = delta * (np.arange(sz, dtype=float) + start)
    flux = np.abs(peak * (np.sin(wavelengths) + 1.1))
    if add_peak:
        flux[4] *= 1.5
    # C fixture uses create_error_DER_SNR(..., 10, ...).
    return Spectrum1D(flux, 10, wavelengths, "linear"), wavelengths


class TestSpectrum1D:
    def test_constructor(self):
        size = 40
        rng = np.random.default_rng(42)
        flux = rng.uniform(0.0, 128.0, size)
        flux_error = rng.uniform(0.0, 1.0, size)
        wavelengths = create_strictly_increasing_wavelengths(size, start=400.0, step=0.2)

        spec = Spectrum1D(flux, flux_error, wavelengths, "linear")
        assert spec.size == size
        assert spec.scale == WaveScale.LINEAR
        assert np.allclose(spec.flux, flux)
        assert np.allclose(spec.flux_error, flux_error)
        assert np.allclose(spec.wavelengths, wavelengths)

        spec = Spectrum1D(flux, None, wavelengths, "linear")
        assert spec.size == size
        assert spec.scale == WaveScale.LINEAR
        assert np.allclose(spec.flux, flux)
        assert np.allclose(spec.flux_error, np.zeros(size))
        assert np.allclose(spec.wavelengths, wavelengths)

        spec = Spectrum1D(flux, 10, wavelengths, "linear")
        assert spec.size == size
        assert spec.scale == WaveScale.LINEAR
        assert np.allclose(spec.flux, flux)
        assert np.allclose(spec.wavelengths, wavelengths)

    def test_constructor_failures(self):
        size1 = 40
        size2 = 42
        rng = np.random.default_rng(42)
        flux1 = rng.uniform(0.0, 128.0, size1)
        flux_error1 = rng.uniform(0.0, 1.0, size1)
        wavelengths1 = create_strictly_increasing_wavelengths(size1, start=400.0, step=0.2)

        flux_error2 = rng.uniform(0.0, 1.0, size2)

        with pytest.raises(ValueError):
            Spectrum1D(flux1, flux_error2, wavelengths1, "linear")

        with pytest.raises(ValueError):
            Spectrum1D(np.array([]), np.array([]), np.array([]), "linear")

        bad_wavelengths = wavelengths1.copy()
        bad_wavelengths[10] = bad_wavelengths[9]
        with pytest.raises(ValueError):
            Spectrum1D(flux1, flux_error1, bad_wavelengths, "linear")

        with pytest.raises(ValueError):
            Spectrum1D(flux1, flux_error1, wavelengths1, "invalid")

        flux_nan = flux1.copy()
        flux_nan[3] = math.nan
        with pytest.raises(ValueError):
            Spectrum1D(flux_nan, flux_error1, wavelengths1, "linear")

    def test_constructor_float_double_parity_like_c_test(self):
        """Parity with C constructor test for float and double input arrays."""
        size = 32
        wavelengths = create_strictly_increasing_wavelengths(size, start=600.0, step=0.25)
        flux64 = np.linspace(1.0, 2.0, size, dtype=np.float64)
        err64 = np.linspace(0.1, 0.2, size, dtype=np.float64)
        spec64 = Spectrum1D(flux64, err64, wavelengths, "linear")
        assert spec64.size == size
        assert np.allclose(spec64.flux, flux64, atol=EPSILON)

        flux32 = np.linspace(1.0, 2.0, size, dtype=np.float32)
        err32 = np.linspace(0.1, 0.2, size, dtype=np.float32)
        spec32 = Spectrum1D(flux32, err32, wavelengths.astype(np.float32), "linear")
        assert spec32.size == size
        assert np.allclose(spec32.flux, flux32.astype(np.float64), atol=EPSILON)

    def test_duplication(self):
        spec = create_test_spectrum()
        spec_copy = spec.duplicate()

        assert spec.size == spec_copy.size
        assert np.allclose(spec.flux, spec_copy.flux)
        assert np.allclose(spec.flux_error, spec_copy.flux_error)
        assert np.allclose(spec.wavelengths, spec_copy.wavelengths)

    def test_scalar_operations(self):
        spec = create_test_spectrum()
        original_flux = spec.flux.copy()
        original_error = spec.flux_error.copy()
        scalar = 1.5

        # FIXME: Disable tests for arithmetic operations accepting a scalar
        # and returning a new Spectrum1D instance. They are not offered in v1.0
        # to keep the overall Python API design consistent. They may become
        # available in the future, or are eventually removed.

        # spec_copy = spec.mul_scalar_create(scalar)
        spec.mul_scalar(scalar)
        assert np.allclose(spec.flux, original_flux * scalar, atol=EPSILON)
        assert np.allclose(spec.flux_error, original_error * scalar, atol=EPSILON)
        # assert np.allclose(spec_copy.flux, original_flux * scalar, atol=EPSILON)
        # assert np.allclose(spec_copy.flux_error, original_error * scalar, atol=EPSILON)

        spec = create_test_spectrum()
        original_flux = spec.flux.copy()
        original_error = spec.flux_error.copy()
        scalar = 1.5

        # spec_copy = spec.div_scalar_create(scalar)
        spec.div_scalar(scalar)
        assert np.allclose(spec.flux, original_flux / scalar, atol=EPSILON)
        assert np.allclose(spec.flux_error, original_error / scalar, atol=EPSILON)
        # assert np.allclose(spec_copy.flux, original_flux / scalar, atol=EPSILON)
        # assert np.allclose(spec_copy.flux_error, original_error / scalar, atol=EPSILON)

        with pytest.raises(ValueError):
            spec.div_scalar(0.0)

        spec = create_test_spectrum()
        original_flux = spec.flux.copy()
        original_error = spec.flux_error.copy()
        scalar = 2.0

        # spec_copy = spec.add_scalar_create(scalar)
        spec.add_scalar(scalar)
        assert np.allclose(spec.flux, original_flux + scalar, atol=EPSILON)
        assert np.allclose(spec.flux_error, original_error, atol=EPSILON)
        # assert np.allclose(spec_copy.flux, original_flux + scalar, atol=EPSILON)
        # assert np.allclose(spec_copy.flux_error, original_error, atol=EPSILON)

        spec = create_test_spectrum()
        original_flux = spec.flux.copy()
        original_error = spec.flux_error.copy()
        scalar = 2.0

        # spec_copy = spec.sub_scalar_create(scalar)
        spec.sub_scalar(scalar)
        assert np.allclose(spec.flux, original_flux - scalar, atol=EPSILON)
        assert np.allclose(spec.flux_error, original_error, atol=EPSILON)
        # assert np.allclose(spec_copy.flux, original_flux - scalar, atol=EPSILON)
        # assert np.allclose(spec_copy.flux_error, original_error, atol=EPSILON)

        spec = create_test_spectrum()
        original_flux = spec.flux.copy()
        scalar = 1.5

        # spec_copy = spec.pow_scalar_create(scalar)
        spec.pow_scalar(scalar)
        assert np.allclose(spec.flux, original_flux**scalar, rtol=RELATIVE_TOLERANCE)
        # assert np.allclose(spec_copy.flux, original_flux**scalar, rtol=RELATIVE_TOLERANCE)

        spec = create_test_spectrum()
        original_flux = spec.flux.copy()
        scalar = 0.5

        # spec_copy = spec.exp_scalar_create(scalar)
        spec.exp_scalar(scalar)
        assert spec.flux.shape == original_flux.shape
        # assert spec_copy.flux.shape == original_flux.shape
        assert np.all(np.isfinite(spec.flux))
        # assert np.all(np.isfinite(spec_copy.flux))

    def test_wavelength_conversion(self):
        spec = create_test_spectrum()
        original_wavelengths = spec.wavelengths.copy()

        spec_log = spec.wavelength_convert_to_log_create()
        assert spec_log.scale == WaveScale.LOG

        spec_lin = spec_log.wavelength_convert_to_linear_create()
        assert spec_lin.scale == WaveScale.LINEAR
        assert np.allclose(spec_lin.wavelengths, original_wavelengths, atol=EPSILON)

    def test_wavelength_conversion_mutator_noop_like_c_test(self):
        """Parity with C mutator path including log->log and linear->linear NOOP."""
        spec = create_test_spectrum()
        w0 = spec.wavelengths.copy()

        spec.wavelength_convert_to_log()
        w_log = spec.wavelengths.copy()
        assert spec.scale == WaveScale.LOG
        assert np.allclose(w_log, np.log(w0), atol=1e-6)

        # NOOP in same scale should keep values unchanged.
        spec.wavelength_convert_to_log()
        assert spec.scale == WaveScale.LOG
        assert np.allclose(spec.wavelengths, w_log, atol=EPSILON)

        spec.wavelength_convert_to_linear()
        w_lin = spec.wavelengths.copy()
        assert spec.scale == WaveScale.LINEAR
        assert np.allclose(w_lin, w0, atol=1e-6)

        # NOOP in same scale should keep values unchanged.
        spec.wavelength_convert_to_linear()
        assert spec.scale == WaveScale.LINEAR
        assert np.allclose(spec.wavelengths, w_lin, atol=EPSILON)

    def test_wavelength_conversion_noop_create_like_c_test(self):
        """Parity with C behavior: converting same scale acts like duplicate."""
        spec = create_test_spectrum()
        spec_log = spec.wavelength_convert_to_log_create()

        # log->log create should return independent object with same content.
        spec_log2 = spec_log.wavelength_convert_to_log_create()
        assert spec_log2 is not spec_log
        assert spec_log2.scale == WaveScale.LOG
        assert np.allclose(spec_log2.wavelengths, spec_log.wavelengths, atol=EPSILON)

        # linear->linear create should return independent object with same content.
        spec_lin2 = spec.wavelength_convert_to_linear_create()
        assert spec_lin2 is not spec
        assert spec_lin2.scale == WaveScale.LINEAR
        assert np.allclose(spec_lin2.wavelengths, spec.wavelengths, atol=EPSILON)

    def test_wavelength_shift(self):
        spec = create_test_spectrum()
        original_wavelengths = spec.wavelengths.copy()
        shift = 3.0

        spec.wavelength_shift(shift)
        assert np.allclose(spec.wavelengths, original_wavelengths + shift, atol=EPSILON)

        spec = create_test_spectrum()
        spec_new = spec.wavelength_shift_create(shift)
        assert np.allclose(spec_new.wavelengths, original_wavelengths + shift, atol=EPSILON)

    def test_wavelength_shift_linear_and_log_like_c_test(self):
        """Parity with C test_spectrum1D_shift_wavelength for linear and log scales."""
        spec_linear = create_test_spectrum()
        wl_linear = spec_linear.wavelengths.copy()
        spec_log = spec_linear.wavelength_convert_to_log_create()
        wl_log = spec_log.wavelengths.copy()

        spec_linear.wavelength_shift(3.0)
        spec_log.wavelength_shift(-3.0)

        assert np.allclose(spec_linear.wavelengths, wl_linear + 3.0, atol=EPSILON)
        assert np.allclose(spec_log.wavelengths, wl_log - 3.0, atol=EPSILON)

    def test_wavelength_multiplication(self):
        spec = create_test_spectrum()
        original_wavelengths = spec.wavelengths.copy()
        scale = 2.0

        spec.wavelength_mult_scalar_linear(scale)
        assert np.allclose(spec.wavelengths, original_wavelengths * scale, atol=EPSILON)

        spec = create_test_spectrum()
        spec_new = spec.wavelength_mult_scalar_linear_create(scale)
        assert np.allclose(spec_new.wavelengths, original_wavelengths * scale, atol=EPSILON)

        with pytest.raises(ValueError):
            spec.wavelength_mult_scalar_linear(0.0)

    def test_wavelength_multiplication_in_log_scale_like_c_test(self):
        """Parity with C test_spectrum1D_mul_wavelength in log scale."""
        spec = create_test_spectrum()
        base_wavelengths = spec.wavelengths.copy()
        spec_log = spec.wavelength_convert_to_log_create()
        spec_log.wavelength_mult_scalar_linear(1e3)

        assert np.allclose(spec_log.wavelengths, np.log(base_wavelengths * 1e3), atol=1e-6)

    def test_wavelength_multiplication_create_linear_and_log_like_c_test(self):
        """Parity with C create-path checks in linear and log scales."""
        spec = create_test_spectrum()
        base_wavelengths = spec.wavelengths.copy()

        sp_lin = spec.wavelength_mult_scalar_linear_create(1e-4)
        assert np.allclose(sp_lin.wavelengths, base_wavelengths * 1e-4, atol=1e-6)

        spec_log = spec.wavelength_convert_to_log_create()
        sp_log = spec_log.wavelength_mult_scalar_linear_create(1e-4)
        assert np.allclose(sp_log.wavelengths, np.log(base_wavelengths * 1e-4), atol=1e-6)

        with pytest.raises((ValueError, hdrlcore.IllegalInputError)):
            spec_log.wavelength_mult_scalar_linear(-2.0)
        with pytest.raises((ValueError, hdrlcore.IllegalInputError)):
            spec_log.wavelength_mult_scalar_linear_create(-2.0)

    def test_wavelength_selection(self):
        wavelengths = np.linspace(1, 20, 20)
        rng = np.random.default_rng(42)
        flux = rng.uniform(1, 100, 20)
        flux_error = rng.uniform(0.1, 1, 20)
        spec = Spectrum1D(flux, flux_error, wavelengths, "linear")

        sp2 = spec.select_window(5, 15, True)
        assert sp2.size > 0
        selected_wavelengths = sp2.wavelengths
        assert np.all(selected_wavelengths >= 5)
        assert np.all(selected_wavelengths <= 15)

        with pytest.raises(ValueError):
            spec.select_window(10, 5, True)

    def test_wavelength_selection_external_like_c_test(self):
        """Parity with C select_window(..., is_internal=False) behavior."""
        wavelengths = np.arange(1, 15, dtype=float)
        flux = np.array([0, 1, 2, 1, 0, -1, -2, -1, 0, 1, 2, 1, 0, -1], dtype=float)
        flux_error = np.array([0.1, 0.2, 0.3, 0.2, 0.1, 0.2, 0.3, 0.2, 0.1, 0.2, 0.3, 0.2, 0.1, 0.2], dtype=float)
        spec = Spectrum1D(flux, flux_error, wavelengths, "linear")

        external = spec.select_window(3, 10, False)
        expected_idx = [0, 1, 10, 11, 12, 13]
        assert external.size == len(expected_idx)
        assert np.allclose(external.wavelengths, wavelengths[expected_idx], atol=EPSILON)
        assert np.allclose(external.flux, flux[expected_idx], atol=EPSILON)
        assert np.allclose(external.flux_error, flux_error[expected_idx], atol=EPSILON)

    def test_resample(self):
        spec = create_linear_spectrum(50)
        new_wavelengths = np.linspace(1, 10, 100)

        for method in [InterpolationMethod.LINEAR, InterpolationMethod.AKIMA, InterpolationMethod.CSPLINE]:
            resampled = spec.resample_to_wavelengths(new_wavelengths, method)
            assert resampled.size == len(new_wavelengths)
            assert np.allclose(resampled.wavelengths, new_wavelengths)
            # For a linear spectrum, all interpolation methods should preserve y=x.
            assert np.allclose(resampled.flux, new_wavelengths, atol=1e-4)

        other = create_linear_spectrum(30)
        resampled_other = spec.resample(other, InterpolationMethod.LINEAR)
        assert resampled_other.size == spec.size
        # Current binding returns a spectrum on self's wavelength grid.
        assert np.allclose(resampled_other.wavelengths, spec.wavelengths, atol=EPSILON)
        assert np.allclose(resampled_other.flux, spec.wavelengths, atol=1e-4)

    @pytest.mark.parametrize(
        ("add_peak", "fit_expected", "interp_expected", "integrate_expected"),
        [
            (True, (116.368, 303.376), (208.699, 247.949), (207.878, 245.443)),
            (False, (209.577, 199.524), (209.65, 199.585), (207.878, 197.992)),
        ],
    )
    def test_resample_spectrum_exact_values_like_c_test(
        self, add_peak, fit_expected, interp_expected, integrate_expected
    ):
        """Parity with C test_spectrum1D_resample_spectrum(add_peak)."""
        spec, unshuffled_lambda = _create_spectrum1d_sin_fixture_like_c_test(sz=17, start=2, add_peak=add_peak)
        sz = spec.size
        new_lambda = (unshuffled_lambda + unshuffled_lambda[np.minimum(np.arange(sz, dtype=int) + 1, sz - 1)]) / 2.0

        sp_fit = spec.resample_fit(new_lambda, 4, 17)
        assert sp_fit.flux[2] == pytest.approx(fit_expected[0], abs=1e-3)
        assert sp_fit.flux[3] == pytest.approx(fit_expected[1], abs=1e-3)

        sp_interp = spec.resample_to_wavelengths(new_lambda, InterpolationMethod.AKIMA)
        assert sp_interp.flux[2] == pytest.approx(interp_expected[0], abs=1e-3)
        assert sp_interp.flux[3] == pytest.approx(interp_expected[1], abs=1e-3)

        sp_integrate = spec.resample_integrate(new_lambda)
        assert sp_integrate.flux[2] == pytest.approx(integrate_expected[0], abs=1e-3)
        assert sp_integrate.flux[3] == pytest.approx(integrate_expected[1], abs=1e-3)

    def test_resample_fit(self):
        spec = create_linear_spectrum(50)
        new_wavelengths = np.linspace(1, 10, 100)

        resampled = spec.resample_fit(new_wavelengths, 2, 5)
        assert resampled.size == len(new_wavelengths)

    def test_resample_windowed_fit(self):
        spec = create_linear_spectrum(100)
        new_wavelengths = np.linspace(1, 10, 50)

        resampled = spec.resample_windowed_fit(new_wavelengths, 2, 3, 20, 1.0)
        assert resampled.size == len(new_wavelengths)

    def test_resample_windowed_fit_accuracy_like_c_test(self):
        """Reduced-size parity with C test_spectrum1D_resample_spectrum_fit_windowed."""
        length = 4000
        nblocks = 40
        window = length // nblocks

        lambdas = np.linspace(0.0, 1.0, length, dtype=float)
        flux = _windowed_fit_reference_function(lambdas)

        # Shifted target sampling grid (like C test's half-step resampling).
        lambdas_resampled = np.linspace(0.5 / (length - 1), 1.0 - 0.5 / (length - 1), length - 2, dtype=float)
        flux_ideal = _windowed_fit_reference_function(lambdas_resampled)

        source = Spectrum1D(flux, np.zeros_like(flux), lambdas, "linear")
        win = source.resample_windowed_fit(lambdas_resampled, 4, 6, window, 1.2)
        no_win = source.resample_fit(lambdas_resampled, 4, 28)

        # Match C intent: both methods stay close to ideal reference signal.
        assert np.allclose(win.flux, flux_ideal, rtol=1.5e-1, atol=1e-6)
        assert np.allclose(no_win.flux, flux_ideal, rtol=1.5e-1, atol=1e-6)

    def test_resample_integrate(self):
        # Constant flux lets us assert integration behavior robustly.
        wavelengths = np.linspace(1.0, 10.0, 50)
        flux = np.full(50, 3.0, dtype=float)
        flux_error = np.full(50, 0.1, dtype=float)
        spec = Spectrum1D(flux, flux_error, wavelengths, "linear")
        new_wavelengths = np.linspace(1, 10, 25)

        resampled = spec.resample_integrate(new_wavelengths)
        assert resampled.size == len(new_wavelengths)
        assert np.allclose(resampled.wavelengths, new_wavelengths, atol=EPSILON)
        # Integrating/resampling a constant signal should keep it constant.
        assert np.allclose(resampled.flux, 3.0, atol=1e-2)

    def test_resample_integrate_same_grid_identity_like_c_test(self):
        """Parity with C integrate test17: same wavelength grid keeps values."""
        source = _create_stair_spectrum(1, 8, 20.5, 1.0)
        resampled = source.resample_integrate(source.wavelengths.copy())
        assert np.allclose(resampled.wavelengths, source.wavelengths, atol=EPSILON)
        assert np.allclose(resampled.flux, source.flux, atol=EPSILON)
        assert np.allclose(resampled.flux_error, source.flux_error, atol=EPSILON)

    @pytest.mark.parametrize(
        "target_wavelengths",
        [
            np.array([22.5, 24.5, 26.5, 28.5, 30.5], dtype=float),  # C test15-style
            np.array([14.5, 16.5, 18.5, 20.5], dtype=float),  # C test16-style
        ],
    )
    def test_resample_integrate_empty_intersection_like_c_test(self, target_wavelengths):
        """Parity with C integrate test15/test16: non-overlapping bins are rejected."""
        source = _create_stair_spectrum(1, 3, 20.5, 1.0)
        resampled = source.resample_integrate(target_wavelengths)
        assert np.all(np.isnan(resampled.flux))
        assert np.all(np.isnan(resampled.flux_error))

    def test_resample_integrate_flux_conservation_like_c_test(self):
        """Parity with C integrate test18: integrated flux is conserved."""
        source = _create_stair_spectrum(1, 4, 20.0, 2.0)
        target_wavelengths = np.array([20.0, 23.0, 26.0], dtype=float)
        resampled = source.resample_integrate(target_wavelengths)

        source_total_flux = _calc_total_flux_from_bins(source)
        resampled_total_flux = _calc_total_flux_from_bins(resampled)
        assert source_total_flux == pytest.approx(15.0, abs=1e-6)
        assert resampled_total_flux == pytest.approx(source_total_flux, rel=1e-6, abs=1e-6)

    def test_resample_integrate_upsample_offset_grid_like_c_test1(self):
        """Parity with C integrate test1 (offset upsampling grid)."""
        source = _create_stair_spectrum(1, 8, 20.0, 2.0)
        target_wavelengths = np.arange(21.0, 30.0, 1.0)
        resampled = source.resample_integrate(target_wavelengths)

        expected_flux = np.array([2.0, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 5.0], dtype=float)
        expected_err = np.array(
            [
                0.2,
                0.2,
                _between_bin_error(2.0, 3.0),
                0.3,
                _between_bin_error(3.0, 4.0),
                0.4,
                _between_bin_error(4.0, 5.0),
                0.5,
                0.5,
            ],
            dtype=float,
        )
        assert np.allclose(resampled.wavelengths, target_wavelengths, atol=EPSILON)
        assert np.allclose(resampled.flux, expected_flux, atol=1e-6)
        assert np.allclose(resampled.flux_error, expected_err, atol=1e-6)

    def test_resample_integrate_upsample_aligned_grid_like_c_test2(self):
        """Parity with C integrate test2 (aligned start/stop grid)."""
        source = _create_stair_spectrum(1, 8, 20.0, 2.0)
        target_wavelengths = np.arange(20.0, 35.0, 1.0)
        resampled = source.resample_integrate(target_wavelengths)

        expected_flux = []
        expected_err = []
        src = 1.0
        for i in range(len(target_wavelengths)):
            if i % 2 == 0:
                expected_flux.append(src)
                expected_err.append(src / 10.0)
            else:
                expected_flux.append(src + 0.5)
                expected_err.append(_between_bin_error(src, src + 1.0))
                src += 1.0

        assert np.allclose(resampled.wavelengths, target_wavelengths, atol=EPSILON)
        assert np.allclose(resampled.flux, np.array(expected_flux), atol=1e-6)
        assert np.allclose(resampled.flux_error, np.array(expected_err), atol=1e-6)

    def test_resample_integrate_upsample_wider_range_like_c_test3(self):
        """Parity with C integrate test3 (wider target range with rejected edges)."""
        source = _create_stair_spectrum(1, 8, 20.0, 2.0)
        target_wavelengths = np.arange(19.0, 36.0, 1.0)
        resampled = source.resample_integrate(target_wavelengths)

        # First/last two bins are rejected in C test3.
        assert np.all(np.isnan(resampled.flux[:2]))
        assert np.all(np.isnan(resampled.flux_error[:2]))
        assert np.all(np.isnan(resampled.flux[-2:]))
        assert np.all(np.isnan(resampled.flux_error[-2:]))

        src = 1.0
        for i in range(2, len(target_wavelengths) - 2):
            if i % 2 == 1:
                assert resampled.flux[i] == pytest.approx(src, abs=1e-6)
                assert resampled.flux_error[i] == pytest.approx(src / 10.0, abs=1e-6)
            else:
                assert resampled.flux[i] == pytest.approx(src + 0.5, abs=1e-6)
                assert resampled.flux_error[i] == pytest.approx(_between_bin_error(src, src + 1.0), abs=1e-6)
                src += 1.0

    def test_resample_integrate_downsample_inside_range_like_c_test8(self):
        """Parity with C integrate test8 (destination bins larger than source)."""
        source = _create_stair_spectrum(1, 8, 20.0, 1.0)
        target_wavelengths = np.arange(21.0, 27.0, 2.0)
        resampled = source.resample_integrate(target_wavelengths)

        expected_flux = np.array([2.5, 4.0, 5.5], dtype=float)
        expected_err = np.array([np.sqrt(6.5), np.sqrt(16.5), np.sqrt(30.5)], dtype=float) / 10.0

        assert np.allclose(resampled.wavelengths, target_wavelengths, atol=EPSILON)
        assert np.allclose(resampled.flux, expected_flux, atol=1e-6)
        assert np.allclose(resampled.flux_error, expected_err, atol=1e-6)

    def test_resample_integrate_downsample_left_edge_reject_like_c_test9(self):
        """Parity with C integrate test9 (left edge outside, then valid bins)."""
        source = _create_stair_spectrum(1, 8, 20.0, 1.0)
        target_wavelengths = np.arange(19.0, 29.0, 2.0)
        resampled = source.resample_integrate(target_wavelengths)

        assert np.isnan(resampled.flux[0])
        assert np.isnan(resampled.flux_error[0])

        expected_flux = np.array([2.0, 4.0, 6.0, 7.5], dtype=float)
        assert np.allclose(resampled.flux[1:5], expected_flux, atol=1e-6)

    def test_resample_integrate_downsample_wider_range_like_c_test10(self):
        """Parity with C integrate test10 (both edge regions rejected)."""
        source = _create_stair_spectrum(1, 8, 20.0, 1.0)
        target_wavelengths = np.arange(19.0, 31.0, 2.0)
        resampled = source.resample_integrate(target_wavelengths)

        assert np.isnan(resampled.flux[0])
        assert np.isnan(resampled.flux_error[0])
        assert np.all(np.isnan(resampled.flux[4:6]))
        assert np.all(np.isnan(resampled.flux_error[4:6]))

        expected_flux = np.array([2.0, 4.0, 6.0], dtype=float)
        assert np.allclose(resampled.flux[1:4], expected_flux, atol=1e-6)

    def test_resample_integrate_bad_pixels_like_c_test5(self):
        source = _create_bad_stair_spectrum(1, 8, 20.0, 2.0, [0, 7, 2, 5])
        target_wavelengths = _get_waves(19.0, 17, 1.0)
        resampled = source.resample_integrate(target_wavelengths)

        assert np.all(np.isnan(resampled.flux[:3]))
        assert np.all(np.isnan(resampled.flux[-3:]))

        src = 2.0
        for i in range(3, len(target_wavelengths) - 3):
            if (4 <= i <= 6) or (10 <= i <= 12):
                assert np.isnan(resampled.flux[i])
                assert np.isnan(resampled.flux_error[i])
                if (i % 2) == 0:
                    src += 1.0
                continue
            if (i % 2) == 1:
                assert resampled.flux[i] == pytest.approx(src, abs=1e-6)
                assert resampled.flux_error[i] == pytest.approx(src / 10.0, abs=1e-6)
            else:
                assert resampled.flux[i] == pytest.approx(src + 0.5, abs=1e-6)
                assert resampled.flux_error[i] == pytest.approx(_between_bin_error(src, src + 1.0), abs=1e-6)
                src += 1.0

    def test_resample_integrate_bad_pixels_like_c_test7(self):
        source = _create_bad_stair_spectrum(1, 8, 20.0, 2.0, [0, 7, 4])
        target_wavelengths = _get_waves(20.5, 15, 1.0)
        resampled = source.resample_integrate(target_wavelengths)

        src = 1.0
        for i in range(len(target_wavelengths)):
            if i in (0, 7, 8) or i >= 13:
                assert np.isnan(resampled.flux[i])
                assert np.isnan(resampled.flux_error[i])
            else:
                assert resampled.flux[i] == pytest.approx(src, abs=1e-6)
                assert resampled.flux_error[i] == pytest.approx(src / 10.0, abs=1e-6)
            src += (i + 1) % 2

    def test_resample_integrate_bad_pixels_like_c_test11(self):
        source = _create_bad_stair_spectrum(1, 8, 20.0, 1.0, [7])
        target_wavelengths = _get_waves(19.0, 6, 2.0)
        resampled = source.resample_integrate(target_wavelengths)

        assert np.isnan(resampled.flux[0])
        assert np.isnan(resampled.flux_error[0])
        assert np.allclose(resampled.flux[1:4], np.array([2.0, 4.0, 6.0]), atol=1e-6)
        assert np.all(np.isnan(resampled.flux[4:6]))
        assert np.all(np.isnan(resampled.flux_error[4:6]))

    def test_resample_integrate_bad_pixels_like_c_test12(self):
        source = _create_bad_stair_spectrum(1, 8, 20.0, 1.0, [4])
        target_wavelengths = _get_waves(19.0, 6, 2.0)
        resampled = source.resample_integrate(target_wavelengths)

        assert np.isnan(resampled.flux[0])
        assert np.isnan(resampled.flux_error[0])
        assert resampled.flux[1] == pytest.approx(2.0, abs=1e-6)
        assert np.isnan(resampled.flux[2])
        assert np.isnan(resampled.flux[3])
        assert np.all(np.isnan(resampled.flux[4:6]))

    def test_resample_integrate_bad_pixels_like_c_test13(self):
        source = _create_bad_stair_spectrum(1, 8, 20.0, 1.0, [0, 7])
        target_wavelengths = _get_waves(19.0, 6, 2.0)
        resampled = source.resample_integrate(target_wavelengths)

        assert np.isnan(resampled.flux[0])
        assert np.isnan(resampled.flux[1])
        assert np.allclose(resampled.flux[2:4], np.array([4.0, 6.0]), atol=1e-6)
        assert np.all(np.isnan(resampled.flux[4:6]))

    def test_resample_integrate_bad_pixels_like_c_test14(self):
        source = _create_bad_stair_spectrum(1, 9, 20.5, 1.0, [0, 4])
        target_wavelengths = _get_waves(21.0, 5, 2.0)
        resampled = source.resample_integrate(target_wavelengths)

        assert resampled.flux[0] == pytest.approx(2.0, abs=1e-6)
        assert resampled.flux[1] == pytest.approx(3.5, abs=1e-6)
        assert np.isnan(resampled.flux[2])
        assert resampled.flux[3] == pytest.approx(7.5, abs=1e-6)
        assert np.isnan(resampled.flux[-1])

    def test_resample_integrate_shuffled_input_output_invariance_like_c_test4(self):
        """Parity intent from C test4: shuffled output grid preserves results."""
        source = _create_stair_spectrum(1, 8, 20.0, 2.0)
        target_wavelengths = np.arange(21.0, 30.0, 1.0)
        reference = source.resample_integrate(target_wavelengths)

        out_perm = np.array([1, 3, 5, 0, 8, 7, 6, 2, 4], dtype=int)
        shuffled_target = target_wavelengths[out_perm]
        shuffled_resampled = source.resample_integrate(shuffled_target)

        # Map shuffled outputs back to canonical order and compare.
        inverse_out_perm = np.argsort(out_perm)
        assert np.allclose(shuffled_resampled.wavelengths[inverse_out_perm], reference.wavelengths, atol=EPSILON)
        assert np.allclose(shuffled_resampled.flux[inverse_out_perm], reference.flux, atol=1e-6, equal_nan=True)
        assert np.allclose(
            shuffled_resampled.flux_error[inverse_out_perm], reference.flux_error, atol=1e-6, equal_nan=True
        )

    def test_resample_integrate_half_bin_split_with_last_outside_like_c_test6(self):
        """Parity with C integrate test6: half-bin split and trailing reject."""
        source = _create_stair_spectrum(1, 8, 20.0, 2.0)
        target_wavelengths = np.arange(20.5, 35.5, 1.0)
        resampled = source.resample_integrate(target_wavelengths)

        src = 1.0
        for i in range(len(target_wavelengths) - 1):
            assert resampled.flux[i] == pytest.approx(src, abs=1e-6)
            assert resampled.flux_error[i] == pytest.approx(src / 10.0, abs=1e-6)
            src = src + ((i + 1) % 2)

        assert np.isnan(resampled.flux[-1])
        assert np.isnan(resampled.flux_error[-1])

    def test_resample_akima_error_interpolation_like_c_test(self):
        """Parity with C test_spectrum1D_resample_spectrum_interpolation_error_test."""
        spec = _create_reference_spectrum_for_resample_error_interpolation()
        x = spec.wavelengths
        y_err = spec.flux_error
        x_r = np.array([1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.9, 11.1, 12.2, 13.9], dtype=float)
        closer_idx = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12], dtype=int)

        sp2 = spec.resample_to_wavelengths(x_r, InterpolationMethod.AKIMA)
        for i, idx in enumerate(closer_idx):
            err_expected = np.sqrt(
                y_err[idx] ** 2.0 * abs(x[idx + 1] - x_r[i]) + y_err[idx + 1] ** 2.0 * abs(x[idx] - x_r[i])
            )
            assert sp2.flux_error[i] == pytest.approx(err_expected, abs=EPSILON)
            assert sp2.wavelengths[i] == pytest.approx(x_r[i], abs=EPSILON)

    def test_resample_fit_error_interpolation_like_c_test(self):
        """Parity with C test_spectrum1D_resample_spectrum_fit_error_test_error_interpol."""
        spec = _create_reference_spectrum_for_resample_error_interpolation()
        x = spec.wavelengths
        y_err = spec.flux_error
        x_r = np.array([1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.9, 11.1, 12.2, 13.9], dtype=float)
        closer_idx = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12], dtype=int)

        sp2 = spec.resample_fit(x_r, 2, 5)
        for i, idx in enumerate(closer_idx):
            err_expected = np.sqrt(
                y_err[idx] ** 2.0 * abs(x[idx + 1] - x_r[i]) + y_err[idx + 1] ** 2.0 * abs(x[idx] - x_r[i])
            )
            assert sp2.flux_error[i] == pytest.approx(err_expected, abs=EPSILON)
            assert sp2.wavelengths[i] == pytest.approx(x_r[i], abs=EPSILON)

    @pytest.mark.parametrize("is_error_free", [True, False])
    def test_resample_fit_error_consistency_after_shift_like_c_test(self, is_error_free):
        """Parity with C test_spectrum1D_resample_spectrum_fit_error_test_shift."""
        spec = _create_reference_spectrum_for_resample_shift(is_error_free)
        wl = spec.wavelengths
        wl_resampled1 = wl[:-1] + 0.2
        wl_resampled2 = wl[1:-1].copy()

        sp2 = spec.resample_fit(wl_resampled1, 4, len(wl) - 3)
        sp3 = sp2.resample_fit(wl_resampled2, 4, len(wl) - 3)

        flux1 = spec.flux
        err1 = spec.flux_error
        flux3 = sp3.flux
        err3 = sp3.flux_error

        for i in range(1, len(wl) - 2):
            assert err3[i - 1] == pytest.approx(err1[i], abs=5e-2)
            assert flux3[i - 1] == pytest.approx(flux1[i], abs=0.5)

    def test_spectrum_spectrum_operations(self):
        spec1 = create_linear_spectrum(50)
        spec2 = create_linear_spectrum(50)

        flux2 = spec2.flux * 1.1
        spec2 = Spectrum1D(flux2, spec2.flux_error, spec2.wavelengths, "linear")

        result = spec1.div_spectrum_create(spec2)
        assert result.size == 50

        result = spec1.mul_spectrum_create(spec2)
        assert result.size == 50

        result = spec1.add_spectrum_create(spec2)
        assert result.size == 50

        result = spec1.sub_spectrum_create(spec2)
        assert result.size == 50

        result = spec1.duplicate()
        result.div_spectrum(spec2)
        assert result.size == 50

        result = spec1.duplicate()
        result.mul_spectrum(spec2)
        assert result.size == 50

        result = spec1.duplicate()
        result.add_spectrum(spec2)
        assert result.size == 50

        result = spec1.duplicate()
        result.sub_spectrum(spec2)
        assert result.size == 50

    def test_spectrum_spectrum_operation_errors_like_c_test(self):
        """Parity with C calculation_error tests for incompatible spectra."""
        spec1 = create_linear_spectrum(50)
        spec2 = create_linear_spectrum(30)

        with pytest.raises((hdrlcore.IllegalInputError, hdrlcore.IncompatibleInputError)):
            spec1.div_spectrum_create(spec2)
        with pytest.raises((hdrlcore.IllegalInputError, hdrlcore.IncompatibleInputError)):
            spec1.mul_spectrum_create(spec2)
        with pytest.raises((hdrlcore.IllegalInputError, hdrlcore.IncompatibleInputError)):
            spec1.add_spectrum_create(spec2)
        with pytest.raises((hdrlcore.IllegalInputError, hdrlcore.IncompatibleInputError)):
            spec1.sub_spectrum_create(spec2)

        with pytest.raises((hdrlcore.IllegalInputError, hdrlcore.IncompatibleInputError)):
            spec1.duplicate().div_spectrum(spec2)
        with pytest.raises((hdrlcore.IllegalInputError, hdrlcore.IncompatibleInputError)):
            spec1.duplicate().mul_spectrum(spec2)
        with pytest.raises((hdrlcore.IllegalInputError, hdrlcore.IncompatibleInputError)):
            spec1.duplicate().add_spectrum(spec2)
        with pytest.raises((hdrlcore.IllegalInputError, hdrlcore.IncompatibleInputError)):
            spec1.duplicate().sub_spectrum(spec2)

    def test_shift_xcorrelation(self):
        wavelengths = np.linspace(1, 10, 100)
        flux = np.sin(np.linspace(0, 10 * np.pi, 100))
        flux_error = np.ones(100) * 0.1
        spec1 = Spectrum1D(flux, flux_error, wavelengths, "linear")

        flux2 = np.roll(flux, 5)
        spec2 = Spectrum1D(flux2, flux_error, wavelengths, "linear")

        result = spec1.compute_shift_xcorrelation(spec2, 10, True)

        assert isinstance(result, XCorrelationResult)
        assert math.isfinite(result.shift)

    def test_shift_xcorrelation_error_cases_like_c_test(self):
        """Parity with C shift xcorrelation null/incompatible input checks."""
        wavelengths = np.linspace(1, 10, 100)
        flux = np.sin(np.linspace(0, 10 * np.pi, 100))
        flux_error = np.ones(100) * 0.1
        spec1 = Spectrum1D(flux, flux_error, wavelengths, "linear")

        with pytest.raises(TypeError):
            spec1.compute_shift_xcorrelation(None, 1, False)

        # Incompatible spectrum shape should fail with HDRL illegal input.
        spec2 = create_linear_spectrum(30)
        with pytest.raises((hdrlcore.IllegalInputError, hdrlcore.IncompatibleInputError)):
            spec1.compute_shift_xcorrelation(spec2, 1, False)

    def test_shift_fit(self):
        wavelengths = np.linspace(1, 10, 100)
        flux = np.sin(np.linspace(0, 10 * np.pi, 100))
        flux_error = np.ones(100) * 0.1
        flux2 = np.roll(flux, 5)
        spec2 = Spectrum1D(flux2, flux_error, wavelengths, "linear")

        with pytest.raises(hdrlcore.IllegalInputError):
            spec2.compute_shift_fit(wguess=5.0, wrange=(1.0, 10.0), fitrange=(0.0, 11.0), halfsize=5.0)
        shift = spec2.compute_shift_fit(wguess=5.0, wrange=(1.0, 10.0), fitrange=(2.0, 9.0), halfsize=5.0)
        assert shift == pytest.approx(0.4)

    def test_shift_fit_on_slope_like_c_test(self):
        """Port of hdrl_spectrum1d_shift-test.c:test_on_slope."""
        spectrum = _create_gaussian_times_absorption_spectrum(abs_mean=1754.0)
        shift = spectrum.compute_shift_fit(
            wguess=1750.0, wrange=(1730.0, 1770.0), fitrange=(1740.0, 1760.0), halfsize=20.0
        )
        assert (1.0 + shift) * 1750.0 == pytest.approx(1754.0, rel=1e-3)

    def test_shift_fit_on_peak_like_c_test(self):
        """Port of hdrl_spectrum1d_shift-test.c:test_on_peak."""
        spectrum = _create_gaussian_times_absorption_spectrum(abs_mean=1504.0)
        shift = spectrum.compute_shift_fit(
            wguess=1500.0, wrange=(1480.0, 1520.0), fitrange=(1490.0, 1510.0), halfsize=20.0
        )
        assert (1.0 + shift) * 1500.0 == pytest.approx(1504.0, rel=1e-3)

    def test_operator_overloads(self):
        spec = create_linear_spectrum(50)
        original_flux = spec.flux.copy()

        scalar = 2.0
        result = spec * scalar
        assert isinstance(result, Spectrum1D)
        assert np.allclose(result.flux, original_flux * scalar, atol=EPSILON)

        spec_copy = spec.duplicate()
        spec_copy *= scalar
        assert np.allclose(spec_copy.flux, original_flux * scalar, atol=EPSILON)

        result = spec / scalar
        assert isinstance(result, Spectrum1D)
        assert np.allclose(result.flux, original_flux / scalar, atol=EPSILON)

        spec_copy = spec.duplicate()
        spec_copy /= scalar
        assert np.allclose(spec_copy.flux, original_flux / scalar, atol=EPSILON)

        result = spec + scalar
        assert isinstance(result, Spectrum1D)
        assert np.allclose(result.flux, original_flux + scalar, atol=EPSILON)

        spec_copy = spec.duplicate()
        spec_copy += scalar
        assert np.allclose(spec_copy.flux, original_flux + scalar, atol=EPSILON)

        result = spec - scalar
        assert isinstance(result, Spectrum1D)
        assert np.allclose(result.flux, original_flux - scalar, atol=EPSILON)

        spec_copy = spec.duplicate()
        spec_copy -= scalar
        assert np.allclose(spec_copy.flux, original_flux - scalar, atol=EPSILON)

        with pytest.raises(ValueError):
            _ = spec / 0.0

    def test_compatibility(self):
        spec1 = create_linear_spectrum(50)
        spec2 = create_linear_spectrum(50)

        assert spec1.is_compatible_with(spec2)

        spec3 = create_linear_spectrum(30)
        assert not spec1.is_compatible_with(spec3)

    def test_save(self):
        spec = create_linear_spectrum(50)
        expected_flux = spec.flux.copy()
        expected_wavelengths = spec.wavelengths.copy()
        expected_errors = spec.flux_error.copy()

        with tempfile.NamedTemporaryFile(suffix=".fits", delete=False) as tmp:
            tmp_path = Path(tmp.name)

            try:
                spec.save(tmp_path)
                assert tmp_path.exists()

                # Validate that saved content is a proper CPL table with
                # expected HDRL spectrum columns and data.
                table = cplcore.Table.load(str(tmp_path), 1)
                assert table is not None

                column_names = table.column_names
                assert "WAVELENGTH" in column_names
                assert "FLUX" in column_names
                assert "FLUX_ERROR" in column_names

                nrows, _ = table.shape
                assert nrows == len(expected_flux)

                # Compare a few representative rows to avoid over-coupling to
                # formatting details while still checking roundtrip integrity.
                for idx in (0, len(expected_flux) // 2, len(expected_flux) - 1):
                    assert table["WAVELENGTH"][idx][0] == pytest.approx(expected_wavelengths[idx], abs=EPSILON)
                    assert table["FLUX"][idx][0] == pytest.approx(expected_flux[idx], abs=EPSILON)
                    assert table["FLUX_ERROR"][idx][0] == pytest.approx(expected_errors[idx], abs=EPSILON)
            finally:
                if tmp_path.exists():
                    tmp_path.unlink()

    def test_constructor_analytical_like_c_test(self):
        """Parity with C test_spectrum1D_constructor_analytical."""
        wav = np.arange(10.0, 110.0, 10.0, dtype=float)

        def analytic(lmbd):
            return (lmbd * 2.0, lmbd * 3.0)

        spec = Spectrum1D(analytic, wav, "linear")
        assert spec.size == len(wav)
        assert spec.scale == WaveScale.LINEAR
        assert np.allclose(spec.wavelengths, wav, atol=EPSILON)
        assert np.allclose(spec.flux, wav * 2.0, atol=1e-6)
        assert np.allclose(spec.flux_error, wav * 3.0, atol=1e-6)

    def test_uniformly_sampled_like_c_test(self):
        """Parity with C test_spectrum1D_test_uniformly_sampled."""
        wav = np.linspace(0.0, 2.0 * np.pi, 100, endpoint=False)
        flux = np.full(100, 0.1, dtype=float)
        err = np.full(100, 0.01, dtype=float)
        spec = Spectrum1D(flux, err, wav, "linear")

        is_ok, delta = spec.is_uniformly_sampled()
        assert is_ok
        assert delta == pytest.approx((2.0 * np.pi) / 100, abs=1e-6)

        wav_bad = np.arange(1.0, 101.0, 1.0, dtype=float)
        wav_bad[4] = 5.1
        spec_bad = Spectrum1D(flux, err, wav_bad, "linear")
        is_ok_bad, _ = spec_bad.is_uniformly_sampled()
        assert not is_ok_bad

        wav_good = np.arange(1.0, 101.0, 1.0, dtype=float)
        spec_good = Spectrum1D(flux, err, wav_good, "linear")
        is_ok_good, delta_good = spec_good.is_uniformly_sampled()
        assert is_ok_good
        assert delta_good == pytest.approx(1.0, abs=1e-6)

    def test_bad_pixel_map_roundtrip(self):
        x = np.arange(1, 7, dtype=float)
        flux = np.linspace(10.0, 15.0, x.size)
        spec = Spectrum1D(flux, None, x, "linear")

        assert np.array_equal(spec.bad_pixel_map, np.zeros(x.size, dtype=np.int32))

        bad_flags = np.array([0, 1, 0, 1, 0, 0], dtype=np.int32)
        rejected = spec.reject_pixels(bad_flags)

        # reject_pixels returns a new spectrum, original must stay unchanged.
        assert np.array_equal(spec.bad_pixel_map, np.zeros(x.size, dtype=np.int32))
        assert np.array_equal(rejected.bad_pixel_map, bad_flags)

    @pytest.mark.parametrize("interpolate", [True, False])
    def test_resample_spectrum_bpm_like_c_test(self, interpolate):
        """
        Port of hdrl_spectrum1D-test.c:test_spectrum1D_resample_spectrum_bpm.

        The C test sets every other input pixel as rejected, then checks:
        - first/last output pixels are rejected (represented as NaN in Python bindings)
        - intermediate output pixels are zero because only rejected pixels contribute
        """
        x = np.arange(1, 16, dtype=float)  # 1..15
        x_r = np.array(
            [1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.9, 11.1, 12.2, 13.9, 14.1],
            dtype=float,
        )

        xlen = x.size
        bad_flags = (np.arange(xlen) % 2 == 0).astype(np.int32)  # even indices rejected
        flux = np.zeros(xlen, dtype=float)
        flux[bad_flags.astype(bool)] = 10.0

        spec = Spectrum1D(flux, None, x, "linear").reject_pixels(bad_flags)

        if interpolate:
            resampled = spec.resample_to_wavelengths(x_r, InterpolationMethod.AKIMA)
        else:
            # In C: hdrl_spectrum1D_resample_fit_parameter_create(2, 5)
            resampled = spec.resample_fit(x_r, 2, 5)

        assert np.isnan(resampled.flux[0])
        assert np.isnan(resampled.flux[-1])
        assert not np.isnan(resampled.flux[1:-1]).any()
        assert np.allclose(resampled.flux[1:-1], 0.0, atol=1e-6)

    @pytest.mark.skip(reason="Legacy module-level private helper bindings removed from hdrl.core.")
    def test_resample_spectrum_private_funcs_like_c_test(self):
        x = np.array([3.0, 2.1, 5.5, 8.7, 3.3, 5.6, 2.1], dtype=float)
        y1 = np.array([11.0, 88.0, -22.0, 56.0, 4.0, 22.0, 23.0], dtype=float)
        y2 = np.array([2.0, 55.0, 2.0, 27.0, 23.0, 1.0, 5.0], dtype=float)

        x_sorted, y1_sorted, y2_sorted = hdrlcore.spectrum1d_sort_on_x(x, y1, y2)
        assert np.allclose(x_sorted, np.array([2.1, 2.1, 3.0, 3.3, 5.5, 5.6, 8.7]), atol=EPSILON)
        assert np.allclose(y1_sorted, np.array([88.0, 23.0, 11.0, 4.0, -22.0, 22.0, 56.0]), atol=EPSILON)
        assert np.allclose(y2_sorted, np.array([55.0, 5.0, 2.0, 23.0, 2.0, 1.0, 27.0]), atol=EPSILON)

        l_out, x_f, y1_f, y2_f = hdrlcore.spectrum1d_resample_filter_dups_and_substitute_with_median(
            np.array([1, 2, 2, 3, 3, 3, 5, 6, 7, 7, 8, 9, 10, 10, 10, 11], dtype=float),
            np.array([4, 3, 7, 8, 9, 4, 3, 7, 2, 4, 5, 2, 8, 7, 1, 12], dtype=float),
            np.array([3, 6, 7, 8, 4, 5, 8, 3, 5, 1, 3, 8, 44, 33, 55, 45], dtype=float),
        )

        assert l_out == 10
        assert np.allclose(x_f, np.array([1, 2, 3, 5, 6, 7, 8, 9, 10, 11], dtype=float), atol=EPSILON)
        assert np.allclose(y1_f, np.array([4, 5, 8, 3, 7, 3, 5, 2, 7, 12], dtype=float), atol=EPSILON)
        assert np.allclose(y2_f, np.array([3, 6.5, 5, 8, 3, 3, 3, 8, 44, 45], dtype=float), atol=EPSILON)

    @pytest.mark.skip(reason="Legacy module-level spectrum resample parameter factories removed.")
    def test_resample_interpolate_parameter_parlist_like_c_test(self):
        pytest.skip("Parlist-based interpolate parameter path removed from bindings.")


class TestSpectrum1DList:
    def test_create_empty_list(self):
        spectrum_list = Spectrum1DList()
        assert spectrum_list is not None
        assert len(spectrum_list) == 0

    def test_create_list(self):
        spectrum_list = Spectrum1DList()
        assert spectrum_list is not None
        assert len(spectrum_list) == 0

    def test_create_from_array(self):
        spectrum1 = create_test_spectrum(100)
        spectrum2 = create_test_spectrum(100)

        spectrum_list = Spectrum1DList([spectrum1, spectrum2])

        assert spectrum_list is not None
        assert len(spectrum_list) == 2

    def test_get_set_spectrum(self):
        spectrum1 = create_test_spectrum(100)
        spectrum2 = create_test_spectrum(100)
        spectrum3 = create_test_spectrum(100)

        spectrum_list = Spectrum1DList([spectrum1, spectrum2])

        retrieved_spectrum = spectrum_list[0]
        assert retrieved_spectrum is not None
        assert retrieved_spectrum.size == 100

        spectrum_list[1] = spectrum3
        retrieved_spectrum = spectrum_list[1]
        assert retrieved_spectrum is not None
        assert retrieved_spectrum.size == 100

    def test_unset_spectrum(self):
        spectrum1 = create_test_spectrum(100)
        spectrum2 = create_test_spectrum(100)

        spectrum_list = Spectrum1DList([spectrum1, spectrum2])

        removed_spectrum = spectrum_list.pop(0)

        assert removed_spectrum is not None
        assert len(spectrum_list) == 1

    def test_unset_sequence_like_c_test(self):
        """Parity with C unset loop behavior preserving sequential order."""
        spectra = [_create_single_bin_spectrum(i) for i in range(1, 7)]
        spectrum_list = Spectrum1DList(spectra)

        popped = []
        while len(spectrum_list):
            popped.append(float(spectrum_list.pop(0).flux[0]))

        assert popped == [1.0, 2.0, 3.0, 4.0, 5.0, 6.0]
        assert len(spectrum_list) == 0

    def test_duplicate_list(self):
        spectrum1 = create_test_spectrum(100)
        spectrum2 = create_test_spectrum(100)

        spectrum_list = Spectrum1DList([spectrum1, spectrum2])

        duplicated_list = spectrum_list.duplicate()

        assert duplicated_list is not None
        assert len(duplicated_list) == 2

    def test_duplicate_list_is_independent_like_c_test(self):
        """Parity with C duplicate-list mutability check."""
        src = Spectrum1DList([_create_single_bin_spectrum(1), _create_single_bin_spectrum(2)])
        dup = src.duplicate()

        dup[0].mul_scalar(5.0)
        dup[1].mul_scalar(5.0)

        assert src[0].flux[0] == pytest.approx(1.0)
        assert src[1].flux[0] == pytest.approx(2.0)
        assert dup[0].flux[0] == pytest.approx(5.0)
        assert dup[1].flux[0] == pytest.approx(10.0)

    def test_list_len(self):
        spectrum1 = create_test_spectrum(100)
        spectrum2 = create_test_spectrum(100)

        spectrum_list = Spectrum1DList([spectrum1, spectrum2])

        assert len(spectrum_list) == 2

    def test_list_getitem(self):
        spectrum1 = create_test_spectrum(100)
        spectrum2 = create_test_spectrum(100)

        spectrum_list = Spectrum1DList([spectrum1, spectrum2])

        retrieved_spectrum = spectrum_list[0]
        assert retrieved_spectrum is not None
        assert retrieved_spectrum.size == 100

    def test_list_setitem(self):
        spectrum1 = create_test_spectrum(100)
        spectrum2 = create_test_spectrum(100)
        spectrum3 = create_test_spectrum(100)

        spectrum_list = Spectrum1DList([spectrum1, spectrum2])

        spectrum_list[1] = spectrum3
        retrieved_spectrum = spectrum_list[1]
        assert retrieved_spectrum is not None
        assert retrieved_spectrum.size == 100

    def test_list_setitem_keyword_arguments(self):
        spectrum1 = create_test_spectrum(40)
        spectrum2 = create_test_spectrum(40)
        spectrum3 = create_test_spectrum(40)
        spectrum_list = Spectrum1DList([spectrum1, spectrum2])

        spectrum_list.__setitem__(index=1, spectrum=spectrum3)
        retrieved_spectrum = spectrum_list[1]
        assert retrieved_spectrum is not None
        assert retrieved_spectrum.size == 40

    def test_list_setitem_invalid_keyword_arguments(self):
        spectrum1 = create_test_spectrum(20)
        spectrum2 = create_test_spectrum(20)
        spectrum_list = Spectrum1DList([spectrum1, spectrum2])

        with pytest.raises(TypeError):
            spectrum_list.__setitem__(spectrum=spectrum1)

    def test_index_errors(self):
        spectrum_list = Spectrum1DList()
        spectrum = create_test_spectrum(10)

        with pytest.raises(IndexError):
            _ = spectrum_list[0]

        with pytest.raises(IndexError):
            spectrum_list[0] = spectrum

        with pytest.raises(IndexError):
            _ = spectrum_list.pop(0)

    def test_insert_duplication_rejected_like_c_test(self):
        """Parity with C insert-duplication rejection semantics."""
        s1 = _create_single_bin_spectrum(1)
        s2 = _create_single_bin_spectrum(2)
        s3 = _create_single_bin_spectrum(3)
        s4 = _create_single_bin_spectrum(4)
        s5 = _create_single_bin_spectrum(5)
        s6 = _create_single_bin_spectrum(6)
        spectrum_list = Spectrum1DList([s1, s2, s3, s4, s5, s6])

        with pytest.raises((hdrlcore.IllegalInputError, ValueError)):
            spectrum_list[4] = s1
        with pytest.raises((hdrlcore.IllegalInputError, ValueError)):
            spectrum_list[3] = s2
        with pytest.raises((hdrlcore.IllegalInputError, ValueError)):
            spectrum_list[4] = s3
        with pytest.raises((hdrlcore.IllegalInputError, ValueError)):
            spectrum_list[5] = s4
        with pytest.raises((hdrlcore.IllegalInputError, ValueError)):
            spectrum_list[0] = s5
        with pytest.raises((hdrlcore.IllegalInputError, ValueError)):
            spectrum_list[2] = s6

    def test_spectrum1dlist_collapse_badpix_like_c_test(self):
        spectra = Spectrum1DList(
            [
                _create_spectrum_long([1, 2, 3, 4]),
                _create_spectrum_long([-1, 2, 4]),
                _create_spectrum_long([1, 3, -4]),
            ]
        )
        w = np.arange(6, dtype=float)
        stacking = hdrlfunc.Collapse.Mean()
        resample = hdrlcore.Spectrum1DResampleMethod.Interpolate(InterpolationMethod.LINEAR)
        collapse_result = spectra.collapse(stacking, w, resample, False)
        collapsed = collapse_result.result

        assert collapsed.size == len(w)
        assert collapse_result.contrib is not None
        assert collapse_result.contrib.shape == (1, len(w))
        assert np.isnan(collapsed.flux[0])
        assert np.isnan(collapsed.flux[-1])
        assert np.allclose(collapsed.flux[1:5], np.array([1.0, 2.0, 3.0, 4.0]), atol=1e-6)

    def test_spectrum1dlist_collapse_mark_rej_in_interpolation_like_c_test(self):
        spectra = Spectrum1DList(
            [
                _create_spectrum_long([1, 2, 3, 4]),
                _create_spectrum_long([-1, 2, -3, 4]),
                _create_spectrum_long([1, -2, 3, -4]),
            ]
        )
        w = np.arange(6, dtype=float)
        stacking = hdrlfunc.Collapse.Mean()
        resample = hdrlcore.Spectrum1DResampleMethod.Interpolate(InterpolationMethod.LINEAR)
        collapse_result = spectra.collapse(stacking, w, resample, True)
        collapsed = collapse_result.result

        assert collapsed.size == len(w)
        assert collapse_result.contrib is not None
        assert collapse_result.contrib.shape == (1, len(w))
        assert np.isnan(collapsed.flux[0])
        assert np.isnan(collapsed.flux[-1])
        assert np.allclose(collapsed.flux[1:5], np.array([1.0, 2.0, 3.0, 4.0]), atol=1e-6)

    def test_spectrum1dlist_collapse_holes_like_c_test(self):
        spectra = Spectrum1DList(
            [
                _create_spectrum_long([1, 2, 3, 4]),
                _create_spectrum_long([2, 4]),
                _create_spectrum_long([1, 3]),
            ]
        )
        w = np.arange(6, dtype=float)
        stacking = hdrlfunc.Collapse.Mean()
        resample = hdrlcore.Spectrum1DResampleMethod.Interpolate(InterpolationMethod.LINEAR)
        collapse_result = spectra.collapse(stacking, w, resample, False)
        collapsed = collapse_result.result

        assert collapsed.size == len(w)
        assert collapse_result.contrib is not None
        assert collapse_result.contrib.shape == (1, len(w))
        assert np.isnan(collapsed.flux[0])
        assert np.isnan(collapsed.flux[-1])
        assert np.allclose(collapsed.flux[1:5], np.array([1.0, 2.0, 3.0, 4.0]), atol=1e-6)
