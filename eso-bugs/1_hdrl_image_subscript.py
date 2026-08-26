# /// script
# requires-python = ">=3.12"
# dependencies = ["numpy"]
# ///
"""PyHDRL: hdrl.core.Image.__getitem__/__setitem__ are unreachable from any subscript.

src/hdrlcore/image_bindings.cpp:208 and :220 bind them with two positional arguments:

    .def("__getitem__", [](hdrl::core::Image& self, cpl_size ypos, cpl_size xpos) {...},
         py::arg("ypos"), py::arg("xpos"))

Python's subscript protocol passes exactly one object to __getitem__ -- an int for h[0],
a tuple for h[0, 0] -- so no subscript expression can ever dispatch to these overloads.
They are reachable only by calling the dunder by name, h.__getitem__(0, 0), which defeats
the purpose of defining a dunder.  The result is that hdrl.core.Image has no working item
access at all.

To be precise about the comparison: cpl.core.Image does NOT support im[y, x] either, so
this is not about tuple indexing.  It is that cpl.core.Image binds __getitem__ with a
single index and returns a live ImageRow proxy, giving working read AND write access via
im[y][x], while the HDRL class -- documented at image_bindings.cpp:41 as providing "a
similar API to cpl.core.Image" -- offers no working form.
"""
import numpy as np
import cpl, hdrl
from cpl.core import Image

pix = np.arange(9, dtype=float).reshape(3, 3)
im = Image(pix.copy())
h = hdrl.core.Image(Image(pix.copy()), Image(np.ones((3, 3))))


def show(expr, env):
    try:
        print(f"      {expr:12s} -> {type(eval(expr, env)).__name__}: {eval(expr, env)}")
    except Exception as e:
        print(f"      {expr:12s} -> {type(e).__name__}: {str(e).splitlines()[0][:58]}")


print("  cpl.core.Image: row proxy, read and write")
for expr in ("im[0]", "im[0][0]", "im[-1]", "im[0, 0]"):
    show(expr, {"im": im})
im[0][1] = 42.0
print(f"      im[0][1] = 42.0 -> writes through: {im.as_array()[0, 1]}")

print("\n  hdrl.core.Image: every subscript form fails")
for expr in ("h[0]", "h[0][0]", "h[0, 0]", "h[(0, 0)]"):
    show(expr, {"h": h})
try:
    h[0, 0] = (5.0, 1.0)
except TypeError as e:
    print(f"      h[0,0] = (5.0, 1.0) -> {type(e).__name__}: {str(e).splitlines()[0][:58]}")

print("\n  the bindings exist but are reachable only by name:")
print("      h.__getitem__(0, 0)            ->", h.__getitem__(0, 0))
h.__setitem__(0, 0, (5.0, 1.0))
print("      h.__setitem__(0, 0, (5.0,1.0)) ->", h.get_pixel(0, 0))

print("\n  EXPECTED: a working subscript -- either a row accessor mirroring cpl.core.Image's")
print("            ImageRow (yielding Values), or a single tuple index h[y, x].")
print("            As bound, both dunders are dead code.")
