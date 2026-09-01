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


class TestImage:
    @pytest.mark.xfail(reason="XFAIL if PyCPL does not have cpl_error_reset after cpl_init.")
    def test_constructor(self):
        himg = hdrlcore.Image.zeros(2, 3)
        assert himg.width == 2

        px = cplcore.Image.zeros(3, 4, cplcore.Type.DOUBLE)
        py = cplcore.Image.zeros(3, 4, cplcore.Type.DOUBLE)
        himg2 = hdrlcore.Image(px, py)
        assert himg2.width == 3
