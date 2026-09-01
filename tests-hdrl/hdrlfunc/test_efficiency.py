import pytest
import numpy as np
from hdrl import core as hdrlcore
from hdrl import func as hdrlfunc

def create_test_spectrum(size=40):
    """Create a test spectrum with known values"""
    np.random.seed(42)  # For reproducibility
    flux = np.random.uniform(1.0, 128.0, size)
    flux_error = np.random.uniform(0.5, 2.0, size)
    wavelengths = np.cumsum(np.random.uniform(1.0, 2.0, size))
    return hdrlcore.Spectrum1D(flux, flux_error, wavelengths, "linear")

def create_linear_spectrum(size=50):
    """Create a linear spectrum for testing resampling"""
    wavelengths = np.linspace(1, 10, size)
    flux = np.linspace(1, 10, size)
    flux_error = np.ones(size) * 0.1
    return hdrlcore.Spectrum1D(flux, flux_error, wavelengths, "linear")

@pytest.fixture
def test_spectrum():
    return create_test_spectrum()

@pytest.fixture
def linear_spectrum():
    return create_linear_spectrum()

@pytest.fixture
def mock_parameters():
    Ap = 1.0
    Am = 1.5
    G = 2.0
    Tex = 10.0
    Atel = 1000.0
    return Ap, Am, G, Tex, Atel

def test_create_parameter(mock_parameters):
    Ap, Am, G, Tex, Atel = mock_parameters
    parameter = hdrlfunc.Efficiency.create_parameter(Ap, Am, G, Tex, Atel)
    assert isinstance(parameter, hdrlfunc.EfficiencyParameter)

def test_create_response_parameter(mock_parameters):
    Ap, Am, G, Tex, _ = mock_parameters
    parameter = hdrlfunc.Efficiency.create_response_parameter(Ap, Am, G, Tex)
    assert isinstance(parameter, hdrlfunc.EfficiencyResponseParameter)

def test_compute(test_spectrum, linear_spectrum, mock_parameters):
    Ap, Am, G, Tex, Atel = mock_parameters
    parameter = hdrlfunc.Efficiency.create_parameter(Ap, Am, G, Tex, Atel)
    result = hdrlfunc.Efficiency.compute(test_spectrum, linear_spectrum, linear_spectrum, parameter)
    assert result is not None
    assert isinstance(result, hdrlcore.Spectrum1D)

def test_compute_response_core(test_spectrum, linear_spectrum, mock_parameters):
    Ap, Am, G, Tex, _ = mock_parameters
    parameter = hdrlfunc.Efficiency.create_response_parameter(Ap, Am, G, Tex)
    result = hdrlfunc.Efficiency.compute_response_core(test_spectrum, linear_spectrum, linear_spectrum, parameter)
    assert result is not None
    assert isinstance(result, hdrlcore.Spectrum1D)

