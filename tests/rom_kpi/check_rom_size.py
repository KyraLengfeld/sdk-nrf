# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
import os
import sys
import json
import yaml

def find_identifier(symbol_tree, ident):
    if symbol_tree['identifier'] == ident:
        return symbol_tree
    elif "children" in symbol_tree:
        for c in symbol_tree["children"]:
            if (found := find_identifier(c, ident)) is not None:
                return found
    return None

def collect_sizes(symbol_tree, identifiers):
            total = 0
            matches = []

            def match(identifier, target):
                if '*' in identifier:
                    import fnmatch
                    return fnmatch.fnmatch(target, identifier)
                return identifier == target

            def recurse(tree):
                if any(match(ident, tree["identifier"]) for ident in identifiers):
                    matches.append((tree["identifier"], tree["size"]))
                for child in tree.get("children", []):
                    recurse(child)

            recurse(symbol_tree)
            return matches

if __name__ == "__main__":
    board = os.environ.get("BOARD")
    if not board:
        print("BOARD environment variable not set.")
        sys.exit(1)

    thresholds_path = os.path.join(os.path.dirname(__file__), "rom_thresholds.yaml")
    try:
        with open(thresholds_path, "r") as ymlfile:
            thresholds = yaml.safe_load(ymlfile).get("thresholds", {})
    except Exception as e:
        print(f"Failed to load threshold file: {e}")
        sys.exit(1)

    threshold = thresholds.get(board)
    if threshold is None:
        print(f"No ROM threshold defined for board: {board}")
        sys.exit(1)

    print("Checking ROM size in", sys.argv[1])
    with open(os.path.join(sys.argv[1], "rom.json")) as json_file:
        report = json.load(json_file)

        identifiers = [
            'include/zephyr/drivers/bluetooth',
            'include/zephyr/drivers/bluetooth.h',
            'drivers/bluetooth',
            'dts/bindings/bluetooth',
            'include/zephyr/bluetooth',
            'samples/bluetooth',
            'subsys/bluetooth/host_extensions',
            'include/bluetooth',
        ]

        matches = collect_sizes(report['symbols'], identifiers)
        size = sum(s for _, s in matches)
        for ident, s in matches:
            print(f"{ident}: {s} bytes")
        print(f"Total size: {size} bytes")
        if size > threshold:
            print(f"FAIL: ROM size exceeds threshold of {threshold} bytes.")
            sys.exit(1)
        else:
            print("PASS: ROM size is within threshold.")
            sys.exit(0)
