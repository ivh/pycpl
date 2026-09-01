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

"""
Test module for HDRL DAR (Differential Atmospheric Refraction) bindings.

This module tests the Python bindings for the HDRL DAR functionality,
based on the C unit tests in hdrl_dar-test.c.
"""

import math

import hdrl.core as hdrlcore
import hdrl.func as hdrlfunc
import pytest
from cpl import core as cplcore
from cpl import drs as cpldrs


def create_test_wcs():
    """Create a test WCS object following the C test pattern exactly."""
    # Create property list with exact values from C test
    pl = cplcore.PropertyList()

    pl.append(cplcore.Property("NAXIS", cplcore.Type.INT, 2))
    pl.append(cplcore.Property("NAXIS1", cplcore.Type.INT, 128))
    pl.append(cplcore.Property("NAXIS2", cplcore.Type.INT, 128))

    # String properties (CTYPE) - exact values from C test
    pl.append(cplcore.Property("CTYPE1", cplcore.Type.STRING, "RA---TAN"))
    pl.append(cplcore.Property("CTYPE2", cplcore.Type.STRING, "DEC--TAN"))

    # Double properties (CRVAL, CRPIX, CD matrix) - exact values from C test
    pl.append(cplcore.Property("CRVAL1", cplcore.Type.DOUBLE, 30.0))
    pl.append(cplcore.Property("CRVAL2", cplcore.Type.DOUBLE, 12.0))
    pl.append(cplcore.Property("CRPIX1", cplcore.Type.DOUBLE, 8.0))
    pl.append(cplcore.Property("CRPIX2", cplcore.Type.DOUBLE, 8.0))
    pl.append(cplcore.Property("CD1_1", cplcore.Type.DOUBLE, -0.2 / 3600.0))
    pl.append(cplcore.Property("CD1_2", cplcore.Type.DOUBLE, 0.0))
    pl.append(cplcore.Property("CD2_1", cplcore.Type.DOUBLE, 0.0))
    pl.append(cplcore.Property("CD2_2", cplcore.Type.DOUBLE, 0.2 / 3600.0))

    # String properties (CUNIT)
    pl.append(cplcore.Property("CUNIT1", cplcore.Type.STRING, "deg"))
    pl.append(cplcore.Property("CUNIT2", cplcore.Type.STRING, "deg"))

    # Integer properties (WCSAXES)
    pl.append(cplcore.Property("WCSAXES", cplcore.Type.INT, 2))

    # Create WCS from property list
    return cpldrs.WCS(pl)


def create_test_wcs_fake():
    """Create a test WCS object with deformed scales for testing hdrl_dar_wcs_get_scales."""
    # Create property list with exact values from C test for deformed scales
    pl = cplcore.PropertyList()

    # String properties (CTYPE) - exact values from C test
    pl.append(cplcore.Property("CTYPE1", cplcore.Type.STRING, "RA---ZPN"))
    pl.append(cplcore.Property("CTYPE2", cplcore.Type.STRING, "DEC--ZPN"))

    # Double properties (CRVAL, CRPIX, CD matrix, PV) - exact values from C test
    pl.append(cplcore.Property("CRVAL1", cplcore.Type.DOUBLE, 5.57368333333))
    pl.append(cplcore.Property("CRVAL2", cplcore.Type.DOUBLE, -72.0576388889))
    pl.append(cplcore.Property("CRPIX1", cplcore.Type.DOUBLE, 5401.6))
    pl.append(cplcore.Property("CRPIX2", cplcore.Type.DOUBLE, 6860.8))
    pl.append(cplcore.Property("CD1_1", cplcore.Type.DOUBLE, 5.81347849634012e-21))
    pl.append(cplcore.Property("CD1_2", cplcore.Type.DOUBLE, 9.49444444444444e-05))
    pl.append(cplcore.Property("CD2_1", cplcore.Type.DOUBLE, -9.49444444444444e-05))
    pl.append(cplcore.Property("CD2_2", cplcore.Type.DOUBLE, -5.81347849634012e-21))
    pl.append(cplcore.Property("PV2_1", cplcore.Type.DOUBLE, 1.0))
    pl.append(cplcore.Property("PV2_2", cplcore.Type.DOUBLE, 0.0))
    pl.append(cplcore.Property("PV2_3", cplcore.Type.DOUBLE, 42.0))
    pl.append(cplcore.Property("PV2_4", cplcore.Type.DOUBLE, 0.0))
    pl.append(cplcore.Property("PV2_5", cplcore.Type.DOUBLE, 0.0))

    # Integer properties (NAXIS) - exact values from C test
    pl.append(cplcore.Property("NAXIS", cplcore.Type.INT, 2))
    pl.append(cplcore.Property("NAXIS1", cplcore.Type.INT, 2048))
    pl.append(cplcore.Property("NAXIS2", cplcore.Type.INT, 2048))

    # Create WCS from property list
    return cpldrs.WCS(pl)


def create_test_lambda_vector_std1():
    """Create wavelength vector for standard star 1 data from C test."""
    # Wavelengths from kStd1 array in C test (every 10th wavelength plane)
    wavelengths = [
        3737.84,
        3770.84,
        3803.84,
        3836.84,
        3869.84,
        3902.84,
        3935.84,
        3968.84,
        4001.84,
        4034.84,
        4067.84,
        4100.84,
        4199.84,
        4232.84,
        4265.84,
        4298.84,
        4331.84,
        4364.84,
        4397.84,
        4430.84,
        4463.84,
        4496.84,
        4529.84,
        4562.84,
        4628.84,
        4661.84,
        4694.84,
        4727.84,
        4760.84,
        4793.84,
        4826.84,
        4859.84,
        4925.84,
        4958.84,
        4991.84,
        5024.84,
        5057.84,
        5090.84,
        5123.84,
        5156.84,
        5189.84,
        5222.84,
        5255.84,
        5288.84,
        5321.84,
        5354.84,
        5387.84,
        5453.84,
        5486.84,
        5519.84,
        5552.84,
        5585.84,
        5618.84,
        5651.84,
        5684.84,
        5717.84,
        5750.84,
        5783.84,
        5816.84,
        5849.84,
        5882.84,
        5915.84,
        5981.84,
        6014.84,
        6047.84,
        6080.84,
        6113.84,
        6146.84,
        6179.84,
        6212.84,
        6245.84,
        6278.84,
        6311.84,
        6344.84,
        6377.84,
        6410.84,
        6443.84,
        6476.84,
        6509.84,
        6542.84,
        6608.84,
        6641.84,
        6674.84,
        6707.84,
        6740.84,
        6773.84,
        6806.84,
        6839.84,
        6905.84,
        6938.84,
    ]
    return cplcore.Vector(wavelengths)


def create_test_lambda_vector_std2():
    """Create wavelength vector for standard star 2 data from C test."""
    # Wavelengths from kStd2 array in C test (every 10th wavelength plane)
    wavelengths = [
        3868.2,
        3901.2,
        3934.2,
        4000.2,
        4066.2,
        4198.2,
        4231.2,
        4264.2,
        4297.2,
        4429.2,
        4462.2,
        4495.2,
        4528.2,
        4561.2,
        4594.2,
        4627.2,
        4660.2,
        4693.2,
        4726.2,
        4759.2,
        4792.2,
        4825.2,
        4858.2,
        4924.2,
        4957.2,
        4990.2,
        5023.2,
        5056.2,
        5089.2,
        5122.2,
        5155.2,
        5188.2,
        5221.2,
        5254.2,
        5287.2,
        5320.2,
        5353.2,
        5386.2,
        5419.2,
        5452.2,
        5485.2,
        5518.2,
        5551.2,
        5584.2,
        5617.2,
        5650.2,
        5683.2,
        5716.2,
        5782.2,
        5815.2,
        5848.2,
        5881.2,
        5914.2,
        5947.2,
        5980.2,
        6013.2,
        6046.2,
        6079.2,
        6112.2,
        6145.2,
        6178.2,
        6211.2,
        6244.2,
        6277.2,
        6310.2,
        6343.2,
        6376.2,
        6409.2,
        6442.2,
        6475.2,
        6508.2,
        6541.2,
        6607.2,
        6640.2,
        6706.2,
        6739.2,
        6772.2,
        6805.2,
        6838.2,
        6904.2,
        6937.2,
    ]
    return cplcore.Vector(wavelengths)


def create_test_lambda_vector_muse1():
    """Create wavelength vector for MUSE data from C test."""
    # Wavelengths from kMuse1 array in C test
    wavelengths = [
        4651.657,
        4701.657,
        4714.157,
        4726.657,
        4739.157,
        4751.657,
        4764.157,
        4776.657,
        4789.157,
        4801.657,
        4814.157,
        4826.657,
        4839.157,
        4851.657,
        4864.157,
        4876.657,
        4889.157,
        4901.657,
        4914.157,
        4926.657,
        4939.157,
        4951.657,
        4964.157,
        4976.657,
        4989.157,
        5001.657,
        5014.157,
        5026.657,
        5039.157,
        5051.657,
        5064.157,
        5076.657,
        5089.157,
        5101.657,
        5114.157,
        5126.657,
        5139.157,
        5151.657,
        5164.157,
        5176.657,
        5189.157,
        5201.657,
        5214.157,
        5226.657,
        5251.657,
        5264.157,
        5289.157,
        5301.657,
        5314.157,
        5326.657,
        5339.157,
        5351.657,
        5364.157,
        5376.657,
        5389.157,
        5401.657,
        5414.157,
        5426.657,
        5439.157,
        5451.657,
        5464.157,
        5476.657,
        5489.157,
        5501.657,
        5514.157,
        5526.657,
        5539.157,
        5551.657,
        5564.157,
        5589.157,
        5601.657,
        5614.157,
        5626.657,
        5639.157,
        5651.657,
        5664.157,
        5676.657,
        5689.157,
        5701.657,
        5714.157,
        5726.657,
        5739.157,
        5751.657,
        5764.157,
        5776.657,
        5789.157,
        5801.657,
        5814.157,
        5826.657,
        5839.157,
        5851.657,
        5876.657,
        5889.157,
        5926.657,
        5939.157,
        5951.657,
        5964.157,
        5989.157,
        6001.657,
        6014.157,
        6026.657,
        6039.157,
        6051.657,
        6064.157,
        6076.657,
        6089.157,
        6101.657,
        6114.157,
        6126.657,
        6139.157,
        6151.657,
        6164.157,
        6176.657,
        6189.157,
        6201.657,
        6214.157,
        6226.657,
        6239.157,
        6251.657,
        6264.157,
        6314.157,
        6326.657,
        6339.157,
        6351.657,
        6376.657,
        6389.157,
        6401.657,
        6414.157,
        6426.657,
        6439.157,
        6451.657,
        6464.157,
        6476.657,
        6489.157,
        6514.157,
        6526.657,
        6539.157,
        6589.157,
        6614.157,
        6626.657,
        6639.157,
        6651.657,
        6664.157,
        6676.657,
        6689.157,
        6701.657,
        6714.157,
        6726.657,
        6739.157,
        6751.657,
        6764.157,
        6776.657,
        6789.157,
        6801.657,
        6814.157,
        6826.657,
        6839.157,
        6851.657,
        6876.657,
        6889.157,
        6901.657,
        6914.157,
        6926.657,
        6939.157,
        6951.657,
        6964.157,
        6989.157,
        7001.657,
        7014.157,
        7026.657,
        7039.157,
        7051.657,
        7064.157,
        7076.657,
        7089.157,
        7101.657,
        7114.157,
        7126.657,
        7139.157,
        7151.657,
        7164.157,
        7176.657,
        7189.157,
        7201.657,
        7214.157,
        7226.657,
        7264.157,
        7351.657,
        7364.157,
        7376.657,
        7389.157,
        7414.157,
        7426.657,
        7451.657,
        7464.157,
        7476.657,
        7489.157,
        7501.657,
        7514.157,
        7526.657,
        7539.157,
        7551.657,
        7564.157,
        7576.657,
        7589.157,
        7601.657,
        7626.657,
        7639.157,
        7664.157,
        7676.657,
        7689.157,
        7701.657,
        7764.157,
        7776.657,
        7801.657,
        7814.157,
        7826.657,
        7864.157,
        7889.157,
        7939.157,
        7951.657,
        7989.157,
        8001.657,
        8039.157,
        8051.657,
        8076.657,
        8089.157,
        8101.657,
        8114.157,
        8126.657,
        8139.157,
        8151.657,
        8164.157,
        8176.657,
        8189.157,
        8201.657,
        8214.157,
        8226.657,
        8239.157,
        8251.657,
        8264.157,
        8301.657,
        8314.157,
        8326.657,
        8339.157,
        8376.657,
        8389.157,
        8439.157,
        8476.657,
        8489.157,
        8514.157,
        8526.657,
        8551.657,
        8564.157,
        8576.657,
        8589.157,
        8601.657,
        8614.157,
        8639.157,
        8689.157,
        8701.657,
        8714.157,
        8726.657,
        8739.157,
        8751.657,
        8776.657,
        8801.657,
        8814.157,
        8876.657,
        8889.157,
        8914.157,
        8926.657,
        8939.157,
        8951.657,
        8964.157,
        8976.657,
        9014.157,
        9026.657,
        9076.657,
        9101.657,
        9114.157,
        9126.657,
        9139.157,
        9164.157,
        9176.657,
        9189.157,
        9201.657,
        9214.157,
        9226.657,
        9239.157,
        9251.657,
        9264.157,
        9276.657,
        9289.157,
        9301.657,
    ]
    return cplcore.Vector(wavelengths)


def test_dar_creation():
    """Test creating a DAR object with valid parameters."""
    # Create test parameters based on C test
    airmass = (1.2784545043, 0.0001)  # From C test header1
    parang = (-157.90507882793, 0.01)  # From C test
    posang = (0.0, 0.01)  # From C test
    temp = (-6.2, 0.01)  # From C test
    rhum = (65.0, 0.01)  # From C test
    pres = (768.4, 0.01)  # From C test
    wcs = create_test_wcs()

    # Create DAR object
    dar = hdrlfunc.Dar(airmass, parang, posang, temp, rhum, pres, wcs)

    # Test that object was created successfully
    assert dar is not None

    # Test accessor methods - they return Value objects, not tuples
    assert dar.airmass.data == airmass[0]
    assert dar.airmass.error == airmass[1]
    assert dar.parang.data == parang[0]
    assert dar.parang.error == parang[1]
    assert dar.posang.data == posang[0]
    assert dar.posang.error == posang[1]
    assert dar.temp.data == temp[0]
    assert dar.temp.error == temp[1]
    assert dar.rhum.data == rhum[0]
    assert dar.rhum.error == rhum[1]
    assert dar.pres.data == pres[0]
    assert dar.pres.error == pres[1]
    assert dar.wcs is not None


def test_dar_compute_std1():
    """Test DAR computation with standard star 1 data from C test."""
    # Create test parameters based on C test
    airmass = (1.2784545043, 0.0001)  # From C test header1
    parang = (-157.90507882793, 0.01)  # From C test
    posang = (0.0, 0.01)  # From C test
    temp = (-6.2, 0.01)  # From C test
    rhum = (65.0, 0.01)  # From C test
    pres = (768.4, 0.01)  # From C test
    wcs = create_test_wcs()

    # Create DAR object
    dar = hdrlfunc.Dar(airmass, parang, posang, temp, rhum, pres, wcs)

    # Create test inputs from C test
    lambdaRef = (5275.644, 0.01)  # kStd1Ref from C test
    lambdaIn = create_test_lambda_vector_std1()

    # Compute DAR corrections
    result = dar.compute(lambdaRef, lambdaIn)

    # Test that result was created successfully
    assert result is not None
    assert result.xShift is not None
    assert result.yShift is not None
    assert result.xShiftErr is not None
    assert result.yShiftErr is not None

    # Test that output vectors have the same size as input
    assert len(result.xShift) == len(lambdaIn)
    assert len(result.yShift) == len(lambdaIn)
    assert len(result.xShiftErr) == len(lambdaIn)
    assert len(result.yShiftErr) == len(lambdaIn)

    # Test that results are reasonable (not all zeros, not all NaN)
    assert any(not math.isclose(x, 0.0, abs_tol=1e-12) for x in result.xShift)
    assert any(not math.isclose(y, 0.0, abs_tol=1e-12) for y in result.yShift)
    assert not all(math.isnan(x) for x in result.xShift)
    assert not all(math.isnan(y) for y in result.yShift)


def test_dar_compute_std2():
    """Test DAR computation with standard star 2 data from C test."""
    # Create test parameters based on C test
    airmass = (1.0233752603, 0.0001)  # From C test header2
    parang = (65.577407, 0.01)  # From C test
    posang = (0.0, 0.01)  # From C test
    temp = (-5.7, 0.01)  # From C test
    rhum = (81.0, 0.01)  # From C test
    pres = (768.4, 0.01)  # From C test
    wcs = create_test_wcs()

    # Create DAR object
    dar = hdrlfunc.Dar(airmass, parang, posang, temp, rhum, pres, wcs)

    # Create test inputs from C test
    lambdaRef = (5273.996, 0.01)  # kStd2Ref from C test
    lambdaIn = create_test_lambda_vector_std2()

    # Compute DAR corrections
    result = dar.compute(lambdaRef, lambdaIn)

    # Test that result was created successfully
    assert result is not None
    assert result.xShift is not None
    assert result.yShift is not None
    assert result.xShiftErr is not None
    assert result.yShiftErr is not None

    # Test that output vectors have the same size as input
    assert len(result.xShift) == len(lambdaIn)
    assert len(result.yShift) == len(lambdaIn)
    assert len(result.xShiftErr) == len(lambdaIn)
    assert len(result.yShiftErr) == len(lambdaIn)


def test_dar_compute_muse1():
    """Test DAR computation with MUSE data from C test."""
    # Create test parameters based on C test
    airmass = (2.14734965358319, 0.0001)  # From C test header3
    parang = (-100.978115969, 0.01)  # From C test
    posang = (0.0, 0.01)  # From C test
    temp = (10.0, 0.01)  # From C test
    rhum = (10.0, 0.01)  # From C test
    pres = (775.0, 0.01)  # From C test
    wcs = create_test_wcs()

    # Create DAR object
    dar = hdrlfunc.Dar(airmass, parang, posang, temp, rhum, pres, wcs)

    # Create test inputs from C test
    lambdaRef = (7100.0, 0.01)  # kMuse1Ref from C test
    lambdaIn = create_test_lambda_vector_muse1()

    # Compute DAR corrections
    result = dar.compute(lambdaRef, lambdaIn)

    # Test that result was created successfully
    assert result is not None
    assert result.xShift is not None
    assert result.yShift is not None
    assert result.xShiftErr is not None
    assert result.yShiftErr is not None

    # Test that output vectors have the same size as input
    assert len(result.xShift) == len(lambdaIn)
    assert len(result.yShift) == len(lambdaIn)
    assert len(result.xShiftErr) == len(lambdaIn)
    assert len(result.yShiftErr) == len(lambdaIn)


def test_dar_invalid_parameters():
    """Test DAR creation with invalid parameters."""
    # Create test parameters with invalid values
    airmass = (-1.0, 0.1)  # Negative airmass should be invalid
    parang = (45.0, 1.0)
    posang = (0.0, 0.5)
    temp = (20.0, 2.0)
    rhum = (50.0, 5.0)
    pres = (1013.25, 10.0)
    wcs = create_test_wcs()

    # This should raise an exception due to invalid airmass
    with pytest.raises(hdrlcore.IllegalInputError):
        hdrlfunc.Dar(airmass, parang, posang, temp, rhum, pres, wcs)


def test_dar_nan_lambda():
    """Test DAR computation with NaN values in lambda vector."""
    # Create test parameters
    airmass = (1.5, 0.1)
    parang = (45.0, 1.0)
    posang = (0.0, 0.5)
    temp = (20.0, 2.0)
    rhum = (50.0, 5.0)
    pres = (1013.25, 10.0)
    wcs = create_test_wcs()

    # Create DAR object
    dar = hdrlfunc.Dar(airmass, parang, posang, temp, rhum, pres, wcs)

    # Create test inputs with NaN
    lambdaRef = (5500.0, 10.0)
    lambdaIn = cplcore.Vector([float("nan")])  # Single NaN value

    # Compute DAR corrections (should handle NaN gracefully)
    result = dar.compute(lambdaRef, lambdaIn)

    # Test that result was created successfully
    assert result is not None
    assert result.xShift is not None
    assert result.yShift is not None
    assert result.xShiftErr is not None
    assert result.yShiftErr is not None

    # Test that output vectors have the same size as input
    assert len(result.xShift) == len(lambdaIn)
    assert len(result.yShift) == len(lambdaIn)
    assert len(result.xShiftErr) == len(lambdaIn)
    assert len(result.yShiftErr) == len(lambdaIn)


def test_dar_different_lambda_refs():
    """Test DAR computation with different reference wavelengths."""
    # Create test parameters
    airmass = (1.0233752603, 0.0001)  # From C test header2
    parang = (65.577407, 0.01)  # From C test
    posang = (0.0, 0.01)  # From C test
    temp = (-5.7, 0.01)  # From C test
    rhum = (81.0, 0.01)  # From C test
    pres = (768.4, 0.01)  # From C test
    wcs = create_test_wcs()

    # Create DAR object
    dar = hdrlfunc.Dar(airmass, parang, posang, temp, rhum, pres, wcs)

    # Create test inputs
    lambdaIn = create_test_lambda_vector_std2()

    # Test with different reference wavelengths
    test_lambda_refs = [2000.0, 7000.0, 30000.0, 19500.0]

    for lambdaRef_val in test_lambda_refs:
        lambdaRef = (lambdaRef_val, 0.01)
        result = dar.compute(lambdaRef, lambdaIn)

        # Test that result was created successfully
        assert result is not None
        assert result.xShift is not None
        assert result.yShift is not None
        assert result.xShiftErr is not None
        assert result.yShiftErr is not None

        # Test that output vectors have the same size as input
        assert len(result.xShift) == len(lambdaIn)
        assert len(result.yShift) == len(lambdaIn)
        assert len(result.xShiftErr) == len(lambdaIn)
        assert len(result.yShiftErr) == len(lambdaIn)


def test_dar_without_error():
    """Test DAR computation without error in lambdaRef."""
    # Create test parameters
    airmass = (1.0233752603, 0.0001)  # From C test header2
    parang = (65.577407, 0.01)  # From C test
    posang = (0.0, 0.01)  # From C test
    temp = (-5.7, 0.01)  # From C test
    rhum = (81.0, 0.01)  # From C test
    pres = (768.4, 0.01)  # From C test
    wcs = create_test_wcs()

    # Create DAR object
    dar = hdrlfunc.Dar(airmass, parang, posang, temp, rhum, pres, wcs)

    # Create test inputs
    lambdaRef = (19500.0, 0.0)  # No error
    lambdaIn = create_test_lambda_vector_std2()

    # Compute DAR corrections
    result = dar.compute(lambdaRef, lambdaIn)

    # Test that result was created successfully
    assert result is not None
    assert result.xShift is not None
    assert result.yShift is not None
    assert result.xShiftErr is not None
    assert result.yShiftErr is not None

    # Test that output vectors have the same size as input
    assert len(result.xShift) == len(lambdaIn)
    assert len(result.yShift) == len(lambdaIn)
    assert len(result.xShiftErr) == len(lambdaIn)
    assert len(result.yShiftErr) == len(lambdaIn)


# Deactivated tests that are not based on C test file:
# - test_dar_null_inputs (not in C test)
# - test_dar_with_none_values (not in C test)
