#!/usr/bin/env python3
# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "pycpl>=1.0.3.post7",
#     "numpy",
# ]
#
# [tool.uv.sources]
# pycpl = { index = "pycpl" }
#
# [[tool.uv.index]]
# name = "pycpl"
# url = "https://ivh.github.io/pycpl/simple/"
# ///
"""Demo script showing pycpl and hdrl usage."""

import numpy as np

import cpl
from cpl import core as cplcore
import hdrl
from hdrl import core as hdrlcore

def main():
    # Create a dummy 2D image (simulated astronomical data with some structure)
    ny, nx = 256, 256
    y, x = np.ogrid[:ny, :nx]

    # Gaussian source + flat background + noise
    cx, cy = nx // 2, ny // 2
    sigma = 20.0
    source = 1000.0 * np.exp(-((x - cx)**2 + (y - cy)**2) / (2 * sigma**2))
    background = 100.0
    noise = np.random.normal(0, 10, (ny, nx))
    data = background + source + noise

    # Create CPL image from numpy array
    cpl_img = cplcore.Image(data, dtype=cplcore.Type.DOUBLE)
    print(f"Created CPL image: {cpl_img.width}x{cpl_img.height}")

    # Create error image (Poisson-like errors)
    errors = np.sqrt(np.abs(data))
    error_img = cplcore.Image(errors, dtype=cplcore.Type.DOUBLE)

    # Create HDRL image with error propagation
    hdrl_img = hdrlcore.Image(cpl_img, error_img)
    print(f"Created HDRL image: {hdrl_img.width}x{hdrl_img.height}")
    pixel = hdrl_img.get_pixel(cy, cx)
    print(f"  Central pixel: data={pixel.data:.2f}, error={pixel.error:.2f}")

    # HDRL statistics with error propagation
    print("\n--- HDRL Error Propagation Demo ---")
    hdrl_mean = hdrl_img.get_mean()
    print(f"Original HDRL mean: {hdrl_mean.data:.4f} +/- {hdrl_mean.error:.4f}")

    # Subtract background (with zero error on the constant)
    hdrl_result = hdrl_img.duplicate()
    hdrl_result.sub_scalar((background, 0.0))
    hdrl_mean_sub = hdrl_result.get_mean()
    print(f"After background subtraction: {hdrl_mean_sub.data:.4f} +/- {hdrl_mean_sub.error:.4f}")

    # Multiply by gain factor (e.g., 2.5 e-/ADU with 1% uncertainty)
    gain = 2.5
    gain_error = 0.025  # 1% error
    hdrl_result.mul_scalar((gain, gain_error))
    hdrl_mean_gain = hdrl_result.get_mean()
    print(f"After gain correction ({gain} +/- {gain_error}): {hdrl_mean_gain.data:.4f} +/- {hdrl_mean_gain.error:.4f}")

    # Check error at central pixel grew correctly
    pixel_result = hdrl_result.get_pixel(cy, cx)
    print(f"  Central pixel after processing: data={pixel_result.data:.2f}, error={pixel_result.error:.2f}")

    # Compare weighted mean vs simple mean
    hdrl_wmean = hdrl_result.get_weighted_mean()
    print(f"Weighted mean (by 1/error^2): {hdrl_wmean.data:.4f} +/- {hdrl_wmean.error:.4f}")

    # Sigma-clipped mean
    hdrl_scmean = hdrl_result.get_sigclip_mean(3.0, 3.0, 5)
    print(f"Sigma-clipped mean (3-sigma, 5 iter): {hdrl_scmean.data:.4f} +/- {hdrl_scmean.error:.4f}")

    print("--- End HDRL Demo ---\n")

    # Image manipulation: subtract background using CPL (for FITS output)
    result_img = cpl_img.duplicate()
    result_img.subtract_scalar(background)

    # Get statistics
    mean_val = result_img.get_mean()
    stdev_val = result_img.get_stdev()
    min_val, max_val = result_img.get_min(), result_img.get_max()
    print(f"CPL result stats: mean={mean_val:.2f}, stdev={stdev_val:.2f}")
    print(f"  min={min_val:.2f}, max={max_val:.2f}")

    # Create property list for FITS header
    pl = cplcore.PropertyList()
    pl.append("OBJECT", "Demo Source")
    pl.append("TELESCOP", "Simulated")
    pl.append("INSTRUME", "PyHDRL Demo")
    pl.append("EXPTIME", 3600.0)
    pl.append("BUNIT", "electrons")
    pl.append("GAIN", gain)
    pl.append("DATAMEAN", hdrl_mean_gain.data)
    pl.append("DATAERR", hdrl_mean_gain.error)
    pl.append("HISTORY", "Created by pyhdrl demo script")

    print("FITS header properties:")
    for prop in pl:
        print(f"  {prop.name} = {prop.value}")

    # Save HDRL result to FITS (data and error planes)
    output_file = "pyhdrl_demo_output.fits"
    hdrl_result.image.save(output_file, pl, cplcore.io.CREATE)
    print(f"\nSaved data to {output_file}")

    # Save error image as extension
    error_pl = cplcore.PropertyList()
    error_pl.append("EXTNAME", "ERROR")
    error_pl.append("BUNIT", "electrons")
    hdrl_result.error.save(output_file, error_pl, cplcore.io.EXTEND)
    print(f"Appended error image as extension")

if __name__ == "__main__":
    main()
