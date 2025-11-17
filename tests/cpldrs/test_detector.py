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

import cpl
import pytest
import os
import numpy as np

IMAGESZ2 = 100


@pytest.fixture
def test_image_small(request):  # Create test image with different types
    im = cpl.core.Image.create_noise_uniform(10, 20, request.param, 10, 20)
    return im


test_dir = os.path.dirname(os.path.realpath(__file__))


class TestDetector:
    # test interpolate_rejected() may not modify a bad-pixel free image
    @pytest.mark.parametrize(
        "test_image_small",
        [
            cpl.core.Type.DOUBLE,
            cpl.core.Type.FLOAT,
            cpl.core.Type.INT,
            cpl.core.Type.DOUBLE_COMPLEX,
            cpl.core.Type.FLOAT_COMPLEX,
        ],
        indirect=["test_image_small"],
    )
    def test_interpolate_rejected_nobad(self, test_image_small):
        original = test_image_small.duplicate()
        cpl.drs.detector.interpolate_rejected(test_image_small)
        assert np.array_equal(original, test_image_small)

    @pytest.mark.parametrize(
        "test_image_small",
        [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT],
        indirect=["test_image_small"],
    )
    def test_interpolate_rejected_isotropic(self, test_image_small):
        original = test_image_small.duplicate()
        # bad pixel locations
        bad_pos_x = [3, 4, 5, 3, 4, 5, 3, 4, 5, 7]
        bad_pos_y = [4, 4, 4, 5, 5, 5, 6, 6, 6, 8]
        bad_positions = list(zip(bad_pos_x, bad_pos_y))

        # Set bad pixel values
        for pos_x, pos_y in bad_positions:
            test_image_small.set_pixel(pos_y, pos_x, np.finfo(np.float32).max)
            test_image_small.reject(pos_y, pos_x)
        assert test_image_small.count_rejected() == 10

        # Confirm good pixel positions are unaffected
        for ypos in range(test_image_small.height):
            for xpos in range(test_image_small.width):
                if (xpos, ypos) not in bad_positions:
                    assert original[ypos][xpos] == test_image_small[ypos][xpos]

        stdev = test_image_small.get_stdev()
        cpl.drs.detector.interpolate_rejected(test_image_small)
        # Stdev should have decreased or be equal
        assert test_image_small.get_stdev() <= stdev
        # Verify the cleaning is isotropic
        for i in range(1, 4):
            turn = original.duplicate()
            # perform same rejection on image to turn
            for pos_x, pos_y in zip(bad_pos_x, bad_pos_y):
                turn.set_pixel(pos_y, pos_x, np.finfo(np.float32).max)
                turn.reject(pos_y, pos_x)

            # Turn then interporlate
            turn.turn(i)
            cpl.drs.detector.interpolate_rejected(turn)
            # Return back to original orientation
            turn.turn(-i)
            # Non-zero tolerance, since order of additions changed
            assert np.allclose(turn, test_image_small, atol=np.finfo(np.double).eps)
        test_image_small.bpm  # Should be able to access the bpm without a segfault

    @pytest.mark.parametrize(
        "test_image_small",
        [
            cpl.core.Type.DOUBLE,
            cpl.core.Type.FLOAT,
            cpl.core.Type.INT,
            cpl.core.Type.DOUBLE_COMPLEX,
            cpl.core.Type.FLOAT_COMPLEX,
        ],
        indirect=["test_image_small"],
    )
    def test_interpolate_rejected_all_rejected(self, test_image_small):
        test_image_small.bpm = ~test_image_small.bpm
        assert (
            test_image_small.count_rejected()
            == test_image_small.width * test_image_small.height
        )
        with pytest.raises(cpl.core.DataNotFoundError):
            cpl.drs.detector.interpolate_rejected(test_image_small)

    @pytest.mark.parametrize(
        "test_image_small",
        [
            cpl.core.Type.DOUBLE,
            cpl.core.Type.FLOAT,
            cpl.core.Type.INT,
            cpl.core.Type.DOUBLE_COMPLEX,
            cpl.core.Type.FLOAT_COMPLEX,
        ],
        indirect=["test_image_small"],
    )
    def test_interpolate_rejected_one_accepted(self, test_image_small):
        # Mark all pixels bad
        test_image_small.bpm = ~test_image_small.bpm
        # Mark a single pixel in the top right corner good
        test_image_small.bpm[-1][-1] = False
        # Check that there are n_pixels - 1 bad pixels
        assert (
            test_image_small.count_rejected()
            == test_image_small.height * test_image_small.width - 1
        )
        # Store pixel value of the single good pixel for later comparison.
        good_pixel_value = test_image_small[-1][-1]
        # Interpolate the bad pixels
        cpl.drs.detector.interpolate_rejected(test_image_small)
        # All pixels should now have the same value as the one pixel that was marked good before interpolation.
        for ypos in range(test_image_small.height):
            for xpos in range(test_image_small.width):
                if test_image_small.type == cpl.core.Type.DOUBLE:
                    atol = np.finfo(np.double).eps
                if test_image_small.type == cpl.core.Type.FLOAT:
                    atol = np.finfo(np.single).eps
                if test_image_small.type == cpl.core.Type.INT:
                    atol = 0
                if test_image_small.type == cpl.core.Type.DOUBLE_COMPLEX:
                    atol = np.finfo(np.complex128).eps
                if test_image_small.type == cpl.core.Type.FLOAT_COMPLEX:
                    atol = np.finfo(np.complex64).eps
                assert np.isclose(
                    test_image_small[ypos][xpos], good_pixel_value, atol=atol
                )

    # The validation noise values are obtained by running through the CPL tests
    @pytest.mark.parametrize(
        "file,validation_noise, validation_err",
        [
            (
                f"{test_dir}/test_images/detector_test_image_double.fits",
                2.8946057952847708,
                0.1415197735598678,
            ),
            (
                f"{test_dir}/test_images/detector_test_image_float.fits",
                2.8998190262621035,
                0.1632607759267108,
            ),
            (
                f"{test_dir}/test_images/detector_test_image_int.fits",
                2.9050278725316145,
                0.1361666969719621,
            ),
        ],
    )
    def test_noise(self, file, validation_noise, validation_err):
        """get noise, no zone def with default hsize and nsamp"""
        im = cpl.core.Image.load(file, dtype=cpl.core.Type.UNSPECIFIED, extension=0)
        noise, noise_err = cpl.drs.detector.get_noise_window(im)
        # Ensure the noise is close enough to the validation by the magnatude of the calculated error
        assert np.isclose(
            noise, validation_noise, atol=noise_err * 5
        )  # High tolerance as error represents stdev of the probability distribution of values
        assert np.isclose(
            noise_err, validation_err, atol=0.1
        )  # Check theres not a huge difference between calculated error

    # The validation bias values are obtained by running through the CPL tests
    @pytest.mark.parametrize(
        "file,validation_bias, validation_err",
        [
            (
                os.path.join(test_dir, "test_images/detector_test_image_double.fits"),
                15.0286948295432232,
                0.3258732659715609,
            ),
            (
                os.path.join(test_dir, "test_images/detector_test_image_float.fits"),
                15.0225052958643115,
                0.3313485833285817,
            ),
            (
                os.path.join(test_dir, "test_images/detector_test_image_int.fits"),
                15.0105832841377094,
                0.3199502028371906,
            ),
        ],
    )
    def test_bias(self, file, validation_bias, validation_err):
        """get bias, no zone def with default hsize and nsamp"""
        im = cpl.core.Image.load(file, dtype=cpl.core.Type.UNSPECIFIED, extension=0)
        bias, bias_err = cpl.drs.detector.get_bias_window(im)
        # Ensure the bias is close enough to the validation by the magnatude of the calculated error
        assert np.isclose(
            bias, validation_bias, atol=bias_err * 5
        )  # High tolerance as error represents stdev of the probability distribution of values
        assert np.isclose(
            bias_err, validation_err, atol=0.1
        )  # Check theres not a huge difference between calculated error and validation error

    # The validation bias values are averages obtained by running through the CPL tests
    @pytest.mark.parametrize(
        "file,validation_noise, validation_err",
        [
            (
                os.path.join(test_dir, "test_images/detector_test_image_double.fits"),
                2.9029928659107256,
                0.1267645761019499,
            ),
            (
                os.path.join(test_dir, "test_images/detector_test_image_float.fits"),
                2.9652561896022309,
                0.1700246028350480,
            ),
            (
                os.path.join(test_dir, "test_images/detector_test_image_int.fits"),
                2.9975005253712856,
                0.1156194998261096,
            ),
        ],
    )
    def test_noise_ring(self, file, validation_noise, validation_err):
        """get ring, no zone def with default hsize and nsamp"""
        zone = (
            int(0.5 * IMAGESZ2 - 1),
            int(0.5 * IMAGESZ2 - 1),
            IMAGESZ2 / 10.0,
            IMAGESZ2 / 5.0,
        )
        im = cpl.core.Image.load(file, dtype=cpl.core.Type.UNSPECIFIED, extension=0)
        noise, noise_err = cpl.drs.detector.get_noise_ring(im, zone)
        # Ensure the bias is close enough to the validation by the magnatude of the calculated error
        print(noise_err)
        assert np.isclose(
            noise, validation_noise, atol=noise_err * 5
        )  # High tolerance as error represents stdev of the probability distribution of values
        assert np.isclose(
            noise_err, validation_err, atol=0.1
        )  # Check theres not a huge difference between calculated error

    @pytest.mark.parametrize(
        "file,validation_noise",
        [
            (
                os.path.join(test_dir, "test_images/detector_test_image_double.fits"),
                2.9196444697845481,
            ),
            (
                os.path.join(test_dir, "test_images/detector_test_image_float.fits"),
                2.9186120895285304,
            ),
            (
                os.path.join(test_dir, "test_images/detector_test_image_int.fits"),
                2.8858536015770677,
            ),
        ],
    )
    def test_empty_ring(self, file, validation_noise):
        """get ring, no zone def with default hsize and nsamp"""
        zone = (
            int(0.5 * IMAGESZ2 - 1),
            int(0.5 * IMAGESZ2 - 1),
            IMAGESZ2 / 10.0,
            IMAGESZ2 / 10.0,
        )
        im = cpl.core.Image.load(file, dtype=cpl.core.Type.UNSPECIFIED, extension=0)
        with pytest.raises(cpl.core.IllegalInputError):
            noise, noise_err = cpl.drs.detector.get_noise_ring(im, zone)
