#!/usr/bin/env python3
"""Regenerate third_party/ardupilot_params/rover.json from ArduPilot/ParameterRepository.

Downloads apm.pdef.json of every Rover-4.x release folder, keeps the fields the parameter window
shows (display name, description, units, range, increment, values, bitmask, reboot required,
read-only) and stores each distinct description once with the versions it applies to. The output
is deterministic: one parameter per line, keys sorted, so a regeneration diffs cleanly.

check   regenerate in memory and compare with the committed file (exit 1 on difference)
"""

import argparse
import json
import re
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "third_party" / "ardupilot_params" / "rover.json"
REPO = "ArduPilot/ParameterRepository"
VEHICLE = "Rover"
MIN_MAJOR = 4
API = f"https://api.github.com/repos/{REPO}"
RAW = f"https://raw.githubusercontent.com/{REPO}"


def fetch(url):
    request = urllib.request.Request(url, headers={"User-Agent": "koggerapp-param-metadata"})
    with urllib.request.urlopen(request, timeout=60) as response:
        return response.read()


def release_versions(commit):
    entries = json.loads(fetch(f"{API}/contents/?ref={commit}"))
    pattern = re.compile(rf"^{VEHICLE}-(\d+)\.(\d+)$")
    versions = []
    for entry in entries:
        m = pattern.match(entry["name"])
        if entry["type"] == "dir" and m and int(m.group(1)) >= MIN_MAJOR:
            versions.append((int(m.group(1)), int(m.group(2))))
    return [f"{a}.{b}" for a, b in sorted(versions)]


def number(text):
    value = float(text)
    return int(value) if value.is_integer() else value


def convert(name, p, problems):
    out = {}
    if p.get("DisplayName"):
        out["name"] = p["DisplayName"].strip()
    if p.get("Description"):
        out["desc"] = " ".join(p["Description"].split())
    if p.get("Units"):
        out["units"] = p["Units"]
    if "Range" in p:
        try:
            out["range"] = [number(p["Range"]["low"]), number(p["Range"]["high"])]
        except (KeyError, ValueError):
            problems.append(f"{name}: range {p['Range']!r}")
    if "Increment" in p:
        try:
            out["inc"] = number(p["Increment"])
        except ValueError:
            problems.append(f"{name}: increment {p['Increment']!r}")
    if "Values" in p:
        values = []
        for k, label in p["Values"].items():
            try:
                values.append([number(k), label])
            except ValueError:
                problems.append(f"{name}: value {k!r}")
        out["values"] = sorted(values, key=lambda v: v[0])
    if "Bitmask" in p:
        bits = []
        for k, label in p["Bitmask"].items():
            try:
                bits.append([int(k), label])
            except ValueError:
                problems.append(f"{name}: bit {k!r}")
        out["bits"] = sorted(bits)
    if p.get("RebootRequired") == "True":
        out["reboot"] = True
    if p.get("ReadOnly") == "True":
        out["readOnly"] = True
    return out


def build():
    commit = json.loads(fetch(f"{API}/commits/main"))["sha"]
    versions = release_versions(commit)
    variants = {}
    problems = []
    for version in versions:
        pdef = json.loads(fetch(f"{RAW}/{commit}/{VEHICLE}-{version}/apm.pdef.json"))
        for group, params in pdef.items():
            if group == "json":
                continue
            for name, p in params.items():
                meta = json.dumps(convert(name, p, problems), sort_keys=True, ensure_ascii=False)
                variants.setdefault(name, {}).setdefault(meta, []).append(version)
        print(f"{VEHICLE}-{version}: {sum(len(v) for v in pdef.values() if isinstance(v, dict))} parameters", file=sys.stderr)

    lines = []
    for name in sorted(variants):
        entries = []
        for meta, used in sorted(variants[name].items(), key=lambda kv: versions.index(kv[1][0])):
            entry = json.loads(meta)
            entry["v"] = used
            entries.append(entry)
        lines.append(json.dumps(name) + ":" + json.dumps(entries, sort_keys=True, ensure_ascii=False, separators=(",", ":")))

    head = {"commit": commit, "source": f"https://github.com/{REPO}", "vehicle": VEHICLE, "versions": versions}
    text = "{\n"
    text += "".join(f"{json.dumps(k)}:{json.dumps(v, separators=(',', ':'))},\n" for k, v in head.items())
    text += '"params":{\n' + ",\n".join(lines) + "\n}\n}\n"
    for problem in sorted(set(problems)):
        print(f"skipped {problem}", file=sys.stderr)
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", nargs="?", choices=["update", "check"], default="update")
    args = parser.parse_args()
    text = build()
    json.loads(text)
    if args.command == "check":
        current = OUTPUT.read_text(encoding="utf-8") if OUTPUT.exists() else ""
        if current != text:
            print(f"{OUTPUT.relative_to(ROOT)} is out of date", file=sys.stderr)
            return 1
        print("up to date", file=sys.stderr)
        return 0
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(text, encoding="utf-8", newline="\n")
    print(f"wrote {OUTPUT.relative_to(ROOT)} ({len(text.encode('utf-8'))} bytes)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
