# /// script
# requires-python = ">=3.12"
# dependencies = ["numpy"]
# ///
"""PyHDRL: the Value namedtuple returned by every statistic is unpicklable.

hdrl.core.Image.get_mean/get_median/get_sum/... return a namedtuple 'Value'.  It is
created by a py::exec'd Python snippet whose globals are not those of a real module,
so the class ends up with __module__ == '__main__' and __qualname__ 'Value'.  pickle
resolves a class by (module, qualname), the lookup fails, and any attempt to send an
HDRL statistic across a process boundary raises PicklingError.

This blocks multiprocessing.Pool / concurrent.futures workers -- the very pattern
PyCPL 1.0.1 added support for in Pyesorex (PIPE-11208).
"""
import pickle
from concurrent.futures import ProcessPoolExecutor
import numpy as np
import cpl, hdrl
from cpl.core import Image


def worker(_):
    h = hdrl.core.Image(Image(np.ones((4, 4))), Image(np.ones((4, 4))))
    return h.get_mean()          # returning a Value from a subprocess


if __name__ == "__main__":
    v = hdrl.core.Image(Image(np.ones((4, 4))), Image(np.ones((4, 4)))).get_mean()
    print("  type(v)              ->", type(v))
    print("  type(v).__module__   ->", type(v).__module__, " (expected 'hdrl.core')")
    try:
        pickle.loads(pickle.dumps(v))
    except Exception as e:
        print(f"  pickle.dumps(v)      -> OBSERVED {type(e).__name__}: {e}")

    print("\n  same failure through a process pool:")
    try:
        with ProcessPoolExecutor(max_workers=1) as ex:
            print("   ", list(ex.map(worker, [0])))
    except Exception as e:
        print(f"    OBSERVED {type(e).__name__}: {str(e).splitlines()[0]}")

    print("\n  workaround: return tuple(v) or (v.data, v.error, v.invalid) instead")
    print("  EXPECTED: Value pickles; define it in a real module namespace")
