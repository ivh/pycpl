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

import numpy as np
import pytest

import cpl.core
import cpl.drs

CPL_WLCALIB_SPC_SIZE = 100

coeffs = [
    1.0,
    2.0 / CPL_WLCALIB_SPC_SIZE,
    3.0 / (CPL_WLCALIB_SPC_SIZE * CPL_WLCALIB_SPC_SIZE),
]


dtrue = cpl.core.Polynomial(1)
dcand = cpl.core.Polynomial(1)

nsamples = 25
hsize = 6

for i, coeff in enumerate(coeffs):
    dtrue.set_coeff([i], coeff)
    dcand.set_coeff([i], coeff * 0.95)


@pytest.fixture(scope="session", autouse=True)
def slitmodel_instance(request):
    linepos = [
        0.2 * CPL_WLCALIB_SPC_SIZE,
        0.24 * CPL_WLCALIB_SPC_SIZE,
        0.5 * CPL_WLCALIB_SPC_SIZE,
        0.75 * CPL_WLCALIB_SPC_SIZE,
        0.82 * CPL_WLCALIB_SPC_SIZE,
    ]
    lines = cpl.core.Bivector(
        ([dtrue.eval_1d(pos)[0] for pos in linepos], [1.0, 1.0, 2.0, 3.0, 4.0])
    )
    return cpl.drs.wlcalib.SlitModel(
        lines, 4.0, 3.0, CPL_WLCALIB_SPC_SIZE, threshold=5.0
    )


class TestWlcalib:
    def test_failure(self, slitmodel_instance):
        # Test 1. Failure test: 1st guess is non-monotone
        wl_search = cpl.core.Vector([0.20] * len(coeffs))
        observed = slitmodel_instance.fill_line_spectrum(dtrue)
        dfind = copy.deepcopy(dtrue)
        dfind.set_coeff([len(coeffs) - 1], -coeffs[len(coeffs) - 1])
        with pytest.raises(cpl.core.DataNotFoundError):
            _ = slitmodel_instance.find_best_1d(
                observed,
                wl_search,
                nsamples,
                0,
                cpl.drs.wlcalib.SlitModel.fill_line_spectrum,
                guess=dfind,
            )

    def test_fit_1st_correct(self, slitmodel_instance):
        # Test 2. "Dummy" test: 1st guess is the correct solution
        observed = slitmodel_instance.fill_line_spectrum(dtrue)
        wl_search = cpl.core.Vector([0.20] * len(coeffs))
        # First dummy test
        best_1d = slitmodel_instance.find_best_1d(
            observed,
            wl_search,
            2 * nsamples,
            0,
            cpl.drs.wlcalib.SlitModel.fill_line_spectrum,
            guess=dtrue,
        )
        assert best_1d.xcmax <= 1.0
        assert 1.0 - np.finfo(np.float32).eps <= best_1d.xcmax
        assert best_1d.result.compare(dtrue, np.finfo(np.float32).eps) == 0

    def test_fit_1d_1st_guess_alteration(self, slitmodel_instance):
        # Test 3. 1st guess is an alteration of the correct solution
        observed = slitmodel_instance.fill_line_spectrum(dtrue)
        wl_search = cpl.core.Vector([0.20] * len(coeffs))

        best_1d = slitmodel_instance.find_best_1d(
            observed,
            wl_search,
            nsamples,
            hsize,
            cpl.drs.wlcalib.SlitModel.fill_line_spectrum,
            guess=dcand,
        )

        assert best_1d.xcmax == best_1d.xcorrs.max()
        assert best_1d.xcmax <= 1.0
        assert 0.98 <= best_1d.xcmax
        assert best_1d.result.compare(dtrue, 0.1) == 0
