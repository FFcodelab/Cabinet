import argparse
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description="Compile the adapter against real FreeRTOS headers")
parser.add_argument("--kernel", required=True, type=Path)
parser.add_argument("--port", required=True, type=Path)
parser.add_argument("--config", type=Path, default=ROOT / "tests/freertos")
parser.add_argument("--cc", default="gcc")
args = parser.parse_args()
compiler = shutil.which(args.cc)
if compiler is None:
    parser.error("Compiler not found")
for directory in (args.kernel / "include", args.port, args.config):
    if not directory.is_dir():
        parser.error("Header directory not found: " + str(directory))
subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic",
                "-fsyntax-only", "-I" + str(ROOT / "include"),
                "-I" + str(args.kernel / "include"), "-I" + str(args.port),
                "-I" + str(args.config), str(ROOT / "ports/freertos/cabinet_task.c")], check=True)
print("PASS FreeRTOS adapter header/API compatibility (not a board runtime test)")
