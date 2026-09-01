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

import math

import pytest
from cpl import core as cplcore
from hdrl import core as hdrlcore
from hdrl import func as hdrlfunc


def create_annular_image():
    """Create a 9x9 image with annular regions as in the C test."""
    image = cplcore.Image.zeros(9, 9, cplcore.Type.DOUBLE)
    image.add_scalar(60)
    # Convert from 1-based C indexing to 0-based Python indexing
    # C: cpl_image_fill_window(image, 2, 2, 8, 8, 70) -> Python: (1, 1, 7, 7)
    image.fill_window((1, 1, 7, 7), 70)
    # C: cpl_image_fill_window(image, 3, 3, 7, 7, 80) -> Python: (2, 2, 6, 6)
    image.fill_window((2, 2, 6, 6), 80)
    # C: cpl_image_fill_window(image, 4, 4, 6, 6, 90) -> Python: (3, 3, 5, 5)
    image.fill_window((3, 3, 5, 5), 90)
    # C: cpl_image_fill_window(image, 5, 5, 5, 5, 100) -> Python: (4, 4, 4, 4)
    image.fill_window((4, 4, 4, 4), 100)
    return image


def create_maglim_test_params():
    """Create common test parameters for maglim tests."""
    # Create a mode parameter (collapse mode, as in the C test)
    # These are intentionally set to 0 as the algorithm auto-sets them
    histo_min = 0.0
    histo_max = 0.0
    bin_size = 0.0
    error_niter = 0
    mode_method = hdrlfunc.Collapse.Method.Median
    mode_param = hdrlfunc.Collapse.Mode(histo_min, histo_max, bin_size, mode_method, error_niter)

    return {
        "zeropoint": 0.0,
        "fwhm_seeing": math.sqrt(4 * math.log(4)),
        "kernel_sx": 9,
        "kernel_sy": 9,
        "method": hdrlfunc.Maglim.ImageExtendMethod.Mirror,
        "mode_param": mode_param,
    }


def test_debug_image():
    """Debug test to see what the image looks like after indexing fix."""
    image = create_annular_image()

    # Print the image data to see what we have
    print("Image data:")
    for y in range(9):
        row = []
        for x in range(9):
            # Use the correct method to get pixel values
            val = image.get_pixel(y, x)
            row.append(f"{val:.1f}")
        print(" ".join(row))

    # Also check min/max values
    min_val = float("inf")
    max_val = float("-inf")
    for y in range(9):
        for x in range(9):
            val = image.get_pixel(y, x)
            min_val = min(min_val, val)
            max_val = max(max_val, val)

    print(f"Min value: {min_val}")
    print(f"Max value: {max_val}")


def test_maglim_compute_negative_fwhm():
    """Test that Maglim.compute() raises IllegalInputError for negative
    FWHM.
    """
    params = create_maglim_test_params()
    image = create_annular_image()

    # Create a Maglim object with negative FWHM
    maglim = hdrlfunc.Maglim(
        params["zeropoint"], -1, params["kernel_sx"], params["kernel_sy"], params["method"], params["mode_param"]
    )

    # Test that compute() raises IllegalInputError
    with pytest.raises(hdrlcore.IllegalInputError):
        maglim.compute(image)


def test_maglim_compute_zero_fwhm():
    """Test that Maglim.compute() raises IllegalInputError for zero
    FWHM.
    """
    params = create_maglim_test_params()
    image = create_annular_image()

    # Create a Maglim object with zero FWHM
    maglim = hdrlfunc.Maglim(
        params["zeropoint"], 0, params["kernel_sx"], params["kernel_sy"], params["method"], params["mode_param"]
    )

    # Test that compute() raises IllegalInputError
    with pytest.raises(hdrlcore.IllegalInputError):
        maglim.compute(image)


def test_maglim_compute_negative_kernel_size_x():
    """Test that Maglim.compute() raises IllegalInputError for negative
    kernel size X.
    """
    params = create_maglim_test_params()
    image = create_annular_image()

    # Create a Maglim object with negative kernel size X
    maglim = hdrlfunc.Maglim(
        params["zeropoint"], params["fwhm_seeing"], -1, params["kernel_sy"], params["method"], params["mode_param"]
    )

    # Test that compute() raises IllegalInputError
    with pytest.raises(hdrlcore.IllegalInputError):
        maglim.compute(image)


def test_maglim_compute_zero_kernel_size_x():
    """Test that Maglim.compute() raises IllegalInputError for zero
    kernel size X.
    """
    params = create_maglim_test_params()
    image = create_annular_image()

    # Create a Maglim object with zero kernel size X
    maglim = hdrlfunc.Maglim(
        params["zeropoint"], params["fwhm_seeing"], 0, params["kernel_sy"], params["method"], params["mode_param"]
    )

    # Test that compute() raises IllegalInputError
    with pytest.raises(hdrlcore.IllegalInputError):
        maglim.compute(image)


def test_maglim_compute_negative_kernel_size_y():
    """Test that Maglim.compute() raises IllegalInputError for negative
    kernel size Y.
    """
    params = create_maglim_test_params()
    image = create_annular_image()

    # Create a Maglim object with negative kernel size Y
    maglim = hdrlfunc.Maglim(
        params["zeropoint"], params["fwhm_seeing"], params["kernel_sx"], -1, params["method"], params["mode_param"]
    )

    # Test that compute() raises IllegalInputError
    with pytest.raises(hdrlcore.IllegalInputError):
        maglim.compute(image)


def test_maglim_compute_zero_kernel_size_y():
    """Test that Maglim.compute() raises IllegalInputError for zero
    kernel size Y.
    """
    params = create_maglim_test_params()
    image = create_annular_image()

    # Create a Maglim object with zero kernel size Y
    maglim = hdrlfunc.Maglim(
        params["zeropoint"], params["fwhm_seeing"], params["kernel_sx"], 0, params["method"], params["mode_param"]
    )

    # Test that compute() raises IllegalInputError
    with pytest.raises(hdrlcore.IllegalInputError):
        maglim.compute(image)


def test_maglim_compute_null_image():
    """Test that passing None as image raises NullInputError."""
    params = create_maglim_test_params()

    # Create a Maglim object
    maglim = hdrlfunc.Maglim(
        params["zeropoint"],
        params["fwhm_seeing"],
        params["kernel_sx"],
        params["kernel_sy"],
        params["method"],
        params["mode_param"],
    )

    # Test invalid input (None image)
    with pytest.raises(hdrlcore.NullInputError):
        maglim.compute(None)


def test_maglim_compute_invalid_method():
    """Test that passing invalid extend_method raises TypeError.

    Note: This test expects TypeError because pybind11's enum type checking
    will catch invalid enum values before the C++ function is called.
    """
    params = create_maglim_test_params()

    # Test invalid input (invalid extend_method)
    with pytest.raises(TypeError):
        hdrlfunc.Maglim(
            params["zeropoint"],
            params["fwhm_seeing"],
            params["kernel_sx"],
            params["kernel_sy"],
            -1,
            params["mode_param"],
        )


def test_maglim_compute_null_mode_param():
    """Test that passing None as mode_param raises IncompatibleInputError."""
    image = create_annular_image()
    params = create_maglim_test_params()

    # Create a Maglim object with None mode_param
    maglim = hdrlfunc.Maglim(
        params["zeropoint"], params["fwhm_seeing"], params["kernel_sx"], params["kernel_sy"], params["method"], None
    )

    # This should raise IncompatibleInputError because mode_param is None
    with pytest.raises(hdrlcore.IncompatibleInputError):
        maglim.compute(image)


def test_maglim_compute_valid():
    """Test that valid inputs produce a valid result."""
    image = create_annular_image()
    params = create_maglim_test_params()

    # Create a Maglim object
    maglim = hdrlfunc.Maglim(
        params["zeropoint"],
        params["fwhm_seeing"],
        params["kernel_sx"],
        params["kernel_sy"],
        params["method"],
        params["mode_param"],
    )

    # This should work without raising an exception
    result = maglim.compute(image)
    assert isinstance(result, float)
    assert math.isclose(result, -5.591854160255954)


def test_maglim_properties():
    """Test that Maglim object properties are accessible."""
    params = create_maglim_test_params()

    maglim = hdrlfunc.Maglim(
        params["zeropoint"],
        params["fwhm_seeing"],
        params["kernel_sx"],
        params["kernel_sy"],
        params["method"],
        params["mode_param"],
    )

    # Test property accessors
    assert maglim.zeropoint == params["zeropoint"]
    assert maglim.fwhm == params["fwhm_seeing"]
    assert maglim.kernel_size_x == params["kernel_sx"]
    assert maglim.kernel_size_y == params["kernel_sy"]
    assert maglim.extend_method == params["method"]
    # Note: mode_param property is not tested as hdrl::core::Parameter is not
    # registered with pybind11


def test_maglim_extend_method_enum():
    """Test that all extend method enum values work correctly."""
    image = create_annular_image()
    params = create_maglim_test_params()

    # Test Nearest method
    maglim_nearest = hdrlfunc.Maglim(
        params["zeropoint"],
        params["fwhm_seeing"],
        params["kernel_sx"],
        params["kernel_sy"],
        hdrlfunc.Maglim.ImageExtendMethod.Nearest,
        params["mode_param"],
    )
    result_nearest = maglim_nearest.compute(image)
    assert isinstance(result_nearest, float)

    # Test Mirror method
    maglim_mirror = hdrlfunc.Maglim(
        params["zeropoint"],
        params["fwhm_seeing"],
        params["kernel_sx"],
        params["kernel_sy"],
        hdrlfunc.Maglim.ImageExtendMethod.Mirror,
        params["mode_param"],
    )
    result_mirror = maglim_mirror.compute(image)
    assert isinstance(result_mirror, float)
