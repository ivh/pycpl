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


class TestImageListView:
    """Test ImageList view operations ported from hdrl_imagelist_view-test.c"""

    def test_basic_operations(self):
        """Test basic ImageList operations that are available in Python bindings."""
        nimages = 20
        ysize = 73  # YSIZE
        xsize = 50  # XSIZE
        hlist = hdrlcore.ImageList()
        clist = cplcore.ImageList()

        # Create an image list
        for i in range(nimages):
            ima = cplcore.Image.zeros(xsize, ysize, cplcore.Type.DOUBLE)
            ima_err = cplcore.Image.zeros(xsize, ysize, cplcore.Type.DOUBLE)

            ima_err.add_scalar(1.0)
            ima.reject(1, 5)
            if (i % 5) == 0:
                ima.reject(2, 5)

            himg = hdrlcore.Image(ima, ima_err)
            hlist.append(himg)
            clist.append(ima)

        # Test basic operations that are available
        assert len(hlist) == nimages
        assert hlist.size_x == xsize
        assert hlist.size_y == ysize

        # Test collapse operations
        result = hlist.collapse_mean()
        assert result.out.image is not None
        assert result.contrib is not None
        assert result.out.image.width == xsize
        assert result.out.image.height == ysize

        # Test consistency
        is_consistent = hlist.is_consistent()
        assert is_consistent == 0  # Should be consistent for same-sized images

        # Test duplication
        hlist_copy = hlist.duplicate()
        assert len(hlist_copy) == len(hlist)
        assert hlist_copy[0].width == hlist[0].width
        assert hlist_copy[0].height == hlist[0].height

    def test_imagelist_operations(self):
        """Test operations between ImageLists."""
        hl1 = hdrlcore.ImageList()
        hl2 = hdrlcore.ImageList()

        # Add some test images
        for i in range(3):
            img1 = hdrlcore.Image.zeros(10, 10)
            img1.add_scalar((float(i), 1.0))
            hl1.append(img1)

            img2 = hdrlcore.Image.zeros(10, 10)
            img2.add_scalar((float(i) * 2, 1.0))
            hl2.append(img2)

        # Test adding imagelists
        hl1.add_imagelist(hl2)

        # Check that the operation worked
        for i in range(3):
            pixel = hl1[i].get_pixel(0, 0)
            expected_data = float(i) + float(i) * 2
            assert pixel.data == expected_data

        # Test subtracting imagelists
        hl1.sub_imagelist(hl2)

        # Check that the operation worked
        for i in range(3):
            pixel = hl1[i].get_pixel(0, 0)
            expected_data = float(i)  # Back to original
            assert pixel.data == expected_data

    def test_scalar_operations(self):
        """Test scalar operations on ImageLists."""
        hl = hdrlcore.ImageList()

        # Add some test images
        for i in range(3):
            img = hdrlcore.Image.zeros(5, 5)
            img.add_scalar((float(i), 1.0))
            hl.append(img)

        # Test adding scalar
        hl.add_scalar((10.0, 1.0))
        for i in range(3):
            pixel = hl[i].get_pixel(0, 0)
            expected_data = float(i) + 10.0
            assert pixel.data == expected_data

        # Test subtracting scalar
        hl.sub_scalar((5.0, 1.0))
        for i in range(3):
            pixel = hl[i].get_pixel(0, 0)
            expected_data = float(i) + 10.0 - 5.0
            assert pixel.data == expected_data

        # Test multiplying scalar
        hl.mul_scalar((2.0, 1.0))
        for i in range(3):
            pixel = hl[i].get_pixel(0, 0)
            expected_data = (float(i) + 10.0 - 5.0) * 2.0
            assert pixel.data == expected_data

        # Test dividing scalar
        hl.div_scalar((2.0, 1.0))
        for i in range(3):
            pixel = hl[i].get_pixel(0, 0)
            expected_data = (float(i) + 10.0 - 5.0) * 2.0 / 2.0
            assert pixel.data == expected_data

    def test_image_operations(self):
        """Test operations with individual images."""
        hl = hdrlcore.ImageList()

        # Add some test images
        for i in range(3):
            img = hdrlcore.Image.zeros(5, 5)
            img.add_scalar((float(i), 1.0))
            hl.append(img)

        # Create a test image
        test_img = hdrlcore.Image.zeros(5, 5)
        test_img.add_scalar((5.0, 1.0))

        # Test adding image
        hl.add_image(test_img)
        for i in range(3):
            pixel = hl[i].get_pixel(0, 0)
            expected_data = float(i) + 5.0
            assert pixel.data == expected_data

        # Test subtracting image
        hl.sub_image(test_img)
        for i in range(3):
            pixel = hl[i].get_pixel(0, 0)
            expected_data = float(i)  # Back to original
            assert pixel.data == expected_data

    def test_consistency_checks(self):
        """Test consistency checks."""
        hl = hdrlcore.ImageList()

        # Add consistent images
        for i in range(3):
            img = hdrlcore.Image.zeros(10, 10)
            hl.append(img)

        # Test that consistent list returns 0
        assert hl.is_consistent() == 0

        # Test empty list consistency
        empty_hl = hdrlcore.ImageList()
        assert empty_hl.is_consistent() == 1  # Empty list is inconsistent
