#!/usr/bin/env python3
"""Apply, check or revert the local fixes to the upstream sources in src/.

Run by setup.py before every build. Idempotent: patches already present in the
tree are skipped. A patch that no longer applies is a hard error, never a fuzzy
or partial apply -- see README.md for what that means and how to react.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

PATCH_DIR = Path(__file__).parent.resolve()
ROOT = PATCH_DIR.parent

PENDING, APPLIED, STALE = "pending", "applied", "stale"


def patches() -> list[Path]:
    return sorted(PATCH_DIR.glob("*.patch"))


def _patch(patch: Path, *args: str) -> int:
    with patch.open("rb") as fh:
        return subprocess.run(
            ["patch", "-p1", "--batch", *args],
            cwd=ROOT,
            stdin=fh,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        ).returncode


def state(patch: Path) -> str:
    if _patch(patch, "--dry-run", "--forward") == 0:
        return PENDING
    if _patch(patch, "--dry-run", "--reverse") == 0:
        return APPLIED
    return STALE


def _require_patch_tool() -> None:
    if shutil.which("patch") is None:
        sys.exit(
            "patches: the 'patch' utility was not found.\n"
            "It is needed to apply the local fixes in patches/ to src/.\n"
            "Install it (Linux: yum/apt install patch) and build again."
        )


def _stale_message(patch: Path) -> str:
    return (
        f"\npatches: {patch.name} no longer applies to src/.\n"
        "Upstream has changed the code it touches -- most likely ESO fixed the\n"
        "bug themselves, which is the outcome we want. Compare their version\n"
        "against the patch, then either rebase the patch or delete it.\n"
        "Never force or fuzz it: a partially applied patch ships silently\n"
        "different behaviour under an unchanged version number.\n"
    )


# Our README is the top-level one; ESO's is kept verbatim beside it. An upstream
# swap that copies their README.md over ours would go unnoticed otherwise, since
# nothing else reads it.
README_MARKER = "# PyCPL with batteries included"


def check_layout() -> bool:
    readme = ROOT / "README.md"
    if readme.exists() and README_MARKER not in readme.read_text():
        print(
            f"\npatches: README.md is not ours -- it lacks {README_MARKER!r}.\n"
            "An upstream README has probably been copied over it. ESO's belongs in\n"
            "README_orig.md; restore ours, which documents the local patches.\n",
            file=sys.stderr,
        )
        return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--apply", action="store_true", help="apply pending patches (default)")
    mode.add_argument("--check", action="store_true", help="report status, change nothing")
    mode.add_argument("--revert", action="store_true", help="restore pristine upstream sources")
    args = parser.parse_args()

    found = patches()
    if not found:
        print("patches: none present")
        return 0

    _require_patch_tool()
    failed = not check_layout()

    for patch in found:
        current = state(patch)

        if args.check:
            print(f"patches: {patch.name}: {current}")
            failed |= current == STALE
            if current == STALE:
                print(_stale_message(patch), file=sys.stderr)
            continue

        if args.revert:
            if current == APPLIED:
                _patch(patch, "--reverse")
                print(f"patches: {patch.name}: reverted")
            else:
                print(f"patches: {patch.name}: not applied, nothing to revert")
            continue

        if current == APPLIED:
            print(f"patches: {patch.name}: already applied")
        elif current == PENDING:
            if _patch(patch, "--forward") != 0:
                print(_stale_message(patch), file=sys.stderr)
                failed = True
            else:
                print(f"patches: {patch.name}: applied")
        else:
            print(_stale_message(patch), file=sys.stderr)
            failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
