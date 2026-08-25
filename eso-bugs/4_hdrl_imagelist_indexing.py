# /// script
# requires-python = ">=3.12"
# dependencies = ["numpy"]
# ///
"""PyHDRL: hdrl.core.ImageList indexing diverges from cpl.core.ImageList.

src/hdrlcore/imagelist_bindings.cpp:273

    .def("__getitem__", [](hdrl::core::ImageList& self, int index) {
           if (index > self.get_size() || index < 0) {      // <-- two problems
             throw py::index_error("ImageList index out of range");
           }
           return self.get_at(index);
         })

(a) Negative indices are rejected instead of wrapping.  cpl.core.ImageList does
    `if (position < 0) position += self.size();` (src/cplcore/image_bindings.cpp:3829),
    so hl[-1] works there and raises IndexError here.  Same divergence for
    cpl.core.Image, whose row access supports im[-1].

(b) The bound check is off by one: `index > size` lets index == size through to
    get_at(), where only std::vector::at() stops it.  Iteration therefore terminates
    by accident, with the message "vector" instead of the intended one -- and any
    future get_at() that does not bounds-check itself turns every `for h in hl`
    into an out-of-bounds read.  __setitem__ two lines below uses the correct
    `index >= self.get_size()`.
"""
import numpy as np
import cpl, hdrl
from cpl.core import Image, ImageList

im = Image(np.arange(9, dtype=float).reshape(3, 3))
il = ImageList([im, im.duplicate()])

hl = hdrl.core.ImageList()
for _ in range(2):
    hl.append(hdrl.core.Image(im.duplicate(), Image(np.ones((3, 3)))))

print("  (a) negative indexing")
print("      cpl.core.ImageList  il[-1] ->", il[-1].get_mean())
try:
    hl[-1]
except Exception as e:
    print(f"      hdrl.core.ImageList hl[-1] -> OBSERVED {type(e).__name__}: {e}")

print("\n  (b) out-of-range message")
for expr, obj in (("il[len(il)]", il), ("hl[len(hl)]", hl)):
    try:
        obj[len(obj)]
    except Exception as e:
        print(f"      {expr:12s} -> {type(e).__name__}: {e!s:<32s} <- {'intended message' if 'ImageList' in str(e) else 'leaked from std::vector::at'}")

print("\n  EXPECTED: hl[-1] wraps like cpl.core.ImageList; the bound test is `index >= size`")
