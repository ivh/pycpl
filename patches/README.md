# Local fixes to the upstream sources

`src/` is pristine upstream PyCPL/PyHDRL. The patches here are applied to it by
`setup.py` before every build, so the wheels we publish differ from ESO's by
exactly this directory.

## What qualifies

Every patch makes our wheel behave differently from ESO's under the same version
number, so the bar is high. Two directions of divergence, only one acceptable:

* **Code that works on ESO's build must keep working on ours.** A patch that
  changes the result of a call which currently *succeeds* is out, however wrong
  that result looks. Those belong upstream, and only upstream.
* **Code that works on ours may fail on ESO's.** Unavoidable for any fix, and
  acceptable when the current behaviour is an outright error or undefined
  behaviour, and when the patched behaviour is what upstream clearly intends and
  will eventually ship.

Every patch must have a ticket filed with ESO first; the reproducer lives in
`eso-bugs/`. The patch is a stopgap, not a fork.

## Current patches

| patch | fixes | reproducer |
|---|---|---|
| `0001-cpldrs-detector-zone-def-lifetime.patch` | `get_noise_window`/`get_bias_window` hand CPL a pointer to a block-scoped array, so `zone_def` never arrives and every window raises `IllegalInputError` from inside CPL. Undefined behaviour: another build may instead silently measure the wrong region. | `eso-bugs/7_cpl_detector_zone_def_dangling.py` |
| `0002-hdrlcore-value-picklable.patch` | The `Value` namedtuple returned by every HDRL statistic is built fresh per call with `__module__ == "__main__"`, so it cannot be pickled and cannot be returned from a worker process. | `eso-bugs/3_hdrl_value_unpicklable.py` |
| `0003-hdrlcore-imagelist-getitem-index.patch` | `hdrl.core.ImageList.__getitem__` rejects negative indices instead of wrapping, and bounds with `>` instead of `>=`. Its own `__setitem__`/`__delitem__` already bound correctly, and `cpl.core.ImageList` already wraps. | `eso-bugs/4_hdrl_imagelist_indexing.py` |
| `0004-cplcore-polynomial-fit-sampsym-buffer.patch` | `Polynomial.fit` releases a `new cpl_boolean[]` array with plain `delete` (undefined behaviour), and leaks it on the three throw paths in between. | `eso-bugs/8_cpl_polynomial_fit_leak.py` |
| `0005-cplcore-vector-wrap-allocator.patch` | `Vector(sequence)` hands a `new double[]` buffer to `cpl_vector_wrap`, but `~Vector` calls `cpl_vector_delete`, which releases it with `cpl_free()`. Mismatched allocator, and a leak whenever the wrap fails. | `eso-bugs/9_cpl_vector_wrap_allocator.md` |
| `0006-cpldrs-detector-noise-ring-buffer.patch` | `get_noise_ring` leaks its `new double[4]` whenever the CPL call fails, since the `delete[]` follows the throwing call. | `eso-bugs/7_...` (noted in the same report) |
| `0007-hdrlcore-image-subscript.patch` | `hdrl.core.Image.__getitem__`/`__setitem__` are bound with two positional arguments, which the subscript protocol can never reach, so the class has no working item access. Rebound on a tuple index, which is the only reading of a two-index dunder. | `eso-bugs/1_hdrl_image_subscript.py` |
| `0008-hdrlcore-image-array-refuse.patch` | `np.asarray(hdrl_image)` silently yields a 0-d `dtype=object` array. Refuses the conversion instead, pointing at `.image`/`.error`. Deliberately does *not* implement a real `__array__`: choosing which plane that returns is upstream's call, and guessing it would let code here disagree silently with ESO's build. | `eso-bugs/5_hdrl_image_asarray_silent.py` |

The three memory-management fixes change no observable behaviour at all — they
remove undefined behaviour and leaks — so they sit outside the (a)/(b) split
above and carry no divergence risk.

## When a patch collides with an upstream test

`tests/` and `tests-hdrl/` are ESO's own suites, pristine like `src/`, and CI runs
both against every wheel. A patch that changes observable behaviour can therefore
make an upstream test fail — and when it does, **the test change belongs in the same
patch file**, not in a separate commit to the test tree. `apply.py` runs `patch -p1`
from the repo root, so a patch may touch any path; keeping the two together means the
pristine tree stays pristine, `--revert` restores both halves, and a `stale` report on
the next upgrade covers the test as well as the code.

Which way to change the test needs a moment's thought, because the two divergence
directions above apply to it too:

* The test asserts the buggy behaviour outright (it pins an exception we no longer
  raise, say). Update the assertion, and say in the patch header that ESO's ticket
  should update it the same way. This is the normal case.
* The test fails for a reason the patch did not intend. That is the patch being wrong,
  not the test. Fix the patch.

Never delete or `skip` an upstream test to make a patch apply cleanly: a skipped test
is a divergence nobody can see. As of PyHDRL 1.0.0 / PyCPL 1.0.4 no patch needs this —
all 1170 + 330 tests pass with the eight patches applied.

## Working with them

```bash
python patches/apply.py --check     # status of each patch, changes nothing
python patches/apply.py --apply     # what setup.py runs; idempotent
python patches/apply.py --revert    # restore pristine src/
```

`git checkout -- src/` does the same as `--revert`, more bluntly.

## On an upstream release

Revert first, drop in the new upstream `src/`, re-apply the four `bind_*` renames
(see `CLAUDE.md`), then run `--check`. A patch reporting `stale` means ESO
touched that code — most likely they fixed it, which is the point of having
filed the ticket. Read their version, then rebase the patch or delete it.

Never force or fuzz a patch that does not apply cleanly. A half-applied patch
ships silently different behaviour under a version number that promises
otherwise, which is the one failure mode this directory exists to avoid.
