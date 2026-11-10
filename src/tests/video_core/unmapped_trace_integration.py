# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the real DeviceMemoryManager fault-path test in two fresh processes.

Usage: python3 src/tests/video_core/unmapped_trace_integration.py build/bin/tests
"""
import os
import pathlib
import subprocess
import sys
import tempfile


def run_test(binary: str, enabled: bool) -> None:
    with tempfile.TemporaryDirectory(prefix="orchid-unmapped-trace-") as directory:
        env = os.environ.copy()
        env["EDEN_TEST_UNMAPPED_LOG_DIR"] = directory
        env["EDEN_NX2_UNMAPPED_READ_TRACE"] = "1" if enabled else "0"
        result = subprocess.run(
            [binary, "[unmapped_trace_integration]"],
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=True,
        )
        output = result.stdout
        old = [line for line in output.splitlines() if "Unmapped Device ReadBlock @" in line]
        trace = [line for line in output.splitlines() if "Unmapped read trace:" in line]
        assert len(old) == 16, f"old warning cap: expected 16, got {len(old)}\n{output}"
        assert len(trace) == 2 * enabled, f"trace count: got {len(trace)}\n{output}"
        if enabled:
            assert sum("device=0x30000+69632" in line for line in trace) == 1, output
            direct = next(line for line in trace if "device=0x30000+69632" in line)
            wrapped = next(line for line in trace if "device=0x4b000+20480" in line)
            assert "gpu=false" in direct and "sync=false" in direct, direct
            assert "gpu=true 0x10b000+24576" in wrapped, wrapped
            assert "gpu_translated=true gpu_chunk=0x10b000+20480" in wrapped, wrapped
            assert "sync=false" in wrapped, wrapped
            assert "memory_tracker.cpp" in direct and "memory_tracker.cpp" in wrapped, output
        print(f"trace={'on' if enabled else 'off'}: {len(old)} warnings, {len(trace)} traces; zero fill passed")


if __name__ == "__main__":
    executable = str(pathlib.Path(sys.argv[1]).resolve())
    run_test(executable, False)
    run_test(executable, True)
