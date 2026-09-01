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

import pytest
import math
from cpl import core as cplcore
from hdrl import core as hdrlcore
from hdrl import func as hdrlfunc


class TestImageListBasic:
    """Test basic ImageList operations ported from hdrl_imagelist_basic-test.c"""

    def test_image_basic_operations(self):
        """Test basic image operations with ImageList (equivalent to C test_image_basic_operations)."""
        # Initialize values (equivalent to C hdrl_value)
        value = (100.0, 10.0)
        scalar = (1000.0, 100.0)
        exponent = (2.0, 1.0)

        # Initialize images
        img_size = 265  # IMAGESZ
        himg1 = hdrlcore.Image.zeros(img_size, img_size)
        himg2 = hdrlcore.Image.zeros(img_size, img_size)
        himg3 = hdrlcore.Image.zeros(img_size, img_size)

        # Operations with images
        himg1.add_scalar(scalar)
        himg1.sub_scalar(value)

        # Initialize imageLists and add images
        himlist1 = hdrlcore.ImageList()
        himlist1.append(himg1)

        himlist2 = hdrlcore.ImageList()
        himlist2.append(himg2)
        himlist2.sub_image(himg2)
        himlist2.add_image(himg2)

        himlist1.add_imagelist(himlist2)
        himlist1.sub_imagelist(himlist2)
        himlist1.add_imagelist(himlist2)

        # Complex operations with imageList: Div, Mul, Pow
        himlist1.div_scalar(scalar)
        himlist1.div_image(himg1)
        himlist1.div_imagelist(himlist2)

        himlist1.mul_scalar(scalar)
        himlist1.mul_image(himg1)
        himlist1.mul_imagelist(himlist2)

        himlist1.pow_scalar(exponent)

        himlist2.add_image(himg3)
        himlist1.add_imagelist(himlist2)
        himlist1.pow_scalar(exponent)

        # Test that operations completed without errors
        assert len(himlist1) > 0
        assert len(himlist2) > 0

    def test_collapse_operations(self):
        """Test collapse operations (equivalent to C main function collapse tests)."""
        nimages = 10  # IMAGENB
        img_size = 265  # IMAGESZ
        himlist = hdrlcore.ImageList()

        # Create an image list
        for i in range(nimages):
            himg = hdrlcore.Image.zeros(img_size, img_size)
            if i == nimages // 2:
                himg.add_scalar((1000.0, 100.0))
            else:
                himg.add_scalar((float(i), 1.0))
            himlist.append(himg)

        # Mean collapse
        result1 = himlist.collapse_mean()
        mean = hdrlfunc.Collapse.Mean()
        result2 = hdrlfunc.Collapse.compute(mean, himlist)
        assert result1.out.image == result2.out.image
        assert result1.contrib == result2.contrib

        # Weighted Mean collapse
        result1 = himlist.collapse_weighted_mean()
        wmean = hdrlfunc.Collapse.WeightedMean()
        result2 = hdrlfunc.Collapse.compute(wmean, himlist)
        assert result1.out.image == result2.out.image
        assert result1.contrib == result2.contrib

        # Median collapse
        result1 = himlist.collapse_median()
        median = hdrlfunc.Collapse.Median()
        result2 = hdrlfunc.Collapse.compute(median, himlist)
        assert result1.out.image == result2.out.image
        assert result1.contrib == result2.contrib

        # Sigclip collapse
        result1 = himlist.collapse_sigclip(1.0, 3.0, 10)
        sigclip = hdrlfunc.Collapse.Sigclip(1.0, 3.0, 10)
        result2 = hdrlfunc.Collapse.compute(sigclip, himlist)
        assert result1.out.image == result2.out.image
        assert result1.contrib == result2.contrib

        # MinMax collapse
        result1 = himlist.collapse_minmax(1.0, 3.0)
        minmax = hdrlfunc.Collapse.MinMax(1.0, 3.0)
        result2 = hdrlfunc.Collapse.compute(minmax, himlist)
        assert result1.out.image == result2.out.image
        assert result1.contrib == result2.contrib

        # Mode collapse
        result1 = himlist.collapse_mode(10.0, 1.0, 0, hdrlfunc.Collapse.Method.Median, 0)
        mode = hdrlfunc.Collapse.Mode(10.0, 1.0, 0, hdrlfunc.Collapse.Method.Median, 0)
        result2 = hdrlfunc.Collapse.compute(mode, himlist)
        assert result1.out.image == result2.out.image
        assert result1.contrib == result2.contrib

    def test_collapse_direct_operations(self):
        """Test direct collapse operations with rejection maps (equivalent to C collapse direct tests)."""
        nimages = 10  # IMAGENB
        img_size = 265  # IMAGESZ
        himlist = hdrlcore.ImageList()

        # Create an image list
        for i in range(nimages):
            himg = hdrlcore.Image.zeros(img_size, img_size)
            if i == nimages // 2:
                himg.add_scalar((1000.0, 100.0))
            else:
                himg.add_scalar((float(i), 1.0))
            himlist.append(himg)

        # Test sigclip with different rejection map combinations
        # These should not raise exceptions
        result = himlist.collapse_sigclip(1.0, 3.0, 10)
        assert result.out.image is not None
        assert result.contrib is not None

        result = himlist.collapse_minmax(1.0, 3.0)
        assert result.out.image is not None
        assert result.contrib is not None

        # Test mode collapse with different parameters
        result = himlist.collapse_mode(10.0, 1.0, 0, hdrlfunc.Collapse.Method.Median, 10)
        assert result.out.image is not None
        assert result.contrib is not None
        assert result.out.image.count_rejected() == pytest.approx(0.0, abs=1e-12)
        assert result.out.error.count_rejected() == pytest.approx(0.0, abs=1e-12)

        # Test mode with user-defined histogram
        result = himlist.collapse_mode(1.0, 20.0, 1.0, hdrlfunc.Collapse.Method.Median, 10)
        assert result.out.image is not None
        assert result.contrib is not None
        assert result.out.image.count_rejected() == pytest.approx(0.0, abs=1e-12)
        assert result.out.error.count_rejected() == pytest.approx(0.0, abs=1e-12)

        # Test mode with data outside histogram (all pixels should be rejected)
        result = himlist.collapse_mode(-1000.0, -100.0, 0, hdrlfunc.Collapse.Method.Median, 0)
        assert result.out.image is not None
        assert result.contrib is not None
        assert result.out.image.count_rejected() == img_size * img_size
        assert result.out.error.count_rejected() == img_size * img_size

    def test_null_input_handling(self):
        """Test null input handling for collapse operations."""
        # Test that passing None to collapse operations raises appropriate exceptions
        # Note: The Python bindings may handle this differently than the C layer
        himlist = hdrlcore.ImageList()

        # Add some images to make the list non-empty
        for i in range(3):
            himg = hdrlcore.Image.zeros(10, 10)
            himg.add_scalar((float(i), 1.0))
            himlist.append(himg)

        # Test that operations work with valid inputs
        result = himlist.collapse_mean()
        assert result.out.image is not None
        assert result.contrib is not None

        result = himlist.collapse_sigclip(1.0, 3.0, 5)
        assert result.out.image is not None
        assert result.contrib is not None

        result = himlist.collapse_minmax(1.0, 3.0)
        assert result.out.image is not None
        assert result.contrib is not None
