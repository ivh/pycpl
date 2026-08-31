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
