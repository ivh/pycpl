#!/usr/bin/env python3
"""Report ESO library releases newer than the ones this repo builds against.

Comparison is by the date ESO lists for the tarball, not by parsing the version:
ESO ships both `hdrl-1.6.0a5` and a later final `hdrl-1.6.0`, and every ordering
rule gets that pair wrong in one direction or the other.
"""

from __future__ import annotations

import re
import sys
import tomllib
import urllib.request
from pathlib import Path

BASE = "https://ftp.eso.org/pub/dfs/pipelines/libraries"
ROOT = Path(__file__).resolve().parents[2]

# component -> how to find what we currently use
COMPONENTS = ("pycpl", "pyhdrl", "cpl", "hdrl")


def listing(component: str) -> list[tuple[str, str]]:
    """[(version, date)] for every tarball ESO publishes for `component`."""
    url = f"{BASE}/{component}/"
    with urllib.request.urlopen(url, timeout=60) as fh:
        html = fh.read().decode("utf-8", "replace")
    text = re.sub(r"<[^>]*>", " ", html)
    pat = re.compile(rf"{component}-([0-9][0-9a-zA-Z.]*)\.tar\.gz\s+(\d{{4}}-\d{{2}}-\d{{2}})")
    return [(m.group(1), m.group(2)) for m in pat.finditer(text)]


def newest(component: str) -> tuple[str, str]:
    entries = listing(component)
    if not entries:
        raise RuntimeError(f"no tarballs found for {component} - did the listing format change?")
    return max(entries, key=lambda vd: vd[1])


def current() -> dict[str, str]:
    pyproject = tomllib.loads((ROOT / "pyproject.toml").read_text())
    vendored = {p.name.split("-", 1)[0]: p.name.split("-", 1)[1]
                for p in (ROOT / "vendor").iterdir() if p.is_dir()}
    return {
        # our version is ESO's with a .postN suffix appended
        "pycpl": re.sub(r"\.post\d+$", "", pyproject["project"]["version"]),
        # nothing else in the tree records which PyHDRL src/ came from
        "pyhdrl": pyproject["tool"]["pycpl"]["upstream"]["pyhdrl"],
        "cpl": vendored["cpl"],
        "hdrl": vendored["hdrl"],
    }


def main() -> int:
    have = current()
    behind = []
    for c in COMPONENTS:
        version, date = newest(c)
        mark = "  " if version == have[c] else "->"
        print(f"{mark} {c:8s} have {have[c]:12s} newest {version:12s} ({date})")
        if version != have[c]:
            behind.append((c, have[c], version, date))

    out = Path(sys.argv[1]) if len(sys.argv) > 1 else None
    if not behind:
        print("\nup to date")
        if out:
            out.write_text("")
        return 0

    lines = ["The following ESO releases are newer than what this repo builds against:", ""]
    lines += [f"- **{c}**: `{new}` (published {date}) — we use `{old}`"
              for c, old, new, date in behind]
    lines += [
        "",
        f"Listings: " + ", ".join(f"[{c}]({BASE}/{c}/)" for c, *_ in behind),
        "",
        "See the *Upgrading PyCPL/PyHDRL Sources* section of `CLAUDE.md` for the procedure.",
        "",
        "<sub>Opened automatically by `.github/workflows/check-upstream.yml`.</sub>",
    ]
    body = "\n".join(lines)
    print("\n" + body)
    if out:
        out.write_text(body)
    return 0


if __name__ == "__main__":
    sys.exit(main())
