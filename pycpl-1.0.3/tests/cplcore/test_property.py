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


class TestProperty:
    def test_constructor_char(self):
        prop = cplcore.Property("named_property", cplcore.Type.CHAR)
        assert prop.type == cplcore.Type.CHAR
        assert prop.name == "named_property"
        assert prop.comment is None

    def test_constructor_bool(self):
        prop = cplcore.Property("named_property", cplcore.Type.BOOL)
        assert prop.type == cplcore.Type.BOOL
        assert prop.name == "named_property"
        assert type(prop.value) == bool
        assert prop.comment is None

    def test_constructor_int(self):
        prop = cplcore.Property("named_property", cplcore.Type.INT)
        assert prop.type == cplcore.Type.INT
        assert prop.name == "named_property"
        assert prop.comment is None

    def test_constructor_long(self):
        prop = cplcore.Property("named_property", cplcore.Type.LONG)
        assert prop.type == cplcore.Type.LONG
        assert prop.name == "named_property"
        assert prop.comment is None

    def test_constructor_double(self):
        prop = cplcore.Property("named_property", cplcore.Type.DOUBLE)
        assert prop.type == cplcore.Type.DOUBLE
        assert prop.name == "named_property"
        assert prop.comment is None

    def test_constructor_double_complex(self):
        prop = cplcore.Property("named_property", cplcore.Type.DOUBLE_COMPLEX)
        assert prop.type == cplcore.Type.DOUBLE_COMPLEX
        assert prop.name == "named_property"
        assert prop.comment is None

    def test_constructor_string(self):
        prop = cplcore.Property("named_property", cplcore.Type.STRING)
        assert prop.type == cplcore.Type.STRING
        assert prop.name == "named_property"
        assert prop.comment is None

    def test_preset_int(self):
        prop = cplcore.Property("named_property", cplcore.Type.INT, -15)
        assert prop.type == cplcore.Type.INT
        assert prop.name == "named_property"
        assert prop.comment is None
        assert prop.value == -15

    def test_preset_string(self):
        prop = cplcore.Property(
            "named_property",
            cplcore.Type.STRING,
            r"\'\'\"n,z+C<sNIU{e;g3uS!\t+,dDNu70d]!{#\R\"d+(>.iy}@s02@kj!C3>m)\nmxi yZOK7",
        )
        assert prop.type == cplcore.Type.STRING
        assert prop.name == "named_property"
        assert prop.comment is None
        assert (
            prop.value
            == r"\'\'\"n,z+C<sNIU{e;g3uS!\t+,dDNu70d]!{#\R\"d+(>.iy}@s02@kj!C3>m)\nmxi yZOK7"
        )

    def test_preset_complex(self):
        prop = cplcore.Property(
            "named_property", cplcore.Type.DOUBLE_COMPLEX, 18920.1982 - 19823.123j
        )
        assert prop.type == cplcore.Type.DOUBLE_COMPLEX
        assert prop.name == "named_property"
        assert prop.comment is None
        assert prop.value == 18920.1982 - 19823.123j

    def test_inference_long(self):
        # Long is default for python ints
        prop = cplcore.Property("long", 1002)
        assert prop.type == cplcore.Type.LONG
        assert prop.name == "long"
        assert prop.value == 1002

    def test_inference_bool(self):
        prop = cplcore.Property("boolean", True)
        assert prop.type == cplcore.Type.BOOL
        assert prop.name == "boolean"
        assert prop.value
        assert type(prop.value) == bool

    def test_inference_double(self):
        # Double is default for python floats
        prop = cplcore.Property("double", 2059.22)
        assert prop.type == cplcore.Type.DOUBLE
        assert prop.name == "double"
        assert prop.value == 2059.22

    def test_inference_string(self):
        prop = cplcore.Property("string", "property_value")
        assert prop.type == cplcore.Type.STRING
        assert prop.name == "string"
        assert prop.value == "property_value"

    def test_inference_complex(self):
        # Double complex is the default for python complex numbers
        prop = cplcore.Property("complex", (2056.22 + 63.4j))
        assert prop.type == cplcore.Type.DOUBLE_COMPLEX
        assert prop.name == "complex"
        assert prop.value == (2056.22 + 63.4j)

    def test_commented_int(self):
        prop = cplcore.Property(
            "named_property", cplcore.Type.INT, -15, "another comment"
        )
        assert prop.type == cplcore.Type.INT
        assert prop.name == "named_property"
        assert prop.comment == "another comment"
        assert prop.value == -15

    def test_commented_string(self):
        prop = cplcore.Property(
            "named_property",
            cplcore.Type.STRING,
            r"\'\'\"n,z+C<sNIU{e;g3uS!\t+,dDNu70d]!{#\R\"d+(>.iy}@s02@kj!C3>m)\nmxi yZOK7",
            "another comment",
        )
        assert prop.type == cplcore.Type.STRING
        assert prop.name == "named_property"
        assert prop.comment == "another comment"
        assert (
            prop.value
            == r"\'\'\"n,z+C<sNIU{e;g3uS!\t+,dDNu70d]!{#\R\"d+(>.iy}@s02@kj!C3>m)\nmxi yZOK7"
        )

    def test_commented_complex(self):
        prop = cplcore.Property(
            "named_property",
            cplcore.Type.DOUBLE_COMPLEX,
            18920.1982 - 19823.123j,
            "another comment",
        )
        assert prop.type == cplcore.Type.DOUBLE_COMPLEX
        assert prop.name == "named_property"
        assert prop.comment == "another comment"
        assert prop.value == 18920.1982 - 19823.123j

    def test_constructor_unsupported_type(self):
        with pytest.raises(cplcore.InvalidTypeError):
            _ = cplcore.Property("named_property", cplcore.Type.UCHAR)

    def test_set_name(self):
        prop = cplcore.Property("named_property", cplcore.Type.LONG)
        assert prop.comment is None

        prop.name = "another_name"
        assert prop.type == cplcore.Type.LONG
        assert prop.name == "another_name"
        assert prop.comment is None

    def test_set_name_none(self):
        prop = cplcore.Property("named_property", cplcore.Type.LONG)
        assert prop.comment is None

        with pytest.raises(TypeError):
            prop.name = None

    def test_set_comment(self):
        prop = cplcore.Property("named_property", cplcore.Type.LONG)
        assert prop.comment is None

        prop.comment = "another_comment"
        assert prop.type == cplcore.Type.LONG
        assert prop.name == "named_property"
        assert prop.comment == "another_comment"

    def test_set_comment_none(self):
        prop = cplcore.Property("named_property", cplcore.Type.LONG)
        assert prop.comment is None

        with pytest.raises(TypeError):
            prop.name = None

    def test_set_int(self):
        prop = cplcore.Property("named_property", cplcore.Type.INT)
        assert prop.value == 0

        prop.value = -15
        assert prop.type == cplcore.Type.INT
        assert prop.name == "named_property"
        assert prop.comment is None
        assert prop.value == -15

    def test_set_string(self):
        prop = cplcore.Property("named_property", cplcore.Type.STRING)
        assert prop.value == ""

        prop.value = r"\'\'\"n,z+C<sNIU{e;g3uS!\t+,dDNu70d]!{#\R\"d+(>.iy}@s02@kj!C3>m)\nmxi yZOK7"
        assert prop.type == cplcore.Type.STRING
        assert prop.name == "named_property"
        assert prop.comment is None
        assert (
            prop.value
            == r"\'\'\"n,z+C<sNIU{e;g3uS!\t+,dDNu70d]!{#\R\"d+(>.iy}@s02@kj!C3>m)\nmxi yZOK7"
        )

    def test_set_complex(self):
        prop = cplcore.Property("named_property", cplcore.Type.DOUBLE_COMPLEX)
        assert prop.value == 0

        prop.value = 18920.1982 - 19823.123j
        assert prop.type == cplcore.Type.DOUBLE_COMPLEX
        assert prop.name == "named_property"
        assert prop.comment is None
        assert prop.value == 18920.1982 - 19823.123j

    def test_retype_string_as_pyint(self):
        prop = cplcore.Property("named_property", cplcore.Type.STRING)

        prop.value = 10248911
        assert prop.type == cplcore.Type.LONG
        assert prop.name == "named_property"
        assert prop.value == 10248911

    def test_retype_long_as_pystr(self):
        prop = cplcore.Property("named_property", cplcore.Type.LONG)

        prop.value = "I'm not LONG for this world anymore"
        assert prop.type == cplcore.Type.STRING
        assert prop.name == "named_property"
        assert prop.value == "I'm not LONG for this world anymore"

    def test_cast_double_as_pyint(self):
        prop = cplcore.Property("named_property", cplcore.Type.DOUBLE)

        prop.value = 10248911
        assert prop.type == cplcore.Type.DOUBLE
        assert prop.name == "named_property"
        assert prop.value == 10248911.0

    def test_cast_int_as_pyfloat(self):
        prop = cplcore.Property("named_property", cplcore.Type.INT)

        # Whilst this is a python float
        # Since float(52) == ctypes.int(float(52))
        # the cast should be allowed, without changing type.
        prop.value = float(pow(2, 20))
        assert prop.type == cplcore.Type.INT
        assert prop.name == "named_property"
        assert prop.value == pow(2, 20)

    def test_retype_int_as_pyfloat(self):
        prop = cplcore.Property("named_property", cplcore.Type.INT)

        # This float is too big to fit into an integer type
        prop.value = float(pow(2, 40))
        assert prop.type == cplcore.Type.DOUBLE
        assert prop.name == "named_property"
        assert prop.value == pow(2, 40)

    def test_retype_int_as_pyint(self):
        prop = cplcore.Property("named_property", cplcore.Type.INT)

        # This bigint is too big to fit into an integer type
        prop.value = pow(2, 40)
        assert prop.type == cplcore.Type.LONG
        assert prop.name == "named_property"
        assert prop.value == pow(2, 40)

    def test_set_none(self):
        prop = cplcore.Property("named_property", cplcore.Type.FLOAT)
        assert prop.value == float(0)

        with pytest.raises(ValueError):
            prop.value = None

        assert prop.type == cplcore.Type.FLOAT
        assert prop.value == 0

    def test_float_equal(self):
        prop1 = cplcore.Property("named_property", cplcore.Type.FLOAT)
        prop2 = cplcore.Property("named_property", cplcore.Type.FLOAT)
        prop1.value = 91023.25
        prop2.value = 91023.25

        assert prop1 == prop2

    def test_float_double_inequal(self):
        prop1 = cplcore.Property("named_property", cplcore.Type.FLOAT)
        prop2 = cplcore.Property("named_property", cplcore.Type.DOUBLE)
        prop1.value = 91023.25
        prop2.value = 91023.25

        assert prop1 != prop2

    def test_float_value_inequal(self):
        prop1 = cplcore.Property("named_property", cplcore.Type.FLOAT)
        prop2 = cplcore.Property("named_property", cplcore.Type.FLOAT)
        prop1.value = 91023.25
        prop2.value = 91023.50

        assert prop1 != prop2

    def test_float_name_inequal(self):
        prop1 = cplcore.Property("named_property1", cplcore.Type.FLOAT)
        prop2 = cplcore.Property("named_property2", cplcore.Type.FLOAT)
        prop1.value = 91023.25
        prop2.value = 91023.25

        assert prop1 != prop2

    def test_save(self):
        img1 = cplcore.Image.zeros(3, 3, cplcore.Type.INT)

        plist1 = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop1.value = 20
        plist1.append(prop1)

        img1[1][1] = 5

        img1.save("test_image.fits", plist1, cplcore.io.CREATE)

        loaded_mask = cplcore.Image.load(
            "test_image.fits", dtype=cplcore.Type.INT, extension=0
        )

        assert loaded_mask[1][1] == 5
        assert loaded_mask.shape == (3, 3)

        loaded_proplist = cplcore.PropertyList.load("test_image.fits", 0)
        assert loaded_proplist["NAMED_PROPERTY1"].value == 20

        img2 = cplcore.Image.zeros(2, 2, cplcore.Type.FLOAT)
        img2[0][0] = 2.5
        img2.save("test_image.fits", cplcore.PropertyList(), cplcore.io.EXTEND)

        loaded_image = cplcore.Image.load(
            "test_image.fits", cplcore.Type.UNSPECIFIED, extension=1
        )
        assert loaded_image[0][0] == 2.5
        assert loaded_image.shape == (2, 2)
        os.remove("test_image.fits")

    def test_repr(self):
        prop1 = cplcore.Property("named_property1", cplcore.Type.FLOAT)
        prop1.value = 91023.25
        expect = """Property('named_property1', <Type.FLOAT: 65536>, 91023.25, None)"""
        assert repr(prop1) == expect

    @pytest.mark.xfail(
        reason="cpl_property_dump currently private and therefore inaccessible by default. See PIPE-10999."
    )
    def test_dump_file(self, tmp_path):
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_property_dump.txt"
        prop1 = cplcore.Property("named_property1", cplcore.Type.FLOAT)
        prop1.value = 91023.25
        filename = tmp_path.joinpath(p)
        prop1.dump(filename=str(filename))
        expect = """Property at address 0x6000036cb800
	name   : 0x6000036cb870 'named_property1'
	comment: 0x0 '(null)'
	type   : 0x0010000
	size   : 1
	value  : 91023.2
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

    @pytest.mark.xfail(
        reason="cpl_property_dump currently private and therefore inaccessible by default. See PIPE-10999."
    )
    def test_dump_string(self):
        prop1 = cplcore.Property("named_property1", cplcore.Type.FLOAT)
        prop1.value = 91023.25
        expect = """Property at address 0x6000036cb800
	name   : 0x6000036cb870 'named_property1'
	comment: 0x0 '(null)'
	type   : 0x0010000
	size   : 1
	value  : 91023.2
"""  # noqa
        # remove memory address details
        expect = re.sub("0x[0-9a-z]+", "", expect)
        outp = re.sub("0x[0-9a-z]+", "", str(prop1))
        expect = re.sub("\\(n[a-z]+l\\)", "", expect)
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)

        assert outp == expect
        assert isinstance(prop1.dump(show=False), str)

    @pytest.mark.xfail(
        reason="cpl_property_dump currently private and therefore inaccessible by default. See PIPE-10999."
    )
    def test_dump_stdout(self):
        cmd = "from cpl import core as cplcore; "
        cmd += """prop1 = cplcore.Property("named_property1", cplcore.Type.FLOAT); """
        cmd += """prop1.value = 91023.25; """
        cmd += "prop1.dump()"
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        expect = """Property at address 0x6000036cb800
	name   : 0x6000036cb870 'named_property1'
	comment: 0x0 '(null)'
	type   : 0x0010000
	size   : 1
	value  : 91023.2
"""  # noqa
        expect = re.sub("0x[0-9a-z]+", "", expect)
        outp = re.sub("0x[0-9a-z]+", "", outp)
        expect = re.sub("\\(n[a-z]+l\\)", "", expect)
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)

        assert outp == expect
