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

import numpy as np
import pytest
from cpl import core as cplcore
from hdrl import core as hdrlcore
from hdrl import func as hdrlfunc


class TestFringe:
    def create_fringe_images(self, nima, ima_sx, ima_sy, base_value=100.0, fringe_amplitude=10.0):
        """Create a list of images with simulated fringe patterns."""
        imglist = hdrlcore.ImageList()
        for _i in range(nima):
            # Create base image with noise
            data = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
            data.add_scalar(base_value)

            # Add fringe pattern (simple sinusoidal pattern)
            for y in range(ima_sy):
                for x in range(ima_sx):
                    fringe = fringe_amplitude * math.sin(2 * math.pi * x / 50.0) * math.cos(2 * math.pi * y / 50.0)
                    data[y][x] += fringe

            # Add some noise
            rng = np.random.default_rng()
            noise = rng.normal(0, 2.0, (ima_sy, ima_sx))
            for y in range(ima_sy):
                for x in range(ima_sx):
                    data[y][x] += noise[y, x]

            # Create error image
            errors = data.duplicate()
            errors.power(0.5)

            # Create HDRL image
            image = hdrlcore.Image(data, errors)
            imglist.append(image)

        return imglist

    def create_object_masks(self, nima, ima_sx, ima_sy):
        """Create a list of object masks."""
        masklist = cplcore.ImageList()
        for _i in range(nima):
            # Create mask with some objects
            mask = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)

            # Add some objects (bright regions)
            for y in range(ima_sy // 4, 3 * ima_sy // 4):
                for x in range(ima_sx // 4, 3 * ima_sx // 4):
                    if (x - ima_sx // 2) ** 2 + (y - ima_sy // 2) ** 2 < (ima_sx // 8) ** 2:
                        mask[y][x] = 1.0

            masklist.append(mask)

        return masklist

    def create_static_mask(self, ima_sx, ima_sy, fringe_regions=None):
        """Create a static mask for fringe regions."""
        stat_mask = cplcore.Mask(ima_sx, ima_sy)

        if fringe_regions is None:
            # Default fringe regions (edges and corners)
            fringe_regions = [
                (0, 0, ima_sx // 4, ima_sy // 4),  # Top-left
                (3 * ima_sx // 4, 0, ima_sx, ima_sy // 4),  # Top-right
                (0, 3 * ima_sy // 4, ima_sx // 4, ima_sy),  # Bottom-left
                (3 * ima_sx // 4, 3 * ima_sy // 4, ima_sx, ima_sy),  # Bottom-right
            ]

        for region in fringe_regions:
            x1, y1, x2, y2 = region
            for y in range(y1, y2):
                for x in range(x1, x2):
                    stat_mask[y][x] = True

        return stat_mask

    def test_fringe_object_creation(self):
        """Test that Fringe object can be created."""
        fringe = hdrlfunc.Fringe()
        assert fringe is not None
        assert isinstance(fringe, hdrlfunc.Fringe)

    def test_fringe_compute_basic(self):
        """Test basic fringe computation without optional parameters."""
        # Create test data
        nima = 5
        ima_sx, ima_sy = 64, 64
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)
        collapse = hdrlfunc.Collapse.Mean()

        # Create fringe object and compute
        fringe = hdrlfunc.Fringe()
        fringe.compute(fringe_images, collapse_params=collapse)
        result = fringe

        # Check result structure
        assert hasattr(result, "master")
        assert hasattr(result, "contrib_map")
        assert hasattr(result, "qctable")

        # Check that master fringe has correct dimensions
        assert result.master.width == ima_sx
        assert result.master.height == ima_sy

        # Check that contrib_map has correct dimensions
        assert result.contrib_map.width == ima_sx
        assert result.contrib_map.height == ima_sy

        # Check that qctable exists
        assert result.qctable is not None

    def test_fringe_compute_with_object_masks(self):
        """Test fringe computation with object masks."""
        # Create test data
        nima = 3
        ima_sx, ima_sy = 64, 64
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)
        object_masks = self.create_object_masks(nima, ima_sx, ima_sy)
        collapse = hdrlfunc.Collapse.Median()

        # Create fringe object and compute
        fringe = hdrlfunc.Fringe()
        fringe.compute(fringe_images, object_masks, collapse_params=collapse)
        result = fringe

        # Check result structure
        assert hasattr(result, "master")
        assert hasattr(result, "contrib_map")
        assert hasattr(result, "qctable")

        # Check dimensions
        assert result.master.width == ima_sx
        assert result.master.height == ima_sy

    def test_fringe_compute_with_static_mask(self):
        """Test fringe computation with static mask."""
        # Create test data
        nima = 4
        ima_sx, ima_sy = 64, 64
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)
        stat_mask = self.create_static_mask(ima_sx, ima_sy)
        collapse = hdrlfunc.Collapse.Mean()

        # Create fringe object and compute
        fringe = hdrlfunc.Fringe()
        fringe.compute(fringe_images, stat_mask=stat_mask, collapse_params=collapse)
        result = fringe

        # Check result structure
        assert hasattr(result, "master")
        assert hasattr(result, "contrib_map")
        assert hasattr(result, "qctable")

        # Check dimensions
        assert result.master.width == ima_sx
        assert result.master.height == ima_sy

    def test_fringe_compute_with_all_parameters(self):
        """Test fringe computation with all optional parameters."""
        # Create test data
        nima = 3
        ima_sx, ima_sy = 64, 64
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)
        object_masks = self.create_object_masks(nima, ima_sx, ima_sy)
        stat_mask = self.create_static_mask(ima_sx, ima_sy)
        collapse = hdrlfunc.Collapse.Median()

        # Create fringe object and compute
        fringe = hdrlfunc.Fringe()
        fringe.compute(fringe_images, object_masks, stat_mask, collapse)
        result = fringe

        # Check result structure
        assert hasattr(result, "master")
        assert hasattr(result, "contrib_map")
        assert hasattr(result, "qctable")

        # Check dimensions
        assert result.master.width == ima_sx
        assert result.master.height == ima_sy

    def test_fringe_correct_basic(self):
        """Test basic fringe correction."""
        # Create test data
        nima = 3
        ima_sx, ima_sy = 64, 64
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)

        # Create master fringe (simplified)
        master_fringe_data = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe_errors = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe = hdrlcore.Image(master_fringe_data, master_fringe_errors)

        # Create fringe object and correct
        fringe = hdrlfunc.Fringe()
        result = fringe.correct(fringe_images, masterfringe=master_fringe)

        # Check result structure
        assert hasattr(result, "qctable")
        assert result.qctable is not None

    def test_fringe_correct_with_object_masks(self):
        """Test fringe correction with object masks."""
        # Create test data
        nima = 3
        ima_sx, ima_sy = 64, 64
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)
        object_masks = self.create_object_masks(nima, ima_sx, ima_sy)

        # Create master fringe
        master_fringe_data = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe_errors = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe = hdrlcore.Image(master_fringe_data, master_fringe_errors)

        # Create fringe object and correct
        fringe = hdrlfunc.Fringe()
        result = fringe.correct(fringe_images, object_masks, masterfringe=master_fringe)

        # Check result structure
        assert hasattr(result, "qctable")
        assert result.qctable is not None

    def test_fringe_correct_with_static_mask(self):
        """Test fringe correction with static mask."""
        # Create test data
        nima = 3
        ima_sx, ima_sy = 64, 64
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)
        stat_mask = self.create_static_mask(ima_sx, ima_sy)

        # Create master fringe
        master_fringe_data = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe_errors = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe = hdrlcore.Image(master_fringe_data, master_fringe_errors)

        # Create fringe object and correct
        fringe = hdrlfunc.Fringe()
        result = fringe.correct(fringe_images, stat_mask=stat_mask, masterfringe=master_fringe)

        # Check result structure
        assert hasattr(result, "qctable")
        assert result.qctable is not None

    def test_fringe_correct_with_all_parameters(self):
        """Test fringe correction with all optional parameters."""
        # Create test data
        nima = 3
        ima_sx, ima_sy = 64, 64
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)
        object_masks = self.create_object_masks(nima, ima_sx, ima_sy)
        stat_mask = self.create_static_mask(ima_sx, ima_sy)

        # Create master fringe
        master_fringe_data = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe_errors = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe = hdrlcore.Image(master_fringe_data, master_fringe_errors)

        # Create fringe object and correct
        fringe = hdrlfunc.Fringe()
        result = fringe.correct(fringe_images, object_masks, stat_mask, master_fringe)

        # Check result structure
        assert hasattr(result, "qctable")
        assert result.qctable is not None

    def test_fringe_compute_null_input(self):
        """Test that fringe compute raises NullInputError with null input."""
        fringe = hdrlfunc.Fringe()
        collapse = hdrlfunc.Collapse.Mean()

        with pytest.raises(hdrlcore.NullInputError):
            fringe.compute(None, None, None, collapse)

    def test_fringe_compute_empty_imagelist(self):
        """Test that fringe compute raises NullInputError with empty imagelist."""
        fringe = hdrlfunc.Fringe()
        collapse = hdrlfunc.Collapse.Mean()
        empty_list = hdrlcore.ImageList()

        with pytest.raises(hdrlcore.NullInputError):
            fringe.compute(empty_list, None, None, collapse)

    def test_fringe_compute_null_collapse(self):
        """Test that fringe compute raises TypeError with null collapse method."""
        # Create test data
        nima = 5
        ima_sx, ima_sy = 64, 64
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)
        # Create fringe object and compute
        fringe = hdrlfunc.Fringe()

        with pytest.raises(TypeError):
            fringe.compute(fringe_images)
        with pytest.raises(TypeError):
            fringe.compute(fringe_images, collapse_params=None)

    def test_fringe_correct_null_input(self):
        """Test that fringe correct raises NullInputError with null input."""
        fringe = hdrlfunc.Fringe()

        # Create a dummy master fringe
        ima_sx, ima_sy = 64, 64
        master_fringe_data = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe_errors = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe = hdrlcore.Image(master_fringe_data, master_fringe_errors)

        with pytest.raises(hdrlcore.NullInputError):
            fringe.correct(None, masterfringe=master_fringe)

    def test_fringe_correct_empty_imagelist(self):
        """Test that fringe correct raises NullInputError with empty imagelist."""
        fringe = hdrlfunc.Fringe()
        empty_list = hdrlcore.ImageList()

        # Create master fringe
        ima_sx, ima_sy = 64, 64
        master_fringe_data = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe_errors = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe = hdrlcore.Image(master_fringe_data, master_fringe_errors)

        with pytest.raises(hdrlcore.NullInputError):
            fringe.correct(empty_list, masterfringe=master_fringe)

    def test_fringe_correct_null_masterfringe(self):
        """Test that fringe correct raises Error with null master fringe pattern."""
        # Create test data
        nima = 3
        ima_sx, ima_sy = 64, 64
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)

        # Create master fringe (simplified)
        master_fringe_data = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe_errors = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe = hdrlcore.Image(master_fringe_data, master_fringe_errors)

        # Create fringe object and correct
        fringe = hdrlfunc.Fringe()

        with pytest.raises(hdrlcore.NullInputError):
            fringe.correct(fringe_images)
        with pytest.raises(hdrlcore.NullInputError):
            fringe.correct(fringe_images, masterfringe=None)

    def test_fringe_compute_incompatible_dimensions(self):
        """Test that fringe compute raises IncompatibleInputError with mismatched dimensions."""
        fringe = hdrlfunc.Fringe()
        collapse = hdrlfunc.Collapse.Mean()

        # Create fringe images
        fringe_images = self.create_fringe_images(3, 64, 64)

        # Create object masks with different dimensions
        object_masks = self.create_object_masks(3, 32, 32)  # Different size

        with pytest.raises(hdrlcore.IncompatibleInputError):
            fringe.compute(fringe_images, object_masks, None, collapse)

    def test_fringe_correct_incompatible_dimensions(self):
        """Test that fringe correct raises IncompatibleInputError with mismatched dimensions."""
        fringe = hdrlfunc.Fringe()

        # Create fringe images
        fringe_images = self.create_fringe_images(3, 64, 64)

        # Create master fringe with different dimensions
        master_fringe_data = cplcore.Image.zeros(32, 32, cplcore.Type.DOUBLE)
        master_fringe_errors = cplcore.Image.zeros(32, 32, cplcore.Type.DOUBLE)
        master_fringe = hdrlcore.Image(master_fringe_data, master_fringe_errors)

        with pytest.raises(hdrlcore.IncompatibleInputError):
            fringe.correct(fringe_images, masterfringe=master_fringe)

    def test_fringe_compute_different_collapse_methods(self):
        """Test fringe computation with different collapse methods."""
        # Create test data
        nima = 3
        ima_sx, ima_sy = 64, 64
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)

        # Test with different collapse methods
        collapse_methods = [
            hdrlfunc.Collapse.Mean(),
            hdrlfunc.Collapse.Median(),
            hdrlfunc.Collapse.WeightedMean(),
            hdrlfunc.Collapse.MinMax(0.1, 0.9),
            hdrlfunc.Collapse.Sigclip(2.0, 2.0, 3),
        ]

        fringe = hdrlfunc.Fringe()

        for collapse in collapse_methods:
            fringe.compute(fringe_images, None, None, collapse)
            result = fringe

            # Check result structure
            assert hasattr(result, "master")
            assert hasattr(result, "contrib_map")
            assert hasattr(result, "qctable")

            # Check dimensions
            assert result.master.width == ima_sx
            assert result.master.height == ima_sy

    def test_fringe_compute_large_images(self):
        """Test fringe computation with larger images."""
        # Create test data with larger images
        nima = 2
        ima_sx, ima_sy = 128, 128
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)
        collapse = hdrlfunc.Collapse.Mean()

        # Create fringe object and compute
        fringe = hdrlfunc.Fringe()
        fringe.compute(fringe_images, collapse_params=collapse)
        result = fringe

        # Check result structure
        assert hasattr(result, "master")
        assert hasattr(result, "contrib_map")
        assert hasattr(result, "qctable")

        # Check dimensions
        assert result.master.width == ima_sx
        assert result.master.height == ima_sy

    def test_fringe_correct_large_images(self):
        """Test fringe correction with larger images."""
        # Create test data with larger images
        nima = 2
        ima_sx, ima_sy = 128, 128
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)

        # Create master fringe
        master_fringe_data = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe_errors = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe = hdrlcore.Image(master_fringe_data, master_fringe_errors)

        # Create fringe object and correct
        fringe = hdrlfunc.Fringe()
        result = fringe.correct(fringe_images, masterfringe=master_fringe)

        # Check result structure
        assert hasattr(result, "qctable")
        assert result.qctable is not None

    def test_fringe_compute_rectangular_images(self):
        """Test fringe computation with rectangular images."""
        # Create test data with rectangular images
        nima = 3
        ima_sx, ima_sy = 64, 32  # Rectangular
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)
        collapse = hdrlfunc.Collapse.Mean()

        # Create fringe object and compute
        fringe = hdrlfunc.Fringe()
        fringe.compute(fringe_images, collapse_params=collapse)
        result = fringe

        # Check result structure
        assert hasattr(result, "master")
        assert hasattr(result, "contrib_map")
        assert hasattr(result, "qctable")

        # Check dimensions
        assert result.master.width == ima_sx
        assert result.master.height == ima_sy

    def test_fringe_correct_rectangular_images(self):
        """Test fringe correction with rectangular images."""
        # Create test data with rectangular images
        nima = 3
        ima_sx, ima_sy = 64, 32  # Rectangular
        fringe_images = self.create_fringe_images(nima, ima_sx, ima_sy)

        # Create master fringe
        master_fringe_data = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe_errors = cplcore.Image.zeros(ima_sx, ima_sy, cplcore.Type.DOUBLE)
        master_fringe = hdrlcore.Image(master_fringe_data, master_fringe_errors)

        # Create fringe object and correct
        fringe = hdrlfunc.Fringe()
        result = fringe.correct(fringe_images, masterfringe=master_fringe)

        # Check result structure
        assert hasattr(result, "qctable")
        assert result.qctable is not None
