# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
import os
import sys
import json

def find_identifier(symbol_tree, ident):
    if symbol_tree['identifier'] == ident:
        return symbol_tree
    elif "children" in symbol_tree:
        for c in symbol_tree["children"]:
            if (found := find_identifier(c, ident)) is not None:
                return found
    return None

if __name__ == "__main__":
    THRESHOLD = 51969
    print("Checking ROM size in", sys.argv[1])
    with open(os.path.join(sys.argv[1], "rom.json")) as json_file:
        report = json.load(json_file)
        host_symbol = find_identifier(report['symbols'], 'subsys/bluetooth/host')
        print(host_symbol["identifier"], host_symbol["size"])

        size = host_symbol["size"]
        if size > THRESHOLD:
            print(f"FAIL: ROM size exceeds threshold of {THRESHOLD} bytes.")
            sys.exit(1)
        else:
            print("PASS: ROM size is within threshold.")
            sys.exit(0)
