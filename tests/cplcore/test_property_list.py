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

import os

import pytest
import subprocess
import re

from cpl import core as cplcore


class TestPropertyList:
    def test_constructor(self):
        plist = cplcore.PropertyList()

        assert len(plist) == 0

    def test_empty_plist(self):
        plist = cplcore.PropertyList()

        assert "anything" not in plist
        assert "" not in plist

        with pytest.raises(IndexError):
            plist[0]

        with pytest.raises(KeyError):
            plist["anything"]

        with pytest.raises(KeyError):
            plist[""]

    def test_append_one(self):
        plist = cplcore.PropertyList()
        prop = cplcore.Property("named_property", cplcore.Type.INT)
        plist.append(prop)

        assert len(plist) == 1
        assert plist[0] == prop
        assert plist[0].name == "named_property"
        assert plist[0].value == 0
        assert plist[0].type == cplcore.Type.INT

        assert plist["named_property"] == prop
        assert plist["named_property"].name == "named_property"
        assert plist["named_property"].value == 0
        assert plist["named_property"].type == cplcore.Type.INT

        assert "named_property" in plist
        assert prop in plist

    def test_append_two(self):
        plist = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.STRING)

        plist.append(prop1)
        plist.append(prop2)

        assert len(plist) == 2

        assert plist[0] == prop1
        assert plist[1] == prop2

        assert "named_property1" in plist
        assert "named_property2" in plist

        assert prop1 in plist
        assert prop2 in plist

    def test_insert_position(self):
        plist = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.STRING)
        prop3 = cplcore.Property("named_property3", cplcore.Type.DOUBLE_COMPLEX)

        plist.append(prop1)  # [prop1]
        plist.insert(0, prop2)  # [prop2, prop1]
        plist.insert(2, prop3)  # [prop2, prop1, prop3]

        assert len(plist) == 3
        assert plist[0] == prop2
        assert plist[1] == prop1
        assert plist[2] == prop3

    def test_insert_position_notfound(self):
        plist = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.STRING)

        with pytest.raises(IndexError):
            plist.insert(-1, prop1)

        with pytest.raises(IndexError):
            plist.insert(1, prop1)

        with pytest.raises(IndexError):
            plist.insert(-1, prop2)

        with pytest.raises(IndexError):
            plist.insert(1, prop2)

    def test_insert_name(self):
        plist = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.STRING)
        prop3 = cplcore.Property("named_property3", cplcore.Type.DOUBLE_COMPLEX)

        plist.append(prop1)  # [prop1]
        plist.insert("named_property1", prop2)  # [prop2, prop1]
        plist.insert("named_property1", prop3)  # [prop2, prop3, prop1]

        assert len(plist) == 3
        assert plist[0] == prop2
        assert plist[1] == prop3
        assert plist[2] == prop1

    def test_insert_name_notfound(self):
        plist = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.STRING)

        with pytest.raises(KeyError):
            plist.insert("non_existant", prop1)

        with pytest.raises(KeyError):
            plist.insert("non_existant", prop2)

    def test_set_position(self):
        plist = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.STRING)
        prop3 = cplcore.Property("named_property3", cplcore.Type.DOUBLE_COMPLEX)

        plist.append(prop1)
        plist.append(prop2)

        plist[0] = prop3

        assert len(plist) == 2
        assert plist[0] == prop3
        assert plist[1] == prop2

    def test_set_name(self):
        plist = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.STRING)
        prop3 = cplcore.Property("named_property3", cplcore.Type.DOUBLE_COMPLEX)

        plist.append(prop1)
        plist.append(prop2)

        plist[0] = prop3

        assert "named_property1" not in plist
        assert "named_property2" in plist
        assert "named_property3" in plist

        assert len(plist) == 2
        assert plist[0] == prop3
        assert plist[1] == prop2

    def test_delete_position(self):
        plist = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.STRING)
        prop3 = cplcore.Property("named_property3", cplcore.Type.DOUBLE_COMPLEX)

        plist.append(prop1)
        plist.append(prop2)
        plist.append(prop3)

        del plist[1]

        assert len(plist) == 2
        assert plist[0] == prop1
        assert plist[1] == prop3

    def test_delete_name(self):
        plist = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.STRING)
        prop3 = cplcore.Property("named_property3", cplcore.Type.DOUBLE_COMPLEX)

        plist.append(prop1)
        plist.append(prop2)
        plist.append(prop3)

        del plist["named_property2"]

        assert len(plist) == 2
        assert plist[0] == prop1
        assert plist[1] == prop3

    def test_delete_position_notfound(self):
        plist = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)

        plist.append(prop1)

        with pytest.raises(IndexError):
            del plist[1]

    def test_delete_name_notfound(self):
        plist = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)

        plist.append(prop1)

        with pytest.raises(KeyError):
            del plist["named_property2"]

    def test_delete_empty(self):
        plist = cplcore.PropertyList()

        with pytest.raises(IndexError):
            del plist[0]

    def test_delete_regexp(self):
        plist = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.STRING)
        prop3 = cplcore.Property("named_property3", cplcore.Type.DOUBLE_COMPLEX)

        plist.append(prop1)
        plist.append(prop2)
        plist.append(prop3)

        plist.del_regexp("z", False)
        assert len(plist) == 3

        plist.del_regexp("named_property[13]", False)
        assert len(plist) == 1

    def test_sort(self):
        plist = cplcore.PropertyList()

        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.LONG)
        prop3 = cplcore.Property("named_property3", cplcore.Type.LONG_LONG)
        prop4 = cplcore.Property("named_property4", cplcore.Type.FLOAT)
        prop5 = cplcore.Property("named_property5", cplcore.Type.DOUBLE)

        prop1.value = 190109
        prop2.value = 2908349090
        prop3.value = -2908349090
        prop4.value = 190109.625
        prop5.value = 190109.190823

        plist.append(prop1)
        plist.append(prop2)
        plist.append(prop3)
        plist.append(prop4)
        plist.append(prop5)

        def compare_func(prop1, prop2):
            from math import inf

            # There shouldn't have been any type casting
            assert prop1.type != prop2.type

            # By multiplying by inf, it is always at exactly
            # 1, or -1
            # 0 * math.inf, however, gives nan, so doesn't work.
            if prop1.value == prop2.value:
                return 0
            else:
                return max(0, min(1, inf * (prop2.value - prop1.value)))

        plist.sort(compare_func)

        assert plist[0].value == -2908349090
        assert plist[1].value == 190109
        assert plist[2].value == 190109.190823
        assert plist[3].value == 190109.625
        assert plist[4].value == 2908349090

    def test_save(self):
        plist1 = cplcore.PropertyList()

        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.LONG)
        prop1.value = 20
        prop2.value = -50
        plist1.append(prop1)
        plist1.append(prop2)
        plist1.save("test_property.fits", cplcore.io.CREATE)

        loaded = cplcore.PropertyList.load("test_property.fits", 0)

        assert loaded["NAMED_PROPERTY1"].value == 20
        assert loaded["NAMED_PROPERTY2"].value == -50

        plist2 = cplcore.PropertyList()
        prop3 = cplcore.Property("named_property3", cplcore.Type.FLOAT)
        prop3.value = 2.5
        plist2.append(prop3)

        plist2.save("test_property.fits", cplcore.io.EXTEND)
        loaded = cplcore.PropertyList.load("test_property.fits", 1)
        assert loaded["NAMED_PROPERTY3"].value == 2.5
        os.remove("test_property.fits")

    def test_append_value(self):
        plist1 = cplcore.PropertyList()
        plist1.append("int_property", 20)
        plist1.append("float_property", -50.5)
        plist1.append("string_property", "value")
        plist1.append("complex_property", 1 + 2j)
        assert plist1["int_property"].value == 20
        assert plist1["int_property"].type == cplcore.Type.LONG
        assert plist1["float_property"].value == -50.5
        assert plist1["float_property"].type == cplcore.Type.DOUBLE
        assert plist1["string_property"].value == "value"
        assert plist1["string_property"].type == cplcore.Type.STRING
        assert plist1["complex_property"].value == 1 + 2j
        assert plist1["complex_property"].type == cplcore.Type.DOUBLE_COMPLEX

    def test_append_proplist(self):
        plist1 = cplcore.PropertyList()
        plist1.append("int_property", 20)
        plist1.append("float_property", -50.5)

        plist2 = cplcore.PropertyList()
        plist2.append("string_property", "value")
        plist2.append("complex_property", 1 + 2j)
        plist1.append(plist2)
        assert plist1["int_property"].value == 20
        assert plist1["int_property"].type == cplcore.Type.LONG
        assert plist1["float_property"].value == -50.5
        assert plist1["float_property"].type == cplcore.Type.DOUBLE
        assert plist1["string_property"].value == "value"
        assert plist1["string_property"].type == cplcore.Type.STRING
        assert plist1["complex_property"].value == 1 + 2j
        assert plist1["complex_property"].type == cplcore.Type.DOUBLE_COMPLEX

    def test_repr(self):
        expect = """PropertyList([Property('named_property1', <Type.INT: 1024>, 20, None), Property('named_property2', <Type.LONG: 4096>, -50, None)])"""
        plist1 = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.LONG)
        prop1.value = 20
        prop2.value = -50
        plist1.append(prop1)
        plist1.append(prop2)
        assert repr(plist1) == expect

    def test_dump_file(self, tmp_path):
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_propertylist_dump.txt"
        plist1 = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.LONG)
        prop1.value = 20
        prop2.value = -50
        plist1.append(prop1)
        plist1.append(prop2)
        filename = tmp_path.joinpath(p)
        plist1.dump(filename=str(filename))
        expect = """Property list at address 0x600000ca1aa0:
Property at address 0x60000208f600
	name   : 0x60000208f670 'named_property1'
	comment: 0x0 '(null)'
	type   : 0x0000400 'int'
	size   : 1
	value  : 20
Property at address 0x60000208fe00
	name   : 0x60000208fe70 'named_property2'
	comment: 0x0 '(null)'
	type   : 0x0001000 'long int'
	size   : 1
	value  : -50
"""  # noqa
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
        plist1 = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.LONG)
        prop1.value = 20
        prop2.value = -50
        plist1.append(prop1)
        plist1.append(prop2)
        expect = """Property list at address 0x600000ca1aa0:
Property at address 0x60000208f600
	name   : 0x60000208f670 'named_property1'
	comment: 0x0 '(null)'
	type   : 0x0000400 'int'
	size   : 1
	value  : 20
Property at address 0x60000208fe00
	name   : 0x60000208fe70 'named_property2'
	comment: 0x0 '(null)'
	type   : 0x0001000 'long int'
	size   : 1
	value  : -50
"""  # noqa
        # remove memory address details
        expect = re.sub("0x[0-9a-z]+", "", expect)
        outp = re.sub("0x[0-9a-z]+", "", str(plist1))
        expect = re.sub("\\(n[a-z]+l\\)", "", expect)
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)

        assert outp == expect
        assert isinstance(plist1.dump(show=False), str)

    def test_dump_stdout(self):
        cmd = "from cpl import core as cplcore; "
        cmd += "plist1 = cplcore.PropertyList(); "
        cmd += """prop1 = cplcore.Property("named_property1", cplcore.Type.INT); """
        cmd += """prop2 = cplcore.Property("named_property2", cplcore.Type.LONG); """
        cmd += "prop1.value = 20; "
        cmd += "prop2.value = -50; "
        cmd += "plist1.append(prop1); "
        cmd += "plist1.append(prop2); "
        cmd += "plist1.dump()"
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        expect = """Property list at address 0x600000ca1aa0:
Property at address 0x60000208f600
	name   : 0x60000208f670 'named_property1'
	comment: 0x0 '(null)'
	type   : 0x0000400 'int'
	size   : 1
	value  : 20
Property at address 0x60000208fe00
	name   : 0x60000208fe70 'named_property2'
	comment: 0x0 '(null)'
	type   : 0x0001000 'long int'
	size   : 1
	value  : -50
"""  # noqa
        expect = re.sub("0x[0-9a-z]+", "", expect)
        outp = re.sub("0x[0-9a-z]+", "", outp)
        expect = re.sub("\\(n[a-z]+l\\)", "", expect)
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)

        assert outp == expect
