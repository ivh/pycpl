# /// script
# requires-python = ">=3.12"
# dependencies = ["numpy"]
# ///
"""PyCPL: cpl.core.Image(numpy.ma.MaskedArray) silently discards the mask as NaN.

The Image constructor takes a generic iterable, so a MaskedArray is consumed
element-wise; numpy converts each masked element to NaN (emitting only a UserWarning)
and the resulting image has NO bad pixels.  CPL does not treat NaN as rejected, so every
subsequent statistic returns nan rather than ignoring the masked pixels.

This is silent scientific data loss for the very natural line `Image(masked_array)`:
the mask a user carefully constructed in numpy is gone, and the failure only shows up
downstream as nan.  cpl.core.Mask is itself an ndarray subclass, so the information is
trivially convertible -- it just is not used here.
"""
import warnings
import numpy as np
import cpl
from cpl.core import Image, Mask

pix = np.arange(9, dtype=float).reshape(3, 3)
mask = np.zeros((3, 3), bool)
mask[0, 0] = mask[2, 1] = True
ma = np.ma.MaskedArray(pix, mask=mask)

with warnings.catch_warnings(record=True) as caught:
    warnings.simplefilter("always")
    im = Image(ma)
    print("  warnings emitted:", [str(w.message) for w in caught])

print(f"  OBSERVED: count_rejected={im.count_rejected()} (expected 2),"
      f" get_mean()={im.get_mean()} (expected {ma.mean()})")
print("  pixels   :", im.as_array().ravel())

im2 = Image(ma.filled(0))
im2.bpm = Mask(np.ma.getmaskarray(ma))
print(f"\n  correct two-step form: count_rejected={im2.count_rejected()},"
      f" get_mean()={im2.get_mean()}")

print("\n  EXPECTED: Image(MaskedArray) sets the bpm from the mask,")
print("            or raises TypeError instead of silently NaN-ing the masked pixels")
