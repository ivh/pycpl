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

import pytest
import subprocess
import re

from cpl import core as cplcore
from cpl import ui as cplui


class TestParameterList:
    def test_len(self):
        plist = cplui.ParameterList()
        p1 = cplui.ParameterValue(
            name="context.param1",
            context="context",
            description="test parameter 1",
            default=2.0,
        )
        p2 = cplui.ParameterValue(
            name="context.param2",
            context="context",
            description="test parameter 2",
            default=-5.0,
        )
        plist.append(p1)
        plist.append(p2)
        assert len(plist) == 2

    def test_get_index(self):
        plist = cplui.ParameterList()
        p1 = cplui.ParameterValue(
            name="context.param1",
            context="context",
            description="test parameter 1",
            default=2.0,
        )
        p2 = cplui.ParameterValue(
            name="context.param2",
            context="context",
            description="test parameter 2",
            default=-5.0,
        )
        plist.append(p1)
        plist.append(p2)

        pret = plist[0]
        assert pret.name == "context.param1"
        assert pret.context == "context"
        assert pret.description == "test parameter 1"
        assert pret.data_type == cplcore.Type.DOUBLE
        assert pret.default == 2.0
        assert pret.value == 2.0

        pret = plist[1]
        assert pret.name == "context.param2"
        assert pret.context == "context"
        assert pret.description == "test parameter 2"
        assert pret.data_type == cplcore.Type.DOUBLE
        assert pret.default == -5.0
        assert pret.value == -5.0

    def test_get_name(self):
        plist = cplui.ParameterList()
        p1 = cplui.ParameterValue(
            name="context.param1",
            context="context",
            description="test parameter 1",
            default=2.0,
        )
        p2 = cplui.ParameterValue(
            name="context.param2",
            context="context",
            description="test parameter 2",
            default=-5.0,
        )
        plist.append(p1)
        plist.append(p2)

        pret = plist["context.param1"]
        assert pret.name == "context.param1"
        assert pret.context == "context"
        assert pret.description == "test parameter 1"
        assert pret.data_type == cplcore.Type.DOUBLE
        assert pret.default == 2.0
        assert pret.value == 2.0

        pret = plist["context.param2"]
        assert pret.name == "context.param2"
        assert pret.context == "context"
        assert pret.description == "test parameter 2"
        assert pret.data_type == cplcore.Type.DOUBLE
        assert pret.default == -5.0
        assert pret.value == -5.0

    @pytest.mark.xfail(
        reason="Equality operator does not currently work. See PIPE-10998."
    )
    def test_equal(self):
        plist1 = cplui.ParameterList()
        p1 = cplui.ParameterValue(
            name="context.param1",
            context="context",
            description="test parameter 1",
            default=2.0,
        )
        p2 = cplui.ParameterValue(
            name="context.param2",
            context="context",
            description="test parameter 2",
            default=-5.0,
        )
        plist1.append(p1)
        plist1.append(p2)

        plist2 = cplui.ParameterList()
        p3 = cplui.ParameterValue(
            name="context.param1",
            context="context",
            description="test parameter 1",
            default=2.0,
        )
        p4 = cplui.ParameterValue(
            name="context.param2",
            context="context",
            description="test parameter 2",
            default=-5.0,
        )
        plist2.append(p3)
        plist2.append(p4)

        assert plist1 == plist2

    # This currently passes but perhaps for the wrong reason, as test_equal does not pass
    def test_not_equal(self):
        plist1 = cplui.ParameterList()
        p1 = cplui.ParameterValue(
            name="context.param1",
            context="context",
            description="test parameter 1",
            default=2.0,
        )
        p2 = cplui.ParameterValue(
            name="context.param2",
            context="context",
            description="test parameter 2",
            default=-5.0,
        )
        plist1.append(p1)
        plist1.append(p2)

        plist2 = cplui.ParameterList()
        p1 = cplui.ParameterValue(
            name="context.param1",
            context="context",
            description="test parameter 1",
            default=2.0,
        )
        p2 = cplui.ParameterValue(
            name="context.param2",
            context="context",
            description="test parameter 2",
            default=-5.1,
        )
        plist2.append(p1)
        plist2.append(p2)

        assert plist1 != plist2

    def test_repr(self):
        plist = cplui.ParameterList()
        p1 = cplui.ParameterValue(
            name="context.param1",
            context="context",
            description="test parameter 1",
            default=2.0,
        )
        p2 = cplui.ParameterValue(
            name="context.param2",
            context="context",
            description="test parameter 2",
            default=-5.0,
        )
        plist.append(p1)
        plist.append(p2)
        expect = "<cpl.ui.ParameterList, 2 Parameters>"
        assert repr(plist) == expect

    def test_dump_file(self, tmp_path):
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_parameterlist_dump.txt"
        plist = cplui.ParameterList()
        p1 = cplui.ParameterValue(
            name="context.param1",
            context="context",
            description="test parameter 1",
            default=2.0,
        )
        p2 = cplui.ParameterValue(
            name="context.param2",
            context="context",
            description="test parameter 2",
            default=-5.0,
        )
        plist.append(p1)
        plist.append(p2)
        filename = tmp_path.joinpath(p)
        plist.dump(filename=str(filename))
        expect = """Parameter at 0x600003ad0af0: class 02 (Value), type 20000 (double)
  Attributes:    name (at 0x6000013f8690): context.param1    description (at 0x6000011ec0a0): test parameter 1    context (at 0x6000013f86a0): context    user tag (at 0x0): 0x0
  Values:        Current: 2    Default: 2
Parameter at 0x600003ad0a80: class 02 (Value), type 20000 (double)
  Attributes:    name (at 0x6000013f86f0): context.param2    description (at 0x6000011ec0e0): test parameter 2    context (at 0x6000013f8700): context    user tag (at 0x0): 0x0
  Values:        Current: -5    Default: -5
"""
        contents = ""
        with open(str(filename), "r") as f:
            for line in f.readlines():
                contents += line

        # remove memory address details
        expect = re.sub("0x[0-9a-z]+", "", expect)
        contents = re.sub("0x[0-9a-z]+", "", contents)
        expect = re.sub("\\(n[a-z]+l\\)", "", expect)
        contents = re.sub("\\(n[a-z]+l\\)", "", contents)
        assert contents == expect

    def test_dump_string(self):
        plist = cplui.ParameterList()
        p1 = cplui.ParameterValue(
            name="context.param1",
            context="context",
            description="test parameter 1",
            default=2.0,
        )
        p2 = cplui.ParameterValue(
            name="context.param2",
            context="context",
            description="test parameter 2",
            default=-5.0,
        )
        plist.append(p1)
        plist.append(p2)
        expect = """Parameter at 0x600003ad0af0: class 02 (Value), type 20000 (double)
  Attributes:    name (at 0x6000013f8690): context.param1    description (at 0x6000011ec0a0): test parameter 1    context (at 0x6000013f86a0): context    user tag (at 0x0): 0x0
  Values:        Current: 2    Default: 2
Parameter at 0x600003ad0a80: class 02 (Value), type 20000 (double)
  Attributes:    name (at 0x6000013f86f0): context.param2    description (at 0x6000011ec0e0): test parameter 2    context (at 0x6000013f8700): context    user tag (at 0x0): 0x0
  Values:        Current: -5    Default: -5
"""
        # remove memory address details
        expect = re.sub("0x[0-9a-z]+", "", expect)
        outp = re.sub("0x[0-9a-z]+", "", str(plist))
        expect = re.sub("\\(n[a-z]+l\\)", "", expect)
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)
        assert outp == expect
        assert isinstance(plist.dump(show=False), str)

    def test_dump_stdout(self):
        cmd = "from cpl import ui as cplui; "
        cmd += "plist = cplui.ParameterList(); "
        cmd += """p1 = cplui.ParameterValue(name="context.param1", context="context", description="test parameter 1", default=2.0); """
        cmd += """p2 = cplui.ParameterValue(name="context.param2", context="context", description="test parameter 2", default=-5.0); """
        cmd += "plist.append(p1); "
        cmd += "plist.append(p2); "
        cmd += "plist.dump()"
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        expect = """Parameter at 0x600003ad0af0: class 02 (Value), type 20000 (double)
  Attributes:    name (at 0x6000013f8690): context.param1    description (at 0x6000011ec0a0): test parameter 1    context (at 0x6000013f86a0): context    user tag (at 0x0): 0x0
  Values:        Current: 2    Default: 2
Parameter at 0x600003ad0a80: class 02 (Value), type 20000 (double)
  Attributes:    name (at 0x6000013f86f0): context.param2    description (at 0x6000011ec0e0): test parameter 2    context (at 0x6000013f8700): context    user tag (at 0x0): 0x0
  Values:        Current: -5    Default: -5
"""
        expect = re.sub("0x[0-9a-z]+", "", expect)
        outp = re.sub("0x[0-9a-z]+", "", outp)
        expect = re.sub("\\(n[a-z]+l\\)", "", expect)
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)

        assert outp == expect
