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

from astropy.io import fits
import numpy as np
import pytest
import subprocess

from cpl import core as cplcore


class TestMask:
    def test_constructor_zero_detected(self):
        with pytest.raises(cplcore.IllegalInputError):
            _ = cplcore.Mask(0, 0)

    def test_constructor_sets_properties(self):
        new_mask = cplcore.Mask(1, 1)
        assert new_mask.shape == (1, 1)
        assert new_mask.width == 1
        assert new_mask.height == 1
        assert not new_mask[0][0]
        assert new_mask.size == new_mask.width * new_mask.height

    def test_constructor_from_list_2d(self):
        list_2d = [[False, True], [True, False]]
        msk = cplcore.Mask(list_2d)
        assert msk.shape == (2, 2)
        assert msk.width == 2
        assert msk.height == 2
        assert msk.size == 4
        assert not msk[0][0]
        assert msk[0][1]
        assert msk[1][0]
        assert not msk[1][1]

    def test_constructor_from_ndarray_2d(self):
        list_2d = np.array([[False, True], [True, False]])
        msk = cplcore.Mask(list_2d)
        assert msk.shape == (2, 2)
        assert msk.width == 2
        assert msk.height == 2
        assert msk.size == 4
        assert not msk[0][0]
        assert msk[0][1]
        assert msk[1][0]
        assert not msk[1][1]

    def test_load_constructor(self, make_mock_image, make_mock_fits):
        mock_image = make_mock_image(
            dtype=np.ubyte, min=0, max=255, width=256, height=512
        )
        # Write this image to a FITS file
        my_fits_filename = make_mock_fits(fits.HDUList([fits.PrimaryHDU(mock_image)]))

        new_mask = cplcore.Mask.load(my_fits_filename)

        assert new_mask.shape == mock_image.shape

        for mrow, irow in zip(new_mask, mock_image):
            for mcell, icell in zip(mrow, irow):
                assert (mcell == 0) == (icell == 0)

    def test_get_bytes(self, make_mock_image):
        byts = bytes.fromhex("a0398fbc032efbcd903cd9a9c94d0fa78ced8ea5") * 5
        msk = cplcore.Mask(20, 5, byts)
        assert msk.tobytes() == byts
        assert msk.width == 20
        assert msk.height == 5
        assert msk.shape == (5, 20)

    def test_get_list(self):
        list_2d = [[False, True], [True, False]]
        msk = cplcore.Mask(list_2d)
        assert msk.tolist() == list_2d

    def test_index_out_of_bounds(self):
        new_mask = cplcore.Mask(1, 1)

        with pytest.raises(IndexError):
            new_mask[1]

        with pytest.raises(IndexError):
            new_mask[0][1]

        # with pytest.raises(IndexError):
        #     new_mask[-1]

        # with pytest.raises(IndexError):
        #     new_mask[0][-1]

    def test_set(self):
        new_mask = cplcore.Mask(2, 2)
        assert not new_mask[0][0]
        assert not new_mask[0][1]
        assert not new_mask[1][0]
        assert not new_mask[1][1]

        new_mask[0][1] = True
        assert not new_mask[0][0]
        assert new_mask[0][1]
        assert not new_mask[1][0]
        assert not new_mask[1][1]

        new_mask[1][1] = 62.7  # Numbers also should work
        assert not new_mask[0][0]
        assert new_mask[0][1]
        assert not new_mask[1][0]
        assert new_mask[1][1]

        new_mask[1][1] = 0  # Unset
        assert not new_mask[0][0]
        assert new_mask[0][1]
        assert not new_mask[1][0]
        assert not new_mask[1][1]

        new_mask[0][1] = False
        assert not new_mask[0][0]
        assert not new_mask[0][1]
        assert not new_mask[1][0]
        assert not new_mask[1][1]

    def test_unresizable(self):
        new_mask = cplcore.Mask(1, 1)
        with pytest.raises(ValueError):
            del new_mask[0]
        assert new_mask.shape == (1, 1)

        with pytest.raises(ValueError):
            del new_mask[0][0]
        assert new_mask.shape == (1, 1)

    def test_empty(self):
        mask1 = cplcore.Mask(3, 3)
        assert mask1.is_empty()

    def test_not_emtpy(self):
        mask1 = cplcore.Mask(3, 3)
        mask1[2][2] = True
        assert not mask1.is_empty()

    def test_count_empty(self):
        mask1 = cplcore.Mask(3, 3)
        assert mask1.count() == 0

    def test_count_one(self):
        mask1 = cplcore.Mask(3, 3)
        # ┌─────┐
        # │· · ·│
        # │     │
        # │· · ·│
        # │     │
        # │· · █│
        # └─────┘
        mask1[2][2] = True
        assert mask1.count() == 1

    def test_count_all(self):
        mask1 = cplcore.Mask(2, 2)
        # ┌───┐
        # │█ █│
        # │   │
        # │█ █│
        # └───┘
        mask1[0][0] = True
        mask1[0][1] = True
        mask1[1][0] = True
        mask1[1][1] = True
        assert mask1.count() == 4

    def test_count_window(self):
        mask1 = cplcore.Mask(3, 3)
        # █ █ ·
        #
        # █ █ ·
        #
        # · · ·
        mask1[0][0] = True
        mask1[0][1] = True
        mask1[1][0] = True
        mask1[1][1] = True
        # █ █ ·
        #  ┌───┐
        # █│█ ·│
        #  │   │
        # ·│· ·│
        #  └───┘
        assert mask1.count((1, 1, 2, 2)) == 1

    def test_count_oob(self):
        mask1 = cplcore.Mask(3, 3)
        # An exception should be raised when trying to extract from out of bounds window
        with pytest.raises(cplcore.IllegalInputError):
            mask1.count((2, 2, 3, 3))

    def test_and(self):
        mask1 = cplcore.Mask(3, 3)
        mask2 = ~cplcore.Mask(3, 3)
        # Line down the center of mask1
        # ·█·
        # ·█·
        # ·█·
        mask1[0][1] = True
        mask1[1][1] = True
        mask1[2][1] = True
        # Line of white along the center of mask2
        # ███
        # ···
        # ███
        mask2[1][0] = False
        mask2[1][1] = False
        mask2[1][2] = False

        # Anded together:
        # ·█·
        # ···
        # ·█·
        mask3 = mask1 & mask2
        assert mask3[0][1]
        assert mask3[2][1]
        assert mask3.count() == 2

    def test_or(self):
        mask1 = cplcore.Mask(3, 3)
        mask2 = ~cplcore.Mask(3, 3)
        # Line down the center of mask1
        # ·█·
        # ·█·
        # ·█·
        mask1[0][1] = True
        mask1[1][1] = True
        mask1[2][1] = True
        # Line of white along the center of mask2
        # ███
        # ···
        # ███
        mask2[1][0] = False
        mask2[1][1] = False
        mask2[1][2] = False

        # OR'd together:
        # ███
        # ·█·
        # ███
        mask3 = mask1 | mask2
        assert not mask3[1][0]
        assert not mask3[1][2]
        assert mask3.count() == 7

    def test_xor(self):
        mask1 = cplcore.Mask(3, 3)
        mask2 = ~cplcore.Mask(3, 3)
        # Line down the center of mask1
        # ·█·
        # ·█·
        # ·█·
        mask1[0][1] = True
        mask1[1][1] = True
        mask1[2][1] = True
        # Line of white along the center of mask2
        # ███
        # ···
        # ███
        mask2[1][0] = False
        mask2[1][1] = False
        mask2[1][2] = False

        # OR'd together:
        # █·█
        # ·█·
        # █·█
        mask3 = mask1 ^ mask2
        assert not mask3[0][1]
        assert not mask3[2][1]
        assert not mask3[1][0]
        assert not mask3[1][2]
        assert mask3.count() == 5

    def test_invert(self):
        mask1 = cplcore.Mask(3, 3)
        # ·█·
        # █·█
        # ·█·
        mask1[0][1] = True
        mask1[2][1] = True
        mask1[1][0] = True
        mask1[1][2] = True

        # Inverted:
        # █·█
        # ·█·
        # █·█
        mask2 = ~mask1
        assert not mask2[0][1]
        assert not mask2[2][1]
        assert not mask2[1][0]
        assert not mask2[1][2]
        assert mask2.count() == 5

    def test_collapse_rows(self):
        mask1 = cplcore.Mask(3, 4)
        # ██·
        # ██·
        # █··
        # █··
        mask1[0][0] = True
        mask1[0][1] = True
        mask1[1][0] = True
        mask1[1][1] = True
        mask1[2][0] = True
        mask1[3][0] = True

        # After collapsing
        # █···
        collapsed = mask1.collapse_rows()
        assert collapsed.shape == (1, 3)
        assert collapsed[0][0]
        assert collapsed.count() == 1

    def test_collapse_cols(self):
        mask1 = cplcore.Mask(4, 3)
        # ████
        # ██··
        # ····
        mask1[0][0] = True
        mask1[0][1] = True
        mask1[0][2] = True
        mask1[0][3] = True
        mask1[1][0] = True
        mask1[1][1] = True

        # After collapsing
        # █
        # ·
        # ·
        collapsed = mask1.collapse_cols()
        assert collapsed.shape == (3, 1)
        assert collapsed[0][0]
        assert collapsed.count() == 1

    def test_extract(self):
        big_mask = cplcore.Mask(3, 3)
        # · · █
        #
        # █ █ █
        #
        # · █ ·
        big_mask[0][2] = True
        big_mask[1][2] = True
        big_mask[1][1] = True
        big_mask[1][0] = True
        big_mask[2][1] = True

        # · · █
        #  ┌───┐
        # █│█ █│
        #  │   │
        # ·│█ ·│
        #  └───┘
        bottom_right = big_mask.extract((1, 1, 2, 2))
        assert bottom_right.shape == (2, 2)
        assert bottom_right[0][0]
        assert bottom_right[0][1]
        assert bottom_right[1][0]
        assert not bottom_right[1][1]

        # The window, here, is inclusive and 2-position based (not position+size)
        # ┌─────┐
        # │· · █│
        # └─────┘
        # █ █ █
        #
        # · █ ·
        top = big_mask.extract((0, 0, 2, 0))
        assert top.shape == (1, 3)
        assert not top[0][0]
        assert not top[0][1]
        assert top[0][2]

    def test_extract_oob(self):
        mask1 = cplcore.Mask(3, 3)
        # An exception should be raised when trying to extract from out of bounds window
        with pytest.raises(cplcore.IllegalInputError):
            mask1.extract((2, 2, 3, 3))

    def test_rotate(self):
        mask1 = cplcore.Mask(3, 4)
        # ··█
        # ███
        # ·█·
        # ·██
        mask1[0][2] = True
        mask1[1][2] = True
        mask1[1][1] = True
        mask1[1][0] = True
        mask1[2][1] = True
        mask1[3][1] = True
        mask1[3][2] = True

        # 90°:
        # ··█·
        # ███·
        # █·██
        # Warning: I don't know why, but (as of March 2021)
        # The CPL cpl_mask_turn turns in the opposite
        # direction that it is documented to turn.
        # -90°:
        # ██·█
        # ·███
        # ·█··

        # RESOLVED: June 2021, we were printing the masks top->bottom rather than bottom -> top
        mask1.rotate(-1)
        assert mask1.shape == (3, 4)
        assert mask1[0][2]
        assert mask1[1][0]
        assert mask1[1][1]
        assert mask1[1][2]
        assert mask1[2][0]
        assert mask1[2][2]
        assert mask1[2][3]
        assert mask1.count() == 7

    def test_shift(self):
        mask1 = cplcore.Mask(3, 3)
        mask1[0][2] = True
        mask1[1][2] = True
        mask1[1][1] = True
        mask1[1][0] = True
        mask1[2][1] = True
        # · · █
        #
        # █ █ █
        #
        # · █ ·
        mask1.shift(1, 2)
        # ┌─────┐
        # │↘ ↘ ↓│↓ ↓
        # │     │
        # │→ → ·│· █
        # │     │
        # │→ → █│█ █
        # └─────┘
        #  → → · █ ·
        # Empty zones are turned to '1's
        # ┌─────┐
        # │█ █ █│↓ ↓
        # │     │
        # │█ █ ·│· █
        # │     │
        # │█ █ █│█ █
        # └─────┘
        #  → → · █ ·
        assert not mask1[1][2]
        assert mask1.count() == 8

    def test_shift_all_away(self):
        mask1 = cplcore.Mask(3, 3)
        # If the input is larger than the image
        # An error is raised
        with pytest.raises(cplcore.IllegalInputError):
            mask1.shift(3, 0)

    def test_flip_horizontal(self):
        mask1 = cplcore.Mask(3, 3)
        mask1[0][2] = True
        mask1[1][2] = True
        mask1[1][1] = True
        mask1[1][0] = True
        mask1[2][1] = True
        # · · █
        # █ █ █
        # · █ ·
        mask1.flip(0)
        # · █ ·
        # █ █ █
        # · · █
        assert mask1[0][1]
        assert not mask1[2][1]
        assert not mask1[0][2]
        assert mask1[2][2]

    def test_flip_vertical(self):
        mask1 = cplcore.Mask(3, 3)
        mask1[0][2] = True
        mask1[1][2] = True
        mask1[1][1] = True
        mask1[1][0] = True
        mask1[2][0] = True
        # · · █
        # █ █ █
        # █ · ·
        mask1.flip(2)
        # █ · ·
        # █ █ █
        # · · █
        assert mask1[0][0]
        assert not mask1[0][2]
        assert mask1[2][2]
        assert not mask1[2][0]

    def test_flip_turn(self):
        mask1 = cplcore.Mask(3, 3)
        mask1[0][0] = True
        mask1.rotate(-1)
        mask1.flip(2)
        mask1.flip(1)
        assert mask1[0][0]
        mask1.rotate(-1)
        mask1.flip(0)
        mask1.flip(3)
        assert mask1[0][0]

    def test_move(self):
        mask1 = cplcore.Mask(4, 4)
        newPos = [1, 0, 3, 2]
        with pytest.raises(ValueError):
            mask1.move(5, newPos)
        with pytest.raises(ValueError):
            mask1.move(0, newPos)
        with pytest.raises(ValueError):
            mask1.move(1, newPos)
        # Create a mask
        mask1[0][0] = True
        mask1[0][3] = True
        mask1[3][0] = True
        mask1[3][3] = True

        # Original mask:
        # █··█
        # ····
        # ····
        # █··█
        # 0  1
        # 2  3

        # Move it around
        mask1.move(2, newPos)  # Should rearranged in 2x2 split, in 0 indexed order
        # 3  1
        # 0  2
        # ·██·
        # ····
        # ····
        # ·██·
        assert not mask1[0][0]
        assert not mask1[0][3]
        assert not mask1[3][0]
        assert not mask1[3][3]
        assert mask1[0][1]
        assert mask1[0][2]
        assert mask1[3][1]
        assert mask1[3][2]

    def test_subsample(self):
        # Create a random mask
        mask1 = cplcore.Mask(12, 9)
        mask1.insert(~cplcore.Mask(3, 7), 4, 2)
        mask1.insert(~cplcore.Mask(11, 2), 1, 3)
        mask1.insert(cplcore.Mask(2, 4), 6, 2)
        mask1.insert(cplcore.Mask(2, 3), 3, 6)
        mask1.insert(~cplcore.Mask(3, 1), 1, 0)
        mask1.insert(~cplcore.Mask(2, 2), 1, 10)
        # ·███········
        # ↑   ↑   ↑
        # ··········██
        # ····██····██
        # ↑   ↑   ↑
        # ·█████··████
        # ·█████··██··
        # ↑   ↑   ↑
        # ····██······
        # ·····██·····
        # ↑   ↑   ↑
        # ·····██·····
        # ·····██·····
        mask2 = mask1.subsample(1, 2)
        assert mask2[1][2]
        assert mask2.shape == (9, 6)
        assert mask2.count() == 17

    # Test the CPL function using example from Schalkoff. Translation of equivalent cpl test to python
    # see R. Schallkoff, "Digital Image Processing and Computer Vision"
    def test_filter_schalkoff(self):
        # Fill the mask border (used several times within cpl_mask-test)
        # self  Mask to be filled at the border
        # hx    Number of borders columns to fill
        # hy    Number of borders rows to fill
        # fill  Value to use in fill
        def fill_border(mask, hx, hy, fill):
            if fill:
                mask.insert(~cplcore.Mask(hx, mask.height), 0, 0)
                mask.insert(~cplcore.Mask(hx, mask.height), mask.width - hx, 0)
                mask.insert(~cplcore.Mask(mask.width, hy), 0, 0)
                mask.insert(~cplcore.Mask(mask.width, hy), 0, mask.height - hy)
            else:
                mask.insert(cplcore.Mask(hx, mask.height), 0, 0)
                mask.insert(cplcore.Mask(hx, mask.height), 0, mask.width - hx)
                mask.insert(cplcore.Mask(mask.width, hy), 0, 0)
                mask.insert(cplcore.Mask(mask.width, hy), mask.height - hy, 0)

        nx = 21
        ny = 18

        borders = [cplcore.Border.NOP, cplcore.Border.ZERO, cplcore.Border.COPY]
        # const size_t nborders = sizeof(borders)/sizeof(borders[0]);

        # Binary Image 6.36a
        maskA = cplcore.Mask(
            [
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            ]
        )

        # Binary Image 6.37b from R. Schallkoff (Dilation w. 3x3 full kernel)
        maskD = cplcore.Mask(
            [
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0],
                [0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0],
                [0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0],
                [0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0],
                [0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0],
                [0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0],
                [0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0],
                [0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0],
                [0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            ]
        )

        # Binary Image 6.37c from R. Schallkoff (Erosion w. 3x3 full kernel)
        maskE = cplcore.Mask(
            [
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            ]
        )

        # Binary Image 6.39a from R. Schallkoff (Opening w. 3x3 full kernel)
        maskO = cplcore.Mask(
            [
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            ]
        )

        # Binary Image 6.39b from R. Schallkoff (Closing w. 3x3 full kernel)
        maskC = cplcore.Mask(
            [
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            ]
        )

        mask0 = cplcore.Mask(nx, ny)
        # Build a 3x3 kernel inside a 5 x 5 mask
        kernel = cplcore.Mask(5, 5)

        # Fill 3x3 kernel
        kernel[1][1] = True
        kernel[1][2] = True
        kernel[1][3] = True
        kernel[2][1] = True
        kernel[2][2] = True
        kernel[2][3] = True
        kernel[3][1] = True
        kernel[3][2] = True
        kernel[3][3] = True
        assert kernel.count() == 3 * 3
        for b in borders:
            for iflip in range(3):
                # Test also with flipped images since the mask is traversed in one specific direction

                # Test erosion
                flipCopy = maskA.copy()
                if iflip != 1:
                    flipCopy.flip(iflip)
                mask0 = flipCopy.filter(kernel, cplcore.Filter.EROSION, b)
                if iflip != 1:
                    mask0.flip(iflip)
                assert mask0 == maskE

                # Test dilation
                flipCopy = maskA.copy()
                if iflip != 1:
                    flipCopy.flip(iflip)
                mask0 = flipCopy.filter(kernel, cplcore.Filter.DILATION, b)
                if iflip != 1:
                    mask0.flip(iflip)
                assert maskD == mask0

                # Test duality of erosion and dilation (kernel is symmetric)
                flipCopy = ~flipCopy
                mask0 = flipCopy.filter(kernel, cplcore.Filter.DILATION, b)
                mask0 = ~mask0

                # No Duality on Border
                fill_border(mask0, 3, 3, False)
                if iflip != 1:
                    mask0.flip(iflip)
                assert mask0 == maskE

                # Test duality of erosion and dilation (kernel is symmetric)
                mask0 = flipCopy.filter(kernel, cplcore.Filter.EROSION, b)
                mask0 = ~mask0

                # No duality on border
                fill_border(mask0, 3, 3, False)
                if iflip != 1:
                    mask0.flip(iflip)
                assert maskD == mask0

                # Test opening
                flipCopy = maskA.copy()
                if iflip != 1:
                    flipCopy.flip(iflip)
                nidem = 2
                while nidem != 0:
                    # Idempotency test as well
                    mask0 = flipCopy.filter(kernel, cplcore.Filter.OPENING, b)
                    nidem -= 1
                if iflip != 1:
                    mask0.flip(iflip)
                assert maskO == mask0

                # Test closing
                flipCopy = maskA.copy()
                if iflip != 1:
                    flipCopy.flip(iflip)
                nidem = 2
                while nidem != 0:
                    # Idempotency test as well
                    mask0 = flipCopy.filter(kernel, cplcore.Filter.CLOSING, b)
                    nidem -= 1
                if iflip != 1:
                    mask0.flip(iflip)
                assert mask0 == maskC

                # Test duality of opening and closing
                flipCopy = ~flipCopy
                mask0 = flipCopy.filter(kernel, cplcore.Filter.OPENING, b)
                mask0 = ~mask0

                # No duality on border
                fill_border(mask0, 3, 3, False)
                if iflip != 1:
                    mask0.flip(iflip)
                assert maskC == mask0

                # Test duality of closing and opening
                mask0 = flipCopy.filter(kernel, cplcore.Filter.CLOSING, b)
                mask0 = ~mask0

                # No duality on border
                fill_border(mask0, 3, 3, False)
                if iflip != 1:
                    mask0.flip(iflip)
                assert maskO == mask0
                # Tests for one flip done

    def test_insert(self):
        big_mask = cplcore.Mask(3, 3)
        [a, b, c, d] = [True, False, False, True]
        bottom_right = cplcore.Mask(2, 2)
        bottom_right[0][0] = a
        bottom_right[0][1] = b
        bottom_right[1][0] = c
        bottom_right[1][1] = d

        big_mask.insert(bottom_right, 1, 1)
        assert not big_mask[0][0]
        assert not big_mask[0][1]
        assert not big_mask[0][2]
        assert not big_mask[1][0]
        assert big_mask[1][1] == a  # 0,0 (bottom_right index) + 1,1 (overwrite index)
        assert big_mask[1][2] == b  # 0,1 (bottom_right index) + 1,1 (overwrite index)
        assert not big_mask[2][0]
        assert big_mask[2][1] == c  # 1,0 (bottom_right index) + 1,1 (overwrite index)
        assert big_mask[2][2] == d  # 1,1 (bottom_right index) + 1,1 (overwrite index)

    def test_image_threshold_binary1(self):
        im = cplcore.Image([[1, 2], [3, 4]])
        mask = cplcore.Mask.threshold_image(
            im, 1.5, 4, True
        )  # All within threshold True, rest False
        assert np.array_equal(mask, [[False, True], [True, False]])

    def test_image_threshold_binary0(self):
        im = cplcore.Image([[1, 2], [3, 4]])
        mask = cplcore.Mask.threshold_image(
            im, 0, 2.4, 0
        )  # All within threshold False, rest True
        assert np.array_equal(mask, [[False, False], [True, True]])

    def test_repr(self):
        mask1 = cplcore.Mask(3, 3)
        assert repr(mask1) == """<cpl.core.Mask, 3x3 empty mask>"""
        mask1[2][2] = True
        assert repr(mask1) == "<cpl.core.Mask, 3x3 non-empty mask>"

    def test_dump_file(self, tmp_path):
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_mask_dump.txt"
        mask1 = cplcore.Mask(3, 3)
        mask1[2][2] = True
        filename = tmp_path.joinpath(p)
        mask1.dump(filename=str(filename))
        outp = """#----- mask: 1 <= x <= 3, 1 <= y <= 3 -----
	X	Y	value
	1	1	0
	1	2	0
	1	3	0
	2	1	0
	2	2	0
	2	3	0
	3	1	0
	3	2	0
	3	3	1
"""  # noqa
        contents = ""
        with open(str(filename), "r") as f:
            for line in f.readlines():
                contents += line
        assert contents == outp

    def test_dump_string(self):
        mask1 = cplcore.Mask(3, 3)
        mask1[2][2] = True
        outp = """#----- mask: 1 <= x <= 3, 1 <= y <= 3 -----
	X	Y	value
	1	1	0
	1	2	0
	1	3	0
	2	1	0
	2	2	0
	2	3	0
	3	1	0
	3	2	0
	3	3	1
"""  # noqa
        assert str(mask1) == outp
        assert isinstance(mask1.dump(show=False), str)
        # test some special cases
        assert mask1.dump(window=None, show=False) == outp
        assert mask1.dump(window=(0, 0, 0, 0), show=False) == outp
        # test a window
        outp = """#----- mask: 1 <= x <= 2, 1 <= y <= 2 -----
	X	Y	value
	1	1	0
	1	2	0
	2	1	0
	2	2	0
"""  # noqa
        assert mask1.dump(window=(0, 0, 1, 1), show=False) == outp

    def test_dump_stdout(self):
        cmd = "from cpl import core as cplcore; "
        cmd += "mask1 = cplcore.Mask(3, 3); "
        cmd += "mask1[2][2] = True ; "
        cmd += "mask1.dump()"
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        expect = """#----- mask: 1 <= x <= 3, 1 <= y <= 3 -----
	X	Y	value
	1	1	0
	1	2	0
	1	3	0
	2	1	0
	2	2	0
	2	3	0
	3	1	0
	3	2	0
	3	3	1
"""  # noqa
        assert outp == expect

    def test_mask_args_yx(self):
        new_mask = cplcore.Mask(20, 5)
        assert not new_mask[0][0]
        assert not new_mask[2][19]  # 20 means 0 to 19
        assert not new_mask[3][19]
        assert not new_mask[4][19]
        with pytest.raises(IndexError):
            new_mask[5]

        new_mask[0][1] = True
        assert not new_mask[0][0]
        assert new_mask[0][1]
        assert not new_mask[1][0]
        assert not new_mask[1][1]
        assert new_mask.count() == 1

        bottom_right = cplcore.Mask(2, 2)
        bottom_right[0][0] = True
        bottom_right[0][1] = True
        bottom_right[1][0] = True
        bottom_right[1][1] = True

        new_mask.insert(bottom_right, 3, 7)  # y,x
        assert not new_mask[0][0]
        assert not new_mask[4][10]
        assert new_mask[3][7]
        assert new_mask[4][7]
        assert new_mask[3][8]
        assert new_mask[4][8]
        assert new_mask[0][1]
        assert not new_mask[3][19]
        assert not new_mask[4][17]
        assert new_mask.count() == 5

        sub_mask = new_mask.subsample(1, 2)
        assert sub_mask[3][4]
        assert sub_mask[4][4]
        assert new_mask.shape == (5, 20)
        assert sub_mask.shape == (5, 10)
        assert sub_mask.count() == 2
