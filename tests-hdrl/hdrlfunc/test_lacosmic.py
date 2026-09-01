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


class TestLaCosmic:
    def test_edge_detect_single_pixels(self):
        # detect single pixel cosmics
        img_mask = cplcore.Mask(200, 300)
        img_data = cplcore.Image.create_noise_uniform(200, 300, cplcore.Type.DOUBLE, 90, 110)
        error = (110.0 - 90.0) / math.sqrt(12)

        img_data[49][49] = 300.0
        img_data[99][99] = 300.0
        img_data[149][149] = 300.0
        img_data[249][99] = 300.0

        img_error = cplcore.Image.zeros(img_data.width, img_data.height, cplcore.Type.DOUBLE)
        img_error.add_scalar(error)

        img_mask[119][119] = True
        img_mask[120][119] = True
        img_mask[121][119] = True
        img_mask[119][120] = True
        img_mask[120][120] = True
        img_mask[121][120] = True
        img_mask[119][121] = True
        img_mask[120][121] = True
        img_mask[121][121] = True

        # set one outlier on a bad pixel
        img_data[121][121] = 300.0
        img_data.reject_from_mask(img_mask)
        himg = hdrlcore.Image(img_data, img_error)
        lac = hdrlfunc.LaCosmic(error * 2, 2.0, 5)
        result_mask = lac.edgedetect(himg)

        assert result_mask[49][49]
        assert result_mask[99][99]
        assert result_mask[149][149]
        assert result_mask[249][99]
        assert not result_mask[121][121]
        assert not result_mask[259][109]

    def test_edge_detect_big_rectangular(self):
        # detect a very big rectangular cosmic
        img_data = cplcore.Image.create_noise_uniform(150, 200, cplcore.Type.DOUBLE, 90, 110)
        error = (110.0 - 90.0) / math.sqrt(12)

        for varx in range(50, 75):
            for vary in range(60, 130):
                img_data[vary - 1][varx - 1] = 5000.0

        for varx in range(20, 120):
            for vary in range(20, 40):
                img_data[vary - 1][varx - 1] = 5000.0

        img_error = cplcore.Image.zeros(img_data.width, img_data.height, cplcore.Type.DOUBLE)
        img_error.add_scalar(error)
        himg = hdrlcore.Image(img_data, img_error)
        lac = hdrlfunc.LaCosmic(error * 2, 0.5, 65)
        result_mask = lac.edgedetect(himg)
        assert result_mask.count() == 100 * 20 + 25 * 70

    def test_edge_detect_big_rectangular_badpix(self):
        # detect a very big rectangular cosmic with bad pixels
        img_data = cplcore.Image.create_noise_uniform(150, 200, cplcore.Type.DOUBLE, 90, 110)
        img_mask = cplcore.Mask(150, 200)
        error = (110.0 - 90.0) / math.sqrt(12)

        for varx in range(50, 75):
            for vary in range(60, 130):
                img_data[vary - 1][varx - 1] = 5000.0

        for varx in range(20, 120):
            for vary in range(20, 40):
                img_data[vary - 1][varx - 1] = 5000.0

        # mark bad pixels
        for varx in range(65, 68):
            for vary in range(1, 150):
                img_mask[vary - 1][varx - 1] = True

        img_data.reject_from_mask(img_mask)

        img_error = cplcore.Image.zeros(img_data.width, img_data.height, cplcore.Type.DOUBLE)
        img_error.add_scalar(error)
        himg = hdrlcore.Image(img_data, img_error)
        lac = hdrlfunc.LaCosmic(error * 2, 0.5, 80)
        result_mask = lac.edgedetect(himg)
        assert result_mask.count() == 100 * 20 + 25 * 70 - 3 * 70 - 3 * 20

    def test_inputs(self):
        # Test constructors
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.LaCosmic(0, 0, 0)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.LaCosmic(0, -1.0, 1)
        with pytest.raises(hdrlcore.IllegalInputError):
            hdrlfunc.LaCosmic(-1.0, 0, 1)

        # some more normal parameters
        lac = hdrlfunc.LaCosmic(5, 2, 5)

        # Test getters
        assert lac.sigma_lim == 5
        assert lac.f_lim == pytest.approx(2.0)
        assert lac.max_iter == 5

        # image smaller than 7x7 kernel
        img1 = hdrlcore.Image.zeros(6, 1000)
        with pytest.raises(hdrlcore.IncompatibleInputError):
            lac.edgedetect(img1)

        # larger image
        img2 = hdrlcore.Image.zeros(1200, 4)
        with pytest.raises(hdrlcore.IncompatibleInputError):
            lac.edgedetect(img2)
