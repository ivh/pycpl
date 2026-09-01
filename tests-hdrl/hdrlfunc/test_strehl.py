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

import cpl.core as cplcore
import numpy as np
import pytest
from hdrl import core as hdrlcore
from hdrl import func as hdrlfunc


def _create_psf_image(nx, ny, xcen, ycen, lam=7.7e-6, m1=4.15, m2=0.55, pixscale=0.03):
    """Creates an hdrl.core.Image with an Airy disk PSF matching the C implementation."""
    from scipy.special import j1

    data_array = np.zeros((ny, nx))
    e = m2 / m1
    as_2_rad = 2 * np.pi / (360.0 * 3600)  # arcsec to radians

    # Convert center coordinates to physical units (like C code)
    centerx = (-(nx / 2.0) + xcen - 1 + 0.5) * pixscale
    centery = (-(ny / 2.0) + ycen - 1 + 0.5) * pixscale
    xhigh = ((nx - 1) * pixscale / 2) - centerx
    yhigh = ((ny - 1) * pixscale / 2) - centery
    xlow = -((nx - 1) * pixscale / 2) - centerx
    ylow = -((ny - 1) * pixscale / 2) - centery
    step_x = (xhigh - xlow) / (nx - 1) if nx > 1 else 0
    step_y = (yhigh - ylow) / (ny - 1) if ny > 1 else 0

    for iy in range(ny):
        y = yhigh if iy == ny - 1 else ylow + iy * step_y
        for ix in range(nx):
            x = xhigh if ix == nx - 1 else xlow + ix * step_x
            r = np.sqrt(x * x + y * y) * as_2_rad * 2 * np.pi * m1 / lam
            if r == 0.0:
                data_array[iy, ix] = 1.0
            else:
                airy = 2 * j1(r) / r - 2 * e * j1(e * r) / r
                c = 1 - e * e
                data_array[iy, ix] = 1 / (c * c) * airy * airy

    cpl_data = cplcore.Image(data_array.ravel().astype(np.float64), width=nx)
    cpl_errors = cplcore.Image.zeros(nx, ny, dtype=cplcore.Type.DOUBLE)

    return hdrlcore.Image(cpl_data, cpl_errors)


@pytest.fixture
def setup_strehl_test_data():
    """Pytest fixture to set up common data for Strehl tests."""
    nx, ny = 256, 256
    wavelength = 7.7e-6
    m1 = 8.3 / 2
    m2 = 1.1 / 2
    pixel_scale_x = 0.03
    pixel_scale_y = 0.03
    flux_radius = 1.5
    bkg_radius_low = 1.5
    bkg_radius_high = 2.0

    # Create a synthetic PSF image centered in the frame using proper Airy disk
    himg = _create_psf_image(nx, ny, nx / 2, ny / 2, wavelength, m1, m2, pixel_scale_x)

    # Add an unmasked, high-value pixel away from the center
    # Note: set_pixel uses (y, x) coordinates in pyhdrl
    himg.set_pixel(231, 28, (1.5, 1.5))

    params = {
        "nx": nx,
        "ny": ny,
        "wavelength": wavelength,
        "m1": m1,
        "m2": m2,
        "pixel_scale_x": pixel_scale_x,
        "pixel_scale_y": pixel_scale_y,
        "flux_radius": flux_radius,
        "bkg_radius_low": bkg_radius_low,
        "bkg_radius_high": bkg_radius_high,
    }
    return himg, params


def test_strehl_class_instantiation():
    wavelength = 1.635e-6
    s = hdrlfunc.Strehl(
        wavelength=wavelength,
        m1=5.08 / 2,
        m2=5.08 / 2 * 0.36,
        pixel_scale_x=0.0331932 / 2.0,
        pixel_scale_y=0.0331932 / 2.0,
        flux_radius=1.5,
        bkg_radius_low=1.5,
        bkg_radius_high=2.0,
    )

    assert s.wavelength == wavelength
    strehl_args = (1.635e-6, 5.08 / 2, 5.08 / 2 * 0.36, 0.0331932 / 2.0, 0.0331932 / 2.0, 1.5, 1.5, 2.0)
    s = hdrlfunc.Strehl(*strehl_args)
    assert s.wavelength == wavelength


def test_strehl_basic_call(setup_strehl_test_data):
    himg, params = setup_strehl_test_data

    strehl_args = (
        params["wavelength"],
        params["m1"],
        params["m2"],
        params["pixel_scale_x"],
        params["pixel_scale_y"],
        params["flux_radius"],
        params["bkg_radius_low"],
        params["bkg_radius_high"],
    )

    strehl = hdrlfunc.Strehl(*strehl_args)
    assert strehl is not None

    # Verify that these scalar values are simple copies of the input
    assert strehl.wavelength == params["wavelength"]
    assert strehl.m1 == params["m1"]
    assert strehl.m2 == params["m2"]
    assert strehl.pixel_scale_x == params["pixel_scale_x"]
    assert strehl.pixel_scale_y == params["pixel_scale_y"]
    assert strehl.flux_radius == params["flux_radius"]
    assert strehl.bkg_radius_low == params["bkg_radius_low"]
    assert strehl.bkg_radius_high == params["bkg_radius_high"]

    result = strehl.compute(himg)

    assert result is not None

    assert result.strehl_value == pytest.approx(1.0, abs=0.02)
    assert result.strehl_error == pytest.approx(0.0, abs=1e-09)
    assert result.star_x == pytest.approx(params["nx"] / 2, abs=0.015)
    assert result.star_y == pytest.approx(params["ny"] / 2, abs=0.015)
    assert result.star_peak == pytest.approx(1.0, abs=1e-04)
    assert result.star_peak_error == pytest.approx(0.0, abs=1e-09)
    assert result.star_flux == pytest.approx(50.0, abs=1.0)
    assert result.star_flux_error == pytest.approx(0.0, abs=1e-09)
    assert result.star_background == pytest.approx(4.6e-05, abs=1e-07)
    assert result.star_background_error == pytest.approx(0.0, abs=1e-09)
    assert result.computed_background_error == pytest.approx(6e-07, abs=1e-07)
    assert result.nbackground_pixels == 6120


def test_strehl_repeated_compute_reuses_same_instance(setup_strehl_test_data):
    """Regression test for safe repeated compute calls on same Strehl object."""
    himg, params = setup_strehl_test_data

    strehl = hdrlfunc.Strehl(
        params["wavelength"],
        params["m1"],
        params["m2"],
        params["pixel_scale_x"],
        params["pixel_scale_y"],
        params["flux_radius"],
        params["bkg_radius_low"],
        params["bkg_radius_high"],
    )

    for _ in range(10):
        result = strehl.compute(himg)
        assert result.strehl_value == pytest.approx(1.0, abs=0.02)
        assert result.star_x == pytest.approx(params["nx"] / 2, abs=0.015)
        assert result.star_y == pytest.approx(params["ny"] / 2, abs=0.015)


def test_strehl_with_bkg(setup_strehl_test_data):
    himg, params = setup_strehl_test_data

    nx, ny = 256, 256
    # background slope
    bkg = np.zeros([nx, ny])
    slope = 1.0 / nx * 100
    for i in range(nx):
        for j in range(ny):
            bkg[j][i] += i * slope

    cpl_data = cplcore.Image(bkg.ravel().astype(np.float64), width=nx)
    cpl_errors = cplcore.Image.zeros(nx, ny, dtype=cplcore.Type.DOUBLE)

    bimg = hdrlcore.Image(cpl_data, cpl_errors)
    # psf with high S/N
    himg.mul_scalar((2000.0, 0.0))
    himg.add_image(bimg)

    strehl_args = (
        params["wavelength"],
        params["m1"],
        params["m2"],
        params["pixel_scale_x"],
        params["pixel_scale_y"],
        0.5,
        2.5,
        3.0,
    )

    strehl = hdrlfunc.Strehl(*strehl_args)
    result = strehl.compute(himg)
    assert result is not None
    assert result.strehl_value == pytest.approx(1.0, abs=0.015)


def test_strehl_fail_on_empty_image(setup_strehl_test_data):
    """Tests failure when the image is empty (failing fit)."""
    _, params = setup_strehl_test_data
    empty_image = hdrlcore.Image.zeros(params["nx"], params["ny"])

    strehl_args = (
        params["wavelength"],
        params["m1"],
        params["m2"],
        params["pixel_scale_x"],
        params["pixel_scale_y"],
        params["flux_radius"],
        params["bkg_radius_low"],
        params["bkg_radius_high"],
    )

    strehl = hdrlfunc.Strehl(*strehl_args)
    assert strehl is not None

    # Empty image should fail to find a star
    with pytest.raises(hdrlcore.DataNotFoundError):
        strehl.compute(empty_image)


def test_strehl_fail_on_none_image(setup_strehl_test_data):
    """Tests failure when the image is None."""
    _, params = setup_strehl_test_data

    strehl_args = (
        params["wavelength"],
        params["m1"],
        params["m2"],
        params["pixel_scale_x"],
        params["pixel_scale_y"],
        params["flux_radius"],
        params["bkg_radius_low"],
        params["bkg_radius_high"],
    )

    strehl = hdrlfunc.Strehl(*strehl_args)
    assert strehl is not None

    with pytest.raises(hdrlcore.InvalidTypeError):
        strehl.compute(None)


def test_strehl_illegal_input(setup_strehl_test_data):
    himg, _ = setup_strehl_test_data

    invalid_args = [
        (-1, 5.08 / 2, 5.08 / 2 * 0.36, 0.0331932 / 2.0, 0.0331932 / 2.0, 1.5, 1.5, 2.0),
        (1.635e-6, -1, 5.08 / 2 * 0.36, 0.0331932 / 2.0, 0.0331932 / 2.0, 1.5, 1.5, 2.0),
        (1.635e-6, 5.08 / 2, -1, 0.0331932 / 2.0, 0.0331932 / 2.0, 1.5, 1.5, 2.0),
        (1.635e-6, 5.08 / 2, 5.08 / 2 * 0.36, -1, 0.0331932 / 2.0, 1.5, 1.5, 2.0),
        (1.635e-6, 5.08 / 2, 5.08 / 2 * 0.36, 0.0331932 / 2.0, -1, 1.5, 1.5, 2.0),
        (1.635e-6, 5.08 / 2, 5.08 / 2 * 0.36, 0.0331932 / 2.0, 0.0331932 / 2.0, -1, 1.5, 2.0),
        (1.635e-6, 5.08 / 2, 5.08 / 2 * 0.36, 0.0331932 / 2.0, 0.0331932 / 2.0, 1.5, -1, 2.0),
        (1.635e-6, 5.08 / 2, 5.08 / 2 * 0.36, 0.0331932 / 2.0, 0.0331932 / 2.0, 1.5, 1.5, -1),
        (1.635e-6, 5.08 / 2, 5.08, 0.0331932 / 2.0, 0.0331932 / 2.0, 1.5, 1.5, 2.0),
    ]

    for strehl_args in invalid_args:
        try:
            strehl = hdrlfunc.Strehl(*strehl_args)
        except hdrlcore.IllegalInputError:
            # Some platforms validate already at parameter creation time.
            continue

        # Other platforms defer validation to the actual compute call.
        with pytest.raises(hdrlcore.IllegalInputError):
            strehl.compute(himg)
