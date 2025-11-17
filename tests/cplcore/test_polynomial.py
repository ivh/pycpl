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

import copy
import platform
from random import random

import numpy as np
import pytest
import subprocess

from cpl import core as cplcore


@pytest.fixture
def fit_cmp(request):
    def _fit_cmp(
        poly, samppos, fitvals, dimdeg, maxdeg, sampsym=None, fitsigm=None, mindeg=None
    ):
        "Fit and compare to a higher dimension fit with a zero-degree"
        poly.fit(samppos, fitvals, dimdeg, maxdeg, sampsym, fitsigm, mindeg)
        mdim = poly.dimension
        ndim = 1 + mdim
        self1p = cplcore.Polynomial(ndim)

        if samppos and (mdim == 1 or dimdeg):
            zeropol = cplcore.Polynomial(mdim)
            samppos1p = cplcore.Matrix.zeros(1 + samppos.height, samppos.width)
            sampsym1p = [False] * ndim if sampsym else None
            mindeg1p = [0] * ndim if mindeg else None
            maxdeg1p = [0] * ndim if maxdeg else None
            for idim in range(ndim):
                # First copy all rows to new matrix, leaving one with zeroes
                for j in range(idim):
                    if j <= samppos.height:
                        for i in range(samppos.width):
                            samppos1p[j][i] = samppos[j][i]
                    if sampsym1p:
                        sampsym1p[j] = sampsym[j]
                    if mindeg1p:
                        mindeg1p[j] = mindeg[j]
                    if maxdeg1p:
                        maxdeg1p[j] = maxdeg[j]
                if idim <= samppos.height:
                    for i in range(samppos.width):
                        samppos1p[idim][i] = 0.0
                if sampsym1p is not None:
                    sampsym1p[idim] = True
                if mindeg1p is not None:
                    mindeg1p[idim] = 0
                if maxdeg1p is not None:
                    maxdeg1p[idim] = 0
                j = idim + 1
                while j < ndim:
                    if j <= samppos.height:
                        for i in range(samppos.width):
                            samppos1p[j][i] = samppos[j - 1][i]
                    if sampsym1p is not None:
                        sampsym1p[j] = sampsym[j - 1]
                    if mindeg1p is not None:
                        mindeg1p[j] = mindeg[j - 1]
                    if maxdeg1p is not None:
                        maxdeg1p[j] = maxdeg[j - 1]
                    j += 1
                if ndim > 3:
                    with pytest.raises(cplcore.UnsupportedModeError):
                        self1p.fit(
                            samppos1p,
                            fitvals,
                            True,
                            maxdeg1p,
                            sampsym=sampsym1p,
                            fitsigm=fitsigm,
                            mindeg=mindeg1p,
                        )
                elif ndim < 3:
                    degree = poly.degree
                    # TODO: if (!code && error1p == CPL_ERROR_SINGULAR_MATRIX && 1 + degree == samppos.width && samppos.width >= 20

                    # No error
                    self1p.fit(
                        samppos1p,
                        fitvals,
                        True,
                        maxdeg1p,
                        sampsym=sampsym1p,
                        fitsigm=fitsigm,
                        mindeg=mindeg1p,
                    )
                    k0 = poly.get_coeff([0])
                    self0p = self1p.extract(idim, zeropol)
                    if mindeg is not None:
                        if mindeg[0] > 0:
                            mytol = 6.0
                    else:
                        mytol = 1.0
                    mytol = mindeg is not None

                    assert (
                        poly.compare(
                            self0p,
                            max(np.fabs(k0), 1)
                            * pow(10.0, degree)
                            * mytol
                            * np.finfo(np.double).eps,
                        )
                        == 0
                    )

                    # TODO: The rest of the test from line 3419, if mindeg!= NULL which tests fit_1d

    return _fit_cmp


@pytest.fixture
def vector_get_mse(request):
    def _vector_get_mse(fitvals, fit, samppos):
        """
        Get the mean squared error from a vector of residuals
        Parameters
        ----------
        """
        residual = cplcore.Vector.zeros(int(1 + len(fitvals) / 2))
        residual, redchisq = fit.fit_residual(fitvals, samppos)
        mse = residual.product(residual) / len(fitvals)
        return mse, redchisq

    return _vector_get_mse


class TestPolynomial:
    def test_2d_numpy_equiv(self):
        import numpy

        # P(x, y) = 4 + 1x + 7y + 10xy + 3x² + 2xy³
        #         = (4 + 1x + 3x²) + (7 + 10x)y + (2x)y³

        np_term1_coeff = numpy.polynomial.Polynomial([4, 1, 3])  # P(x) = 4 + 1x + 3x²
        np_term2_coeff = numpy.polynomial.Polynomial([7, 10])  # P(x) = 7 + 10x
        np_term3_coeff = numpy.polynomial.Polynomial([0, 2])  # P(x) = 2x

        np_poly = numpy.polynomial.Polynomial(
            [np_term1_coeff, np_term2_coeff, 0, np_term3_coeff]
        )

        cpl_poly = cplcore.Polynomial(2)  # 2-dimensional
        cpl_poly.set_coeff([0, 0], 4)
        cpl_poly.set_coeff([0, 1], 1)
        cpl_poly.set_coeff([0, 2], 3)

        cpl_poly.set_coeff([1, 0], 7)
        cpl_poly.set_coeff([1, 1], 10)

        cpl_poly.set_coeff([3, 1], 2)

        # P(6,-8) = (4 + 6 + 108) + -8(7 + 60) + -512(12)
        #         = 118 - 536 - 6144
        #         = -6562

        assert cpl_poly.eval(cplcore.Vector([-8, 6])) == -6562
        assert np_poly(-8)(6) == -6562

    def test_numpy_conversion(self):
        import numpy

        # P(x, y) = 4 + 1x + 7y + 10xy + 3x² + 2xy³
        #         = (4 + 1x + 3x²) + (7 + 10x)y + (2x)y³

        np_term1_coeff = numpy.polynomial.Polynomial([4, 1, 3])  # P(x) = 4 + 1x + 3x²
        np_term2_coeff = numpy.polynomial.Polynomial([7, 10])  # P(x) = 7 + 10x
        np_term3_coeff = numpy.polynomial.Polynomial([0, 2])  # P(x) = 2x

        np_poly = numpy.polynomial.Polynomial(
            [np_term1_coeff, np_term2_coeff, 0, np_term3_coeff]
        )

        assert cplcore.Polynomial.from_numpy(np_poly).eval([-8, 6]) == -6562

    @pytest.mark.parametrize(
        "evalh_true, xy",
        (
            [
                (-8.17776e-05, [1e3, 1e2]),  # Evaluate at a specific point, at a root
                (-8176, [1e5, 1e0]),  # evaluate at another root
                (15.999, [2.0, 3.0]),  # not near a root
                (1.28e17, [2e4, 1e0]),
            ]  # closer to a root
            if not (
                platform.system() == "Darwin" and platform.processor().startswith("arm")
            )  # Currently m1 chips report different results due to double and long double having the same precision
            else [
                (0.0, [1e3, 1e2]),  # Evaluate at a specific point, at a root
                (0.0, [1e5, 1e0]),  # evaluate at another root
                (15.999, [2.0, 3.0]),  # not near a root
                (1.28e17, [2e4, 1e0]),
            ]
        ),
    )
    def test_eval_2d(self, evalh_true, xy):
        # Test the evaluation functions in cpl_polynomial
        poly2d = cplcore.Polynomial(2)
        poly2d.set_coeff([4, 0], 1.0)
        poly2d.set_coeff([5, 1], -1e-5)
        # evaluate at a specific point, at a root

        evalh = poly2d.eval(cplcore.Vector(xy))
        assert np.isclose(evalh, evalh_true, atol=np.finfo(np.double).eps)

    # based off cpl_polynomial_eval_2d_test from cpl_polynomial-test.c
    def tests_eval_2d_empty(self):
        poly = cplcore.Polynomial(2)
        # test empty polynomial
        res = poly.eval_2d(123.0, 123.0)[0]
        assert np.isclose(res, 0.0, np.finfo(np.double).eps)

    def tests_eval_2d_0_degree(self):
        poly = cplcore.Polynomial(2)
        poly.set_coeff([0, 0], 123)
        # test degree 0 polynomial
        res = poly.eval_2d(123.0, 123.0)[0]
        assert np.isclose(res, 123.0, np.finfo(np.double).eps)

    def test_eval_2d_random(self):
        mytol = 1e-7
        poly = cplcore.Polynomial(2)
        # Set random coefficients
        # CPL tests use a random number between 0 and 1
        poly.set_coeff([0, 1], random())
        poly.set_coeff([1, 0], random())
        poly.set_coeff([1, 1], random())
        poly.set_coeff([2, 0], random())
        poly.set_coeff([0, 3], random())

        # eval with generic function

        eval_xy = [random(), random()]

        stable_eval = poly.eval(cplcore.Vector(eval_xy))
        eval_res = poly.eval_2d(eval_xy[0], eval_xy[1])
        assert np.isclose(eval_res[0], stable_eval, atol=mytol)
        gradient = eval_res[1]

        # test gradient
        dev_poly = copy.deepcopy(poly)
        dev_poly.derivative(0)
        stable_xpd = dev_poly.eval(cplcore.Vector(eval_xy))
        assert np.isclose(stable_xpd, gradient[0], atol=mytol)
        dev_poly = copy.deepcopy(poly)
        dev_poly.derivative(1)
        stable_ypd = dev_poly.eval(cplcore.Vector(eval_xy))
        assert np.isclose(stable_ypd, gradient[1], atol=mytol)

    # based off cpl_polynomial_eval_3d_test from cpl_polynomial-test.c
    def tests_eval_3d_empty(self):
        poly = cplcore.Polynomial(3)
        # test empty polynomial
        res = poly.eval_3d(123.0, 123.0, 123.0)[0]
        assert np.isclose(res, 0.0, np.finfo(np.double).eps)

    def tests_eval_3d_0_degree(self):
        poly = cplcore.Polynomial(3)
        poly.set_coeff([0, 0, 0], 123)
        # test degree 0 polynomial
        res = poly.eval_3d(123.0, 123.0, 123.0)[0]
        assert np.isclose(res, 123.0, np.finfo(np.double).eps)

    def test_eval_3d_random(self):
        mytol = 1e-7
        poly = cplcore.Polynomial(3)
        # Set random coefficients
        # CPL tests use a random number between 0 and 1
        poly.set_coeff([0, 1, 0], random())
        poly.set_coeff([1, 0, 2], random())
        poly.set_coeff([1, 1, 3], random())
        poly.set_coeff([2, 0, 1], random())
        poly.set_coeff([0, 3, 0], random())
        poly.set_coeff([4, 0, 4], random())

        # eval with generic function

        eval_xyz = [random(), random(), random()]

        stable_eval = poly.eval(cplcore.Vector(eval_xyz))
        eval_res = poly.eval_3d(eval_xyz[0], eval_xyz[1], eval_xyz[2])
        assert np.isclose(eval_res[0], stable_eval, atol=mytol)
        gradient = eval_res[1]

        # test gradient
        dev_poly = copy.deepcopy(poly)
        dev_poly.derivative(0)
        stable_xpd = dev_poly.eval(cplcore.Vector(eval_xyz))
        assert np.isclose(stable_xpd, gradient[0], atol=mytol)
        dev_poly = copy.deepcopy(poly)
        dev_poly.derivative(1)
        stable_ypd = dev_poly.eval(cplcore.Vector(eval_xyz))
        assert np.isclose(stable_ypd, gradient[1], atol=mytol)

        dev_poly = copy.deepcopy(poly)
        dev_poly.derivative(2)
        stable_zpd = dev_poly.eval(cplcore.Vector(eval_xyz))
        assert np.isclose(stable_zpd, gradient[2], atol=mytol)

    def test_fit_residual(self, fit_cmp, vector_get_mse):
        samppos1 = cplcore.Matrix([1, 3, 5, 2, 4, 6], 1)
        taylor = cplcore.Vector([0, 2 * 4, 2 * 20, 2 * 1, 2 * 10, 2 * 35])
        poly1a = cplcore.Polynomial(1)
        # Unordered insertion
        fit_cmp(poly1a, samppos1, taylor, False, [3], mindeg=[0])
        eps, redchisq = vector_get_mse(taylor, poly1a, samppos1)
        assert 0.0 <= redchisq
        assert np.isclose(
            eps, 0.0, atol=4359 * np.finfo(np.double).eps * np.finfo(np.double).eps
        )

    def test_repr(self):
        empty = cplcore.Polynomial(1)
        assert repr(empty) == """<cpl.core.Polynomial, degree 0>"""
        np_term1_coeff = np.polynomial.Polynomial([4, 1, 3])  # P(x) = 4 + 1x + 3x²
        np_poly = np.polynomial.Polynomial([np_term1_coeff])
        cpl_poly = cplcore.Polynomial.from_numpy(np_poly)
        assert repr(cpl_poly) == """<cpl.core.Polynomial, degree 2>"""

    def test_dump_string(self):
        # P(x, y) = 4 + 1x + 7y + 10xy + 3x² + 2xy³
        #         = (4 + 1x + 3x²) + (7 + 10x)y + (2x)y³

        np_term1_coeff = np.polynomial.Polynomial([4, 1, 3])  # P(x) = 4 + 1x + 3x²
        np_poly = np.polynomial.Polynomial([np_term1_coeff])
        assert cplcore.Polynomial.from_numpy(np_poly).eval([-8, 6]) == 118
        testpoly = cplcore.Polynomial.from_numpy(np_poly)
        outp = """#----- 2 dimensional polynomial of degree 2 -----
1.dim.power  2.dim.power  coefficient
      0            0      4
      0            1      1
      0            2      3
#----- 3 coefficient(s) -----
#------------------------------------
"""
        assert str(testpoly) == outp
        assert isinstance(testpoly.dump(show=False), str)

    def test_dump_file(self, tmp_path):
        d = tmp_path / "sub"
        d.mkdir()
        p = d / "cpl_polynomial_dump.txt"
        filename = tmp_path.joinpath(p)
        np_term1_coeff = np.polynomial.Polynomial([4, 1, 3])  # P(x) = 4 + 1x + 3x²
        np_poly = np.polynomial.Polynomial([np_term1_coeff])
        testpoly = cplcore.Polynomial.from_numpy(np_poly)
        testpoly.dump(filename=str(filename))
        outp = """#----- 2 dimensional polynomial of degree 2 -----
1.dim.power  2.dim.power  coefficient
      0            0      4
      0            1      1
      0            2      3
#----- 3 coefficient(s) -----
#------------------------------------
"""
        contents = ""
        with open(str(filename), "r") as f:
            for line in f.readlines():
                contents += line
        assert contents == outp

    def test_dump_stdout(self):
        cmd = "from cpl import core as cplcore; "
        cmd += "import numpy as np; "
        cmd += "np_term1_coeff = np.polynomial.Polynomial([4, 1, 3]); "
        cmd += "np_poly = np.polynomial.Polynomial([np_term1_coeff]); "
        cmd += "testpoly = cplcore.Polynomial.from_numpy(np_poly); "
        cmd += "testpoly.dump() "
        response = subprocess.run(["python", "-c", cmd], stdout=subprocess.PIPE)
        outp = str(response.stdout, "utf-8")
        expect = """#----- 2 dimensional polynomial of degree 2 -----
1.dim.power  2.dim.power  coefficient
      0            0      4
      0            1      1
      0            2      3
#----- 3 coefficient(s) -----
#------------------------------------
"""
        assert outp == expect

    def test_copy(self):
        poly = cplcore.Polynomial(2)
        poly.set_coeff([0, 0], random())
        poly.set_coeff([0, 1], random())
        poly.set_coeff([1, 0], random())
        poly.set_coeff([1, 1], random())
        poly.set_coeff([2, 0], random())
        poly.set_coeff([0, 2], random())

        poly_copy = poly.copy()

        assert poly_copy is not poly
        assert poly_copy.dimension == poly.dimension
        assert poly_copy.degree == poly.degree
        assert poly_copy.get_coeff([0, 0]) == poly.get_coeff([0, 0])
        assert poly_copy.get_coeff([0, 1]) == poly.get_coeff([0, 1])
        assert poly_copy.get_coeff([1, 0]) == poly.get_coeff([1, 0])
        assert poly_copy.get_coeff([1, 1]) == poly.get_coeff([1, 1])
        assert poly_copy.get_coeff([2, 0]) == poly.get_coeff([2, 0])
        assert poly_copy.get_coeff([0, 2]) == poly.get_coeff([0, 2])
