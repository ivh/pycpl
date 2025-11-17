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
import sys

import cpl.core as cplcore

do_bench = False


class TestMatrix:
    # Python port of assert cpl_test_matrix_abs
    @classmethod
    def isclose(cls, first, second, tolerance):
        assert first.width == second.width
        assert first.height == second.height
        diff = cplcore.Matrix(first)
        diff.subtract(second)

        difval = max((abs(e) for row in diff for e in row))
        return difval <= tolerance

    @pytest.mark.xfail(reason="Machine translated CPL tests with multiple errors.")
    def test_main(self):
        import sys
        import math

        nreps = 1
        nelem = 70
        subnelem = 10
        a2390 = [
            2.125581475848425e-314,
            3.952525166729972e-323,
            -0,
            -2.393462054246040e-162,
            3.952525166729972e-323,
            1.677069691870074e-311,
            -0,
            -6.726400100717507e-161,
            -0,
            -0,
            0,
            1.172701769974922e-171,
            -2.393462054246040e-162,
            -6.726400100717507e-161,
            1.172701769974922e-171,
            6.189171527656329e10,
        ]
        b2390 = [
            -1.709710615916477e-161,
            -4.805142076113790e-160,
            8.376377601213216e-171,
            4.962260007044318e01,
        ]
        a3097 = [
            22.3366126198544243663945962908,
            -16.2991659056583451103961124318,
            0.0747823788545301237906670621669,
            0.0237870531554605392499102123338,
            -16.2991659056583451103961124318,
            17.2102745748494783128990093246,
            -0.124262796786316020991591813072,
            0.00881594777810015828301004603418,
            0.0747823788545301237906670621669,
            -0.124262796786316020991591813072,
            0.00370155728670603376487258096006,
            0.00719839528504809342962511564679,
            0.0237870531554605392499102123338,
            0.00881594777810015828301004603418,
            0.00719839528504809342962511564679,
            0.0220958028150731733418865587737,
        ]
        b3097 = [1, 2, 3, 4]
        l = 0

        # INVALID         void(None.stdev())
        matrix = cplcore.Matrix(4, 4, a3097)
        value = matrix.stdev()
        assert 0 <= value
        matrix = cplcore.Matrix(4, 4, a3097)
        matrix
        copia = cplcore.Matrix(4, 1, b3097)
        copia
        try:
            xsolv = matrix.solve(copia)

            xsolv
            print("%s" % ("Compute determinant of an ill-conditioned matrix"))
            assert math.isclose(0, matrix.determinant(), abs_tol=sys.float_info.epsilon)
        except cplcore.Error as es:
            try:
                es.raise_last()
            except cplcore.SingularMatrixError:
                print(
                    "cpl_matrix_solve() classified a near-singular matrix as singular. This is not a bug, but it indicates that cpl_matrix_solve() is less accurate on this computer. This loss of accuracy may be caused by compiling CPL with optimization"
                    % ()
                )
                assert math.isclose(matrix.determinant(), 0, abs_tol=0)

        matrix = cplcore.Matrix(4, 4, a2390)
        matrix
        copia = cplcore.Matrix(4, 1, b2390)
        copia
        try:
            xsolv = matrix.solve(copia)

            xsolv.dump()
            value = xsolv.mean()
            # DELETED cpl_test_error_macro(CPL_ERROR_NONE
            assert 1 if value == value else 0 != True
        except cplcore.Error as es:
            try:
                es.raise_last()
            except cplcore.SingularMatrixError:
                print(
                    "cpl_matrix_solve() classified a near-singular matrix as singular. This is not a bug, but it indicates that cpl_matrix_solve() is less accurate on this computer"
                    % ()
                )
                xsolv = cplcore.Matrix(copia)

        with pytest.raises(cplcore.DivisionByZeroError):
            try:
                matrix.solve_lu(xsolv, [0, 0, 0, 0])
            except cplcore.Error as es:
                es.raise_last()

        print("%s" % ("Creating the test matrix... "))
        matrix = cplcore.Matrix(7, 10)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        matrix

        r = 7
        c = 10
        print("%s" % ("Creating another test matrix... "))
        matrix = cplcore.Matrix(r, c)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        matrix

        matrix.fill(13.565656565)
        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of constant matrix... " % (i, j))
                assert math.isclose(13.565656565, matrix[i][j], abs_tol=0.000001)

        matrix = cplcore.Matrix(3, 3)
        for i in range(3):
            matrix[i][i] = 1

        print("%s" % ("Checking if matrix is identity... "))
        assert 1 == matrix.is_identity(-1.0)

        dArray = [9 + i * 0.01 for i in range(nelem)]
        assert len(dArray) == nelem

        subdArray = [1 + i * 0.01 for i in range(subnelem)]
        assert len(subdArray) == subnelem

        matrix = cplcore.Matrix(5, 5)
        for i in range(5):
            matrix[i][i] = dArray[i]

        print("%s" % ("Checking if matrix is diagonal... "))
        assert 1 == matrix.is_diagonal(-1.0)

        print("%s" % ("Creating the final test matrix... "))
        matrix = cplcore.Matrix(7, 10, dArray)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        matrix

        print("%s" % ("Reading a matrix element... "))
        assert math.isclose(9.57, matrix[5][7], abs_tol=0.001)

        print("%s" % ("Writing a matrix element... "))
        matrix[6][5] = 1.00

        print("%s" % ("Reading just written matrix element... "))
        assert math.isclose(1.00, matrix[6][5], abs_tol=0.001)

        print("%s" % ("Copy a matrix... "))
        copia = cplcore.Matrix(matrix)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        print("%s" % ("Subtract original from copy... "))
        assert copia.subtract(matrix)

        print("%s" % ("Check if result is zero... "))
        assert 1 == copia.is_zero(-1)

        print("%s" % ("Extracting submatrix... "))
        copia = matrix.extract(2, 3, 1, 1, 4, 3)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        print("%s" % ("Reading a submatrix element... "))
        assert math.isclose(9.44, copia[2][1], abs_tol=0.001)

        print("%s" % ("Extracting submatrix (beyond upper boundaries)... "))
        copia = matrix.extract(4, 4, 1, 1, 1000, 1000)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        print("%s" % ("Reading new submatrix element... "))
        assert math.isclose(9.55, copia[1][1], abs_tol=0.001)

        print("%s" % ("Extracting submatrix (step 3, beyond boundaries)... "))
        copia = matrix.extract(0, 0, 3, 3, 1000, 1000)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        print("%s" % ("Reading new submatrix element... "))
        assert math.isclose(9.33, copia[1][1], abs_tol=0.001)

        # INVALID         null=None.extract_row(1)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            try:
                matrix.extract_row(9999999)
            except cplcore.Error as es:
                es.raise_last()
        with pytest.raises(cplcore.AccessOutOfRangeError):
            try:
                matrix.extract_row(-1)
            except cplcore.Error as es:
                es.raise_last()
        # INVALID         None.extract_column(1)
        with pytest.raises(cplcore.AccessOutOfRangeError):
            try:
                matrix.extract_column(9999999)
            except cplcore.Error as es:
                es.raise_last()
        with pytest.raises(cplcore.AccessOutOfRangeError):
            try:
                matrix.extract_column(-1)
            except cplcore.Error as es:
                es.raise_last()
        print("%s" % ("Copy one row... "))
        copia = matrix.extract_row(2)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        c = matrix.width
        for i in range(c):
            print("Check element %ld of copied row... " % (i))
            assert math.isclose(matrix[2][i], copia[0][i], abs_tol=0.00001)

        value = 9.00
        for i in range(nelem):
            dArray[i] = value
            value += 0.01

        print("%s" % ("Copy main diagonal... "))
        copia = matrix.extract_diagonal(0)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        r = matrix.height
        for i in range(r):
            print("Check element %ld of copied diagonal... " % (i))
            assert math.isclose(matrix[i][i], copia[i][0], abs_tol=0.00001)

        print("%s" % ("Copy third diagonal... "))
        copia = matrix.extract_diagonal(2)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        for i in range(r):
            print("Check element %ld of copied third diagonal... " % (i))
            assert math.isclose(matrix[i][i + 2], copia[i][0], abs_tol=0.00001)

        value = 9.00
        for i in range(nelem):
            dArray[i] = value
            value += 0.01

        print("%s" % ("A new test matrix... "))
        matrix = cplcore.Matrix(10, 7, dArray)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        matrix

        print("%s" % ("Copy main diagonal (vertical)... "))
        copia = matrix.extract_diagonal(0)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        c = matrix.width
        for i in range(c):
            print("Check element %ld of copied diagonal (vertical)... " % (i))
            assert math.isclose(matrix[i][i], copia[0][i], abs_tol=0.00001)

        print("%s" % ("Copy second diagonal (vertical)... "))
        copia = matrix.extract_diagonal(1)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        for i in range(c):
            print("Check element %ld of copied second diagonal (vertical)... " % (i))
            assert math.isclose(matrix[i + 1][i], copia[0][i], abs_tol=0.00001)

        print("%s" % ("Write same value to entire matrix... "))
        assert matrix.fill(3.33)

        c = matrix.width
        r = c * matrix.height
        for i in range(r):
            print("Check element %ld, %ld of matrix... " % (i / c, i % c))
            assert math.isclose(3.33, matrix[int(i / c)][int(i % c)], abs_tol=0.00001)

        print("%s" % ("Write 2 to row 4..."))
        assert matrix.fill_row(2.0, 4)

        for i in range(c):
            print("Check element %ld of constant row... " % (i))
            assert math.isclose(2.0, matrix[4][i], abs_tol=0.00001)

        print("%s" % ("Write 9.99 to column 2..."))
        assert matrix.fill_column(9.99, 2)

        r = matrix.height
        for i in range(r):
            print("Check element %ld of constant column... " % (i))
            assert math.isclose(9.99, matrix[i][2], abs_tol=0.00001)

        print("%s" % ("Write 1.11 to main diagonal..."))
        assert matrix.fill_diagonal(1.11, 0)

        for i in range(c):
            print("Check element %ld of constant main diagonal... " % (i))
            assert math.isclose(1.11, matrix[i][i], abs_tol=0.00001)

        print("%s" % ("Write 1.11 to second diagonal..."))
        assert matrix.fill_diagonal(1.10, -1)

        for i in range(c):
            print("Check element %ld of constant second diagonal... " % (i))
            assert math.isclose(1.1, matrix[i + 1][i], abs_tol=0.00001)

        value = 9.00
        for i in range(nelem):
            dArray[i] = value
            value += 0.01

        print("%s" % ("One more test matrix... "))
        matrix = cplcore.Matrix(7, 10, dArray)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        matrix

        print("%s" % ("Copying square submatrix... "))
        copia = matrix.extract(0, 0, 1, 1, 4, 4)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        r = copia.height
        c = copia.width
        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of submatrix... " % (i, j))
                assert math.isclose(matrix[i][j], copia[i][j], abs_tol=0.00001)

        print("%s" % ("Subtracting constant from submatrix... "))
        assert copia.add_scalar(-8.0)

        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of modified submatrix... " % (i, j))
                assert math.isclose(matrix[i][j] - 8.0, copia[i][j], abs_tol=0.00001)

        print("%s" % ("Writing submatrix into matrix..."))
        assert matrix.copy(copia, 2, 2)

        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of modified matrix... " % (i + 2, j + 2))
                assert math.isclose(copia[i][j], matrix[i + 2][j + 2], abs_tol=0.00001)

        print("%s" % ("Writing same value into submatrix... "))
        assert matrix.fill_window(0, 1, 1, 4, 3)

        for i in range(1, 4):
            for j in range(1, 3):
                print("Check element %ld, %ld of constant submatrix... " % (i, j))
                assert math.isclose(0, matrix[i][j], abs_tol=0.00001)

        value = 9.00
        for i in range(nelem):
            dArray[i] = value
            value += 0.01

        copia = cplcore.Matrix(matrix)
        r = copia.height
        c = copia.width
        print("%s" % ("Writing same value into submatrix (beyond borders)... "))
        assert matrix.fill_window(0, 2, 2, 10, 10)

        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of overwritten matrix... " % (i, j))
                if (i > 1) and (j > 1):
                    assert math.isclose(0, matrix[i][j], abs_tol=0.00001)

                else:
                    assert math.isclose(copia[i][j], matrix[i][j], abs_tol=0.00001)
        value = 9.00
        for i in range(nelem):
            dArray[i] = value
            value += 0.01

        copia = cplcore.Matrix(matrix)
        copy = cplcore.Matrix(matrix)
        r = copia.height
        c = copia.width
        print("%s" % ("Shift matrix elements by +1,0... "))
        assert matrix.shift(1, 0)

        code = TestMatrix.shift_brute(copy, 1, 0)
        # DELETED cpl_test_eq_error_macro(code, CPL_ERROR_NONE
        assert TestMatrix.isclose(matrix, copy, 0)
        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of first shifted matrix... " % (i, j))
                if i > (r - 2):
                    assert math.isclose(
                        copia[i][j], matrix[(i + 1) - r][j], abs_tol=0.00001
                    )

                else:
                    assert math.isclose(
                        copia[i][j], matrix[(i + 1) - r][j], abs_tol=0.00001
                    )

        print("%s" % ("Shift matrix elements by -1,+1... "))
        assert matrix.shift(-1, 1)

        code = TestMatrix.shift_brute(copy, -1, 1)
        # DELETED cpl_test_eq_error_macro(code, CPL_ERROR_NONE
        assert TestMatrix.isclose(matrix, copy, 0)
        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of second shifted matrix... " % (i, j))
                if j > (c - 2):
                    assert math.isclose(
                        copia[i][j], matrix[i][(j + 1) - c], abs_tol=0.00001
                    )

                else:
                    assert math.isclose(copia[i][j], matrix[i][j + 1], abs_tol=0.00001)

        print("%s" % ("Shift matrix elements by +4,+3... "))
        assert matrix.shift(4, 3)

        code = TestMatrix.shift_brute(copy, 4, 3)
        # DELETED cpl_test_eq_error_macro(code, CPL_ERROR_NONE
        assert TestMatrix.isclose(matrix, copy, 0)
        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of third shifted matrix... " % (i, j))
                if (j > (c - 5)) and (i > (r - 5)):
                    assert math.isclose(
                        copia[i][j], matrix[(i + 4) - r][(j + 4) - c], abs_tol=0.00001
                    )

                elif j > (c - 5):
                    assert math.isclose(
                        copia[i][j], matrix[i + 4][(j + 4) - c], abs_tol=0.00001
                    )

                elif i > (r - 5):
                    assert math.isclose(
                        copia[i][j], matrix[(i + 4) - r][j + 4], abs_tol=0.00001
                    )

                else:
                    assert math.isclose(
                        copia[i][j], matrix[i + 4][j + 4], abs_tol=0.00001
                    )

        print("%s" % ("Shift matrix elements by -8,-8... "))
        assert matrix.shift(-8, -8)

        code = TestMatrix.shift_brute(copy, -8, -8)
        # DELETED cpl_test_eq_error_macro(code, CPL_ERROR_NONE
        assert TestMatrix.isclose(matrix, copy, 0)
        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of fourth shifted matrix... " % (i, j))
                if (j < 4) and (i < 4):
                    assert math.isclose(
                        copia[i][j], matrix[(i - 4) + r][(j - 4) + c], abs_tol=0.00001
                    )

                elif j < 4:
                    assert math.isclose(
                        copia[i][j], matrix[i - 4][(j - 4) + c], abs_tol=0.00001
                    )

                elif i < 4:
                    assert math.isclose(
                        copia[i][j], matrix[(i - 4) + r][j - 4], abs_tol=0.00001
                    )

                else:
                    assert math.isclose(
                        copia[i][j], matrix[i - 4][j - 4], abs_tol=0.00001
                    )

        code = matrix.shift(4, 4)
        # DELETED cpl_test_eq_error_macro(code, CPL_ERROR_NONE
        code = TestMatrix.shift_brute(copy, 4, 4)
        # DELETED cpl_test_eq_error_macro(code, CPL_ERROR_NONE
        assert TestMatrix.isclose(matrix, copy, 0)
        assert TestMatrix.isclose(matrix, copia, 0)
        for i in range((-2) * matrix.height, 2 + (2 * matrix.height)):
            for j in range((-2) * matrix.width, 2 + (2 * matrix.width)):
                code = matrix.shift(i, 0)
                code = TestMatrix.shift_brute(copy, i, 0)
                # DELETED cpl_test_eq_error_macro(code, CPL_ERROR_NONE
                assert TestMatrix.isclose(matrix, copy, 0)
                code = matrix.shift(0, j)
                code = TestMatrix.shift_brute(copy, 0, j)
                # DELETED cpl_test_eq_error_macro(code, CPL_ERROR_NONE
                assert TestMatrix.isclose(matrix, copy, 0)
                code = matrix.shift(-i, -j)
                code = TestMatrix.shift_brute(copy, -i, -j)
                # DELETED cpl_test_eq_error_macro(code, CPL_ERROR_NONE
                assert TestMatrix.isclose(matrix, copy, 0)

        # INVALID         code=None.shift(1, 1)
        copia = cplcore.Matrix(matrix)
        print("%s" % ("Chop matrix... "))
        assert matrix.threshold_small(9.505)

        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of chopped matrix... " % (i, j))
                if copia[i][j] < 9.505:
                    assert math.isclose(0, matrix[i][j], abs_tol=0.00001)

                else:
                    assert math.isclose(copia[i][j], matrix[i][j], abs_tol=0.00001)

        dArray = [9 + i * 0.01 for i in range(nelem)]
        assert len(dArray) == nelem

        print("%s" % ("Creating one more test matrix... "))
        matrix = cplcore.Matrix(7, 10, dArray)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        matrix

        copia = cplcore.Matrix(matrix)
        print("%s" % ("Swap rows 1 and 3... "))
        assert matrix.swap_rows(1, 3)

        for j in range(c):
            print("Check column %ld of row swapped matrix... " % (j))
            assert math.isclose(copia[1][j], matrix[3][j], abs_tol=0.00001)
            assert math.isclose(copia[3][j], matrix[1][j], abs_tol=0.00001)

        value = 9.00
        for i in range(nelem):
            dArray[i] = value
            matrix[int(i / 10)][i % 10] = value
            value += 0.01

        print("%s" % ("Swap columns 1 and 3... "))
        assert matrix.swap_columns(1, 3)

        for i in range(r):
            print("Check row %ld of column swapped matrix... " % (i))
            assert math.isclose(copia[i][1], matrix[i][3], abs_tol=0.00001)
            assert math.isclose(copia[i][1], matrix[i][3], abs_tol=0.00001)

        value = 9.00
        for i in range(nelem):
            dArray[i] = value
            value += 0.01

        print("%s" % ("Creating 5x5 square matrix... "))
        copia = cplcore.Matrix(5, 5)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        print("%s" % ("Writing upper left 5x5 of matrix into copy..."))
        assert copia.copy(matrix, 0, 0)

        print("%s" % ("Swapping row 1 with column 1... "))
        assert copia.swap_rowcolumn(1)

        for i in range(5):
            print("Check row/column element %ld of swapped matrix... " % (i))
            assert math.isclose(matrix[i][1], copia[1][i], abs_tol=0.00001)
            assert math.isclose(matrix[i][1], copia[1][i], abs_tol=0.00001)

        copia = cplcore.Matrix(matrix)
        print("%s" % ("Flip matrix upside down... "))
        assert matrix.flip_rows()

        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of row-flipped matrix... " % (i, j))
                assert math.isclose(
                    copia[i][j], matrix[(r - i) - 1][j], abs_tol=0.00001
                )

        print("%s" % ("Flip matrix left right... "))
        assert matrix.flip_columns()

        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of row-flipped matrix... " % (i, j))
                assert math.isclose(
                    copia[i][j], matrix[(r - i) - 1][(c - j) - 1], abs_tol=0.00001
                )

        value = 9.00
        for i in range(nelem):
            dArray[i] = value
            value += 0.01

        print("%s" % ("Compute transpose... "))
        copia = matrix.transpose_create()
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of transposed matrix... " % (i, j))
                assert math.isclose(matrix[i][j], copia[j][i], abs_tol=0.00001)

        print("%s" % ("Sort matrix rows... "))
        assert copia.sort_rows(1)

        print("%s" % ("Flip matrix left right (again)... "))
        assert copia.flip_columns()

        print("%s" % ("Sort matrix columns... "))
        assert copia.sort_columns(1)

        copia = cplcore.Matrix(matrix)
        print("%s" % ("Delete rows 2 to 4... "))
        assert matrix.erase_rows(2, 3)

        k = 0
        for i in range(r):
            if (i > 1) and (i < 5):
                break

            for j in range(c):
                print("Check element %ld, %ld of row-deleted matrix... " % (k, j))
                assert math.isclose(copia[i][j], matrix[k][j], abs_tol=0.00001)

            k += 1
        print("%s" % ("Delete columns 2 to 4... "))
        assert matrix.erase_columns(2, 3)

        k = 0
        for i in range(r):
            if (i > 1) and (i < 5):
                break

            for j in range(c):
                if (j > 1) and (j < 5):
                    break

                print("Check element %ld, %ld of row-deleted matrix... " % (k, j))
                assert math.isclose(copia[i][j], matrix[k][j], abs_tol=0.00001)
                l += 1
            k += 1

        dArray = [9 + i * 0.01 for i in range(nelem)]
        assert len(dArray) == nelem

        print("%s" % ("Creating the steenth test matrix... "))
        matrix = cplcore.Matrix(7, 10, dArray)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        matrix

        copia = cplcore.Matrix(matrix)
        print("%s" % ("Cut a frame around matrix... "))
        assert matrix.resize(-2, -2, -2, -2)

        for i in range(2, r - 2):
            for j in range(2, c - 2):
                print("Check element %ld, %ld of cropped matrix... " % (i - 2, j - 2))
                assert math.isclose(matrix[i - 2][j - 2], copia[i][j], abs_tol=0.00001)

        print("%s" % ("Add a frame around matrix... "))
        assert matrix.resize(2, 2, 2, 2)

        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of expanded matrix... " % (i, j))
                if (((i > 1) and (i < (r - 2))) and (j > 1)) and (j < (c - 2)):
                    assert math.isclose(copia[i][j], matrix[i][j], abs_tol=0.00001)

                else:
                    assert math.isclose(0, matrix[i][j], abs_tol=0.00001)

        print("%s" % ("Reframe a matrix... "))
        assert matrix.resize(-2, 2, 2, -2)

        dArray = [9 + i * 0.01 for i in range(nelem)]
        assert len(dArray) == nelem

        print("%s" % ("Creating matrix for merging test... "))
        matrix = cplcore.Matrix(7, 10, dArray)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        matrix

        print("%s" % ("Creating second matrix for merging test... "))
        copia = matrix.extract(0, 0, 1, 1, 7, 1)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        product = cplcore.Matrix(copia)
        print("%s" % ("Horizontal merging... "))
        assert copia.append(matrix, 0)

        for i in range(r):
            for j in range(c):
                print(
                    "Check element %ld, %ld of horizontally merged matrix... "
                    % (i, j + 1)
                )
                assert math.isclose(matrix[i][j], copia[i][j + 1], abs_tol=0.00001)

        for i in range(r):
            print("Check element %ld, %d of horizontally merged matrix... " % (i, 0))
            assert math.isclose(product[i][0], copia[i][0], abs_tol=0.00001)

        print("%s" % ("Creating third matrix for merging test... "))
        copia = matrix.extract(0, 0, 1, 1, 4, 10)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        product = cplcore.Matrix(copia)
        print("%s" % ("Vertical merging... "))
        assert copia.append(matrix, 1)

        for i in range(4):
            for j in range(c):
                print("Check element %ld, %ld of vertically merged matrix... " % (i, j))
                assert math.isclose(product[i][j], copia[i][j], abs_tol=0.00001)

        for i in range(r):
            for j in range(c):
                print(
                    "Check element %ld, %ld of vertically merged matrix... "
                    % (i + 4, j)
                )
                assert math.isclose(matrix[i][j], copia[i + 4][j], abs_tol=0.00001)

        print("%s" % ("Create matrix for add test... "))
        copia = cplcore.Matrix(matrix)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        print("%s" % ("Adding two matrices... "))
        assert copia.add(matrix)

        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of sum matrix... " % (i, j))
                assert math.isclose(matrix[i][j] * 2, copia[i][j], abs_tol=0.00001)

        print("%s" % ("Subtracting two matrices... "))
        assert copia.subtract(matrix)

        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of diff matrix... " % (i, j))
                assert math.isclose(matrix[i][j], copia[i][j], abs_tol=0.00001)

        print("%s" % ("Multiply two matrices... "))
        assert copia.multiply(matrix)

        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of mult matrix... " % (i, j))
                assert math.isclose(
                    matrix[i][j] * matrix[i][j], copia[i][j], abs_tol=0.00001
                )

        print("%s" % ("Divide two matrices... "))
        assert copia.divide(matrix)

        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of ratio matrix... " % (i, j))
                assert math.isclose(matrix[i][j], copia[i][j], abs_tol=0.00001)

        print("%s" % ("Divide a matrix by 2... "))
        assert copia.multiply_scalar(1.0 / 2.0)

        for i in range(r):
            for j in range(c):
                print("Check element %ld, %ld of half matrix... " % (i, j))
                assert math.isclose(matrix[i][j] / 2, copia[i][j], abs_tol=0.00001)

        print("%s" % ("Creating first matrix for product... "))
        matrix = cplcore.Matrix(3, 4)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        matrix

        print("%s" % ("Creating second matrix for product... "))
        copia = cplcore.Matrix(4, 5)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        value = 0
        i = 0
        k = 0
        while i < 3:
            for j in range(4):
                k += 1
                print("Writing to element %ld of the first matrix... " % (k))
                matrix[i][j] = value
                value += 1

            i += 1
        value -= 10.0
        i = 0
        k = 0
        while i < 4:
            for j in range(5):
                k += 1
                print("Writing to element %ld of the second matrix... " % (k))
                copia[i][j] = value
                value += 1

            i += 1
        print("%s" % ("Matrix product... "))
        product = matrix.product(copia)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        product

        print("%s" % ("Check element 0, 0 of product... "))
        assert math.isclose(10.0, product[0][0], abs_tol=0.00001)

        print("%s" % ("Check element 0, 1 of product... "))
        assert math.isclose(16.0, product[0][1], abs_tol=0.00001)

        print("%s" % ("Check element 0, 2 of product... "))
        assert math.isclose(22.0, product[0][2], abs_tol=0.00001)

        print("%s" % ("Check element 0, 3 of product... "))
        assert math.isclose(28.0, product[0][3], abs_tol=0.00001)

        print("%s" % ("Check element 0, 4 of product... "))
        assert math.isclose(34.0, product[0][4], abs_tol=0.00001)

        print("%s" % ("Check element 1, 0 of product... "))
        assert math.isclose(-30.0, product[1][0], abs_tol=0.00001)

        print("%s" % ("Check element 1, 1 of product... "))
        assert math.isclose(-8.0, product[1][1], abs_tol=0.00001)

        print("%s" % ("Check element 1, 2 of product... "))
        assert math.isclose(14.0, product[1][2], abs_tol=0.00001)

        print("%s" % ("Check element 1, 3 of product... "))
        assert math.isclose(36.0, product[1][3], abs_tol=0.00001)

        print("%s" % ("Check element 1, 4 of product... "))
        assert math.isclose(58.0, product[1][4], abs_tol=0.00001)

        print("%s" % ("Check element 2, 0 of product... "))
        assert math.isclose(-70.0, product[2][0], abs_tol=0.00001)

        print("%s" % ("Check element 2, 1 of product... "))
        assert math.isclose(-32.0, product[2][1], abs_tol=0.00001)

        print("%s" % ("Check element 2, 2 of product... "))
        assert math.isclose(6.0, product[2][2], abs_tol=0.00001)

        print("%s" % ("Check element 2, 3 of product... "))
        assert math.isclose(44.0, product[2][3], abs_tol=0.00001)

        print("%s" % ("Check element 2, 4 of product... "))
        assert math.isclose(82.0, product[2][4], abs_tol=0.00001)

        print("%s" % ("Creating coefficient matrix... "))
        matrix = cplcore.Matrix(3, 3)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        matrix

        matrix[0][0] = 1
        matrix[0][1] = 1
        matrix[0][2] = 1
        matrix[1][0] = 1
        matrix[1][1] = -2
        matrix[1][2] = 2
        matrix[2][0] = 0
        matrix[2][1] = 1
        matrix[2][2] = -2
        print("%s" % ("Find min pos 2... "))
        r, c = matrix.minpos()

        print("%s" % ("Check min row pos 2..."))
        assert 1 == r

        print("%s" % ("Check min col pos 2..."))
        assert 1 == c

        print("%s" % ("Check min value 2... "))
        assert math.isclose(-2.0, matrix.min(), abs_tol=0.0001)

        print("%s" % ("Find max pos 2... "))
        r, c = matrix.maxpos()

        print("%s" % ("Check max row pos 2..."))
        assert 1 == r

        print("%s" % ("Check max col pos 2..."))
        assert 2 == c

        print("%s" % ("Check max value 2... "))
        assert math.isclose(2.0, matrix.max(), abs_tol=0.0001)

        print("%s" % ("Creating nonhomo matrix... "))
        copia = cplcore.Matrix(3, 1)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        copia[0][0] = 0
        copia[1][0] = 4
        copia[2][0] = 2
        matrix.dump()
        print("%s" % ("Compute determinant of a regular matrix"))
        assert math.isclose(5.0, matrix.determinant(), abs_tol=sys.float_info.epsilon)

        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        print("%s" % ("Solving a linear system... "))
        product = matrix.solve(copia)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        product

        print("%s" % ("Check linear system solution, x = 4... "))
        assert math.isclose(4.0, product[0][0], abs_tol=0.0001)

        print("%s" % ("Check linear system solution, y = -2... "))
        assert math.isclose(-2.0, product[1][0], abs_tol=0.0001)

        print("%s" % ("Check linear system solution, z = -2... "))
        assert math.isclose(-2.0, product[2][0], abs_tol=0.0001)

        print("%s" % ("Least squares... "))
        product = matrix.solve_normal(copia)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        product

        print("%s" % ("Creating 1x1 matrix to invert... "))
        matrix = cplcore.Matrix(1, 1)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        matrix

        matrix[0][0] = 2
        print("%s" % ("Compute determinant of a 1x1-matrix"))
        assert math.isclose(2.0, matrix.determinant(), abs_tol=0)

        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        print("%s" % ("Invert 1x1 matrix... "))
        copia = matrix.invert_create()
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        print("%s" % ("Product by 1x1 inverse... "))
        product = matrix.product(copia)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        product

        print("%s" % ("Checking if 1x1 product is identity... "))
        assert 1 == product.is_identity(-1.0)

        product = matrix.product(copia)
        # DELETED cpl_test_eq_error_macro(code, CPL_ERROR_NONE
        k = product.is_identity(-1.0)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        assert 1 == k

        print("%s" % ("Creating matrix to invert... "))
        matrix = cplcore.Matrix(2, 2)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        matrix

        matrix[0][0] = 1
        matrix[0][1] = 1
        matrix[1][0] = -1
        matrix[1][1] = 2
        matrix.dump()
        print("%s" % ("Invert matrix... "))
        copia = matrix.invert_create()
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        copia

        copia.dump()
        print("%s" % ("Product by inverse... "))
        product = matrix.product(copia)
        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        product

        product.dump()
        print("%s" % ("Checking if product is identity... "))
        assert 1 == product.is_identity(-1.0)

        assert matrix.set_size(2, 2)
        assert copia.set_size(2, 1)
        assert matrix.fill(0)
        for i in range(2):
            matrix[i][1] = 1.0

        assert copia.fill(1.0)
        matrix.dump()
        copia.dump()
        print("Try to solve a 2 X 2 singular system" % ())
        with pytest.raises(cplcore.SingularMatrixError):
            try:
                matrix.solve(copia)
            except cplcore.Error as es:
                es.raise_last()

        xtrue = cplcore.Matrix(1, 1)
        print("%s" % ("Compute determinant of a singular matrix"))
        assert math.isclose(0, matrix.determinant(), abs_tol=0)

        # DELETED cpl_test_error_macro(CPL_ERROR_NONE
        """
        Compute the determinant
        This time with an existing error code
        """
        try:
            raise cplcore.EOLError()
        except cplcore.Error as e:
            dvalzero = matrix.determinant()

            print("%s" % ("Compute determinant of a singular matrix"))
            assert math.isclose(0, dvalzero, abs_tol=0)

        print(
            "Try to solve increasingly large systems Ax=b, with A(i,j) = 1/(2*n-(1+i+j)) and x(j) = 1"
            % ()
        )
        size = subnelem
        while 1 < (nreps * nelem):
            assert xtrue.set_size(size, 1)
            assert xtrue.fill(1.0)
            assert TestMatrix.fill_illcond(matrix, size)
            product = matrix.product(xtrue)
            product
            xsolv = matrix.solve(product)
            xsolv
            x_min = xsolv.min()
            x_max = xsolv.max()
            is_done = True if (x_min < 0.5) or (x_max >= 2.0) else False
            p2 = matrix.product(xsolv)
            assert p2.subtract(product)
            residual = TestMatrix.get_2norm_1(p2)

            assert xsolv.subtract(xtrue)
            error = TestMatrix.get_2norm_1(xsolv)
            print(
                "2-norm of residual with ill-conditioned %d x %d [DBL_EPSILON]: %g"
                % (size, size, residual / sys.float_info.epsilon)
            )
            print(
                "2-norm of error on solution with ill-conditioned %d x %d : %g"
                % (size, size, error)
            )
            xsolv.dump()

            if is_done:
                print(
                    "Stopping at n=%d, because the most significant bit of at least one element in the solution is wrong, x_min=%g, x_max=%g"
                    % (size, x_min, x_max)
                )
                break

            size += 1
        print(
            "Try to solve increasingly large systems A^TAx=A^Tb, with A(i,j) = 1/(2*n-(1+i+j)) and x(j) = 1"
            % ()
        )
        k = 0
        did_fail = False
        for size in range(1, nreps * nelem):
            assert xtrue.set_size(size, 1)
            assert xtrue.fill(1.0)
            assert TestMatrix.fill_illcond(matrix, size)
            product = matrix.product(xtrue)
            product
            try:
                try:
                    xsolv = matrix.solve_normal(product)
                except cplcore.Error as es:
                    es.raise_last()
            except cplcore.SingularMatrixError:
                assert xsolv == None
                did_fail = True

                break

            xsolv
            x_min = xsolv.min()
            x_max = xsolv.max()
            is_done = True if (x_min < 0.5) or (x_max >= 2.0) else False
            p2 = matrix.product(xsolv)
            assert p2.subtract(product)
            residual = TestMatrix.get_2norm_1(p2)

            assert xsolv.subtract(xtrue)
            error = TestMatrix.get_2norm_1(xsolv)
            print(
                "2-norm of residual with ill-conditioned %d x %d (normal) [DBL_EPSILON]: %g"
                % (size, size, residual / sys.float_info.epsilon)
            )
            print(
                "2-norm of error on solution with ill-conditioned %d x %d (normal): %g"
                % (size, size, error)
            )
            xsolv.dump()

            if is_done:
                print(
                    "Stopping at n=%d, because the most significant bit of at least one element in the solution is wrong, x_min=%g, x_max=%g"
                    % (size, x_min, x_max)
                )
                break

            k += 1
        if did_fail:
            assert 6 <= k

        print(
            "Compute the determinant of increasingly large matrices, with A(i,j) = 1/(1+abs(i-j))"
            % ()
        )
        try:
            try:
                for size in range(1, nelem * nreps):
                    assert TestMatrix.fill_test(matrix, size, size)
                    value = matrix.determinant()
                    print(
                        "Determinant of %d by %d test-matrix: %g" % (size, size, value)
                    )

                print("Stopping at n=%d, with a determinant of %g" % (size, value))
            except cplcore.Error as es:
                es.raise_last()
        except cplcore.Error:
            print(
                "Stopping at n=%d, because the determinant was truncated to zero"
                % (size)
            )

        print(
            "Compute the determinant of increasingly large ill-conditioned matrices, with A(i,j) = 1/(2*n-(1+i+j))"
            % ()
        )

        size = 1
        try:
            try:
                while True:
                    assert TestMatrix.fill_illcond(matrix, size)
                    value = matrix.determinant()
                    print(
                        "Determinant of %d by %d ill-conditioned-matrix: %g"
                        % (size, size, value)
                    )

                    size += 1
            except cplcore.Error as es:
                es.raise_last()
        except cplcore.Error:
            print(
                "Stopping at n=%d, because the determinant was truncated to zero"
                % (size)
            )

    @classmethod
    def fill_illcond(cls, self, size):
        assert size > 0
        self.set_size(size, size)

        for i in range(size):
            for j in range(size):
                value = 1.0 / float((2 * size) - ((i + j) + 1))
                self[i][j] = value

    @classmethod
    def fill_test(cls, self, nr, nc):
        assert nr > 0
        assert nc > 0
        self.set_size(nr, nc)

        for i in range(nr):
            for j in range(nc):
                value = 1.0 / float(1 + abs(i - j))
                self[i][j] = value

    @classmethod
    def get_2norm_1(cls, self):
        from math import sqrt

        norm = 0
        try:
            assert self.width == 1
        except AssertionError:
            return -2.0

        for i in range(self.height):
            value = self[i][0]
            norm += value * value

        return sqrt(norm)

    @pytest.mark.parametrize("nsize", [400 if do_bench else 40])
    @pytest.mark.parametrize("n", [10 if do_bench else 1])
    def bench_solve_normal(self, nsize, n):
        xtrue = cplcore.Matrix(1, 1)
        matrix = cplcore.Matrix(1, 1)
        assert xtrue.set_size(nsize, 1)
        assert xtrue.fill(1.0)
        assert TestMatrix.fill_test(matrix, nsize, nsize)
        product = matrix.product(xtrue)
        product
        bytes = n * TestMatrix.get_bytes_cplcore.Matrix(matrix)
        secs = os.times().user
        for i in range(n):
            xsolv = matrix.solve_normal(product)
            xsolv

        secs = os.times().user - secs
        print(
            "Speed while solving with size %d X %d in %g secs [Mflop/s]: %g (%g)"
            % (
                nsize,
                nsize,
                secs / float(i),
                (float(flops0) / secs) / 1e6,
                float(flops0),
            )
        )
        if secs > 0:
            print("Processing rate [MB/s]: %g" % ((1e-6 * float(bytes)) / secs))

    @pytest.mark.parametrize("nsize", [128])
    @pytest.mark.parametrize("n", [100 if do_bench else 1])
    def bench_product_transpose(self, nsize, n):
        inp = cplcore.Matrix(nsize, nsize)
        out = cplcore.Matrix(nsize, nsize)
        bytes = n * TestMatrix.get_bytes_cplcore.Matrix(inp)
        inp.fill(1.0)
        secs = os.times().user
        for i in range(n):
            error = out.product_transpose(inp, inp)
            # DELETED cpl_test_eq_error_macro(error, CPL_ERROR_NONE

        secs = os.times().user - secs
        print(
            "Time spent transposing %d %dx%d matrices [s]: %g" % (n, nsize, nsize, secs)
        )
        if secs > 0:
            print("Processing rate [MB/s]: %g" % ((1e-6 * float(bytes)) / secs))

    @classmethod
    def product_bilinear_brute(cls, A, B):
        C = cplcore.Matrix(B.height, B.width)
        error = C.product_transpose(A, B)
        self = B.product(C)
        return self

    @classmethod
    def shift_brute(cls, matrix, rshift, cshift):
        nr = matrix.height
        nc = matrix.width

        assert matrix is not None

        rshift = rshift % nr
        cshift = cshift % nc
        if (rshift != 0) or (cshift != 0):
            copy = cplcore.Matrix(matrix)
            if rshift < 0:
                rshift += nr

            if cshift < 0:
                cshift += nc

            matrix.copy(copy, rshift, cshift)
            if rshift:
                matrix.copy(copy, rshift - nr, cshift)

            if cshift:
                matrix.copy(copy, rshift, cshift - nc)

            if rshift and cshift:
                matrix.copy(copy, rshift - nr, cshift - nc)

    @classmethod
    def product_bilinear_impl(cls, nr, nc):
        self = cplcore.Matrix(nr, nr)
        jacobi = cplcore.Matrix(nr, nc)
        matrix = cplcore.Matrix(nc, nc)
        TestMatrix.fill_test(jacobi, nr, nc)
        # DELETED cpl_test_eq_error_macro(error, CPL_ERROR_NONE
        TestMatrix.fill_test(matrix, nc, nc)
        # DELETED cpl_test_eq_error_macro(error, CPL_ERROR_NONE
        # f0 is unused
        # f0=0
        t0 = os.times().user
        self.product_bilinear(matrix, jacobi)
        tfunc = os.times().user - t0
        ffunc = 0
        # DELETED cpl_test_eq_error_macro(error, CPL_ERROR_NONE
        # f0=0
        t0 = os.times().user
        brute = TestMatrix.product_bilinear_brute(matrix, jacobi)
        tbrute = os.times().user - t0
        fbrute = 0

        return ffunc, fbrute, self, brute, tfunc, tbrute

    """
        for i in range(1,11 + 1):
            for j in range(1,11 + 1):
                TestMatrix.product_bilinear_test(i, j, False)

        if do_bench:
            TestMatrix.product_bilinear_test(100, 500, True)
            TestMatrix.product_bilinear_test(500, 100, True)
            TestMatrix.product_bilinear_test(500, 500, True)
    """

    @pytest.mark.parametrize("nr", range(1, 12))
    @pytest.mark.parametrize("nc", range(1, 12))
    def product_bilinear(self, nr, nc):
        ffunc, fbrute, selfm, brute, tfunc, tbrute = TestMatrix.product_bilinear_impl(
            nr, nc
        )

        assert ffunc == fbrute
        assert TestMatrix.isclose(self, brute, 250.0 * sys.float_info.epsilon)

    @pytest.mark.parametrize("nr", [100, 500])
    @pytest.mark.parametrize("nc", [100, 500])
    def bench_product_bilinear(self, nr, nc):
        ffunc, fbrute, selfm, brute, tfunc, tbrute = TestMatrix.product_bilinear_impl(
            nr, nc
        )

        assert ffunc == fbrute
        assert TestMatrix.isclose(self, brute, 250.0 * sys.float_info.epsilon)

        print(
            "Time spent on B * A * B', B: %ldx%ld [s]: %g <=> %g (brute force)"
            % (nr, nc, tfunc, tbrute)
        )
        if (tfunc > 0) and (tbrute > 0):
            print(
                "Speed while computing B * A * B', B: %ldx%ld [Mflop/s]: %g <=> %g (brute force)"
                % (nr, nc, (float(ffunc) / tfunc) / 1e6, (float(fbrute) / tbrute) / 1e6)
            )

    """
    TestMatrix.test_banded(10, 1)
    TestMatrix.test_banded(10, 2)
    if do_bench:
        TestMatrix.test_banded(400, 10)
        TestMatrix.test_banded(1000, 2)
    """

    @pytest.mark.parametrize(
        "args", [[10, 1], [10, 2], *([[400, 10], [1000, 2]] if do_bench else [])]
    )
    def test_banded(self, args):
        nc, np = args
        A = cplcore.Matrix.zeros(nc, nc)
        p = cplcore.Matrix.zeros(nc, nc)
        for i in range(-np, np + 1):
            A.fill_diagonal(1.0, i)
            # DELETED cpl_test_eq_error_macro(error, CPL_ERROR_NONE

        t0 = os.times().user
        p, i = A.decomp_lu()
        tfunc = os.times().user - t0
        # DELETED cpl_test_eq_error_macro(error, CPL_ERROR_NONE

        # print("Time spent on LU-decomposition: %d X %d [s]: %g" % (nc, nc, tfunc))
        # if tfunc > 0:
        #     print("Speed while computing LU-decomposition: %d X %d [Mflop/s]: %g" % (nc, nc, (float(ffunc) / tfunc) / 1e6))
