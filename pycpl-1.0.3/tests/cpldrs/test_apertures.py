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
import copy
import subprocess


@pytest.fixture
def image_1(request):  # Request is a type
    maxval = 10.0
    to_ret = cpl.core.Image.zeros(10, 10, cpl.core.Type.DOUBLE)
    to_ret[3][3] = to_ret[3][4] = to_ret[3][5] = to_ret[3][6] = 1.0
    to_ret[4][3] = to_ret[4][4] = to_ret[4][5] = to_ret[4][6] = 1.0
    to_ret[5][3] = to_ret[5][4] = to_ret[5][6] = 1.0
    to_ret[5][5] = maxval
    to_ret[6][3] = to_ret[6][4] = to_ret[6][5] = to_ret[6][6] = 1.0
    to_ret[8][8] = 2.0 * maxval
    return to_ret.cast(request.param)


@pytest.fixture
def image_and_bpm_1(request):
    maxval = 10.0
    to_ret = cpl.core.Image.zeros(10, 10, cpl.core.Type.DOUBLE)
    to_ret[3][3] = to_ret[3][4] = to_ret[3][5] = to_ret[3][6] = 1.0
    to_ret[4][3] = to_ret[4][4] = to_ret[4][5] = to_ret[4][6] = 1.0
    to_ret[5][3] = to_ret[5][4] = to_ret[5][6] = 1.0
    to_ret[5][5] = maxval
    to_ret[6][3] = to_ret[6][4] = to_ret[6][5] = to_ret[6][6] = 1.0
    to_ret[8][8] = 2.0 * maxval
    # New mask with image thresholds
    bpm = cpl.core.Mask(to_ret, 0.5, 0.5 + maxval)
    bpm = ~bpm
    return to_ret.cast(request.param), bpm


@pytest.fixture(scope="function")
def apertures_test_one():
    # Subject the aperture to some self-consistency checks
    def _apertures_test_one(apertures):
        nsize = len(apertures)
        for i in range(1, nsize + 1):
            _ = apertures.get_npix(i)
            assert apertures.get_left(i) <= apertures.get_pos_x(i)
            assert apertures.get_pos_x(i) <= apertures.get_right(i)

            assert apertures.get_bottom(i) <= apertures.get_pos_y(i)
            assert apertures.get_pos_y(i) <= apertures.get_top(i)

            assert apertures.get_left(i) <= apertures.get_maxpos_x(i)
            assert apertures.get_maxpos_x(i) <= apertures.get_right(i)

            assert apertures.get_bottom(i) <= apertures.get_maxpos_y(i)
            assert apertures.get_maxpos_y(i) <= apertures.get_top(i)

            assert apertures.get_left(i) <= apertures.get_minpos_x(i)
            assert apertures.get_minpos_x(i) <= apertures.get_right(i)

            assert apertures.get_bottom(i) <= apertures.get_minpos_y(i)
            assert apertures.get_minpos_y(i) <= apertures.get_top(i)

            assert apertures.get_min(i) <= apertures.get_mean(i)
            assert apertures.get_mean(i) <= apertures.get_max(i)

            assert apertures.get_min(i) <= apertures.get_median(i)
            assert apertures.get_median(i) <= apertures.get_max(i)

    return _apertures_test_one


@pytest.fixture(scope="function")
def apertures_test_one_idx():
    "apertures_test_one, but use an index __getitem__ to get each aperture"

    # Subject the aperture to some self-consistency checks
    def _apertures_test_one_idx(apertures):
        nsize = len(apertures)
        for i in range(0, nsize):
            _ = apertures[i].npix
            assert apertures[i].left <= apertures[i].pos_x
            assert apertures[i].pos_x <= apertures[i].right

            assert apertures[i].bottom <= apertures[i].pos_y
            assert apertures[i].pos_y <= apertures[i].top

            assert apertures[i].left <= apertures[i].maxpos_x
            assert apertures[i].maxpos_x <= apertures[i].right

            assert apertures[i].bottom <= apertures[i].maxpos_y
            assert apertures[i].maxpos_y <= apertures[i].top

            assert apertures[i].left <= apertures[i].minpos_x
            assert apertures[i].minpos_x <= apertures[i].right

            assert apertures[i].bottom <= apertures[i].minpos_y
            assert apertures[i].minpos_y <= apertures[i].top

            assert apertures[i].min <= apertures[i].mean
            assert apertures[i].mean <= apertures[i].max

            assert apertures[i].min <= apertures[i].median
            assert apertures[i].median <= apertures[i].max

    return _apertures_test_one_idx


@pytest.fixture(scope="function")
def apertures_test_one_iter():
    "apertures_test_one, but use an iterator to get each aperture"

    # Subject the aperture to some self-consistency checks
    def _apertures_test_one_iter(apertures):
        _ = len(apertures)
        for apt in apertures:
            _ = apt.npix
            assert apt.left <= apt.pos_x
            assert apt.pos_x <= apt.right

            assert apt.bottom <= apt.pos_y
            assert apt.pos_y <= apt.top

            assert apt.left <= apt.maxpos_x
            assert apt.maxpos_x <= apt.right

            assert apt.bottom <= apt.maxpos_y
            assert apt.maxpos_y <= apt.top

            assert apt.left <= apt.minpos_x
            assert apt.minpos_x <= apt.right

            assert apt.bottom <= apt.minpos_y
            assert apt.minpos_y <= apt.top

            assert apt.min <= apt.mean
            assert apt.mean <= apt.max

            assert apt.min <= apt.median
            assert apt.median <= apt.max

    return _apertures_test_one_iter


@pytest.fixture(scope="function")
def sort_test():
    # Test sorting correctness
    def _sort_test(aperts):
        # By peaks
        aperts.sort_by_max()
        for pos in range(1, len(aperts)):
            assert aperts.get_max(pos + 1) <= aperts.get_max(pos)

        # By size
        aperts.sort_by_npix()
        for pos in range(1, len(aperts)):
            assert aperts.get_npix(pos + 1) <= aperts.get_npix(pos)

        # By brightness
        aperts.sort_by_flux()
        for pos in range(1, len(aperts)):
            assert aperts.get_flux(pos + 1) <= aperts.get_flux(pos)

    return _sort_test


@pytest.fixture(scope="function")
def sort_test_idx():
    "Test sort, but use index to get individual apertures"

    # Test sorting correctness
    def _sort_test(aperts):
        # By peaks
        aperts.sort_by_max()
        for pos in range(0, len(aperts) - 1):
            assert aperts[pos + 1].max <= aperts[pos].max

        # By size
        aperts.sort_by_npix()
        for pos in range(0, len(aperts) - 1):
            assert aperts[pos + 1].npix <= aperts[pos].npix

        # By brightness
        aperts.sort_by_flux()
        for pos in range(0, len(aperts) - 1):
            assert aperts[pos + 1].flux <= aperts[pos].flux

    return _sort_test


class TestApertures:
    # Test the apertures detection with a thresholding method
    # def test_extract(self, cpl_image_fill_test_create, apertures_test_one):
    #     imd = cpl_image_fill_test_create(512, 512)
    #     sigmas = cpl.core.Vector([5, 2, 1, 0.5])
    #     aperts = cpl.drs.Apertures.extract(imd, sigmas)
    #     apertures_test_one(aperts.Apertures)

    def test_extract(self, cpl_image_fill_test_create, apertures_test_one, sort_test):
        imd = cpl_image_fill_test_create(512, 512)
        sigmas = cpl.core.Vector([5, 2, 1, 0.5])
        aperts, pos = cpl.drs.Apertures.extract(imd, sigmas)
        apertures_test_one(aperts)
        sort_test(aperts)

    # Test the apertures detection (threshold) on a zone"
    # def test_extract_window(self, cpl_image_fill_test_create, apertures_test_one):
    #     IMAGESZ2 = 512
    #     imd = cpl_image_fill_test_create(512, 512)
    #     sigmas = cpl.core.Vector([5, 2, 1, 0.5])
    #     aperts = cpl.drs.Apertures.extract_window(imd, sigmas, (int(IMAGESZ2 / 4), int(IMAGESZ2 / 4),
    #                                                             int(3 * IMAGESZ2 / 4), int(3 * IMAGESZ2 / 4)))
    #     apertures_test_one(aperts.Apertures)

    def test_extract_window(
        self, cpl_image_fill_test_create, apertures_test_one, sort_test
    ):
        IMAGESZ2 = 512
        imd = cpl_image_fill_test_create(IMAGESZ2, IMAGESZ2)
        sigmas = cpl.core.Vector([5, 2, 1, 0.5])
        aperts, pos = cpl.drs.Apertures.extract_window(
            imd,
            sigmas,
            (
                int(IMAGESZ2 / 4),
                int(IMAGESZ2 / 4),
                int(3 * IMAGESZ2 / 4),
                int(3 * IMAGESZ2 / 4),
            ),
        )
        apertures_test_one(aperts)
        sort_test(aperts)

    @pytest.mark.parametrize(
        "image_1",
        [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT],
        indirect=["image_1"],
    )
    def test_bad_label_illegal_input_1(self, image_1):
        img = image_1
        labels = cpl.core.Image.zeros(10, 10, cpl.core.Type.INT)
        with pytest.raises(cpl.core.IllegalInputError):
            _ = cpl.drs.Apertures(img, labels)
        labels[9][9] = -1.0
        with pytest.raises(cpl.core.IllegalInputError):
            cpl.drs.Apertures(img, labels)

    @pytest.mark.parametrize(
        "image_1",
        [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT],
        indirect=["image_1"],
    )
    def test_bad_label_illegal_input_2(self, image_1):
        img = image_1
        bin_img = cpl.core.Mask(img, 0.5, np.finfo(np.double).max)
        labels, count = cpl.core.Image.labelise_create(bin_img)
        labels[9][9] = -1.0
        with pytest.raises(cpl.core.IllegalInputError):
            cpl.drs.Apertures(img, labels)

    @pytest.mark.parametrize(
        "image_1",
        [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT],
        indirect=["image_1"],
    )
    def test_bad_label_success(self, image_1, apertures_test_one):
        img = image_1
        labels = cpl.core.Image.zeros(10, 10, cpl.core.Type.INT)
        labels[0][0] = 1.0
        aperts = cpl.drs.Apertures(img, labels)
        apertures_test_one(aperts)
        assert 1 == len(aperts)

    @pytest.mark.parametrize(
        "image_1",
        [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT],
        indirect=["image_1"],
    )
    def test_bad_label_data_not_found(self, image_1):
        img = image_1
        labels = cpl.core.Image.zeros(10, 10, cpl.core.Type.INT)
        labels[0][0] = 2.0
        with pytest.raises(cpl.core.DataNotFoundError):
            _ = cpl.drs.Apertures(img, labels)

    @pytest.mark.parametrize(
        "image_and_bpm_1",
        [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT],
        indirect=["image_and_bpm_1"],
    )
    def test_bad_pixel(self, image_and_bpm_1, apertures_test_one):
        img, bpm = image_and_bpm_1
        imb = copy.deepcopy(img)
        imb.reject_from_mask(bpm)
        bin_img = cpl.core.Mask(img, 0.5, np.finfo(np.double).max)
        labels, count = cpl.core.Image.labelise_create(bin_img)
        # Compute statistics on detected apertures
        aperts = cpl.drs.Apertures(img, labels)
        assert len(aperts) == 2
        aperts.sort_by_npix()
        apertures_test_one(aperts)
        assert aperts.get_npix(1) == 10 * 10 - imb.count_rejected()
        # min
        assert np.isclose(
            aperts.get_min(1),
            imb.get_min((0, 0, 9, 9)),
            atol=4.0 * np.finfo(np.double).eps,
        )
        # max
        assert np.isclose(
            aperts.get_max(1),
            imb.get_max((0, 0, 9, 9)),
            atol=4.0 * np.finfo(np.double).eps,
        )
        # mean
        assert np.isclose(
            aperts.get_mean(1),
            imb.get_mean((0, 0, 9, 9)),
            atol=4.0 * np.finfo(np.double).eps,
        )
        # median
        assert np.isclose(
            aperts.get_median(1),
            imb.get_median((0, 0, 9, 9)),
            atol=4.0 * np.finfo(np.double).eps,
        )
        # stdev
        assert np.isclose(
            aperts.get_stdev(1),
            imb.get_stdev((0, 0, 9, 9)),
            atol=4.0 * np.finfo(np.double).eps,
        )
        # flux
        assert np.isclose(
            aperts.get_flux(1),
            imb.get_flux((0, 0, 9, 9)),
            atol=4.0 * np.finfo(np.double).eps,
        )
        # centroid_x
        assert np.isclose(
            aperts.get_centroid_x(1),
            imb.get_centroid_x((0, 0, 9, 9)),
            atol=4.0 * np.finfo(np.double).eps,
        )
        # centroid_y
        assert np.isclose(
            aperts.get_centroid_y(1),
            imb.get_centroid_y((0, 0, 9, 9)),
            atol=4.0 * np.finfo(np.double).eps,
        )

        xpos, ypos = imb.get_maxpos()
        assert aperts.get_left(1) == 4
        assert aperts.get_left_y(1) == 4
        assert aperts.get_right(1) == 7
        assert aperts.get_right_y(1) == 4

        assert aperts.get_bottom(1) == 4
        assert aperts.get_top(1) == 7
        assert aperts.get_bottom_x(1) == 4
        assert aperts.get_top_x(1) == 4

        assert aperts.get_left(1) <= xpos
        assert ypos <= aperts.get_right(1)

    @pytest.mark.parametrize(
        "image_and_bpm_1",
        [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT],
        indirect=["image_and_bpm_1"],
    )
    def test_thresholding_statistics(
        self, image_and_bpm_1, apertures_test_one, sort_test
    ):
        img, bpm = image_and_bpm_1
        imb = copy.deepcopy(img)
        imb.reject_from_mask(bpm)

        bin_img = cpl.core.Mask(img, 0.5, np.finfo(np.double).max)
        labels, count = cpl.core.Image.labelise_create(bin_img)
        # set bad pixel
        img.reject(5, 5)

        # Compute statistics on detected apertures
        aperts = cpl.drs.Apertures(img, labels)
        apertures_test_one(aperts)
        sort_test(aperts)
        assert len(aperts) == 2
        # Get highest peak
        aperts.sort_by_max()
        # Extra from sort test since it also checks the image
        assert aperts.get_max(1) == img.get_max()
        assert aperts.get_max(2) <= aperts.get_max(1)

    def test_extract_sigma_datanotfound(self, cpl_image_fill_test_create):
        imd = cpl_image_fill_test_create(512, 512)
        with pytest.raises(cpl.core.DataNotFoundError):
            cpl.drs.Apertures.extract_sigma(imd, 100.0)

    def test_extract_sigma(
        self, cpl_image_fill_test_create, apertures_test_one, sort_test
    ):
        imd = cpl_image_fill_test_create(512, 512)
        aperts = cpl.drs.Apertures.extract_sigma(imd, 0.5)
        apertures_test_one(aperts)
        sort_test(aperts)

    def test_extract_mask_datanotfound(self, cpl_image_fill_test_create):
        imd = cpl_image_fill_test_create(512, 512)
        bpm = cpl.core.Mask(512, 512)
        with pytest.raises(cpl.core.DataNotFoundError):
            cpl.drs.Apertures.extract_mask(imd, bpm)

    def test_extract_mask(
        self, cpl_image_fill_test_create, apertures_test_one, sort_test
    ):
        imd = cpl_image_fill_test_create(512, 512)
        bpm = cpl.core.Mask(512, 512)
        bpm = ~bpm
        aperts = cpl.drs.Apertures.extract_mask(imd, bpm)
        apertures_test_one(aperts)
        sort_test(aperts)

    @pytest.mark.parametrize(
        "image_and_bpm_1",
        [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT],
        indirect=["image_and_bpm_1"],
    )
    def test_thresholding_statistics_idx(
        self, image_and_bpm_1, apertures_test_one_idx, sort_test_idx
    ):
        img, bpm = image_and_bpm_1
        imb = copy.deepcopy(img)
        imb.reject_from_mask(bpm)

        bin_img = cpl.core.Mask(img, 0.5, np.finfo(np.double).max)
        labels, count = cpl.core.Image.labelise_create(bin_img)
        # set bad pixel
        img.reject(5, 5)

        # Compute statistics on detected apertures
        aperts = cpl.drs.Apertures(img, labels)
        apertures_test_one_idx(aperts)
        sort_test_idx(aperts)
        assert len(aperts) == 2
        # Get highest peak
        aperts.sort_by_max()
        # Extra from sort test since it also checks the image
        assert aperts[0].max == img.get_max()
        assert aperts[1].max <= aperts[0].max

    @pytest.mark.parametrize(
        "image_and_bpm_1",
        [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT],
        indirect=["image_and_bpm_1"],
    )
    def test_thresholding_statistics_iter(
        self, image_and_bpm_1, apertures_test_one_iter, sort_test_idx
    ):
        img, bpm = image_and_bpm_1
        imb = copy.deepcopy(img)
        imb.reject_from_mask(bpm)

        bin_img = cpl.core.Mask(img, 0.5, np.finfo(np.double).max)
        labels, count = cpl.core.Image.labelise_create(bin_img)
        # set bad pixel
        img.reject(5, 5)

        # Compute statistics on detected apertures
        aperts = cpl.drs.Apertures(img, labels)
        apertures_test_one_iter(aperts)

    @pytest.mark.parametrize(
        "image_and_bpm_1",
        [cpl.core.Type.DOUBLE, cpl.core.Type.FLOAT, cpl.core.Type.INT],
        indirect=["image_and_bpm_1"],
    )
    def test_repr(self, image_and_bpm_1, apertures_test_one_iter, sort_test_idx):
        img, bpm = image_and_bpm_1
        imb = copy.deepcopy(img)
        imb.reject_from_mask(bpm)

        bin_img = cpl.core.Mask(img, 0.5, np.finfo(np.double).max)
        labels, count = cpl.core.Image.labelise_create(bin_img)
        # set bad pixel
        img.reject(5, 5)

        # Compute statistics on detected apertures
        aperts = cpl.drs.Apertures(img, labels)
        assert repr(aperts) == """<cpl.drs.Apertures, 2 Apertures>"""

    def test_dump_file(self, tmpdir):
        # load test image with known apertures, compare with truth string obtained from CPL
        truth = """#        X      Y XCENTROID YCENTROID   XMAX   YMAX   XMIN   YMIN    pix    max    min   mean    med    dev   flux
  1  356.0  187.0     355.9     186.9    355    186    357    187      9   0.92   0.40   0.65   0.55   0.21     5.88
  2  285.0  223.0     284.9     223.1    284    222    286    222      9   0.98   0.27   0.67   0.63   0.26     6.07
  3  222.5  302.0     222.2     302.0    222    302    224    303     12   0.88   0.27   0.61   0.61   0.22     7.27
  4  156.5  305.0     156.6     305.1    158    305    156    306     12   0.97   0.33   0.64   0.57   0.24     7.64
  5  361.0  317.5     361.0     317.4    360    317    361    318     12   0.89   0.26   0.61   0.66   0.20     7.27
  6  330.0  377.0     330.1     377.1    331    378    330    377      9   0.98   0.40   0.63   0.62   0.18     5.68
"""
        to_save = tmpdir.mkdir("dump_files_tmp").join("apertures_dump.txt")
        IMAGESZ2 = 512
        imd = cpl.core.Image.load(
            os.path.dirname(os.path.realpath(__file__))
            + "/test_images/apertures_test_image.fits"
        )
        sigmas = cpl.core.Vector([5, 2, 1, 0.5])
        aperts = cpl.drs.Apertures.extract_window(
            imd,
            sigmas,
            (
                int(IMAGESZ2 / 4),
                int(IMAGESZ2 / 4),
                int(3 * IMAGESZ2 / 4),
                int(3 * IMAGESZ2 / 4),
            ),
        )
        aperts.Apertures.dump(filename=str(to_save), mode="w")
        with open(to_save) as saved_file:
            file_text = saved_file.read()
            assert file_text == truth

    def test_dump_string(self, tmpdir):
        # load test image with known apertures, compare with truth string obtained from CPL
        truth = """#        X      Y XCENTROID YCENTROID   XMAX   YMAX   XMIN   YMIN    pix    max    min   mean    med    dev   flux
  1  356.0  187.0     355.9     186.9    355    186    357    187      9   0.92   0.40   0.65   0.55   0.21     5.88
  2  285.0  223.0     284.9     223.1    284    222    286    222      9   0.98   0.27   0.67   0.63   0.26     6.07
  3  222.5  302.0     222.2     302.0    222    302    224    303     12   0.88   0.27   0.61   0.61   0.22     7.27
  4  156.5  305.0     156.6     305.1    158    305    156    306     12   0.97   0.33   0.64   0.57   0.24     7.64
  5  361.0  317.5     361.0     317.4    360    317    361    318     12   0.89   0.26   0.61   0.66   0.20     7.27
  6  330.0  377.0     330.1     377.1    331    378    330    377      9   0.98   0.40   0.63   0.62   0.18     5.68
"""
        IMAGESZ2 = 512
        imd = cpl.core.Image.load(
            os.path.dirname(os.path.realpath(__file__))
            + "/test_images/apertures_test_image.fits"
        )
        sigmas = cpl.core.Vector([5, 2, 1, 0.5])
        aperts = cpl.drs.Apertures.extract_window(
            imd,
            sigmas,
            (
                int(IMAGESZ2 / 4),
                int(IMAGESZ2 / 4),
                int(3 * IMAGESZ2 / 4),
                int(3 * IMAGESZ2 / 4),
            ),
        )
        outp = str(aperts.Apertures)
        assert outp == truth
        assert repr(aperts.Apertures) == """<cpl.drs.Apertures, 6 Apertures>"""
        assert isinstance(aperts.Apertures.dump(show=False), str)

    def test_dump_stdout(self, tmpdir):
        # load test image with known apertures, compare with truth string obtained from CPL
        truth = """#        X      Y XCENTROID YCENTROID   XMAX   YMAX   XMIN   YMIN    pix    max    min   mean    med    dev   flux
  1  356.0  187.0     355.9     186.9    355    186    357    187      9   0.92   0.40   0.65   0.55   0.21     5.88
  2  285.0  223.0     284.9     223.1    284    222    286    222      9   0.98   0.27   0.67   0.63   0.26     6.07
  3  222.5  302.0     222.2     302.0    222    302    224    303     12   0.88   0.27   0.61   0.61   0.22     7.27
  4  156.5  305.0     156.6     305.1    158    305    156    306     12   0.97   0.33   0.64   0.57   0.24     7.64
  5  361.0  317.5     361.0     317.4    360    317    361    318     12   0.89   0.26   0.61   0.66   0.20     7.27
  6  330.0  377.0     330.1     377.1    331    378    330    377      9   0.98   0.40   0.63   0.62   0.18     5.68
"""
        img_path = (
            os.path.dirname(os.path.realpath(__file__))
            + "/test_images/apertures_test_image.fits"
        )
        cmd = "import cpl; "
        cmd += """IMAGESZ2 = 512 ; """
        cmd += """imd = cpl.core.Image.load('{}'); """.format(img_path)
        cmd += """sigmas = cpl.core.Vector([5, 2, 1, 0.5]); """
        cmd += """aperts = cpl.drs.Apertures.extract_window(imd, sigmas, (int(IMAGESZ2 / 4), int(IMAGESZ2 / 4), int(3 * IMAGESZ2 / 4), int(3 * IMAGESZ2 / 4))); """
        cmd += """aperts.Apertures.dump() """
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        assert outp == truth
