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

from pathlib import Path

import astropy.units as u
import cpl
import numpy as np
import pytest
from astropy.coordinates import EarthLocation, SkyCoord
from astropy.io import fits
from astropy.table import Table
from astropy.time import Time
from hdrl.func import Barycorr


# Mocking the EOP table
@pytest.fixture
def mock_eop_table():
    eop_data = {
        "MJD": np.array([58843.0, 58844.0, 58845.0, 58846.0]),
        "PMX": np.array([0.088559, 0.086875, 0.08503, 0.082854]),
        "PMY": np.array([0.280081, 0.280687, 0.281291, 0.281647]),
        "DUT": np.array([-0.1761465, -0.1762087, -0.1762508, -0.1763259]),
        "FLAG": np.array(["I", "I", "I", "I"]),
    }
    _test_tmpfile = Path("eop_table.fits").absolute()
    # Convert dictionary to Astropy Table
    eop_table = Table(eop_data)
    # Create an HDU (Header/Data Unit) from the table
    hdu = fits.BinTableHDU(eop_table)
    # Create a FITS file (optional: add a primary header)
    primary_hdu = fits.PrimaryHDU()
    hdulist = fits.HDUList([primary_hdu, hdu])
    # Write to file
    hdulist.writeto(_test_tmpfile, overwrite=True)
    eop = cpl.core.Table.load(str(_test_tmpfile), 1, False)
    yield eop
    _test_tmpfile.unlink(missing_ok=True)


def astropy_barycorr(ra, dec, mjd, lon, lat, height):
    coord = SkyCoord(ra=ra * u.deg, dec=dec * u.deg, frame="icrs")
    location = EarthLocation(lon=lon * u.deg, lat=lat * u.deg, height=height * u.m)
    time = Time(mjd, format="mjd", scale="utc")
    return coord.radial_velocity_correction("barycentric", obstime=time, location=location).to(u.m / u.s).value


def test_barycorr_class_instantiation():
    """Test that Barycorr object can be created."""
    b = Barycorr()
    assert b is not None
    assert isinstance(b, Barycorr)


@pytest.mark.parametrize(
    ("ra", "dec", "mjd", "lon", "lat", "elev"),
    [
        (120.0, 30.0, 59800.0, 10.0, 45.0, 100.0),
        (0.0, 0.0, 59000.0, -70.0, -30.0, 2500.0),
    ],
)
def test_barycorr_compute_vs_astropy(mock_eop_table, ra, dec, mjd, lon, lat, elev):
    v_cpp = Barycorr.compute(
        target=(ra, dec), observer=(lat, lon, elev), eop_table=mock_eop_table, mjd_obs=mjd, time_to_mid_exposure=0.0
    )
    v_astropy = astropy_barycorr(ra, dec, mjd, lon, lat, elev)
    assert np.isclose(v_cpp, v_astropy, atol=100.0), f"Mismatch: {v_cpp} vs {v_astropy}"


def test_barycorr_invalid_target_tuple(mock_eop_table):
    with pytest.raises(TypeError):
        Barycorr.compute(
            target=(999.0,),  # invalid
            observer=(0.0, 0.0, 0.0),
            eop_table=mock_eop_table,
            mjd_obs=60000.0,
            time_to_mid_exposure=0.0,
        )


def test_barycorr_invalid_observer_tuple(mock_eop_table):
    with pytest.raises(TypeError):
        Barycorr.compute(
            target=(180.0, -91.0),
            observer=(0.0, 0.0),  # invalid
            eop_table=mock_eop_table,
            mjd_obs=60000.0,
            time_to_mid_exposure=0.0,
        )


def test_missing_required_inputs(mock_eop_table):
    with pytest.raises(TypeError):  # missing mjd_obs
        Barycorr.compute(
            target=(180.0, 0.0), observer=(0.0, 0.0, 0.0), eop_table=mock_eop_table, time_to_mid_exposure=0.0
        )


def test_barycorr_null_eop_table():
    ra = 149.823138
    dec = -27.39211
    mjd_obs = 58844.22531243
    time_to_mid_exposure = 900.0
    lon = -70.4045
    lat = -24.6268
    elev = 2648.0

    with pytest.raises(ValueError):
        Barycorr.compute(
            target=(ra, dec),
            observer=(lat, lon, elev),
            eop_table=None,
            mjd_obs=mjd_obs,
            time_to_mid_exposure=time_to_mid_exposure,
        )
