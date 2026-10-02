#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Exercise the asset-free native map-selection diagnostic."""

import argparse
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import subprocess
import threading
import time


FIRST_REGION = 0x13572468
FIRST_ZONE = 0x24681357
FIRST_MODE = 0xA5
SECOND_REGION = 0x89ABCDEF
SECOND_ZONE = 0x10203040
SECOND_MODE = 0x5A
EXPECTED_VTABLE = 0x0BADF00D


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-directory", type=Path, required=True)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--output-directory", type=Path, required=True)
    parser.add_argument(
        "--case",
        choices=[
            "capture",
            "idle",
            "bad-read",
            "bad-profile",
            "guards",
            "cancel",
            "breakpoint",
        ],
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
    kernel.GenerateConsoleCtrlEvent.argtypes = [wintypes.DWORD, wintypes.DWORD]
    kernel.GenerateConsoleCtrlEvent.restype = wintypes.BOOL
    kernel.AllocConsole.restype = wintypes.BOOL
    kernel.FreeConsole.restype = wintypes.BOOL
    kernel.GetConsoleWindow.restype = wintypes.HWND
    user = ctypes.WinDLL("user32", use_last_error=True)
    user.ShowWindow.argtypes = [wintypes.HWND, ctypes.c_int]

    diagnostic = args.build_directory / "trace_map_selection.exe"
    fixture_exe = args.build_directory / "map_selection_fixture.exe"

    def parse_fixture(mode=None):
        fixture = subprocess.Popen(
            [str(fixture_exe), *([mode] if mode else [])],
            stdout=subprocess.PIPE,
            text=True,
            creationflags=subprocess.CREATE_NO_WINDOW,
        )
        first = fixture.stdout.readline().split()
        if len(first) != 5:
            raise AssertionError(first)
        pid = int(first[0])
        sites = [int(value, 16) for value in first[1:]]
        if len(set(sites)) != 4:
            raise AssertionError("Compiler folded fixture sites together")
        state = fixture.stdout.readline().split()
        if state[0] != "state" or len(state) != 19:
            raise AssertionError(state)
        values = [int(value, 16) for value in state[1:]]
        names = [
            "scene_global",
            "manager_vtable",
            "manager",
            "matched_region",
            "other_manager",
            "table",
            "root_begin",
            "root_end",
            "element",
            "first_game",
            "second_game",
            "first_region",
            "first_zone",
            "first_mode",
            "second_region",
            "second_zone",
            "second_mode",
            "scene",
        ]
        state_values = dict(zip(names, values, strict=True))
        heartbeats = []
        fixture_events = []

        def drain():
            for line in fixture.stdout:
                fixture_events.append((time.monotonic(), line.strip()))
                if line.startswith("alive "):
                    heartbeats.append((time.monotonic(), line.strip()))

        reader = threading.Thread(target=drain)
        reader.start()
        return fixture, reader, heartbeats, fixture_events, pid, sites, state_values

    def debugged(pid):
        handle = kernel.OpenProcess(0x400, False, pid)
        if not handle:
            raise AssertionError(ctypes.get_last_error())
        try:
            value = wintypes.BOOL()
            if not kernel.CheckRemoteDebuggerPresent(handle, ctypes.byref(value)):
                raise AssertionError(ctypes.get_last_error())
            return bool(value.value)
        finally:
            kernel.CloseHandle(handle)

    def assert_fixture_cleanup(
        fixture, heartbeats, fixture_events, reader, finished, minimum_replays=0
    ):
        deadline = time.monotonic() + 2.0
        while (
            not any(
                stamp > finished and line == "alive 0" for stamp, line in heartbeats
            )
            and time.monotonic() < deadline
        ):
            if fixture.poll() is not None:
                raise AssertionError("Fixture exited before post-detach check")
            time.sleep(0.05)
        if fixture.poll() is not None:
            raise AssertionError("Fixture exited before post-detach check")
        if debugged(fixture.pid):
            raise AssertionError("Debugger remains attached")
        if not any(
            stamp > finished and line == "alive 0" for stamp, line in heartbeats
        ):
            raise AssertionError("Fixture did not run after detach")
        if fixture.wait(timeout=25) != 0:
            raise AssertionError("Fixture did not exit normally")
        reader.join(timeout=2)
        replay = [
            line for _, line in fixture_events if line.startswith("post-detach-cycles ")
        ]
        if replay and int(replay[-1].split()[1]) < minimum_replays:
            raise AssertionError(
                ("Fixture did not replay all seams after detach", replay)
            )
        if minimum_replays and not replay:
            raise AssertionError(
                ("Fixture did not replay all seams after detach", replay)
            )

    def base_command(pid, sites, state, log, seconds):
        return [
            str(diagnostic),
            "--pid",
            str(pid),
            "--seconds",
            str(seconds),
            "--output",
            str(log),
            "--dbgeng",
            str(args.engine.resolve()),
            "--fixture",
            "--setmap-site",
            hex(sites[0]),
            "--constructor-site",
            hex(sites[1]),
            "--lookup-site",
            hex(sites[2]),
            "--result-site",
            hex(sites[3]),
            "--scene-global",
            hex(state["scene_global"]),
            "--manager-vtable",
            hex(state["manager_vtable"]),
        ]

    def load_rows(path):
        return [
            json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()
        ]

    def post_detach_records(fixture_events):
        records = []
        for _, line in fixture_events:
            fields = line.split()
            if len(fields) != 7 or fields[0] != "post-detach-thread":
                continue
            if fields[3:] != ["setmap", "constructor", "lookup", "result"]:
                continue
            records.append((fields[1], int(fields[2])))
        return records

    def exercise(name, mode=None, cancel=False):
        fixture, reader, heartbeats, fixture_events, pid, sites, state = parse_fixture(
            mode
        )
        log = args.output_directory / f"{name}.jsonl"
        tool = None
        owned_console = False
        try:
            flags = (
                subprocess.CREATE_NEW_PROCESS_GROUP
                if cancel
                else subprocess.CREATE_NO_WINDOW
            )
            if cancel:
                owned_console = bool(kernel.AllocConsole())
                if owned_console:
                    user.ShowWindow(kernel.GetConsoleWindow(), 0)
            tool = subprocess.Popen(
                base_command(
                    pid,
                    sites,
                    state,
                    log,
                    4 if cancel else 3 if name == "breakpoint" else 1,
                ),
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
                time.sleep(0.75)
                if not kernel.GenerateConsoleCtrlEvent(1, tool.pid):
                    raise AssertionError(ctypes.get_last_error())
            stdout, stderr = tool.communicate(timeout=20)
            if name in {"bad-read", "bad-profile"}:
                if tool.returncode != 1 or not log.exists():
                    raise AssertionError((tool.returncode, stdout, stderr))
                rows = load_rows(log)
                if rows[-1]["kind"] != "failed" or not rows[-1]["detach_confirmed"]:
                    raise AssertionError(rows[-1])
                if (
                    name == "bad-read"
                    and "ReadVirtualUncached" not in rows[-1]["error"]
                    and "Partial" not in rows[-1]["error"]
                ):
                    raise AssertionError(rows[-1])
                if name == "bad-profile" and "vtable" not in rows[-1]["error"]:
                    raise AssertionError(rows[-1])
                assert_fixture_cleanup(
                    fixture,
                    heartbeats,
                    fixture_events,
                    reader,
                    time.monotonic(),
                    1,
                )
                if name == "bad-profile":
                    exercise_attachment_exception()
                return
            if name == "capture":
                if tool.returncode != 1 or not stderr or not log.exists():
                    raise AssertionError((tool.returncode, stdout, stderr))
            elif tool.returncode != 0 or stderr:
                raise AssertionError((tool.returncode, stdout, stderr))
            rows = load_rows(log)
            if rows[0]["fixture"] is not True:
                raise AssertionError(rows[0])
            profiles = [row for row in rows if row["kind"] == "profile"]
            if len(profiles) != 4 or not all(row["passed"] for row in profiles):
                raise AssertionError(profiles)
            terminal = rows[-1]
            expected_terminal_kind = "failed" if name == "capture" else "detached"
            if (
                terminal["kind"] != expected_terminal_kind
                or not terminal["detach_confirmed"]
            ):
                raise AssertionError(terminal)
            if (
                terminal.get("attachment_validated") is not True
                or not terminal.get("observer_breakin_thread_id")
                or terminal.get("attach_thread_start_offset")
                != terminal.get("expected_helper_start")
            ):
                raise AssertionError(terminal)
            if name == "capture":
                if "thread creation" not in terminal["error"]:
                    raise AssertionError(terminal)
            else:
                expected_reason = "cancelled" if cancel else "duration"
                if terminal["stop_reason"] != expected_reason:
                    raise AssertionError(terminal)
            observations = [
                row
                for row in rows
                if row["kind"] not in {"identity", "profile", "detached", "failed"}
            ]
            if name == "idle":
                if observations:
                    raise AssertionError(observations)
            else:
                setmaps = [row for row in observations if row["kind"] == "setmap"]
                ignored = [
                    row for row in observations if row["kind"] == "receiver_ignored"
                ]
                constructors = [
                    row for row in observations if row["kind"] == "region_constructor"
                ]
                lookups = [
                    row for row in observations if row["kind"] == "region_lookup"
                ]
                results = [
                    row for row in observations if row["kind"] == "lookup_result"
                ]
                if (
                    not setmaps
                    or not ignored
                    or not constructors
                    or len(lookups) < 3
                    or len(results) < 3
                ):
                    raise AssertionError(observations)
                if name == "capture":
                    if terminal["initial_thread_count"] < 2:
                        raise AssertionError(terminal)
                    by_tid = {}
                    for row in observations:
                        by_tid.setdefault(row["tid"], set()).add(row["kind"])
                    required_kinds = {
                        "setmap",
                        "region_constructor",
                        "region_lookup",
                        "lookup_result",
                    }
                    if len(by_tid) < 2 or any(
                        required_kinds - kinds for kinds in by_tid.values()
                    ):
                        raise AssertionError(observations)
                elif (
                    name not in {"cancel", "breakpoint"}
                    and len({row["tid"] for row in observations}) < 2
                ):
                    raise AssertionError(
                        "Fixture did not exercise a post-arming worker thread"
                    )
                if terminal["ignored_receiver_hits"] != len(ignored):
                    raise AssertionError(terminal)
                if terminal["receiver_hits"] != len(setmaps) + len(ignored):
                    raise AssertionError(terminal)
                for row in setmaps:
                    if (
                        len(row["game_header_hex"]) != 32
                        or len(row["application_hex"]) != 32
                    ):
                        raise AssertionError(row)
                    if row["application_base"] != row["game_header_base"] + 0x10:
                        raise AssertionError(row)
                    if (
                        row["integration_outer_header_size"] != 16
                        or row["integration_subpacket_size"] != 48
                    ):
                        raise AssertionError(row)
                    if row["captured_game_application_size"] != 32:
                        raise AssertionError(row)
                    if (
                        "11121314" in row["game_header_hex"]
                        or "2728292a" in row["game_header_hex"]
                    ):
                        raise AssertionError(
                            "Outer header bytes leaked into game header"
                        )
                values = {(row["region"], row["zone"], row["mode"]) for row in setmaps}
                if not {
                    (FIRST_REGION, FIRST_ZONE, FIRST_MODE),
                    (SECOND_REGION, SECOND_ZONE, SECOND_MODE),
                }.issubset(values):
                    raise AssertionError(values)
                constructor = constructors[0]
                if (
                    constructor["region_argument"] != FIRST_REGION
                    or constructor["scene_region_before"] != 0x11112222
                ):
                    raise AssertionError(constructor)
                if constructor["scene_manager_before"] == state["manager"]:
                    raise AssertionError(constructor)
                known = [row for row in lookups if row["known_manager_caller"]]
                if not known or any(
                    row["query_full_dword"] != FIRST_REGION for row in known[:1]
                ):
                    raise AssertionError(lookups)
                if any(
                    row["root_pointer_begin"] != state["root_begin"]
                    or row["root_pointer_end"] != state["root_end"]
                    for row in lookups
                ):
                    raise AssertionError(lookups)
                matched = [
                    row
                    for row in results
                    if not row["null_result"] and row.get("manager_context") == "known"
                ]
                nulls = [
                    row
                    for row in results
                    if row["null_result"] and row.get("manager_context") == "known"
                ]
                unrelated = [
                    row for row in results if row.get("manager_context") == "unresolved"
                ]
                if not matched or not nulls or not unrelated:
                    raise AssertionError(results)
                if any(
                    not row["exact_lookup_join"]
                    or row["lookup_join_esp"] != row["result_esp"]
                    for row in matched + nulls
                ):
                    raise AssertionError(results)
                if (
                    matched[0]["matched_root_key"] != FIRST_REGION
                    or matched[0]["matched_auxiliary"] != 0x0A0B0C0D
                ):
                    raise AssertionError(matched[0])
                if (
                    nulls[0]["matched_root_key"] is not None
                    or nulls[0]["matched_auxiliary"] is not None
                ):
                    raise AssertionError(nulls[0])
                if any("manager_region" in row for row in unrelated):
                    raise AssertionError(unrelated)
            finished = time.monotonic()
            assert_fixture_cleanup(
                fixture,
                heartbeats,
                fixture_events,
                reader,
                finished,
                4 if name == "capture" else 1,
            )
            post_replays = post_detach_records(fixture_events)
            if name == "capture":
                roles = {role: tid for role, tid in post_replays}
                if set(roles) != {"main", "worker0", "worker1", "worker2"}:
                    raise AssertionError((post_replays, fixture_events[-10:]))
                if len(set(roles.values())) != 4:
                    raise AssertionError((post_replays, fixture_events[-10:]))
                workers = [
                    line for _, line in fixture_events if line.startswith("workers ")
                ]
                initial_workers = [
                    line
                    for _, line in fixture_events
                    if line.startswith("initial-worker ")
                ]
                worker_ids = workers[-1].split() if workers else []
                initial_worker_id = (
                    initial_workers[-1].split()[1] if initial_workers else ""
                )
                if (
                    len(worker_ids) != 3
                    or not initial_worker_id
                    or int(initial_worker_id) != roles["worker0"]
                    or int(worker_ids[1]) != roles["worker1"]
                    or int(worker_ids[2]) != roles["worker2"]
                ):
                    raise AssertionError((workers, post_replays, fixture_events[-10:]))
            elif name == "idle" and len(post_replays) != 1:
                raise AssertionError((post_replays, fixture_events[-10:]))
            if name == "capture":
                if not any(
                    line.startswith("post-detach-cycles ")
                    and int(line.split()[1]) == len(post_replays)
                    for _, line in fixture_events
                ):
                    raise AssertionError((post_replays, fixture_events[-10:]))
            if name == "breakpoint" and not any(
                line.startswith("breakpoint-handled 1") for _, line in fixture_events
            ):
                raise AssertionError(
                    ("Breakpoint handler did not run", fixture_events[-10:])
                )
            if name == "breakpoint" and not any(
                line == "breakpoint-before 1" for _, line in fixture_events
            ):
                raise AssertionError(
                    (
                        "Breakpoint was not forwarded while attached",
                        fixture_events[-10:],
                    )
                )
            print(f"PASS {name}", flush=True)
        finally:
            if tool is not None and tool.poll() is None:
                tool.wait(timeout=20)
            if fixture.poll() is None:
                fixture.wait(timeout=15)
            reader.join(timeout=1)
            if owned_console:
                kernel.FreeConsole()

    def exercise_attachment_exception():
        fixture, reader, heartbeats, fixture_events, pid, sites, state = parse_fixture(
            "attach-exception"
        )
        log = args.output_directory / "attachment-exception.jsonl"
        tool = None
        try:
            command = base_command(pid, sites, state, log, 1)
            command[command.index("--setmap-site") + 1] = hex(sites[0] + 1)
            tool = subprocess.Popen(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                creationflags=subprocess.CREATE_NO_WINDOW,
            )
            stdout, stderr = tool.communicate(timeout=15)
            if (
                tool.returncode != 1
                or not log.exists()
                or "Loaded fixture code/profile mismatch" not in stderr
            ):
                raise AssertionError((tool.returncode, stdout, stderr))
            rows = load_rows(log)
            if rows[-1]["kind"] != "failed" or not rows[-1]["detach_confirmed"]:
                raise AssertionError(rows[-1])
            if rows[-1].get("attachment_validated") is not False:
                raise AssertionError(rows[-1])
            finished = time.monotonic()
            assert_fixture_cleanup(
                fixture, heartbeats, fixture_events, reader, finished, 1
            )
            if not any(line == "attachment-handled 1" for _, line in fixture_events):
                raise AssertionError(
                    ("Attachment breakpoint was not forwarded", fixture_events[-10:])
                )
            print("PASS attachment-exception", flush=True)
        finally:
            if tool is not None and tool.poll() is None:
                tool.wait(timeout=20)
            if fixture.poll() is None:
                fixture.wait(timeout=15)
            reader.join(timeout=1)

    def guards():
        fixture, reader, heartbeats, fixture_events, pid, sites, state = parse_fixture(
            "guards"
        )
        try:
            identity_log = args.output_directory / "identity.jsonl"
            rejected = subprocess.run(
                [
                    str(diagnostic),
                    "--pid",
                    str(pid),
                    "--seconds",
                    "1",
                    "--output",
                    str(identity_log),
                    "--dbgeng",
                    str(args.engine.resolve()),
                ],
                capture_output=True,
                text=True,
                timeout=15,
            )
            if (
                rejected.returncode != 1
                or "Target image identity rejected" not in rejected.stderr
                or identity_log.exists()
            ):
                raise AssertionError(
                    (rejected.returncode, rejected.stdout, rejected.stderr)
                )
            rebased_log = args.output_directory / "rebase.jsonl"
            rejected = subprocess.run(
                base_command(pid, sites, state, rebased_log, 1)[:]
                + ["--setmap-site", hex(sites[0] + 1)],
                capture_output=True,
                text=True,
                timeout=15,
            )
            if (
                rejected.returncode != 1
                or "Loaded fixture code/profile mismatch" not in rejected.stderr
            ):
                raise AssertionError(
                    (rejected.returncode, rejected.stdout, rejected.stderr)
                )
            if debugged(pid):
                raise AssertionError("Profile rejection left debugger attached")
            existing = args.output_directory / "existing.jsonl"
            existing.write_text("preserve\n", encoding="utf-8")
            rejected = subprocess.run(
                base_command(pid, sites, state, existing, 1),
                capture_output=True,
                text=True,
                timeout=15,
            )
            if (
                rejected.returncode != 1
                or "Cannot create new output" not in rejected.stderr
            ):
                raise AssertionError(
                    (rejected.returncode, rejected.stdout, rejected.stderr)
                )
            if existing.read_text(encoding="utf-8") != "preserve\n":
                raise AssertionError("Existing output was modified")
            invalid = subprocess.run(
                [
                    str(diagnostic),
                    "--pid",
                    str(pid),
                    "--seconds",
                    "0",
                    "--output",
                    str(args.output_directory / "invalid.jsonl"),
                    "--dbgeng",
                    str(args.engine.resolve()),
                ],
                capture_output=True,
                text=True,
                timeout=15,
            )
            if invalid.returncode != 1 or "seconds 1..30" not in invalid.stderr:
                raise AssertionError(
                    (invalid.returncode, invalid.stdout, invalid.stderr)
                )
            if debugged(pid):
                raise AssertionError("Guard checks attached to fixture")
            print("PASS guards", flush=True)
        finally:
            finished = time.monotonic()
            assert_fixture_cleanup(
                fixture, heartbeats, fixture_events, reader, finished
            )
            reader.join(timeout=1)

    names = (
        [args.case]
        if args.case
        else [
            "capture",
            "idle",
            "bad-read",
            "bad-profile",
            "guards",
            "cancel",
            "breakpoint",
        ]
    )
    for name in names:
        if name == "guards":
            guards()
        else:
            exercise(
                name,
                name
                if name in {"idle", "bad-read", "bad-profile", "cancel", "breakpoint"}
                else None,
                name == "cancel",
            )


if __name__ == "__main__":
    main()
