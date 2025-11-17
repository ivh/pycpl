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

import numpy as np
import pytest

import cpl

CPL_PHYS_Wien = 2.8977685e-3
CPL_PHYS_C = 299792458.0
CPL_PHYS_Wien_Freq = 879e10
WL_MIN = 2e-6
WL_MAX = 32e-6
FUNCTION_SIZE = 1024
temp = 285
half_search = FUNCTION_SIZE // 2


def fill_blackbody_test(out_unit, evalpoints, in_unit, temp):
    """
    Based off cpl_photom_fill_blackbody_test from cpl_photom-test.c

    evalpoints must be strictly monotone, increasing for wavelengths,
    decreasing for frequencies.
    """
    print(evalpoints)
    spectrum = cpl.drs.photom.fill_blackbody(out_unit, evalpoints, in_unit, temp)
    assert 0.0 < spectrum[0]
    if out_unit == cpl.drs.photom.Unit.ENERGYRADIANCE:
        if in_unit == cpl.drs.photom.Unit.LENGTH:
            xtpt = CPL_PHYS_Wien / temp  # wavelength
            for i in range(1, spectrum.size):
                # Frequencies must increase
                assert evalpoints[i - 1] < evalpoints[i]
                if evalpoints[i] < xtpt:
                    # Must be monotonely increasing
                    spectrum[i - 1] < spectrum[i]
                elif i < spectrum.size - 1:
                    # Must be monotonely decreasing
                    spectrum[i + 1] < spectrum[i]
        elif in_unit == cpl.drs.photom.Unit.FREQUENCY:
            xtpt = temp * CPL_PHYS_Wien_Freq  # Frequency
            for i in range(1, spectrum.size):
                # Frequencies must increase
                assert evalpoints[i] < evalpoints[i - 1]
                if evalpoints[i] < xtpt:
                    # Must be monotonely increasing
                    spectrum[i - 1] < spectrum[i]
                elif i < spectrum.size - 1:
                    # Must be monotonely decreasing
                    spectrum[i + 1] < spectrum[i]
    if out_unit == cpl.drs.photom.Unit.PHOTONRADIANCE:
        sp = spectrum
        wl = evalpoints
        has_peak = False  # Find and use known peak
        if in_unit == cpl.drs.photom.Unit.LENGTH:
            assert 0.0 < wl[0]
            for i in range(1, spectrum.size):
                assert wl[i - 1] < wl[i]
                if sp[i - 1] < sp[i]:
                    assert not has_peak
                elif not has_peak:
                    has_peak = True
                else:
                    # Must be monotonely decreasing
                    assert sp[i] < sp[i - 1]
        elif in_unit == cpl.drs.photom.Unit.FREQUENCY:
            # Frequencies must decrease
            for i in range(1, spectrum.size):
                assert wl[i] < wl[i - 1]
                if sp[i - 1] < sp[i]:
                    assert not has_peak
                elif not has_peak:
                    has_peak = True  # found peak
                else:
                    # Must be monotonely decreasing
                    assert sp[i] < sp[i - 1]
            assert 0.0 < wl[spectrum.size - 1]
    return spectrum  # Return resulting vector


@pytest.fixture
def wavelengths():
    return cpl.core.Vector(np.linspace(start=WL_MIN, stop=WL_MAX, num=FUNCTION_SIZE))


class TestPhotom:
    # Does not test Unsupported mode and Illegal inputs errors since they are impossible to get in this version of PyCPL:
    # Unsupported mode enums are simply not bound as they do not have any purpose in other parts of PyCPL
    # Incompatible input cannot happen as PyCPL allocates the output vector for you and thus cannot differ from evalpoints
    def test_fill_blackbody_photonradiance(self, wavelengths):
        fill_blackbody_test(
            cpl.drs.photom.Unit.PHOTONRADIANCE,
            wavelengths,
            cpl.drs.photom.Unit.LENGTH,
            temp,
        )

    def test_fill_blackbody_energyradiance(self, wavelengths):
        fill_blackbody_test(
            cpl.drs.photom.Unit.ENERGYRADIANCE,
            wavelengths,
            cpl.drs.photom.Unit.LENGTH,
            temp,
        )

    def test_fill_blackbody_less(self, wavelengths):
        fill_blackbody_test(
            cpl.drs.photom.Unit.LESS, wavelengths, cpl.drs.photom.Unit.LENGTH, temp
        )

    def test_vxc(self, wavelengths):
        # Run previous tests but store the results
        energyradiance_result = fill_blackbody_test(
            cpl.drs.photom.Unit.ENERGYRADIANCE,
            wavelengths,
            cpl.drs.photom.Unit.LENGTH,
            temp,
        )
        less_result = fill_blackbody_test(
            cpl.drs.photom.Unit.LESS, wavelengths, cpl.drs.photom.Unit.LENGTH, temp
        )
        vxc, delta = cpl.core.Vector.correlate(
            less_result, energyradiance_result, half_search
        )
        assert delta == half_search
        assert np.isclose(
            vxc[int(half_search)], 1.0, atol=np.finfo(np.double).eps * FUNCTION_SIZE
        )

    def test_frequency_mode_photonradiance(self, wavelengths):
        frequencies = cpl.core.Vector(
            [CPL_PHYS_C / wavelength for wavelength in wavelengths]
        )
        fill_blackbody_test(
            cpl.drs.photom.Unit.PHOTONRADIANCE,
            frequencies,
            cpl.drs.photom.Unit.FREQUENCY,
            temp,
        )

    def test_frequency_mode_energyradiance(self, wavelengths):
        frequencies = cpl.core.Vector(
            [CPL_PHYS_C / wavelength for wavelength in wavelengths]
        )
        fill_blackbody_test(
            cpl.drs.photom.Unit.ENERGYRADIANCE,
            frequencies,
            cpl.drs.photom.Unit.FREQUENCY,
            temp,
        )

    def test_incompatible_illegal_input_temp(self, wavelengths):
        with pytest.raises(cpl.core.IllegalInputError):
            fill_blackbody_test(
                cpl.drs.photom.Unit.LESS, wavelengths, cpl.drs.photom.Unit.LENGTH, 0.0
            )

    def test_incompatible_illegal_input_wavelength(self, wavelengths):
        wavelengths[0] = 0.0
        with pytest.raises(cpl.core.IllegalInputError):
            fill_blackbody_test(
                cpl.drs.photom.Unit.LESS, wavelengths, cpl.drs.photom.Unit.LENGTH, temp
            )
