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
from cpl import core as cplcore
from hdrl import core as hdrlcore
from hdrl import func as hdrlfunc


class TestBias:
    def test_collapse_sigclip(self):
        img1 = cplcore.Image([[1, 2, 3]])
        img2 = cplcore.Image([[2, 3, 4]])
        img3 = cplcore.Image([[5, 6, 7]])
        imlist1 = cplcore.ImageList([img1, img2, img3])

        img4 = cplcore.Image([[10, 20, 30]])
        img5 = cplcore.Image([[20, 30, 40]])
        img6 = cplcore.Image([[50, 60, 70]])
        imlist2 = cplcore.ImageList([img4, img5, img6])

        himlist = hdrlcore.ImageList(imlist1, imlist2)
        collapse = hdrlfunc.Collapse.Sigclip(3.0, 3.0, 5)
        results = himlist.collapse(collapse)
        hdrl_img_out = results.out

        # params = hdrlfunc.Collapse.sigclip_parameter_create(3.0,3.0,5)
        # (hdrl_img_out, cpl_img_contrib) = himlist.collapse(params)
        assert hdrl_img_out.error[0][0] == pytest.approx(18.2574, abs=1e-4)
        assert hdrl_img_out.error[0][1] == pytest.approx(23.3333, abs=1e-4)
        assert hdrl_img_out.error[0][2] == pytest.approx(28.6744, abs=1e-4)

        assert hdrl_img_out.image[0][0] == pytest.approx(2.66667, abs=1e-4)
        assert hdrl_img_out.image[0][1] == pytest.approx(3.66667, abs=1e-4)
        assert hdrl_img_out.image[0][2] == pytest.approx(4.66667, abs=1e-4)
