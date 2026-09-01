# This file is part of the PyHDRL Python language bindings
# Copyright (C) 2023-2026 European Southern Observatory
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
Test module for HDRL Airmass bindings.

This module tests the Python bindings for the HDRL airmass functionality,
based on the C unit tests in hdrl_utils-test.c.
"""

import hdrl.core as hdrlcore
import hdrl.func as hdrlfunc
import pytest


def test_airmass_creation():
    """Test Airmass object creation with valid parameters."""
    # Test data from C test (MUSE data 1)
    ra = (122.994945, 0.0)
    dec = (74.95304, 0.0)
    lst = (25407.072748, 0.0)
    exptime = (120.0, 0.0)
    latitude = (37.2236, 0.0)
    airmass_type = hdrlfunc.AirmassApprox.Hardie

    airmass = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, airmass_type)

    # Test accessor methods
    assert airmass.ra.data == ra[0]
    assert airmass.ra.error == ra[1]
    assert airmass.dec.data == dec[0]
    assert airmass.dec.error == dec[1]
    assert airmass.lst.data == lst[0]
    assert airmass.lst.error == lst[1]
    assert airmass.exptime.data == exptime[0]
    assert airmass.exptime.error == exptime[1]
    assert airmass.latitude.data == latitude[0]
    assert airmass.latitude.error == latitude[1]
    assert airmass.type == airmass_type


def test_airmass_compute_hardie_no_error():
    """Test airmass computation with Hardie approximation without error propagation."""
    # Test data from C test (MUSE data 1)
    ra = (122.994945, 0.0)
    dec = (74.95304, 0.0)
    lst = (25407.072748, 0.0)
    exptime = (120.0, 0.0)
    latitude = (37.2236, 0.0)
    airmass_type = hdrlfunc.AirmassApprox.Hardie

    airmass = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, airmass_type)
    result = airmass.compute()

    # Check result has data and error attributes
    assert hasattr(result, "data")
    assert hasattr(result, "error")

    # Check the computed airmass value (from C test: should be ~1.27803)
    assert result.data > 1.27803 - 0.001
    assert result.data < 1.27803 + 0.001
    assert result.error == pytest.approx(0.0, abs=1e-12)


def test_airmass_compute_muse_data_2():
    """Test airmass computation with MUSE data 2."""
    # Test data from C test (MUSE data 2)
    ra = (238.071555, 0.0)
    dec = (32.92533, 0.0)
    lst = (60515.584209, 0.0)
    exptime = (300.0, 0.0)
    latitude = (37.2236, 0.0)
    airmass_type = hdrlfunc.AirmassApprox.Hardie

    airmass = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, airmass_type)
    result = airmass.compute()

    # Check the computed airmass value (from C test: should be ~1.02529)
    assert result.data > 1.02529 - 0.001
    assert result.data < 1.02529 + 0.001
    assert result.error == pytest.approx(0.0, abs=1e-12)


def test_airmass_compute_muse_data_3():
    """Test airmass computation with MUSE data 3."""
    # Test data from C test (MUSE data 3)
    ra = (0.125, 0.0)
    dec = (-30.0, 0.0)
    lst = (69446.2765265328, 0.0)
    exptime = (3600.0, 0.0)
    latitude = (-24.625278, 0.0)
    airmass_type = hdrlfunc.AirmassApprox.Hardie

    airmass = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, airmass_type)
    result = airmass.compute()

    # Check the computed airmass value (from C test: should be ~1.79364)
    assert result.data > 1.79364 - 0.001
    assert result.data < 1.79364 + 0.001
    assert result.error == pytest.approx(0.0, abs=1e-12)


def test_airmass_compute_with_error_propagation():
    """Test airmass computation with error propagation."""
    # Test data from C test with error propagation
    delta = 1e-2
    ra = (122.994945, delta * abs(122.994945))
    dec = (74.95304, delta * abs(74.95304))
    lst = (25407.072748, delta * abs(25407.072748))
    exptime = (120.0, delta * abs(120.0))
    latitude = (37.2236, delta * abs(37.2236))
    airmass_type = hdrlfunc.AirmassApprox.Hardie

    airmass = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, airmass_type)
    result = airmass.compute()

    # Check the computed airmass value and error (from C test)
    assert result.data > 1.27803 - 0.001
    assert result.data < 1.27803 + 0.001
    assert result.error > 0.0136602 - 0.0001
    assert result.error < 0.0136602 + 0.0001


def test_airmass_different_approximations():
    """Test airmass computation with different approximation methods."""
    # Test data from C test
    ra = (122.994945, 0.0)
    dec = (74.95304, 0.0)
    lst = (25407.072748, 0.0)
    exptime = (120.0, 0.0)
    latitude = (37.2236, 0.0)

    # Test Hardie approximation
    airmass_hardie = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, hdrlfunc.AirmassApprox.Hardie)
    result_hardie = airmass_hardie.compute()
    assert result_hardie.data > 1.27803 - 0.001
    assert result_hardie.data < 1.27803 + 0.001

    # Test Young & Irvine approximation
    airmass_young_irvine = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, hdrlfunc.AirmassApprox.YoungIrvine)
    result_young_irvine = airmass_young_irvine.compute()
    assert result_young_irvine.data > 1.2778 - 0.001
    assert result_young_irvine.data < 1.2778 + 0.001

    # Test Young approximation
    airmass_young = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, hdrlfunc.AirmassApprox.Young)
    result_young = airmass_young.compute()
    assert result_young.data > 1.27755 - 0.001
    assert result_young.data < 1.27755 + 0.001


def test_airmass_invalid_ra():
    """Test airmass computation with invalid RA."""
    ra = (-1.0, 0.0)  # Invalid RA
    dec = (74.95304, 0.0)
    lst = (25407.072748, 0.0)
    exptime = (120.0, 0.0)
    latitude = (37.2236, 0.0)
    airmass_type = hdrlfunc.AirmassApprox.Hardie

    airmass = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, airmass_type)

    with pytest.raises(hdrlcore.IllegalInputError):
        airmass.compute()


def test_airmass_invalid_dec():
    """Test airmass computation with invalid Dec."""
    ra = (122.994945, 0.0)
    dec = (180.0, 0.0)  # Invalid Dec
    lst = (25407.072748, 0.0)
    exptime = (120.0, 0.0)
    latitude = (37.2236, 0.0)
    airmass_type = hdrlfunc.AirmassApprox.Hardie

    airmass = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, airmass_type)

    with pytest.raises(hdrlcore.IllegalInputError):
        airmass.compute()


def test_airmass_invalid_lst():
    """Test airmass computation with invalid LST."""
    ra = (122.994945, 0.0)
    dec = (74.95304, 0.0)
    lst = (-1.0, 0.0)  # Invalid LST
    exptime = (120.0, 0.0)
    latitude = (37.2236, 0.0)
    airmass_type = hdrlfunc.AirmassApprox.Hardie

    airmass = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, airmass_type)

    with pytest.raises(hdrlcore.IllegalInputError):
        airmass.compute()


def test_airmass_invalid_exptime():
    """Test airmass computation with invalid exposure time."""
    ra = (122.994945, 0.0)
    dec = (74.95304, 0.0)
    lst = (25407.072748, 0.0)
    exptime = (-1.0, 0.0)  # Invalid exposure time
    latitude = (37.2236, 0.0)
    airmass_type = hdrlfunc.AirmassApprox.Hardie

    airmass = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, airmass_type)

    with pytest.raises(hdrlcore.IllegalInputError):
        airmass.compute()


def test_airmass_invalid_latitude():
    """Test airmass computation with invalid latitude."""
    ra = (122.994945, 0.0)
    dec = (74.95304, 0.0)
    lst = (25407.072748, 0.0)
    exptime = (120.0, 0.0)
    latitude = (180.0, 0.0)  # Invalid latitude
    airmass_type = hdrlfunc.AirmassApprox.Hardie

    airmass = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, airmass_type)

    with pytest.raises(hdrlcore.IllegalInputError):
        airmass.compute()


def test_airmass_invalid_method():
    """Test airmass computation with invalid method."""
    ra = (122.994945, 0.0)
    dec = (74.95304, 0.0)
    lst = (25407.072748, 0.0)
    exptime = (120.0, 0.0)
    latitude = (37.2236, 0.0)

    with pytest.raises(AttributeError):
        airmass_type = hdrlfunc.AirmassApprox.SomeMethod  # Non-existent method

    airmass_type = 0  # Invalid method
    with pytest.raises(TypeError):
        hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, airmass_type)


def test_airmass_none_values():
    """Test airmass computation with None values (should default to 0,0)."""
    ra = None
    dec = None
    lst = None
    exptime = None
    latitude = None
    airmass_type = hdrlfunc.AirmassApprox.Hardie

    airmass = hdrlfunc.Airmass(ra, dec, lst, exptime, latitude, airmass_type)

    # None values should be converted to (0,0) and computation should succeed
    result = airmass.compute()
    assert hasattr(result, "data")
    assert hasattr(result, "error")
    # Allow tiny platform-dependent FP differences around zenith.
    assert result.data == pytest.approx(1.0)
    assert result.error == pytest.approx(0.0, abs=1e-12)


def test_airmass_approximation_enum():
    """Test that the AirmassApprox enum values are correct."""
    assert hdrlfunc.AirmassApprox.Hardie == 1
    assert hdrlfunc.AirmassApprox.YoungIrvine == 2
    assert hdrlfunc.AirmassApprox.Young == 3
