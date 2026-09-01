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


from hdrl import func as hdrlfunc


class TestCollapse:
    def test_inputs(self):
        # tests adapted from hdrl_collapse-test.c
        hdrlfunc.Collapse.Mean()
        hdrlfunc.Collapse.Median()
        hdrlfunc.Collapse.WeightedMean()
        hdrlfunc.Collapse.Sigclip(3.0, 3.0, 5)
        hdrlfunc.Collapse.MinMax(2.0, 2.0)
        hdrlfunc.Collapse.Mode(100.0, 200.0, 1.0, hdrlfunc.Collapse.Method.Fit, 100)

    # def test_static(self):
    # test usage of collapse functions as static functions
    # ==> See hdrlcore/test_imagelist.py
