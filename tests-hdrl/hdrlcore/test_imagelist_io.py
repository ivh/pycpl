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


class TestImageListIO:
    """Test ImageList IO operations ported from hdrl_imagelist_io-test.c"""

    def test_create(self):
        """Test ImageList creation (equivalent to C test_create)."""
        # Create data initial data
        data = cplcore.ImageList()
        errs = cplcore.ImageList()
        img = cplcore.Image.zeros(64, 64, cplcore.Type.DOUBLE)
        err = cplcore.Image.zeros(64, 64, cplcore.Type.DOUBLE)
        img.add_scalar(1.0)
        err.add_scalar(0.05)

        # Create expected results (err / sqrt(nz) for mean)
        n = 5
        for i in range(n):
            data.append(img.duplicate())
            errs.append(err.duplicate())

        # Create new hdrl_imagelist
        hl = hdrlcore.ImageList(data, errs)
        assert len(hl) == n

        # Test that the created ImageList has the expected properties
        assert hl.size_x == 64
        assert hl.size_y == 64

    def test_get(self):
        """Test ImageList get operations (equivalent to C test_get)."""
        # Test with None (should raise appropriate exceptions)
        # Note: Python doesn't have NULL pointers, so we test with empty list
        hl = hdrlcore.ImageList()

        # Test empty list properties
        assert len(hl) == 0

        # Test that size_x and size_y raise exceptions for empty list
        with pytest.raises(hdrlcore.IllegalInputError):
            _ = hl.size_x
        with pytest.raises(hdrlcore.IllegalInputError):
            _ = hl.size_y

        # Add an image and test properties
        hl.append(hdrlcore.Image.zeros(5, 6))
        assert len(hl) == 1
        assert hl.size_x == 5
        assert hl.size_y == 6

    def test_interface(self):
        """Test ImageList interface operations (equivalent to C test_interface)."""
        hl = hdrlcore.ImageList()

        # Test that operations work correctly
        # Note: The C test tests iterator functionality which may not be directly
        # available in Python bindings, so we test basic operations instead

        # Add an image
        hl.append(hdrlcore.Image.zeros(5, 5))
        assert len(hl) == 1

        # Test that we can access the image
        img = hl[0]
        assert img.width == 5
        assert img.height == 5

        # Test that we can modify the image
        img.add_scalar((10.0, 1.0))
        pixel = img.get_pixel(0, 0)
        assert pixel.data == pytest.approx(10.0)
        assert pixel.error == pytest.approx(1.0)

    def test_basic_operations(self):
        """Test basic ImageList operations."""
        hl = hdrlcore.ImageList()

        # Test append and length
        assert len(hl) == 0
        hl.append(hdrlcore.Image.zeros(10, 10))
        assert len(hl) == 1

        # Test indexing
        img = hl[0]
        assert img.width == 10
        assert img.height == 10

        # Test setting pixel
        img.set_pixel(0, 0, (5.0, 1.0))
        pixel = img.get_pixel(0, 0)
        assert pixel.data == pytest.approx(5.0)
        assert pixel.error == pytest.approx(1.0)

        # Test adding scalar to entire list
        hl.add_scalar((10.0, 2.0))
        pixel = img.get_pixel(0, 0)
        assert pixel.data == pytest.approx(15.0)  # 5.0 + 10.0
        assert pixel.error == pytest.approx(math.sqrt(1.0**2 + 2.0**2))

    def test_imagelist_operations(self):
        """Test operations between ImageLists."""
        hl1 = hdrlcore.ImageList()
        hl2 = hdrlcore.ImageList()

        # Create test images
        for i in range(3):
            img1 = hdrlcore.Image.zeros(5, 5)
            img1.add_scalar((float(i), 1.0))
            hl1.append(img1)

            img2 = hdrlcore.Image.zeros(5, 5)
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

    def test_error_handling(self):
        """Test error handling for ImageList operations."""
        hl = hdrlcore.ImageList()

        # Test operations on empty list
        # These should work without errors
        hl.add_scalar((1.0, 1.0))
        hl.sub_scalar((1.0, 1.0))
        hl.mul_scalar((2.0, 1.0))
        hl.div_scalar((2.0, 1.0))

        # Test that we can still add images after operations
        hl.append(hdrlcore.Image.zeros(5, 5))
        assert len(hl) == 1
