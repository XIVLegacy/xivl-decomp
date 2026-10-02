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
HOOK_KINDS = {
    "setmap",
    "receiver_ignored",
    "region_constructor",
    "region_lookup",
    "lookup_result",
    "partial_error",
}


def event_identity_is_validated(row):
    if "event_thread" not in row:
        return True
    event_thread = row.get("event_thread")
    validation = row.get("identity_validation")
    return (
        isinstance(event_thread, dict)
        and event_thread.get("available") is True
        and event_thread.get("result") == 0
        and isinstance(validation, dict)
        and validation.get("validated") is True
        and row.get("tid_qualification") == "validated_event"
    )


def correlate_handler_delivery(handler_lines, exception_rows):
    first_chance_rows = [row for row in exception_rows if row.get("first_chance") == 1]
    deferred_rows = [row for row in exception_rows if row.get("first_chance") != 1]
    deliveries = []
    matched_rows = set()
    unmatched_handlers = []
    for line in handler_lines:
        fields = line.split()
        if len(fields) != 5 or fields[0] != "single-step-handler":
            unmatched_handlers.append((line, "malformed"))
            continue
        try:
            handler_tid = int(fields[2])
            handler_eip = int(fields[3], 16)
        except ValueError:
            unmatched_handlers.append((line, "malformed"))
            continue
        if fields[4] != "continue-search":
            unmatched_handlers.append((line, "handler did not continue search"))
            continue
        match = next(
            (
                (index, row)
                for index, row in enumerate(first_chance_rows)
                if index not in matched_rows
                and row.get("tid") == handler_tid
                and row.get("exception_address") == handler_eip
                and row.get("debug_context", {}).get("eip", {}).get("available") is True
                and row.get("debug_context", {}).get("eip", {}).get("result") == 0
                and row.get("debug_context", {}).get("eip", {}).get("value")
                == handler_eip
                and event_identity_is_validated(row)
            ),
            None,
        )
        if match is None:
            unmatched_handlers.append((line, "no TID and address match"))
        else:
            index, row = match
            matched_rows.add(index)
            deliveries.append(row)
    unmatched_rows = [
        row for index, row in enumerate(first_chance_rows) if index not in matched_rows
    ]
    if unmatched_handlers or unmatched_rows:
        raise AssertionError(
            (
                "Unmatched handler delivery evidence",
                unmatched_handlers,
                unmatched_rows,
                deferred_rows,
                handler_lines,
                exception_rows,
            )
        )
    return {"deliveries": deliveries, "deferred_rows": deferred_rows}


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
            "exception-delivery",
            "natural-comparison",
            "target-exit",
            "record-cap",
            "retail-refusal",
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
                reader.join(timeout=2)
                raise AssertionError(
                    (
                        "Fixture exited before post-detach check",
                        fixture.returncode,
                        fixture_events[-20:],
                    )
                )
            time.sleep(0.05)
        if fixture.poll() is not None:
            reader.join(timeout=2)
            raise AssertionError(
                (
                    "Fixture exited before post-detach check",
                    fixture.returncode,
                    fixture_events[-20:],
                )
            )
        if debugged(fixture.pid):
            raise AssertionError("Debugger remains attached")
        if not any(
            stamp > finished and line == "alive 0" for stamp, line in heartbeats
        ):
            raise AssertionError("Fixture did not run after detach")
        try:
            exit_code = fixture.wait(timeout=25)
        except subprocess.TimeoutExpired as error:
            reader.join(timeout=2)
            raise AssertionError(
                (
                    "Fixture did not exit before cleanup timeout",
                    fixture.poll(),
                    fixture_events[-20:],
                )
            ) from error
        reader.join(timeout=2)
        if exit_code != 0:
            raise AssertionError(
                ("Fixture did not exit normally", exit_code, fixture_events[-20:])
            )
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

    def assert_fixture_exit(fixture, fixture_events, reader):
        try:
            exit_code = fixture.wait(timeout=15)
        except subprocess.TimeoutExpired as error:
            reader.join(timeout=2)
            raise AssertionError(
                (
                    "Fixture did not exit before target-exit cleanup timeout",
                    fixture.poll(),
                    fixture_events[-20:],
                )
            ) from error
        reader.join(timeout=2)
        if exit_code != 0:
            raise AssertionError(
                ("Fixture did not exit normally", exit_code, fixture_events[-20:])
            )
        if not any(line.startswith("worker-cycles ") for _, line in fixture_events):
            raise AssertionError(
                ("Target exit omitted worker cleanup", fixture_events[-10:])
            )
        if not any(line == "post-detach-cycles 0" for _, line in fixture_events):
            raise AssertionError(("Target exited after detach", fixture_events[-10:]))

    def retain_fixture_result(name, fixture, fixture_events, reader):
        try:
            exit_code = fixture.wait(timeout=15)
        except subprocess.TimeoutExpired as error:
            exit_code = fixture.poll()
            reader.join(timeout=2)
            tail = fixture_events[-20:]
            (args.output_directory / f"{name}-fixture-events.txt").write_text(
                "\n".join(line for _, line in fixture_events) + "\n",
                encoding="utf-8",
            )
            (args.output_directory / f"{name}-fixture-exit.txt").write_text(
                f"exit_code={exit_code}\n" + "\n".join(line for _, line in tail) + "\n",
                encoding="utf-8",
            )
            raise AssertionError(("Fixture did not exit", exit_code, tail)) from error
        reader.join(timeout=2)
        tail = fixture_events[-20:]
        (args.output_directory / f"{name}-fixture-events.txt").write_text(
            "\n".join(line for _, line in fixture_events) + "\n",
            encoding="utf-8",
        )
        (args.output_directory / f"{name}-fixture-exit.txt").write_text(
            f"exit_code={exit_code}\n" + "\n".join(line for _, line in tail) + "\n",
            encoding="utf-8",
        )
        return exit_code

    def preserve_fixture_tail(name, fixture, fixture_events):
        events_path = args.output_directory / f"{name}-fixture-events.txt"
        exit_path = args.output_directory / f"{name}-fixture-exit.txt"
        exit_code = fixture.poll()
        if not events_path.exists():
            events_path.write_text(
                "\n".join(line for _, line in fixture_events) + "\n",
                encoding="utf-8",
            )
        if not exit_path.exists():
            exit_path.write_text(
                f"exit_code={exit_code}\n"
                + "\n".join(line for _, line in fixture_events[-20:])
                + "\n",
                encoding="utf-8",
            )

    def base_command(
        pid,
        sites,
        state,
        log,
        seconds,
        fixture_record_cap=None,
        fixture_initial_threads_only=False,
        fixture_label=None,
    ):
        command = [
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
        if fixture_record_cap is not None:
            command.extend(["--fixture-record-cap", str(fixture_record_cap)])
        if fixture_initial_threads_only:
            command.append("--fixture-initial-threads-only")
        if fixture_label is not None:
            command.extend(["--fixture-label", fixture_label])
        return command

    def load_rows(path):
        return [
            json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()
        ]

    def assert_observation_count(rows):
        terminal = rows[-1]
        hook_rows = [row for row in rows if row.get("kind") in HOOK_KINDS]
        if terminal.get("observations") != len(hook_rows):
            raise AssertionError((terminal, len(hook_rows), hook_rows[-3:]))

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
                    4
                    if cancel
                    else 3
                    if name
                    in {"breakpoint", "record-cap", "target-exit", "retail-refusal"}
                    else 1,
                    12 if name == "record-cap" else None,
                    name == "retail-refusal",
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
                deadline = time.monotonic() + 5.0
                while (
                    not any(line.startswith("workers ") for _, line in fixture_events)
                    and time.monotonic() < deadline
                ):
                    if fixture.poll() is not None:
                        raise AssertionError(
                            "Fixture exited before cancel coordination"
                        )
                    time.sleep(0.05)
                if not any(line.startswith("workers ") for _, line in fixture_events):
                    raise AssertionError(
                        (
                            "Cancel coordination did not create workers",
                            fixture_events[-10:],
                        )
                    )
                if not kernel.GenerateConsoleCtrlEvent(1, tool.pid):
                    raise AssertionError(ctypes.get_last_error())
            stdout, stderr = tool.communicate(timeout=20)
            if name == "retail-refusal":
                if (
                    tool.returncode != 1
                    or not log.exists()
                    or "Application thread creation is outside the bounded four-point profile"
                    not in stderr
                ):
                    raise AssertionError(
                        (tool.returncode, stdout, stderr, fixture_events[-20:])
                    )
                rows = load_rows(log)
                assert_observation_count(rows)
                terminal = rows[-1]
                if (
                    terminal["kind"] != "failed"
                    or not terminal["detach_confirmed"]
                    or terminal["thread_policy"] != "initial_threads_only"
                    or terminal["thread_created_events"] < 1
                    or "Application thread creation is outside the bounded four-point profile"
                    not in terminal.get("error", "")
                ):
                    raise AssertionError((terminal, fixture_events[-20:]))
                if not any(row["kind"] == "thread_created" for row in rows):
                    raise AssertionError((terminal, rows[-5:]))
                finished = time.monotonic()
                assert_fixture_cleanup(
                    fixture,
                    heartbeats,
                    fixture_events,
                    reader,
                    finished,
                    1,
                )
                (
                    args.output_directory / "retail-refusal-fixture-events.txt"
                ).write_text(
                    "\n".join(line for _, line in fixture_events) + "\n",
                    encoding="utf-8",
                )
                post_replays = post_detach_records(fixture_events)
                roles = {role: tid for role, tid in post_replays}
                if not {"main", "worker0"}.issubset(roles) or len(roles) < 3:
                    raise AssertionError((post_replays, fixture_events[-20:]))
                if len(set(roles.values())) != len(roles):
                    raise AssertionError((post_replays, fixture_events[-20:]))
                if not any(line.startswith("workers ") for _, line in fixture_events):
                    raise AssertionError(
                        (
                            "Refusal did not leave later workers alive",
                            fixture_events[-20:],
                        )
                    )
                print("PASS retail-refusal", flush=True)
                return
            if name in {"bad-read", "bad-profile"}:
                if tool.returncode != 1 or not log.exists():
                    raise AssertionError((tool.returncode, stdout, stderr))
                rows = load_rows(log)
                assert_observation_count(rows)
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
                if tool.returncode != 0 or stderr or not log.exists():
                    raise AssertionError((tool.returncode, stdout, stderr))
            elif tool.returncode != 0 or stderr:
                raise AssertionError((tool.returncode, stdout, stderr))
            if name == "target-exit":
                if "Target exited. Observations:" not in stdout:
                    raise AssertionError((stdout, stderr))
            elif "Detached; target left running. Observations:" not in stdout:
                raise AssertionError((stdout, stderr))
            rows = load_rows(log)
            assert_observation_count(rows)
            if rows[0]["fixture"] is not True:
                raise AssertionError(rows[0])
            profiles = [row for row in rows if row["kind"] == "profile"]
            if len(profiles) != 4 or not all(row["passed"] for row in profiles):
                raise AssertionError(profiles)
            terminal = rows[-1]
            expected_terminal_kind = "detached"
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
            if name != "capture":
                expected_reason = (
                    "cancelled"
                    if cancel
                    else "target_exit"
                    if name == "target-exit"
                    else "record_cap"
                    if name == "record-cap"
                    else "duration"
                )
                if terminal["stop_reason"] != expected_reason:
                    raise AssertionError(terminal)
            if name == "record-cap":
                hook_rows = [row for row in rows if row.get("kind") in HOOK_KINDS]
                lifecycle_rows = [
                    row
                    for row in rows
                    if row.get("kind") in {"thread_created", "thread_exited"}
                ]
                record_rows = [
                    row
                    for row in rows
                    if row.get("kind")
                    not in {"identity", "profile", "detached", "failed"}
                ]
                if (
                    terminal["stop_reason"] != "record_cap"
                    or terminal["record_cap"] != 12
                    or terminal["records"] != terminal["record_cap"]
                    or terminal["observations"] != 10
                    or len(record_rows) != terminal["records"]
                    or len(hook_rows) != 10
                    or len(lifecycle_rows) != 2
                    or record_rows[-1]["kind"] != "thread_created"
                ):
                    raise AssertionError(
                        (terminal, record_rows, hook_rows, lifecycle_rows)
                    )
                finished = time.monotonic()
                assert_fixture_cleanup(
                    fixture,
                    heartbeats,
                    fixture_events,
                    reader,
                    finished,
                    1,
                )
                (args.output_directory / "record-cap-fixture-events.txt").write_text(
                    "\n".join(line for _, line in fixture_events) + "\n",
                    encoding="utf-8",
                )
                if not any(
                    line.startswith("record-cap-worker ") for _, line in fixture_events
                ):
                    raise AssertionError((terminal, fixture_events[-10:]))
                print("PASS record-cap", flush=True)
                return
            observations = [
                row
                for row in rows
                if row["kind"]
                not in {
                    "identity",
                    "profile",
                    "detached",
                    "failed",
                    "thread_created",
                    "thread_exited",
                    "target_exception",
                }
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
            if name == "target-exit":
                assert_fixture_exit(fixture, fixture_events, reader)
            else:
                assert_fixture_cleanup(
                    fixture,
                    heartbeats,
                    fixture_events,
                    reader,
                    finished,
                    3 if name == "capture" else 1,
                )
            (args.output_directory / f"{name}-fixture-events.txt").write_text(
                "\n".join(line for _, line in fixture_events) + "\n",
                encoding="utf-8",
            )
            post_replays = post_detach_records(fixture_events)
            if name == "capture":
                roles = {role: tid for role, tid in post_replays}
                if set(roles) != {"main", "worker0", "worker2"}:
                    raise AssertionError((post_replays, fixture_events[-10:]))
                if len(set(roles.values())) != 3:
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
                    or int(worker_ids[2]) != roles["worker2"]
                ):
                    raise AssertionError((workers, post_replays, fixture_events[-10:]))
                worker1_id = int(worker_ids[1])
                worker2_id = int(worker_ids[2])
                lifecycle_created = [
                    row for row in rows if row["kind"] == "thread_created"
                ]
                lifecycle_exited = [
                    row for row in rows if row["kind"] == "thread_exited"
                ]
                if (
                    terminal["thread_policy"] != "global_all_threads"
                    or terminal["thread_created_events"] < 2
                    or terminal["thread_exit_events"] < 1
                    or not any(row["tid"] == worker1_id for row in lifecycle_created)
                    or not any(row["tid"] == worker2_id for row in lifecycle_created)
                    or not any(row["tid"] == worker1_id for row in lifecycle_exited)
                ):
                    raise AssertionError(
                        (terminal, lifecycle_created, lifecycle_exited)
                    )
                required_kinds = {
                    "setmap",
                    "region_constructor",
                    "region_lookup",
                    "lookup_result",
                }
                attached_by_tid = {}
                for row in observations:
                    attached_by_tid.setdefault(row["tid"], set()).add(row["kind"])
                for tid in (roles["worker0"], worker1_id, worker2_id):
                    if required_kinds - attached_by_tid.get(tid, set()):
                        raise AssertionError((tid, attached_by_tid, observations))
                if not any(
                    line.startswith("worker-exit worker1 ")
                    for _, line in fixture_events
                ):
                    raise AssertionError(
                        ("worker1 did not exit while attached", fixture_events[-10:])
                    )
                if not any(
                    line.startswith("worker-exception-handled 1 ")
                    for _, line in fixture_events
                ):
                    raise AssertionError(
                        ("worker exception was not handled", fixture_events[-10:])
                    )
                target_exceptions = [
                    row for row in rows if row["kind"] == "target_exception"
                ]
                if not any(
                    row["tid"] == worker2_id and row["exception_code"] == 0x80000003
                    for row in target_exceptions
                ):
                    raise AssertionError((worker2_id, target_exceptions))
            elif name == "idle" and len(post_replays) != 1:
                raise AssertionError((post_replays, fixture_events[-10:]))
            elif name == "cancel":
                if (
                    terminal["thread_policy"] != "global_all_threads"
                    or terminal["thread_created_events"] < 1
                ):
                    raise AssertionError(terminal)
                if not {role for role, _ in post_replays}.intersection(
                    {"worker0", "worker1", "worker2"}
                ):
                    raise AssertionError((post_replays, fixture_events[-10:]))
            elif name == "target-exit":
                if (
                    not terminal["process_exited"]
                    or terminal["thread_policy"] != "global_all_threads"
                ):
                    raise AssertionError(terminal)
                if not any(
                    line.startswith("worker-exit worker1 ")
                    for _, line in fixture_events
                ) or not any(
                    line.startswith("worker-exception-handled 1 ")
                    for _, line in fixture_events
                ):
                    raise AssertionError((terminal, fixture_events[-10:]))
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
            try:
                if tool is not None and tool.poll() is None:
                    tool.wait(timeout=20)
                if fixture.poll() is None:
                    fixture.wait(timeout=15)
            finally:
                try:
                    reader.join(timeout=1)
                finally:
                    try:
                        preserve_fixture_tail(name, fixture, fixture_events)
                    finally:
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
            assert_observation_count(rows)
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

    def exercise_exception_delivery(include_forced, include_natural):
        def assert_exception_context(rows):
            exceptions = [row for row in rows if row["kind"] == "target_exception"]
            for row in exceptions:
                context = row.get("debug_context", {})
                if set(context) != {
                    "eip",
                    "eflags",
                    "dr0",
                    "dr1",
                    "dr2",
                    "dr3",
                    "dr4",
                    "dr5",
                    "dr6",
                    "dr7",
                }:
                    raise AssertionError(row)
                if any(
                    set(value) != {"available", "result", "value"}
                    for value in context.values()
                ):
                    raise AssertionError(row)
                if not row.get("forwarded") or row.get("breakpoint", {}).get(
                    "callback"
                ):
                    raise AssertionError(row)
            return exceptions

        controls = (
            (
                (
                    "intentional-tf-no-handler",
                    "single-step-tf",
                    "intentional-tf-single-step-no-handler",
                    False,
                ),
                (
                    "intentional-tf-handler",
                    "single-step-tf-handler",
                    "intentional-tf-single-step-handler",
                    False,
                ),
                (
                    "application-exception",
                    "application-exception",
                    "intentional-application-exception",
                    True,
                ),
            )
            if include_forced
            else ()
        )
        for name, mode, label, intentional in controls:
            fixture, reader, heartbeats, fixture_events, pid, sites, state = (
                parse_fixture(mode)
            )
            log = args.output_directory / f"{name}.jsonl"
            tool = None
            try:
                tool = subprocess.Popen(
                    base_command(
                        pid,
                        sites,
                        state,
                        log,
                        3,
                        fixture_label=label,
                    ),
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    creationflags=subprocess.CREATE_NO_WINDOW,
                )
                armed = tool.stdout.readline()
                if "Recording armed" not in armed:
                    stdout, stderr = tool.communicate(timeout=20)
                    raise AssertionError((armed, stdout, stderr))
                stdout, stderr = tool.communicate(timeout=20)
                if tool.returncode != 0 or stderr or not log.exists():
                    raise AssertionError((tool.returncode, stdout, stderr))
                rows = load_rows(log)
                assert_observation_count(rows)
                if rows[0].get("fixture_label") != label:
                    raise AssertionError(rows[0])
                exceptions = assert_exception_context(rows)
                expected_code = 0x80000003 if intentional else 0x80000004
                if not exceptions or any(
                    row["exception_code"] != expected_code for row in exceptions
                ):
                    raise AssertionError((expected_code, exceptions))
                terminal = rows[-1]
                if terminal["kind"] != "detached" or not terminal.get(
                    "detach_confirmed"
                ):
                    raise AssertionError(terminal)
                if terminal.get("forwarded_exception_count", 0) < len(exceptions):
                    raise AssertionError(terminal)
                if intentional:
                    if terminal.get("process_exited"):
                        raise AssertionError(terminal)
                    finished = time.monotonic()
                    assert_fixture_cleanup(
                        fixture, heartbeats, fixture_events, reader, finished, 1
                    )
                    exit_code = retain_fixture_result(
                        name, fixture, fixture_events, reader
                    )
                    if exit_code != 0 or not any(
                        line.startswith("application-exception-handled 1")
                        for _, line in fixture_events
                    ):
                        raise AssertionError((exit_code, fixture_events[-20:]))
                else:
                    exit_code = retain_fixture_result(
                        name, fixture, fixture_events, reader
                    )
                    if exit_code == 0:
                        raise AssertionError(
                            (
                                "Intentional TF control unexpectedly exited normally",
                                fixture_events[-20:],
                            )
                        )
                    handler_lines = [
                        line
                        for _, line in fixture_events
                        if line.startswith("single-step-handler ")
                    ]
                    if mode == "single-step-tf-handler":
                        if not any(
                            line.endswith("continue-search") for line in handler_lines
                        ):
                            raise AssertionError(
                                (
                                    "Handler delivery was not logged",
                                    fixture_events[-20:],
                                )
                            )
                        correlation = correlate_handler_delivery(
                            handler_lines, exceptions
                        )
                        if len(correlation["deliveries"]) != len(handler_lines):
                            raise AssertionError(correlation)
                    elif handler_lines:
                        raise AssertionError(
                            (
                                "No-handler control installed a handler",
                                fixture_events[-20:],
                            )
                        )
                print(f"PASS {name}", flush=True)
            finally:
                try:
                    if tool is not None and tool.poll() is None:
                        tool.wait(timeout=20)
                    if fixture.poll() is None:
                        fixture.wait(timeout=15)
                finally:
                    reader.join(timeout=1)
                    preserve_fixture_tail(name, fixture, fixture_events)

        natural_controls = (
            ("natural-no-handler", None, "natural-single-step-no-handler"),
            ("natural-handler", "natural-handler", "natural-single-step-handler"),
        )
        if not include_natural:
            natural_controls = ()
        for name, mode, label in natural_controls:
            fixture, reader, heartbeats, fixture_events, pid, sites, state = (
                parse_fixture(mode)
            )
            log = args.output_directory / f"{name}.jsonl"
            tool = None
            try:
                tool = subprocess.Popen(
                    base_command(
                        pid,
                        sites,
                        state,
                        log,
                        3,
                        fixture_label=label,
                    ),
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    creationflags=subprocess.CREATE_NO_WINDOW,
                )
                armed = tool.stdout.readline()
                if "Recording armed" not in armed:
                    stdout, stderr = tool.communicate(timeout=20)
                    raise AssertionError((armed, stdout, stderr))
                stdout, stderr = tool.communicate(timeout=20)
                if tool.returncode != 0 or stderr or not log.exists():
                    raise AssertionError((tool.returncode, stdout, stderr))
                rows = load_rows(log)
                assert_observation_count(rows)
                if rows[0].get("fixture_label") != label:
                    raise AssertionError(rows[0])
                terminal = rows[-1]
                if terminal["kind"] != "detached" or not terminal.get(
                    "detach_confirmed"
                ):
                    raise AssertionError(terminal)
                exceptions = assert_exception_context(rows)
                single_steps = [
                    row for row in exceptions if row["exception_code"] == 0x80000004
                ]
                handler_lines = [
                    line
                    for _, line in fixture_events
                    if line.startswith("single-step-handler ")
                ]
                if mode is None and handler_lines:
                    raise AssertionError(
                        (
                            "Unchanged no-handler control logged delivery",
                            fixture_events[-20:],
                        )
                    )
                if not single_steps:
                    if handler_lines:
                        correlate_handler_delivery(handler_lines, single_steps)
                    print(
                        f"NO OCCURRENCE {name}: no spontaneous EXCEPTION_SINGLE_STEP; origin and delivery discriminator unavailable",
                        flush=True,
                    )
                elif mode == "natural-handler":
                    if not handler_lines:
                        raise AssertionError(
                            (
                                "Observed natural single-step without handler delivery evidence",
                                single_steps,
                                handler_lines,
                            )
                        )
                    correlation = correlate_handler_delivery(
                        handler_lines, single_steps
                    )
                    if len(correlation["deliveries"]) != len(handler_lines):
                        raise AssertionError(correlation)
                if fixture.poll() is None:
                    finished = time.monotonic()
                    assert_fixture_cleanup(
                        fixture, heartbeats, fixture_events, reader, finished, 1
                    )
                exit_code = retain_fixture_result(name, fixture, fixture_events, reader)
                if mode is None and exit_code != 0:
                    raise AssertionError((exit_code, fixture_events[-20:]))
                print(f"PASS {name}", flush=True)
            finally:
                try:
                    if tool is not None and tool.poll() is None:
                        tool.wait(timeout=20)
                    if fixture.poll() is None:
                        fixture.wait(timeout=15)
                finally:
                    reader.join(timeout=1)
                    preserve_fixture_tail(name, fixture, fixture_events)

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
            "target-exit",
            "record-cap",
            "retail-refusal",
        ]
    )
    for name in names:
        if name == "guards":
            guards()
        elif name == "exception-delivery":
            exercise_exception_delivery(include_forced=True, include_natural=False)
        elif name == "natural-comparison":
            exercise_exception_delivery(include_forced=False, include_natural=True)
        else:
            exercise(
                name,
                name
                if name
                in {
                    "idle",
                    "bad-read",
                    "bad-profile",
                    "cancel",
                    "breakpoint",
                    "target-exit",
                    "record-cap",
                }
                else None,
                name == "cancel",
            )


if __name__ == "__main__":
    main()
