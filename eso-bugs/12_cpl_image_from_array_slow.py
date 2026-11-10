# /// script
# requires-python = ">=3.12"
# dependencies = ["numpy"]
# ///
"""PyCPL: cpl.core.Image(ndarray) never takes its memcpy path, and rejects complex arrays.

image_from_arr() in cplcore/image_bindings.cpp guards the memcpy with "all strides
== 1".  Buffer strides are in bytes, so for a 2-d array of any image pixel type that
is never true, and every array is copied one pixel at a time through Python's
iterator protocol: ~35 ns/pixel, 140 ms for a 2k x 2k frame against well under 1 ms
for the memcpy.

Separately, pystruct_type_to_cpl() does not know the complex format codes 'Zf' and
'Zd' and throws, so Image(complex ndarray) fails outright with InvalidTypeError
rather than falling back to the slow path.  This is the PIPE-11023 xfail in
tests/cplcore/test_image.py::test_numpy_round_trip.

Two things a fix must not do, both of which the obvious stride comparison would:
  * send int64 arrays down the memcpy path.  They infer as CPL_TYPE_LONG, which
    make_image() rejects; today they succeed, narrowed to INT element-wise.
  * send ndarray subclasses down it.  Image(MaskedArray) currently yields NaN for
    masked elements (see 6_...); a memcpy would expose the values under the mask.
"""
import time
import numpy as np
import cpl
from cpl.core import Image

print(cpl.__file__, cpl.__version__)
a = np.random.default_rng(0).normal(size=(2048, 2048)).astype(np.float32)


def best(arr, n=3):
    ts = []
    for _ in range(n):
        t0 = time.perf_counter()
        Image(arr)
        ts.append(time.perf_counter() - t0)
    return min(ts) * 1e3


t0 = time.perf_counter()
a.copy()
print(f"  numpy copy of the same 16 MB   : {(time.perf_counter() - t0) * 1e3:7.2f} ms")
for label, arr in [("float32 C-contiguous", a), ("float32 transposed", a.T),
                   ("int32", a.astype(np.intc)), ("float64", a.astype(float))]:
    print(f"  OBSERVED Image({label:21s}): {best(arr):7.2f} ms   (EXPECTED: ~ the copy)")

for dt in (np.complex64, np.complex128):
    try:
        im = Image(a[:4, :4].astype(dt))
        print(f"  OBSERVED Image({dt.__name__}): {im.type}")
    except Exception as e:
        print(f"  OBSERVED Image({dt.__name__}): {type(e).__name__}"
              f"   (EXPECTED: a complex image)")
