# This file is part of PyCPL the ESO CPL Python language bindings
# Copyright (C) 2020-2024 European Southern Observatory
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

# Creates an example FITS file
# for use as a pytest fixture

# The example created should be the same as that
# which was in https://gitlab.eso.org/cpl/pycpl-experiments

from math import ceil
import os
from os import unlink, fsencode
from pathlib import Path
from random import randint
import sys

from astropy.io import fits
from numpy import random as np_random, repeat as np_repeat
from pytest import fixture
import pytest

from cpl import core


@fixture(scope="session")
def make_mock_image():
    def _make_mock_image(**kwargs):
        # hdulist = fits.open('/home/afarrell/Documents/Pipelines/pipedata/regression/giraffe/dfs/raw/giscience/GIRAF.2019-09-01T01:52:48.681.fits')
        # return hdulist[0].data

        # Start the base array with a bit of randomness
        data = np_random.rand(kwargs.get("height", 4096), kwargs.get("width", 2148))
        # Log/Power modifier
        distribution = kwargs.get("distribution", 5)
        # Minimum value of any band
        band_min = kwargs.get("min", 255)
        # Maximum value of the brightest band
        band_max = pow(kwargs.get("max", 31768), 1 / distribution)
        # How much a band varies when going along its length, relative to band's max brightness
        noise = kwargs.get("noise", 0.125)
        # Defines how wide bands are, and what how they're structured horizontally
        base_single = kwargs.get("base", [0, 0.6, 1, 0.6, 0])

        band_width = len(base_single)
        num_bands = ceil(data.shape[1] / band_width)

        bands_multipliers = np_random.rand(num_bands) * band_max

        # Repeat this into a 2D array of 1 row of repeated band bases
        # Creates [[0,0.6,1,0.6,0], [0,0.6,1,0.6,0], ..., [0,0.6,1,0.6,0]]
        band_data = np_repeat([base_single], num_bands, 0)
        # Turns into 1-row 2d-array of [0,0,6,1,0.6,0,0,0.6,1,0.6,0,...]
        band_data = band_data.reshape((1, band_data.size))
        # Truncate to the width of data
        band_data = band_data[:, : data.shape[1]]
        # Repeat to the size of data because bands are uniform along their height
        band_data = np_repeat(band_data, data.shape[0], 0)

        # Now to randomise the band definitions
        for x, multiplier in zip(
            range(0, data.shape[1], band_width), bands_multipliers
        ):
            my_band = band_data[:, x : (x + band_width)]
            my_band *= pow(band_max - multiplier, distribution) - band_min
            my_band += band_min

        data *= band_data
        data *= noise
        band_data -= data

        return band_data.astype(kwargs.get("dtype", band_data.dtype))

    return _make_mock_image


def hdulist_no_data():
    # Without a HDU in the HDU list,
    # astropy refuses to write anything, so one must be included.
    return fits.HDUList([fits.PrimaryHDU()])


def random_filename(tmp_path_factory):
    # Create mock 8-length filename with a mock integer
    # Put the newly created example fits in said file, then
    # Put in tmp_path_factory and return said filename
    tmp_dir = tmp_path_factory.mktemp(basename="fits-", numbered=True)
    tmp_file = tmp_dir.joinpath(str(randint(10000000, 99999999)) + ".fits")
    # The path should be absolute (resolve) for cpl to find it.
    return str(tmp_file.resolve())


@fixture(scope="session")
def make_mock_fits(request, tmp_path_factory):
    """
    Return a function that generates FITS files, returning string absolute filenames

    The returned function takes an optional parameter: 'existing_filename_or_hdulist',
    which Allows generating 2 files for the same data, or adding more than just blank data.

    The return of said function is a string, naming a generated file. The filename
    is a random name ending in .fits

    If the function is called with None, then a mock FITS file is created, with
    just enough data for astropy to actually write the file, and its filename is returned.

    If the function is called with a astropy.io.fits.HDUList, then that HDUList is
    put into the randomly named file.

    If the function is called with a path-like or string, said file is copied
    to the new randomly named file.

    Example::

        def test_foo(make_mock_fits):
            # Existing FITS files and their tags are required to be put in the SOF file
            my_fits_filename = make_mock_fits()

            from astropy.io import fits
            fits.open(my_fits_filename)
                ...

    """

    created_files = []

    def _make_mock_fits(existing_filename_or_hdulist=None):
        if existing_filename_or_hdulist is None:
            # Create a blank hdulist
            hdu_list = hdulist_no_data()
        elif isinstance(existing_filename_or_hdulist, fits.HDUList):
            # Use given HDUList data
            hdu_list = existing_filename_or_hdulist
        else:
            # Use data from an existing file
            hdu_list = fits.open(existing_filename_or_hdulist)

        filename = random_filename(tmp_path_factory)

        # New file created
        hdu_list.writeto(filename)
        created_files.append(filename)

        return filename

    yield _make_mock_fits

    # Teardown of this fixture:
    for filename in created_files:
        try:
            unlink(filename)
        except Exception:
            pass


@fixture(scope="session")
def make_mock_sof(request, tmp_path_factory):
    """
    Return a function that generates SOF files, returning string absolute filenames
    However, said function needs to be given the contents of said generated SOF file,
    I.e. a list of filenames of FITS files and tags for said files

    The input list format is of the form: array of [tuple of (string filename, string tag)]

    Example::

        def test_foo(make_mock_fits, make_mock_sof):
            # Existing FITS files and their tags are required to be put in the SOF file
            my_fits_file_1     = 'example1.fits'
            my_fits_file_1_tag = 'example1_tag'
            my_fits_file_2     = make_mock_fits()
            my_fits_file_2_tag = 'example2_tag'

            # Create the SOF file:
            filename = make_mock_sof([
                (my_fits_file1, my_fits_file_tag1)
                (my_fits_file2, my_fits_file_tag2)
            ])

            # Use the SOF file
            sof_file = open(filename, 'r')
                ...

    """

    cleanups = []

    def _make_mock_sof(fits_files=[]):
        """
        :ptype fits_files: list of (filename, tag)
        """
        sof_filename = random_filename(tmp_path_factory)
        sof_file = open(sof_filename, "wb+")

        # Use said fits_files argument:
        # Append to the SOF file each line describing the fits files
        for fits_filename, tag in fits_files:
            if hasattr(os, "PathLike") and isinstance(
                fits_filename, getattr(os, "PathLike")
            ):
                fits_filename = fits_filename.__fspath__()
            elif isinstance(fits_filename, Path):
                fits_filename = str(fits_filename)

            if isinstance(fits_filename, str):
                # Write as if the file was opened in 'w+' mode instead of 'wb+'
                sof_file.write(
                    fsencode(fits_filename)
                )  # Filenames are different encoding MAYBE?
            else:
                sof_file.write(fits_filename)

            sof_file.write(bytes(" " + tag + "\n", encoding=sys.getdefaultencoding()))
            sof_file.write(
                bytes("#Test comment \n", encoding=sys.getdefaultencoding())
            )  # Generate a comment that should be ignored

        # New file created
        sof_file.close()

        def sof_cleanup():
            try:
                unlink(sof_filename)
            except Exception:
                pass

        cleanups.append(sof_cleanup)
        return sof_filename

    yield _make_mock_sof

    # Teardown of this fixture:
    # since it might need to call teardown of make_mock_fixture
    # this will call any cleanup functions that accumulated
    for cleanup_func in cleanups:
        cleanup_func()


@pytest.fixture(scope="package")
def cpl_image_fill_test_create():
    """Generates an image for testing purposes, translated from cpl_image_gen.c."""

    def _cpl_image_fill_test_create(nx, ny):
        xposrate = [0.4, 0.5, 0.8, 0.9, 0.2, 0.5, 0.9, 0.3, 0.1, 0.7]
        yposrate = [0.4, 0.2, 0.3, 0.9, 0.7, 0.8, 0.9, 0.8, 0.7, 0.7]

        nbpoints = 10
        sigma = 10.0
        flux = 1000.0

        assert nx > 0
        assert ny > 0

        # Positions and flux for each fake star
        xpos = [nx * xp for xp in xposrate]
        ypos = [ny * yp for yp in yposrate]
        fluxes = [flux / (div + 1) for div in range(nbpoints)]

        # Set the background
        test_im = core.Image.create_noise_uniform(nx, ny, core.Type.DOUBLE, -1, 1)

        # Put fake stars
        for xp, yp, fl in zip(xpos, ypos, fluxes):
            tmp_im = core.Image.create_gaussian_like(
                test_im, xp, yp, 10.0, sigma, sigma
            )
            tmp_im.multiply_scalar(fl)
            test_im.add(tmp_im)

        return test_im

    return _cpl_image_fill_test_create
