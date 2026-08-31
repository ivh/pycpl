# /// script
# requires-python = ">=3.12"
# dependencies = ["numpy"]
# ///
"""PyCPL: zone_def is passed to CPL as a dangling pointer -- get_noise_window and
get_bias_window are unusable with any window.

src/cpldrs/detector.cpp:67 (get_noise_window), identically at :44 (get_bias_window):

    const size* zone_def_ptr;
    if (zone_def.has_value()) {
        const size zone_def_arr[4] = { ...+1, ...+1, ...+1, ...+1 };  // block-scoped
        zone_def_ptr = zone_def_arr;            // lifetime ends at the closing brace
    } else {
        zone_def_ptr = nullptr;
    }
    double noise, error;
    Error::throw_errors_with(cpl_flux_get_noise_window, diff.ptr(), zone_def_ptr, ...);

zone_def_arr is destroyed when the if-block ends, so CPL dereferences dead stack.  It
reads four garbage integers, the guard in cpl_detector.c:268

    cpl_ensure_code(rect[0] < rect[1] && rect[2] < rect[3], CPL_ERROR_ILLEGAL_INPUT);

fails, and the user sees an error stack pointing into CPL -- so the defect looks like a
C-library problem when it is entirely in the binding layer.

PROOF that the passed values never arrive: zone_def=(0, 2047, 0, 2047) on a 2048x2048
image is precisely the region used by zone_def=None.  None succeeds; the explicit,
identical window fails.  No input validation could legitimately reject it.

get_noise_ring is unaffected -- it heap-allocates its zone array (detector.cpp:93) --
which isolates the cause to the stack-lifetime bug rather than the +1 FITS conversion.

SEVERITY: this is undefined behaviour, not a deterministic error.  Here the stack garbage
happens to fail the guard on every call.  With another compiler, optimisation level or
platform the array may survive, or hold plausible values, in which case the call SUCCEEDS
and returns a read-noise computed over an arbitrary region, with no warning.  A silently
wrong detector characterisation is far worse than the exception seen here.

Minor, same file: get_noise_ring leaks its `new double[4]` when throw_errors_with throws,
because the delete[] follows the call (detector.cpp:105).
"""
import numpy as np
import cpl

rng = np.random.default_rng(1)
diff = rng.normal(size=(2048, 2048)) * 70 - rng.normal(size=(2048, 2048)) * 70
im = cpl.core.Image(diff)


def noise(zone):
    try:
        return f"OK  noise={cpl.drs.detector.get_noise_window(diff_image=im, zone_def=zone)[0]:.4f}"
    except Exception as e:
        return f"OBSERVED {type(e).__name__}"


def bias(zone):
    try:
        return f"OK  bias={cpl.drs.detector.get_bias_window(bias_image=im, zone_def=zone)[0]:.4f}"
    except Exception as e:
        return f"OBSERVED {type(e).__name__}"


print("  get_noise_window on a 2048x2048 frame")
print(f"      zone_def=None                    -> {noise(None)}")
for z in [(0, 2047, 0, 2047), (100, 1947, 100, 1947), (500, 1500, 500, 1500), (10, 2000, 10, 2000)]:
    tag = "  <- identical region to None" if z == (0, 2047, 0, 2047) else ""
    print(f"      zone_def={str(z):22s} -> {noise(z)}{tag}")

print("\n  get_bias_window: same defect")
print(f"      zone_def=None                    -> {bias(None)}")
print(f"      zone_def=(0, 2047, 0, 2047)      -> {bias((0, 2047, 0, 2047))}")

print("\n  get_noise_ring: heap-allocated zone array, works")
print("      zone_def=(1024, 1024, 100., 900.) ->",
      cpl.drs.detector.get_noise_ring(diff_image=im, zone_def=(1024, 1024, 100.0, 900.0)))

sub = im.extract(window=(100, 100, 1947, 1947))
print("\n  user workaround (extract, then zone_def=None), costs a full image copy:")
print("      ", cpl.drs.detector.get_noise_window(diff_image=sub, zone_def=None))
print(f"      numpy std of the same frame: {diff.std():.4f}")

print("\n  EXPECTED: zone_def honoured.  FIX: declare zone_def_arr in the enclosing scope")
print("            (or std::array<size,4>) so it outlives the call, in both functions.")
