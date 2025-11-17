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

from contextlib import nullcontext as does_not_raise
import ctypes
import ctypes.util
import math

from astropy.io import fits
import numpy as np
import pytest
from scipy import ndimage
import subprocess

from cpl import core as cplcore
from cpl import drs as cpldrs


class TestImage:
    def test_constructor_zero_detected(self):
        with pytest.raises(cplcore.IllegalInputError):
            _ = cplcore.Image.zeros(0, 0, cplcore.Type.INT)

    def test_constructor_int_from_list_2d(self):
        list_2d = [[5, 6], [19, -12]]
        img = cplcore.Image(list_2d, cplcore.Type.INT)
        assert img.type == cplcore.Type.INT
        assert img.shape == (2, 2)
        assert img.width == 2
        assert img.height == 2
        assert img.size == 4
        assert img[0][0] == 5
        assert img[0][1] == 6
        assert img[1][0] == 19
        assert img[1][1] == -12

    def test_constructor_complex_from_list_2d(self):
        list_2d = [[5 + 6j, -3 - 6j, 0], [99 + 94j, -559 + 1j, -50 + 4j]]
        img = cplcore.Image(list_2d, cplcore.Type.FLOAT_COMPLEX)
        assert img.type == cplcore.Type.FLOAT_COMPLEX
        assert img.shape == (2, 3)
        assert img.width == 3
        assert img.height == 2
        assert img.size == 6
        assert img[0][0] == 5 + 6j
        assert img[0][1] == -3 - 6j
        assert img[0][2] == 0
        assert img[1][0] == 99 + 94j
        assert img[1][1] == -559 + 1j
        assert img[1][2] == -50 + 4j

    def test_constructor_int_from_ndarray_2d(self):
        list_2d = np.array([[5, 6], [19, -12], [4.65, -99]], np.intc)
        img = cplcore.Image(list_2d)
        assert img.type == cplcore.Type.INT

        assert img.shape == list_2d.shape
        assert img.width == 2
        assert img.height == 3
        assert img.size == 6
        assert img[0][0] == 5
        assert img[0][1] == 6
        assert img[1][0] == 19
        assert img[1][1] == -12
        # Floating point values are trnucated
        assert img[2][0] == 4
        assert img[2][1] == -99

    @pytest.mark.parametrize(
        "cpl_typeid, pixel_type",
        [
            (cplcore.Type.INT, np.intc),
            (cplcore.Type.FLOAT, np.single),
            (cplcore.Type.DOUBLE, np.double),
        ],
    )
    def test_constructor_load_native(
        self, make_mock_image, make_mock_fits, cpl_typeid, pixel_type
    ):
        mock_image = make_mock_image(dtype=pixel_type, width=256, height=512)

        # Write this image to a FITS file
        my_fits_filename = make_mock_fits(fits.HDUList([fits.PrimaryHDU(mock_image)]))

        new_img = cplcore.Image.load(my_fits_filename, dtype=cplcore.Type.UNSPECIFIED)

        assert new_img.shape == mock_image.shape
        assert new_img.type == cpl_typeid

        # Just check some random indicies
        # Checking the whole image takes too long
        assert new_img[0][0] == mock_image[0][0]
        assert new_img[61][126] == mock_image[61][126]
        assert new_img[172][91] == mock_image[172][91]
        assert new_img[482][146] == mock_image[482][146]
        assert new_img[49][211] == mock_image[49][211]
        assert new_img[46][142] == mock_image[46][142]
        assert new_img[44][41] == mock_image[44][41]
        assert new_img[393][186] == mock_image[393][186]
        assert new_img[184][235] == mock_image[184][235]
        assert new_img[466][40] == mock_image[466][40]
        assert new_img[130][93] == mock_image[130][93]
        assert new_img[511][255] == mock_image[511][255]

    @pytest.mark.parametrize("pixel_type", [np.intc, np.single, np.double])
    def test_constructor_load_default(
        self, make_mock_image, make_mock_fits, pixel_type
    ):
        mock_image = make_mock_image(dtype=pixel_type, width=256, height=512)

        # Write this image to a FITS file
        my_fits_filename = make_mock_fits(fits.HDUList([fits.PrimaryHDU(mock_image)]))

        new_img = cplcore.Image.load(my_fits_filename)

        assert new_img.shape == mock_image.shape
        assert new_img.type == cplcore.Type.DOUBLE

        # Just check some random indicies
        # Checking the whole image takes too long
        assert new_img[0][0] == mock_image[0][0]
        assert new_img[61][126] == mock_image[61][126]
        assert new_img[172][91] == mock_image[172][91]
        assert new_img[482][146] == mock_image[482][146]
        assert new_img[49][211] == mock_image[49][211]
        assert new_img[46][142] == mock_image[46][142]
        assert new_img[44][41] == mock_image[44][41]
        assert new_img[393][186] == mock_image[393][186]
        assert new_img[184][235] == mock_image[184][235]
        assert new_img[466][40] == mock_image[466][40]
        assert new_img[130][93] == mock_image[130][93]
        assert new_img[511][255] == mock_image[511][255]

    @pytest.mark.parametrize(
        "cpl_typeid, pixel_type",
        [
            (cplcore.Type.INT, np.intc),
            (cplcore.Type.FLOAT, np.single),
            (cplcore.Type.DOUBLE, np.double),
        ],
    )
    def test_constructor_load_type(
        self, make_mock_image, make_mock_fits, cpl_typeid, pixel_type
    ):
        mock_image = make_mock_image(dtype=pixel_type, width=256, height=512)

        # Write this image to a FITS file
        my_fits_filename = make_mock_fits(fits.HDUList([fits.PrimaryHDU(mock_image)]))

        new_img = cplcore.Image.load(my_fits_filename, dtype=cpl_typeid)

        assert new_img.shape == mock_image.shape
        assert new_img.type == cpl_typeid

        # Just check some random indicies
        # Checking the whole image takes too long
        assert new_img[0][0] == mock_image[0][0]
        assert new_img[61][126] == mock_image[61][126]
        assert new_img[172][91] == mock_image[172][91]
        assert new_img[482][146] == mock_image[482][146]
        assert new_img[49][211] == mock_image[49][211]
        assert new_img[46][142] == mock_image[46][142]
        assert new_img[44][41] == mock_image[44][41]
        assert new_img[393][186] == mock_image[393][186]
        assert new_img[184][235] == mock_image[184][235]
        assert new_img[466][40] == mock_image[466][40]
        assert new_img[130][93] == mock_image[130][93]
        assert new_img[511][255] == mock_image[511][255]

    @pytest.mark.parametrize(
        "cpl_typeid, pixel_type, tiny",
        [
            (cplcore.Type.DOUBLE, np.intc, np.finfo(np.double).eps),
            (cplcore.Type.INT, np.single, 1),
            (cplcore.Type.INT, np.double, 1),
        ],
    )
    def test_constructor_load_cast(
        self, make_mock_image, make_mock_fits, cpl_typeid, pixel_type, tiny
    ):
        mock_image = make_mock_image(dtype=pixel_type, width=256, height=512)

        # Write this image to a FITS file
        my_fits_filename = make_mock_fits(fits.HDUList([fits.PrimaryHDU(mock_image)]))

        new_img = cplcore.Image.load(my_fits_filename, dtype=cpl_typeid)

        assert new_img.shape == mock_image.shape
        assert new_img.type == cpl_typeid

        # Just check some random indicies
        # Checking the whole image takes too long
        assert np.abs(new_img[0][0] - mock_image[0][0]) < tiny
        assert np.abs(new_img[172][91] - mock_image[172][91]) < tiny
        assert np.abs(new_img[61][126] - mock_image[61][126]) < tiny
        assert np.abs(new_img[482][146] - mock_image[482][146]) < tiny
        assert np.abs(new_img[49][211] - mock_image[49][211]) < tiny
        assert np.abs(new_img[46][142] - mock_image[46][142]) < tiny
        assert np.abs(new_img[44][41] - mock_image[44][41]) < tiny
        assert np.abs(new_img[393][186] - mock_image[393][186]) < tiny
        assert np.abs(new_img[184][235] - mock_image[184][235]) < tiny
        assert np.abs(new_img[466][40] - mock_image[466][40]) < tiny
        assert np.abs(new_img[130][93] - mock_image[130][93]) < tiny
        assert np.abs(new_img[511][255] - mock_image[511][255]) < tiny

    def test_constructor_load_window(self, make_mock_image, make_mock_fits):
        mock_image = np.block(
            [
                [
                    np.array([1] * 9, dtype=np.double).reshape(3, 3),
                    np.array([2] * 9, dtype=np.double).reshape(3, 3),
                ],
                [
                    np.array([3] * 9, dtype=np.double).reshape(3, 3),
                    np.array([4] * 9, dtype=np.double).reshape(3, 3),
                ],
            ]
        )

        # Write this image to a FITS file
        my_fits_filename = make_mock_fits(fits.HDUList([fits.PrimaryHDU(mock_image)]))

        new_img = cplcore.Image.load(
            my_fits_filename, dtype=cplcore.Type.DOUBLE, area=(0, 0, 2, 2)
        )

        # Check loaded image structure and type
        assert new_img.shape == (3, 3)
        assert new_img.type == cplcore.Type.DOUBLE

        # Verify expected mean value of the loaded block
        assert np.abs(np.mean(new_img) - 1.0) < np.finfo(np.double).eps

    def test_get_bpm(self, make_mock_image, make_mock_fits):
        # In order to test bpm setting a non-trivial BPM
        # we create a BPM from loading a FITS file:

        bpm_base_image = make_mock_image(
            dtype=np.ubyte, min=0, max=256, width=256, height=512
        )
        bpm_filename = make_mock_fits(fits.HDUList([fits.PrimaryHDU(bpm_base_image)]))
        bpm_in = cplcore.Mask.load(bpm_filename)

        # Set the BPM:

        img = cplcore.Image.zeros(
            bpm_base_image.shape[1], bpm_base_image.shape[0], cplcore.Type.DOUBLE
        )
        img.bpm = bpm_in

        # Just check some random indicies
        # Checking the whole mask takes too long
        assert img.bpm[0][0] == bpm_in[0][0]
        assert img.bpm[73][98] == bpm_in[73][98]
        assert img.bpm[54][87] == bpm_in[54][87]
        assert img.bpm[205][54] == bpm_in[205][54]
        assert img.bpm[223][77] == bpm_in[223][77]
        assert img.bpm[74][67] == bpm_in[74][67]
        assert img.bpm[214][123] == bpm_in[214][123]
        assert img.bpm[147][149] == bpm_in[147][149]
        assert img.bpm[68][25] == bpm_in[68][25]
        assert img.bpm[39][215] == bpm_in[39][215]
        assert img.bpm[245][167] == bpm_in[245][167]
        assert img.bpm[255][255] == bpm_in[255][255]

        # 1's (True) in the mask mean that the pixel is rejected
        assert img.bpm[0][0] == (img[0][0] is None)
        assert img.bpm[73][98] == (img[73][98] is None)
        assert img.bpm[54][87] == (img[54][87] is None)
        assert img.bpm[205][54] == (img[205][54] is None)
        assert img.bpm[223][77] == (img[223][77] is None)
        assert img.bpm[74][67] == (img[74][67] is None)
        assert img.bpm[214][123] == (img[214][123] is None)
        assert img.bpm[147][149] == (img[147][149] is None)
        assert img.bpm[68][25] == (img[68][25] is None)
        assert img.bpm[39][215] == (img[39][215] is None)
        assert img.bpm[245][167] == (img[245][167] is None)
        assert img.bpm[255][255] == (img[255][255] is None)

    def test_modify_bpm(self, make_mock_image, make_mock_fits):
        # Starts off all 0's (Not rejected)
        shape = (1, 2)
        img = cplcore.Image.zeros(shape[1], shape[0], cplcore.Type.DOUBLE)
        assert not img.bpm[0][0]
        assert not img.bpm[0][1]
        assert not img.is_rejected(0, 0)
        assert not img.is_rejected(0, 1)

        # It should be modifyiable from construction
        img.bpm[0][1] = True
        assert not img.bpm[0][0]
        assert img.bpm[0][1]
        assert not img.is_rejected(0, 0)
        assert img.is_rejected(0, 1)

        # All 1's
        img.bpm = ~cplcore.Mask(shape[1], shape[0])
        assert img.bpm[0][0]
        assert img.bpm[0][1]
        assert img.is_rejected(0, 0)
        assert img.is_rejected(0, 1)

        # Should still be modifiable
        img.bpm[0][0] = False
        assert not img.bpm[0][0]
        assert img.bpm[0][1]
        assert not img.is_rejected(0, 0)
        assert img.is_rejected(0, 1)

        # And modifiable from image
        img.bpm[0][1] = False
        assert not img.bpm[0][0]
        assert not img.bpm[0][1]
        assert not img.is_rejected(0, 0)
        assert not img.is_rejected(0, 1)

    def test_set_int(self):
        img = cplcore.Image.zeros(2, 1, cplcore.Type.INT)
        img[0][0] = 5
        assert img[0][0] == 5

        with pytest.raises(cplcore.InvalidTypeError):
            img[0][1] = 71 + 3j
            assert img[0][1] == 71

    def test_set_complex(self):
        img = cplcore.Image.zeros(2, 2, cplcore.Type.DOUBLE_COMPLEX)
        img[0][0] = 5 + 7j
        assert img[0][0] == 5 + 7j

        img[1][0] = 96
        print(img)
        assert img[1][0] == 96

    def test_set_rejected(self):
        img = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        assert not img.bpm[0][0]
        # Store something as the rejected pixel
        img[0][0] = 5

        # Then reject:
        img[0][0] = None

        # Check it's rejected
        assert img[0][0] is None
        assert img.bpm[0][0]
        assert img.is_rejected(0, 0)

        # Now check the rejected pixel is preserved
        img.accept(0, 0)
        assert img[0][0] == 5

    def test_reject(self):
        img = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        assert not img.bpm[0][0]
        # Store something as the rejected pixel
        img[0][0] = 5

        # Then reject:
        img.reject(0, 0)

        # Check it's rejected
        assert img[0][0] is None
        assert img.bpm[0][0]
        assert img.is_rejected(0, 0)

        # Now check the rejected pixel is preserved
        img.accept(0, 0)
        assert img[0][0] == 5

    @pytest.fixture(scope="function")
    def pathological_image(self, request):
        img = cplcore.Image(
            ((1, math.nan, -np.inf), (0, 42, math.inf), (np.inf, 99, np.nan))
        )
        return img

    @pytest.mark.parametrize(
        "test_values,test_pixels",
        (
            ({math.nan}, ((0, 1), (2, 2))),
            ({-np.inf, 0}, ((0, 2), (1, 0))),
            ({math.inf, np.nan}, ((0, 1), (1, 2), (2, 0), (2, 2))),
        ),
        ids=("math.nan", "0, -np.inf", "np.nan, math.inf"),
    )
    def test_reject_value(self, pathological_image, test_values, test_pixels):
        for coords in test_pixels:
            assert not pathological_image.is_rejected(*coords)
        pathological_image.reject_value(test_values)
        for coords in test_pixels:
            assert pathological_image.is_rejected(*coords)

    def test_reject_value_unsupported(self, pathological_image):
        with pytest.raises(cplcore.UnsupportedModeError):
            pathological_image.reject_value({0, 1})

    def test_reject_value_complex(self):
        complex_image = cplcore.Image(
            ((1 + 3j, 2), (3, 4)), dtype=cplcore.Type.DOUBLE_COMPLEX
        )
        with pytest.raises(cplcore.InvalidTypeError):
            complex_image.reject_value({0})

    def test_accept(self):
        img = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        # Store something as the rejected pixel
        img[0][0] = 5
        img.reject(0, 0)

        img.accept(0, 0)

        # Check it's accepted
        assert img[0][0] == 5
        assert not img.bpm[0][0]
        assert not img.is_rejected(0, 0)

    def test_accept_all(self):
        img = cplcore.Image.zeros(1, 2, cplcore.Type.DOUBLE)
        # Store something as the rejected pixel
        img[0][0] = 5
        img[1][0] = -10
        img.reject(0, 0)
        img.reject(1, 0)

        img.accept_all()

        # Check it's accepted
        assert img[0][0] == 5
        assert img[1][0] == -10
        assert not img.bpm[0][0]
        assert not img.bpm[1][0]
        assert not img.is_rejected(0, 0)
        assert not img.is_rejected(1, 0)

    def test_count_rejected(self):
        img = cplcore.Image.zeros(2, 1, cplcore.Type.DOUBLE)
        assert img.count_rejected() == 0
        img[0][0] = None
        assert img.count_rejected() == 1
        img[0][1] = None
        assert img.count_rejected() == 2

    def test_conjugate_scalar(self):
        img = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        with pytest.raises(cplcore.InvalidTypeError):
            img.conjugate()

    def test_conjugate(self):
        img = cplcore.Image.zeros(1, 1, cplcore.Type.FLOAT_COMPLEX)
        img[0][0] = 5 + 6j
        img.conjugate()
        assert img[0][0] == 5 - 6j

    def test_split_real_imag_scalar(self):
        img = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        with pytest.raises(cplcore.InvalidTypeError):
            img.split_real_imag()

    def test_split_real_imag(self):
        img3 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE_COMPLEX)
        img3[0][0] = 2 + 4j
        img1, img2 = img3.split_real_imag()

        assert img1[0][0] == 2
        assert img2[0][0] == 4
        assert img1.shape == (1, 1)
        assert img2.shape == (1, 1)
        assert img1.type == cplcore.Type.DOUBLE
        assert img2.type == cplcore.Type.DOUBLE

    def test_split_abs_arg_scalar(self):
        img = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        with pytest.raises(cplcore.InvalidTypeError):
            img.split_abs_arg()

    def test_split_abs_arg(self):
        img3 = cplcore.Image.zeros(1, 1, cplcore.Type.FLOAT_COMPLEX)
        img3[0][0] = 2 + 4j
        img1, img2 = img3.split_abs_arg()

        # Abs: Distance from 0 2+4j is √(2² + 4²)
        # This is an almost-equality test
        assert math.isclose(img1[0][0], pow(pow(2, 2) + pow(4, 2), 1 / 2), rel_tol=1e-5)
        # Arg: Radians angle from 0 to 2+4j
        # This is an almost-equality test
        assert math.isclose(math.tan(img2[0][0]), 4 / 2, rel_tol=1e-5)
        assert img1.shape == (1, 1)
        assert img2.shape == (1, 1)
        assert img1.type == cplcore.Type.FLOAT
        assert img2.type == cplcore.Type.FLOAT

    def test_fill_rejected(self):
        img = cplcore.Image.zeros(2, 1, cplcore.Type.DOUBLE)
        img[0][0] = None
        img.fill_rejected(5)

        assert img[0][0] is None
        assert img[0][1] == 0
        img.accept(0, 0)
        assert img[0][0] == 5

    def test_fill_window(self):
        img = cplcore.Image.zeros(3, 3, cplcore.Type.DOUBLE)
        img.fill_window((0, 0, 1, 1), 5)
        assert img[0][0] == 5
        assert img[0][1] == 5
        assert img[1][0] == 5
        assert img[1][1] == 5

        assert img[0][2] == 0
        assert img[1][2] == 0
        assert img[2][0] == 0
        assert img[2][1] == 0
        assert img[2][2] == 0

    def test_duplicate(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 5
        img1.reject(0, 0)

        img2 = img1.duplicate()
        # Got all values
        assert img2[0][0] is None
        img2.accept(0, 0)
        assert img2[0][0] == 5

        # Didn't affect original
        img2[0][0] = 8
        assert img1[0][0] is None
        img1.accept(0, 0)
        assert img1[0][0] == 5

    def test_cast_imprecise(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 6.129012783418
        img2 = img1.cast(cplcore.Type.FLOAT)
        # Significant bits of a single-precision float: 23
        assert math.isclose(img2[0][0], 6.129012783418, rel_tol=pow(2, -24))

    def test_cast_int(self):
        img1 = cplcore.Image.zeros(2, 2, cplcore.Type.DOUBLE)
        # Away from 0
        img1[0][0] = 5.90123
        img1[0][1] = -918203123.90
        # Towards 0:
        img1[1][0] = 5.10123
        img1[1][1] = -918203123.10

        img2 = img1.cast(cplcore.Type.INT)
        # Truncated
        assert img2[0][0] == 5
        assert img2[0][1] == -918203123

        assert img2[1][0] == 5
        assert img2[1][1] == -918203123

    def test_is_complex(self):
        assert not cplcore.Image.zeros(1, 1, cplcore.Type.INT).is_complex
        assert not cplcore.Image.zeros(1, 1, cplcore.Type.FLOAT).is_complex
        assert not cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE).is_complex
        assert cplcore.Image.zeros(1, 1, cplcore.Type.FLOAT_COMPLEX).is_complex
        assert cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE_COMPLEX).is_complex

    def test_add_type_mismatch(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.FLOAT)

        img1[0][0] = 8912
        img2[0][0] = -1.891234

        img1.add(img2)
        assert img1[0][0] == 8910
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.INT

    def test_add_size_mismatch(self):
        img1 = cplcore.Image.zeros(2, 1, cplcore.Type.DOUBLE)
        img2 = cplcore.Image.zeros(1, 2, cplcore.Type.DOUBLE)

        with pytest.raises(cplcore.IncompatibleInputError):
            img1.add(img2)

    def test_add(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)

        img1[0][0] = 8912
        img2[0][0] = -1982

        img1.add(img2)
        assert img1[0][0] == 8912 - 1982
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.DOUBLE

        assert img2[0][0] == -1982
        assert img2.shape == (1, 1)
        assert img2.type == cplcore.Type.DOUBLE

    def test_subtract_type_mismatch(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.FLOAT)

        img1[0][0] = 8912
        img2[0][0] = -1.891234

        img1.subtract(img2)
        assert img1[0][0] == 8913
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.INT

    def test_subtract_size_mismatch(self):
        img1 = cplcore.Image.zeros(2, 1, cplcore.Type.DOUBLE)
        img2 = cplcore.Image.zeros(1, 2, cplcore.Type.DOUBLE)

        with pytest.raises(cplcore.IncompatibleInputError):
            img1.subtract(img2)

    def test_subtract(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)

        img1[0][0] = 8912
        img2[0][0] = -1982

        img1.subtract(img2)
        assert img1[0][0] == 8912 + 1982
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.DOUBLE

        assert img2[0][0] == -1982
        assert img2.shape == (1, 1)
        assert img2.type == cplcore.Type.DOUBLE

    def test_multiply_type_mismatch(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.FLOAT)

        img1[0][0] = 8912
        img2[0][0] = -1.891234

        img1.multiply(img2)
        assert img1[0][0] == int(8912 * -1.891234)
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.INT

    def test_multiply_size_mismatch(self):
        img1 = cplcore.Image.zeros(2, 1, cplcore.Type.DOUBLE)
        img2 = cplcore.Image.zeros(1, 2, cplcore.Type.DOUBLE)

        with pytest.raises(cplcore.IncompatibleInputError):
            img1.multiply(img2)

    def test_multiply(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)

        img1[0][0] = 8912
        img2[0][0] = -1982

        img1.multiply(img2)
        assert img1[0][0] == 8912 * -1982
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.DOUBLE

        assert img2[0][0] == -1982
        assert img2.shape == (1, 1)
        assert img2.type == cplcore.Type.DOUBLE

    def test_divide_type_mismatch(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.FLOAT)

        img1[0][0] = 8912
        img2[0][0] = -1.891234

        img1.divide(img2)
        assert img1[0][0] == int(8912 / -1.891234)
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.INT

    def test_divide_size_mismatch(self):
        img1 = cplcore.Image.zeros(2, 1, cplcore.Type.DOUBLE)
        img2 = cplcore.Image.zeros(1, 2, cplcore.Type.DOUBLE)

        with pytest.raises(cplcore.IncompatibleInputError):
            img1.divide(img2)

    def test_divide(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)

        img1[0][0] = 8912
        img2[0][0] = -1982

        img1.divide(img2)
        assert img1[0][0] == 8912 / -1982
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.DOUBLE

        assert img2[0][0] == -1982
        assert img2.shape == (1, 1)
        assert img2.type == cplcore.Type.DOUBLE

    def test_add_scalar(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 8912

        img1.add_scalar(-1982)
        assert img1[0][0] == 8912 + -1982
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.DOUBLE

    def test_subtract_scalar(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 8912

        img1.subtract_scalar(-1982)
        assert img1[0][0] == 8912 - -1982
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.DOUBLE

    def test_multiply_scalar(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 8912

        img1.multiply_scalar(-1982)
        assert img1[0][0] == 8912 * -1982
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.DOUBLE

    def test_divide_scalar(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 8912

        img1.divide_scalar(-1982)
        assert img1[0][0] == 8912 / -1982
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.DOUBLE

    def test_and_type_mismatch(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.FLOAT)

        with pytest.raises(cplcore.InvalidTypeError):
            img1.and_with(img2)

    def test_and_size_mismatch(self):
        img1 = cplcore.Image.zeros(2, 1, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(1, 2, cplcore.Type.INT)

        with pytest.raises(cplcore.IncompatibleInputError):
            img1.and_with(img2)

    def test_and(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.INT)

        img1[0][0] = 8912
        img2[0][0] = -1982

        img1.and_with(img2)
        assert img1[0][0] == 8912 & -1982
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.INT

        assert img2[0][0] == -1982
        assert img2.shape == (1, 1)
        assert img2.type == cplcore.Type.INT

    def test_or_type_mismatch(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.FLOAT)

        with pytest.raises(cplcore.InvalidTypeError):
            img1.or_with(img2)

    def test_or_size_mismatch(self):
        img1 = cplcore.Image.zeros(2, 1, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(1, 2, cplcore.Type.INT)

        with pytest.raises(cplcore.IncompatibleInputError):
            img1.or_with(img2)

    def test_or(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.INT)

        img1[0][0] = 8912
        img2[0][0] = -1982

        img1.or_with(img2)
        assert img1[0][0] == 8912 | -1982
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.INT

        assert img2[0][0] == -1982
        assert img2.shape == (1, 1)
        assert img2.type == cplcore.Type.INT

    def test_xor_type_mismatch(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.FLOAT)

        with pytest.raises(cplcore.InvalidTypeError):
            img1.xor_with(img2)

    def test_xor_size_mismatch(self):
        img1 = cplcore.Image.zeros(2, 1, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(1, 2, cplcore.Type.INT)

        with pytest.raises(cplcore.IncompatibleInputError):
            img1.xor_with(img2)

    def test_xor(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.INT)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.INT)

        img1[0][0] = 8912
        img2[0][0] = -1982

        img1.xor_with(img2)
        assert img1[0][0] == 8912 ^ -1982
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.INT

        assert img2[0][0] == -1982
        assert img2.shape == (1, 1)
        assert img2.type == cplcore.Type.INT

    def test_and_scalar(self):
        img = cplcore.Image.zeros(1, 1, cplcore.Type.INT)
        img[0][0] = 90234

        img.and_scalar(29102)  # Unsigned ints only
        assert img[0][0] == 90234 & 29102

    def test_or_scalar(self):
        img = cplcore.Image.zeros(1, 1, cplcore.Type.INT)
        img[0][0] = 90234

        img.or_scalar(29102)  # Unsigned ints only
        assert img[0][0] == 90234 | 29102

    def test_xor_scalar(self):
        img = cplcore.Image.zeros(1, 1, cplcore.Type.INT)
        img[0][0] = 90234

        img.xor_scalar(29102)  # Unsigned ints only
        assert img[0][0] == 90234 ^ 29102

    def test_power(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 91

        img1.power(5 / 2)
        assert math.isclose(img1[0][0], pow(91, 5 / 2), rel_tol=1e-5)
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.DOUBLE

    def test_power_type_mismatch(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE_COMPLEX)

        with pytest.raises(cplcore.InvalidTypeError):
            img1.power(1)

    def test_exponential(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 5 / 2

        img1.exponential(91)
        assert math.isclose(img1[0][0], pow(91, 5 / 2), rel_tol=1e-5)
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.DOUBLE

    def test_exponential_type_mismatch(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE_COMPLEX)

        with pytest.raises(cplcore.InvalidTypeError):
            img1.exponential(1)

    def test_logarithm(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = pow(91, 5 / 2)

        img1.logarithm(91)
        assert math.isclose(img1[0][0], 5 / 2, rel_tol=1e-5)
        assert img1.shape == (1, 1)
        assert img1.type == cplcore.Type.DOUBLE

    def test_logarithm_type_mismatch(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE_COMPLEX)

        with pytest.raises(cplcore.InvalidTypeError):
            img1.logarithm(1)

    def test_abs(self):
        img1 = cplcore.Image.zeros(2, 2, cplcore.Type.DOUBLE)
        img1[0][0] = -8123
        img1[0][1] = 981

        img1.abs()
        assert img1[0][0] == 8123
        assert img1[0][1] == 981
        assert img1.shape == (2, 2)
        assert img1.type == cplcore.Type.DOUBLE

    def test_abs_type_mismatch(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE_COMPLEX)

        with pytest.raises(cplcore.InvalidTypeError):
            img1.abs()

    def test_hypot(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)
        img1[0][0] = 90234
        img2[0][0] = 29102

        img3 = cplcore.Image.hypot(img1, img2)
        assert img3[0][0] == math.hypot(90234, 29102)
        assert img3.shape == (1, 1)
        assert img3.type == cplcore.Type.DOUBLE

        assert img2[0][0] == 29102
        assert img2.shape == (1, 1)
        assert img2.type == cplcore.Type.DOUBLE

    def test_hypot_type_mismatch(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE_COMPLEX)
        img2 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)

        with pytest.raises(cplcore.InvalidTypeError):
            _ = cplcore.Image.hypot(img1, img2)

    def test_negate(self):
        img1 = cplcore.Image.zeros(2, 2, cplcore.Type.INT)
        img1[0][0] = -8123
        img1[1][0] = 981

        img1.negate()
        assert img1[0][0] == ~-8123
        assert img1[1][0] == ~981
        assert img1.shape == (2, 2)
        assert img1.type == cplcore.Type.INT

    def test_negate_type_mismatch(self):
        img1 = cplcore.Image.zeros(1, 1, cplcore.Type.DOUBLE)

        with pytest.raises(cplcore.InvalidTypeError):
            img1.negate()

    def test_extract_outside(self):
        img1 = cplcore.Image.zeros(3, 3, cplcore.Type.DOUBLE_COMPLEX)

        with pytest.raises(cplcore.IllegalInputError):
            # Copies completley outside the range of the above image
            img1.extract((-1, 0, 2, 2))

    def test_extract(self):
        img1 = cplcore.Image.zeros(3, 3, cplcore.Type.DOUBLE_COMPLEX)
        img1[0][0] = -1837
        img1[1][0] = 14751
        img1[2][0] = 2267
        img1[0][1] = -59494
        img1[1][1] = -7984
        img1[2][1] = 20176
        img1[0][2] = -168
        img1[1][2] = -35832
        img1[2][2] = 14924

        img2 = img1.extract((1, 1, 2, 2))
        # Check img1 didn't change
        assert img1[0][0] == -1837
        assert img1[1][0] == 14751
        assert img1[2][0] == 2267
        assert img1[0][1] == -59494
        assert img1[1][1] == -7984
        assert img1[2][1] == 20176
        assert img1[0][2] == -168
        assert img1[1][2] == -35832
        assert img1[2][2] == 14924
        assert img1.shape == (3, 3)
        assert img1.type == cplcore.Type.DOUBLE_COMPLEX

        assert img2[0][0] == -7984
        assert img2[1][0] == 20176
        assert img2[0][1] == -35832
        assert img2[1][1] == 14924
        assert img2.shape == (2, 2)
        assert img2.type == cplcore.Type.DOUBLE_COMPLEX

    def test_shift_outside(self):
        img1 = cplcore.Image.zeros(3, 3, cplcore.Type.DOUBLE_COMPLEX)

        with pytest.raises(cplcore.IllegalInputError):
            # Copies completley outside the range of the above image
            img1.shift(-3, 1)

    def test_shift(self):
        img1 = cplcore.Image.zeros(3, 3, cplcore.Type.DOUBLE_COMPLEX)
        img1[0][0] = -1837
        img1[1][0] = 14751
        img1[2][0] = 2267
        img1[0][1] = -59494
        img1[1][1] = -7984
        img1[2][1] = 20176
        img1[0][2] = -168
        img1[1][2] = -35832
        img1[2][2] = 14924

        img1.shift(1, -2)
        assert img1[0][0] is None
        assert img1[1][0] == -168
        assert img1[2][0] == -35832
        assert img1[0][1] is None
        assert img1[1][1] is None
        assert img1[2][1] is None
        assert img1[0][2] is None
        assert img1[1][2] is None
        assert img1[2][2] is None

    def test_copy_type_mismatch(self):
        img1 = cplcore.Image.zeros(3, 3, cplcore.Type.DOUBLE_COMPLEX)

        with pytest.raises(cplcore.IllegalInputError):
            img1.copy_into(cplcore.Image.zeros(0, 0, cplcore.Type.FLOAT_COMPLEX), 2, 3)

    def test_copy_outside(self):
        img1 = cplcore.Image.zeros(3, 3, cplcore.Type.DOUBLE_COMPLEX)

        with pytest.raises(cplcore.AccessOutOfRangeError):
            # Copies completley outside the range of the above image
            img1.copy_into(cplcore.Image.zeros(2, 2, cplcore.Type.DOUBLE_COMPLEX), 2, 3)

    def test_copy(self):
        img1 = cplcore.Image.zeros(3, 3, cplcore.Type.DOUBLE_COMPLEX)
        img1[0][0] = -8358.33327
        img1[1][0] = 3128.12588
        img1[2][0] = -4153.22000
        img1[0][1] = 51171.49282
        img1[1][1] = 6767.62923
        img1[2][1] = 2250.98820
        img1[0][2] = -803.09425
        img1[1][2] = -430.64858
        img1[2][2] = 189.28972

        img2 = cplcore.Image.zeros(2, 4, cplcore.Type.DOUBLE_COMPLEX)
        img2[0][0] = 53390.30430
        img2[1][0] = 47109.23029
        img2[2][0] = -34898.49157
        img2[2][0] = -27994.50244
        img2[0][1] = 12684.53395
        img2[1][1] = 43749.02437
        img2[2][1] = 13124.31088
        img2[3][1] = 28393.03378

        img1.copy_into(img2, 2, 1)
        assert img1[0][0] == -8358.33327
        assert img1[1][0] == 3128.12588
        assert img1[2][0] == -4153.22000
        assert img1[0][1] == 51171.49282
        assert img1[1][1] == 6767.62923
        assert img1[2][1] == 53390.30430
        assert img1[0][2] == -803.09425

        assert img1[1][2] == -430.64858
        assert img1[2][2] == 12684.53395

        # Ensure not changed
        assert img2[0][0] == 53390.30430
        assert img2[0][1] == 12684.53395
        assert img2[1][0] == 47109.23029
        assert img2[1][1] == 43749.02437

    def test_save(self, tmp_path):
        mask1 = cplcore.Mask(3, 3)

        plist1 = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop1.value = 20
        plist1.append(prop1)

        mask1[1][1] = True

        mask_path_str = str(tmp_path / "mask1.fits")
        mask1.save(mask_path_str, plist1, cplcore.io.CREATE)

        loaded_mask = cplcore.Mask.load(mask_path_str, 0, 0)

        assert loaded_mask[1][1]
        assert loaded_mask.shape == (3, 3)

        loaded_proplist = cplcore.PropertyList.load(mask_path_str, 0)
        assert loaded_proplist["NAMED_PROPERTY1"].value == 20

        mask2 = cplcore.Mask(2, 2)
        mask2[0][0] = True
        mask2.save(mask_path_str, cplcore.PropertyList(), cplcore.io.EXTEND)

        loaded_mask = cplcore.Mask.load(mask_path_str, 0, 1)
        assert loaded_mask[0][0]
        assert loaded_mask.shape == (2, 2)

    def test_iter(self):
        arr = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]])
        im = cplcore.Image(arr)
        iterator_count = 0
        for i, row in enumerate(im):
            iterator_count += 1
            assert np.array_equal(arr[i], row)
        assert iterator_count == 3  # Check if iterator went over 3 times
        # run a second time to check if the iterator reset properly
        for i, row in enumerate(im):
            iterator_count += 1
            assert np.array_equal(arr[i], row)
        assert iterator_count == 6  # Check if it actually went through again

    def test_iter_row(self):
        arr = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]])
        im = cplcore.Image(arr)
        for x in range(3):
            row = im[x]
            iterator_count = 0
            for y, value in enumerate(row):
                iterator_count += 1
                assert row[y] == value
            assert iterator_count == 3  # Check if iterator went over 3 times
            # run a second time to check if the iterator reset properly
            for y, value in enumerate(row):
                iterator_count += 1
                assert row[y] == value
            assert iterator_count == 6  # Check if it actually went through again

    def test_iqe_small_zone(self):
        arr = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]])
        im = cplcore.Image(arr, cplcore.Type.FLOAT)
        with pytest.raises(cplcore.IllegalInputError):
            im.image_quality_est((0, 0, 2, 2))

    def test_iqe(self, make_mock_image):
        arr = make_mock_image(dtype=np.float32, min=10, max=255, width=256, height=512)
        _ = cplcore.Image(arr, cplcore.Type.FLOAT)
        # print(np.array(im))
        # print(im.iqe((2,2,50,50)))

    def test_move(self):
        # Create a image
        im = cplcore.Image(
            np.array(
                [
                    [1, 2, 3],
                    [4, 5, 6],
                    [7, 8, 9],
                    [10, 11, 12],
                    [13, 14, 15],
                    [16, 17, 18],
                ],
                dtype="intc",
            )
        )
        newPos = [9, 8, 7, 6, 5, 4, 3, 2, 1]
        with pytest.raises(ValueError):
            im.move(5, newPos)
        with pytest.raises(ValueError):
            im.move(0, newPos)
        with pytest.raises(ValueError):
            im.move(1, newPos)

        # 16   17   18           6    5    4
        # 13   14   15           3    2    1
        # 10   11   12   ---->  12   11   10
        #  7    8    9           9    8    7
        #  4    5    6          18   17   16
        #  1    2    3          15   14   13

        im.move(3, newPos)
        imRes = cplcore.Image(
            np.array(
                [
                    [15, 14, 13],
                    [18, 17, 16],
                    [9, 8, 7],
                    [12, 11, 10],
                    [3, 2, 1],
                    [6, 5, 4],
                ],
                dtype="intc",
            )
        )
        assert im == imRes

    def test_warp(self):
        IMAGESZ = 64
        nbpoints = 10
        nx = 512
        ny = 512
        xposrate = [0.4, 0.5, 0.8, 0.9, 0.2, 0.5, 0.9, 0.3, 0.1, 0.7]
        yposrate = [0.4, 0.2, 0.3, 0.9, 0.7, 0.8, 0.9, 0.8, 0.7, 0.7]
        xpos = [nx * rate for rate in xposrate]
        ypos = [ny * rate for rate in yposrate]
        sigma = 10.0
        flux = 1000.0
        # Set the background
        test_im = cplcore.Image.create_noise_uniform(
            IMAGESZ, IMAGESZ, cplcore.Type.DOUBLE, -1, 1
        )
        # Put fake stars
        for i in range(nbpoints):
            tmp_im = cplcore.Image.create_gaussian(
                IMAGESZ,
                IMAGESZ,
                cplcore.Type.DOUBLE,
                xpos[i],
                ypos[i],
                10.0,
                sigma,
                sigma,
            )
            tmp_im.multiply_scalar(flux / float(i + 1))
            test_im.add(tmp_im)
        imd = test_im
        _ = imd.cast(cplcore.Type.FLOAT)
        px = cplcore.Polynomial(2)
        py = cplcore.Polynomial(2)
        px.set_coeff([1, 0], 0.945946)
        px.set_coeff([0, 1], -0.135135)
        px.set_coeff([0, 0], -6.75676)
        py.set_coeff([1, 0], -0.202703)
        py.set_coeff([0, 1], 0.743243)
        py.set_coeff([0, 0], -12.8378)
        xyprofile = cplcore.Vector.kernel_profile(
            cplcore.Kernel.DEFAULT, 2.0, int(1 + 2.2 * 1000)
        )

        warped = cplcore.Image.create_jacobian_polynomial_like(test_im, px, py)

        dx = warped.duplicate()
        dy = warped.duplicate()
        dy.multiply_scalar(0)
        dx.multiply_scalar(0)
        dx.add_scalar(1.0)
        dy.add_scalar(-2.0)
        warped = imd.warp(dx, dy, xyprofile, 2, xyprofile, 2)

    def test_warp_polynomial_test_shift(self, make_mock_image):
        # This make take a while as we're checking EVERY pixel
        tolerances = {
            cplcore.Type.INT: 0.0,
            cplcore.Type.FLOAT: np.finfo(np.single).eps,
            cplcore.Type.DOUBLE: np.finfo(np.double).eps,
        }
        IMAGESZ = 64
        xyradius = 2.2
        PIXRANGE = 100
        CPL_KERNEL_TABSPERPIX = 1000
        imtypes = [cplcore.Type.DOUBLE, cplcore.Type.FLOAT, cplcore.Type.INT]
        # Line 93 in cpl_image_resample-test.c

        for dx in range(1 - IMAGESZ, IMAGESZ, int(IMAGESZ / 2)):
            for dy in range(1 - IMAGESZ, IMAGESZ, int(IMAGESZ / 2)):
                # Warp_polynomial_test_shift
                # cpl_image_warp_polynomial_test_shift(IMAGESZ, IMAGESZ, i, j,
                # xyprofile, xyradius);
                px = cplcore.Polynomial(2)
                py = cplcore.Polynomial(2)
                px.set_coeff([0, 0], -dx)
                py.set_coeff([0, 0], -dy)
                px.set_coeff([1, 0], 1.0)
                py.set_coeff([0, 1], 1.0)
                for t in imtypes:
                    trans = cplcore.Image.zeros(IMAGESZ, IMAGESZ, t)
                    temp = cplcore.Image.zeros_like(trans)
                    noise = cplcore.Image.create_noise_uniform_like(
                        trans, -PIXRANGE, PIXRANGE
                    )
                    count = 0
                    while count == 0 or count == IMAGESZ * IMAGESZ:
                        mask = cplcore.Mask.threshold_image(
                            noise, -0.2 * PIXRANGE, 0.2 * PIXRANGE, True
                        )
                        count = mask.count()
                    noise.reject_from_mask(mask)
                    trans.copy_into(noise, 0, 0)
                    trans.shift(dy, dx)
                    imtol = tolerances[t]
                    # check_kernel(temp = temp, ref= trans, warp=noise, px, py,
                    # xyprofile, xyradius, imtol = tolerances[t]);
                    kernels = [
                        cplcore.Kernel.TANH,
                        cplcore.Kernel.SINC,
                        cplcore.Kernel.SINC2,
                        cplcore.Kernel.LANCZOS,
                        cplcore.Kernel.HANN,
                        cplcore.Kernel.NEAREST,
                        cplcore.Kernel.HAMMING,
                    ]
                    imtol0 = (
                        1.0
                        if trans.type == cplcore.Type.INT
                        else imtol * 10.0 * PIXRANGE
                    )
                    for k in kernels:
                        tol = imtol if bool(int(k)) else imtol0
                        xyprofile = cplcore.Vector.kernel_profile(
                            k, xyradius, int(1 + xyradius * CPL_KERNEL_TABSPERPIX)
                        )
                        temp = noise.warp_polynomial(
                            px, py, xyprofile, xyradius, xyprofile, xyradius
                        )
                        # Test against ref
                        # TODO: Right now we can't just use the np.all_close function on the images since the rejected pixels resolve to 0, and
                        #       the images may have different reject pixels. If you find a way to get around this then we should try it
                        for x in range(IMAGESZ):
                            for y in range(IMAGESZ):
                                output_pix = temp[x][y]
                                true_pix = trans[x][y]
                                if output_pix and true_pix:
                                    # only check non-rejected pixels
                                    assert np.isclose(
                                        output_pix, true_pix, xyradius * xyradius * tol
                                    )

    def test_warp_polynomial_test_turn(self, make_mock_image):
        # This make take a while as we check EVERY pixel
        tolerances = {
            cplcore.Type.INT: 0.0,
            cplcore.Type.FLOAT: np.finfo(np.single).eps,
            cplcore.Type.DOUBLE: np.finfo(np.double).eps,
        }
        IMAGESZ = 64
        xyradius = 2.2
        PIXRANGE = 100
        CPL_KERNEL_TABSPERPIX = 1000
        imtypes = [cplcore.Type.DOUBLE, cplcore.Type.FLOAT, cplcore.Type.INT]
        # Line 93 in cpl_image_resample-test.c
        nx = IMAGESZ
        ny = IMAGESZ

        # Warp_polynomial_test_turn
        # cpl_image_warp_polynomial_test_shift(IMAGESZ, IMAGESZ, i, j,
        # xyprofile, xyradius);
        px = cplcore.Polynomial(2)
        py = cplcore.Polynomial(2)
        py.set_coeff([0, 0], 1 + IMAGESZ)
        px.set_coeff([0, 1], 1)
        py.set_coeff([1, 0], -1)
        for t in imtypes:
            noise = cplcore.Image.create_noise_uniform(nx, ny, t, -PIXRANGE, PIXRANGE)
            trans = cplcore.Image.zeros_like(noise)
            count = 0
            # Mask with a mix of elements
            while count == 0 or count == IMAGESZ * IMAGESZ:
                mask = cplcore.Mask.threshold_image(
                    noise, -0.2 * PIXRANGE, 0.2 * PIXRANGE, True
                )
                count = mask.count()
            noise.reject_from_mask(mask)
            trans.copy_into(noise, 0, 0)
            trans.turn(-1)
            imtol = tolerances[t]
            # check_kernel(temp = temp, ref= trans, warp=noise, px, py,
            # xyprofile, xyradius, imtol = tolerances[t]);
            kernels = [
                cplcore.Kernel.TANH,
                cplcore.Kernel.SINC,
                cplcore.Kernel.SINC2,
                cplcore.Kernel.LANCZOS,
                cplcore.Kernel.HANN,
                cplcore.Kernel.NEAREST,
                cplcore.Kernel.HAMMING,
            ]
            imtol0 = 1.0 if trans.type == cplcore.Type.INT else imtol * 10.0 * PIXRANGE
            for k in kernels:
                tol = imtol if bool(int(k)) else imtol0
                xyprofile = cplcore.Vector.kernel_profile(
                    k, xyradius, int(1 + xyradius * CPL_KERNEL_TABSPERPIX)
                )
                temp = noise.warp_polynomial(
                    px, py, xyprofile, xyradius, xyprofile, xyradius
                )
                # Test against ref
                for x in range(IMAGESZ):
                    for y in range(IMAGESZ):
                        output_pix = temp[x][y]
                        true_pix = trans[x][y]
                        if output_pix and true_pix:
                            # only check non-rejected pixels
                            # TODO: Right now we can't just use the np.all_close function on the images since the rejected pixels resolve to 0, and
                            #       the images may have different reject pixels. If you find a way to get around this then we should try it
                            assert np.isclose(
                                output_pix, true_pix, xyradius * xyradius * tol
                            )

    def test_get_interpolated(self, make_mock_image):
        CPL_KERNEL_DEF_WIDTH = 2.0
        CPL_KERNEL_DEF_SAMPLES = 2001
        ttype = [cplcore.Type.INT, cplcore.Type.FLOAT, cplcore.Type.DOUBLE]
        tsize = [50, 500]
        value1 = [2, 3]
        value2 = [5, 7]
        mean = (value1[0] + value1[1] + value2[0] + value2[1]) / 4.0
        txyrad = [CPL_KERNEL_DEF_WIDTH, 24.0, 100.0, 240.0]
        for imsize in tsize:
            c = 0.5 + imsize / 2.0
            dimage = cplcore.Image.zeros(imsize, imsize, cplcore.Type.DOUBLE)
            for j in range(imsize):
                pvalue = value1 if 2 * j < imsize else value2
                for i in range(imsize):
                    dimage[i][j] = pvalue[0 if 2 * i < imsize else 1]
            for imtype in ttype:
                image = dimage if imtype == cplcore.Type.DOUBLE else dimage.cast(imtype)
                for xyradius in txyrad:
                    xyprofile = cplcore.Vector.kernel_profile(
                        cplcore.Kernel.TANH, xyradius, CPL_KERNEL_DEF_SAMPLES
                    )
                    value, confidence = image.get_interpolated(
                        c, c, xyprofile, xyradius, xyprofile, xyradius
                    )
                    # print(mean, value, 4.0*xyradius*np.finfo(np.double).eps)
                    assert np.isclose(
                        value, mean, 4.0 * xyradius * np.finfo(np.double).eps
                    )
                    if xyradius < imsize:
                        assert np.isclose(
                            confidence, 1.0, 30.0 * np.finfo(np.double).eps
                        )
                    else:
                        assert np.isclose(
                            confidence, 1.0, xyradius * np.finfo(np.double).eps
                        )

    def test_get_interpolated_illegal_input(self, make_mock_image):
        CPL_KERNEL_DEF_SAMPLES = 2001
        ttype = [cplcore.Type.INT, cplcore.Type.FLOAT, cplcore.Type.DOUBLE]
        tsize = [50, 500]
        value1 = [2, 3]
        value2 = [5, 7]
        txyrad = [-1.0, 0.0]
        xyprofile = cplcore.Vector.zeros(CPL_KERNEL_DEF_SAMPLES)
        for imsize in tsize:
            c = 0.5 * imsize / 2.0
            dimage = cplcore.Image.zeros(imsize, imsize, cplcore.Type.DOUBLE)
            for j in range(imsize):
                pvalue = value1 if 2 * j < imsize else value2
                for i in range(imsize):
                    dimage[i][j] = pvalue[0 if 2 * i < imsize else 1]
            for imtype in ttype:
                image = dimage if imtype == cplcore.Type.DOUBLE else dimage.cast(imtype)
                for xyradius in txyrad:
                    with pytest.raises(cplcore.IllegalInputError):
                        xyprofile = cplcore.Vector.kernel_profile(
                            cplcore.Kernel.DEFAULT, xyradius, CPL_KERNEL_DEF_SAMPLES
                        )
                    with pytest.raises(cplcore.IllegalInputError):
                        image.get_interpolated(
                            c, c, xyprofile, xyradius, xyprofile, xyradius
                        )

    @pytest.mark.parametrize(
        "nx,ny,hm,hn",
        [
            (8, 8, 0, 0),
            (8, 8, 1, 0),
            (8, 8, 0, 1),
            (8, 8, 1, 1),
            (8, 8, 2, 0),
            (8, 8, 0, 2),
            (8, 8, 2, 2),
            (31, 31, 2, 2),
            (15, 63, 1, 1),
        ],
    )
    def test_filter(self, nx, ny, hm, hn):
        """
        Translated from cpl_image_filter-test.c
        """
        # These tests, ported from CPL, use random numbers and tight tolerances. To get them
        # to consistently pass it is necessary to explictly set the random number seed to
        # a fixed value.
        libc_name = ctypes.util.find_library("c")
        libc = ctypes.CDLL(libc_name)
        libc.srand(2)

        # Filter mode along with whether to use it with cpl_image_filter_mask
        filter_mode = [
            (cplcore.Filter.MEDIAN, True),
            (cplcore.Filter.STDEV, True),
            (cplcore.Filter.STDEV_FAST, True),
            (cplcore.Filter.AVERAGE, True),
            (cplcore.Filter.AVERAGE_FAST, True),
            (cplcore.Filter.LINEAR, False),
            (cplcore.Filter.LINEAR_SCALE, False),
        ]
        # Compare with a different filter on a kernel, also of all ones
        filter_ones = [
            (cplcore.Filter.MEDIAN, True),
            (cplcore.Filter.STDEV_FAST, True),
            (cplcore.Filter.STDEV, True),
            (cplcore.Filter.LINEAR, False),
            (cplcore.Filter.LINEAR, False),
            (cplcore.Filter.MORPHO, False),
            (cplcore.Filter.MORPHO_SCALE, False),
        ]
        pixel_type = [cplcore.Type.INT, cplcore.Type.FLOAT, cplcore.Type.DOUBLE]
        m = 1 + 2 * hm
        n = 1 + 2 * hn
        xmin = -100.0
        xmax = 200.0
        window1 = ~cplcore.Mask(m, n)
        window2 = cplcore.Mask(m + 2, n + 2)
        shift = cplcore.Mask(m, n)
        xwindow1 = cplcore.Matrix.zeros(n, m)
        xwindow2 = cplcore.Matrix.zeros(n + 2, m + 2)
        xshift = cplcore.Matrix.zeros(n, m)
        xwindow1.fill(1.0)
        for j in range(1, n + 1):
            for i in range(1, m + 1):
                # TODO: this made me discover the indexing order for mask and image is reversed compared to CPL.  Need to have a talk on whether to reverse it back.
                # CPL shape: (width, height), numpy/PyCPL shape: (height, width)
                # Matrix is still the same, and all functions still take the same order of params (including the constructors using _new())
                window2[j][i] = True
                xwindow2[j][i] = 1.0

        assert window1.count() == m * n
        assert window2.count() == m * n  # This fails if gm or hn is equal to 0. Hmmmm
        for itype1 in pixel_type:
            for itype2 in pixel_type:
                for (ifilt, ifilt_mask), (ifilt_ones, ifilt_mask_ones) in zip(
                    filter_mode, filter_ones
                ):
                    if ifilt == cplcore.Filter.MEDIAN and itype1 != itype2:
                        continue  # Skip
                    testimg = cplcore.Image.create_noise_uniform(
                        nx, ny, itype1, xmin, xmax
                    )
                    tol = 5e-05 if itype1 == cplcore.Type.FLOAT else 0.0

                    if (
                        m == 1
                        and n == 1
                        and (
                            ifilt is cplcore.Filter.STDEV
                            or ifilt is cplcore.Filter.STDEV_FAST
                        )
                    ):  # Expected to raise DataNo
                        with pytest.raises(cplcore.DataNotFoundError):
                            if ifilt_mask:
                                filtim1 = testimg.filter_mask(
                                    window1, ifilt, cplcore.Border.FILTER, itype2
                                )
                            else:
                                filtim1 = testimg.filter(
                                    xwindow1, ifilt, cplcore.Border.FILTER, itype2
                                )
                        with pytest.raises(cplcore.DataNotFoundError):
                            if ifilt_mask:
                                filtim2 = testimg.filter_mask(
                                    window2, ifilt, cplcore.Border.FILTER, itype2
                                )
                            else:
                                filtim2 = testimg.filter(
                                    xwindow2, ifilt, cplcore.Border.FILTER, itype2
                                )
                        continue  # Go to the next filter mode

                    if ifilt_mask:
                        filtim1 = testimg.filter_mask(
                            window1, ifilt, cplcore.Border.FILTER, itype2
                        )
                        filtim2 = testimg.filter_mask(
                            window2, ifilt, cplcore.Border.FILTER, itype2
                        )
                    else:
                        filtim1 = testimg.filter(
                            xwindow1, ifilt, cplcore.Border.FILTER, itype2
                        )
                        filtim2 = testimg.filter(
                            xwindow2, ifilt, cplcore.Border.FILTER, itype2
                        )

                    if ifilt == cplcore.Filter.MEDIAN:
                        for j in range(ny):
                            for i in range(nx):
                                if j < n or ny - n < j or i < m or nx - m < i:
                                    filtim1.reject(j, i)
                                    filtim2.reject(j, i)
                    else:
                        if (
                            ifilt == cplcore.Filter.AVERAGE_FAST
                            or ifilt == cplcore.Filter.STDEV_FAST
                        ):
                            isfloat = (
                                itype1 == cplcore.Type.FLOAT
                                or itype2 == cplcore.Type.FLOAT
                            )
                            tol = (
                                10.0
                                * (xmax - xmin)
                                * (
                                    np.finfo(np.single).eps
                                    if isfloat
                                    else np.finfo(np.double).eps
                                )
                            )
                        elif (
                            ifilt == cplcore.Filter.STDEV
                            and itype2 == cplcore.Type.DOUBLE
                            and (m * n) > 1
                            and (nx > m)
                            and (ny > n)
                        ):
                            # Verify non-border value
                            stdev = testimg.get_stdev(
                                (nx - m, ny - n, nx - 1, ny - 1)
                            )  # -1 as those were cpl coodinates in the test files
                            filtered = filtim1[ny - hn - 1][nx - hm - 1]
                            if (
                                filtered
                            ):  # get does not return value on a invalid value flag
                                assert np.isclose(
                                    stdev, filtered, 4.0 * np.finfo(np.double).eps
                                )

                        if ifilt == cplcore.Filter.STDEV_FAST:
                            tol = 1.0

                    for filtim1row, filtim2row in zip(filtim1, filtim2):
                        if not np.allclose(filtim1row, filtim2row, atol=tol):
                            print(np.array(filtim1row))
                            print(np.array(filtim2row))
                            print(np.array(ifilt))
                            print(itype2)

                    assert np.allclose(filtim1.as_array(), filtim2.as_array(), atol=tol)

                    if ifilt_ones != ifilt:
                        # Result should equal that of a different filter also with all ones
                        # The additions are reordered (sorted), i.e. different round-off
                        isfloat = (
                            itype1 == cplcore.Type.FLOAT or itype2 == cplcore.Type.FLOAT
                        )
                        tol = (xmax - xmin) * (
                            np.finfo(np.single).eps
                            if isfloat
                            else np.finfo(np.double).eps
                        )
                        if ifilt == cplcore.Filter.LINEAR_SCALE and not isfloat:
                            tol *= float(n * m)
                        elif (
                            ifilt == cplcore.Filter.LINEAR
                            and ifilt_ones == cplcore.Filter.MORPHO
                            and not isfloat
                        ):
                            tol *= 1.5

                        if (
                            ifilt == cplcore.Filter.AVERAGE_FAST
                            or ifilt == cplcore.Filter.AVERAGE
                        ):
                            tol *= (1.0 + float(n)) * (1.0 + float(m))

                        if (
                            ifilt == cplcore.Filter.STDEV_FAST
                            or ifilt_ones == cplcore.Filter.STDEV_FAST
                        ):
                            if itype1 == cplcore.Type.INT or itype2 == cplcore.Type.INT:
                                continue  # Not supported, just continue
                            tol *= 100.0 * (n * m)
                        if ifilt_mask_ones:
                            filtim2 = testimg.filter_mask(
                                window1, ifilt_ones, cplcore.Border.FILTER, itype2
                            )
                        else:
                            print(ifilt_ones)
                            filtim2 = testimg.filter(
                                xwindow1, ifilt_ones, cplcore.Border.FILTER, itype2
                            )
                        if tol >= 0.0:
                            for filtim1row, filtim2row in zip(filtim1, filtim2):
                                for pixel1, pixel2 in zip(filtim1row, filtim2row):
                                    if not np.isclose(pixel1, pixel2, atol=tol):
                                        print("INCORRECT!")
                                        print(np.array(ifilt))
                                        print(np.array(ifilt_ones))
                                        print(np.array(filtim1row))
                                        print(np.array(filtim2row))
                                        print(pixel1, pixel2)
                            assert np.allclose(
                                filtim1.as_array(), filtim2.as_array(), atol=tol
                            )
                    if itype1 == itype2:
                        # if input and output pixel types are identical, a mask with 1 element corresponds to a shift
                        # Try all 1 element masks
                        for j in range(n):
                            for i in range(m):
                                # Empty mask
                                shift = shift ^ shift
                                xshift.fill(0.0)
                                # Set one element
                                shift[j][i] = True

                                xshift[(n - 1) - j][i] = 1.0
                                assert shift.count() == 1
                                # This filter corresponds to a shift of ((hm+1) - i, (hn+1) - j)
                                if (
                                    ifilt == cplcore.Filter.STDEV
                                    or ifilt == cplcore.Filter.STDEV_FAST
                                ):
                                    with pytest.raises(cplcore.DataNotFoundError):
                                        if ifilt_mask:
                                            filtim1 = testimg.filter_mask(
                                                shift,
                                                ifilt,
                                                cplcore.Border.FILTER,
                                                itype2,
                                            )
                                        else:
                                            filtim1 = testimg.filter(
                                                xshift,
                                                ifilt,
                                                cplcore.Border.FILTER,
                                                itype2,
                                            )
                                    continue
                                else:
                                    if ifilt_mask:
                                        filtim1 = testimg.filter_mask(
                                            shift, ifilt, cplcore.Border.FILTER, itype2
                                        )
                                    else:
                                        filtim1 = testimg.filter(
                                            xshift, ifilt, cplcore.Border.FILTER, itype2
                                        )
                                    filtim2.copy_into(testimg, 0, 0)
                                    filtim2.shift(hn - j, hm - i)
                                    np.testing.assert_equal(filtim1, filtim2)

                                if ifilt == cplcore.Filter.MEDIAN:
                                    filtim1 = testimg.filter_mask(
                                        shift, ifilt, cplcore.Border.COPY, itype2
                                    )
                                    if i == hm and j == hn:
                                        np.testing.assert_equal(filtim1, testimg)
                                    filtim1 = testimg.filter_mask(
                                        shift, ifilt, cplcore.Border.ZERO, itype2
                                    )
                                    if i == hm and j == hn:
                                        print(filtim1)
                                        print(testimg)
                                        print(hn, hm)
                                        print(filtim1.as_array()[hn:-hn, hm:-hm])
                                        print(testimg.as_array()[hn:-hn, hm:-hm])
                                        np.testing.assert_equal(
                                            filtim1.as_array()[hn:-hn, hm:-hm],
                                            testimg.as_array()[hn:-hn, hm:-hm],
                                        )

    # Something about this causes collapse_sigclip to fail...
    @pytest.mark.parametrize(
        "nx,ny,hm,hn",
        [
            (8, 8, 0, 0),
            (8, 8, 1, 0),
            (8, 8, 0, 1),
            (8, 8, 1, 1),
            (8, 8, 2, 0),
            (8, 8, 0, 2),
            (8, 8, 2, 2),
            (31, 31, 2, 2),
            (15, 63, 1, 1),
        ],
    )
    def test_filter_type_mismatch(self, nx, ny, hm, hn):
        """
        Check if type mismatch is successfuly thrown when using MEDIAN and the types are different
        """
        pixel_type = [cplcore.Type.INT, cplcore.Type.FLOAT, cplcore.Type.DOUBLE]
        m = 1 + 2 * hm
        n = 1 + 2 * hn
        xmin = -100.0
        xmax = 200.0
        window1 = ~cplcore.Mask(m, n)
        window2 = cplcore.Mask(m + 2, n + 2)
        _ = cplcore.Mask(m, n)
        xwindow1 = cplcore.Matrix.zeros(n, m)
        xwindow2 = cplcore.Matrix.zeros(n + 2, m + 2)
        _ = cplcore.Matrix.zeros(n, m)
        xwindow1.fill(1.0)
        for j in range(1, n + 1):
            for i in range(1, m + 1):
                # TODO: this made me discover the indexing order for mask and image is reversed compared to CPL.  Need to have a talk on whether to reverse it back.
                # CPL shape: (width, height), numpy/PyCPL shape: (height, width)
                # Matrix is still the same, and all functions still take the same order of params (including the constructors using _new())
                window2[j][i] = True
                xwindow2[j][i] = 1.0

        assert window1.count() == m * n
        assert window2.count() == m * n  # This fails if gm or hn is equal to 0. Hmmmm
        for itype1 in pixel_type:
            for itype2 in pixel_type:
                if itype1 != itype2:
                    testimg = cplcore.Image.create_noise_uniform(
                        nx, ny, itype1, xmin, xmax
                    )
                    with pytest.raises(cplcore.TypeMismatchError):
                        _ = testimg.filter_mask(
                            window1,
                            cplcore.Filter.MEDIAN,
                            cplcore.Border.FILTER,
                            itype2,
                        )
                    with pytest.raises(cplcore.TypeMismatchError):
                        _ = testimg.filter_mask(
                            window2,
                            cplcore.Filter.MEDIAN,
                            cplcore.Border.FILTER,
                            itype2,
                        )

    def test_filter_5198(self):
        """
        Verify the border filtering with and without a central bad pixel
        Not deeded when the median of an even sized number of samples is defined as one of the sampled values
        """
        img = cplcore.Image(
            np.array(range(100 * 163)).reshape(163, 100), cplcore.Type.DOUBLE
        )
        m = cplcore.Mask(3, 3)
        res = img.duplicate()
        resbpm = img.duplicate()
        m = ~m
        res = img.filter_mask(
            m, cplcore.Filter.MEDIAN, cplcore.Border.FILTER, cplcore.Type.DOUBLE
        )
        img.reject(30, 30)
        resbpm = img.filter_mask(
            m, cplcore.Filter.MEDIAN, cplcore.Border.FILTER, cplcore.Type.DOUBLE
        )

        for j in range(27, 33):
            for i in range(27, 33):
                res[j][i] = 0.0
                resbpm[j][i] = 0.0
        assert np.array_equal(res, resbpm)

    def test_iqe2(self):
        IMAGEMIN = 185
        IMAGEMAX = 255
        # Generate base image (cpl_image_fill_test_create)
        nbpoints = 10
        nx = 512
        ny = 512
        xposrate = [0.4, 0.5, 0.8, 0.9, 0.2, 0.5, 0.9, 0.3, 0.1, 0.7]
        yposrate = [0.4, 0.2, 0.3, 0.9, 0.7, 0.8, 0.9, 0.8, 0.7, 0.7]
        xpos = [nx * rate for rate in xposrate]
        ypos = [ny * rate for rate in yposrate]
        sigma = 10.0
        flux = 1000.0
        # Set the background
        test_im = cplcore.Image.create_noise_uniform(
            512, 512, cplcore.Type.DOUBLE, -1, 1
        )
        # Put fake stars
        for i in range(nbpoints):
            tmp_im = cplcore.Image.create_gaussian(
                512, 512, cplcore.Type.DOUBLE, xpos[i], ypos[i], 10.0, sigma, sigma
            )
            tmp_im.multiply_scalar(flux / float(i + 1))
            test_im.add(tmp_im)
        imd = test_im
        imf = imd.cast(cplcore.Type.FLOAT)

        res = imf.image_quality_est((IMAGEMIN, IMAGEMIN, IMAGEMAX, IMAGEMAX))
        assert res.size == 7
        print("iqe test result:", res)

    def test_iqe_invalid_type(self):
        IMAGEMIN = 185
        IMAGEMAX = 255
        # Generate base image (cpl_image_fill_test_create)
        nbpoints = 10
        nx = 512
        ny = 512
        xposrate = [0.4, 0.5, 0.8, 0.9, 0.2, 0.5, 0.9, 0.3, 0.1, 0.7]
        yposrate = [0.4, 0.2, 0.3, 0.9, 0.7, 0.8, 0.9, 0.8, 0.7, 0.7]
        xpos = [nx * rate for rate in xposrate]
        ypos = [ny * rate for rate in yposrate]
        sigma = 10.0
        flux = 1000.0
        # Set the background
        test_im = cplcore.Image.create_noise_uniform(
            512, 512, cplcore.Type.DOUBLE, -1, 1
        )
        # Put fake stars
        for i in range(nbpoints):
            tmp_im = cplcore.Image.create_gaussian(
                512, 512, cplcore.Type.DOUBLE, xpos[i], ypos[i], 10.0, sigma, sigma
            )
            tmp_im.multiply_scalar(flux / float(i + 1))
            test_im.add(tmp_im)
        imd = test_im
        with pytest.raises(cplcore.InvalidTypeError):
            imd.image_quality_est((IMAGEMIN, IMAGEMIN, IMAGEMAX, IMAGEMAX))

    def test_iqe_illegal_input(self):
        IMAGESZ = 512
        IMAGEMIN = 185
        IMAGEMAX = 255
        # Generate base image (cpl_image_fill_test_create)
        nbpoints = 10
        nx = 512
        ny = 512
        xposrate = [0.4, 0.5, 0.8, 0.9, 0.2, 0.5, 0.9, 0.3, 0.1, 0.7]
        yposrate = [0.4, 0.2, 0.3, 0.9, 0.7, 0.8, 0.9, 0.8, 0.7, 0.7]
        xpos = [nx * rate for rate in xposrate]
        ypos = [ny * rate for rate in yposrate]
        sigma = 10.0
        flux = 1000.0
        # Set the background
        test_im = cplcore.Image.create_noise_uniform(
            512, 512, cplcore.Type.DOUBLE, -1, 1
        )
        # Put fake stars
        for i in range(nbpoints):
            tmp_im = cplcore.Image.create_gaussian_like(
                test_im, xpos[i], ypos[i], 10.0, sigma, sigma
            )
            tmp_im.multiply_scalar(flux / float(i + 1))
            test_im.add(tmp_im)
        imd = test_im
        imf = imd.cast(cplcore.Type.FLOAT)
        for window in [
            (IMAGESZ, IMAGESZ, IMAGESZ + 1, IMAGESZ + 1),
            (IMAGEMIN, IMAGEMIN, IMAGEMIN + 2, IMAGEMAX),
            (IMAGEMIN, IMAGEMIN, IMAGEMAX, IMAGEMIN + 2),
        ]:
            with pytest.raises(cplcore.IllegalInputError):
                imf.image_quality_est(window)

    def test_create_jacobian_polynomial(self):
        px = cplcore.Polynomial(2)
        py = cplcore.Polynomial(2)
        px.set_coeff([0, 0], 1)
        py.set_coeff([0, 0], 1)
        px.set_coeff([1, 2], 24)
        py.set_coeff([0, 1], 22.42)
        _ = cplcore.Image.create_jacobian_polynomial(2, 2, cplcore.Type.DOUBLE, px, py)

    def test_create_jacobian(self):
        px = cplcore.Image.zeros(2, 2, cplcore.Type.DOUBLE)
        py = cplcore.Image.zeros(2, 2, cplcore.Type.DOUBLE)
        px[0][0] = 1
        py[0][0] = 1
        px[1][1] = 24
        py[0][1] = 22.42
        _ = cplcore.Image.create_jacobian(px, py)

    def test_threshold(self):
        im = cplcore.Image([[1.1, 2.5], [3.21, 4.555]])
        im.threshold(0.1, 62.5, lo_cut=2.4, hi_cut=3)
        assert np.array_equal(im, [[0.1, 2.5], [62.5, 62.5]])

    def test_equals_double(self):
        im = cplcore.Image([[1.1, 2.5], [3.21, 4.555]])
        im2 = cplcore.Image([[1.1, 2.5], [3.21, 4.555]])
        assert im.equals(im2)
        assert im == im2

    def test_equals_int(self):
        im = cplcore.Image([[2, 5], [52, 25]])
        im2 = cplcore.Image([[2, 5], [52, 25]])
        assert im.equals(im2)
        assert im == im2

    def test_equals_int_float_diff(self):
        im = cplcore.Image([[2, 5], [52, 25]])
        im2 = cplcore.Image([[2.0, 5.0], [52.0, 25.0]])
        assert not im.equals(im2)
        assert im != im2

    def test_equals_diff_dimensions(self):
        im = cplcore.Image([[2], [5], [52], [25]])
        im2 = cplcore.Image([[2, 5], [52, 25]])
        assert not im.equals(im2)
        assert im != im2

    def test_equals_diff_values(self):
        im = cplcore.Image([[2, 5], [25, 52]])
        im2 = cplcore.Image([[2, 5], [52, 25]])
        assert not im.equals(im2)
        assert im != im2

    def test_equals_complex(self):
        im = cplcore.Image(
            [[5 + 6j, -3 - 6j, 0], [99 + 94j, -559 + 1j, -50 + 4j]],
            cplcore.Type.DOUBLE_COMPLEX,
        )
        im2 = cplcore.Image(
            [[5 + 6j, -3 - 6j, 0], [99 + 94j, -559 + 1j, -50 + 4j]],
            cplcore.Type.DOUBLE_COMPLEX,
        )
        assert im.equals(im2)
        assert im == im2

    def test_fft_revert_back(self, make_mock_image):
        mock_image = make_mock_image(dtype=np.complex128, width=256, height=256)
        im = cplcore.Image(mock_image, cplcore.Type.DOUBLE_COMPLEX)
        res = im.fft()
        should_be_original = res.fft(inverse=True)
        # Compare with numpy output
        print(np.array(im))
        print(np.array(should_be_original))
        assert np.allclose(im, should_be_original)

    def test_fft_4x4(self, make_mock_image):
        mock_image = make_mock_image(dtype=np.complex128, width=4, height=4)
        im = cplcore.Image(mock_image, cplcore.Type.DOUBLE_COMPLEX)
        npres = np.fft.fft2(im)
        res = im.fft()
        print("Input:")
        print(mock_image)
        print("cpl:")
        print(np.array(res))
        print("numpy:")
        print(npres)
        drs_res = cpldrs.fft.fft_image(im, cpldrs.fft.Mode.FORWARD)
        print("drs:")
        print(np.array(drs_res))
        res_real, res_imag = res.split_real_imag()
        # Compare with numpy output
        assert np.allclose(res_real, npres.real, atol=np.finfo(np.double).eps)

        # TODO: It is known that the CPL convention for FFT is different from fftw
        #      and numpy causing the imaginary component it to be sign flipped.
        #      When compared to numpy.
        #      However it is yet to be checked so for now just compare against
        #      the negation of the imaginary component
        assert np.allclose(
            res_imag, np.negative(npres.imag), atol=np.finfo(np.double).eps
        )

    def test_fft_default(self, make_mock_image):
        mock_image = make_mock_image(dtype=np.complex128, width=256, height=256)
        im = cplcore.Image(mock_image, cplcore.Type.DOUBLE_COMPLEX)
        npres = np.fft.fft2(im)
        res = im.fft()
        res_real, res_imag = res.split_real_imag()
        # Compare with numpy output
        assert np.allclose(res_real, npres.real, atol=np.finfo(np.double).eps)

        # TODO: It is known that the CPL convention for FFT is different from fftw
        #       and numpy causing the imaginary component it to be sign flipped.
        #       When compared to numpy.
        #       However it is yet to be checked so for now just compare against
        #       the negation of the imaginary component
        assert np.allclose(
            res_imag, np.negative(npres.imag), atol=np.finfo(np.double).eps
        )

    def test_fft_double(self, make_mock_image):
        "Test with just a single double image"
        mock_image = make_mock_image(dtype=np.double, width=256, height=256)
        im = cplcore.Image(mock_image, cplcore.Type.DOUBLE)
        res = im.fft()
        res_real, res_imag = res.split_real_imag()
        npres = np.fft.fft2(im)

        # Compare with numpy output
        assert np.allclose(res_real, npres.real, atol=np.finfo(np.double).eps)

        # TODO: It is known that the CPL convention for FFT is different from fftw
        #       and numpy causing the imaginary component it to be sign flipped.
        #       When compared to numpy.
        #       However it is yet to be checked so for now just compare against
        #       the negation of the imaginary component
        assert np.allclose(
            res_imag, np.negative(npres.imag), atol=np.finfo(np.double).eps
        )

    def test_fft_inverse(self, make_mock_image):
        mock_image = make_mock_image(dtype=np.complex128, width=256, height=256)
        im = cplcore.Image(mock_image, cplcore.Type.DOUBLE_COMPLEX)
        npres = np.fft.ifft2(im)
        res = im.fft(inverse=True)
        res_real, res_imag = res.split_real_imag()
        # Compare with numpy output
        assert np.allclose(res_real, npres.real, atol=np.finfo(np.double).eps)

        # TODO: It is known that the CPL convention for FFT is different from fftw
        #       and numpy causing the imaginary component it to be sign flipped.
        #       When compared to numpy.
        #       However it is yet to be checked so for now just compare against
        #       the negation of the imaginary component
        assert np.allclose(
            res_imag, np.negative(npres.imag), atol=np.finfo(np.double).eps
        )

    def test_fft_unnormalized(self, make_mock_image):
        pytest.importorskip(
            "numpy",
            minversion="1.20.0",
            reason="Using kwarg not present until numpy 1.20.0",
        )
        mock_image = make_mock_image(dtype=np.complex128, width=256, height=256)
        im = cplcore.Image(mock_image, cplcore.Type.DOUBLE_COMPLEX)
        npres = np.fft.ifft2(im, norm="forward")
        res = im.fft(inverse=True, unnormalized=True)
        res_real, res_imag = res.split_real_imag()
        assert np.allclose(res_real, npres.real, atol=np.finfo(np.double).eps)

        # TODO: It is known that the CPL convention for FFT is different from fftw
        #       and numpy causing the imaginary component it to be sign flipped.
        #       When compared to numpy.
        #       However it is yet to be checked so for now just compare against
        #       the negation of the imaginary component
        assert np.allclose(
            res_imag, np.negative(npres.imag), atol=np.finfo(np.double).eps
        )

    def test_fft_swap_halves(self, make_mock_image):
        mock_image = make_mock_image(dtype=np.complex128, width=256, height=256)
        im = cplcore.Image(mock_image, cplcore.Type.DOUBLE_COMPLEX)
        npres = np.fft.fft2(im)
        # How CPL "swap halves" is basically just swapping Quadrants around diagonally. 4th->1st, 3rd->2nd, 2nd->3rd, 1st->4th
        # Q1 is upper left, Q2 is upper right, Q3 is lower left, Q4 is lower right
        # Split into quadrants based on https://stackoverflow.com/questions/12811981/slicing-python-matrix-into-quadrants
        Q1, Q2, Q3, Q4 = [
            M for SubA in np.split(npres, 2, axis=0) for M in np.split(SubA, 2, axis=1)
        ]

        # Merge the quadrants back together
        bottom_half = np.concatenate(
            (Q2, Q1), axis=1
        )  # Q1 on the right, Q2 on the left
        top_half = np.concatenate((Q4, Q3), axis=1)  # Q4 on the left, Q3 on the right

        # Merge halves together
        npres_swapped = np.concatenate((top_half, bottom_half), axis=0)

        res = im.fft(swap_halves=True)
        res_real, res_imag = res.split_real_imag()
        # Compare with numpy output
        assert np.allclose(res_real, npres_swapped.real, atol=np.finfo(np.double).eps)

        # TODO: It is known that the CPL convention for FFT is different from fftw
        #      and numpy causing the imaginary component it to be sign flipped.
        #      When compared to numpy.
        #      However it is yet to be checked so for now just compare against
        #      the negation of the imaginary component
        assert np.allclose(
            res_imag, np.negative(npres_swapped.imag), atol=np.finfo(np.double).eps
        )

    @pytest.mark.parametrize(
        "test_array",
        (
            np.array(((1, 2), (3, 4)), dtype=np.intc),
            np.array(((1.1, 2.2), (3.3, 4.4)), dtype=np.single),
            np.array(((1.1, 2.2), (3.3, 4.4)), dtype=np.double),
        ),
        ids=("int", "float", "double"),
    )
    def test_as_array(self, test_array):
        # Convert numpy array to PyCPL Image
        im = cplcore.Image(test_array)
        # Confirm that converting PyCPL Image back to numpy array reproduces original image.
        assert np.all(im.as_array() == test_array)
        assert im.as_array().dtype == test_array.dtype

    @pytest.fixture(
        scope="module",
        params=[
            {"type": cplcore.Type.INT, "expectation": does_not_raise()},
            {"type": cplcore.Type.FLOAT, "expectation": does_not_raise()},
            {"type": cplcore.Type.DOUBLE, "expectation": does_not_raise()},
            {
                "type": cplcore.Type.DOUBLE_COMPLEX,
                "expectation": pytest.raises(cplcore.InvalidTypeError),
            },
        ],  # testing all four datatypes
        ids=["Int", "Float", "Double", "BadComplex"],
    )
    def small_image(self, request):
        img = cplcore.Image(
            [[37, -1837, -9], [451, -34, 1320], [974, -87, 387]],
            dtype=request.param["type"],
        )
        return {"img": img, "expectation": request.param["expectation"]}

    @pytest.fixture(
        scope="module",
        params=[
            {"window": None, "expectation": does_not_raise()},
            {"window": (0, 0, 1, 1), "expectation": does_not_raise()},
            {"window": (1, 1, 2, 2), "expectation": does_not_raise()},
            {
                "window": (9999, 42, 0, 1),
                "expectation": pytest.raises(cplcore.IllegalInputError),
            },
            {
                "window": (-4, 0, -2, 1),
                "expectation": pytest.raises(
                    (cplcore.IllegalInputError, cplcore.AccessOutOfRangeError)
                ),
            },
            {"window": (0.1, 1, 2, 1.95), "expectation": pytest.raises(RuntimeError)},
        ],
        ids=[
            "NoWindow",
            "TopLeft",
            "BottomRight",
            "BadTooBig",
            "BadNegative",
            "BadFloat",
        ],
    )
    def small_window(self, request):
        return request.param

    def test_min(self, small_image, small_window):
        with small_window["expectation"]:
            with small_image["expectation"]:
                if small_window["window"] is None:
                    assert small_image["img"].get_min() == pytest.approx(
                        np.min(small_image["img"])
                    )
                else:
                    assert small_image["img"].get_min(
                        window=small_window["window"]
                    ) == pytest.approx(
                        np.min(small_image["img"].extract(small_window["window"]))
                    )

    def test_max(self, small_image, small_window):
        with small_window["expectation"]:
            with small_image["expectation"]:
                if small_window["window"] is None:
                    assert small_image["img"].get_max() == pytest.approx(
                        np.max(small_image["img"])
                    )
                else:
                    assert small_image["img"].get_max(
                        window=small_window["window"]
                    ) == pytest.approx(
                        np.max(small_image["img"].extract(small_window["window"]))
                    )

    def test_mean(self, small_image, small_window):
        with small_window["expectation"]:
            with small_image["expectation"]:
                if small_window["window"] is None:
                    assert small_image["img"].get_mean() == pytest.approx(
                        np.mean(small_image["img"])
                    )
                else:
                    assert small_image["img"].get_mean(
                        window=small_window["window"]
                    ) == pytest.approx(
                        np.mean(small_image["img"].extract(small_window["window"]))
                    )

    def test_median(self, small_image, small_window):
        with small_window["expectation"]:
            with small_image["expectation"]:
                if small_window["window"] is None:
                    assert small_image["img"].get_median() == pytest.approx(
                        np.median(small_image["img"])
                    )
                else:
                    if small_image["img"].type == cplcore.Type.INT:
                        # pycpl uses integer arithmetic when calculating medians of integer images, numpy doesn't.
                        assert small_image["img"].get_median(
                            window=small_window["window"]
                        ) == math.floor(
                            np.median(
                                small_image["img"].extract(small_window["window"])
                            )
                        )
                    else:
                        assert small_image["img"].get_median(
                            window=small_window["window"]
                        ) == pytest.approx(
                            np.median(
                                small_image["img"].extract(small_window["window"])
                            )
                        )

    def test_stdev(self, small_image, small_window):
        with small_window["expectation"]:
            with small_image["expectation"]:
                if small_window["window"] is None:
                    assert small_image["img"].get_stdev() == pytest.approx(
                        np.std(small_image["img"], ddof=1)
                    )
                else:
                    assert small_image["img"].get_stdev(
                        window=small_window["window"]
                    ) == pytest.approx(
                        np.std(
                            small_image["img"].extract(small_window["window"]), ddof=1
                        )
                    )

    def test_absflux(self, small_image, small_window):
        with small_window["expectation"]:
            with small_image["expectation"]:
                if small_window["window"] is None:
                    assert small_image["img"].get_absflux() == pytest.approx(
                        np.sum(np.absolute(small_image["img"]))
                    )
                else:
                    assert small_image["img"].get_absflux(
                        window=small_window["window"]
                    ) == pytest.approx(
                        np.sum(
                            np.absolute(
                                small_image["img"].extract(small_window["window"])
                            )
                        )
                    )

    def test_get_centroid(self, small_image, small_window):
        with small_window["expectation"]:
            with small_image["expectation"]:
                if small_window["window"] is None:
                    assert small_image["img"].get_centroid_y(), small_image[
                        "img"
                    ].get_centroid_x() == pytest.approx(
                        ndimage.center_of_mass(np.array(small_image["img"]))
                    )
                else:
                    assert small_image["img"].get_centroid_y(
                        window=small_window["window"]
                    ), small_image["img"].get_centroid_x(
                        window=small_window["window"]
                    ) == pytest.approx(
                        ndimage.center_of_mass(
                            np.array(small_image["img"].extract(small_window["window"]))
                        )
                    )

    def test_median_dev(self, small_image, small_window):
        def mad(data, axis=None):
            if data.type == cplcore.Type.INT:
                return np.floor(np.median(data, axis)), np.mean(
                    np.absolute(data - np.floor(np.median(data, axis)), axis)
                )
            else:
                return np.median(data, axis), np.mean(
                    np.absolute(data - np.median(data, axis))
                )

        with small_window["expectation"]:
            with small_image["expectation"]:
                if small_window["window"] is None:
                    my_mad = mad(small_image["img"])
                    assert small_image["img"].get_median_dev() == pytest.approx(my_mad)
                else:
                    my_mad_window = mad(
                        small_image["img"].extract(small_window["window"])
                    )
                    assert small_image["img"].get_median_dev(
                        window=small_window["window"]
                    ) == pytest.approx(my_mad_window)

    def test_mad(self, small_image, small_window):
        def mad(data, axis=None):
            if data.type == cplcore.Type.INT:
                return np.floor(np.median(data, axis)), np.floor(
                    np.median(np.absolute(data - np.floor(np.median(data, axis)), axis))
                )
            else:
                return np.median(data, axis), np.median(
                    np.absolute(data - np.median(data, axis))
                )

        with small_window["expectation"]:
            with small_image["expectation"]:
                if small_window["window"] is None:
                    my_mad = mad(small_image["img"])
                    assert small_image["img"].get_mad() == pytest.approx(my_mad)
                else:
                    my_mad_window = mad(
                        small_image["img"].extract(small_window["window"])
                    )
                    assert small_image["img"].get_mad(
                        window=small_window["window"]
                    ) == pytest.approx(my_mad_window)

    def test_dump_stdout(self):
        # for some reason we can't capture stdout using capsys
        # a cheeky workaround using subprocess does the trick
        cmd = "from cpl import core as cplcore; "
        cmd += "list_2d = [[5, 6], [19, -12]];"
        cmd += "img = cplcore.Image(list_2d, cplcore.Type.INT);"
        cmd += "img.dump()"
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        list_2d = [[5, 6], [19, -12]]
        img = cplcore.Image(list_2d, cplcore.Type.INT)
        expect = """#----- image: 1 <= x <= 2, 1 <= y <= 2 -----
	X	Y	value
	1	1	5
	2	1	6
	1	2	19
	2	2	-12
"""  # noqa
        # first check the output matches __str__ output
        assert str(img) == outp
        assert str(img) == expect

        with pytest.raises(cplcore.AccessOutOfRangeError):
            img.dump(window=(0, 0, 3, 3))
        with pytest.raises(cplcore.IllegalInputError):
            img.dump(window=(5, 0, 1, 1))

    def test_dump_file_img(self, tmp_path, capsys):
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_image_dump.txt"
        filename = tmp_path.joinpath(p)
        list_2d = [[5, 6], [19, -12]]
        img = cplcore.Image(list_2d, cplcore.Type.INT)
        win = (0, 0, img.width - 1, img.height - 1)
        img.dump(filename=str(filename), window=win)

        expect = """#----- image: 1 <= x <= 2, 1 <= y <= 2 -----
	X	Y	value
	1	1	5
	2	1	6
	1	2	19
	2	2	-12
"""  # noqa
        contents = ""
        with open(str(filename), "r") as f:
            for line in f.readlines():
                contents += line
        assert contents == expect

    def test_dump_string(self, tmp_path, capsys):
        list_2d = [[5, 6], [19, -12]]
        img = cplcore.Image(list_2d, cplcore.Type.INT)
        val = img.dump(window=(0, 0, img.width - 1, img.height - 1), show=False)

        assert type(val) == str
        expect = """#----- image: 1 <= x <= 2, 1 <= y <= 2 -----
	X	Y	value
	1	1	5
	2	1	6
	1	2	19
	2	2	-12
"""  # noqa

        assert val == expect
        # test some special cases
        assert img.dump(window=None, show=False) == expect
        assert img.dump(window=(0, 0, 0, 0), show=False) == expect

        with pytest.raises(cplcore.AccessOutOfRangeError):
            img.dump(window=(0, 0, 3, 3), show=False)
        with pytest.raises(cplcore.IllegalInputError):
            img.dump(window=(5, 0, 1, 1), show=False)

    def test_repr(self):
        list_2d = [[5, 6], [19, -12]]
        img = cplcore.Image(list_2d, cplcore.Type.INT)
        repr_val = repr(img).strip()
        assert (
            repr_val == """Image with 2 X 2 pixel(s) of type 'int' and 0 bad pixel(s)"""
        )

    def test_to_type(self):
        list_2d = [[5.1, 6.4], [19.6, -12.88], [4.5352, 9.4938]]
        img = cplcore.Image(list_2d, cplcore.Type.FLOAT)
        assert img.type == cplcore.Type.FLOAT
        # need to add approx as floating point representation (6.400000095367432+0j) == (6.4 + 0j)
        # 5.1 = 5.099999904632568
        assert img[0][0] == pytest.approx(5.1)
        assert img[0][1] == pytest.approx(6.4)
        assert img[1][0] == pytest.approx(19.6)
        assert img[1][1] == pytest.approx(-12.88)
        assert img[2][0] == pytest.approx(4.5352)
        assert img[2][1] == pytest.approx(9.4938)

        fimg = img.to_type(cplcore.Type.INT)
        assert fimg.type == cplcore.Type.INT
        assert fimg[0][0] == 5
        assert fimg[0][1] == 6
        assert fimg[1][0] == 19
        assert fimg[1][1] == -12
        assert fimg[2][0] == 4
        assert fimg[2][1] == 9

        img = img.to_type(cplcore.Type.DOUBLE)
        assert img.type == cplcore.Type.DOUBLE
        assert img[0][0] == pytest.approx(5.1)
        assert img[0][1] == pytest.approx(6.4)
        assert img[1][0] == pytest.approx(19.6)
        assert img[1][1] == pytest.approx(-12.88)
        assert img[2][0] == pytest.approx(4.5352)
        assert img[2][1] == pytest.approx(9.4938)

        img = img.to_type(cplcore.Type.DOUBLE_COMPLEX)
        assert img.type == cplcore.Type.DOUBLE_COMPLEX
        assert img[0][0] == 5.099999904632568 + 0j
        assert img[0][1] == pytest.approx(6.4 + 0j)
        assert img[1][0] == pytest.approx(19.6)
        assert img[1][1] == pytest.approx(-12.88)
        assert img[2][0] == pytest.approx(4.5352)
        assert img[2][1] == pytest.approx(9.4938)

    @pytest.mark.parametrize(
        "pycpl_type,np_type",
        [
            (cplcore.Type.INT, np.intc),
            (cplcore.Type.FLOAT, np.single),
            (cplcore.Type.DOUBLE, np.double),
            (cplcore.Type.FLOAT_COMPLEX, np.csingle),
            (cplcore.Type.DOUBLE_COMPLEX, np.cdouble),
        ],
        ids=("int", "float", "double", "complex-float", "complex-double"),
    )
    def test_to_numpy(self, pycpl_type, np_type):
        img = cplcore.Image([[1, 2, 3], [4, 5, 6]], dtype=pycpl_type)
        arr = np.array([[1, 2, 3], [4, 5, 6]], dtype=np_type)
        # strict keyword only available in numpy >= 1.24.0
        # np.testing.assert_array_equal(img.as_array(), arr, strict=True)
        np.testing.assert_array_equal(img.as_array(), arr)
        assert img.as_array().dtype == arr.dtype

    @pytest.mark.parametrize(
        "pycpl_type,np_type",
        [
            (cplcore.Type.INT, np.intc),
            (cplcore.Type.FLOAT, np.single),
            (cplcore.Type.DOUBLE, np.double),
            (cplcore.Type.FLOAT_COMPLEX, np.csingle),
            (cplcore.Type.DOUBLE_COMPLEX, np.cdouble),
        ],
        ids=("int", "float", "double", "complex-float", "complex-double"),
    )
    def test_implicit_to_numpy(self, pycpl_type, np_type):
        img = cplcore.Image([[1, 2, 3], [4, 5, 6]], dtype=pycpl_type)
        assert np.sum(img) == pytest.approx(21)

    @pytest.mark.parametrize(
        "pycpl_type,np_type",
        [
            (cplcore.Type.INT, np.intc),
            (cplcore.Type.FLOAT, np.single),
            (cplcore.Type.DOUBLE, np.double),
            (cplcore.Type.FLOAT_COMPLEX, np.csingle),
            (cplcore.Type.DOUBLE_COMPLEX, np.cdouble),
        ],
        ids=("int", "float", "double", "complex-float", "complex-double"),
    )
    def test_numpy_round_trip(self, pycpl_type, np_type):
        if pycpl_type in {cplcore.Type.FLOAT_COMPLEX, cplcore.Type.DOUBLE_COMPLEX}:
            pytest.xfail(
                reason="Conversion of numpy arrays to Images does not currently work for arrays with complex types, see PIPE-11023"
            )
        img1 = cplcore.Image([[1, 2, 3], [4, 5, 6]], dtype=pycpl_type)
        np_img = img1.as_array()
        img2 = cplcore.Image(np_img)
        assert img1 == img2

    @pytest.mark.parametrize(
        "border_mode,expectation",
        [
            (
                cplcore.Border.FILTER,
                np.array(
                    [
                        [0, 0, 0, 0, 0],
                        [0, 0, 1, 2, 3],
                        [0, 5, 6, 7, 8],
                        [0, 10, 11, 12, 13],
                        [0, 15, 16, 17, 18],
                    ],
                    dtype=np.intc,
                ),
            ),
            (
                cplcore.Border.COPY,
                np.array(
                    [
                        [0, 1, 2, 3, 4],
                        [5, 0, 1, 2, 9],
                        [10, 5, 6, 7, 14],
                        [15, 10, 11, 12, 19],
                        [20, 21, 22, 23, 24],
                    ],
                    dtype=np.intc,
                ),
            ),
            (
                cplcore.Border.ZERO,
                np.array(
                    [
                        [0, 0, 0, 0, 0],
                        [0, 0, 1, 2, 0],
                        [0, 5, 6, 7, 0],
                        [0, 10, 11, 12, 0],
                        [0, 0, 0, 0, 0],
                    ],
                    dtype=np.intc,
                ),
            ),
            (
                cplcore.Border.CROP,
                np.array([[0, 1, 2], [5, 6, 7], [10, 11, 12]], dtype=np.intc),
            ),
        ],
        ids=("filter", "copy", "zero", "crop"),
    )
    def test_filter_border_modes(self, border_mode, expectation):
        # filter_mask in median filter mode can use all border modes.
        test_image = cplcore.Image(np.arange(0, 25).reshape(5, 5))
        shift_mask = cplcore.Mask(3, 3)
        shift_mask[0][0] = True
        filtered_image = test_image.filter_mask(
            shift_mask,
            filter=cplcore.Filter.MEDIAN,
            border=border_mode,
            dtype=cplcore.Type.INT,
        )
        np.testing.assert_array_equal(filtered_image.as_array(), expectation)

    def test_filter_bad_border_modes(self):
        test_image = cplcore.Image(np.arange(0, 25).reshape(5, 5))
        shift_mask = cplcore.Mask(3, 3)
        shift_mask[0][0] = True

        # NOP doesn't work in PyCPL
        with pytest.raises(cplcore.UnsupportedModeError):
            _ = test_image.filter_mask(
                shift_mask,
                filter=cplcore.Filter.MEDIAN,
                border=cplcore.Border.NOP,
                dtype=cplcore.Type.INT,
            )

        # COPY and CROP can only be used in MEDIAN mode.
        with pytest.raises(cplcore.UnsupportedModeError):
            _ = test_image.filter_mask(
                shift_mask,
                filter=cplcore.Filter.AVERAGE,
                border=cplcore.Border.COPY,
                dtype=cplcore.Type.INT,
            )

        with pytest.raises(cplcore.UnsupportedModeError):
            _ = test_image.filter_mask(
                shift_mask,
                filter=cplcore.Filter.AVERAGE_FAST,
                border=cplcore.Border.CROP,
                dtype=cplcore.Type.INT,
            )

    def test_maxpos_simple(self):
        list_2d = [[1, 2], [3, 4], [5, 6]]
        img = cplcore.Image(list_2d, cplcore.Type.INT)
        assert img.type == cplcore.Type.INT
        assert img.shape == (3, 2)

        assert img.get_maxpos(window=None) == (2, 1)

        win1 = (0, 0, 1, 1)
        assert img.get_maxpos(window=win1) == (1, 1)

        win2 = (1, 1, 2, 2)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img.get_maxpos(window=win2) == (0, 0)

        win3 = (0, 0, 2, 2)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img.get_maxpos(window=win3) == (0, 0)

        win4 = (9999, 42, 0, 1)
        with pytest.raises(cplcore.IllegalInputError):
            img.get_maxpos(window=win4) == (0, 0)
        win5 = (-4, 0, -2, 1)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img.get_maxpos(window=win5) == (0, 0)
        win6 = (0.1, 1, 2, 1.95)
        with pytest.raises(RuntimeError):
            img.get_maxpos(window=win6) == (0, 0)

    def test_minpos_simple(self):
        list_2d = [[1, 2], [3, 4], [5, 6]]
        img = cplcore.Image(list_2d, cplcore.Type.INT)
        assert img.type == cplcore.Type.INT
        assert img.shape == (3, 2)

        assert img.get_minpos(window=None) == (0, 0)

        win1 = (0, 0, 1, 1)
        assert img.get_minpos(window=win1) == (0, 0)

        win2 = (1, 1, 2, 2)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            _ = img.get_minpos(window=win2)

        win3 = (0, 0, 2, 2)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            _ = img.get_minpos(window=win3)

        win4 = (9999, 42, 0, 1)
        with pytest.raises(cplcore.IllegalInputError):
            _ = img.get_minpos(window=win4)
        win5 = (-4, 0, -2, 1)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            _ = img.get_minpos(window=win5)
        win6 = (0.1, 1, 2, 1.95)
        with pytest.raises(RuntimeError):
            _ = img.get_minpos(window=win6)

    def test_interpolated_yx(self):
        list_2d = [[5, 6], [19, -12], [19, -12], [19, -12], [19, -12], [19, -12]]
        img = cplcore.Image(list_2d, cplcore.Type.INT)
        assert img.shape == (6, 2)
        xyprofile = cplcore.Vector.zeros(10)  # cplcore.Vector.zeros(10)
        value, confidence = img.get_interpolated(
            800.5, 20.5, xyprofile, 20.0, xyprofile, 20.0
        )
        # x, y here are not image pixel locations

    def test_extract_subsample(self):
        img = cplcore.Image.zeros(3, 30, cplcore.Type.INT)
        sub_img1 = img.extract_subsample(1, 10)
        assert sub_img1.shape == (30, 1)

        sub_img2 = img.extract_subsample(1, 1)
        assert sub_img2.shape == (30, 3)

        sub_img3 = img.extract_subsample(2, 5)
        assert sub_img3.shape == (15, 1)

    def test_rebin(self):
        img = cplcore.Image.zeros(10, 100, cplcore.Type.INT)
        sub_img1 = img.rebin(1, 1, 1, 1)
        assert sub_img1.shape == (100, 10)
        sub_img2 = img.rebin(1, 1, 5, 5)
        assert sub_img2.shape == (20, 2)
        sub_img3 = img.rebin(1, 1, 50, 5)
        assert sub_img3.shape == (2, 2)

        # Changing statring point of binning
        sub_img4 = img.rebin(50, 5, 1, 1)
        assert sub_img4.shape == (51, 6)
        sub_img5 = img.rebin(50, 5, 5, 5)
        assert sub_img5.shape == (10, 1)
        sub_img6 = img.rebin(50, 5, 2, 1)
        assert sub_img6.shape == (25, 6)

    def test_accept_yx(self):
        img = cplcore.Image.zeros(10, 50, cplcore.Type.DOUBLE)
        # Store something as the rejected pixel
        img[0][0] = 5
        img.reject(0, 0)
        img.accept(0, 0)

        # Check it's accepted
        assert img[0][0] == 5
        assert not img.bpm[0][0]
        assert not img.is_rejected(0, 0)

        img[40][7] = 5
        img.reject(40, 7)
        img.accept(40, 7)
        # Check it's accepted
        assert img[40][7] == 5
        assert not img.bpm[40][7]
        assert not img.is_rejected(40, 7)

    def test_data_types(self):
        img = cplcore.Image.zeros(10, 50, cplcore.Type.INT)
        assert img[40][7] == 0
        assert img[7] is not None
        assert img[40] is not None
        img = cplcore.Image.zeros(100, 50, cplcore.Type.FLOAT)
        assert img[40][7] == 0.0
        assert img[7] is not None
        with pytest.raises(IndexError):
            _ = img[70]
            _ = img[11]

        img = cplcore.Image.zeros(10, 50, cplcore.Type.DOUBLE)
        assert img[40][7] == 0.0
        assert img[7] is not None
        with pytest.raises(IndexError):
            _ = img[11]
            _ = img[70]
        img = cplcore.Image.zeros(10, 50, cplcore.Type.DOUBLE_COMPLEX)
        assert img[40][7] == 0.0 + 0j
        assert img[7] is not None
        with pytest.raises(IndexError):
            _ = img[70]
            _ = img[11]
        img = cplcore.Image.zeros(10, 50, cplcore.Type.FLOAT_COMPLEX)
        assert img[40][7] == 0.0 + 0j
        assert img[7] is not None
        with pytest.raises(IndexError):
            _ = img[70]
            _ = img[11]
        # cannot make long_long img

    def test_getter(self):
        list_2d = np.array([[1, 2], [3, 4], [5, 6]], np.intc)
        img = cplcore.Image(list_2d)
        assert img.type == cplcore.Type.INT

        assert img.shape == list_2d.shape
        assert img.width == 2
        assert img.height == 3
        assert img.size == 6
        assert img[0][0] == 1
        assert img[0][1] == 2
        assert img[1][0] == 3
        assert img[1][1] == 4
        assert img[2][0] == 5
        assert img[2][1] == 6
        assert img.shape == (3, 2)
        assert img[0] is not None
        assert img[1] is not None
        assert img[2] is not None
        with pytest.raises(IndexError):
            _ = img[3]

    def test_getter_numpy_double(self):
        list_2d = np.array(
            [[5.90123, -918203123.90], [5.10123, -918203123.10]], np.double
        )
        img1 = cplcore.Image(list_2d)

        assert img1[0][0] == 5.90123
        assert img1[0][1] == -918203123.90
        assert img1[1][0] == 5.10123
        assert img1[1][1] == -918203123.10

    def test_setter_numpy_double(self):
        list_2d = np.array(
            [[5.90123, -918203123.90], [5.10123, -918203123.10]], np.double
        )
        img1 = cplcore.Image(list_2d)

        assert img1[0][0] == 5.90123
        assert img1[0][1] == -918203123.90
        assert img1[1][0] == 5.10123
        assert img1[1][1] == -918203123.10

        img2 = cplcore.Image(img1)
        img2[0][0] = 5.90123
        img2[0][1] = -918203123.90
        img2[1][0] = 5.10123
        img2[1][1] = -918203123.10

        assert img2[0][0] == 5.90123
        assert img2[0][1] == -918203123.90
        assert img2[1][0] == 5.10123
        assert img2[1][1] == -918203123.10

    def test_get_float(self):
        list_2d = [[5.1, 6.4], [19.6, -12.88], [4.5352, 9.4938]]
        img = cplcore.Image(list_2d, cplcore.Type.FLOAT)
        assert img.type == cplcore.Type.FLOAT
        # need to add approx as floating point representation (6.400000095367432+0j) == (6.4 + 0j)
        # 5.1 = 5.099999904632568
        assert img[0][0] == pytest.approx(5.1)
        assert img[0][1] == pytest.approx(6.4)
        assert img[1][0] == pytest.approx(19.6)
        assert img[1][1] == pytest.approx(-12.88)
        assert img[2][0] == pytest.approx(4.5352)
        assert img[2][1] == pytest.approx(9.4938)

        img2 = cplcore.Image.zeros(2, 3, cplcore.Type.FLOAT)
        assert img2[0][0] == 0.0
        assert img2[0][1] == 0.0
        assert img2[1][0] == 0.0

    def test_get_pixels_int(self):
        img = cplcore.Image(
            [[1, 2, 3], [4, 5, 6], [7, 8, 9], [10, 11, 12], [13, 14, 15], [16, 17, 18]],
            dtype=cplcore.Type.INT,
        )
        assert img.shape == (6, 3)  # (height, width) = (y, x)
        assert img.type == cplcore.Type.INT
        assert img.get_pixels(0, 1) == [1.0]
        assert img.get_pixels(0, 4) == [1.0, 2.0, 3.0, 4.0]
        assert img.get_pixels(2, 4) == [3.0, 4.0, 5.0, 6.0]
        assert img.get_pixels(5, 4) == [6.0, 7.0, 8.0, 9.0]
        assert img.get_pixel(0, 0) == 1
        assert img.get_pixel(0, 1) == 2
        assert img.get_pixel(1, 0) == 4
        assert img.get_pixel(1, 1) == 5
        assert img.get_pixel(5, 1) == 17
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img.get_pixel(5, 2) == 11
            img.get_pixel(6, 1) == 11

    def test_get_pixels_float(self):
        img = cplcore.Image(
            [
                [1.1, 2.1, 3.1],
                [4.22, 5.22, 6.22],
                [7.333, 8.333, 9.333],
                [10.4, 11.4, 12.4],
            ],
            dtype=cplcore.Type.FLOAT,
        )
        assert img.shape == (4, 3)  # (height, width) = (y, x)
        assert img.type == cplcore.Type.FLOAT
        assert img.get_pixels(0, 1) == pytest.approx([1.1])
        assert img.get_pixels(0, 4) == pytest.approx([1.1, 2.1, 3.1, 4.22])
        assert img.get_pixels(2, 4) == pytest.approx([3.1, 4.22, 5.22, 6.22])
        assert img.get_pixels(5, 4) == pytest.approx([6.22, 7.333, 8.333, 9.333])

        assert img.get_pixel(0, 0) == pytest.approx(1.1)
        assert img.get_pixel(0, 1) == pytest.approx(2.1)
        assert img.get_pixel(1, 0) == pytest.approx(4.22)
        assert img.get_pixel(1, 1) == pytest.approx(5.22)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            assert img.get_pixel(4, 1) == 11.4

    def test_get_pixels_double(self):
        img = cplcore.Image(
            [
                [-1.0e50, 1.0e-50, 3.0e12],
                [-4.0e50, 5.0e-50, 6.0e12],
                [-7.0e50, 8.0e-50, 9.0e12],
                [-10.0e50, 11.0e-50, 12.0e12],
            ],
            dtype=cplcore.Type.DOUBLE,
        )
        assert img.shape == (4, 3)  # (height, width) = (y, x)
        assert img.type == cplcore.Type.DOUBLE
        assert img.get_pixels(0, 1) == [-1.0e50]
        assert img.get_pixels(0, 4) == [-1e50, 1e-50, 3000000000000.0, -4e50]
        assert img.get_pixels(2, 4) == [3.0e12, -4.0e50, 5.0e-50, 6.0e12]
        assert img.get_pixels(5, 4) == [6.0e12, -7.0e50, 8.0e-50, 9.0e12]

        assert img.get_pixel(0, 0) == -1.0e50
        assert img.get_pixel(0, 1) == 1.0e-50
        assert img.get_pixel(1, 0) == -4.0e50
        assert img.get_pixel(1, 1) == 5.0e-50
        with pytest.raises(cplcore.AccessOutOfRangeError):
            assert img.get_pixel(4, 1) == 11.4

    def test_get_pixels_float_complex(self):
        img = cplcore.Image(
            [[5 + 6j, -3 - 6j, 0], [99 + 94j, -559 + 1j, -50 + 4j]],
            dtype=cplcore.Type.FLOAT_COMPLEX,
        )
        assert img.shape == (2, 3)  # (height, width) = (y, x)
        assert img.type == cplcore.Type.FLOAT_COMPLEX
        assert img.get_pixels(0, 1) == [5 + 6j]
        assert img.get_pixels(0, 4) == [(5 + 6j), (-3 - 6j), 0j, (99 + 94j)]
        with pytest.raises(cplcore.AccessOutOfRangeError):
            assert img.get_pixels(8, 6) == 8

        assert img.get_pixel(0, 0) == 5 + 6j
        assert img.get_pixel(0, 1) == -3 - 6j
        assert img.get_pixel(1, 0) == 99 + 94j
        assert img.get_pixel(1, 1) == -559 + 1j
        with pytest.raises(cplcore.AccessOutOfRangeError):
            assert img.get_pixel(4, 1) == 11.4

    def test_get_pixels_double_complex(self):
        img = cplcore.Image(
            [[1e-40 + 1e50j, -3e10 - 6e10j, 0], [99 + 94j, -559 + 1j, -50 + 4j]],
            dtype=cplcore.Type.DOUBLE_COMPLEX,
        )
        assert img.shape == (2, 3)  # (height, width) = (y, x)
        assert img.type == cplcore.Type.DOUBLE_COMPLEX
        assert img.get_pixels(0, 1) == [1e-40 + 1e50j]
        assert img.get_pixels(0, 4) == [
            (1e-40 + 1e50j),
            (-3e10 - 6e10j),
            0j,
            (99 + 94j),
        ]
        with pytest.raises(cplcore.AccessOutOfRangeError):
            assert img.get_pixels(6, 8) == 9

        assert img.get_pixel(0, 0) == 1e-40 + 1e50j
        assert img.get_pixel(0, 1) == -3e10 - 6e10j
        assert img.get_pixel(1, 0) == 99 + 94j
        assert img.get_pixel(1, 1) == -559 + 1j
        with pytest.raises(cplcore.AccessOutOfRangeError):
            assert img.get_pixel(4, 1) == 11.4

    def test_set_pixels_int(self):
        list_2d = [[0, 1], [2, 3], [4, -5], [6, 7], [8, 9], [10, 11]]
        img = cplcore.Image(list_2d, cplcore.Type.INT)
        # first check from zero
        img.set_pixels([-10, -20, -30], 0)

        assert img[0][0] == -10
        assert img[0][1] == -20
        assert img[1][0] == -30

        # check that the rejection works using None
        img.set_pixels([-10, None, -30], 0)
        assert img[0][1] is None
        assert img.is_rejected(0, 1)

        # now try in the middle of the image
        img.set_pixels([-40, -50, -60], 6)
        assert img[3][0] == -40
        assert img[3][1] == -50
        assert img[4][0] == -60

        # check that the rejection works using None
        img.set_pixels([-40, None, -60], 6)
        assert img[3][1] is None
        assert img.is_rejected(3, 1)

        # try to set pixels outside the image
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img.set_pixels([-70, -80, -90], 11)

    def test_set_pixel_int(self):
        list_2d = [[0, 1], [2, 3], [4, -5], [6, 7], [8, 9], [10, 11]]
        img = cplcore.Image(list_2d, cplcore.Type.INT)
        assert img.shape == (6, 2)  # (height, width) = (y, x)
        assert img.type == cplcore.Type.INT

        img.set_pixel(0, 1, 75764)
        assert img[0][0] == 0
        assert img[0][1] == 75764
        assert img[1][0] == 2
        assert img[1][1] == 3

        img.set_pixel(1, 1, 75764)
        assert img[0][0] == 0
        assert img[0][1] == 75764
        assert img[1][0] == 2
        assert img[1][1] == 75764
        assert img.count_rejected() == 0

        img.set_pixels([0, 1, 2], 0)
        assert img.count_rejected() == 0
        img.set_pixels([0, 1, 2, 3], 2)
        assert img.count_rejected() == 0

        img1 = cplcore.Image.zeros(2, 3, cplcore.Type.INT)
        img1.set_pixels([4666, 5666, 6666], 0)
        assert img1[0][0] == 4666
        assert img1[0][1] == 5666
        assert img1[1][0] == 6666
        assert img1[1][1] == 0
        assert img1.count_rejected() == 0

    def test_set_pixels_double(self):
        list_2d = [
            [0e10, 1e10],
            [2e10, 3e10],
            [4e10, -5e10],
            [6e10, 7e10],
            [8e10, 9e10],
            [10e10, 11e10],
        ]
        img = cplcore.Image(list_2d, cplcore.Type.DOUBLE)
        # first check from zero
        img.set_pixels([-10e20, -20e20, -30e20], 0)

        assert img[0][0] == -10e20
        assert img[0][1] == -20e20
        assert img[1][0] == -30e20

        # check that the rejection works using None
        img.set_pixels([-10e20, None, -30e20], 0)
        assert img[0][1] is None
        assert img.is_rejected(0, 1)

        # now try in the middle of the image
        img.set_pixels([-40e20, -50e20, -60e20], 6)
        assert img[3][0] == -40e20
        assert img[3][1] == -50e20
        assert img[4][0] == -60e20

        # check that the rejection works using None
        img.set_pixels([-40e20, None, -60e20], 6)
        assert img[3][1] is None
        assert img.is_rejected(3, 1)

        # try to set pixels outside the image
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img.set_pixels([-70e3, -80e3, -90e3], 11)

    def test_set_pixel_double(self):
        img = cplcore.Image(
            [
                [-1.0e50, 1.0e-50, 3.0e12],
                [-4.0e50, 5.0e-50, 6.0e12],
                [-7.0e50, 8.0e-50, 9.0e12],
                [-10.0e50, 11.0e-50, 12.0e12],
            ],
            dtype=cplcore.Type.DOUBLE,
        )
        assert img.shape == (4, 3)  # (height, width) = (y, x)
        assert img.type == cplcore.Type.DOUBLE

        img.set_pixel(0, 1, 75764e2)
        assert img[0][0] == -1.0e50
        assert img[0][1] == 75764e2
        assert img[1][0] == -4.0e50
        assert img[1][1] == 5.0e-50

        img.set_pixel(1, 1, 75764)
        assert img[0][0] == -1.0e50
        assert img[0][1] == 75764e2
        assert img[1][0] == -4.0e50
        assert img[1][1] == 75764
        assert img.count_rejected() == 0

        img.set_pixels([0, 1e4, 2e-4], 0)
        assert img.count_rejected() == 0
        img.set_pixels([0, 1e-5, 2e3, 3e3], 2)
        assert img.count_rejected() == 0

        img1 = cplcore.Image.zeros(2, 3, cplcore.Type.DOUBLE)
        img1.set_pixels([4666e2, 5666e2, 6666e2], 0)
        assert img1[0][0] == 4666e2
        assert img1[0][1] == 5666e2
        assert img1[1][0] == 6666e2
        assert img1[1][1] == 0
        assert img1.count_rejected() == 0

    def test_set_pixels_float(self):
        list_2d = [
            [0.1, 1.1],
            [2.1, 3.1],
            [4.1, -5.1],
            [6.1, 7.1],
            [8.1, 9.1],
            [10.1, 11.1],
        ]
        img = cplcore.Image(list_2d, cplcore.Type.FLOAT)
        # first check from zero
        img.set_pixels([-10.23423, -20.872367812, -30.863912], 0)

        assert img[0][0] == pytest.approx(-10.23423)
        assert img[0][1] == pytest.approx(-20.872367812)
        assert img[1][0] == pytest.approx(-30.863912)

        # check that the rejection works using None
        img.set_pixels([-10, None, -30], 0)
        assert img[0][1] is None
        assert img.is_rejected(0, 1)

        # now try in the middle of the image
        img.set_pixels([-40.5, -50.66, -60.77], 6)
        assert img[3][0] == pytest.approx(-40.5)
        assert img[3][1] == pytest.approx(-50.66)
        assert img[4][0] == pytest.approx(-60.77)

        # check that the rejection works using None
        img.set_pixels([-40, None, -60], 6)
        assert img[3][1] is None
        assert img.is_rejected(3, 1)

        # try to set pixels outside the image
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img.set_pixels([-70, -80, -90], 11)

    def set_pixel_float(self):
        list_2d = np.array([[0.1, 1.1], [2.22, 3.22], [4.3, -5.3], [6.444, 7.444]])
        img = cplcore.Image(list_2d, cplcore.Type.FLOAT)
        assert img.shape == (4, 2)  # (height, width) = (y, x)
        assert img.type == cplcore.Type.FLOAT

        img.set_pixel(0, 1, 7576.4)
        assert img[0][0] == 0.1
        assert img[0][1] == 7576.4

        img.set_pixel(1, 1, 75764)
        assert img[0][0] == 0.1
        assert img[0][1] == 75764.4
        assert img[1][0] == 2.22
        assert img[1][1] == 75764
        assert img.count_rejected() == 0

        img.set_pixels([0, 1, 2], 0)
        assert img.count_rejected() == 0
        img.set_pixels([0, 1, 2, 3], 2)
        assert img.count_rejected() == 2

        img1 = cplcore.Image.zeros(2, 3, cplcore.Type.FLOAT)
        img1.set_pixels([4666, 5666, 6666], 0)
        assert img1[0][0] == 6666
        assert img1[0][1] == 5666
        assert img1[1][0] == 0
        assert img1[1][1] == 0
        assert img1.count_rejected() == 0

    def test_set_pixels_float_complex(self):
        list_2d = [
            [0.1 + 3j, 1.1 + 3j],
            [2.1 + 3j, 3.1 + 3j],
            [4.1 + 3.11j, -5.1 + 3.11j],
            [6.1 + 3.22j, 7.1 + 3.22j],
            [8.1 + 3j, 9.1 + 3j],
            [10.1 + 3j, 11.1 + 3j],
        ]
        img = cplcore.Image(list_2d, cplcore.Type.FLOAT_COMPLEX)
        # first check from zero
        img.set_pixels([-10 + 23j, -20 - 88.8j, -30 + 863912j], 0)

        assert img[0][0] == pytest.approx(-10 + 23j)
        assert img[0][1] == pytest.approx(-20 - 88.8j)
        assert img[1][0] == pytest.approx(-30 + 863912j)

        # check that the rejection works using None
        img.set_pixels([-10j, None, -30], 0)
        assert img[0][1] is None
        assert img.is_rejected(0, 1)

        # now try in the middle of the image
        img.set_pixels([-40, -50 + 2.22j, -60.7 - 9.2j], 6)
        assert img[3][0] == -40
        assert img[3][1] == pytest.approx(-50 + 2.22j)
        assert img[4][0] == pytest.approx(-60.7 - 9.2j)

        # check that the rejection works using None
        img.set_pixels([-40 + 8j, None, -60 - 9j], 6)
        assert img[3][1] is None
        assert img.is_rejected(3, 1)

        # try to set pixels outside the image
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img.set_pixels([-70, -80 + 66j, -90], 11)

    def set_pixel_float_complex(self):
        list_2d = np.array([5 + 6j, -3 - 6j, 0], [99 + 94j, -559 + 1j, -50 + 4j])
        img = cplcore.Image(list_2d, cplcore.Type.FLOAT_COMPLEX)
        assert img.shape == (2, 3)  # (height, width) = (y, x)
        assert img.type == cplcore.Type.FLOAT_COMPLEX

        img.set_pixel(0, 1, 7576.4 + 2j)
        assert img[0][0] == 5 + 6j
        assert img[0][1] == 7576.4 + 2j

        img.set_pixel(1, 1, 75764)
        assert img[0][0] == 5 + 6j
        assert img[0][1] == 75764.4 + 2j
        assert img[1][0] == 99 + 94j
        assert img[1][1] == 75764
        assert img.count_rejected() == 0

        img.set_pixels([0, 1, 2], 0)
        assert img.count_rejected() == 0
        img.set_pixels([0, 1, 2, 3], 2)
        assert img.count_rejected() == 2

        img1 = cplcore.Image.zeros(2, 3, cplcore.Type.FLOAT_COMPLEX)
        img1.set_pixels([46 + 66j, 56 + 66j, 66 + 66j], 0)
        assert img1[0][0] == 66 + 66j
        assert img1[0][1] == 56 + 66j
        assert img1[1][0] == 0
        assert img1[1][1] == 0
        assert img1.count_rejected() == 0

    def test_set_pixels_double_complex(self):
        list_2d = [
            [5e5 + 6j, -3e3 - 6e6j, 0],
            [99e9 + 94e9j, -559e5 + 1e1j, -50 + 4e4j],
        ]
        img = cplcore.Image(list_2d, cplcore.Type.DOUBLE_COMPLEX)
        # first check from zero
        img.set_pixels([-10e2 + 23j, -20 - 88.8e2j, -30e2 + 863912j], 0)

        assert img[0][0] == -10e2 + 23j
        assert img[0][1] == -20 - 88.8e2j
        assert img[1][0] == 99e9 + 94e9j

        # check that the rejection works using None
        img.set_pixels([-10, None, -30], 0)
        assert img[0][1] is None
        assert img.is_rejected(0, 1)

        # now try in the middle of the image
        img.set_pixels([-40, -50 - 88.8e2j, -60 - 88.8e2j], 3)
        assert img[1][0] == -40
        assert img[1][1] == -50 - 88.8e2j
        assert img[1][2] == -60 - 88.8e2j

        # check that the rejection works using None
        img.set_pixels([-40, None, -60], 3)
        assert img[1][1] is None
        assert img.is_rejected(1, 1)

        # try to set pixels outside the image
        with pytest.raises(cplcore.AccessOutOfRangeError):
            img.set_pixels([-70, -80 - 88.8e2j, -90], 11)

    def set_pixel_double_complex(self):
        list_2d = np.array(
            [5e5 + 6j, -3e3 - 6e6j, 0], [99e9 + 94e9j, -559e5 + 1e1j, -50 + 4e4j]
        )
        img = cplcore.Image(list_2d, cplcore.Type.DOUBLE_COMPLEX)
        assert img.shape == (2, 3)  # (height, width) = (y, x)
        assert img.type == cplcore.Type.DOUBLE_COMPLEX

        img.set_pixel(0, 1, 7576e4 + 2j)
        assert img[0][0] == 5e5 + 6j
        assert img[0][1] == 7576e4 + 2j

        img.set_pixel(1, 1, 75764)
        assert img[0][0] == 5 + 6j
        assert img[0][1] == 75764e4 + 2j
        assert img[1][0] == 99 + 94j
        assert img[1][1] == 75764
        assert img.count_rejected() == 0

        img.set_pixels([0, 1, 2], 0)
        assert img.count_rejected() == 0
        img.set_pixels([0, 1, 2, 3], 2)
        assert img.count_rejected() == 2

        img1 = cplcore.Image.zeros(2, 3, cplcore.Type.FLOAT_COMPLEX)
        img1.set_pixels([4e6 + 66j, 56 + 6e6j, 6e6 + 66j], 0)
        assert img1[0][0] == 6e6 + 66j
        assert img1[0][1] == 56 + 6e6j
        assert img1[1][0] == 0
        assert img1[1][1] == 0
        assert img1.count_rejected() == 0

    def test_normalise_scale(self):
        img1 = cplcore.Image.zeros(2, 2, cplcore.Type.DOUBLE)
        img1[0][0] = 2e5
        img1[0][1] = -0.81
        img1[1][0] = 90812.89
        img1[1][1] = -45e-10
        img1.normalise(cplcore.Image.Normalise.SCALE)
        assert img1[0][0] == 1
        assert img1[0][1] == 0
        assert img1[1][0] == pytest.approx(0.4540666610300228)
        assert img1[1][1] == pytest.approx(4.049983575066521e-06)
        assert img1.shape == (2, 2)
        assert img1.type == cplcore.Type.DOUBLE

    def test_turn(self):
        list_2d = np.array([[1.1, 2.2], [3.33, 4.44]])
        img = cplcore.Image(list_2d, cplcore.Type.DOUBLE)
        assert img.shape == (2, 2)  # (height, width) = (y, x)

        img.turn(1)
        assert img[0][0] == pytest.approx(2.2)
        assert img[0][1] == 4.44
        assert img[1][0] == 1.1
        assert img[1][1] == 3.33

        img.turn(2)
        assert img[0][0] == 3.33
        assert img[0][1] == 1.1
        assert img[1][0] == 4.44
        assert img[1][1] == 2.2

        img1 = cplcore.Image.zeros(2, 3, cplcore.Type.FLOAT_COMPLEX)
        img1.set_pixels([4e6 + 66j, 56 + 6e6j, 6e6 + 66j], 0)
        with pytest.raises(cplcore.InvalidTypeError):
            _ = img1.turn(2)

    def test_get_fwhm(self):
        list_2d = np.array([[0.1, 1.1], [2.22, 3.22], [4.3, -5.3]])
        img = cplcore.Image(list_2d, cplcore.Type.FLOAT)
        assert img.shape == (3, 2)  # (height, width) = (y, x)
        assert img.type == cplcore.Type.FLOAT

        assert img.get_fwhm(0, 0) == (None, None)
        assert img.get_fwhm(0, 1) == (None, None)
        assert img.get_fwhm(1, 1) == (None, None)
        assert img.get_fwhm(1, 1) == (None, None)
        assert img.get_fwhm(2, 0) == (None, None)

        with pytest.raises(cplcore.DataNotFoundError):
            assert img.get_fwhm(2, 1) == (None, None)

        with pytest.raises(cplcore.AccessOutOfRangeError):
            img.get_fwhm(5, 10)

        img.reject(0, 1)
        with pytest.raises(cplcore.DataNotFoundError):
            assert img.get_fwhm(0, 1) == (None, None)
