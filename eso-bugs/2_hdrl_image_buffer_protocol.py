# /// script
# requires-python = ">=3.12"
# dependencies = ["numpy"]
# ///
"""PyHDRL: hdrl.core.Image advertises the buffer protocol but has no buffer.

src/hdrlcore/image_bindings.cpp:38 declares the class with py::buffer_protocol():

    py::class_<...> image_class(m, "Image", py::buffer_protocol());

but no .def_buffer(...) is ever attached.  The type therefore exposes tp_as_buffer /
__buffer__ to Python, so buffer consumers try to use it and get an internal pybind11
error instead of a clean TypeError.  cpl.core.Image and cpl.core._Mask1D both declare
the protocol *and* define def_buffer, so they work.
"""
import numpy as np
import cpl, hdrl
from cpl.core import Image

h = hdrl.core.Image(Image(np.arange(9, dtype=float).reshape(3, 3)),
                    Image(np.ones((3, 3))))

print("  hasattr(h, '__buffer__') ->", hasattr(h, "__buffer__"), "(protocol is advertised)")
try:
    memoryview(h)
except Exception as e:
    print(f"  memoryview(h) -> OBSERVED {type(e).__name__}: {e}")

print("\n  for contrast, cpl.core.Image works:")
print("   ", np.asarray(memoryview(Image(np.arange(4, dtype=float).reshape(2, 2)))).ravel())

print("\n  EXPECTED: either a working def_buffer over the data plane,")
print("            or drop py::buffer_protocol() so memoryview() raises a plain TypeError")
