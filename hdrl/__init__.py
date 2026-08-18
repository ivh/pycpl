import sys

from cpl.hdrl import *
from cpl import hdrl as _hdrl

core = _hdrl.core
func = _hdrl.func
debug = _hdrl.debug

# Register submodules so `import hdrl.core` works as with the official ESO PyHDRL.
for _name in ("core", "func", "debug"):
    sys.modules[f"hdrl.{_name}"] = getattr(_hdrl, _name)
