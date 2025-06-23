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
        host_symbol = find_identifier(report['symbols'], 'subsys/bluetooth/host')
        print(host_symbol["identifier"], host_symbol["size"])

        size = host_symbol["size"]
        if size > threshold:
            print(f"FAIL: ROM size exceeds threshold of {THRESHOLD} bytes.")
            sys.exit(1)
        else:
            print("PASS: ROM size is within threshold.")
            sys.exit(0)
