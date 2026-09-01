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

import cpl
import hdrl
import numpy as np
import pytest


def test_overscan_parameters():
    # Set up the region and collapse parameters
    rect_region = (1, 1, 20, 20)
    box_hsize = 10
    ccd_ron = 10.0
    direction = "x"

    # Create collapse parameters - equivalent to minmax in C
    collapse = hdrl.func.Collapse.MinMax(2, 3)

    # Initialize Overscan object
    overscan = hdrl.func.Overscan(direction, ccd_ron, box_hsize, collapse, rect_region)

    # Verify properties
    assert overscan.direction == "x"
    assert overscan.ccd_ron == pytest.approx(10.0)
    assert overscan.box_hsize == 10

    # Check the region values
    region = overscan.overscan_region
    assert region == (1, 1, 20, 20)

    # Create a dummy image
    img = hdrl.core.Image.zeros(100, 100)
    try:
        overscan.compute(img)
    except Exception as e:
        pytest.fail(f"compute() raised unexpected exception: {e}")


def test_overscan_null_input():
    # Create overscan region and collapse parameters
    region = (1, 1, 1, 1)
    collapse = hdrl.func.Collapse.Mean()

    # Create the Overscan parameter object
    direction = "y"
    ccd_ron = 1.0
    box_hsize = 1

    overscan = hdrl.func.Overscan(direction, ccd_ron, box_hsize, collapse, region)

    # Attempt to compute with None input image -> should raise an exception
    with pytest.raises(hdrl.core.NullInputError):
        overscan.compute(None)


def test_overscan_null_region():
    # Create a minimal valid input image
    image_data = hdrl.core.Image.zeros(1, 1)

    # Create collapse method
    collapse = hdrl.func.Collapse.Mean()

    # Now try to create Overscan with a None region
    direction = "y"
    ccd_ron = 1.0
    box_hsize = 1

    overscan = hdrl.func.Overscan(direction, ccd_ron, box_hsize, collapse, None)
    with pytest.raises(hdrl.core.IllegalInputError):
        overscan.compute(image_data)


def test_overscan_wrong_region():
    image_data = hdrl.core.Image.zeros(10, 5)  # shape: (height=10, width=5)

    # Create parameters
    os_region = (1, 1, 10, 5)
    os_collapse = hdrl.func.Collapse.Mean()
    overscan = hdrl.func.Overscan("y", 1.0, 1, os_collapse, os_region)

    # Update region to one that is invalid (outside bounds)
    os_region = (0, 2, 4, 2)
    overscan = hdrl.func.Overscan("y", 1.0, 1, os_collapse, os_region)

    # Test overscan_compute with wrong region — expects None or raises error
    with pytest.raises(hdrl.core.IllegalInputError):
        overscan.compute(image_data)

    # Now test another invalid region (exceeding image width)
    os_region = (1, 5, 6, 10)
    overscan = hdrl.func.Overscan("y", 1.0, 1, os_collapse, os_region)

    # Again expecting None or exception
    with pytest.raises(hdrl.core.IllegalInputError):
        overscan.compute(image_data)


def test_hdrl_overscan_uniform_image():
    inp_value = 42.0
    width, height = 100, 100

    # Create uniform image and error arrays
    image_data = np.full((height, width), inp_value, dtype=np.float64)
    image_errs = np.full((height, width), inp_value, dtype=np.float64) ** 0.5

    cpl_image_data = cpl.core.Image(image_data)
    cpl_image_errs = cpl.core.Image(image_errs)

    # Wrap into hdrl Image
    image = hdrl.core.Image(cpl_image_data, cpl_image_errs)

    # Set up overscan region & collapse params
    os_collapse = hdrl.func.Collapse.Mean()
    os_region = (1, 1, 20, height)
    direction = "x"
    ccd_ron = 1.0
    box_hsize = 5
    overscan = hdrl.func.Overscan(direction, ccd_ron, box_hsize, os_collapse, os_region)

    # Compute overscan signal
    overscan.compute(image)
    correction = overscan.correction
    assert correction is not None
    assert isinstance(correction, hdrl.core.Image)

    # Overscan correction should be near constant (since input is uniform)
    # corr_data = result.correction.data().to_numpy()
    # assert np.allclose(corr_data, inp_value, atol=1e-6)

    # Apply overscan correction to actual image area (excluding overscan)
    reg = (21, 1, width, height)
    correction = overscan.correct(image, reg)
    assert correction is not None
    assert isinstance(correction.corrected, hdrl.core.Image)

    # Check that corrected image values are ~0 (since we subtract overscan)
    # corrected_array = correction.corrected.data().to_numpy()
    # assert np.allclose(corrected_array, 0.0, atol=1e-6)


def test_hdrl_overscan_test_dir(Nx=50, Ny=20, hbox=2):
    error = 10.0

    # 1) Create image that increments in x axis
    image_data = np.tile(np.arange(Nx, dtype=np.float64), (Ny, 1))
    image_errs = np.sqrt(image_data)

    cpl_image_data = cpl.core.Image(image_data)
    cpl_image_errs = cpl.core.Image(image_errs)

    image = hdrl.core.Image(cpl_image_data, cpl_image_errs)
    sigma_clip = error * np.sqrt(Ny * (1 + 2 * hbox))

    # Overscan Parameters (Y-direction)
    os_region = (1, 1, Nx, Ny)
    os_collapse = hdrl.func.Collapse.Mean()
    overscan = hdrl.func.Overscan("y", sigma_clip, hbox, os_collapse, os_region)

    # Compute overscan correction in y
    overscan.compute(image)
    assert overscan.correction is not None

    # # Check correction is incrementing as expected
    # mean_line = image_data[0, :]
    # correction_line = res_os_comp.correction.data[0, :]
    # assert np.allclose(mean_line, correction_line, atol=2 * (1 + 2*hbox) * Ny * 1e-6)

    # # Test errors pattern: central = error, larger at boundaries
    # errors_line = res_os_comp.correction.errs[0, :]
    # expected_errs = np.full_like(errors_line, error)
    # for i in range(hbox):
    #     cor = np.sqrt((1 + 2*hbox) / (1 + 2*i))
    #     expected_errs[i] = error * cor
    #     expected_errs[-(i+1)] = error * cor
    # assert np.allclose(errors_line, expected_errs, atol=Ny * 1e-6)

    # # Overscan Parameters (X-direction)
    # os_param_x = hdrl.func.Overscan(
    #     axis='x',
    #     sigma_clip=error * np.sqrt(Nx * (1 + 2*hbox)),
    #     hbox=hbox,
    #     collapse=os_collapse,
    #     region=os_region
    # )
    # res_os_comp_x = os_param_x.compute(image_data)
    # assert res_os_comp_x is not None

    # # Correction is mean of column
    # correction_col = res_os_comp_x.correction.data[:, 0]
    # expected_correction = np.full_like(correction_col, (Nx-1)/2.)
    # assert np.allclose(correction_col, expected_correction, atol=2 * (1 + 2*hbox) * Nx * 1e-6)

    # # Errors larger at boundaries
    # errors_col = res_os_comp_x.correction.errs[:, 0]
    # expected_errs_x = np.full_like(errors_col, error)
    # for i in range(hbox):
    #     cor = np.sqrt((1 + 2*hbox) / (1 + 2*i))
    #     expected_errs_x[i] = error * cor
    #     expected_errs_x[-(i+1)] = error * cor
    # assert np.allclose(errors_col, expected_errs_x, atol=3 * Nx * 1e-6)

    # # Correct y direction
    # res_os_cor_y = os_param.correct(image, comp=res_os_comp)
    # corrected_y = res_os_cor_y.corrected.data
    # assert np.allclose(corrected_y, 0, atol=2 * (1 + 2*hbox) * Ny * 1e-6)

    # # Error after correction y-direction
    # corrected_errs_y = res_os_cor_y.corrected.errs
    # expected_errs_y = np.sqrt(image_errs**2 + error**2)
    # # Adjust boundaries as above
    # for i in range(hbox):
    #     cor = np.sqrt((1 + 2*hbox) / (1 + 2*i))
    #     expected_errs_y[:, i] = np.hypot(image_errs[:, i], error * cor)
    #     expected_errs_y[:, -(i+1)] = np.hypot(image_errs[:, -i-1], error * cor)
    # assert np.allclose(corrected_errs_y, expected_errs_y, atol=Ny * 1e-6)

    # # Correct x-direction
    # res_os_cor_x = os_param_x.correct(image, comp=res_os_comp_x)
    # corrected_x = res_os_cor_x.corrected.data
    # assert np.allclose(corrected_x.mean(axis=0), 0, atol=2 * (1 + 2*hbox) * Nx * 1e-6)

    # # Error after correction x-direction
    # corrected_errs_x = res_os_cor_x.corrected.errs
    # expected_errs_x2 = np.sqrt(image_errs**2 + error**2)
    # for i in range(hbox):
    #     cor = np.sqrt((1 + 2*hbox) / (1 + 2*i))
    #     expected_errs_x2[i, :] = np.hypot(image_errs[i, :], error * cor)
    #     expected_errs_x2[-(i+1), :] = np.hypot(image_errs[-(i+1), :], error * cor)
    # assert np.allclose(corrected_errs_x, expected_errs_x2, atol=3 * Nx * 1e-6)
