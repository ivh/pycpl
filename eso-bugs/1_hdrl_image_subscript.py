# /// script
# requires-python = ">=3.12"
# dependencies = ["numpy"]
# ///
"""PyHDRL: hdrl.core.Image.__getitem__/__setitem__ are unreachable via subscript syntax.

The bindings define them with two separate positional arguments
(src/hdrlcore/image_bindings.cpp:208 and :220):

    .def("__getitem__", [](hdrl::core::Image& self, cpl_size ypos, cpl_size xpos) {...})

Python's subscript protocol always passes a *single* argument -- a tuple for h[y, x] --
so no form of h[...] can ever dispatch to these overloads.  They are reachable only by
calling the dunder by name, which no user does.  cpl.core.Image binds __getitem__ with
one argument and works as expected.
"""
import numpy as np
import cpl, hdrl
from cpl.core import Image

h = hdrl.core.Image(Image(np.arange(9, dtype=float).reshape(3, 3)),
                    Image(np.ones((3, 3))))

for expr in ("h[0, 0]", "h[(0, 0)]", "h[0]"):
    try:
        print(f"  {expr:12s} -> {eval(expr)}")
    except TypeError as e:
        print(f"  {expr:12s} -> OBSERVED TypeError: {str(e).splitlines()[0]}")

print("\n  reachable only as h.__getitem__(0, 0) ->", h.__getitem__(0, 0))

try:
    h[0, 0] = (5.0, 1.0)
except TypeError as e:
    print("  h[0,0] = (5.0, 1.0) -> OBSERVED TypeError:", str(e).splitlines()[0])
h.__setitem__(0, 0, (5.0, 1.0))
print("  h.__setitem__(0, 0, (5.0, 1.0)) ->", h.get_pixel(0, 0))

print("\n  EXPECTED: h[0, 0] returns the Value, h[0, 0] = (5.0, 1.0) sets it")
print("  FIX: bind __getitem__/__setitem__ to take a std::pair/py::tuple index")
