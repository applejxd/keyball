# /// script
# requires-python = ">=3.12,<4.0"
# dependencies = [
#   "keymap-drawer==0.23.0",
#   "pymupdf==1.26.4",
#   "pyyaml==6.0.2",
# ]
# ///

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

import pymupdf
import yaml

REPO_ROOT = Path(__file__).resolve().parents[1]
GENERATED_FILES = (
    "layout.json",
    "keymap.yaml",
    "keymap.svg",
    "combos.svg",
    "keymap.pdf",
    "reference.md",
)

LAYER_LABELS = {
    "KL_BASE": "Base",
    "KL_SMB": "Symbols",
    "KL_NUMFN": "Num/Fn",
    "KL_EMACS": "Emacs",
    "KL_CX": "C-x",
}

KEY_LABELS = {
    "KC_NO": "",
    "KC_A": "A",
    "KC_B": "B",
    "KC_C": "C",
    "KC_D": "D",
    "KC_E": "E",
    "KC_F": "F",
    "KC_G": "G",
    "KC_H": "H",
    "KC_I": "I",
    "KC_J": "J",
    "KC_K": "K",
    "KC_L": "L",
    "KC_M": "M",
    "KC_N": "N",
    "KC_O": "O",
    "KC_P": "P",
    "KC_Q": "Q",
    "KC_R": "R",
    "KC_S": "S",
    "KC_T": "T",
    "KC_U": "U",
    "KC_V": "V",
    "KC_W": "W",
    "KC_X": "X",
    "KC_Y": "Y",
    "KC_Z": "Z",
    "KC_0": "0",
    "KC_1": "1",
    "KC_2": "2",
    "KC_3": "3",
    "KC_4": "4",
    "KC_5": "5",
    "KC_6": "6",
    "KC_7": "7",
    "KC_8": "8",
    "KC_9": "9",
    "KC_F1": "F1",
    "KC_F2": "F2",
    "KC_F3": "F3",
    "KC_F4": "F4",
    "KC_F5": "F5",
    "KC_F6": "F6",
    "KC_F7": "F7",
    "KC_F8": "F8",
    "KC_F9": "F9",
    "KC_F10": "F10",
    "KC_F11": "F11",
    "KC_F12": "F12",
    "KC_F23": "F23",
    "KC_TAB": "Tab",
    "KC_ENT": "Enter",
    "KC_ESC": "Esc",
    "KC_BSPC": "BS",
    "KC_DEL": "Del",
    "KC_HOME": "Home",
    "KC_END": "End",
    "KC_PGUP": "PgUp",
    "KC_PGDN": "PgDn",
    "KC_LEFT": "Left",
    "KC_RGHT": "Right",
    "KC_RIGHT": "Right",
    "KC_UP": "Up",
    "KC_DOWN": "Down",
    "KC_PSCR": "PrtSc",
    "KC_SPC": "Space",
    "KC_COMM": ",",
    "KC_DOT": ".",
    "KC_SLSH": "/",
    "KC_SCLN": ";",
    "KC_MINS": "-",
    "KC_EQL": "^",
    "KC_LBRC": "@",
    "KC_RBRC": "[",
    "KC_QUOT": ":",
    "KC_NUHS": "]",
    "KC_INT1": "\\",
    "KC_INT3": "Yen",
    "KC_LNG1": "Kana",
    "KC_LNG2": "Eisu",
    "KC_LCTL": "Ctrl",
    "KC_RCTL": "Ctrl",
    "KC_LALT": "Alt",
    "KC_RALT": "Alt",
    "KC_LGUI": "Win",
    "KC_RGUI": "Win",
    "KC_LSFT": "Shift",
    "KC_RSFT": "Shift",
    "KC_BTN1": "Mouse 1",
    "KC_BTN2": "Mouse 2",
    "KC_BTN3": "Mouse 3",
    "KC_BTN4": "Back",
    "KC_BTN5": "Forward",
    "RGB_TOG": "RGB",
    "CUT_LINE": "Cut line",
    "SET_MARK": "Set mark",
    "ABORT": "Abort",
}

JIS_SHIFTED_LABELS = {
    "KC_1": "!",
    "KC_2": '"',
    "KC_3": "#",
    "KC_4": "$",
    "KC_5": "%",
    "KC_6": "&",
    "KC_7": "'",
    "KC_8": "(",
    "KC_9": ")",
    "KC_LBRC": "`",
    "KC_EQL": "~",
    "KC_INT3": "|",
    "KC_SCLN": "+",
    "KC_RBRC": "{",
    "KC_NUHS": "}",
    "KC_MINS": "=",
    "KC_QUOT": "*",
    "KC_INT1": "_",
    "KC_COMM": "<",
    "KC_DOT": ">",
    "KC_SLSH": "?",
}

MOD_WRAPPERS = {
    "C": "Ctrl",
    "LCTL": "Ctrl",
    "RCTL": "Ctrl",
    "S": "Shift",
    "LSFT": "Shift",
    "RSFT": "Shift",
    "A": "Alt",
    "LALT": "Alt",
    "RALT": "Alt",
    "G": "Win",
    "LGUI": "Win",
    "RGUI": "Win",
}

MOD_TAPS = {
    "LCTL_T": "Ctrl",
    "RCTL_T": "Ctrl",
    "LSFT_T": "Shift",
    "RSFT_T": "Shift",
    "LALT_T": "Alt",
    "RALT_T": "Alt",
    "LGUI_T": "Win",
    "RGUI_T": "Win",
}


class GenerationError(RuntimeError):
    pass


def remove_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//.*?$", "", text, flags=re.MULTILINE)


def split_top_level(text: str) -> list[str]:
    values: list[str] = []
    start = 0
    depth = 0
    for index, char in enumerate(text):
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
        elif char == "," and depth == 0:
            values.append(text[start:index].strip())
            start = index + 1
    tail = text[start:].strip()
    if tail:
        values.append(tail)
    return values


def matching_paren(text: str, opening: int) -> int:
    return matching_delimiter(text, opening, "(", ")")


def matching_delimiter(
    text: str,
    opening: int,
    open_character: str,
    close_character: str,
) -> int:
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == open_character:
            depth += 1
        elif text[index] == close_character:
            depth -= 1
            if depth == 0:
                return index
    raise GenerationError(f"Unterminated {open_character}{close_character} expression")


def parse_call(expression: str) -> tuple[str, list[str]] | None:
    expression = expression.strip()
    match = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)\s*\((.*)\)", expression, re.DOTALL)
    if not match:
        return None
    return match.group(1), split_top_level(match.group(2))


def parse_macro_parameters(header: str, macro: str) -> list[str]:
    collapsed = re.sub(r"\\\r?\n", " ", header)
    match = re.search(
        rf"#define\s+{re.escape(macro)}\s*\((.*?)\)\s*\{{",
        collapsed,
        flags=re.DOTALL,
    )
    if not match:
        raise GenerationError(f"Layout macro {macro} was not found")
    return [value.strip() for value in match.group(1).split(",") if value.strip()]


def resolve_layout_alias(header: str, macro: str) -> str:
    seen: set[str] = set()
    while macro not in seen:
        seen.add(macro)
        match = re.search(
            rf"^\s*#define\s+{re.escape(macro)}\s+(LAYOUT_[A-Za-z0-9_]+)\s*$",
            header,
            flags=re.MULTILINE,
        )
        if not match:
            return macro
        macro = match.group(1)
    raise GenerationError(f"Circular layout alias involving {macro}")


def parse_macro_matrix_map(header: str, macro: str) -> dict[str, str]:
    collapsed = re.sub(r"\\\r?\n", " ", header)
    match = re.search(rf"#define\s+{re.escape(macro)}\s*\(", collapsed)
    if not match:
        raise GenerationError(f"Layout macro {macro} was not found")
    opening_paren = collapsed.find("(", match.start())
    closing_paren = matching_paren(collapsed, opening_paren)
    opening_brace = collapsed.find("{", closing_paren)
    if opening_brace < 0:
        raise GenerationError(f"Layout macro {macro} has no matrix body")
    closing_brace = matching_delimiter(collapsed, opening_brace, "{", "}")
    body = collapsed[opening_brace + 1 : closing_brace]

    matrix: dict[str, str] = {}
    rows = re.findall(r"\{([^{}]+)\}", body)
    for row_index, row in enumerate(rows):
        for column_index, value in enumerate(row.split(",")):
            parameter = value.strip()
            if re.fullmatch(r"[LR]\d\d", parameter):
                matrix[parameter] = f"{row_index},{column_index}"
    if not matrix:
        raise GenerationError(f"Layout macro {macro} matrix mapping is empty")
    return matrix


def parse_layers(keymap_source: str) -> list[tuple[str, str, list[str]]]:
    source = remove_comments(keymap_source)
    start = source.find("const uint16_t PROGMEM keymaps")
    if start < 0:
        raise GenerationError("QMK keymaps array was not found")
    source = source[start:]
    pattern = re.compile(
        r"\[([A-Za-z_][A-Za-z0-9_]*)\]\s*=\s*(LAYOUT_[A-Za-z0-9_]+)\s*\("
    )
    layers: list[tuple[str, str, list[str]]] = []
    for match in pattern.finditer(source):
        opening = match.end() - 1
        closing = matching_paren(source, opening)
        layers.append(
            (
                match.group(1),
                match.group(2),
                split_top_level(source[opening + 1 : closing]),
            )
        )
    if not layers:
        raise GenerationError("No layers were found in the QMK keymaps array")
    return layers


def parse_option(label: str) -> tuple[str, tuple[int, int] | None]:
    parts = label.split("\n")
    matrix = parts[0].strip()
    option = None
    if len(parts) >= 4 and parts[3].strip():
        option_parts = parts[3].split(",")
        if len(option_parts) == 2:
            option = (int(option_parts[0]), int(option_parts[1]))
    return matrix, option


def parse_via_geometry(via_path: Path, ball: str) -> dict[str, dict[str, float]]:
    selected_option = {"none": 0, "right": 1, "left": 2, "dual": 3}[ball]
    rows = json.loads(via_path.read_text(encoding="utf-8"))["layouts"]["keymap"]

    y = 0.0
    rotation = 0.0
    rotation_x = 0.0
    rotation_y = 0.0
    result: dict[str, dict[str, float]] = {}

    for row_index, row in enumerate(rows):
        x = rotation_x
        if row_index:
            y += 1.0
        width = 1.0
        height = 1.0

        for item in row:
            if isinstance(item, dict):
                if "r" in item:
                    rotation = float(item["r"])
                if "rx" in item:
                    rotation_x = float(item["rx"])
                    x = rotation_x
                if "ry" in item:
                    rotation_y = float(item["ry"])
                    y = rotation_y
                x += float(item.get("x", 0.0))
                y += float(item.get("y", 0.0))
                width = float(item.get("w", 1.0))
                height = float(item.get("h", 1.0))
                continue

            matrix, option = parse_option(item)
            if matrix and (option is None or option == (0, selected_option)):
                key: dict[str, float] = {"x": round(x, 3), "y": round(y, 3)}
                if width != 1.0:
                    key["w"] = round(width, 3)
                if height != 1.0:
                    key["h"] = round(height, 3)
                if rotation:
                    key["r"] = round(rotation, 3)
                    key["rx"] = round(rotation_x, 3)
                    key["ry"] = round(rotation_y, 3)
                if matrix in result:
                    raise GenerationError(
                        f"Duplicate geometry for matrix position {matrix}"
                    )
                result[matrix] = key

            x += width
            width = 1.0
            height = 1.0

    return result


def layer_label(name: str, metadata: dict[str, Any]) -> str:
    configured = metadata.get("layers", {}).get(name, {}).get("label")
    if configured:
        return str(configured)
    if name in LAYER_LABELS:
        return LAYER_LABELS[name]
    return name.removeprefix("KL_").replace("_", " ").title()


def basic_key_label(keycode: str, metadata: dict[str, Any]) -> str:
    configured = metadata.get("custom_keys", {}).get(keycode, {}).get("label")
    if configured:
        return str(configured)
    if keycode in KEY_LABELS:
        return KEY_LABELS[keycode]
    if keycode.startswith("KC_"):
        return keycode.removeprefix("KC_").replace("_", " ").title()
    return keycode.replace("_", " ").title()


def modifier_label(modifiers: list[str], key: str) -> str:
    values = [*modifiers, key]
    return "+".join(value for value in values if value)


def format_key(
    expression: str,
    metadata: dict[str, Any],
    modifiers: list[str] | None = None,
) -> Any:
    expression = expression.strip()
    modifiers = [] if modifiers is None else modifiers

    if expression in {"_______", "KC_TRNS"}:
        return {"t": "▽", "type": "trans"}
    if expression in {"XXXXXXX", "KC_NO"}:
        return {"t": "", "type": "none"}

    call = parse_call(expression)
    if call:
        function, args = call
        if function == "LT" and len(args) == 2:
            return {
                "t": basic_key_label(args[1], metadata),
                "h": layer_label(args[0], metadata),
            }
        if function in MOD_TAPS and len(args) == 1:
            return {
                "t": basic_key_label(args[0], metadata),
                "h": MOD_TAPS[function],
            }
        if function == "OSL" and len(args) == 1:
            return {"t": layer_label(args[0], metadata), "h": "1-shot"}
        if function in {"MO", "TG", "TO"} and len(args) == 1:
            return {"t": layer_label(args[0], metadata), "h": function}
        if function in MOD_WRAPPERS and len(args) == 1:
            if function == "S" and args[0] in JIS_SHIFTED_LABELS and not modifiers:
                return JIS_SHIFTED_LABELS[args[0]]
            return format_key(
                args[0],
                metadata,
                [*modifiers, MOD_WRAPPERS[function]],
            )

    return modifier_label(modifiers, basic_key_label(expression, metadata))


def group_layer(keys: list[Any], parameters: list[str]) -> list[list[Any]]:
    rows: list[list[Any]] = []
    current_row: str | None = None
    for parameter, key in zip(parameters, keys, strict=True):
        row = parameter[1]
        if row != current_row:
            rows.append([])
            current_row = row
        rows[-1].append(key)
    return rows


def parse_combos(
    keymap_source: str,
    base_expressions: list[str],
    base_layer_name: str,
    metadata: dict[str, Any],
) -> list[dict[str, Any]]:
    source = remove_comments(keymap_source)
    combo_keys: dict[str, list[str]] = {}
    for match in re.finditer(
        r"const\s+uint16_t\s+PROGMEM\s+([A-Za-z_][A-Za-z0-9_]*)\s*\[\]\s*=\s*\{(.*?)\};",
        source,
        flags=re.DOTALL,
    ):
        values = [
            value for value in split_top_level(match.group(2)) if value != "COMBO_END"
        ]
        combo_keys[match.group(1)] = values

    expression_positions: dict[str, list[int]] = {}
    for index, expression in enumerate(base_expressions):
        expression_positions.setdefault(expression.strip(), []).append(index)

    combos: list[dict[str, Any]] = []
    for match in re.finditer(
        r"COMBO\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,\s*([A-Za-z0-9_()]+)\s*\)",
        source,
    ):
        combo_name, output = match.groups()
        if combo_name not in combo_keys:
            continue
        positions: list[int] = []
        for trigger in combo_keys[combo_name]:
            matches = expression_positions.get(trigger.strip(), [])
            if len(matches) != 1:
                raise GenerationError(
                    f"Combo {combo_name} trigger {trigger} did not resolve to one base key"
                )
            positions.append(matches[0])
        combos.append(
            {
                "p": positions,
                "k": format_key(output, metadata),
                "l": [base_layer_name],
                "draw_separate": True,
            }
        )
    return combos


def generate_model(
    keyboard_dir: Path,
    keymap: str,
    ball: str,
    metadata: dict[str, Any],
) -> tuple[
    list[dict[str, float]],
    dict[str, list[list[Any]]],
    list[dict[str, Any]],
    list[tuple[str, str, list[str]]],
    dict[str, list[str]],
    str,
]:
    keyboard_name = keyboard_dir.name
    header = (keyboard_dir / f"{keyboard_name}.h").read_text(encoding="utf-8")
    keymap_path = keyboard_dir / "keymaps" / keymap / "keymap.c"
    keymap_source = keymap_path.read_text(encoding="utf-8")
    parsed_layers = parse_layers(keymap_source)

    layout_alias = resolve_layout_alias(header, parsed_layers[0][1])
    universal_parameters = parse_macro_parameters(header, layout_alias)
    matrix_by_parameter = parse_macro_matrix_map(header, layout_alias)
    variant_macro = "LAYOUT_no_ball" if ball == "none" else f"LAYOUT_{ball}_ball"
    variant_parameters = set(parse_macro_parameters(header, variant_macro))
    selected_parameters = [
        parameter
        for parameter in universal_parameters
        if parameter in variant_parameters
    ]

    geometry_by_matrix = parse_via_geometry(keyboard_dir / "via.json", ball)
    layout: list[dict[str, float]] = []
    for parameter in selected_parameters:
        matrix = matrix_by_parameter[parameter]
        if matrix not in geometry_by_matrix:
            raise GenerationError(
                f"No {ball}-ball geometry for {parameter} at matrix position {matrix}"
            )
        layout.append(geometry_by_matrix[matrix])

    layers: dict[str, list[list[Any]]] = {}
    raw_layers: dict[str, list[str]] = {}
    for layer_name, layout_name, expressions in parsed_layers:
        resolved_layout = resolve_layout_alias(header, layout_name)
        parameters = parse_macro_parameters(header, resolved_layout)
        if len(parameters) != len(expressions):
            raise GenerationError(
                f"{layer_name} has {len(expressions)} keys, expected {len(parameters)}"
            )
        expression_by_parameter = dict(zip(parameters, expressions, strict=True))
        selected_expressions = [
            expression_by_parameter[parameter] for parameter in selected_parameters
        ]
        label = layer_label(layer_name, metadata)
        raw_layers[label] = selected_expressions
        layers[label] = group_layer(
            [format_key(expression, metadata) for expression in selected_expressions],
            selected_parameters,
        )

    base_layer_name = layer_label(parsed_layers[0][0], metadata)
    combos = parse_combos(
        keymap_source,
        raw_layers[base_layer_name],
        base_layer_name,
        metadata,
    )
    return layout, layers, combos, parsed_layers, raw_layers, keymap_source


def display_key(value: Any) -> str:
    if isinstance(value, str):
        return value
    if not isinstance(value, dict):
        return str(value)
    tap = str(value.get("t", ""))
    hold = str(value.get("h", ""))
    if tap and hold:
        return f"{tap} / hold {hold}"
    return tap or hold


def markdown_value(value: Any) -> str:
    return str(value).replace("|", "\\|").replace("\n", "<br>")


def markdown_table(headers: list[str], rows: list[list[Any]]) -> list[str]:
    if not rows:
        return ["該当なし。", ""]
    lines = [
        "| " + " | ".join(headers) + " |",
        "|" + "|".join("---" for _ in headers) + "|",
    ]
    lines.extend(
        "| " + " | ".join(markdown_value(value) for value in row) + " |" for row in rows
    )
    lines.append("")
    return lines


def parse_combo_definitions(keymap_source: str) -> list[tuple[list[str], str]]:
    source = remove_comments(keymap_source)
    combo_keys: dict[str, list[str]] = {}
    for match in re.finditer(
        r"const\s+uint16_t\s+PROGMEM\s+([A-Za-z_][A-Za-z0-9_]*)\s*\[\]\s*=\s*\{(.*?)\};",
        source,
        flags=re.DOTALL,
    ):
        combo_keys[match.group(1)] = [
            value for value in split_top_level(match.group(2)) if value != "COMBO_END"
        ]

    definitions: list[tuple[list[str], str]] = []
    for match in re.finditer(
        r"COMBO\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,\s*([A-Za-z0-9_()]+)\s*\)",
        source,
    ):
        name, output = match.groups()
        if name in combo_keys:
            definitions.append((combo_keys[name], output))
    return definitions


def parse_key_overrides(
    keymap_source: str,
    metadata: dict[str, Any],
) -> list[list[str]]:
    modifier_names = {
        "MOD_MASK_CTRL": "Ctrl",
        "MOD_MASK_SHIFT": "Shift",
        "MOD_MASK_ALT": "Alt",
        "MOD_MASK_GUI": "Win",
    }
    rows: list[list[str]] = []
    source = remove_comments(keymap_source)
    for match in re.finditer(
        r"const\s+key_override_t\s+[A-Za-z_][A-Za-z0-9_]*\s*=\s*ko_make_basic\s*\((.*?)\)\s*;",
        source,
        flags=re.DOTALL,
    ):
        arguments = split_top_level(match.group(1))
        if len(arguments) != 3:
            continue
        trigger_modifiers = [
            modifier_names.get(value.strip(), value.strip())
            for value in arguments[0].split("|")
        ]
        trigger_key = display_key(format_key(arguments[1], metadata))
        trigger = modifier_label(trigger_modifiers, trigger_key)
        replacement = display_key(format_key(arguments[2], metadata))
        rows.append([trigger, replacement])
    return rows


def parse_custom_keycodes(keymap_source: str) -> list[str]:
    source = remove_comments(keymap_source)
    match = re.search(
        r"enum\s+custom_keycodes\s*\{(.*?)\}\s*;",
        source,
        flags=re.DOTALL,
    )
    if not match:
        return []
    result: list[str] = []
    for value in split_top_level(match.group(1)):
        name = value.split("=", 1)[0].strip()
        if name:
            result.append(name)
    return result


def parse_feature_flags(paths: list[Path]) -> dict[str, str]:
    flags: dict[str, str] = {}
    for path in paths:
        if not path.exists():
            continue
        for line in path.read_text(encoding="utf-8").splitlines():
            match = re.match(
                r"\s*([A-Z0-9_]+_ENABLE)\s*=\s*(yes|no)\b",
                line,
                flags=re.IGNORECASE,
            )
            if match:
                flags[match.group(1)] = match.group(2).lower()
    return flags


def resolve_layer_value(
    value: str,
    parsed_layers: list[tuple[str, str, list[str]]],
    metadata: dict[str, Any],
) -> str:
    value = value.strip()
    if value.isdigit():
        index = int(value)
        if index < len(parsed_layers):
            return layer_label(parsed_layers[index][0], metadata)
    return layer_label(value, metadata)


def parse_auto_mouse(
    config_source: str,
    parsed_layers: list[tuple[str, str, list[str]]],
    metadata: dict[str, Any],
) -> list[list[str]]:
    source = remove_comments(config_source)
    enabled = bool(
        re.search(
            r"^\s*#define\s+POINTING_DEVICE_AUTO_MOUSE_ENABLE\b", source, re.MULTILINE
        )
    )
    layer_match = re.search(
        r"^\s*#define\s+AUTO_MOUSE_DEFAULT_LAYER\s+([A-Za-z0-9_]+)",
        source,
        re.MULTILINE,
    )
    layer = (
        resolve_layer_value(layer_match.group(1), parsed_layers, metadata)
        if layer_match
        else "-"
    )
    return [["Enabled" if enabled else "Disabled", layer]]


def parse_rgb_layers(
    keymap_source: str,
    parsed_layers: list[tuple[str, str, list[str]]],
    metadata: dict[str, Any],
) -> list[list[str]]:
    source = remove_comments(keymap_source)
    rows: list[list[str]] = []
    for match in re.finditer(
        r"case\s+([A-Za-z0-9_]+)\s*:\s*"
        r"rgblight_sethsv(?:_noeeprom)?\s*\(\s*(HSV_[A-Za-z0-9_]+)\s*\)",
        source,
        flags=re.DOTALL,
    ):
        layer = resolve_layer_value(match.group(1), parsed_layers, metadata)
        color = match.group(2).removeprefix("HSV_").replace("_", " ").title()
        rows.append([layer, color])
    return rows


def parse_tap_holds(
    parsed_layers: list[tuple[str, str, list[str]]],
    metadata: dict[str, Any],
) -> tuple[list[list[str]], list[list[str]]]:
    tap_holds: list[list[str]] = []
    layer_actions: list[list[str]] = []
    for layer_name, _, expressions in parsed_layers:
        source_layer = layer_label(layer_name, metadata)
        for expression in expressions:
            call = parse_call(expression)
            if not call:
                continue
            function, arguments = call
            if function == "LT" and len(arguments) == 2:
                tap = basic_key_label(arguments[1], metadata)
                target = layer_label(arguments[0], metadata)
                tap_holds.append([source_layer, tap, target])
                layer_actions.append([source_layer, tap, "Hold", target])
            elif function in MOD_TAPS and len(arguments) == 1:
                tap_holds.append(
                    [
                        source_layer,
                        basic_key_label(arguments[0], metadata),
                        MOD_TAPS[function],
                    ]
                )
            elif function in {"OSL", "MO", "TG", "TO"} and len(arguments) == 1:
                target = layer_label(arguments[0], metadata)
                behavior = {
                    "OSL": "One shot",
                    "MO": "While held",
                    "TG": "Toggle",
                    "TO": "Switch",
                }[function]
                layer_actions.append([source_layer, target, behavior, target])
    return tap_holds, layer_actions


def generate_reference(
    keyboard: str,
    keymap: str,
    ball: str,
    keyboard_dir: Path,
    metadata: dict[str, Any],
    parsed_layers: list[tuple[str, str, list[str]]],
    keymap_source: str,
) -> str:
    config_source = (keyboard_dir / "keymaps" / keymap / "config.h").read_text(
        encoding="utf-8"
    )
    feature_flags = parse_feature_flags(
        [
            keyboard_dir / "rules.mk",
            keyboard_dir / "keymaps" / keymap / "rules.mk",
        ]
    )
    tap_holds, layer_actions = parse_tap_holds(parsed_layers, metadata)

    lines = [
        f"# {keyboard} {keymap} 自動生成リファレンス",
        "",
        "> `keymap.c`、`config.h`、`rules.mk`、`via.json` と `metadata.yaml` から",
        "> 自動生成される。直接編集せず、`mise run keymap:generate` で更新する。",
        "",
        "## 前提",
        "",
    ]
    platform = metadata.get("platform", {})
    lines.extend(
        markdown_table(
            ["項目", "値"],
            [
                ["OS", platform.get("os", "未指定")],
                ["配列", platform.get("layout", "未指定")],
                ["トラックボール構成", ball.title()],
            ],
        )
    )
    notes = platform.get("notes", [])
    if notes:
        lines.extend(["### 注意", ""])
        lines.extend(f"- {note}" for note in notes)
        lines.append("")

    lines.extend(["## レイヤー", ""])
    layer_rows = []
    for index, (name, _, _) in enumerate(parsed_layers):
        layer_metadata = metadata.get("layers", {}).get(name, {})
        layer_rows.append(
            [
                index,
                name,
                layer_label(name, metadata),
                layer_metadata.get("purpose", ""),
            ]
        )
    lines.extend(markdown_table(["番号", "識別子", "表示名", "用途"], layer_rows))

    lines.extend(["## レイヤー操作", ""])
    lines.extend(
        markdown_table(
            ["元レイヤー", "キー", "動作", "移動先"],
            layer_actions,
        )
    )

    lines.extend(["## Tap / Hold", ""])
    lines.extend(
        markdown_table(
            ["レイヤー", "Tap", "Hold"],
            tap_holds,
        )
    )

    lines.extend(["## Combo", ""])
    combo_rows = [
        [
            " + ".join(
                display_key(format_key(trigger, metadata)) for trigger in triggers
            ),
            display_key(format_key(output, metadata)),
        ]
        for triggers, output in parse_combo_definitions(keymap_source)
    ]
    lines.extend(markdown_table(["入力", "出力"], combo_rows))

    lines.extend(["## Key Override", ""])
    lines.extend(
        markdown_table(
            ["入力", "出力"],
            parse_key_overrides(keymap_source, metadata),
        )
    )

    lines.extend(["## カスタムキー", ""])
    custom_rows = []
    for name in parse_custom_keycodes(keymap_source):
        entry = metadata.get("custom_keys", {}).get(name, {})
        custom_rows.append(
            [
                name,
                entry.get("label", basic_key_label(name, metadata)),
                entry.get("description", "説明未登録"),
            ]
        )
    lines.extend(markdown_table(["識別子", "表示名", "動作"], custom_rows))

    lines.extend(["## RGBレイヤー色", ""])
    lines.extend(
        markdown_table(
            ["レイヤー", "色"],
            parse_rgb_layers(keymap_source, parsed_layers, metadata),
        )
    )

    lines.extend(["## Auto Mouse", ""])
    lines.extend(
        markdown_table(
            ["状態", "対象レイヤー"],
            parse_auto_mouse(config_source, parsed_layers, metadata),
        )
    )

    lines.extend(["## ビルド機能", ""])
    lines.extend(
        markdown_table(
            ["機能", "状態"],
            [
                [name, "Enabled" if value == "yes" else "Disabled"]
                for name, value in feature_flags.items()
            ],
        )
    )

    external_shortcuts = metadata.get("external_shortcuts", [])
    lines.extend(["## 外部ショートカット", ""])
    lines.extend(
        markdown_table(
            ["キー", "用途"],
            [
                [shortcut.get("keys", ""), shortcut.get("purpose", "")]
                for shortcut in external_shortcuts
            ],
        )
    )

    lines.extend(["## 状態遷移・操作フロー", ""])
    workflows = metadata.get("workflows", {})
    if workflows:
        for name, steps in workflows.items():
            lines.extend([f"### {name}", ""])
            lines.extend(f"{index}. {step}" for index, step in enumerate(steps, 1))
            lines.append("")
    else:
        lines.extend(["該当なし。", ""])

    return "\n".join(lines).rstrip() + "\n"


def write_pdf(pages: list[tuple[Path, str]], pdf_path: Path) -> None:
    header_height = 38
    footer_height = 24

    result = pymupdf.open()
    for svg_path, title in pages:
        svg_document = pymupdf.open("svg", svg_path.read_bytes())
        diagram = pymupdf.open("pdf", svg_document.convert_to_pdf())
        source_page = diagram[0]
        width = source_page.rect.width
        page = result.new_page(
            width=width,
            height=source_page.rect.height + header_height + footer_height,
        )
        page.insert_text(
            (20, 23),
            title,
            fontsize=13,
            fontname="helv",
            color=(0.2, 0.2, 0.2),
        )
        page.draw_line(
            (20, header_height - 6),
            (width - 20, header_height - 6),
            color=(0.65, 0.65, 0.65),
            width=0.5,
        )
        page.show_pdf_page(
            pymupdf.Rect(
                0,
                header_height,
                width,
                header_height + source_page.rect.height,
            ),
            diagram,
            0,
        )
        footer = "Generated from QMK keymap.c"
        page.insert_text(
            (width - 185, page.rect.height - 9),
            footer,
            fontsize=8,
            fontname="helv",
            color=(0.5, 0.5, 0.5),
        )
    result.set_metadata(
        {
            "title": pages[0][1],
            "author": "keyball keymap generator",
            "subject": "Generated QMK keymap cheat sheet",
            "creator": "keymap-drawer and PyMuPDF",
            "producer": "keyball keymap generator",
        }
    )
    result.save(pdf_path, garbage=4, deflate=True, no_new_id=True)


def generate(
    keyboard: str,
    keymap: str,
    ball: str,
    output_dir: Path,
    metadata_path: Path,
) -> None:
    keyboard_dir = REPO_ROOT / "qmk_firmware" / "keyboards" / "keyball" / keyboard
    if not keyboard_dir.is_dir():
        raise GenerationError(f"Keyboard directory does not exist: {keyboard_dir}")

    metadata = (
        yaml.safe_load(metadata_path.read_text(encoding="utf-8")) or {}
        if metadata_path.exists()
        else {}
    )
    (
        layout,
        layers,
        combos,
        parsed_layers,
        _raw_layers,
        keymap_source,
    ) = generate_model(keyboard_dir, keymap, ball, metadata)
    output_dir.mkdir(parents=True, exist_ok=True)

    layout_path = output_dir / "layout.json"
    yaml_path = output_dir / "keymap.yaml"
    svg_path = output_dir / "keymap.svg"
    combos_svg_path = output_dir / "combos.svg"
    pdf_path = output_dir / "keymap.pdf"
    reference_path = output_dir / "reference.md"

    layout_path.write_text(
        json.dumps(layout, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    model = {
        "layers": layers,
        "combos": combos,
        "draw_config": {
            "separate_combo_diagrams": True,
            "svg_extra_style": (
                ".none rect { fill: #f8f8f8; stroke: #d8d8d8; }\n"
                ".none text { fill: #aaaaaa; }\n"
            ),
        },
    }
    yaml_path.write_text(
        yaml.safe_dump(
            model,
            allow_unicode=True,
            sort_keys=False,
            width=1000,
        ),
        encoding="utf-8",
    )

    keymap_command = shutil.which("keymap")
    if not keymap_command:
        raise GenerationError(
            "keymap-drawer executable was not found; run this script with uv"
        )
    subprocess.run(
        [
            keymap_command,
            "draw",
            str(yaml_path),
            "--qmk-info-json",
            str(layout_path),
            "--keys-only",
            "--output",
            str(svg_path),
        ],
        check=True,
    )
    subprocess.run(
        [
            keymap_command,
            "draw",
            str(yaml_path),
            "--qmk-info-json",
            str(layout_path),
            "--combos-only",
            "--output",
            str(combos_svg_path),
        ],
        check=True,
    )
    title = f"{keyboard} {keymap} keymap ({ball} ball)"
    write_pdf(
        [
            (svg_path, title),
            (combos_svg_path, f"{keyboard} {keymap} combos"),
        ],
        pdf_path,
    )
    reference_path.write_text(
        generate_reference(
            keyboard,
            keymap,
            ball,
            keyboard_dir,
            metadata,
            parsed_layers,
            keymap_source,
        ),
        encoding="utf-8",
    )


def check_generated(
    keyboard: str,
    keymap: str,
    ball: str,
    output_dir: Path,
    metadata_path: Path,
) -> int:
    with tempfile.TemporaryDirectory(prefix="keymap-diagram-") as temporary:
        generated_dir = Path(temporary)
        generate(keyboard, keymap, ball, generated_dir, metadata_path)
        changed = []
        for filename in GENERATED_FILES:
            expected = output_dir / filename
            actual = generated_dir / filename
            if not expected.exists() or expected.read_bytes() != actual.read_bytes():
                changed.append(filename)
        if changed:
            print(
                "Generated keymap files are stale: " + ", ".join(changed),
                file=sys.stderr,
            )
            return 1
    print("Generated keymap files are up to date.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate a Keyball keymap cheat sheet"
    )
    parser.add_argument("--keyboard", default="keyball39")
    parser.add_argument("--keymap", default="emacs")
    parser.add_argument(
        "--ball", choices=("none", "right", "left", "dual"), default="right"
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
    )
    parser.add_argument(
        "--metadata",
        type=Path,
        help="Optional metadata YAML; defaults to the keymap documentation directory",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Fail when checked-in generated files differ from current sources",
    )
    args = parser.parse_args()
    output_dir = args.output_dir or (
        REPO_ROOT / "docs" / "spec" / "keymaps" / args.keyboard / args.keymap
    )
    metadata_path = args.metadata or (
        REPO_ROOT
        / "docs"
        / "spec"
        / "keymaps"
        / args.keyboard
        / args.keymap
        / "metadata.yaml"
    )

    try:
        if args.check:
            return check_generated(
                args.keyboard,
                args.keymap,
                args.ball,
                output_dir,
                metadata_path,
            )
        generate(
            args.keyboard,
            args.keymap,
            args.ball,
            output_dir,
            metadata_path,
        )
    except (GenerationError, subprocess.CalledProcessError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    print(f"Generated keymap files in {output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
