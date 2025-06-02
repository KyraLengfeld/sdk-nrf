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

# List of sample paths to test (add more samples below)
SAMPLES = [
    SCRIPT_DIR.parents[1] / "samples" / "bluetooth" / "central_bas",
    # add more samples here
]

# Path to YAML file containing ROM thresholds
THRESHOLD_CONFIG = SCRIPT_DIR / "rom_thresholds.yaml"

def load_thresholds(config_file):
    """
    Load board-specific thresholds from the YAML file.
    Structure:
    thresholds:
        board_name:
            sample_name: threshold
    """
    with open(config_file, 'r') as f:
        data = yaml.safe_load(f)
        return data.get("thresholds", {})

# do
# west build -b nrf5340dk/nrf5340/cpuapp samples/bluetooth/central_bas -d build --pristine
# west build -d build/central_bas -t rom_report
def main():
    # Load board+sample-specific thresholds from config
    thresholds = load_thresholds(THRESHOLD_CONFIG)

    if not thresholds:
        print("No thresholds found in config.")
        sys.exit(1)

    failed = False  # Collect failures across samples

    for sample_path in SAMPLES:
        sample_name = sample_path.name  # e.g., "central_bas"

        for board, board_thresholds in thresholds.items():
            if sample_name not in board_thresholds:
                print(f"No threshold defined for sample '{sample_name}' on board '{board}'. Skipping.")
                continue

            threshold = board_thresholds[sample_name]

            print(f"Running ROM KPI check for board: {board}, sample: {sample_name}")

            # Build directory: build_<board>_<sample>
            build_dir = Path(f"build_{board.replace('/', '_')}_{sample_name}")

            # Construct build command
            build_cmd = [
                "west", "build",
                "-b", board,
                str(sample_path),
                "-d", str(build_dir)
            ]

            print(f"Building sample with: {' '.join(build_cmd)}")
            try:
                subprocess.run(build_cmd, check=True)
            except subprocess.CalledProcessError:
                print(f"Build failed for board {board}, sample {sample_name}")
                failed = True
                continue

            # Extract sub-image name from board triple (e.g. "cpuapp", "cpunet")
            sub_image = board.split("/")[-1]

# Is at: tests/rom_kpi/build_nrf5340dk_central_bas/
# Is at: tests/rom_kpi/build_nrf5340dk_nrf5340_cpuapp_central_bas/
# Is at: tests/rom_kpi/build_nrf5340dk_nrf5340_cpunet_central_bas/
            # Construct ELF path accordingly
            elf_file = build_dir / sub_image / "zephyr" / "zephyr.elf"

            if not elf_file.exists():
                print(f"ELF file not found for sub-image '{sub_image}': {elf_file}")
                sys.exit(1)

            print(f"Found ELF file for sub-image '{sub_image}': {elf_file}")

            # Run the rom_report CMake target using west build
            rom_report_cmd = [
                "west", "build",
                "-d", str(build_dir),
                "-t", "rom_report"
            ]

            print(f"Running rom_report CMake target: {' '.join(rom_report_cmd)}")
            try:
                subprocess.run(rom_report_cmd, check=True)
            except subprocess.CalledProcessError:
                print(f"rom_report failed for board {board}, sample {sample_name}")
                failed = True
                continue

            # Path to the rom_report output (automatically created by the target)
            rom_report_output = build_dir / "rom_report"

            if not rom_report_output.exists():
                print(f"rom_report output not found: {rom_report_output}")
                failed = True
                continue

            # DEBUG PRINT
            print(f"rom_report output is available at: {rom_report_output}")

            # parse rom_report

            with open(rom_report_output, 'r') as f:
                report = json.load(f)

            components = report.get("components", [])

            # Filter components matching 'host' for now
            host_components = [
                c for c in components if "host" in c.get("name", "").lower()
            ]

            # Sum up ROM usage of those components
            host_rom_usage = sum(c.get("rom", 0) for c in host_components)

            # Log results to console
            print(f"Host ROM usage for {board}/{sample_name}: {host_rom_usage} bytes (Threshold: {threshold})")

            # check threshold

            if host_rom_usage > threshold:
                print(f"ROM usage exceeds threshold: {host_rom_usage} > {threshold}")
                failed = True
            else:
                print("ROM usage is within threshold.")

    if failed:
        print("Failed.")
        sys.exit(1)
    else:
        print("[Passed")

if __name__ == "__main__":
    main()
