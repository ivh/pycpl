# /// script
# requires-python = ">=3.12"
# dependencies = ["numpy"]
# ///
"""PyHDRL: np.asarray(hdrl_image) silently returns a 0-d object array.

hdrl.core.Image exposes neither __array__ nor a working buffer (see the buffer-protocol
report), so numpy falls back to wrapping the object itself: the result is a 0-d array of
dtype=object holding the Image, not the pixel data.  No exception, no warning.  Anything
downstream -- arr.mean(), arr[0,0], plt.imshow(arr) -- then fails far from the cause, or
worse, quietly computes on the wrong thing.

cpl.core.Image defines __array__ and returns real pixels, so code that works on a
cpl.core.Image degrades silently when handed an hdrl.core.Image.
"""
import numpy as np
import cpl, hdrl
from cpl.core import Image

pix = np.arange(9, dtype=float).reshape(3, 3)
cpl_img = Image(pix)
h = hdrl.core.Image(Image(pix), Image(np.ones((3, 3))))

a_cpl = np.asarray(cpl_img)
print(f"  np.asarray(cpl.core.Image)  -> dtype={a_cpl.dtype}, shape={a_cpl.shape}, mean={a_cpl.mean()}")

a_hdrl = np.asarray(h)
print(f"  np.asarray(hdrl.core.Image) -> OBSERVED dtype={a_hdrl.dtype}, shape={a_hdrl.shape}, size={a_hdrl.size}")
try:
    print("  a.mean() ->", a_hdrl.mean())
except Exception as e:
    print(f"  a.mean() -> {type(e).__name__}: {str(e).splitlines()[0][:70]}")

print("\n  workaround: np.asarray(h.image)   (a full copy of the data plane)")
print("  EXPECTED: __array__ on hdrl.core.Image returning the data plane,")
print("            or a TypeError rather than a 0-d object array")
