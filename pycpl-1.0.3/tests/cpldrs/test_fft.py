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

import numpy as np
import pytest

import cpl.core as cplcore
import cpl.drs as cpldrs

imtypes = [
    cplcore.Type.DOUBLE,
    cplcore.Type.FLOAT,
    cplcore.Type.DOUBLE_COMPLEX,
    cplcore.Type.FLOAT_COMPLEX,
]


class TestFFT:
    def test_image_forward(self):
        im = cplcore.Image(
            [[1, 2, 3, 4, 5, 6, 7, 8], [1, 2, 3, 4, 5, 6, 7, 8]],
            cplcore.Type.DOUBLE_COMPLEX,
        )
        res = cpldrs.fft.fft_image(im, cpldrs.fft.Mode.FORWARD)
        # Compare with numpy output
        assert np.array_equal(res, np.fft.fftn(im))
        # Ensure NOSCALE has no effect
        with_noscale = cpldrs.fft.fft_image(
            im, cpldrs.fft.Mode.FORWARD, find=cpldrs.fft.FIND_MEASURE, scale=False
        )
        assert np.array_equal(res, with_noscale)

    def test_image_backward(self):
        im = cplcore.Image(
            [[1, 2, 3, 4, 5, 6, 7, 8], [1, 2, 3, 4, 5, 6, 7, 8]],
            cplcore.Type.DOUBLE_COMPLEX,
        )
        res = cpldrs.fft.fft_image(im, cpldrs.fft.Mode.BACKWARD)
        # Compare with numpy output
        assert np.array_equal(res, np.fft.ifftn(im))

    def test_image_forward_float(self):
        im = cplcore.Image(
            [[1, 2, 3, 4, 5, 6, 7, 8], [1, 2, 3, 4, 5, 6, 7, 8]],
            cplcore.Type.FLOAT_COMPLEX,
        )
        res = cpldrs.fft.fft_image(im, cpldrs.fft.Mode.FORWARD)
        # Compare with numpy output
        assert np.allclose(res, np.fft.fftn(im), np.finfo(np.complex64).eps)
        # Ensure NOSCALE has no effect
        with_noscale = cpldrs.fft.fft_image(im, cpldrs.fft.Mode.FORWARD, scale=False)
        assert np.allclose(res, with_noscale)  # All close due to float precision

    def test_image_backward_float(self):
        im = cplcore.Image(
            [[1, 2, 3, 4, 5, 6, 7, 8], [1, 2, 3, 4, 5, 6, 7, 8]],
            cplcore.Type.FLOAT_COMPLEX,
        )
        res = cpldrs.fft.fft_image(im, cpldrs.fft.Mode.BACKWARD)
        # Compare with numpy output
        assert np.allclose(
            res, np.fft.ifftn(im), np.finfo(np.complex64).eps
        )  # All close due to float precision

    def test_image_backward_noscale(self):
        pytest.importorskip(
            "numpy",
            minversion="1.20.0",
            reason="Using kwarg not present until numpy 1.20.0",
        )
        im = cplcore.Image(
            [[1, 2, 3, 4, 5, 6, 7, 8], [1, 2, 3, 4, 5, 6, 7, 8]],
            cplcore.Type.DOUBLE_COMPLEX,
        )
        res = cpldrs.fft.fft_image(im, cpldrs.fft.Mode.BACKWARD, scale=False)
        # Compare with numpy output
        assert np.array_equal(
            res, np.fft.ifftn(im, norm="forward")
        )  # Inverse FFT so backwards is forwards when not using scale

    def test_image_backwards_forwards(self):
        im = cplcore.Image(
            [[1, 2, 3, 4, 5, 6, 7, 8], [1, 2, 3, 4, 5, 6, 7, 8]],
            cplcore.Type.DOUBLE_COMPLEX,
        )
        res = cpldrs.fft.fft_image(im, cpldrs.fft.Mode.BACKWARD)
        # Compare with numpy output
        res = cpldrs.fft.fft_image(res, cpldrs.fft.Mode.FORWARD)
        assert np.array_equal(res, [[1, 2, 3, 4, 5, 6, 7, 8], [1, 2, 3, 4, 5, 6, 7, 8]])

    def test_image_forwards_backwards(self):
        im = cplcore.Image(
            [[1, 2, 3, 4, 5, 6, 7, 8], [1, 2, 3, 4, 5, 6, 7, 8]],
            cplcore.Type.DOUBLE_COMPLEX,
        )
        res = cpldrs.fft.fft_image(im, cpldrs.fft.Mode.FORWARD)
        # Compare with numpy output
        res = cpldrs.fft.fft_image(res, cpldrs.fft.Mode.BACKWARD)
        assert np.array_equal(res, [[1, 2, 3, 4, 5, 6, 7, 8], [1, 2, 3, 4, 5, 6, 7, 8]])

    def test_image_forward_non_complex(self):
        im = cplcore.Image(
            [[1, 2, 3, 4, 5, 6, 7, 8], [1, 2, 3, 4, 5, 6, 7, 8]], cplcore.Type.DOUBLE
        )
        res = cpldrs.fft.fft_image(im, cpldrs.fft.Mode.FORWARD)

        # Compare with numpy output. Use np.allclose to compare the results to avoid
        # rounding issues. This however will not catch shape issues, which should not
        # be present by construction though.
        assert np.allclose(
            # Conversion will only use half the columns
            res.extract((0, 0, 4, 1)), np.fft.rfftn(im)
        )

    def test_image_forward_non_complex_failure(self):
        im = cplcore.Image(
            [[1, 2, 3, 4, 5, 6, 7, 8], [1, 2, 3, 4, 5, 6, 7, 8]], cplcore.Type.DOUBLE
        )
        # Compare with numpy output
        with pytest.raises(cplcore.TypeMismatchError):
            _ = cpldrs.fft.fft_image(
                im, cpldrs.fft.Mode.BACKWARD
            )  # should fail due to non-complex, backwards

    def test_imagelist_forward(self):
        imlist = cplcore.ImageList(
            [
                cplcore.Image(
                    [[1, 2, 3, 4, 5, 6, 7, 8], [1, 2, 3, 4, 5, 6, 7, 8]],
                    cplcore.Type.DOUBLE_COMPLEX,
                ),
                cplcore.Image(
                    [[14, 6, 7, 12, 5, 5, 5, 5], [2, 4, 5, 62, 5, 2, 5, 3]],
                    cplcore.Type.DOUBLE_COMPLEX,
                ),
            ]
        )
        res_imlist = cpldrs.fft.fft_imagelist(imlist, cpldrs.fft.Mode.FORWARD)
        # Compare with numpy output
        assert len(imlist) == len(res_imlist)
        for im, res in zip(imlist, res_imlist):
            assert np.array_equal(
                res, np.fft.fftn(im)
            )  # Conversion will only use half the columns
