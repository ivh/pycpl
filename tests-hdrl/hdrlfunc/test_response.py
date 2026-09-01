import hdrl
import numpy as np
import pytest

# Constants for testing

TEST_FLUX = np.array([1, 2, 3, 4, 0, 1], dtype=np.float64)
TEST_FLUX_ERROR = np.array([0.1, 0.2, 0.1, 0.1, 0.05, 0.3], dtype=np.float64)
TEST_WAVELENGTHS = np.array([3.1, 5.0, 7.0, 9.0, 10.9, 10.95], dtype=np.float64)


@pytest.fixture
def sample_spectrum():
    """Create a sample spectrum for testing"""
    return hdrl.core.Spectrum1D(TEST_FLUX, TEST_FLUX_ERROR, TEST_WAVELENGTHS, "linear")


@pytest.fixture
def sample_spectrum_list(sample_spectrum):
    """Create a sample spectrum list for testing"""
    # Create a list with one element using create_from_array
    return hdrl.core.Spectrum1DList([sample_spectrum])


@pytest.fixture
def sample_telluric_models(sample_spectrum):
    """Create a sample list of telluric models for testing"""
    # Create multiple spectra for telluric models
    telluric_spectra = []
    for i in range(5):  # Create 5 telluric model spectra
        # Create slightly modified versions of the original spectrum
        modified_flux = TEST_FLUX * (0.9 + 0.2 * i / 5)
        modified_spectrum = hdrl.core.Spectrum1D(modified_flux, TEST_FLUX_ERROR * 0.8, TEST_WAVELENGTHS, "linear")
        telluric_spectra.append(modified_spectrum)

    # Create the list from the array of spectra
    return hdrl.core.Spectrum1DList(telluric_spectra)


@pytest.fixture
def sample_parameters():
    """Create sample parameters for testing"""
    velocity_param = hdrl.func.Response.velocity_parameter_create(
        wguess=1.0, range_wmin=3.5, range_wmax=10.9, fit_wmin=4.0, fit_wmax=7.0, fit_half_win=1
    )

    calc_param = hdrl.func.Response.calc_parameter_create(Ap=1.0, Am=0.5, G=0.1, Tex=5000.0)

    return velocity_param, calc_param


class TestResponse:
    """Test the Response class methods"""

    def test_velocity_parameter_create(self):
        """Test creating velocity parameters"""
        param = hdrl.func.Response.velocity_parameter_create(
            wguess=1.0, range_wmin=3.5, range_wmax=10.5, fit_wmin=4.0, fit_wmax=7.0, fit_half_win=1
        )
        assert isinstance(param, hdrl.func.ResponseVelocityParameter)

    def test_velocity_parameter_create_invalid(self):
        """Test creating velocity parameters with invalid inputs"""
        with pytest.raises(hdrl.core.IllegalInputError):
            hdrl.func.Response.velocity_parameter_create(
                wguess=1.0,
                range_wmin=10.5,  # Invalid: range_wmin > range_wmax
                range_wmax=3.5,
                fit_wmin=4.0,
                fit_wmax=7.0,
                fit_half_win=1,
            )

        with pytest.raises(hdrl.core.IllegalInputError):
            hdrl.func.Response.velocity_parameter_create(
                wguess=1.0,
                range_wmin=3.5,
                range_wmax=10.5,
                fit_wmin=7.0,  # Invalid: fit_wmin > fit_wmax
                fit_wmax=4.0,
                fit_half_win=1,
            )

        with pytest.raises(hdrl.core.IllegalInputError):
            hdrl.func.Response.velocity_parameter_create(
                wguess=1.0,
                range_wmin=3.5,
                range_wmax=10.5,
                fit_wmin=4.0,
                fit_wmax=7.0,
                fit_half_win=0,  # Invalid: fit_half_win must be > 0
            )

    def test_calc_parameter_create(self):
        """Test creating calculation parameters"""
        param = hdrl.func.Response.calc_parameter_create(Ap=1.0, Am=0.5, G=0.1, Tex=5000.0)
        assert isinstance(param, hdrl.func.ResponseCalcParameter)

    def test_calc_parameter_create_invalid(self):
        """Test creating calculation parameters with invalid inputs"""
        with pytest.raises(hdrl.core.IllegalInputError):
            hdrl.func.Response.calc_parameter_create(
                Ap=0.0,  # Invalid: must be positive
                Am=0.5,
                G=0.1,
                Tex=5000.0,
            )

        with pytest.raises(hdrl.core.IllegalInputError):
            hdrl.func.Response.calc_parameter_create(
                Ap=1.0,
                Am=-0.5,  # Invalid: must be positive
                G=0.1,
                Tex=5000.0,
            )

    def test_response_compute_with_velocity_dense_spectrum(self):
        """Compute response with velocity correction on a dense spectrum."""
        wlen = np.arange(3.0, 11.01, 0.1, dtype=np.float64)
        baseline = 1.0 + 0.05 * (wlen - 7.0)
        absorption = 0.7 * np.exp(-0.5 * ((wlen - 7.0) / 0.15) ** 2)
        flux = baseline - absorption
        flux_err = np.full_like(flux, 0.02, dtype=np.float64)

        s1 = hdrl.core.Spectrum1D(flux, flux_err, wlen, "linear")

        vel_par = hdrl.func.Response.velocity_parameter_create(
            wguess=7.0, range_wmin=3.0, range_wmax=11.0, fit_wmin=6.0, fit_wmax=8.0, fit_half_win=1
        )
        calc_par = hdrl.func.Response.calc_parameter_create(Ap=1.0, Am=0.5, G=0.1, Tex=5000.0)
        fit_points = np.array([3.1, 6.8, 6.9, 7.0, 7.5, 9.0, 11.0], dtype=np.float64)
        high_abs_regions = (
            np.array([8.9], dtype=np.float64),
            np.array([9.1], dtype=np.float64),
        )
        fit_par = hdrl.func.Response.fit_parameter_create(
            radius=11,
            fit_points=fit_points,
            wrange=1.0,
            high_abs_regions=high_abs_regions,
        )

        result = hdrl.func.Response.compute(
            s1, s1, s1, hdrl.func.ResponseTelluricParameter(), vel_par, calc_par, fit_par
        )

        assert np.isfinite(result.get_doppler_shift())
        assert abs(result.get_doppler_shift()) < 1e-6

    def test_response_compute(self):
        """Compute response using the original C test vectors."""
        flx_t1 = np.array([1, 2, 3, 4, 0, 1], dtype=np.float64)
        flx_e_t1 = np.array([0.1, 0.2, 0.1, 0.1, 0.05, 0.3], dtype=np.float64)
        wlen_t1 = np.array([3.1, 5.0, 7.0, 9.0, 10.9, 10.95], dtype=np.float64)
        t1 = hdrl.core.Spectrum1D(flx_t1, flx_e_t1, wlen_t1, "linear")

        flx_t2 = np.array([2, 4, 6, 8, 0, 1], dtype=np.float64)
        flx_e_t2 = np.array([0.1, 0.2, 0.1, 0.1, 0.05, 3.3], dtype=np.float64)
        wlen_t2 = np.array([3.1, 5.0, 7.0, 9.0, 10.9, 10.95], dtype=np.float64)
        t2 = hdrl.core.Spectrum1D(flx_t2, flx_e_t2, wlen_t2, "linear")

        telluric_models = hdrl.core.Spectrum1DList([t2, t1])

        areas = (
            np.array([3.0, 6.9, 10.0], dtype=np.float64),
            np.array([5.1, 7.1, 11.0], dtype=np.float64),
        )
        tell_par = hdrl.func.Response.telluric_evaluation_parameter_create(
            telluric_models=telluric_models,
            w_step=1.0,
            half_win=15,
            normalize=False,
            shift_in_log_scale=False,
            quality_areas=areas,
            fit_areas=areas,
            lmin=3.0,
            lmax=11.0,
        )

        calc_par = hdrl.func.Response.calc_parameter_create(Ap=1.0, Am=0.5, G=0.1, Tex=5000.0)
        vel_par = hdrl.func.Response.velocity_parameter_create(
            wguess=1.0, range_wmin=3.5, range_wmax=10.5, fit_wmin=4.0, fit_wmax=7.0, fit_half_win=1
        )

        fit_points = np.array([3.1, 6.8, 6.9, 7.0, 7.5, 9.0, 11.0], dtype=np.float64)
        high_abs_regions = (
            np.array([8.9], dtype=np.float64),
            np.array([9.1], dtype=np.float64),
        )
        fit_par = hdrl.func.Response.fit_parameter_create(
            radius=11,
            fit_points=fit_points,
            wrange=1.0,
            high_abs_regions=high_abs_regions,
        )

        result = hdrl.func.Response.compute(
            t1, t1, t1, tell_par, hdrl.func.ResponseVelocityParameter(), calc_par, fit_par
        )

        assert result.get_best_telluric_model_idx() == 1
        assert np.isclose(result.get_avg_diff_from_1(), 0.0041, rtol=1e-2)
        assert np.isclose(result.get_stddev(), 0.707, rtol=1e-2)

    def test_telluric_evaluation_parameter_create(self, sample_spectrum_list):
        """Test creating telluric evaluation parameters"""
        quality_areas = (
            np.array([3.0, 6.9, 10.0], dtype=np.float64),
            np.array([5.1, 7.1, 11.0], dtype=np.float64),
        )
        fit_areas = (
            np.array([3.0, 6.9, 10.0], dtype=np.float64),
            np.array([5.1, 7.1, 11.0], dtype=np.float64),
        )

        param = hdrl.func.Response.telluric_evaluation_parameter_create(
            telluric_models=sample_spectrum_list,
            w_step=0.5,
            half_win=10,
            normalize=True,
            shift_in_log_scale=False,
            quality_areas=quality_areas,
            fit_areas=fit_areas,
            lmin=3.5,
            lmax=7.5,
        )
        assert isinstance(param, hdrl.func.ResponseTelluricParameter)

    def test_telluric_evaluation_parameter_create_invalid(self, sample_spectrum_list):
        """Test creating telluric evaluation parameters with invalid inputs"""
        quality_areas = (
            np.array([3.0, 6.9, 10.0], dtype=np.float64),
            np.array([5.1, 7.1, 11.0], dtype=np.float64),
        )
        fit_areas = (
            np.array([3.0, 6.9, 10.0], dtype=np.float64),
            np.array([5.1, 7.1, 11.0], dtype=np.float64),
        )

        with pytest.raises(hdrl.core.IllegalInputError):
            hdrl.func.Response.telluric_evaluation_parameter_create(
                telluric_models=sample_spectrum_list,
                w_step=0.0,  # Invalid: must be positive
                half_win=10,
                normalize=True,
                shift_in_log_scale=False,
                quality_areas=quality_areas,
                fit_areas=fit_areas,
                lmin=3,
                lmax=11,
            )

        with pytest.raises(hdrl.core.IllegalInputError):
            hdrl.func.Response.telluric_evaluation_parameter_create(
                telluric_models=sample_spectrum_list,
                w_step=0.5,
                half_win=10,
                normalize=True,
                shift_in_log_scale=False,
                quality_areas=quality_areas,
                fit_areas=fit_areas,
                lmin=11,  # Invalid: lmin > lmax
                lmax=3.0,
            )

    def test_fit_parameter_create_invalid(self):
        """Test creating fit parameters with invalid inputs"""
        fit_points = np.array([3.1, 6.8, 6.9, 7.0, 7.5, 9.0, 11.0], dtype=np.float64)
        high_abs_regions = (
            np.array([8.9], dtype=np.float64),
            np.array([9.1], dtype=np.float64),
        )

        with pytest.raises(hdrl.core.IllegalInputError):
            hdrl.func.Response.fit_parameter_create(
                radius=0,  # Invalid: must be > 0
                fit_points=fit_points,
                wrange=1.0,
                high_abs_regions=high_abs_regions,
            )

        with pytest.raises(hdrl.core.IllegalInputError):
            hdrl.func.Response.fit_parameter_create(
                radius=10,
                fit_points=np.array([], dtype=np.float64),  # Invalid
                wrange=1.0,
                high_abs_regions=high_abs_regions,
            )

        with pytest.raises(hdrl.core.IllegalInputError):
            hdrl.func.Response.fit_parameter_create(
                radius=11,
                fit_points=fit_points,
                wrange=0.0,  # Invalid: must be positive
                high_abs_regions=high_abs_regions,
            )

        high_abs_regions = (
            np.array([], dtype=np.float64),
            np.array([], dtype=np.float64),
        )
        with pytest.raises(hdrl.core.IllegalInputError):
            hdrl.func.Response.fit_parameter_create(
                radius=10,
                fit_points=fit_points,
                wrange=1.0,
                high_abs_regions=high_abs_regions,  # Invalid
            )
