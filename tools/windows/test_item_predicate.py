#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Exercise the Windows diagnostic against an asset-free x86 process."""

import argparse
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import subprocess
import threading
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-directory", type=Path, required=True)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--output-directory", type=Path, required=True)
    parser.add_argument(
        "--case", choices=["capture", "idle", "bad-state", "guards", "cancel"]
    )
    args = parser.parse_args()
    args.output_directory.mkdir(parents=True, exist_ok=False)
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CheckRemoteDebuggerPresent.argtypes = [
        wintypes.HANDLE,
        ctypes.POINTER(wintypes.BOOL),
    ]
    kernel.GetConsoleWindow.restype = wintypes.HWND
    user = ctypes.WinDLL("user32", use_last_error=True)
    user.ShowWindow.argtypes = [wintypes.HWND, ctypes.c_int]
    diagnostic = args.build_directory / "sample_virtual_item_state.exe"
    fixture_exe = args.build_directory / "item_predicate_fixture.exe"

    def exercise(name, mode=None, cancel=False):
        owned_console = False
        if cancel:
            owned_console = bool(kernel.AllocConsole())
            if owned_console:
                user.ShowWindow(kernel.GetConsoleWindow(), 0)
        fixture = subprocess.Popen(
            [str(fixture_exe), *([mode] if mode else [])],
            stdout=subprocess.PIPE,
            text=True,
            creationflags=subprocess.CREATE_NO_WINDOW,
        )
        pid, first, second = fixture.stdout.readline().split()
        assert first != second, "Compiler folded fixture sites together"
        heartbeats = []

        def drain():
            for line in fixture.stdout:
                if line.startswith("alive "):
                    heartbeats.append((time.monotonic(), line.strip()))

        reader = threading.Thread(target=drain)
        reader.start()
        log = args.output_directory / f"{name}.jsonl"
        base = [
            str(diagnostic),
            "--pid",
            pid,
            "--seconds",
            "4" if cancel else "1",
            "--output",
            str(log),
            "--dbgeng",
            str(args.engine.resolve()),
        ]
        sites = ["--first-site", "0x" + first, "--second-site", "0x" + second]
        try:
            if name == "guards":
                rejected = subprocess.run(
                    base, capture_output=True, text=True, timeout=15
                )
                assert rejected.returncode == 1
                assert "Target image identity rejected" in rejected.stderr
                assert not log.exists()
                rejected = subprocess.run(
                    base + ["--fixture", "--first-site", "0"],
                    capture_output=True,
                    text=True,
                    timeout=15,
                )
                assert rejected.returncode == 1
                assert "ReadVirtualUncached failed" in rejected.stderr
                log.write_text("preserve\n")
                rejected = subprocess.run(
                    base + ["--fixture", *sites],
                    capture_output=True,
                    text=True,
                    timeout=15,
                )
                assert rejected.returncode == 1
                assert "Cannot create new output" in rejected.stderr
                assert log.read_text() == "preserve\n"
                records = None
            else:
                flags = (
                    subprocess.CREATE_NEW_PROCESS_GROUP
                    if cancel
                    else subprocess.CREATE_NO_WINDOW
                )
                tool = subprocess.Popen(
                    base + ["--fixture", *sites],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    creationflags=flags,
                )
                armed = tool.stdout.readline()
                if "Recording armed" not in armed:
                    stdout, stderr = tool.communicate(timeout=15)
                    raise AssertionError((armed, stdout, stderr))
                if cancel:
                    time.sleep(0.2)
                    assert kernel.GenerateConsoleCtrlEvent(1, tool.pid), (
                        ctypes.get_last_error()
                    )
                stdout, stderr = tool.communicate(timeout=15)
                if mode == "bad-state":
                    assert tool.returncode == 1, (stdout, stderr)
                    assert "Checker vtable rejected" in stderr
                    records = [
                        json.loads(line)
                        for line in log.read_text(encoding="utf-8").splitlines()
                    ]
                    assert records[-1]["kind"] == "failed"
                    assert records[-1]["detach_confirmed"] is True
                    assert any(row["kind"] == "predicate" for row in records)
                else:
                    assert tool.returncode == 0, (stdout, stderr)
                    assert not stderr, stderr
                    records = [
                        json.loads(line) for line in log.read_text().splitlines()
                    ]
                    assert records[0]["fixture"] is True
                    assert records[-1]["kind"] == "detached"
                    assert records[-1]["observations"] == len(records) - 2
                    assert records[-1]["stop_reason"] == (
                        "cancelled" if cancel else "duration"
                    )
                    observations = records[1:-1]
                    if mode == "idle":
                        assert not observations
                    else:
                        assert {(r["phase"], r["result"]) for r in observations} == {
                            (1, 0),
                            (1, 1),
                            (2, 0),
                            (2, 1),
                        }
                        assert all(r["builder_18"] == 314159 for r in observations)
                        assert all(
                            r["first_done_before_store"] == r["phase"] - 1
                            for r in observations
                        )
            finished = time.monotonic()
            time.sleep(0.2)
            assert fixture.poll() is None, "Target exited during diagnostic"
            handle = kernel.OpenProcess(0x400, False, int(pid))
            assert handle, ctypes.get_last_error()
            try:
                debugged = wintypes.BOOL()
                assert kernel.CheckRemoteDebuggerPresent(handle, ctypes.byref(debugged))
                assert not debugged.value, "Debugger remains attached"
            finally:
                kernel.CloseHandle(handle)
            assert any(
                stamp > finished and line == "alive 0" for stamp, line in heartbeats
            ), "Target did not run after detach"
            assert fixture.wait(timeout=10) == 0, "Target did not exit normally"
            print(f"PASS {name}", flush=True)
        finally:
            # Fixtures end on their own; never terminate a target to pass cleanup.
            fixture.wait(timeout=15)
            reader.join(timeout=1)
            if owned_console:
                kernel.FreeConsole()

    for name in (
        [args.case]
        if args.case
        else ["capture", "idle", "bad-state", "guards", "cancel"]
    ):
        exercise(
            name, name if name in {"idle", "bad-state"} else None, name == "cancel"
        )


if __name__ == "__main__":
    main()
