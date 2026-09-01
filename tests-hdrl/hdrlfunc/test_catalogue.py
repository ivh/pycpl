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
Tests for the catalogue function.

This module tests the hdrl.func.Catalogue class based on the C unit tests.
"""

import math

import pytest
from cpl import core as cplcore
from hdrl import core as hdrlcore
from hdrl import func as hdrlfunc

# Exact dimensions from C test
NX = 1001
NY = 753


def create_test_image_with_objects():
    """Create a test image with multiple Gaussian objects, matching C test exactly."""

    # Create empty image
    image = cplcore.Image.zeros(NX, NY, cplcore.Type.DOUBLE)

    # Parameters from C test
    sigma = 2.0
    sky = 500.0
    norm2 = 2.0 * math.pi * sigma * sigma

    # Add scalar 100 to confidence map (equivalent to cpl_image_add_scalar(cnf, 100.))
    # Note: We don't create confidence map here as it's created separately in tests

    # Object positions and norms from C test
    xpos = [100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 950.0]
    ypos = [100.0, 200.0, 300.0, 400.0, 550.0, 600.0, 650.0, 700.0, 230.0, 170.0]
    norm = [1000.0, 100.0, 200.0, 500.0, 550.0, 600.0, 650.0, 700.0, 750.0, 800.0]
    nobj = len(xpos)

    # Add Gaussian objects (equivalent to C test loop)
    for i in range(nobj):
        # Create Gaussian at this position
        tmp_im = cplcore.Image.create_gaussian(
            NX, NY, cplcore.Type.DOUBLE, xpos[i], ypos[i], norm[i] * norm2, sigma, sigma
        )
        image.add(tmp_im)

    # Add noise and sky background (equivalent to C test)
    noise = cplcore.Image.create_noise_uniform(NX, NY, cplcore.Type.DOUBLE, -10.0, 10.0)
    noise.add_scalar(sky)
    image.add(noise)

    # Initialize the BPM and accept all pixels
    image.accept_all()

    return image, nobj


def create_flat_image():
    """Create a completely flat image with all pixels set to 1.0, matching C test."""
    flat_img = cplcore.Image.zeros(NX, NY, cplcore.Type.DOUBLE)
    flat_img.add_scalar(1.0)  # Equivalent to cpl_image_add_scalar(flat_img, 1.0)
    flat_img.accept_all()
    return flat_img


def create_peak_image():
    """Create an image with a single sharp Gaussian peak, matching C test."""
    peak_img = cplcore.Image.zeros(NX, NY, cplcore.Type.DOUBLE)

    # Fill Gaussian peak at (100, 100) with amplitude 1000.0, sigma 1.0
    # Equivalent to cpl_image_fill_gaussian(peak_img, 100, 100, 1000.0, 1.0, 1.0)
    tmp_im = cplcore.Image.create_gaussian(NX, NY, cplcore.Type.DOUBLE, 100, 100, 1000.0, 1.0, 1.0)
    peak_img.add(tmp_im)

    # Add scalar 1.0 (equivalent to cpl_image_add_scalar(peak_img, 1.0))
    peak_img.add_scalar(1.0)

    peak_img.accept_all()
    return peak_img


def create_test_wcs():
    """Create a test WCS object following the C test pattern exactly."""
    from cpl import drs as cpldrs

    # Create property list with exact values from C test
    pl = cplcore.PropertyList()

    # String properties (CTYPE) - exact values from C test
    pl.append(cplcore.Property("CTYPE1", cplcore.Type.STRING, "RA---TAN"))
    pl.append(cplcore.Property("CTYPE2", cplcore.Type.STRING, "DEC--TAN"))

    # Double properties (CRVAL, CRPIX, CD matrix) - exact values from C test
    pl.append(cplcore.Property("CRVAL1", cplcore.Type.DOUBLE, 30.0))
    pl.append(cplcore.Property("CRVAL2", cplcore.Type.DOUBLE, 12.0))
    pl.append(cplcore.Property("CRPIX1", cplcore.Type.DOUBLE, 512.0))
    pl.append(cplcore.Property("CRPIX2", cplcore.Type.DOUBLE, 512.0))
    pl.append(cplcore.Property("CD1_1", cplcore.Type.DOUBLE, -1.0 / 3600))
    pl.append(cplcore.Property("CD1_2", cplcore.Type.DOUBLE, 0.0))
    pl.append(cplcore.Property("CD2_1", cplcore.Type.DOUBLE, 0.0))
    pl.append(cplcore.Property("CD2_2", cplcore.Type.DOUBLE, 1.0 / 3600))

    # Integer properties (NAXIS) - exact dimensions from C test
    pl.append(cplcore.Property("NAXIS1", cplcore.Type.INT, NX))
    pl.append(cplcore.Property("NAXIS2", cplcore.Type.INT, NY))

    # Create WCS from property list
    return cpldrs.WCS(pl)


def create_test_confidence_map():
    """Create a test confidence map following the C test pattern exactly."""

    # Create empty image with CPL_TYPE_INT (equivalent to cpl_image_new(NX, NY, CPL_TYPE_INT))
    confidence_map = cplcore.Image.zeros(NX, NY, cplcore.Type.INT)

    # Initialize the BPM and accept all pixels
    confidence_map.accept_all()

    # Add scalar 100.0 (equivalent to cpl_image_add_scalar(cnf, 100.))
    confidence_map.add_scalar(100.0)

    return confidence_map


def create_test_confidence_map_no_bpm():
    """Create a test confidence map without BPM to test if that's the issue."""

    # Create empty image with CPL_TYPE_INT (equivalent to cpl_image_new(NX, NY, CPL_TYPE_INT))
    confidence_map = cplcore.Image.zeros(NX, NY, cplcore.Type.INT)

    # Don't call accept_all() - leave BPM as default
    # Add scalar 100.0 (equivalent to cpl_image_add_scalar(cnf, 100.))
    confidence_map.add_scalar(100.0)

    return confidence_map


def test_null_input_errors():
    """Test null input error handling, matching C test exactly."""
    image, _ = create_test_image_with_objects()
    wcs = create_test_wcs()

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=3,
        obj_threshold=2.5,
        obj_deblending=False,
        obj_core_radius=3.0,
        bkg_estimate=True,
        bkg_mesh_size=64,
        bkg_smooth_fwhm=2.0,
        det_eff_gain=2.0,
        det_saturation=65000.0,
        resulttype=7,  # HDRL_CATALOGUE_ALL
    )

    # Test all combinations of null inputs (equivalent to C test nested loops)
    # Test with None image
    with pytest.raises(hdrlcore.NullInputError):
        catalogue.compute(None)

    # Test with None wcs (should work)
    result = catalogue.compute(image, confidence_map=None, wcs=None)
    assert result is not None

    # Test with wcs (should work)
    result = catalogue.compute(image, confidence_map=None, wcs=wcs)
    assert result is not None

    # Test with confidence map using new pycpl_image type
    confidence_map = create_test_confidence_map()
    result = catalogue.compute(image, confidence_map=confidence_map, wcs=None)
    assert result is not None


def test_basic_object_detection():
    """Test basic object detection and results, matching C test exactly."""
    image, expected_objects = create_test_image_with_objects()

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=5,  # Exact value from C test
        obj_threshold=1.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=5.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=3.0,  # Exact value from C test
        det_eff_gain=1.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=7,  # HDRL_CATALOGUE_ALL
    )

    result = catalogue.compute(image, confidence_map=None, wcs=None)

    # Check that result is not None
    assert result is not None

    # Check that catalogue table exists and has expected number of objects
    assert result.catalogue is not None
    assert len(result.catalogue) == expected_objects  # Exact number from C test

    # Check that segmentation map exists
    assert result.segmentation_map is not None
    assert result.segmentation_map.get_max() == expected_objects  # Exact check from C test
    assert result.segmentation_map.get_min() == 0  # Exact check from C test

    # Check that background exists
    assert result.background is not None
    # Check background mean is close to sky value (500.0 ± 5)
    assert abs(result.background.get_mean() - 500.0) < 5  # Exact check from C test

    # Check that QC list exists
    assert result.qclist is not None


def test_no_background_subtraction():
    """Test catalogue computation without background subtraction, matching C test exactly."""
    image, expected_objects = create_test_image_with_objects()

    # Create image with background already subtracted (equivalent to C test)
    sky = 500.0  # Exact value from C test
    image_corrected = image.duplicate()  # Create a copy
    image_corrected.subtract_scalar(sky)  # Subtract the sky background

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=5,  # Exact value from C test
        obj_threshold=1.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=5.0,  # Exact value from C test
        bkg_estimate=False,  # CPL_FALSE - don't estimate background
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=3.0,  # Exact value from C test
        det_eff_gain=1.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=7,  # HDRL_CATALOGUE_ALL
    )

    result = catalogue.compute(image_corrected, confidence_map=None, wcs=None)

    assert result is not None
    assert result.catalogue is not None
    assert len(result.catalogue) == expected_objects  # Exact number from C test
    assert result.segmentation_map is not None
    assert result.segmentation_map.get_max() == expected_objects  # Exact check from C test
    assert result.segmentation_map.get_min() == 0  # Exact check from C test
    assert result.background is None  # Should be None when bkg_estimate=False
    assert result.qclist is not None


def test_bad_confidence_map():
    """Test with bad confidence map (negative values), matching C test exactly."""
    image, _ = create_test_image_with_objects()
    confidence_map = create_test_confidence_map()

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=3,  # Exact value from C test
        obj_threshold=2.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=3.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=2.0,  # Exact value from C test
        det_eff_gain=2.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=7,  # HDRL_CATALOGUE_ALL
    )

    # Test with good confidence map first (should work)
    result = catalogue.compute(image, confidence_map=confidence_map)
    assert result is not None

    # Manipulate confidence map like in C test: subtract 200 (creates negative values)
    # Equivalent to cpl_image_subtract_scalar(cnf, 200)
    confidence_map.subtract_scalar(200.0)

    # This should raise an error due to negative values in confidence map
    # Equivalent to CPL_ERROR_INCOMPATIBLE_INPUT
    with pytest.raises(hdrlcore.IncompatibleInputError):
        catalogue.compute(image, confidence_map=confidence_map)

    # Restore confidence map like in C test: add back 200
    # Equivalent to cpl_image_add_scalar(cnf, 200)
    confidence_map.add_scalar(200.0)

    # Test that confidence map is restored and works again
    result = catalogue.compute(image, confidence_map=confidence_map)
    assert result is not None


def test_double_confidence_map():
    """Test with double precision confidence map, matching C test exactly."""
    image, _ = create_test_image_with_objects()
    confidence_map = create_test_confidence_map()

    # Cast confidence map to double (equivalent to cpl_image_cast(cnf, CPL_TYPE_DOUBLE))
    double_confidence_map = confidence_map.cast(cplcore.Type.DOUBLE)

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=3,  # Exact value from C test
        obj_threshold=2.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=3.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=2.0,  # Exact value from C test
        det_eff_gain=2.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=7,  # HDRL_CATALOGUE_ALL
    )

    result = catalogue.compute(image, confidence_map=double_confidence_map)

    assert result is not None
    assert result.catalogue is not None
    assert result.segmentation_map is not None
    assert result.background is not None


def test_image_with_bad_pixels():
    """Test image with bad pixels, matching C test exactly."""
    image, _ = create_test_image_with_objects()

    # Reject some pixels (equivalent to cpl_image_reject(img, 60, 23))
    image.reject(60, 23)  # Exact coordinates from C test

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=3,  # Exact value from C test
        obj_threshold=2.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=3.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=2.0,  # Exact value from C test
        det_eff_gain=2.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=7,  # HDRL_CATALOGUE_ALL
    )

    result = catalogue.compute(image, confidence_map=None, wcs=None)

    assert result is not None


def test_image_with_bad_pixels_and_confidence():
    """Test image with bad pixels and confidence map, matching C test exactly."""
    image, _ = create_test_image_with_objects()
    confidence_map = create_test_confidence_map()

    # Reject some pixels (equivalent to cpl_image_reject(img, 60, 23))
    image.reject(60, 23)  # Exact coordinates from C test

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=3,  # Exact value from C test
        obj_threshold=2.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=3.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=2.0,  # Exact value from C test
        det_eff_gain=2.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=7,  # HDRL_CATALOGUE_ALL
    )

    result = catalogue.compute(image, confidence_map=confidence_map)

    assert result is not None
    assert result.catalogue is not None
    assert result.segmentation_map is not None
    assert result.background is not None


def test_image_with_bad_pixels_and_double_confidence():
    """Test image with bad pixels and double confidence map, matching C test exactly."""
    image, _ = create_test_image_with_objects()
    confidence_map = create_test_confidence_map()

    # Cast confidence map to double (equivalent to cpl_image_cast(cnf, CPL_TYPE_DOUBLE))
    double_confidence_map = confidence_map.cast(cplcore.Type.DOUBLE)

    # Reject some pixels (equivalent to cpl_image_reject(img, 60, 23))
    image.reject(60, 23)  # Exact coordinates from C test

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=3,  # Exact value from C test
        obj_threshold=2.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=3.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=2.0,  # Exact value from C test
        det_eff_gain=2.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=7,  # HDRL_CATALOGUE_ALL
    )

    result = catalogue.compute(image, confidence_map=double_confidence_map)

    assert result is not None
    assert result.catalogue is not None
    assert result.segmentation_map is not None
    assert result.background is not None


def test_image_and_confidence():
    """Test image with confidence map, matching C test exactly."""
    image, _ = create_test_image_with_objects()
    confidence_map = create_test_confidence_map()

    # Accept all pixels (equivalent to cpl_image_accept_all(img))
    image.accept_all()

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=3,  # Exact value from C test
        obj_threshold=2.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=3.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=2.0,  # Exact value from C test
        det_eff_gain=2.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=7,  # HDRL_CATALOGUE_ALL
    )

    result = catalogue.compute(image, confidence_map=confidence_map)

    assert result is not None


def test_image_confidence_and_wcs():
    """Test image with confidence map and WCS, matching C test exactly."""
    image, _ = create_test_image_with_objects()
    confidence_map = create_test_confidence_map()
    wcs = create_test_wcs()

    # Accept all pixels (equivalent to cpl_image_accept_all(img))
    image.accept_all()

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=3,  # Exact value from C test
        obj_threshold=2.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=3.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=2.0,  # Exact value from C test
        det_eff_gain=2.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=7,  # HDRL_CATALOGUE_ALL
    )

    result = catalogue.compute(image, confidence_map=confidence_map, wcs=wcs)

    assert result is not None
    assert result.catalogue is not None
    assert result.segmentation_map is not None
    assert result.background is not None


def test_double_image_confidence_and_wcs():
    """Test double precision image with confidence map and WCS, matching C test exactly."""
    image, _ = create_test_image_with_objects()
    confidence_map = create_test_confidence_map()
    wcs = create_test_wcs()

    # Cast image to double (equivalent to cpl_image_cast(img, CPL_TYPE_DOUBLE))
    double_image = image.cast(cplcore.Type.DOUBLE)

    # Accept all pixels (equivalent to cpl_image_accept_all(img))
    double_image.accept_all()

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=3,  # Exact value from C test
        obj_threshold=2.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=3.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=2.0,  # Exact value from C test
        det_eff_gain=2.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=7,  # HDRL_CATALOGUE_ALL
    )

    result = catalogue.compute(double_image, confidence_map=confidence_map, wcs=wcs)

    assert result is not None
    assert result.catalogue is not None
    assert result.segmentation_map is not None
    assert result.background is not None


def test_no_segmap_and_background():
    """Test with only catalogue output (no segmentation map and background), matching C test exactly."""
    image, _ = create_test_image_with_objects()
    confidence_map = create_test_confidence_map()
    wcs = create_test_wcs()

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=5,  # Exact value from C test
        obj_threshold=1.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=5.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=3.0,  # Exact value from C test
        det_eff_gain=1.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=4,  # HDRL_CATALOGUE_CAT_COMPLETE
    )

    result = catalogue.compute(image, confidence_map=confidence_map, wcs=wcs)

    assert result is not None
    assert result.catalogue is not None
    assert result.segmentation_map is None  # Should be None
    assert result.background is None  # Should be None


def test_no_segmap():
    """Test with catalogue and background output (no segmentation map), matching C test exactly."""
    image, _ = create_test_image_with_objects()
    confidence_map = create_test_confidence_map()
    wcs = create_test_wcs()

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=5,  # Exact value from C test
        obj_threshold=1.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=5.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=3.0,  # Exact value from C test
        det_eff_gain=1.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=5,  # HDRL_CATALOGUE_CAT_COMPLETE | HDRL_CATALOGUE_BKG
    )

    result = catalogue.compute(image, confidence_map=confidence_map, wcs=wcs)

    assert result is not None
    assert result.catalogue is not None
    assert result.segmentation_map is None  # Should be None
    assert result.background is not None


def test_no_background():
    """Test with catalogue and segmentation map output (no background), matching C test exactly."""
    image, _ = create_test_image_with_objects()
    confidence_map = create_test_confidence_map()
    wcs = create_test_wcs()

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=5,  # Exact value from C test
        obj_threshold=1.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=5.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=3.0,  # Exact value from C test
        det_eff_gain=1.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=6,  # HDRL_CATALOGUE_CAT_COMPLETE | HDRL_CATALOGUE_SEGMAP
    )

    result = catalogue.compute(image, confidence_map=confidence_map, wcs=wcs)

    assert result is not None
    assert result.catalogue is not None
    assert result.segmentation_map is not None
    assert result.background is None  # Should be None


def test_no_catalogue():
    """Test with segmentation map and background output (no catalogue), matching C test exactly."""
    image, _ = create_test_image_with_objects()
    confidence_map = create_test_confidence_map()
    wcs = create_test_wcs()

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=5,  # Exact value from C test
        obj_threshold=1.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=5.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=3.0,  # Exact value from C test
        det_eff_gain=1.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=3,  # HDRL_CATALOGUE_SEGMAP | HDRL_CATALOGUE_BKG
    )

    result = catalogue.compute(image, confidence_map=confidence_map, wcs=wcs)

    assert result is not None
    assert result.catalogue is None  # Should be None when resulttype=3 (no catalogue requested)
    assert result.segmentation_map is not None
    assert result.background is not None


def test_flat_image():
    """Test pathological case with a completely flat image, matching C test exactly."""
    flat_image = create_flat_image()

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=5,  # Exact value from C test
        obj_threshold=1.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=5.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=3.0,  # Exact value from C test
        det_eff_gain=1.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=7,  # HDRL_CATALOGUE_CAT_COMPLETE | HDRL_CATALOGUE_SEGMAP | HDRL_CATALOGUE_BKG
    )

    # This should raise an error due to no objects found (equivalent to CPL_ERROR_DATA_NOT_FOUND)
    with pytest.raises(hdrlcore.DataNotFoundError):
        catalogue.compute(flat_image, confidence_map=None, wcs=None)


def test_peak_image():
    """Test pathological case with a single sharp peak in the image, matching C test exactly."""
    peak_image = create_peak_image()

    catalogue = hdrlfunc.Catalogue(
        obj_min_pixels=5,  # Exact value from C test
        obj_threshold=1.5,  # Exact value from C test
        obj_deblending=False,  # CPL_FALSE
        obj_core_radius=5.0,  # Exact value from C test
        bkg_estimate=True,  # CPL_TRUE
        bkg_mesh_size=64,  # Exact value from C test
        bkg_smooth_fwhm=3.0,  # Exact value from C test
        det_eff_gain=1.0,  # Exact value from C test
        det_saturation=65000.0,  # HDRL_SATURATION_INIT
        resulttype=7,  # HDRL_CATALOGUE_CAT_COMPLETE | HDRL_CATALOGUE_SEGMAP | HDRL_CATALOGUE_BKG
    )

    result = catalogue.compute(peak_image, confidence_map=None, wcs=None)

    assert result is not None
    assert result.catalogue is not None
    assert result.segmentation_map is not None
    assert result.background is not None
