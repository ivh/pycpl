# pybind11 3.0.3 Build Failure (Issue #3)

## Summary

pybind11 3.0.3 introduces stricter handling of member function pointers from
base classes, causing PyCPL to fail to build. Workaround: pin `pybind11<3.0.3`.

## Root Cause

PR [pybind/pybind11#5992](https://github.com/pybind/pybind11/pull/5992) reworked
`method_adaptor` and `cpp_function` constructors, adding `static_assert` checks
via `rebind_member_ptr` and SFINAE-based overload resolution for member function
pointers inherited from base classes.

## Affected Code

### 1. Unregistered base class `Recipe` (primary)

In `src/cplui/plugin_bindings.cpp:101-119`, `CRecipe` is bound but its parent
class `Recipe` is never registered with pybind11:

```cpp
py::class_<cpl::ui::CRecipe, std::shared_ptr<cpl::ui::CRecipe>> crecipe(m, "CRecipe");
```

All property bindings use `Recipe::` member pointers directly:

```cpp
.def_property("author", &cpl::ui::Recipe::get_author, &cpl::ui::Recipe::set_author, ...)
.def_property("copyright", &cpl::ui::Recipe::get_copyright, ...)
.def_property("description", &cpl::ui::Recipe::get_description, ...)
.def_property("email", &cpl::ui::Recipe::get_email, ...)
.def_property("synopsis", &cpl::ui::Recipe::get_synopsis, ...)
.def_property("version", &cpl::ui::Recipe::get_version, ...)
.def_property("name", &cpl::ui::Recipe::get_name, ...)
```

### 2. Duplicate dunder `.def()` calls (secondary, may also be affected)

Several files define the same dunder method twice -- once with the typed
operator and once with a `py::object` fallback:

- `src/cplcore/error_bindings.cpp` -- `__eq__`
- `src/hdrlcore/error_bindings.cpp` -- `__eq__`
- `src/cplcore/vector_bindings.cpp` -- `__eq__`, `__ne__`
- `src/cplcore/polynomial_bindings.cpp` -- `__eq__`, `__ne__`
- `src/cplcore/property_bindings.cpp` -- `__contains__`, `__getitem__`, `__setitem__`
- `src/cplcore/image_bindings.cpp` -- `__getitem__`, `__str__`

## Proper Fix Options

1. **Register `Recipe` as a pybind11 base class:**
   ```cpp
   py::class_<cpl::ui::CRecipe, cpl::ui::Recipe, std::shared_ptr<cpl::ui::CRecipe>> crecipe(m, "CRecipe");
   ```
   This requires also registering `Recipe` via `py::class_<cpl::ui::Recipe>`.

2. **Wrap base-class member pointers in lambdas:**
   ```cpp
   .def_property("author",
       [](cpl::ui::CRecipe& self) { return self.get_author(); },
       [](cpl::ui::CRecipe& self, const std::string& v) { self.set_author(v); }, ...)
   ```

3. **Pin pybind11 (current workaround):**
   ```toml
   requires = ["setuptools>=70", "wheel", "pybind11>=2.8,<3.0.3", "cmake"]
   ```
