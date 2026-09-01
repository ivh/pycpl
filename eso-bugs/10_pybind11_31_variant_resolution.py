# /// script
# requires-python = ">=3.12"
# dependencies = []
# ///
"""PyCPL: built against pybind11 >= 3.1.0, every int and bool parameter is
unsettable and every int property silently becomes double-complex.

PyCPL passes values across the Python/C++ boundary as std::variant and lets
pybind11's variant caster pick the alternative.  That caster resolves by FIRST
MATCH IN DECLARATION ORDER, and is safe only because of its conversion-free
first pass (pybind11 stl.h, unchanged wording since 2.x):

    // Do a first pass without conversions to improve constructor resolution.
    // E.g. py::int_(1).cast<variant<double, int>>() needs to fill the `int`
    // slot of the variant. Without two-pass loading `double` would be filled
    // because it appears first and a conversion is possible.

PyCPL's variants deliberately put double/complex FIRST, as a workaround for a
different pybind11 ordering quirk:

    src/cplui/parameter.hpp:55
        typedef std::variant<double, bool, std::string, int> value_type;

    src/cplcore/property.hpp:134
        using value_type = std::variant<std::complex<double>, double, bool,
                                        std::string, long, std::complex<float>,
                                        long long, int, char, float>;

pybind11 PR #5879 (released in 3.1.0, 2026-08-06) makes the float/complex
casters accept integers in the no-convert pass.  The first pass therefore no
longer protects the integral slots, and a Python int lands in `double` -- or,
for Property, in `std::complex<double>`.

MEASURED with a minimal module, variant<double, bool, std::string, int>, same
source compiled against each pybind11:

    Python value      pybind11 3.0.4      pybind11 3.1.0
    1                 int                 double
    True              bool                double
    1.5               double              double
    'x'               string              string

    #include <pybind11/pybind11.h>
    #include <pybind11/stl.h>
    #include <variant>
    using value_type = std::variant<double, bool, std::string, int>;
    std::string which(value_type v) {
      if (std::get_if<double>(&v)) return "double";
      if (std::get_if<bool>(&v))   return "bool";
      if (std::get_if<std::string>(&v)) return "string";
      if (std::get_if<int>(&v))    return "int";
      return "?";
    }
    PYBIND11_MODULE(vtest, m) { m.def("which", &which); }

CONSEQUENCE 1 -- cpl.ui, loud.  ParameterValue::set_value (parameter.cpp:280)
switches on the CPL type and std::get_if<int> now fails, so it throws
MismatchedParameterException.  Every int parameter and every bool parameter is
unsettable, which takes PyEsoRex down before a recipe can run: Parameter.from_cplui
copies value across on load, so `pyesorex <any recipe with an int parameter>`
dies in load_recipe with "A parameter of type int does not match the received type".

CONSEQUENCE 2 -- cpl.core, SILENT AND WORSE.  Property has no target type to
check against: value_to_cpl_type (property.hpp:156) derives the CPL type from
whichever alternative the caster filled, and complex<double> heads that variant.
EVERY numeric property therefore collapses to CPL_TYPE_DOUBLE_COMPLEX -- ints,
floats and bools alike: Property('EXPTIME', 3) holds (3+0j), Property('K', 1.5)
holds (1.5+0j), Property('K', True) holds (1+0j).  No exception, no warning -- they
are written to the FITS header as complex keywords.  Product headers are corrupted
with no indication to the user.

pyproject.toml declares `pybind11>=2.8` with no upper bound, so this reaches
anyone who builds 1.0.4 from source today, and any wheel built after 2026-08-06.

FIX, in order of preference:

  (a) Reorder both variants so the integral and boolean alternatives precede the
      floating and complex ones, restoring under 3.1 the resolution 3.0 produced:
          cplui:   <bool, int, std::string, double>
          cplcore: <bool, long, double, std::complex<double>, std::string, ...>
      The acceptance test is exact equivalence, cell by cell, against the stock
      caster under pybind11 3.0.
  (b) A type_caster specialisation per variant that dispatches explicitly on the
      Python type (PyBool_Check -> bool, PyLong_Check -> long, PyFloat_Check ->
      double, PyComplex_Check -> complex<double>), so resolution never depends on
      declaration order again.  ~70 lines, no change to any bound signature.

  In either case, pin `pybind11>=2.8,<3.1` in pyproject.toml until the fix ships:
  the variants are ordering-sensitive by construction, and the next caster change
  upstream will land on them again.
"""
import cpl
import cpl.ui

print(f"  PyCPL {cpl.__version__}  ({cpl.__file__})")

print("\n  cpl.ui.ParameterValue -- loud failure")
for default, expect in [(1, "int"), (True, "bool"), (1.5, "double"), ("s", "string")]:
    p = cpl.ui.ParameterValue("p", "description", "context", default)
    try:
        p.value = default
        result = f"OK    value={p.value!r}"
    except Exception as e:
        result = f"OBSERVED {type(e).__name__}: {e}"
    print(f"      default={default!r:6} -> {str(p.data_type):16} set to itself: {result}")
print(f"      EXPECTED: all four accept their own value back.")

print("\n  cpl.core.Property -- silent corruption")
for value, expect in [(3, "Type.LONG"), (True, "Type.BOOL"), (1.5, "Type.DOUBLE"),
                      ("s", "Type.STRING"), (3 + 0j, "Type.DOUBLE_COMPLEX")]:
    prop = cpl.core.Property("KEY", value)
    flag = "" if str(prop.type) == expect else f"   <- OBSERVED, EXPECTED {expect}"
    print(f"      Property('KEY', {value!r:6}) -> {str(prop.type):22} value={prop.value!r}{flag}")

print("\n  and it reaches the FITS header:")
pl = cpl.core.PropertyList()
pl.append(cpl.core.Property("EXPTIME", 3))
print(f"      PropertyList['EXPTIME'] -> {pl['EXPTIME'].type} {pl['EXPTIME'].value!r}"
      "   <- EXPECTED Type.LONG 3")
