import argparse
import concurrent.futures
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def run(arguments):
    subprocess.run([str(item) for item in arguments], check=True, cwd=ROOT)


def verify_report(path, scenario):
    data = json.loads(path.read_text(encoding="utf-8"))
    assert data["schema"] == 1
    assert data["completed"] == 1
    assert not data["safe_latched"]
    assert data["requests"] > 100
    trace = {item["ms"]: item for item in data["trace"]}
    assert len(trace) == 161
    for ms, item in trace.items():
        assert sum(item["watts"]) <= (800 if ms < 5000 else 400)
        if 4500 <= ms < 4800:
            assert item["watts"] == [0, 0, 0, 0]
    assert trace[800]["exchange"] == 7
    assert trace[8000]["update"] == 2
    if scenario == "fault":
        assert data["timeouts"] > 0
        assert trace[3600]["watts"][2] == 0
        assert trace[4400]["watts"][2] > 0
    else:
        assert data["timeouts"] == 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", default=os.environ.get("CC", "gcc"))
    parser.add_argument("--out", type=Path, default=ROOT / "build")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    compiler = shutil.which(args.cc)
    if compiler is None:
        parser.error("C11 compiler not found; use --cc with a GCC/Clang path")
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-Wshadow",
             "-Wconversion", "-Wstrict-prototypes", "-O1", "-g", "-Iinclude", "-Iports/host"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    sources = sorted((ROOT / "src").rglob("*.c")) + [ROOT / "ports/host/host_port.c"]
    objects = [out / ("_".join(source.relative_to(ROOT).parts) + ".o") for source in sources]

    def compile_one(pair):
        source, target = pair
        run([compiler, *flags, "-c", source, "-o", target])

    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(compile_one, zip(sources, objects)))
    suffix = ".exe" if os.name == "nt" else ""
    passed = []
    for source in sorted((ROOT / "tests").glob("test_*.c")):
        target = out / (source.stem + suffix)
        run([compiler, *flags, source, *objects, "-o", target])
        run([target])
        passed.append(source.stem)
        print("PASS", source.stem)
    simulator = out / ("cabinet_sim" + suffix)
    run([compiler, *flags, ROOT / "apps/simulator/main.c", ROOT / "apps/simulator/report.c",
         *objects, "-o", simulator])
    for scenario in ("normal", "fault"):
        report = out / (scenario + ".json")
        run([simulator, scenario, report])
        verify_report(report, scenario)
        passed.append("scenario_" + scenario)
        print("PASS scenario_" + scenario)
    result = {"compiler": subprocess.check_output([compiler, "--version"], text=True).splitlines()[0],
              "checks": passed, "sanitizers": args.sanitize}
    (out / "verification.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("Artifacts:", out)


if __name__ == "__main__":
    main()
