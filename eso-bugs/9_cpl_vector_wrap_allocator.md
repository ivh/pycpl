# PyCPL: `Vector(sequence)` allocates with `new[]` and CPL releases with `free()`

Code inspection, not a runnable reproducer — see *How to verify* below for why.

## The defect

`src/cplcore/vector.cpp:30`

```cpp
Vector::Vector(std::vector<double> values)
{
  double* new_data = new double[values.size()];
  std::copy(values.begin(), values.end(), new_data);
  m_interface =
      Error::throw_errors_with(cpl_vector_wrap, values.size(), new_data);
}
```

`cpl_vector_wrap` takes ownership of the buffer, and the destructor at
`src/cplcore/vector.cpp:78` releases the vector with `cpl_vector_delete`, which frees
the wrapped data:

```c
/* vendor cpl-7.4/cplcore/cpl_vector.c:240 */
void cpl_vector_delete(cpl_vector *v)
{
    if (v == NULL) return;
    if (v->data != NULL) cpl_free(v->data);   /* :245 */
    cpl_free(v);
}
```

So a buffer from `operator new[]` is released with `cpl_free()`, i.e. `free()`. That is
undefined behaviour: the standard requires `new[]` to be paired with `delete[]`, and
nothing guarantees the two allocators share a heap.

Every `cpl.core.Vector` built from a Python sequence goes through this constructor.

Secondary, same lines: when `cpl_vector_wrap` fails — an empty sequence raises
`IllegalInputError` today — `throw_errors_with` throws and the buffer leaks, since
nothing owns it yet.

## Why it has not bitten anyone

With the default toolchain allocators `operator new[]` for a trivially-destructible
type is a thin wrapper over `malloc`, so `free()` on that pointer happens to work. The
defect is latent, and becomes real as soon as anything replaces `operator new` without
replacing `malloc` — a debug or profiling allocator, a sanitizer build, or an embedding
application that overrides global `operator new`.

## How to verify

There is no runtime symptom to demonstrate on a normal build, which is why this report
has no script. Two ways to make it visible:

* Build PyCPL with AddressSanitizer — the build system already supports it
  (`PYCPL_BUILD_SANITIZE=address`, `setup.py`) — and construct any
  `cpl.core.Vector([...])`, then let it be collected. ASan reports
  `alloc-dealloc-mismatch (operator new [] vs free)`.
* Or read the two files above; the pairing is unambiguous.

## Fix

Allocate with the allocator that CPL will free with:

```cpp
const size_t count = values.size();
double* new_data =
    count > 0 ? static_cast<double*>(cpl_malloc(count * sizeof(double))) : nullptr;
std::copy(values.begin(), values.end(), new_data);
m_interface = Error::throw_errors_with(cpl_vector_wrap, count, new_data);
```

The `count > 0` guard keeps the empty-sequence case behaving exactly as before
(`IllegalInputError` from `cpl_vector_wrap`) without allocating anything to leak.

## Related

The same class of defect appears in `src/cplcore/polynomial.cpp` (`new[]` released with
plain `delete`, plus a leak on three throw paths — see `8_cpl_polynomial_fit_leak.py`,
which does have a measurable symptom) and in `src/cpldrs/detector.cpp:105`
(`get_noise_ring` leaks its `new double[4]` whenever the CPL call throws). All three are
worth fixing in one pass.
