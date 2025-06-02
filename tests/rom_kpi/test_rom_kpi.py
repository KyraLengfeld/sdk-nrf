#!/usr/bin/env python3

import os
import sys
import subprocess
import json
from pathlib import Path
import yaml

# Absolute path to this script (e.g., tests/rom_kpi/rom_kpi_check.py)
SCRIPT_PATH = Path(__file__).resolve()
SCRIPT_DIR = SCRIPT_PATH.parent

# Path to the sample being analyzed (can be changed)
SAMPLE_PATH = SCRIPT_DIR.parents[1] / "samples" / "hello_world"

# Path to rom_report.py
ROM_REPORT_SCRIPT = SCRIPT_DIR.parents[1] / "scripts" / "rom_report" / "rom_report.py"

# Path to YAML file containing ROM thresholds
THRESHOLD_CONFIG = SCRIPT_DIR / "rom_thresholds.yaml"

def load_thresholds(config_file):
    """
    Load board-specific thresholds from the YAML file.
    Structure:
    thresholds:
      board_name: value
    """
    with open(config_file, 'r') as f:
        data = yaml.safe_load(f)
        return data.get("thresholds", {})

# do
# west build -b nrf5340dk/nrf5340/cpuapp samples/bluetooth/central_bas -d build --pristine
# west build -d build/central_bas -t rom_report
def main():
    board = os.environ.get("BOARD")
    if not board:
        print("Error: BOARD environment variable not set.")
        sys.exit(1)

    print(f"[INFO] Running ROM KPI check for board: {board}")

    # Load board-specific threshold from config
    thresholds = load_thresholds(THRESHOLD_CONFIG)
    if board not in thresholds:
        print(f"[ERROR] No ROM threshold defined for board: {board}")
        sys.exit(1)

    threshold = thresholds[board]

    # build sample

    # Twister sets ZEPHYR_BASE as CWD; use tmp build dir
    build_dir = Path("build_" + board.replace("/", "_"))

    # Construct build command
    build_cmd = [
        "west", "build",
        "-b", board,
        str(SAMPLE_PATH),
        "-d", str(build_dir)
    ]

    print(f"[INFO] Building sample with: {' '.join(build_cmd)}")
    subprocess.run(build_cmd, check=True)

    # run rom_report

    # Path to final ELF binary from Zephyr build
    elf_file = build_dir / "zephyr" / "zephyr.elf"
    if not elf_file.exists():
        print(f"[ERROR] ELF file not found: {elf_file}")
        sys.exit(1)

    # Output path for rom_report JSON
    rom_json = build_dir / "rom_report.json"

    # Command to run rom_report.py
    rom_report_cmd = [
        "python3", str(ROM_REPORT_SCRIPT),
        "--format", "json",
        "-o", str(rom_json),
        str(elf_file)
    ]

    print(f"[INFO] Running ROM report tool: {' '.join(rom_report_cmd)}")
    subprocess.run(rom_report_cmd, check=True)

    if not rom_json.exists():
        print(f"[ERROR] ROM report JSON not created: {rom_json}")
        sys.exit(1)

    # parse rom_report

    with open(rom_json, 'r') as f:
        report = json.load(f)

    components = report.get("components", [])

    # TODO: look into rom_report again, see what to extract

    # Filter components matching 'host' for now
    host_components = [
        c for c in components if "host" in c.get("name", "").lower()
    ]

    # Sum up ROM usage of those components
    host_rom_usage = sum(c.get("rom", 0) for c in host_components)

    # Log results to console
    print(f"[RESULT] Host ROM usage for {board}: {host_rom_usage} bytes (Threshold: {threshold})")

    # check threshold

    if host_rom_usage > threshold:
        print(f"[FAIL] ROM usage exceeds threshold: {host_rom_usage} > {threshold}")
        sys.exit(1)

    print("[PASS] ROM usage is within threshold.")

if __name__ == "__main__":
    main()
