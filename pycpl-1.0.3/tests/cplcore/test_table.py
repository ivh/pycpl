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

import math
import os
import warnings

import numpy as np
import pandas as pd
import pytest
import subprocess

from cpl import core as cplcore


@pytest.fixture(scope="module")
def small_table():
    t = cplcore.Table.empty(3)
    t["ints"] = [1, 2, 3]
    t["chars"] = ["a", "b", "c"]
    return t


class TestTable:
    def test_array_constructor(self):
        pd = pytest.importorskip(
            "pandas", reason="pandas not installed, cannot run this test."
        )
        p = pd.DataFrame({"a": [[1, 2, 3], [2, 3, 1]]})
        tab = cplcore.Table(p)
        assert np.array_equal(tab["a"][0][0], [1, 2, 3])
        assert np.array_equal(tab["a"][1][0], [2, 3, 1])

    def test_assign_string(self):
        t = cplcore.Table.empty(5)
        t["strings"] = ["foo", "barr", "c", "4", "5"]
        assert t.get_column_type("strings") == cplcore.Type.STRING
        assert np.array_equal(t["strings"], ["foo", "barr", "c", "4", "5"])
        assert t["strings"][0][0] == "foo"
        assert t["strings"][1][0] == "barr"
        assert t["strings"][2][0] == "c"
        assert t["strings"][3][0] == "4"
        assert t["strings"][4][0] == "5"

    def test_assign_int(self):
        t = cplcore.Table.empty(3)
        t["ints"] = [123142681, 21231223, -331213]
        assert np.array_equal(t["ints"], [123142681, 21231223, -331213])
        assert t.get_column_type("ints") == cplcore.Type.LONG_LONG
        assert t["ints"][0] == (123142681, False)
        assert t["ints", 1] == (21231223, False)
        assert t["ints", 2] == (-331213, False)
        t["intc"] = np.array([123, 4, -5], dtype=np.intc)
        assert t.get_column_type("intc") == cplcore.Type.INT
        assert np.array_equal(t["intc"], [123, 4, -5])
        assert t["intc"][0] == (123, False)
        assert t["intc"][1] == (4, False)
        assert t["intc", 2] == (-5, False)

    def test_assign_float(self):
        t = cplcore.Table.empty(3)
        t["double"] = [2.3, 4.14, -2424.2]
        assert t.get_column_type("double") == cplcore.Type.DOUBLE
        assert t["double"][0] == (2.3, False)
        assert t["double", 1] == (4.14, False)
        assert t["double", 2] == (-2424.2, False)
        assert np.array_equal(t["double"], [2.3, 4.14, -2424.2])
        t["float"] = np.array([3.14, 2.2, -5.5], dtype="f")
        assert t.get_column_type("float") == cplcore.Type.FLOAT
        assert t["float", 0][0] == pytest.approx(3.14, abs=0.00001)
        assert t["float", 0] == (
            3.140000104904175,
            False,
        )  # extra float added by CPL no rounding
        assert t["float", 1] == (2.200000047683716, False)
        assert t["float", 1][0] == pytest.approx(2.2, abs=0.00001)
        assert t["float", 2] == (-5.5, False)
        assert np.array_equal(t["float"], np.array([3.14, 2.2, -5.5], dtype=np.float32))

    def test_numpy_array_from_array_long(self):
        t = cplcore.Table.empty(3)
        t.new_column_array("test", cplcore.Type.LONG_LONG, 3)
        t["test"][0] = [5, 6, 7]
        t["test"][2] = [10, 20, 30]

        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            assert np.array_equal(t["test"], [[5, 6, 7], [0, 0, 0], [10, 20, 30]])

    def test_numpy_array_from_array_int(self):
        t = cplcore.Table.empty(3)
        t.new_column_array("test", cplcore.Type.INT, 3)
        t["test"][0] = np.array([5, 6, 7], dtype="intc")
        t["test"][2] = np.array([10, 20, 30], dtype="intc")

        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            assert np.array_equal(t["test"], [[5, 6, 7], [0, 0, 0], [10, 20, 30]])

    def test_numpy_array_from_array_double(self):
        t = cplcore.Table.empty(3)
        t.new_column_array("test", cplcore.Type.DOUBLE, 3)
        t["test"][0] = [5.2, 6, 7]
        t["test"][2] = [60.22, 55.11, -27.9]
        assert np.array_equal(t["test"][0][0], [5.2, 6, 7])
        assert np.array_equal(t["test"][2][0], [60.22, 55.11, -27.9])
        assert t["test"][0][1] is False
        assert t["test"][2][1] is False
        # Invalid element should return an empty array.
        np.testing.assert_array_equal(t["test"][1][0], np.array([]))
        assert t["test"][1][0].shape == (0,)
        assert t["test"][1][1] is True

    def test_numpy_array_from_array_double_complex(self):
        t = cplcore.Table.empty(3)
        t.new_column_array("test", cplcore.Type.DOUBLE_COMPLEX, 3)
        t["test"][0] = [5.2, 6, 7 + 22.2j]
        t["test"][1] = [60.22 + 56j, 55.11 + 22j, -27.9 - 27j]
        assert np.array_equal(t["test"][0][0], [5.2, 6, 7 + 22.2j])
        assert np.array_equal(t["test"][1][0], [60.22 + 56j, 55.11 + 22j, -27.9 - 27j])
        assert t["test"][0][1] is False
        assert t["test"][0][1] is False
        # Invalid element should return an empty array.
        np.testing.assert_array_equal(t["test"][2][0], np.array([]))
        assert t["test"][2][0].shape == (0,)
        assert t["test"][2][1] is True

    def test_column_new(self):
        t = cplcore.Table.empty(3)
        t.new_column("test", cplcore.Type.FLOAT)
        t["test"][0] = math.pi
        t["test", 1] = 331322.2
        assert t["test", 0][0] == pytest.approx(math.pi, abs=0.00001)
        assert t["test"][0][1] is False
        assert t["test"][1] == (
            331322.1875,
            False,
        )  # numbers changed after decimal roundinf error in CPL

    def test_assign_complex(self):
        t = cplcore.Table.empty(3)
        t["dComplex"] = [3 + 2j, -60000 + 2323j, 322323.0002 - 3.14j]
        assert np.array_equal(
            t["dComplex"], [3 + 2j, -60000 + 2323j, 322323.0002 - 3.14j]
        )
        assert t["dComplex", 0][0] == 3 + 2j
        assert t["dComplex", 1][0] == -60000 + 2323j
        assert t["dComplex", 2][0] == 322323.0002 - 3.14j

        t["fComplex"] = np.array(
            [3 + 2j, -60000 + 2323j, 322323.0002 - 3.14j], dtype=np.complex128
        )
        assert np.array_equal(
            t["fComplex"], [3 + 2j, -60000 + 2323j, 322323.0002 - 3.14j]
        )

    def test_column_stats(self):
        t = cplcore.Table.empty(3)
        t["values"] = [1, 2, 3]
        assert t.get_column_mean("values") == 2
        assert t.get_column_min("values") == 1
        assert t.get_column_minpos("values") == 0
        assert t.get_column_max("values") == 3
        assert t.get_column_maxpos("values") == 2
        assert t.get_column_stdev("values") == 1

    def test_insert(self):
        t1 = cplcore.Table.empty(3)
        t2 = cplcore.Table.empty(3)
        t1["values1"] = [1, 2, 3]
        t1["values2"] = [4, 5, 6]
        t2["values1"] = [10, 20, 30]
        t2["values2"] = [40, 50, 60]
        assert t1.compare_structure(t2)
        t1.insert(t2, 1)

        assert np.array_equal(t1["values1"], [1, 10, 20, 30, 2, 3])
        assert np.array_equal(t1["values2"], [4, 40, 50, 60, 5, 6])

    def test_array_complex(self):
        t = cplcore.Table.empty(3)
        t["arrays"] = [[1 + 32j, 2 + 0j], [3 + 4j, 4 + 4242j], [5 + 0j, -6 + 022j]]
        assert t.get_column_type("arrays") == cplcore.Type.ARRAY
        assert np.array_equal(t["arrays", 0][0], [1 + 32j, 2 + 0j])
        assert np.array_equal(t["arrays", 1][0], [3 + 4j, 4 + 4242j])
        assert np.array_equal(t["arrays", 2][0], [5 + 0j, -6 + 022j])

    def test_fill_array(self):
        t = cplcore.Table.empty(5)
        t.new_column_array("arrays", cplcore.Type.LONG_LONG, 3)
        t["arrays", 2:4] = [1, 2, 3]
        assert t.get_column_type("arrays") == cplcore.Type.ARRAY
        # Invalid elements should return empty arrays
        np.testing.assert_array_equal(t["arrays", 0][0], np.array([]))
        assert t["arrays", 0][0].shape == (0,)
        np.testing.assert_array_equal(t["arrays"][1][0], np.array([]))
        assert t["arrays"][1][0].shape == (0,)
        assert t["arrays", 0][1] is True
        assert t["arrays"][1][1] is True
        assert np.array_equal(t["arrays", 2][0], [1, 2, 3])
        assert np.array_equal(t["arrays", 3][0], [1, 2, 3])
        assert t["arrays", 2][1] is False
        assert t["arrays"][3][1] is False

    def test_fill_int(self):
        t = cplcore.Table.empty(10)
        t.new_column("ints", cplcore.Type.LONG_LONG)
        t["ints", 2:4] = 23
        assert t["ints", 0] == (0, True)
        assert t["ints", 1] == (0, True)
        assert t["ints", 2] == (23, False)
        assert t["ints", 3] == (23, False)

    def test_fill_float(self):
        from math import pi

        t = cplcore.Table.empty(10)
        t.new_column("floats", cplcore.Type.DOUBLE)
        t["floats", 2:4] = pi
        assert t["floats", 0] == (0.0, True)
        assert t["floats", 1] == (0.0, True)
        assert t["floats", 2] == (pi, False)
        assert t["floats", 3] == (pi, False)

    def test_selection_manual(self):
        t = cplcore.Table.empty(10)
        t["test1"] = [1, 2, 3, 4, 5] * 2
        t["test2"] = [9, 8, 7, 6, 5] * 2
        t.unselect_all()
        t.select_row(5)
        t.select_row(0)
        t.select_row(2)

        selected = [0, 2, 5]
        assert t.selected == 3
        assert np.array_equal(t.where_selected(), selected)
        for row in range(10):
            if row in selected:
                assert t.is_selected(row)
            else:
                assert not t.is_selected(row)
        t.not_selected()
        all_indices = range(10)
        selected = [x for x in all_indices if x not in selected]
        assert t.selected == 7
        assert np.array_equal(t.where_selected(), selected)

    def test_save(self):
        t1 = cplcore.Table.empty(10)
        t1["test1"] = [1, 2, 3, 4, 5] * 2
        t1["test2"] = [9, 8, 7, 6, 5] * 2
        t1.new_column("not_used", cplcore.Type.FLOAT)

        plist1 = cplcore.PropertyList()
        prop1 = cplcore.Property("named_property1", cplcore.Type.INT)
        prop1.value = 20
        plist1.append(prop1)
        t1.set_invalid("test1", 0)

        t1.save(plist1, plist1, "test_table.fits", cplcore.io.CREATE)
        # Test regular load
        loaded_table = cplcore.Table.load("test_table.fits", 1, False)

        assert loaded_table.count_invalid("not_used") == 0

        assert np.array_equal(loaded_table["test1"], [1, 2, 3, 4, 5] * 2)
        assert len(loaded_table) == 10
        assert loaded_table.shape == (10, 3)
        assert loaded_table.get_column_type("test1") == cplcore.Type.LONG_LONG
        assert loaded_table.column_names == ["test1", "test2", "not_used"]
        # Test regular load, invalidate empty rows
        loaded_table = cplcore.Table.load("test_table.fits", 1, True)
        assert loaded_table.count_invalid("not_used") == 10

        t2 = cplcore.Table.empty(5)
        t2.new_column_array("test1", cplcore.Type.DOUBLE, 3)
        t2["test1", 2] = [2.2, 4.5, 6.774]
        t2.new_column("test2", cplcore.Type.DOUBLE)
        t2["test2", 0] = 55.0
        t2.new_column("not_used", cplcore.Type.FLOAT)

        t2.save(plist1, plist1, "test_table.fits", cplcore.io.EXTEND)
        # Test loaded result
        # Test load window of all columns, do not invalidate empty cells
        loaded_table = cplcore.Table.load_window(
            "test_table.fits", 2, check_nulls=False, start=0, nrow=3
        )
        ser1 = pd.Series(["one", "two"])
        ser1[0] = np.nan
        assert len(loaded_table) == 3
        assert loaded_table.column_names == ["test1", "test2", "not_used"]
        # assert loaded_table["test2", 1][0] == ser1[0]
        # assert loaded_table["test2", 1] == (ser1[0], False)
        # assert (nan, False) == (nan, False)At index 0 diff: nan != nan
        assert loaded_table["test2", 0] == (55.0, False)
        # assert (loaded_table["test2", 2]) == (ser1[0], False)
        assert loaded_table.count_invalid("not_used") == 0

        # Test load window with only some columns, invalidate empty cells
        loaded_table = cplcore.Table.load_window(
            "test_table.fits",
            1,
            check_nulls=True,
            start=2,
            nrow=2,
            cols=["test2", "not_used"],
        )
        assert len(loaded_table) == 2
        assert loaded_table.column_names == ["test2", "not_used"]
        assert loaded_table.count_invalid("not_used") == 2
        assert np.array_equal(loaded_table["test2"], [7, 6])
        os.remove("test_table.fits")

    def test_valid_checks(self):
        t2 = cplcore.Table.empty(5)
        t2["test1"] = [1, 2, 3, 4, 5]  # Complete
        t2.new_column("test2", cplcore.Type.DOUBLE)  # Partially incomplete
        t2["test2", 0] = 55.0
        t2["test2", 3] = 55.0
        t2.new_column("not_used", cplcore.Type.FLOAT)  # Not complete
        assert t2.count_invalid("not_used") == 5
        assert t2.count_invalid("test2") == 3
        assert t2.count_invalid("test1") == 0

        assert t2.has_valid("test1")
        assert t2.has_valid("test2")
        assert not t2.has_valid("not_used")
        assert not t2.has_invalid("test1")
        assert t2.has_invalid("test2")
        assert t2.has_invalid("not_used")

        assert t2.is_valid("test1", 0)
        assert t2.is_valid("test2", 0)
        assert not t2.is_valid("test2", 1)
        assert not t2.is_valid("not_used", 1)

    def test_column_complex_mean(self):
        t1 = cplcore.Table.empty(5)
        t1["complex"] = [1.23 + 0j, 22j, 3223, 55.225j, 22.3 + 22321j]
        mean = t1.get_column_mean_complex("complex")
        assert np.isclose(
            mean,
            649.306 + ((22j + 55.225j + 22321j) / 5),
            atol=np.finfo(np.complex128).eps,
        )

    def test_column_rename(self):
        t = cplcore.Table.empty(5)
        t["test1"] = [1, 2, 3, 4, 5]  # Complete
        t.name_column("test1", "renamed_column")
        assert t.column_names == ["renamed_column"]
        assert np.array_equal(t["renamed_column"], [1, 2, 3, 4, 5])

    def test_column_set_size(self):
        t1 = cplcore.Table.empty(5)
        assert len(t1) == 5
        t1.set_size(500000)
        assert len(t1) == 500000

    def test_column_duplicate(self):
        t1 = cplcore.Table.empty(5)
        t2 = cplcore.Table.empty(5)
        t1["test1"] = [1, 2, 3, 4, 5]  # Complete
        t2.duplicate_column("duplicate", t1, "test1")
        # t.name_column("test1","renamed_column")
        # assert t.column_names==["renamed_column"]
        assert np.array_equal(t2["duplicate"], [1, 2, 3, 4, 5])

    def test_column_add_scalar(self):
        t1 = cplcore.Table.empty(5)
        t1["integer"] = [1, 2, 3, 4, 5]  # Complete
        t1.add_scalar("integer", 5)
        assert np.array_equal(t1["integer"], [6, 7, 8, 9, 10])

        t1["complex"] = [1.23 + 0j, 22j, 3223, 55.225j, 22.3 + 22321j]
        t1.add_scalar_complex("complex", 3 + 56j)

        assert np.array_equal(
            t1["complex"],
            [4.23 + 56j, 3 + 78j, 3226 + 56j, 3 + 111.225j, 25.3 + 22377j],
        )

    def test_column_subtract_scalar(self):
        t1 = cplcore.Table.empty(5)
        t1["integer"] = [1, 2, 3, 4, 5]  # Complete
        t1.subtract_scalar("integer", 5)
        assert np.array_equal(t1["integer"], [-4, -3, -2, -1, 0])

        t1["complex"] = [1.23 + 0j, 22j, 3223, 55.225j, 22.3 + 22321j]
        t1.subtract_scalar_complex("complex", 3 - 56j)
        assert np.array_equal(
            t1["complex"],
            [-1.77 + 56j, -3 + 78j, 3220 + 56j, -3 + 111.225j, 19.3 + 22377j],
        )

    def test_column_multiply_scalar(self):
        t1 = cplcore.Table.empty(5)
        t1["integer"] = [1, 2, 3, 4, 5]  # Complete
        t1.multiply_scalar("integer", 5)
        assert np.array_equal(t1["integer"], [5, 10, 15, 20, 25])

        t1["complex"] = [1.23 + 0j, 22j, 3223, 55.225j, 22.3 + 22321j]
        t1.multiply_scalar_complex("complex", 52.3 + 44j)
        assert np.array_equal(
            t1["complex"],
            [
                (1.23 + 0j) * (52.3 + 44j),
                (22j) * (52.3 + 44j),
                3223 * (52.3 + 44j),
                (55.225j) * (52.3 + 44j),
                (22.3 + 22321j) * (52.3 + 44j),
            ],
        )

    def test_column_divide_scalar(self):
        t1 = cplcore.Table.empty(5)
        t1["integer"] = [1, 2, 3, 4, 5]  # Complete
        t1.divide_scalar("integer", 5)

        assert np.array_equal(t1["integer"], [0, 0, 0, 0, 1])
        t1.new_column("doubles", cplcore.Type.DOUBLE)
        t1["doubles"] = [1.0, 2, 3, 4, 5]
        t1.divide_scalar("doubles", 5)
        # up to double precision
        assert np.allclose(t1["doubles"], [0.2, 0.4, 0.6, 0.8, 1.0])

        t1["complex"] = [1.23 + 0j, 22j, 3223, 55.225j, 22.3 + 22321j]
        t1.divide_scalar_complex("complex", 52.3 + 44j)
        assert np.allclose(
            t1["complex"],
            [
                (1.23 + 0j) / (52.3 + 44j),
                (22j) / (52.3 + 44j),
                3223 / (52.3 + 44j),
                (55.225j) / (52.3 + 44j),
                (22.3 + 22321j) / (52.3 + 44j),
            ],
        )

    def test_add_columns(self):
        t1 = cplcore.Table.empty(5)
        t1["integer"] = [1, 2, 3, 4, 5]
        t1["doubles"] = [1.2, 3.5, 5, 6, 7.3]
        t1.add_columns("doubles", "integer")
        assert np.array_equal(t1["doubles"], [2.2, 5.5, 8, 10, 12.3])
        t1.add_columns("integer", "doubles")
        assert np.array_equal(t1["integer"], [3, 7, 11, 14, 17])

    def test_subtract_columns(self):
        t1 = cplcore.Table.empty(5)
        t1["integer"] = [1, 2, 3, 4, 5]
        t1["doubles"] = [1.2, 3.5, 5, 6, 7.3]
        t1.subtract_columns("doubles", "integer")

        assert np.allclose(t1["doubles"], [0.2, 1.5, 2, 2, 2.3])

        t1.subtract_columns("integer", "doubles")
        assert np.array_equal(t1["integer"], [1 - 1, 2 - 2, 3 - 2, 4 - 2, 5 - 3])
        # from_column target has values rounded up when casting to long long

    def test_multiply_columns(self):
        t1 = cplcore.Table.empty(5)
        t1["integer"] = [1, 2, 3, 4, 5]
        t1["doubles"] = [1.2, 3.5, 5, 6, 7.3]
        t1.multiply_columns("doubles", "integer")
        # Multiply is up to double precision
        assert np.allclose(t1["doubles"], [1 * 1.2, 2 * 3.5, 3 * 5, 4 * 6, 5 * 7.3])

    def test_divide_columns(self):
        t1 = cplcore.Table.empty(5)
        t1["integer"] = [1, 2, 3, 4, 5]
        t1["doubles"] = [1.2, 3.5, 5, 6, 7.3]
        t1.divide_columns("doubles", "integer")
        # up to double precision
        assert np.allclose(t1["doubles"], [1.2, 3.5 / 2, 5 / 3, 6 / 4, 7.3 / 5])

    def test_logarithm_column(self):
        t1 = cplcore.Table.empty(5)
        t1.new_column("values", cplcore.Type.DOUBLE)
        bases = [2, 2, 2, 2, 2]
        powers = [3, 5.5, 22.2, 67.222, 1]
        t1["values"] = np.power(bases, powers)
        t1.logarithm_column("values", 2)

        # Check equality to double precision
        assert np.allclose(t1["values"], powers)

    def test_exponential_column(self):
        t1 = cplcore.Table.empty(5)
        bases = np.array([5 / 2, 2.5, 0.2, 5, -4], np.float64)
        t1["values"] = bases
        t1.exponential_column("values", 91)
        assert np.allclose(
            t1["values"],
            np.array(
                [pow(91, 5 / 2), pow(91, 2.5), pow(91, 0.2), pow(91, 5), pow(91, -4)]
            ),
        )

    def test_conjugate_column(self):
        t1 = cplcore.Table.empty(5)
        t1["complex"] = [1.23 + 0j, 22j, 3223, 55.225j, 22.3 + 22321j]
        t1.conjugate_column("complex")
        assert np.array_equal(
            t1["complex"], [1.23 - 0j, -22j, 3223 - 0j, -55.225j, 22.3 - 22321j]
        )

    def test_power_column(self):
        t1 = cplcore.Table.empty(5)
        t1["values"] = [1, 2, 3, 4, 5]
        t1.power_column("values", 3)
        assert np.array_equal(t1["values"], [1, 8, 27, 64, 125])

    def test_arg_column(self):
        t1 = cplcore.Table.empty(3)
        t1["values"] = [1.0 + 0j, 1.0j, 1 + 1j]
        t1.arg_column("values")
        assert np.allclose(t1["values"], np.angle([1.0, 1.0j, 1 + 1j]))

    def test_column_real_column(self):
        t1 = cplcore.Table.empty(5)
        t1["complex"] = [1.23 + 0j, 22j, 3223, 55.225j, 22.3 + 22321j]
        t1.real_column("complex")
        assert np.array_equal(t1["complex"], [1.23, 0, 3223, 0, 22.3])

    def test_column_imag_column(self):
        t1 = cplcore.Table.empty(5)
        t1["complex"] = [1.23 + 0j, 22j, 3223, 55.225j, 22.3 + 22321j]
        t1.imag_column("complex")
        assert np.array_equal(t1["complex"], [0, 22, 0, 55.225, 22321])

    def test_deepcopy(self):
        from copy import deepcopy

        t1 = cplcore.Table.empty(5)
        t1["integer"] = [1, 2, 3, 4, 5]
        dup_table = deepcopy(t1)
        del t1
        assert np.array_equal(dup_table["integer"], [1, 2, 3, 4, 5])

    def test_or_selected(self):
        t1 = cplcore.Table.empty(5)
        t1["left"] = [1, 2, 3, 4, 5]
        t1["right"] = [-5, 0, 5, 15, 5]
        t1.or_selected("left", cplcore.Table.Operator.EQUAL_TO, "right")
        # Or_ selected only performed on unselected rows
        assert t1.selected == 5
        assert np.array_equal(t1.where_selected(), [0, 1, 2, 3, 4])
        t1.unselect_all()
        t1.or_selected("left", cplcore.Table.Operator.EQUAL_TO, "right")
        assert t1.selected == 1
        assert np.array_equal(t1.where_selected(), [4])

    def test_and_selected(self):
        t1 = cplcore.Table.empty(5)
        t1["left"] = [1, 2, 3, 4, 5]
        t1["right"] = [-5, 0, 5, 15, 5]
        # and_selected only performed on selected rows
        t1.and_selected("left", cplcore.Table.Operator.GREATER_THAN, "right")
        assert t1.selected == 2
        assert np.array_equal(t1.where_selected(), [0, 1])
        t1.unselect_all()
        t1.and_selected("left", cplcore.Table.Operator.GREATER_THAN, "right")
        assert t1.selected == 0

    def test_selected_invalid(self):
        t1 = cplcore.Table.empty(5)
        t1.new_column("values1", cplcore.Type.LONG_LONG)
        t1["values1"][0] = 2
        t1["values1"][2] = 2
        t1.and_selected_invalid("values1")
        assert t1.selected == 3
        assert np.array_equal(t1.where_selected(), [1, 3, 4])
        t1.set_invalid("values1", 0)
        t1.or_selected_invalid("values1")
        assert t1.selected == 4
        assert np.array_equal(t1.where_selected(), [0, 1, 3, 4])
        t1.unselect_all()
        t1.or_selected_invalid("values1")
        assert t1.selected == 4
        assert np.array_equal(t1.where_selected(), [0, 1, 3, 4])

    def test_set_invalid(self):
        t = cplcore.Table.empty(3)

        col = "ivals"
        t.new_column(col, cplcore.Type.INT)
        t[col] = [-1, 5, 3]
        assert t[col, 0][0] == -1
        assert t[col, 0][1] is False
        t.set_invalid(col, 0)
        assert t[col, 0][0] == 0
        assert t[col, 0][1] is True

        col = "fvals"
        t.new_column(col, cplcore.Type.FLOAT)
        t[col] = [-1.0, 5.0, 3.0]
        assert t[col, 0][0] == -1.0
        assert t[col, 0][1] is False
        t.set_invalid(col, 0)
        assert t[col, 0][0] == 0.0
        assert t[col, 0][1] is True

        col = "dvals"
        t.new_column(col, cplcore.Type.DOUBLE)
        t[col] = [-1.0e50, 5.0e-50, 3.0e12]
        assert t[col, 0][0] == -1.0e50
        assert t[col, 0][1] is False
        t.set_invalid(col, 0)
        assert t[col, 0][0] == 0.0
        assert t[col, 0][1] is True

        col = "lvals"
        t.new_column(col, cplcore.Type.LONG_LONG)
        t[col] = [8738456817645827345, 34878237482734, 9328481923842934]
        assert t[col, 0][0] == 8738456817645827345
        assert t[col, 0][1] is False
        t.set_invalid(col, 0)
        assert t[col, 0][0] == 0.0
        assert t[col, 0][1] is True

        col = "fcvals"
        t.new_column(col, cplcore.Type.FLOAT_COMPLEX)
        t[col] = [1e4 + 0j, 1e-4 + 1e5j, 2e1 - 0.05j]
        assert isinstance(t[col, 0][0], complex)
        assert t[col, 0][0] == 1e4 + 0j
        assert t[col, 0][1] is False
        t.set_invalid(col, 0)
        assert isinstance(t[col, 0][0], complex)
        assert t[col, 0][0] == 0j
        assert t[col, 0][1] is True

        col = "dcvals"
        t.new_column(col, cplcore.Type.DOUBLE_COMPLEX)
        t[col] = [1e40 + 0j, 1e-40 + 1e50j, 2e10 - 0.05j]
        assert isinstance(t[col, 0][0], complex)
        assert t[col, 0][0] == 1e40 + 0j
        assert t[col, 0][1] is False
        t.set_invalid(col, 0)
        assert isinstance(t[col, 0][0], complex)
        assert t[col, 0][0] == 0j
        assert t[col, 0][1] is True

        col = "svals"
        t.new_column(col, cplcore.Type.STRING)
        t[col] = ["foo", "bar", "jump"]
        assert t[col, 0][0] == "foo"
        assert t[col, 0][1] is False
        t.set_invalid(col, 0)
        assert t[col, 0][0] == ""
        assert t[col, 0][1] is True

        col = "avals"
        t[col] = [[1 + 32j, 2 + 0j], [3 + 4j, 4 + 4242j], [5 + 0j, -6 + 022j]]
        assert t.get_column_depth(col) == 2
        assert np.array_equal(t[col, 0][0], [1 + 32j, 2 + 0j])
        assert t[col, 0][1] is False
        t.set_invalid(col, 0)
        # Invalid element should return an empty array
        np.testing.assert_array_equal(t[col, 0][0], np.array([]))
        assert t[col, 0][0].shape == (0,)
        assert t[col, 0][1] is True

    def test_sort(self):
        plist = cplcore.PropertyList()
        p1 = cplcore.Property("bool", cplcore.Type.BOOL)
        p2 = cplcore.Property("values", cplcore.Type.BOOL)
        p1.value = True
        p2.value = False
        plist.append(p1)
        plist.append(p2)
        t1 = cplcore.Table.empty(5)
        t1["values"] = [1241, 1214, 7866, 3333, 4565]
        t1["bool"] = [1, 0, 0, 0, 1]
        t1.sort(plist)
        # Sort by the bool column first
        assert np.array_equal(t1["values"], [1241, 4565, 1214, 3333, 7866])
        assert np.array_equal(t1["bool"], [1, 1, 0, 0, 0])

    def test_fill_invalid(self):
        t = cplcore.Table.empty(5)
        t.new_column("ints", cplcore.Type.INT)
        t.new_column("longs", cplcore.Type.LONG_LONG)
        t.new_column("floats", cplcore.Type.FLOAT)
        t.new_column("doubles", cplcore.Type.DOUBLE)
        t.new_column("float_complex", cplcore.Type.FLOAT_COMPLEX)
        t.new_column("double_complex", cplcore.Type.DOUBLE_COMPLEX)
        t.new_column("strings", cplcore.Type.STRING)
        t["ints"][2] = 25
        t["longs"][0] = 12345
        t["floats"][3] = 23.5
        t["float_complex"][1] = 22.2 + 5j
        t["double_complex"][1] = 22.2276 + 5j
        t["doubles"][2] = 23.2357
        t["strings"][2] = "pycpl"
        t.fill_invalid("ints", -55)
        t.fill_invalid("longs", 31232332)
        t.fill_invalid("floats", 3.14)
        t.fill_invalid("doubles", 55.2567)
        t.fill_invalid("float_complex", 96.3 + 4j)
        t.fill_invalid("double_complex", 349.25 + 128j)
        # t.fill_invalid("strings", "empty")
        # t.fill_invalid() is not defined for strings; they are empty strings by default
        assert np.array_equal(t["ints"], [-55, -55, 25, -55, -55])
        assert np.array_equal(
            t["longs"], [12345, 31232332, 31232332, 31232332, 31232332]
        )
        assert np.array_equal(t["strings"], ["", "", "pycpl", "", ""])
        assert np.allclose(t["floats"], [3.14, 3.14, 3.14, 23.5, 3.14])
        assert np.allclose(t["doubles"], [55.2567, 55.2567, 23.2357, 55.2567, 55.2567])
        assert np.allclose(
            t["float_complex"], [96.3 + 4j, 22.2 + 5j, 96.3 + 4j, 96.3 + 4j, 96.3 + 4j]
        )
        assert np.allclose(
            t["double_complex"],
            [349.25 + 128j, 22.2276 + 5j, 349.25 + 128j, 349.25 + 128j, 349.25 + 128j],
        )

    def test_as_array(self):
        t = cplcore.Table.empty(5)
        t.new_column("ints", cplcore.Type.INT)
        t.new_column("longs", cplcore.Type.LONG_LONG)
        t.new_column("floats", cplcore.Type.FLOAT)
        t.new_column("doubles", cplcore.Type.DOUBLE)
        t.new_column("float_complex", cplcore.Type.FLOAT_COMPLEX)
        t.new_column("double_complex", cplcore.Type.DOUBLE_COMPLEX)
        t.new_column("strings", cplcore.Type.STRING)

        # first check as_array for empty columns
        mask = [True, True, True, True, True]
        assert np.array_equal(
            t["ints"].as_array, np.ma.MaskedArray(data=[0, 0, 0, 0, 0], mask=mask)
        )
        assert np.array_equal(
            t["longs"].as_array, np.ma.MaskedArray(data=[0, 0, 0, 0, 0], mask=mask)
        )
        assert np.ma.allclose(
            t["floats"].as_array,
            np.ma.MaskedArray(data=[0.0, 0.0, 0.0, 0.0, 0.0], mask=mask),
        )
        assert np.ma.allclose(
            t["doubles"].as_array,
            np.ma.MaskedArray(data=[0.0, 0.0, 0.0, 0.0, 0.0], mask=mask),
        )
        assert np.ma.allclose(
            t["float_complex"].as_array,
            np.ma.MaskedArray(
                data=[0.0 + 0.0j, 0.0 + 0.0j, 0.0 + 0.0j, 0.0 + 0.0j, 0.0 + 0.0j],
                mask=mask,
            ),
        )
        assert np.ma.allclose(
            t["double_complex"].as_array,
            np.ma.MaskedArray(
                data=[0.0 + 0.0j, 0.0 + 0.0j, 0.0 + 0.0j, 0.0 + 0.0j, 0.0 + 0.0j],
                mask=mask,
            ),
        )
        assert np.array_equal(
            t["strings"].as_array,
            np.ma.MaskedArray(data=["", "", "", "", ""], mask=mask),
        )

        t["ints"][2] = 25
        t["longs"][0] = 12345
        t["floats"][3] = 23.5
        t["float_complex"][1] = 22.2 + 5j
        t["double_complex"][1] = 22.2276 + 5j
        t["doubles"][2] = 23.2357
        t["strings"][2] = "pycpl"

        # now check as_array for normal columns
        assert np.array_equal(
            t["ints"].as_array,
            np.ma.MaskedArray(
                data=[0, 0, 25, 0, 0], mask=[False, False, True, False, False]
            ),
        )
        assert np.array_equal(
            t["longs"].as_array,
            np.ma.MaskedArray(
                data=[12345, 0, 0, 0, 0], mask=[True, False, False, False, False]
            ),
        )
        assert np.ma.allclose(
            t["floats"].as_array,
            np.ma.MaskedArray(
                data=[0, 0, 0, 23.5, 0], mask=[False, False, False, True, False]
            ),
        )
        assert np.ma.allclose(
            t["float_complex"].as_array,
            np.ma.MaskedArray(
                data=[0, 22.2 + 5j, 0, 0, 0], mask=[False, True, False, False, False]
            ),
        )
        assert np.ma.allclose(
            t["double_complex"].as_array,
            np.ma.MaskedArray(
                data=[0, 22.2276 + 5j, 0, 0, 0], mask=[False, True, False, False, False]
            ),
        )
        assert np.ma.allclose(
            t["doubles"].as_array,
            np.ma.MaskedArray(
                data=[0, 0, 23.2357, 0, 0], mask=[False, False, True, False, False]
            ),
        )
        assert np.array_equal(
            t["strings"].as_array,
            np.ma.MaskedArray(
                data=["", "", "pycpl", "", ""], mask=[False, False, True, False, False]
            ),
        )

        # now check as_array for array columns
        t = cplcore.Table.empty(3)
        t.new_column_array("ints", cplcore.Type.INT, 3)
        t.new_column_array("longs", cplcore.Type.LONG_LONG, 3)
        t.new_column_array("floats", cplcore.Type.FLOAT, 3)
        t.new_column_array("doubles", cplcore.Type.DOUBLE, 3)
        t.new_column_array("float_complex", cplcore.Type.FLOAT_COMPLEX, 3)
        t.new_column_array("double_complex", cplcore.Type.DOUBLE_COMPLEX, 3)
        t.new_column_array("strings", cplcore.Type.STRING, 3)
        mask = [[False, False, False], [True, True, True], [False, False, False]]

        t["ints"][0] = np.array([5, 6, 7], dtype="intc")
        t["ints"][2] = np.array([10, 20, 30], dtype="intc")
        t["longs"][0] = [5, 6, 7]
        t["longs"][2] = [10, 20, 30]
        t["floats"][0] = np.array([5.0, 6.0, 7.0], dtype="f")
        t["floats"][2] = np.array([10.0, 20.0, 30.0], dtype="f")
        t["doubles"][0] = [5.0, 6.0, 7.0]
        t["doubles"][2] = [10.0, 20.0, 30.0]
        t["float_complex"][0] = np.array(
            [5.0 + 5.0j, 6.0 + 6.0j, 7.0 + 7.0j], dtype=np.csingle
        )
        t["float_complex"][2] = np.array(
            [10.0 + 10.0j, 20.0 + 20.0j, 30.0 + 30.0j], dtype=np.csingle
        )
        t["double_complex"][0] = [5.0 + 5.0j, 6.0 + 6.0j, 7.0 + 7.0j]
        t["double_complex"][2] = [10.0 + 10.0j, 20.0 + 20.0j, 30.0 + 30.0j]
        t["strings"][0] = np.array(["a", "b", "c"])
        t["strings"][2] = np.array(["", "d", "e"])

        assert np.array_equal(
            t["ints"].as_array,
            np.ma.MaskedArray(data=[[5, 6, 7], [0, 0, 0], [10, 20, 30]], mask=mask),
        )
        assert np.array_equal(
            t["longs"].as_array,
            np.ma.MaskedArray(data=[[5, 6, 7], [0, 0, 0], [10, 20, 30]], mask=mask),
        )
        assert np.ma.allclose(
            t["floats"].as_array,
            np.ma.MaskedArray(
                data=[[5.0, 6.0, 7.0], [0.0, 0.0, 0.0], [10.0, 20.0, 30.0]], mask=mask
            ),
        )
        assert np.ma.allclose(
            t["doubles"].as_array,
            np.ma.MaskedArray(
                data=[[5.0, 6.0, 7.0], [0.0, 0.0, 0.0], [10.0, 20.0, 30.0]], mask=mask
            ),
        )
        assert np.ma.allclose(
            t["float_complex"].as_array,
            np.ma.MaskedArray(
                data=[
                    [5.0 + 5.0j, 6.0 + 6.0j, 7.0 + 7.0j],
                    [0.0, 0.0, 0.0],
                    [10.0 + 10.0j, 20.0 + 20.0j, 30.0 + 30.0j],
                ],
                mask=mask,
            ),
        )
        assert np.ma.allclose(
            t["double_complex"].as_array,
            np.ma.MaskedArray(
                data=[
                    [5.0 + 5.0j, 6.0 + 6.0j, 7.0 + 7.0j],
                    [0.0, 0.0, 0.0],
                    [10.0 + 10.0j, 20.0 + 20.0j, 30.0 + 30.0j],
                ],
                mask=mask,
            ),
        )
        assert np.array_equal(
            t["strings"].as_array,
            np.ma.MaskedArray(
                data=[["a", "b", "c"], ["", "", ""], ["", "d", "e"]], mask=mask
            ),
        )
        assert t["strings"].as_array.dtype == "<U1"

    def test_selected_window(self):
        t = cplcore.Table.empty(5)
        t["values"] = [1, 2, 3, 4, 5]
        t.and_selected_window(2, 2)
        assert t.selected == 2
        assert np.array_equal(t.where_selected(), [2, 3])
        t.unselect_all()
        t.or_selected_window(2, 2)
        assert t.selected == 2
        assert np.array_equal(t.where_selected(), [2, 3])

    def test_selected_string(self):
        t = cplcore.Table.empty(5)
        t["strings"] = ["not_selected", "a", "b", "selected", "not_selected"]
        t.and_selected_string(
            "strings", cplcore.Table.Operator.NOT_EQUAL_TO, "not_selected"
        )
        assert t.selected == 3
        assert np.array_equal(t.where_selected(), [1, 2, 3])
        t.unselect_all()
        t.or_selected_string("strings", cplcore.Table.Operator.NOT_GREATER_THAN, "c")
        assert t.selected == 2
        assert np.array_equal(t.where_selected(), [1, 2])

    def test_selected_numerical_int(self):
        t = cplcore.Table.empty(5)
        t.new_column("ints", cplcore.Type.INT)
        t["ints"] = [1, 2, 3, 4, 5]
        t.and_selected_numerical("ints", cplcore.Table.Operator.NOT_EQUAL_TO, 3)
        assert t.selected == 4
        assert np.array_equal(t.where_selected(), [0, 1, 3, 4])
        t.unselect_all()
        t.or_selected_numerical("ints", cplcore.Table.Operator.NOT_EQUAL_TO, 3)
        assert t.selected == 4
        assert np.array_equal(t.where_selected(), [0, 1, 3, 4])

    def test_selected_numerical_long(self):
        t = cplcore.Table.empty(5)
        t["longs"] = [1, 2, 3, 4, 5]
        t.and_selected_numerical("longs", cplcore.Table.Operator.NOT_EQUAL_TO, 3)
        assert t.selected == 4
        assert np.array_equal(t.where_selected(), [0, 1, 3, 4])
        t.unselect_all()
        t.or_selected_numerical("longs", cplcore.Table.Operator.NOT_EQUAL_TO, 3)
        assert t.selected == 4
        assert np.array_equal(t.where_selected(), [0, 1, 3, 4])

    def test_selected_numerical_float(self):
        t = cplcore.Table.empty(5)
        t.new_column("floats", cplcore.Type.FLOAT)
        t["floats"] = [1.0, 2.0, 6.5, 2.4, 5.5]
        t.and_selected_numerical("floats", cplcore.Table.Operator.NOT_LESS_THAN, 3.2)
        assert t.selected == 2
        assert np.array_equal(t.where_selected(), [2, 4])
        t.unselect_all()
        t.or_selected_numerical("floats", cplcore.Table.Operator.NOT_LESS_THAN, 3.2)
        assert t.selected == 2
        assert np.array_equal(t.where_selected(), [2, 4])

    def test_selected_numerical_double(self):
        t = cplcore.Table.empty(5)
        t["doubles"] = [1.033, 2.02336, 6.5, 2.4, 5.5]
        t.and_selected_numerical("doubles", cplcore.Table.Operator.EQUAL_TO, 6.5)
        assert t.selected == 1
        assert np.array_equal(t.where_selected(), [2])
        t.unselect_all()
        t.or_selected_numerical("doubles", cplcore.Table.Operator.EQUAL_TO, 6.5)
        assert t.selected == 1
        assert np.array_equal(t.where_selected(), [2])

    def test_selected_numerical_double_complex(self):
        t = cplcore.Table.empty(5)
        t["double_complex"] = [1.23 - 0j, -22j, 3223 - 0j, -22j, 22.3 - 22321j]
        t.and_selected_numerical(
            "double_complex", cplcore.Table.Operator.EQUAL_TO, -22j
        )
        assert t.selected == 2
        assert np.array_equal(t.where_selected(), [1, 3])
        t.unselect_all()
        t.or_selected_numerical(
            "double_complex", cplcore.Table.Operator.NOT_EQUAL_TO, -22j
        )
        assert t.selected == 3
        assert np.array_equal(t.where_selected(), [0, 2, 4])

    def test_selected_numerical_float_complex(self):
        t = cplcore.Table.empty(5)
        t.new_column("float_complex", cplcore.Type.FLOAT_COMPLEX)
        t["float_complex"] = [1.23 - 0j, -22j, 3223 - 0j, -22j, 22.3 - 22321j]
        t.and_selected_numerical("float_complex", cplcore.Table.Operator.EQUAL_TO, -22j)
        assert t.selected == 2
        assert np.array_equal(t.where_selected(), [1, 3])
        t.unselect_all()
        t.or_selected_numerical(
            "float_complex", cplcore.Table.Operator.NOT_EQUAL_TO, -22j
        )
        assert t.selected == 3
        assert np.array_equal(t.where_selected(), [0, 2, 4])

    def test_to_recarray(self):
        t = cplcore.Table.empty(5)
        t["ints"] = [1, 2, 3, 4, 5]
        t["doubles"] = [1.2, 5.6, 2.3, 67, 2.5]
        t["complex"] = [1.23 - 0j, -22j, 3223 - 0j, -22j, 22.3 - 22321j]
        as_recarray = t.to_records()
        assert np.array_equal(as_recarray["ints"], [1, 2, 3, 4, 5])
        assert np.array_equal(as_recarray["doubles"], [1.2, 5.6, 2.3, 67, 2.5])
        assert np.array_equal(
            as_recarray["complex"], [1.23 - 0j, -22j, 3223 - 0j, -22j, 22.3 - 22321j]
        )

    def test_delete_with_column(self):
        t = cplcore.Table.empty(5)
        t["values"] = [1, 2, 3, 4, 5]
        column = t["values"]
        del t
        # Column should still be accessible without segfaults
        assert column[0] == (1, False)
        assert column[1] == (2, False)
        assert column[2] == (3, False)
        assert column[3] == (4, False)
        assert column[4] == (5, False)

    def test_pandas_compatibility(self):
        pd = pytest.importorskip(
            "pandas", reason="pandas not installed, cannot run this test."
        )
        df = pd.DataFrame(
            {"a": [1, 2, 3, 4, 5, 6], "b": ["hi", "there", "ESO", "how", "are", "you"]}
        )
        t1 = cplcore.Table(df)
        assert np.array_equal(t1["a"], [1, 2, 3, 4, 5, 6])
        assert np.array_equal(t1["b"], ["hi", "there", "ESO", "how", "are", "you"])

    def test_invalid_return(self):
        t = cplcore.Table.empty(3)
        t["intc"] = np.array([123, 4, -5], dtype=np.intc)
        assert t.get_column_type("intc") == cplcore.Type.INT
        assert np.array_equal(t["intc"], [123, 4, -5])
        assert t["intc"][0] == (123, False)
        assert t["intc"][1] == (4, False)
        assert t["intc", 2] == (-5, False)
        t1 = cplcore.Table.empty(10)
        t1.new_column("floats", cplcore.Type.DOUBLE)
        t1["floats", 2:4] = math.pi
        assert t1["floats", 0] == (0.0, True)

    def test_row_access(self, small_table):
        row = small_table[1]
        assert type(row) is dict
        assert row == {"ints": (2, False), "chars": ("b", False)}

    def test_row_too_large(self, small_table):
        with pytest.raises(IndexError):
            _ = small_table[99]

    def test_row_negative(self, small_table):
        with pytest.raises(IndexError):
            _ = small_table[-1]

    def test_row_iterator(self, small_table):
        rows = [row for row in small_table]
        assert len(rows) == 3
        assert rows[2] == {"ints": (3, False), "chars": ("c", False)}

    def test_bad_column_string(self):
        t = cplcore.Table.empty(5)
        t.new_column("strings", cplcore.Type.STRING)
        t["strings"] = ["foo", None, "c", "4", "5"]
        assert t["strings", 0] == ("", True)
        assert t["strings", 1] == ("", True)
        t["strings"] = ["foo", 8, "c", "4", "5"]
        assert t["strings", 0] == ("foo", False)
        assert t["strings", 1] == ("8", False)

    def test_dump_string(self):
        t = cplcore.Table.empty(5)
        t["ints"] = [1, 2, 3, 4, 5]
        outp = """    ints   

 0       1  
 1       2  
 2       3  
 3       4  
 4       5  
"""  # noqa
        assert str(t) == outp
        assert isinstance(t.dump(show=False), str)

    def test_dump_file(self, tmp_path):
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_table_dump.txt"
        filename = tmp_path.joinpath(p)
        t = cplcore.Table.empty(5)
        t["ints"] = [1, 2, 3, 4, 5]
        t.dump(filename=str(filename))
        outp = """    ints   

 0       1  
 1       2  
 2       3  
 3       4  
 4       5  
"""  # noqa
        contents = ""
        with open(str(filename), "r") as f:
            for line in f.readlines():
                contents += line
        assert contents == outp

    def test_dump_stdout(self):
        cmd = "from cpl import core as cplcore; "
        cmd += "t = cplcore.Table.empty(5);"
        cmd += """t["ints"] = [1, 2, 3, 4, 5];"""
        cmd += "t.dump()"
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        expect = """    ints   

 0       1  
 1       2  
 2       3  
 3       4  
 4       5  
"""  # noqa
        assert outp == expect

    def test_repr(self):
        t = cplcore.Table.empty(5)
        t["ints"] = [1, 2, 3, 4, 5]
        expect = """Table with 5/5 selected rows and 1 columns:

  ints:
     5 long long integer elements, of which 0 are flagged invalid.
"""  # noqa
        assert repr(t) == expect
