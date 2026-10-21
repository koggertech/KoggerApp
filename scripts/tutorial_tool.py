#!/usr/bin/env python3
"""Tutorial scenario tooling.

gen           write src/tutorial/generated/tutorial_strings.cpp (git-ignored, not compiled) so
              lupdate sees the scenario texts; win_update_translations.bat runs it first
check         validate every scenario against the schema and the live QML, and verify that
              the CMake resource list matches resources/tutorial
translations  list scenario texts that are missing or unfinished in the .ts files
              (run after lupdate); non-zero exit while anything is left
"""

import argparse
import json
import xml.etree.ElementTree as ET
import math
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCENARIO_DIR = ROOT / "resources" / "tutorial"
STRINGS_CPP = ROOT / "src" / "tutorial" / "generated" / "tutorial_strings.cpp"
TUTORIAL_CMAKE = ROOT / "src" / "tutorial" / "CMakeLists.txt"
STORE_QML = ROOT / "qml" / "app" / "WorkspaceStore.qml"
CORE_H = ROOT / "src" / "core.h"
QML_DIR = ROOT / "qml"
ICON_DIR = ROOT / "resources" / "icons"
TS_FILES = {lang: ROOT / "translations" / f"translation_{lang}.ts" for lang in ("ru", "pl")}

ADVANCE = ["target", "anywhere", "condition", "button", "timer"]
ON_MISSING = ["center", "skip", "abort"]

SCENARIO_FIELDS = {
    "id": ("str",), "version": ("num", 1), "devOnly": ("bool",), "title": ("str",),
    "defaults": ("obj",), "steps": ("arr",),
}
COMMON_FIELDS = {
    "enter": ("arr",), "exit": ("arr",),
    "advance": ("str", ADVANCE), "onMissing": ("str", ON_MISSING),
    "delayMs": ("num", 0), "timeoutMs": ("num", 0), "autoAdvanceMs": ("num", 0),
    "dim": ("obj",), "spotlight": ("obj",), "arrow": ("obj",), "bubble": ("obj",), "buttons": ("obj",),
}
STEP_FIELDS = dict(COMMON_FIELDS, **{
    "id": ("str",), "checkpoint": ("bool",), "anchor": ("str",), "title": ("str",), "text": ("str",),
    "comment": ("str",), "until": ("obj",), "next": ("str",),
})
BLOCK_FIELDS = {
    "dim": {"enabled": ("bool",), "opacity": ("num", 0, 1), "blockInput": ("bool",)},
    "spotlight": {"enabled": ("bool",), "shape": ("str", ["roundRect", "circle", "rect"]),
                  "padding": ("num", 0), "radius": ("num", 0), "pulse": ("bool",),
                  "passThrough": ("bool",), "anchors": ("arr",),
                  "merge": ("str", ["auto", "all", "none"]), "mergeGap": ("num", 0)},
    "arrow": {"enabled": ("bool",), "direction": ("str", ["auto", "up", "down", "left", "right"]),
              "length": ("num", 0), "offset": ("num",), "bounce": ("bool",)},
    "bubble": {"enabled": ("bool",), "placement": ("str", ["auto", "top", "bottom", "left", "right", "center"]),
               "maxWidth": ("num", 1), "tail": ("bool",), "counter": ("bool",), "icon": ("str",), "hint": ("bool",)},
    "buttons": {"close": ("bool",), "next": ("bool",), "back": ("bool",), "skip": ("bool",),
                "nextLabel": ("str",), "backLabel": ("str",), "skipLabel": ("str",)},
}
HOSTS = ["store", "core"]
UNTIL_FIELDS = {"on": ("str", HOSTS), "property": ("str",), "equals": ("any",)}
ACTION_FIELDS = {"on": ("str", HOSTS), "call": ("str",), "args": ("arr",), "set": ("str",), "value": ("any",)}
LABEL_KEYS = ("nextLabel", "backLabel", "skipLabel")


def type_ok(value, kind):
    if kind == "bool":
        return isinstance(value, bool)
    if kind == "num":
        return isinstance(value, (int, float)) and not isinstance(value, bool)
    if kind == "str":
        return isinstance(value, str)
    if kind == "obj":
        return isinstance(value, dict)
    if kind == "arr":
        return isinstance(value, list)
    return True


def validate_fields(obj, spec, where, errors):
    for key, value in obj.items():
        if key not in spec:
            errors.append(f"{where}: unknown key '{key}'")
            continue
        rule = spec[key]
        kind = rule[0]
        if not type_ok(value, kind):
            errors.append(f"{where}.{key}: expected {kind}")
            continue
        if kind == "str" and len(rule) > 1 and value not in rule[1]:
            errors.append(f"{where}.{key}: '{value}' is not one of {', '.join(rule[1])}")
        if kind == "num":
            lo = rule[1] if len(rule) > 1 else -math.inf
            hi = rule[2] if len(rule) > 2 else math.inf
            if not (lo <= value <= hi):
                errors.append(f"{where}.{key}: {value} is out of range")


def validate_step_like(obj, spec, where, errors):
    validate_fields(obj, spec, where, errors)
    for key, value in obj.items():
        at = f"{where}.{key}"
        if key in BLOCK_FIELDS and isinstance(value, dict):
            validate_fields(value, BLOCK_FIELDS[key], at, errors)
        if key == "until" and isinstance(value, dict):
            validate_fields(value, UNTIL_FIELDS, at, errors)
            if "property" not in value or "equals" not in value:
                errors.append(f"{at}: needs 'property' and 'equals'")
        if key in ("enter", "exit") and isinstance(value, list):
            for i, action in enumerate(value):
                a = f"{at}[{i}]"
                if not isinstance(action, dict):
                    errors.append(f"{a}: expected object")
                    continue
                validate_fields(action, ACTION_FIELDS, a, errors)
                is_call, is_set = "call" in action, "set" in action
                if is_call == is_set:
                    errors.append(f"{a}: needs exactly one of 'call' or 'set'")
                if is_call and "value" in action:
                    errors.append(f"{a}: 'value' belongs to 'set'")
                if is_set and "args" in action:
                    errors.append(f"{a}: 'args' belongs to 'call'")
                if is_set and "value" not in action:
                    errors.append(f"{a}: 'set' needs 'value'")
    spotlight = obj.get("spotlight")
    if isinstance(spotlight, dict):
        for anchor in spotlight.get("anchors", []) if isinstance(spotlight.get("anchors"), list) else []:
            if not isinstance(anchor, str) or not anchor:
                errors.append(f"{where}.spotlight.anchors: expected non-empty strings")


def deep_merge(base, over):
    out = dict(base)
    for key, value in over.items():
        if isinstance(out.get(key), dict) and isinstance(value, dict):
            out[key] = deep_merge(out[key], value)
        else:
            out[key] = value
    return out


def load_scenarios():
    scenarios = []
    for path in sorted(SCENARIO_DIR.glob("*.json")):
        with path.open(encoding="utf-8") as f:
            scenarios.append((path, json.load(f)))
    return scenarios


def validate_scenario(path, data, errors):
    where = path.name
    if not isinstance(data, dict):
        errors.append(f"{where}: root must be an object")
        return
    local = []
    validate_fields(data, SCENARIO_FIELDS, "scenario", local)
    for key in ("id", "version", "title", "steps"):
        if key not in data:
            local.append(f"scenario: missing '{key}'")
    if local:
        errors.extend(f"{where}: {e}" for e in local)
        return
    if data["id"] != path.stem:
        local.append(f"scenario: id '{data['id']}' must match file name '{path.stem}'")
    if data["version"] != int(data["version"]):
        local.append("scenario.version: must be an integer")
    defaults = data.get("defaults", {})
    validate_step_like(defaults, COMMON_FIELDS, "defaults", local)
    steps = data["steps"]
    if not steps:
        local.append("scenario.steps: must not be empty")
    ids = []
    for i, step in enumerate(steps):
        at = f"steps[{i}]"
        if not isinstance(step, dict):
            local.append(f"{at}: expected object")
            ids.append(f"#{i}")
            continue
        validate_step_like(step, STEP_FIELDS, at, local)
        sid = step.get("id") or f"#{i}"
        if sid in ids:
            local.append(f"{at}: duplicate step id '{sid}'")
        ids.append(sid)
    for i, step in enumerate(steps):
        if not isinstance(step, dict):
            continue
        at = f"steps[{i}]"
        merged = deep_merge(defaults, step)
        nxt = merged.get("next", "")
        if nxt and nxt not in ids:
            local.append(f"{at}.next: unknown step '{nxt}'")
        advance = merged.get("advance", "target")
        if advance == "condition" and "until" not in merged:
            local.append(f"{at}: advance 'condition' needs 'until'")
        if advance == "timer" and not merged.get("autoAdvanceMs", 0) > 0:
            local.append(f"{at}: advance 'timer' needs autoAdvanceMs > 0")
    errors.extend(f"{where}: {e}" for e in local)


def qml_sources():
    for path in QML_DIR.rglob("*.qml"):
        if "legacy_qml" in path.parts:
            continue
        yield path


def declared_anchors():
    literal, prefixes = set(), set()
    block = re.compile(r"KTutorialAnchor\s*\{(.*?)\}", re.S)
    key_literal = re.compile(r"\bkey\s*:\s*\"([^\"]+)\"\s*(\+)?")
    for path in qml_sources():
        text = path.read_text(encoding="utf-8")
        for match in block.finditer(text):
            for key in key_literal.finditer(match.group(1)):
                (prefixes if key.group(2) else literal).add(key.group(1))
    return literal, prefixes


def store_members():
    text = STORE_QML.read_text(encoding="utf-8")
    functions = set(re.findall(r"^\s*function\s+(\w+)\s*\(", text, re.M))
    properties = set(re.findall(r"^\s*(?:readonly\s+|required\s+|default\s+)*property\s+[\w<>.]+\s+(\w+)", text, re.M))
    return functions, properties


def core_members():
    text = CORE_H.read_text(encoding="utf-8")
    functions = set(re.findall(r"Q_INVOKABLE\s+[^;{(]*?\b(\w+)\s*\(", text))
    properties = set(re.findall(r"Q_PROPERTY\(\s*[\w:<>*\s]+?\s+(\w+)\s+READ", text))
    return functions, properties


def host_members():
    return {"store": store_members(), "core": core_members()}


def check_references(path, data, errors):
    literal, prefixes = declared_anchors()
    hosts = host_members()
    defaults = data.get("defaults", {})
    for i, step in enumerate(data.get("steps", [])):
        if not isinstance(step, dict):
            continue
        at = f"{path.name}: steps[{i}]"
        merged = deep_merge(defaults, step)
        anchors = [merged.get("anchor", "")] + list(merged.get("spotlight", {}).get("anchors", []))
        for anchor in filter(None, anchors):
            if anchor not in literal and not any(anchor.startswith(p) for p in prefixes):
                errors.append(f"{at}: anchor '{anchor}' is not declared by any KTutorialAnchor")
        for list_key in ("enter", "exit"):
            for action in merged.get(list_key, []):
                on = action.get("on", "store")
                functions, properties = hosts[on]
                if "call" in action and action["call"] not in functions:
                    errors.append(f"{at}.{list_key}: '{action['call']}' is not a {on} function")
                if "set" in action and action["set"] not in properties:
                    errors.append(f"{at}.{list_key}: '{action['set']}' is not a {on} property")
        until = merged.get("until")
        if isinstance(until, dict):
            on = until.get("on", "store")
            if until.get("property") not in hosts[on][1]:
                errors.append(f"{at}.until: '{until.get('property')}' is not a {on} property")
        icon = merged.get("bubble", {}).get("icon", "")
        if icon and not (ICON_DIR / icon).is_file():
            errors.append(f"{at}.bubble.icon: resources/icons/{icon} does not exist")


def collect_strings(data):
    context = f"tutorial_{data['id']}"
    items = [(context, data["title"], None)]
    defaults = data.get("defaults", {})
    for key in LABEL_KEYS:
        label = defaults.get("buttons", {}).get(key)
        if label:
            items.append((context, label, None))
    for step in data["steps"]:
        comment = step.get("comment")
        for key in ("title", "text"):
            if step.get(key):
                items.append((context, step[key], comment))
        for key in LABEL_KEYS:
            label = step.get("buttons", {}).get(key)
            if label:
                items.append((context, label, comment))
    return items


def cpp_literal(text):
    escaped = text.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n").replace("\t", "\\t")
    return f"\"{escaped}\""


def render_strings(scenarios):
    lines = ["#include <QtGlobal>", "", "namespace {", "",
             "[[maybe_unused]] constexpr const char* kTutorialStrings[] = {"]
    seen = set()
    for _, data in scenarios:
        for context, text, comment in collect_strings(data):
            if (context, text) in seen:
                continue
            seen.add((context, text))
            lines.append(f"    QT_TRANSLATE_NOOP({cpp_literal(context)}, {cpp_literal(text)}),")
    lines += ["    nullptr,", "};", "", "}", ""]
    return "\n".join(lines)


def listed_resources():
    text = TUTORIAL_CMAKE.read_text(encoding="utf-8")
    return set(re.findall(r"resources/tutorial/([\w.-]+\.json)", text))


def cmd_gen(_args):
    scenarios = load_scenarios()
    errors = []
    for path, data in scenarios:
        validate_scenario(path, data, errors)
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    STRINGS_CPP.parent.mkdir(parents=True, exist_ok=True)
    STRINGS_CPP.write_text(render_strings(scenarios), encoding="utf-8", newline="\n")
    print(f"wrote {STRINGS_CPP.relative_to(ROOT)}")
    return 0


def cmd_check(_args):
    scenarios = load_scenarios()
    errors = []
    for path, data in scenarios:
        before = len(errors)
        validate_scenario(path, data, errors)
        if len(errors) == before:
            check_references(path, data, errors)
    files ={path.name for path, _ in scenarios}
    listed = listed_resources()
    for name in sorted(files - listed):
        errors.append(f"resources/tutorial/{name} is not listed in src/tutorial/CMakeLists.txt")
    for name in sorted(listed - files):
        errors.append(f"src/tutorial/CMakeLists.txt lists missing resources/tutorial/{name}")
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"ok: {len(scenarios)} scenario(s)")
    return 0


def ts_messages(path):
    root = ET.parse(path).getroot()
    out = {}
    for ctx in root.iter("context"):
        name = ctx.findtext("name")
        for msg in ctx.iter("message"):
            tr = msg.find("translation")
            if msg.get("numerus") == "yes":
                done = all((n.text or "").strip() for n in tr.iter("numerusform"))
            else:
                done = bool((tr.text or "").strip())
            out[(name, msg.findtext("source"))] = done and tr.get("type") not in ("unfinished", "vanished", "obsolete")
    return out


def cmd_translations(_args):
    scenarios = load_scenarios()
    wanted = []
    for _, data in scenarios:
        for context, text, _comment in collect_strings(data):
            if (context, text) not in wanted:
                wanted.append((context, text))
    problems = []
    for lang, path in TS_FILES.items():
        messages = ts_messages(path)
        for context, text in wanted:
            state = messages.get((context, text))
            if state is None:
                problems.append(f"{lang}: missing  [{context}] {text}")
            elif not state:
                problems.append(f"{lang}: unfinished [{context}] {text}")
    if problems:
        print("\n".join(problems), file=sys.stderr)
        print(f"{len(problems)} scenario text(s) need translation; run gen + lupdate first if any are missing", file=sys.stderr)
        return 1
    print(f"ok: {len(wanted)} scenario text(s) translated in {', '.join(TS_FILES)}")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("gen").set_defaults(func=cmd_gen)
    sub.add_parser("check").set_defaults(func=cmd_check)
    sub.add_parser("translations").set_defaults(func=cmd_translations)
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
