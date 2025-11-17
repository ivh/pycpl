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
import re
import subprocess

from cpl import core as cplcore
from cpl import ui as cplui


class TestParameter:
    def test_contructor_int_sets_properties(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter = cplui.ParameterValue(
            name=name, description=description, context=context, default=5
        )

        assert new_parameter.name == name
        assert new_parameter.description == description
        assert new_parameter.context == context

        assert new_parameter.data_type == cplcore.Type.INT
        assert new_parameter.default == 5
        assert new_parameter.value == 5

    def test_contructor_double_sets_properties(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter = cplui.ParameterValue(
            name=name, description=description, context=context, default=5.5
        )

        assert new_parameter.name == name
        assert new_parameter.description == description
        assert new_parameter.context == context

        assert new_parameter.data_type == cplcore.Type.DOUBLE
        assert new_parameter.default == 5.5
        assert new_parameter.value == 5.5

    def test_contructor_bool_sets_properties(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter = cplui.ParameterValue(
            name=name, description=description, context=context, default=True
        )

        assert new_parameter.name == name
        assert new_parameter.description == description
        assert new_parameter.context == context

        assert new_parameter.data_type == cplcore.Type.BOOL
        assert type(new_parameter.default) == bool
        assert type(new_parameter.value) == bool
        assert new_parameter.default
        assert new_parameter.value

    def test_contructor_string_sets_properties(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter = cplui.ParameterValue(
            name=name, description=description, context=context, default="Hello World"
        )

        assert new_parameter.name == name
        assert new_parameter.description == description
        assert new_parameter.context == context

        assert new_parameter.data_type == cplcore.Type.STRING
        assert new_parameter.default == "Hello World"
        assert new_parameter.value == "Hello World"

    def test_int_same_param_new_value_equal(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cplui.ParameterValue(
            name=name, description=description, context=context, default=5
        )
        new_parameter2 = cplui.ParameterValue(
            name=name, description=description, context=context, default=5
        )

        assert new_parameter1 == new_parameter2

        new_parameter1.value = new_parameter2.value = -pow(2, 27) - 1

        assert new_parameter1 == new_parameter2

    def test_double_same_param_new_value_equal(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cplui.ParameterValue(
            name=name, description=description, context=context, default=5.5
        )
        new_parameter2 = cplui.ParameterValue(
            name=name, description=description, context=context, default=5.5
        )

        assert new_parameter1 == new_parameter2

        new_parameter1.value = new_parameter2.value = 0.10982048

        assert new_parameter1 == new_parameter2

    def test_bool_same_param_new_value_equal(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cplui.ParameterValue(
            name=name, description=description, context=context, default=True
        )
        new_parameter2 = cplui.ParameterValue(
            name=name, description=description, context=context, default=True
        )

        assert new_parameter1 == new_parameter2

        new_parameter1.value = new_parameter2.value = False

        assert new_parameter1 == new_parameter2

    def test_string_same_param_new_value_equal(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cplui.ParameterValue(
            name=name, description=description, context=context, default="Hello World"
        )
        new_parameter2 = cplui.ParameterValue(
            name=name, description=description, context=context, default="Hello World"
        )

        assert new_parameter1 == new_parameter2

        new_parameter1.value = new_parameter2.value = "This is a test"

        assert new_parameter1 == new_parameter2

    def test_int_value_and_default_unlinked(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cplui.ParameterValue(
            name=name, description=description, context=context, default=5
        )

        new_parameter1.value = -pow(2, 27) - 1
        assert new_parameter1.default == 5
        assert new_parameter1.value == -pow(2, 27) - 1

    def test_double_value_and_default_unlinked(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cplui.ParameterValue(
            name=name, description=description, context=context, default=5.5
        )

        new_parameter1.value = 0.10982048
        assert new_parameter1.default == 5.5
        assert new_parameter1.value == 0.10982048

    def test_bool_value_and_default_unlinked(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cplui.ParameterValue(
            name=name, description=description, context=context, default=True
        )

        new_parameter1.value = False
        assert new_parameter1.default
        assert not new_parameter1.value

    def test_string_value_and_default_unlinked(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter1 = cplui.ParameterValue(
            name=name, description=description, context=context, default="Hello World"
        )

        new_parameter1.value = ""
        assert new_parameter1.default == "Hello World"
        assert new_parameter1.value == ""

    def test_type_mismatch_int(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter = cplui.ParameterValue(
            name=name, description=description, context=context, default=5
        )

        # Unfortunatley the behaviour as of now throws a Runtime Error: Unexpected index
        # which is a bad message
        with pytest.raises(RuntimeError):
            new_parameter.value = "Hello"
        assert new_parameter.value == 5

        with pytest.raises(RuntimeError):
            new_parameter.value = True
        assert new_parameter.value == 5

        with pytest.raises(RuntimeError):
            new_parameter.value = False
        assert new_parameter.value == 5

        with pytest.warns(RuntimeWarning):
            new_parameter.value = 5.5

    def test_type_mismatch_double(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter = cplui.ParameterValue(
            name=name, description=description, context=context, default=5.5
        )

        # Unfortunatley the behaviour as of now throws a Runtime Error: Unexpected index
        # which is a bad message
        with pytest.raises(RuntimeError):
            new_parameter.value = "Hello"
        assert new_parameter.value == 5.5

        with pytest.raises(RuntimeError):
            new_parameter.value = True
        assert new_parameter.value == 5.5

        with pytest.raises(RuntimeError):
            new_parameter.value = False
        assert new_parameter.value == 5.5

        # This should NOT throw
        new_parameter.value = 5
        new_parameter.value = 1
        new_parameter.value = 0

    def test_type_mismatch_bool(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter = cplui.ParameterValue(
            name=name, description=description, context=context, default=True
        )

        # Unfortunatley the behaviour as of now throws a Runtime Error: Unexpected index
        # which is a bad message
        with pytest.raises(RuntimeError):
            new_parameter.value = "Hello"
        assert new_parameter.value

        with pytest.raises(RuntimeError):
            new_parameter.value = 1
        assert new_parameter.value

        with pytest.raises(RuntimeError):
            new_parameter.value = 0
        assert new_parameter.value

        with pytest.raises(RuntimeError):
            new_parameter.value = 5.5
        assert new_parameter.value

    def test_type_mismatch_string(self):
        name = "Normally this might say unittest.param1, a dot-separated string"
        description = """This is the description to a unit test Parameter"""
        context = "This is usually the name except the last segment"

        new_parameter = cplui.ParameterValue(
            name=name, description=description, context=context, default="Hello World!"
        )

        # Unfortunatley the behaviour as of now throws a Runtime Error: Unexpected index
        # which is a bad message
        with pytest.raises(RuntimeError):
            new_parameter.value = True
        assert new_parameter.value == "Hello World!"

        with pytest.raises(RuntimeError):
            new_parameter.value = False
        assert new_parameter.value == "Hello World!"

        with pytest.raises(RuntimeError):
            new_parameter.value = 1
        assert new_parameter.value == "Hello World!"

        with pytest.raises(RuntimeError):
            new_parameter.value = 0
        assert new_parameter.value == "Hello World!"

        with pytest.raises(RuntimeError):
            new_parameter.value = 5.5


class TestParameterEnum:
    def test_constructor_int_enum_sets_properties(self):
        name = ""
        description = ""
        context = ""

        new_parameter = cplui.ParameterEnum(
            name=name,
            description=description,
            context=context,
            default=5,
            alternatives=[1, 2, 3, 4, 5],
        )

        assert new_parameter.name == name
        assert new_parameter.description == description
        assert new_parameter.context == context

        assert new_parameter.data_type == cplcore.Type.INT
        assert new_parameter.default == 5
        assert new_parameter.alternatives == [1, 2, 3, 4, 5]

    def test_constructor_double_enum_sets_properties(self):
        name = ""
        description = ""
        context = ""

        new_parameter = cplui.ParameterEnum(
            name=name,
            description=description,
            context=context,
            default=0.1598211111,
            alternatives=[0.15982, 0.1598211111, 3.51231321, 0.15983],
        )

        assert new_parameter.name == name
        assert new_parameter.description == description
        assert new_parameter.context == context

        assert new_parameter.data_type == cplcore.Type.DOUBLE
        assert new_parameter.default == 0.1598211111
        assert new_parameter.alternatives == [
            0.15982,
            0.1598211111,
            3.51231321,
            0.15983,
        ]

    def test_constructor_string_enum_sets_properties(self):
        name = ""
        description = ""
        context = ""

        new_parameter = cplui.ParameterEnum(
            name=name,
            description=description,
            context=context,
            default="cpl",
            alternatives=["eso", "cpl", "aao", "otherStuff"],
        )

        assert new_parameter.name == name
        assert new_parameter.description == description
        assert new_parameter.context == context

        assert new_parameter.data_type == cplcore.Type.STRING
        assert new_parameter.default == "cpl"
        assert new_parameter.alternatives == ["eso", "cpl", "aao", "otherStuff"]

    def test_constructor_int_not_found_except(self):
        name = ""
        description = ""
        context = ""

        with pytest.raises(cplcore.IllegalInputError):
            _ = cplui.ParameterEnum(
                name=name,
                description=description,
                context=context,
                default=6,
                alternatives=[1, 2, 3, 4, 5],
            )

    def test_constructor_double_not_found_except(self):
        name = ""
        description = ""
        context = ""

        # with pytest.raises(Exception):
        with pytest.raises(cplcore.IllegalInputError):
            _ = cplui.ParameterEnum(
                name=name,
                description=description,
                context=context,
                default=50.2,
                alternatives=[1.5555, 2.142124, 3.142214, 404.12, 51],
            )

    def test_constructor_string_not_found_except(self):
        name = ""
        description = ""
        context = ""
        with pytest.raises(cplcore.IllegalInputError):
            _ = cplui.ParameterEnum(
                name=name,
                description=description,
                context=context,
                default="PyRex",
                alternatives=["eso", "cpl", "aao", "otherStuff"],
            )

    def test_constructor_int_set_out_of_range_no_except(self):
        # Since cpl doesn't raise any exceptions for setting values outside the enum, neither should PyCPL
        name = ""
        description = ""
        context = ""

        # with pytest.raises(Exception):
        new_parameter = cplui.ParameterEnum(
            name=name,
            description=description,
            context=context,
            default=5,
            alternatives=[1, 2, 3, 4, 5],
        )
        new_parameter.value = 299910128

    def test_constructor_double_set_out_of_range_no_except(self):
        # Since cpl doesn't raise any exceptions for setting values outside the enum, neither should PyCPL
        name = ""
        description = ""
        context = ""

        # with pytest.raises(Exception):
        new_parameter = cplui.ParameterEnum(
            name=name,
            description=description,
            context=context,
            default=0.1598211111,
            alternatives=[0.15982, 0.1598211111, 3.51231321, 0.15983],
        )
        new_parameter.value = 4.19203

    def test_constructor_string_set_out_of_range_no_except(self):
        # Since cpl doesn't raise any exceptions for setting values outside the enum, neither should PyCPL
        name = ""
        description = ""
        context = ""

        # with pytest.raises(Exception):
        new_parameter = cplui.ParameterEnum(
            name=name,
            description=description,
            context=context,
            default="eso",
            alternatives=["eso", "cpl", "aao", "otherStuff"],
        )
        new_parameter.value = "PyEsoRex"

    def test_int_enum_same_param_new_value_equal(self):
        new_parameter1 = cplui.ParameterEnum(
            "name", "description", "context", 297810128, [-12389, 297810128]
        )
        new_parameter2 = cplui.ParameterEnum(
            "name", "description", "context", -12389, [-12389, 297810128]
        )

        assert new_parameter1 != new_parameter2
        new_parameter2.value = 297810128
        assert new_parameter1 == new_parameter2

        # Outside of enum
        # But we're only testing if it stays equal
        new_parameter1.value = new_parameter2.value = 677810128

        assert new_parameter1 == new_parameter2
        new_parameter1.value = new_parameter2.value = -677810128
        assert new_parameter1 == new_parameter2

    def test_double_enum_same_param_new_value_equal(self):
        new_parameter1 = cplui.ParameterEnum(
            "name",
            "description",
            "context",
            default="cpl",
            alternatives=["eso", "cpl", "aao", "otherStuff"],
        )
        new_parameter2 = cplui.ParameterEnum(
            "name",
            "description",
            "context",
            default="otherStuff",
            alternatives=["eso", "cpl", "aao", "otherStuff"],
        )

        assert new_parameter1 != new_parameter2
        new_parameter2.value = "cpl"
        assert new_parameter1 == new_parameter2

        # Outside of enum
        # But we're only testing if it stays equal
        new_parameter1.value = new_parameter2.value = "677810128"
        assert new_parameter1 == new_parameter2

    def test_int_enum_alternatives_unchangeable(self):
        new_parameter = cplui.ParameterEnum(
            "name", "description", "context", 297810128, [-12389, 297810128]
        )

        with pytest.raises(AttributeError):
            new_parameter.alternatives = [1, 2]

    def test_double_enum_alternatives_unchangeable(self):
        new_parameter = cplui.ParameterEnum(
            "name",
            "description",
            "context",
            default=0.1598211111,
            alternatives=[0.15982, 0.1598211111, 3.51231321, 0.15983],
        )

        with pytest.raises(AttributeError):
            new_parameter.alternatives = [4.6, 2.222]

    def test_double_string_alternatives_unchangeable(self):
        new_parameter = cplui.ParameterEnum(
            "name",
            "description",
            "context",
            default="otherStuff",
            alternatives=["eso", "cpl", "aao", "otherStuff"],
        )

        with pytest.raises(AttributeError):
            new_parameter.alternatives = ["a", "b"]

    def test_type_mismatch_int_enum(self):
        new_parameter = cplui.ParameterEnum(
            "name", "description", "context", 297810128, [-12389, 297810128]
        )

        # Unfortunatley the behaviour as of now throws a Runtime Error: Unexpected index
        # which is a bad message
        with pytest.raises(RuntimeError):
            new_parameter.value = "Hello"
        assert new_parameter.value == 297810128

        with pytest.raises(RuntimeError):
            new_parameter.value = True
        assert new_parameter.value == 297810128

        with pytest.raises(RuntimeError):
            new_parameter.value = False
        assert new_parameter.value == 297810128

        with pytest.warns(RuntimeWarning):
            new_parameter.value = 5.5

    def test_type_mismatch_double_range(self):
        new_parameter = cplui.ParameterEnum(
            "name",
            "description",
            "context",
            default=0.1598211111,
            alternatives=[0.15982, 0.1598211111, 3.51231321, 0.15983],
        )

        # Unfortunatley the behaviour as of now throws a Runtime Error: Unexpected index
        # which is a bad message
        with pytest.raises(RuntimeError):
            new_parameter.value = "Hello"
        assert new_parameter.value == 0.1598211111

        with pytest.raises(RuntimeError):
            new_parameter.value = True
        assert new_parameter.value == 0.1598211111

        with pytest.raises(RuntimeError):
            new_parameter.value = False
        assert new_parameter.value == 0.1598211111

        # This should NOT throw
        new_parameter.value = 5
        new_parameter.value = 1
        new_parameter.value = 0

    def test_type_mismatch_string_enum(self):
        new_parameter = cplui.ParameterEnum(
            "name",
            "description",
            "context",
            default="otherStuff",
            alternatives=["eso", "cpl", "aao", "otherStuff"],
        )

        # Unfortunatley the behaviour as of now throws a Runtime Error: Unexpected index
        # which is a bad message
        with pytest.raises(RuntimeError):
            new_parameter.value = 0.1598211111
        assert new_parameter.value == "otherStuff"

        with pytest.raises(RuntimeError):
            new_parameter.value = True
        assert new_parameter.value == "otherStuff"

        with pytest.raises(RuntimeError):
            new_parameter.value = False
        assert new_parameter.value == "otherStuff"

        with pytest.raises(RuntimeError):
            new_parameter.value = 24
        assert new_parameter.value == "otherStuff"


class TestParameterRange:
    def test_constructor_int_range_sets_properties(self):
        name = ""
        description = ""
        context = ""

        new_parameter = cplui.ParameterRange(
            name=name,
            description=description,
            context=context,
            default=901123,
            min=-12389,
            max=297810128,
        )

        assert new_parameter.name == name
        assert new_parameter.description == description
        assert new_parameter.context == context

        assert new_parameter.data_type == cplcore.Type.INT
        assert new_parameter.default == 901123
        assert new_parameter.value == 901123
        assert new_parameter.min == -12389
        assert new_parameter.max == 297810128

    def test_constructor_double_range_sets_properties(self):
        name = ""
        description = ""
        context = ""

        new_parameter = cplui.ParameterRange(
            name=name,
            description=description,
            context=context,
            default=0.1598211111,
            min=0.15982,
            max=0.15983,
        )

        assert new_parameter.name == name
        assert new_parameter.description == description
        assert new_parameter.context == context

        assert new_parameter.data_type == cplcore.Type.DOUBLE
        assert new_parameter.default == 0.1598211111
        assert new_parameter.value == 0.1598211111
        assert new_parameter.min == 0.15982
        assert new_parameter.max == 0.15983

    def test_constructor_int_out_of_range_no_except(self):
        name = ""
        description = ""
        context = ""

        # with pytest.raises(Exception):
        new_parameter = cplui.ParameterRange(
            name=name,
            description=description,
            context=context,
            default=-12390,
            min=-12389,
            max=297810128,
        )
        new_parameter.value = 299910128

    def test_constructor_double_out_of_range_no_except(self):
        name = ""
        description = ""
        context = ""

        # with pytest.raises(Exception):
        new_parameter = cplui.ParameterRange(
            name=name,
            description=description,
            context=context,
            default=0.159819999,
            min=0.15982,
            max=0.15983,
        )
        new_parameter.value = 4.19203

    def test_int_range_same_param_new_value_equal(self):
        new_parameter1 = cplui.ParameterRange(
            "name", "description", "context", 901123, -12389, 297810128
        )
        new_parameter2 = cplui.ParameterRange(
            "name", "description", "context", 901123, -12389, 297810128
        )

        # Bigger than max
        # But we're only testing if it stays equal
        assert new_parameter1 == new_parameter2
        new_parameter1.value = new_parameter2.value = 677810128

        assert new_parameter1 == new_parameter2
        new_parameter1.value = new_parameter2.value = -677810128

    def test_double_range_same_param_new_value_equal(self):
        new_parameter1 = cplui.ParameterRange(
            "name", "description", "context", 901.123, -12.389, 297810.128
        )
        new_parameter2 = cplui.ParameterRange(
            "name", "description", "context", 901.123, -12.389, 297810.128
        )

        # Bigger than max
        # But we're only testing if it stays equal
        assert new_parameter1 == new_parameter2
        new_parameter1.value = new_parameter2.value = 677810.128

        assert new_parameter1 == new_parameter2
        new_parameter1.value = new_parameter2.value = -677810.128

    def test_int_range_range_unchangeable(self):
        new_parameter = cplui.ParameterRange(
            "name", "description", "context", default=901123, min=-12389, max=297810128
        )

        with pytest.raises(AttributeError):
            new_parameter.min = 0

        with pytest.raises(AttributeError):
            new_parameter.max = -10

    def test_double_range_range_unchangeable(self):
        new_parameter = cplui.ParameterRange(
            "name",
            "description",
            "context",
            default=0.1598211111,
            min=0.15982,
            max=0.15983,
        )

        with pytest.raises(AttributeError):
            new_parameter.min = 0

        with pytest.raises(AttributeError):
            new_parameter.max = -10

    def test_type_mismatch_int_range(self):
        new_parameter = cplui.ParameterRange(
            "name", "description", "context", default=901123, min=-12389, max=297810128
        )

        # Unfortunatley the behaviour as of now throws a Runtime Error: Unexpected index
        # which is a bad message
        with pytest.raises(RuntimeError):
            new_parameter.value = "Hello"
        assert new_parameter.value == 901123

        with pytest.raises(RuntimeError):
            new_parameter.value = True
        assert new_parameter.value == 901123

        with pytest.raises(RuntimeError):
            new_parameter.value = False
        assert new_parameter.value == 901123

        with pytest.warns(RuntimeWarning):
            new_parameter.value = 5.5

    def test_type_mismatch_double_range(self):
        new_parameter = cplui.ParameterRange(
            "name",
            "description",
            "context",
            default=0.1598211111,
            min=0.15982,
            max=0.15983,
        )

        # Unfortunatley the behaviour as of now throws a Runtime Error: Unexpected index
        # which is a bad message
        with pytest.raises(RuntimeError):
            new_parameter.value = "Hello"
        assert new_parameter.value == 0.1598211111

        with pytest.raises(RuntimeError):
            new_parameter.value = True
        assert new_parameter.value == 0.1598211111

        with pytest.raises(RuntimeError):
            new_parameter.value = False
        assert new_parameter.value == 0.1598211111

        # This should NOT throw
        new_parameter.value = 5
        new_parameter.value = 1
        new_parameter.value = 0

    def test_presence_flag(self):
        value_param = cplui.ParameterValue(
            "name", "description", "context", default=0.1598211111
        )

        value_param.presence = True
        assert value_param.presence
        value_param.presence = False
        assert not value_param.presence

        enum_param = cplui.ParameterEnum(
            "name", "description", "context", default=3, alternatives=[1, 2, 3]
        )
        enum_param.presence = False
        assert not enum_param.presence
        enum_param.presence = True
        assert enum_param.presence

        range_param = cplui.ParameterRange(
            "name", "description", "context", default=50, min=2, max=100
        )
        print(range_param.presence)

        range_param.presence = True
        assert range_param.presence
        range_param.presence = False
        assert not range_param.presence

    def test_repr(self):
        param = cplui.ParameterValue(
            name="Dump string",
            description="Dump string Parameter",
            context="Dump string context",
            default=5,
        )
        expect = """<cpl.ui.ParameterValue: name='Dump string', value=5>"""
        assert repr(param) == expect

    def test_dump_file(self, tmp_path):
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_parameter_dump.txt"
        param = cplui.ParameterValue(
            name="Dump string",
            description="Dump string Parameter",
            context="Dump string context",
            default=5,
        )
        filename = tmp_path.joinpath(p)
        param.dump(filename=str(filename))
        expect = """Parameter at 0x6000029c5490: class 02 (Value), type 400 (int)
  Attributes:    name (at 0x6000000e9b50): Dump string    description (at 0x600000284560): Dump string Parameter    context (at 0x600000284540): Dump string context    user tag (at 0x0): 0x0
  Values:        Current: 5    Default: 5
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
        param = cplui.ParameterValue(
            name="Dump string",
            description="Dump string Parameter",
            context="Dump string context",
            default=5,
        )
        expect = """Parameter at 0x6000029c5490: class 02 (Value), type 400 (int)
  Attributes:    name (at 0x6000000e9b50): Dump string    description (at 0x600000284560): Dump string Parameter    context (at 0x600000284540): Dump string context    user tag (at 0x0): 0x0
  Values:        Current: 5    Default: 5
"""
        # remove memory address details
        expect = re.sub("0x[0-9a-z]+", "", expect)
        outp = re.sub("0x[0-9a-z]+", "", str(param))
        expect = re.sub("\\(n[a-z]+l\\)", "", expect)
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)

        assert outp == expect
        assert isinstance(param.dump(show=False), str)

    def test_dump_stdout(self):
        cmd = "from cpl import ui as cplui; "
        cmd += """param = cplui.ParameterValue(name="Dump string", description="Dump string Parameter", context="Dump string context", default=5); """
        cmd += "param.dump()"
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        expect = """Parameter at 0x6000029c5490: class 02 (Value), type 400 (int)
  Attributes:    name (at 0x6000000e9b50): Dump string    description (at 0x600000284560): Dump string Parameter    context (at 0x600000284540): Dump string context    user tag (at 0x0): 0x0
  Values:        Current: 5    Default: 5
"""
        # remove memory address details
        expect = re.sub("0x[0-9a-z]+", "", expect)
        outp = re.sub("0x[0-9a-z]+", "", outp)
        expect = re.sub("\\(n[a-z]+l\\)", "", expect)
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)

        assert outp == expect

    def test_dump_string_all_types(self):
        pv = cplui.ParameterValue(
            name="hellouser.username",
            context="hellouser",
            description="User's name",
            default="Unknown user",
        )
        pr = cplui.ParameterRange(
            name="hellouser.nfiles",
            context="hellouser",
            description="Number of input files to process.",
            default=3,
            min=1,
            max=5,
        )
        pe = cplui.ParameterEnum(
            name="hellouser.reaction",
            context="hellouser",
            description="Reaction to the user.",
            default="nice",
            alternatives=("nice", "nasty"),
        )

        expect = """Parameter at 0x6000039f51f0: class 02 (Value), type 21 (string)
  Attributes:    name (at 0x6000012d9020): hellouser.username    description (at 0x6000010cfe90): User's name    context (at 0x6000010cfe80): hellouser    user tag (at 0x0): 0x0
  Values:        Current: Unknown user    Default: Unknown user
"""
        expect = re.sub("0x[0-9a-z]+", "", expect)
        outp = re.sub("0x[0-9a-z]+", "", str(pv))
        expect = re.sub("\\(n[a-z]+l\\)", "", expect)
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)
        assert outp == expect
        outp = re.sub("0x[0-9a-z]+", "", pv.dump(show=False))
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)
        assert outp == expect

        expect = """Parameter at 0x600000468620: class 04 (Range), type 400 (int)
  Attributes:    name (at 0x600002f54ba0): hellouser.nfiles    description (at 0x60000216bd80): Number of input files to process.    context (at 0x600002d45e40): hellouser    user tag (at 0x0): 0x0
  Values:        Current: 3    Default: 3
"""
        expect = re.sub("0x[0-9a-z]+", "", expect)
        outp = re.sub("0x[0-9a-z]+", "", str(pr))
        expect = re.sub("\\(n[a-z]+l\\)", "", expect)
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)
        assert outp == expect
        outp = re.sub("0x[0-9a-z]+", "", pr.dump(show=False))
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)
        assert outp == expect

        expect = """Parameter at 0x600001a25730: class 08 (Enum), type 21 (string)
  Attributes:    name (at 0x60000311de80): hellouser.reaction    description (at 0x60000311dea0): Reaction to the user.    context (at 0x600003307720): hellouser    user tag (at 0x0): 0x0
  Values:        Current: nice    Default: nice
"""
        expect = re.sub("0x[0-9a-z]+", "", expect)
        outp = re.sub("0x[0-9a-z]+", "", str(pe))
        expect = re.sub("\\(n[a-z]+l\\)", "", expect)
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)
        assert outp == expect
        outp = re.sub("0x[0-9a-z]+", "", pe.dump(show=False))
        outp = re.sub("\\(n[a-z]+l\\)", "", outp)
        assert outp == expect
