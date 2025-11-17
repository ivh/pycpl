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
import random

IMAGESZ = 256
off_y_values = [0.0, 13.5, 3.5, 8.5, 32.5, 22.5, 18.5, -56.5, 3.5, 10.5]
off_x_values = [0.0, -6.5, -18.5, 54.5, 33.5, 46.5, -3.5, 36.5, 42.5, -13.5]
# Create imagelist of images shifted from the 1st image, used only once


@pytest.fixture
def imagelist_shifted():
    NFRAMES = 10

    def _cpl_imagelist_fill_shifted(imlist, napp, dx, dy):
        """
        @brief      Create imagelist of images shifted from the 1st image
        @param self Imagelist with one image to append to
        @param napp The number of shifted images to append
        @param dx   The array of n+1 X-shifts (0.0 as 1st element)
        @param dy   The array of n+1 Y-shifts (0.0 as 1st element)
        """
        img = imlist[0]
        imtype = img.type
        nx = img.shape[1]
        ny = img.shape[1]
        ishift_0 = [0, 0]
        ishift_x = [1, 0]
        ishift_y = [0, 1]
        xyradius = 2.0
        # (1 + (cpl_size)((CPL_KERNEL_TABSPERPIX) * (CPL_KERNEL_DEF_WIDTH)))
        shift_x = cpl.core.Polynomial(2)
        shift_y = cpl.core.Polynomial(2)
        assert len(imlist) == 1
        assert 1 <= napp
        # Identity transforms
        shift_x.set_coeff(ishift_x, 1.0)
        shift_y.set_coeff(ishift_y, 1.0)
        # Resampling profile
        xyprofile = cpl.core.Vector.kernel_profile(
            cpl.core.Kernel.DEFAULT, xyradius, 2001
        )
        # Append images to image set
        for i in range(1, napp + 1):
            copy = cpl.core.Image.zeros(nx, ny, imtype)
            # shift in X and Y
            shift_x.set_coeff(ishift_0, dx[i])
            shift_y.set_coeff(ishift_0, dy[i])

            copy = img.warp_polynomial(
                shift_x, shift_y, xyprofile, xyradius, xyprofile, xyradius
            )
            imlist.append(copy)
        assert len(imlist) == napp + 1

    # iset object modified by end of function
    iset = cpl.core.ImageList()
    iset.append(
        cpl.core.Image.load(
            os.path.dirname(os.path.realpath(__file__))
            + "/test_images/geom_img_test_image.fits"
        )
    )
    _cpl_imagelist_fill_shifted(iset, NFRAMES - 1, off_x_values, off_y_values)

    assert len(iset) == NFRAMES
    return iset


class TestGeometric:
    # Test with empty imagelist
    def test_offset_saa_empty(self):
        iset = cpl.core.ImageList()
        offs = cpl.core.Bivector(([0.0], [0.0]))
        with pytest.raises(cpl.core.IllegalInputError):
            cpl.drs.geometric_transforms.offset_saa(
                iset,
                offs,
                cpl.core.Kernel.DEFAULT,
                0,
                0,
                cpl.drs.geometric_transforms.Combine.FIRST,
            )

    # Insert one image into imagelist
    @pytest.mark.parametrize(
        "combine",
        [
            cpl.drs.geometric_transforms.Combine.INTERSECT,
            cpl.drs.geometric_transforms.Combine.UNION,
            cpl.drs.geometric_transforms.Combine.FIRST,
        ],
    )
    def test_offset_saa_single(self, combine, cpl_image_fill_test_create):
        offs = cpl.core.Bivector(([0.0], [0.0]))
        img = cpl_image_fill_test_create(IMAGESZ, IMAGESZ)
        iset = cpl.core.ImageList([img])

        # Shift and add single image with different combine methods
        res = cpl.drs.geometric_transforms.offset_saa(
            iset, offs, cpl.core.Kernel.DEFAULT, 0, 0, combine
        )
        contribution = res.contribution
        combined = res.combined
        assert contribution.type == cpl.core.Type.INT
        assert contribution.get_min() == 1
        assert contribution.get_max() == 1
        assert combined.count_rejected() == 0
        assert contribution.shape == combined.shape
        assert img == combined

    @pytest.mark.parametrize(
        "kernel",
        [
            cpl.core.Kernel.DEFAULT,
            cpl.core.Kernel.NEAREST,
            cpl.core.Kernel.TANH,
            cpl.core.Kernel.SINC,
            cpl.core.Kernel.SINC2,
            cpl.core.Kernel.LANCZOS,
            cpl.core.Kernel.HAMMING,
            cpl.core.Kernel.HANN,
        ],
    )
    def test_offset_saa_kernels(self, kernel):
        nz = 2 + 10  # const int nz = 2 + NFRAMES;
        imglist = cpl.core.ImageList()
        offset = cpl.core.Bivector.zeros(nz)
        for iz in range(nz):
            img = cpl.core.Image.zeros(IMAGESZ, IMAGESZ, cpl.core.Type.FLOAT)
            img.add_scalar(float(nz - iz - nz / 5))
            imglist.append(img)
            offset.x[iz] = random.random() if iz != 0 else 0.0
            offset.y[iz] = random.random() if iz != 0 else 0.0
        res = cpl.drs.geometric_transforms.offset_saa(
            imglist,
            offset,
            kernel,
            int(nz / 5),
            int(nz / 4),
            cpl.drs.geometric_transforms.Combine.INTERSECT,
        )

        # So the CPL test files compares the values as integers (even though its comparing doubles),
        # and how that conversion works in C is just round to 0 decimal places
        assert (nz - nz / 5 - nz / 4 + 1) / 2.0 == pytest.approx(
            res.combined.get_max(), 0.5
        )
        assert nz - nz / 5 - nz / 4 == pytest.approx(res.contribution.get_max(), 0.5)

        central0 = res.combined.extract((2, 2, IMAGESZ - 3, IMAGESZ - 3))
        central1 = res.contribution.extract((2, 2, IMAGESZ - 3, IMAGESZ - 3))

        assert (nz - nz / 5 - nz / 4 + 1) / 2.0 == pytest.approx(
            central0.get_min(), 0.5
        )
        assert nz - nz / 5 - nz / 4 == pytest.approx(central1.get_min(), 0.5)

    def test_offset_fine_aperts(self, imagelist_shifted):
        MAX_SHIFT_ERROR2 = 0.1
        offs_est = cpl.core.Bivector((off_x_values, off_y_values))
        offs_est.x.add_scalar(2.0)
        offs_est.y.add_scalar(-3.0)
        sigmas = cpl.core.Vector([5, 2, 1, 0.5])
        # get some cross-correlation apertures
        aperts, pisigma = cpl.drs.Apertures.extract(imagelist_shifted[0], sigmas)
        naperts = len(aperts)
        assert 4 == naperts
        assert sigmas[pisigma] == 5
        aperts_pos = cpl.core.Bivector.zeros(naperts)
        for i in range(naperts):
            aperts_pos.x[i] = aperts.get_pos_x(i + 1)
            aperts_pos.y[i] = aperts.get_pos_y(i + 1)
        # Refine the offsets with cpl_geom_img_offset_fine
        correl = cpl.core.Vector.zeros(10)
        offs_ref, correl = cpl.drs.geometric_transforms.offset_fine(
            imagelist_shifted, offs_est, aperts_pos, 15, 15, 15, 15
        )
        assert len(offs_ref.x) == 10
        # assert len(offs_ref.x) == 0
        assert np.array(offs_ref.x) == pytest.approx(off_x_values, abs=MAX_SHIFT_ERROR2)
        assert np.array(offs_ref.y) == pytest.approx(off_y_values, abs=MAX_SHIFT_ERROR2)

    @pytest.mark.parametrize(
        "geom",
        [
            cpl.drs.geometric_transforms.Combine.INTERSECT,
            cpl.drs.geometric_transforms.Combine.UNION,
            cpl.drs.geometric_transforms.Combine.FIRST,
        ],
    )
    def test_offset_combine(self, geom, imagelist_shifted):
        rejmin = 1
        rejmax = 1
        # Refine offsets for the shifted imagelist with offset fine
        offs_est = cpl.core.Bivector((off_x_values, off_y_values))
        # Estimate distortion
        offs_est.x.add_scalar(2.0)
        offs_est.y.add_scalar(-3.0)
        sigmas = cpl.core.Vector([5, 2, 1, 0.5])
        aperts, pisigma = cpl.drs.Apertures.extract(imagelist_shifted[0], sigmas)
        naperts = len(aperts)
        assert 4 == naperts
        assert sigmas[pisigma] == 5
        aperts_pos = cpl.core.Bivector.zeros(naperts)
        for i in range(naperts):
            aperts_pos.x[i] = aperts.get_pos_x(i + 1)
            aperts_pos.y[i] = aperts.get_pos_y(i + 1)
        offs_ref, correl = cpl.drs.geometric_transforms.offset_fine(
            imagelist_shifted, offs_est, aperts_pos, 15, 15, 15, 15
        )

        # TODO cpl_image **combined2 = cpl_geom_img_offset_combine(iset, offs_ref, 0, NULL, NULL, NULL, 0,
        # 0, 0, 0, rejmin, rejmax, geom);
        # On line 293 of cpl_geom_img-test
        rejmin = 1
        rejmax = 1
        # cpl_geom_img_offset_combine() is just a wrapper around cpl_geom_img_offset_saa(), so
        # just ensure the equivalent call returns the same result
        res_combine = cpl.drs.geometric_transforms.offset_combine(
            imagelist_shifted, offs_ref, 1, 1, geom, False
        )
        # shift and add
        res_saa = cpl.drs.geometric_transforms.offset_saa(
            imagelist_shifted, offs_ref, cpl.core.Kernel.DEFAULT, rejmin, rejmax, geom
        )
        assert res_saa.combined == res_combine.combined
        assert res_saa.contribution == res_combine.contribution

    @pytest.mark.parametrize(
        "geom",
        [
            cpl.drs.geometric_transforms.Combine.INTERSECT,
            cpl.drs.geometric_transforms.Combine.UNION,
            cpl.drs.geometric_transforms.Combine.FIRST,
        ],
    )
    def test_offset_saa_combine_modes(self, geom, imagelist_shifted):
        rejmin = 1
        rejmax = 1
        NFRAMES = 10
        maximg = NFRAMES - rejmin - rejmax
        # Refine offsets for the shifted imagelist with offset fine
        offs_est = cpl.core.Bivector((off_x_values, off_y_values))
        # Estimate distortion
        offs_est.x.add_scalar(2.0)
        offs_est.y.add_scalar(-3.0)
        sigmas = cpl.core.Vector([5, 2, 1, 0.5])
        aperts, pisigma = cpl.drs.Apertures.extract(imagelist_shifted[0], sigmas)
        naperts = len(aperts)
        assert 4 == naperts
        assert sigmas[pisigma] == 5
        aperts_pos = cpl.core.Bivector.zeros(naperts)
        for i in range(naperts):
            aperts_pos.x[i] = aperts.get_pos_x(i + 1)
            aperts_pos.y[i] = aperts.get_pos_y(i + 1)
        offs_ref, correl = cpl.drs.geometric_transforms.offset_fine(
            imagelist_shifted, offs_est, aperts_pos, 15, 15, 15, 15
        )
        # shift and add
        res = cpl.drs.geometric_transforms.offset_saa(
            imagelist_shifted, offs_ref, cpl.core.Kernel.DEFAULT, rejmin, rejmax, geom
        )
        assert res.contribution.type == cpl.core.Type.INT
        if res.contribution.get_min() == 0:
            assert res.combined.count_rejected() > 0
        else:
            assert 1 <= res.contribution.get_min()
            assert res.combined.count_rejected() == 0
        assert res.combined.shape == res.contribution.shape

        if geom == cpl.drs.geometric_transforms.Combine.INTERSECT:
            assert res.contribution.get_max() == maximg
            assert 1 <= res.contribution.get_min()
        else:
            assert 0 <= res.contribution.get_min()
            assert res.contribution.get_max() <= maximg

    # Use all possible pairs of combine modes and kernels
    @pytest.mark.parametrize(
        "geom",
        [
            cpl.drs.geometric_transforms.Combine.INTERSECT,
            cpl.drs.geometric_transforms.Combine.UNION,
            cpl.drs.geometric_transforms.Combine.FIRST,
        ],
    )
    @pytest.mark.parametrize(
        "kernel",
        [
            cpl.core.Kernel.DEFAULT,
            cpl.core.Kernel.NEAREST,
            cpl.core.Kernel.TANH,
            cpl.core.Kernel.SINC,
            cpl.core.Kernel.SINC2,
            cpl.core.Kernel.LANCZOS,
            cpl.core.Kernel.HAMMING,
            cpl.core.Kernel.HANN,
        ],
    )
    def test_offset_saa_combine_modes_and_kernels_nobad(
        self, geom, kernel, imagelist_shifted
    ):
        # Shift and add without bad pixels
        for image in imagelist_shifted:
            image.accept_all()
        rejmin = 1
        rejmax = 1
        NFRAMES = 10
        maximg = NFRAMES - rejmin - rejmax
        # Refine offsets for the shifted imagelist with offset fine
        offs_est = cpl.core.Bivector((off_x_values, off_y_values))
        # Estimate distortion
        offs_est.x.add_scalar(2.0)
        offs_est.y.add_scalar(-3.0)
        sigmas = cpl.core.Vector([5, 2, 1, 0.5])
        aperts, pisigma = cpl.drs.Apertures.extract(imagelist_shifted[0], sigmas)
        naperts = len(aperts)
        assert 4 == naperts
        assert sigmas[pisigma] == 5
        aperts_pos = cpl.core.Bivector.zeros(naperts)
        for i in range(naperts):
            aperts_pos.x[i] = aperts.get_pos_x(i + 1)
            aperts_pos.y[i] = aperts.get_pos_y(i + 1)
        offs_ref, correl = cpl.drs.geometric_transforms.offset_fine(
            imagelist_shifted, offs_est, aperts_pos, 15, 15, 15, 15
        )

        res = cpl.drs.geometric_transforms.offset_saa(
            imagelist_shifted, offs_ref, kernel, rejmin, rejmax, geom
        )
        assert res.contribution.type == cpl.core.Type.INT
        if res.contribution.get_min() == 0:
            assert res.combined.count_rejected() > 0
        else:
            assert 1 <= res.contribution.get_min()
            assert res.combined.count_rejected() == 0
        assert res.combined.shape == res.contribution.shape

        if geom == cpl.drs.geometric_transforms.Combine.INTERSECT:
            assert res.contribution.get_max() == maximg
            assert 1 <= res.contribution.get_min()
        else:
            assert 0 <= res.contribution.get_min()
            assert res.contribution.get_max() <= maximg

    # Shift and add of two uniform images, no offsets

    @pytest.mark.parametrize(
        "geom",
        [
            cpl.drs.geometric_transforms.Combine.INTERSECT,
            cpl.drs.geometric_transforms.Combine.UNION,
            cpl.drs.geometric_transforms.Combine.FIRST,
        ],
    )
    @pytest.mark.parametrize(
        "kernel",
        [
            cpl.core.Kernel.DEFAULT,
            cpl.core.Kernel.NEAREST,
            cpl.core.Kernel.TANH,
            cpl.core.Kernel.SINC,
            cpl.core.Kernel.SINC2,
            cpl.core.Kernel.LANCZOS,
            cpl.core.Kernel.HAMMING,
            cpl.core.Kernel.HANN,
        ],
    )
    def test_offset_saa_combine_modes_and_kernels_uniform(
        self, geom, kernel, imagelist_shifted
    ):
        # Shift and add without the bad pixels
        img = cpl.core.Image.load(
            os.path.dirname(os.path.realpath(__file__))
            + "/test_images/geom_img_test_image.fits"
        )
        img.threshold(1.0, 1.0, 1.0, 1.0)
        img.accept_all()
        img2 = img.duplicate()
        iset = cpl.core.ImageList([img, img2])
        offs_ref = cpl.core.Bivector(([0, 0], [0, 0]))
        res = cpl.drs.geometric_transforms.offset_saa(
            iset, offs_ref, kernel, 0, 0, geom
        )

        assert res.combined.shape == (IMAGESZ, IMAGESZ)
        assert res.combined.shape == res.contribution.shape
        assert res.contribution.type == cpl.core.Type.INT
        if res.contribution.get_min() == 0:
            assert res.combined.count_rejected() > 0
        else:
            assert 1 <= res.contribution.get_min()
            assert res.combined.count_rejected() == 0

        assert res.contribution.get_max() == 2

        assert (
            1 if geom == cpl.drs.geometric_transforms.Combine.INTERSECT else 0
        ) <= res.contribution.get_min()
        # Resampling introduces noise at the edge, so only compare the centres

        res_central = res.combined.extract((2, 2, IMAGESZ - 3, IMAGESZ - 3))
        img_central = img.extract((2, 2, IMAGESZ - 3, IMAGESZ - 3))
        assert res_central == img_central

    # try to combine two images, the second shifted along the X-axis
    @pytest.mark.parametrize(
        "geom",
        [
            cpl.drs.geometric_transforms.Combine.INTERSECT,
            cpl.drs.geometric_transforms.Combine.UNION,
            cpl.drs.geometric_transforms.Combine.FIRST,
        ],
    )
    @pytest.mark.parametrize(
        "kernel",
        [
            cpl.core.Kernel.DEFAULT,
            cpl.core.Kernel.NEAREST,
            cpl.core.Kernel.TANH,
            cpl.core.Kernel.SINC,
            cpl.core.Kernel.SINC2,
            cpl.core.Kernel.LANCZOS,
            cpl.core.Kernel.HAMMING,
            cpl.core.Kernel.HANN,
        ],
    )
    def test_offset_saa_combine_modes_and_kernels_x_shift(
        self, geom, kernel, imagelist_shifted
    ):
        MAX_SHIFT_ERROR1 = 15
        # Shift and add without the bad pixels
        img = cpl.core.Image.load(
            os.path.dirname(os.path.realpath(__file__))
            + "/test_images/geom_img_test_image.fits"
        )
        img.threshold(1.0, 1.0, 1.0, 1.0)
        img.accept_all()
        img2 = img.duplicate()
        img.shift(15, 0)
        img.accept_all()

        iset = cpl.core.ImageList([img, img2])
        offs_ref = cpl.core.Bivector(([0, 15], [0, 0]))
        res = cpl.drs.geometric_transforms.offset_saa(
            iset, offs_ref, kernel, 0, 0, geom
        )

        assert res.contribution.get_max() == 2

        if res.contribution.get_min() == 0:
            assert res.combined.count_rejected() > 0
        else:
            assert res.combined.count_rejected() == 0

        assert res.combined.height == IMAGESZ

        expected_x = {
            cpl.drs.geometric_transforms.Combine.INTERSECT: IMAGESZ - MAX_SHIFT_ERROR1,
            cpl.drs.geometric_transforms.Combine.FIRST: IMAGESZ,
            cpl.drs.geometric_transforms.Combine.UNION: IMAGESZ + MAX_SHIFT_ERROR1,
        }

        assert res.combined.width == expected_x[geom]

        if geom == cpl.drs.geometric_transforms.Combine:
            assert res.contribution.get_min() == 1
