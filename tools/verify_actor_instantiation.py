#!/usr/bin/env python3
"""Validate the actor instantiation contract, its reference models and optional exe bytes."""

from __future__ import annotations

import argparse
import copy
import json
import re
import struct
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "actor_instantiation.json"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
SHA_RE = re.compile(r"^[0-9a-f]{64}$")
HEX8_RE = re.compile(r"^0x[0-9a-f]{8}$")
SECTIONS = (
    "record",
    "stream",
    "selfPlayer",
    "queue",
    "creation",
    "desktopWidget",
    "getStaticActor",
    "observed",
)
LOCAL_DESKTOP_KEY = 0xC0000024
APPLICATION_BYTES = 264
TEXT_BYTES = 32
STREAM_OFFSET = 0x44
STREAM_BYTES = 192
SELF_WORDS = 20
TAG_END = 0x0F
TAG_STOP_MIN = 0x0D
EXPECTED_RECORD = {
    "opcode": "0x00cc",
    "headerBytes": "0x10",
    "dispatcherArmVa": "0x004d88e2",
    "wrapperVa": "0x00574780",
    "wrapperCallVa": "0x005747fb",
    "applyVa": "0x00774ad0",
    "firstDword": {
        "offset": "0x00",
        "store": "implementation+0x14, copied without conversion",
        "meaning": "untraced",
    },
    "nameOffset": "0x04",
    "classOffset": "0x24",
    "textBytes": TEXT_BYTES,
    "streamOffset": "0x44",
    "streamBytes": STREAM_BYTES,
    "applicationBytes": APPLICATION_BYTES,
    "terminatorGuard": {
        "va": "0x0078fa80",
        "rule": "The last nonzero byte of the 192-byte stream span must be 0x0f or 0xff.",
        "failure": "The literal [clientversionerror] is passed to 0x00cc7ea0 and the record submits no request.",
    },
}
EXPECTED_STEPS = [
    ("path", "0x0078f8d0"),
    ("control A", "0x0078f840"),
    ("control B", "0x0078f840"),
    ("setup bundle", None),
    ("control C", "0x0078f840"),
    ("word C", "0x0078f900"),
    ("control D", "0x0078f840"),
    ("word D", "0x0078f870"),
    ("control E", "0x0078f840"),
    ("word E", "0x0078f870"),
    ("self block", None),
    ("values", "0x00790aa0"),
]
EXPECTED_BUNDLE_READERS = [
    "0x0078f8d0",
    "0x0078f8d0",
    "0x0078f870",
    "0x0078fa30",
    "0x0078f840",
    "0x0078f840",
]
EXPECTED_TAGS = {
    "0x00": "signed 32-bit integer, Lua number",
    "0x01": "32-bit float, loaded with fld dword at 0x00790786, Lua number",
    "0x02": "NUL terminated string",
    "0x03": "true",
    "0x04": "false",
    "0x05": "nil",
    "0x06": "actor key resolved to an instance table, an unresolved key sets the visitor error flag and still appends a null binding in the same argument slot",
    "0x07": "own visitor slot 0x18, untraced",
    "0x08": "own visitor slot 0x20 through 0x00793290, untraced",
    "0x09": "own visitor slot 0x1c, untraced",
    "0x0a": "tag plus 9 bytes consumed, no Lua value (visitor slot 0x24 is a bare return)",
    "0x0b": "tag plus 8 bytes consumed, no Lua value (visitor slot 0x28 is a bare return)",
    "0x0c": "tag plus 1 byte consumed, no Lua value (visitor slot 0x30 is a bare return)",
    "0x0d": "stop through jump table entry 0x007909b6, no value",
    "0x0e": "stop through jump table entry 0x007909b6, no value",
    "0x0f": "END, stops without a value",
    "0x10-0xff": "stop through the unsigned compare at 0x00790719 and the bound at 0x00790722, no value",
    "length exhausted": "stop when the remaining length is zero, no value",
}
EXPECTED_ON_INIT = (
    "_onInit(self, v1, ..., vn) with the visitor values in order. The path, the "
    "controls, the setup bundle, words C, D and E and the self block are not "
    "arguments."
)
EXPECTED_SELF = {
    "selfIdGetterVa": "0x004d7490",
    "selfIdOffset": "0x1783c",
    "selfIdWriter": {
        "caseVa": "0x004dcc2c",
        "storeVa": "0x004d9b11",
        "previousIdOffset": "0x17840",
        "source": "the id the dispatcher passes for opcode 0x0002, the wrapped source of the sub-event",
    },
    "keyTests": [
        {"va": "0x00774b52", "effect": "reads the self block of the stream"},
        {
            "va": "0x00772cfd",
            "effect": "registers the native MyPlayer prototype before binding the class",
        },
        {"va": "0x0075bbf0", "effect": "admits the candidate in queue state 2"},
    ],
    "prototype": {
        "nameVa": "0x00fd898c",
        "name": "MyPlayer",
        "allocationBytes": "0x168",
        "constructorVa": "0x006f9140",
        "vtableSlot3Va": "0x00fd7868",
        "slot3TargetVa": "0x006e3440",
    },
    "luaClass": "The class field names the Lua class. No installed script is named MyPlayer.",
    "currentPlayerSetter": {
        "va": "0x00758e40",
        "soleCallerVa": "0x006e3482",
        "storesAt": "CoreWork+0x0c",
        "clearsWhenArgumentNull": True,
    },
    "getMyPlayer": {
        "callbackVa": "0x00707970",
        "chain": "[[[context+0x4] via 0x00cc7510]+0x4]+0x0c then 0x00748b30",
        "returns": "the instance table of the control stored by the setter, nil while it is null",
    },
    "isMyPlayer": "PlayerBaseClass.isMyPlayer returns whether worldMaster:_getMyPlayer() is self.",
    "publishedBefore": "The setter runs from instance creation (0x00cdceb0 through slot 3), before 0x0078bbb0 requests the _onInit thread.",
}
EXPECTED_STATES = [
    (0, 1, "class equals Debug"),
    (1, 2, "class equals WorldMaster"),
    (2, 3, "candidate key equals the self id"),
    (
        3,
        5,
        "flag 0x174 clear skips to 5, otherwise class equals CommandDebuggerGM",
    ),
    (
        5,
        4,
        "flags 0x174 and 0x175 clear skips to 7, otherwise class equals CommandDebuggerDEV",
    ),
    (4, 7, "class equals CommandDebuggerTEST"),
    (7, 8, "path starts with /Area/"),
    (8, 9, "candidate key equals 0xc0000024"),
    (
        9,
        10,
        "bound key 0x170 equals 0xc0000024, then state 10 and the current candidate is committed in the same call",
    ),
    (10, 10, "any candidate"),
]
EXPECTED_QUEUE = {
    "stateOffset": "0x16c",
    "pendingOffset": "0x2c",
    "bindKeyOffset": "0x170",
    "flagOffsets": {"gm": "0x174", "dev": "0x175"},
    "initial": {
        "state": 0,
        "bindKey": "0x0130c778",
        "gm": 0,
        "dev": "constructor argument, source untraced",
    },
    "passVa": "0x00766f00",
    "stateVa": "0x007663d0",
    "acceptance": "At the commit label 0x00766748 an owner object at candidate+0x134 must pass 0x00765410, otherwise the previous state is restored.",
    "immediateGate": "0x00774530 binds at once only when state 0x16c is 10 and control A is true.",
    "order": "Candidates are appended at the tail and scanned from the head.",
    "bindKeyStore": "0x00772e0e stores the bound candidate key at +0x170 after 0x007687f0 returns.",
    "readiness": {
        "tier": "STATIC",
        "boundKeyGateVa": "0x007664ae",
        "boundKeyGate": "In states 0 to 9, when the bound key at +0x170 is not the null key 0x0130c778 and resolves to a control, the pass returns without binding until 0x00cc72a0 reads registry byte +0x7d of that control as nonzero. A key that does not resolve is reset to the null key and the pass continues. State 10 skips the test.",
        "containerGateVa": "0x00766401",
        "containerGate": "In every state, each entry of the container at +0x20 (count at +0x28) whose key resolves and passes the same test is erased, and the pass returns without binding while any entry remains. Who fills the container is not traced.",
        "readyByte": "Registry byte +0x7d is written by 0x00ce1dd0 at 0x00ce1e0b, which 0x00ce26c0 calls with 1 from the init-thread completion at 0x00cce23e. That the completion is the return of the _onInit thread is inferred.",
    },
}
EXPECTED_CREATION = {
    "listenerVa": "0x00772c80",
    "bindVa": "0x007687f0",
    "createVa": "0x0078bbb0",
    "instanceVa": "0x00cdcba0",
    "publishVa": "0x00cd7970",
    "steps": [
        "0x00cc7800 enters 0x00cdcba0, which returns null when the class record of the class field is absent or not valid",
        "0x00cdcba0 publishes the name field as a global",
        "0x00cdcba0 clones the instance and 0x00ce2440 calls its slot 3, which for the MyPlayer prototype stores the current player",
        "0x0078bbb0 runs the optional initializer callback",
        "0x00cc7ad0 requests the init thread",
        "0x00cd0910 queues _onInit with the value vector",
    ],
    "nameRule": "An empty name makes an unnamed instance. A first character outside a to z and the underscore is reported through 0x00cd55a0 and creation continues.",
    "globalRule": "globals[name] is set to the instance table when globals[name] is nil, and an existing global makes the creation return null.",
    "initWait": "0x0078bbb0 requests the _onInit thread and does not wait for it. In queue states 0 to 9 the next candidate is held until the registry byte +0x7d of the last bound control is set at init completion, so Debug, WorldMaster, the player, the area master and the local DesktopWidget each finish initialization before the next is bound. State 10 does not wait.",
}
EXPECTED_DESKTOP = {
    "requestVa": "0x00774700",
    "key": "0xc0000024",
    "path": "/Widget/DesktopWidget",
    "class": "DesktopWidget",
    "name": "desktopWidget",
    "values": [],
    "trigger": "The self branch of 0x00774ad0 ensures the 0xc0000024 element, resolves the DesktopWidget class record and passes it to 0x004d87c0, which reaches 0x00774700. The conditions inside 0x00774700 are untraced.",
    "onInit": "_onInit(self) with no further arguments",
}
EXPECTED_STATIC = {
    "callbackVa": "0x006dcdf0",
    "bodyVa": "0x0078cc30",
    "contractManifest": "xivl-client-structs:manifests/boot_native_call_contract.json rows _getStaticActor",
    "classTable": "xivl-client-data:manifests/staticactor_class_paths.json",
    "worldMasterArgument": 310001,
    "worldMasterClassPath": "/Judge/JudgeMaster",
    "instanceName": "_static%X, X the low 20 bits of the identity",
    "records": "No s2c 0x00cc record creates a static actor.",
}
EXPECTED_CONTROL_PATTERNS = {
    "4,4,4,4,4": 793,
    "4,4,4,4,3": 13,
    "4,3,4,4,4": 13,
    "4,4,4,3,4": 12,
}
EXPECTED_JUMP_TABLE = [
    "0x00766511",
    "0x00766535",
    "0x00766559",
    "0x0076658f",
    "0x0076663a",
    "0x007665e0",
    "0x00766765",
    "0x0076665e",
    "0x007666be",
    "0x00766702",
    "0x00766748",
]
STATE_ROWS = {
    0: "queue state 0",
    1: "queue state 1",
    2: "queue state 2",
    3: "queue state 3",
    4: "queue state 4",
    5: "queue state 5",
    7: "queue state 7",
    8: "queue state 8",
    9: "queue state 9",
    10: "queue commit and rollback",
}
STRING_ROWS = {
    "string Debug": "Debug",
    "string WorldMaster": "WorldMaster",
    "string CommandDebuggerGM": "CommandDebuggerGM",
    "string CommandDebuggerDEV": "CommandDebuggerDEV",
    "string CommandDebuggerTEST": "CommandDebuggerTEST",
    "string /Area/": "/Area/",
    "string MyPlayer": "MyPlayer",
    "string clientversionerror": "[clientversionerror]",
    "string DesktopWidget path": "/Widget/DesktopWidget",
    "string DesktopWidget class": "DesktopWidget",
    "string desktopWidget name": "desktopWidget",
    "string onInit": "_onInit",
}
REQUIRED_CHECKS = {
    "dispatcher 0xcc arm calls the wrapper",
    "dispatcher case 2 factory and self id store",
    "self id getter",
    "self id store",
    "wrapper casts and calls the actor record apply",
    "apply self key compare",
    "apply stores the first application dword",
    "apply name and class strings",
    "apply reader over the stream",
    "apply terminator guard",
    "apply path and first two controls",
    "apply optional bundle reads",
    "apply third control and word",
    "apply fourth control and byte write",
    "apply fifth control and discarded word",
    "apply self sixteen word loop",
    "apply self four word tail",
    "apply value vector and visitor",
    "apply visitor call",
    "apply self flag write",
    "apply submission call",
    "control reader",
    "visitor bare return",
    "numeric reader byte order",
    "string reader",
    "byte reader",
    "terminator guard",
    "visitor decoder float arm",
    "visitor decoder dispatch",
    "queue constructor state fields",
    "queue immediate arm gate",
    "queue state jump table",
    "queue state 0",
    "queue state 1",
    "queue state 2",
    "queue state 3",
    "queue state 4",
    "queue state 5",
    "queue state 7",
    "queue state 8",
    "queue state 9",
    "queue commit and rollback",
    "queue waiting container readiness loop",
    "queue bound key readiness gate",
    "registry readiness byte reader",
    "self key test of the queue",
    "pending listener self branch",
    "pending listener prototype and register",
    "pending listener records the bound key",
    "create sequence",
    "create then init thread then onInit",
    "init thread completion call",
    "init completion entry",
    "activation stores the ready byte",
    "instance named creation",
    "instance name first character",
    "instance slot 3 hook",
    "global existence test",
    "global publication",
    "globals index helpers",
    "MyPlayer slot 3",
    "MyPlayer vtable slot 3",
    "current player setter",
    "_getMyPlayer callback",
    "local desktop widget key",
    "local desktop widget empty vector",
    "local desktop widget submission",
    "tag decoder jump table",
    "visitor vtable",
    *STRING_ROWS,
}
EXPECTED_TAG_TABLE = [
    "0x007909b6",
    "0x00790757",
    "0x00790779",
    "0x007907a1",
    "0x0079072f",
    "0x0079072f",
    "0x0079081e",
    "0x0079084b",
    "0x00790870",
    "0x007908de",
    "0x007908a9",
    "0x0079092f",
    "0x00790961",
    "0x0079098e",
    "0x007909b6",
    "0x007909b6",
    "0x007909b6",
]
EXPECTED_VISITOR_TABLE = [
    "0x007927b0",
    "0x00793cf0",
    "0x00793430",
    "0x00793d00",
    "0x00793d10",
    "0x00793d20",
    "0x00793db0",
    "0x00792610",
    "0x00793ed0",
    "0x00776340",
    "0x00776340",
    "0x00794100",
    "0x00776340",
]


class RecordError(ValueError):
    """The record is rejected or cannot be read."""


def terminator_ok(stream: bytes) -> bool:
    """0x0078fa80: the last nonzero byte of the span is 0x0f or 0xff."""
    for byte in reversed(stream):
        if byte:
            return byte in (TAG_END, 0xFF)
    return False


def split_application(app: bytes) -> tuple[bytes, str, str, bytes]:
    """The first dword, name, class and stream of a 264-byte application."""
    if len(app) != APPLICATION_BYTES:
        raise RecordError(f"application is {len(app)} bytes")

    def text(start: int) -> str:
        raw = app[start : start + TEXT_BYTES]
        return raw.split(b"\0", 1)[0].decode("latin-1")

    return app[:4], text(4), text(0x24), app[STREAM_OFFSET : STREAM_OFFSET + 192]


class Cursor:
    def __init__(self, data: bytes) -> None:
        self.data = data
        self.at = 0

    def take(self, count: int) -> bytes:
        if self.at + count > len(self.data):
            raise RecordError("stream overrun")
        chunk = self.data[self.at : self.at + count]
        self.at += count
        return chunk

    def control(self) -> bool:
        return self.take(1)[0] == 0x03

    def word(self) -> int:
        return int.from_bytes(self.take(5)[1:], "big")

    def byte(self) -> int:
        return self.take(2)[1]

    def text(self) -> str:
        self.take(1)
        end = self.data.find(b"\0", self.at)
        if end < 0:
            raise RecordError("unterminated string")
        raw = self.data[self.at : end]
        self.at = end + 1
        return raw.decode("latin-1")


def parse_stream(stream: bytes, is_self: bool) -> dict:
    """Reference model of the reads in 0x00774ad0 and the visitor 0x00790aa0."""
    if not terminator_ok(stream):
        raise RecordError("client version error")
    cur = Cursor(stream)
    out: dict = {"path": cur.text(), "bundle": None, "self": None}
    control_a = cur.control()
    control_b = cur.control()
    if control_b:
        out["bundle"] = {
            "texts": [cur.text(), cur.text()],
            "word": cur.word(),
            "byte": cur.byte(),
            "bools": [cur.control(), cur.control()],
        }
    out["controlA"] = control_a
    control_c = cur.control()
    out["wordC"] = cur.word() if control_c else None
    control_d = cur.control()
    word_d = cur.word() if control_d else None
    out["wordD"] = word_d
    out["controlD"] = bool(control_d and word_d)
    control_e = cur.control()
    out["wordE"] = cur.word() if control_e else None
    out["controls"] = [control_a, control_b, control_c, control_d, control_e]
    if is_self:
        out["self"] = [cur.word() for _ in range(SELF_WORDS)]
    out["values"], ended = parse_values(cur)
    out["ended"] = ended
    out["tailClear"] = ended and not any(stream[cur.at + 1 :])
    return out


def parse_values(cur: Cursor) -> tuple[list, bool]:
    """Values up to the first stop and whether the stop was the END tag.

    Tags 0x0d and above and an exhausted length leave through one exit at
    0x007909b6, without a value and without advancing the cursor.
    """
    values: list = []
    while True:
        if cur.at >= len(cur.data):
            return values, False
        tag = cur.data[cur.at]
        if tag >= TAG_STOP_MIN:
            return values, tag == TAG_END
        cur.at += 1
        if tag == 0x00:
            values.append(struct.unpack(">i", cur.take(4))[0])
        elif tag == 0x01:
            values.append(struct.unpack(">f", cur.take(4))[0])
        elif tag == 0x02:
            cur.at -= 1
            values.append(cur.text())
        elif tag in (0x03, 0x04):
            values.append(tag == 0x03)
        elif tag == 0x05:
            values.append(None)
        elif tag == 0x06:
            # an unresolved key keeps its slot as a null binding
            values.append(("actor", int.from_bytes(cur.take(4), "big")))
        elif tag == 0x0A:
            cur.take(9)
        elif tag == 0x0B:
            cur.take(8)
        elif tag == 0x0C:
            cur.take(1)
        else:
            raise RecordError(f"tag {tag:#04x} is not decoded")


def parse_application(app: bytes, is_self: bool) -> dict:
    first, name, cls, stream = split_application(app)
    out = parse_stream(stream, is_self)
    out.update({"first": first, "name": name, "class": cls})
    return out


def creation_waits(
    document: dict,
    candidates: list[dict],
    self_id: int,
    gm: bool = False,
    dev: bool = False,
) -> list[tuple[int, int | None]]:
    """(key, key whose init completion it waits for) in the order the container binds.

    Candidates carry key, cls and path. The local DesktopWidget request joins the
    queue ahead of the self record when that record is applied. A pass that
    starts below state 10 waits for the last bound key to finish initialization
    (0x007664ae), so the first bind and the binds made in state 10 wait for none.
    """
    nxt = {row["state"]: row["next"] for row in document["queue"]["states"]}
    queue: list[dict] = []
    for cand in candidates:
        if cand["key"] == self_id:
            queue.append(
                {
                    "key": LOCAL_DESKTOP_KEY,
                    "cls": "DesktopWidget",
                    "path": "/Widget/DesktopWidget",
                }
            )
        queue.append(cand)

    def accepts(state: int, cand: dict) -> bool:
        if state == 0:
            return cand["cls"] == "Debug"
        if state == 1:
            return cand["cls"] == "WorldMaster"
        if state == 2:
            return cand["key"] == self_id
        if state == 3:
            return cand["cls"] == "CommandDebuggerGM"
        if state == 5:
            return cand["cls"] == "CommandDebuggerDEV"
        if state == 4:
            return cand["cls"] == "CommandDebuggerTEST"
        if state == 7:
            return cand["path"].startswith("/Area/")
        if state == 8:
            return cand["key"] == LOCAL_DESKTOP_KEY
        return state == 10

    state = 0
    entry = 0
    bound = None
    result: list[tuple[int, int | None]] = []
    while True:
        if state == 3 and not gm:
            state = nxt[3]
        elif state == 5 and not (gm or dev):
            state = 7
        elif state == 9:
            if bound != LOCAL_DESKTOP_KEY:
                return result
            state = nxt[9]
        else:
            pick = next((c for c in queue if accepts(state, c)), None)
            if pick is None:
                return result
            queue.remove(pick)
            result.append((pick["key"], bound if entry != 10 else None))
            bound = pick["key"]
            state = entry = nxt[state]


def creation_order(
    document: dict,
    candidates: list[dict],
    self_id: int,
    gm: bool = False,
    dev: bool = False,
) -> list[int]:
    """Keys in the order the waiting container binds them."""
    return [key for key, _ in creation_waits(document, candidates, self_id, gm, dev)]


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-actor-instantiation-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binarySha256") != EXPECTED_SHA256
        or source.get("size") != EXPECTED_SIZE
        or source.get("imageBase") != "0x00400000"
    ):
        errors.append("retail evidence identity changed")
    if any(name not in document for name in SECTIONS):
        errors.append("a section is missing")
        return errors
    if document["record"] != EXPECTED_RECORD:
        errors.append("record layout changed")
    stream = document["stream"]
    steps = [(row.get("step"), row.get("reader")) for row in stream.get("steps", [])]
    if steps != EXPECTED_STEPS:
        errors.append("stream step order or readers changed")
    bundle = [
        row for row in stream.get("steps", []) if row.get("step") == "setup bundle"
    ]
    if [row.get("readers") for row in bundle] != [EXPECTED_BUNDLE_READERS]:
        errors.append("setup bundle readers changed")
    selfs = [row for row in stream.get("steps", []) if row.get("step") == "self block"]
    if [row.get("condition") for row in selfs] != [
        "record key equals the self id"
    ] or "20 words" not in selfs[0].get("wire", ""):
        errors.append("self block condition or width changed")
    if stream.get("tags") != EXPECTED_TAGS:
        errors.append("value tag map changed")
    if stream.get("onInitCall") != EXPECTED_ON_INIT:
        errors.append("_onInit argument rule changed")
    if document["selfPlayer"] != EXPECTED_SELF:
        errors.append("self player contract changed")
    queue = document["queue"]
    states = [
        (row.get("state"), row.get("next"), row.get("test"))
        for row in queue.get("states", [])
    ]
    if states != EXPECTED_STATES:
        errors.append("queue states changed")
    rest = {key: value for key, value in queue.items() if key != "states"}
    if rest != EXPECTED_QUEUE:
        errors.append("queue fields changed")
    if document["creation"] != EXPECTED_CREATION:
        errors.append("creation contract changed")
    if document["desktopWidget"] != EXPECTED_DESKTOP:
        errors.append("DesktopWidget request changed")
    if document["getStaticActor"] != EXPECTED_STATIC:
        errors.append("static actor contract changed")
    observed = document["observed"]
    captures = observed.get("captures", [])
    if (
        observed.get("corpus") != "xivl-captures:sources/pcap-1.23b/manifest.yaml"
        or observed.get("lane") != "main connection 0, s2c"
        or observed.get("applicationBytes") != APPLICATION_BYTES
        or observed.get("records") != sum(row.get("records", 0) for row in captures)
        or observed.get("selfRecords")
        != sum(row.get("selfRecords", 0) for row in captures)
        or observed.get("parsedToEnd") != observed.get("records")
        or observed.get("records") != 831
        or observed.get("selfRecords") != 12
        or len(captures) != 17
        or any(not SHA_RE.match(str(row.get("sha256", ""))) for row in captures)
        or len({row.get("capture") for row in captures}) != len(captures)
    ):
        errors.append("capture measurement changed")
    patterns = observed.get("controlPatterns", {})
    if patterns != EXPECTED_CONTROL_PATTERNS or sum(patterns.values()) != observed.get(
        "records"
    ):
        errors.append("control pattern census changed")
    classes = {row.get("class"): row for row in observed.get("classes", [])}
    if (
        classes.get("Player", {}).get("selfRecords") != 12
        or classes.get("Player", {}).get("valueTypes") != {"T": 14}
        or classes.get("Debug", {}).get("valueTypes") != {"TT": 13}
        or classes.get("WorldMaster", {}).get("valueTypes") != {"n": 13}
        or classes.get("ZoneMasterFstF0", {}).get("valueTypes") != {"TFF": 9}
    ):
        errors.append("zone-in group value types changed")
    if observed.get("loginOrder", [None])[:5] != [
        "0x0002",
        "Player",
        "Debug",
        "WorldMaster",
        "AfterQuestWarpDirector",
    ]:
        errors.append("login arrival order changed")
    contract = document.get("contract", [])
    if (
        len(contract) != 10
        or "until the previous instance's _onInit has completed" not in contract[6]
        or "after the area master's _onInit has completed" not in contract[7]
    ):
        errors.append("consumer contract changed")
    if (
        not all(
            item.get("label") in ("CALIBRATION", "INFERRED")
            for item in document.get("open", [])
        )
        or len(document.get("open", [])) != 12
    ):
        errors.append("open questions changed")
    checks = document.get("staticChecks")
    if not isinstance(checks, list):
        errors.append("staticChecks is not a list")
        checks = []
    names = [item.get("name") for item in checks if isinstance(item, dict)]
    if len(names) != len(set(names)):
        errors.append("staticChecks names are not unique")
    if REQUIRED_CHECKS - set(names):
        errors.append(f"staticChecks lacks {sorted(REQUIRED_CHECKS - set(names))}")
    by_name = {item.get("name"): item for item in checks if isinstance(item, dict)}
    for item in checks:
        if not isinstance(item, dict) or item.get("kind") not in {
            "signature",
            "dwords",
            "string",
        }:
            errors.append("staticChecks entry has an unknown kind")
        elif not HEX8_RE.match(str(item.get("va", ""))):
            errors.append(f"staticChecks {item.get('name')!r} has a malformed va")
        elif item["kind"] == "signature":
            try:
                same = len(bytes.fromhex(item["pattern"])) == len(
                    bytes.fromhex(item["mask"])
                )
            except (KeyError, ValueError):
                same = False
            if not same:
                errors.append(f"staticChecks {item['name']!r} pattern or mask invalid")
    table = by_name.get("queue state jump table", {})
    if table.get("values") != EXPECTED_JUMP_TABLE:
        errors.append("queue state jump table changed")
    for state, row_name in STATE_ROWS.items():
        row = by_name.get(row_name, {})
        if row.get("va") != EXPECTED_JUMP_TABLE[state]:
            errors.append(f"queue state {state} row is not at its table entry")
    if by_name.get("tag decoder jump table", {}).get("values") != EXPECTED_TAG_TABLE:
        errors.append("tag decoder jump table changed")
    if by_name.get("visitor vtable", {}).get("values") != EXPECTED_VISITOR_TABLE:
        errors.append("visitor vtable changed")
    for name, text in STRING_ROWS.items():
        if by_name.get(name, {}).get("text") != text:
            errors.append(f"string row {name!r} changed")
    if by_name.get("MyPlayer vtable slot 3", {}).get("values") != [
        EXPECTED_SELF["prototype"]["slot3TargetVa"]
    ]:
        errors.append("MyPlayer slot 3 target changed")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (
        r"[A-Za-z]:\\\\",
        "/" + "Users/",
        "/" + "home/",
        "agent-islands",
    )
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a private path")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations: list[dict] = []
    for path, value in [
        (("record", "streamOffset"), "0x48"),
        (("record", "applyVa"), "0x00774ad1"),
        (("record", "terminatorGuard", "va"), "0x0078fa81"),
        (("stream", "onInitCall"), "_onInit(self, path, v1)"),
        (("selfPlayer", "selfIdOffset"), "0x17840"),
        (("selfPlayer", "isMyPlayer"), "never"),
        (("selfPlayer", "currentPlayerSetter", "soleCallerVa"), "0x006e3483"),
        (("queue", "immediateGate"), "0x00774530 binds at once"),
        (("creation", "globalRule"), "a global is never set"),
        (("creation", "initWait"), "The _onInit thread is requested and not awaited."),
        (("queue", "readiness", "boundKeyGateVa"), "0x007664af"),
        (("contract", 6), "Queue _onInit(self, v1..vn) and run it as a coroutine."),
        (("contract", 7), "Create the DesktopWidget instance with no arguments."),
        (("desktopWidget", "name"), "desktopwidget"),
        (("desktopWidget", "values"), [1]),
        (("getStaticActor", "worldMasterArgument"), 310002),
        (("observed", "selfRecords"), 13),
        (("observed", "records"), 832),
    ]:
        changed = copy.deepcopy(document)
        cursor = changed
        for key in path[:-1]:
            cursor = cursor[key]
        cursor[path[-1]] = value
        mutations.append(changed)
    for index in range(len(document["queue"]["states"])):
        changed = copy.deepcopy(document)
        changed["queue"]["states"][index]["next"] += 1
        mutations.append(changed)
    swapped = copy.deepcopy(document)
    states = swapped["queue"]["states"]
    states[0]["test"], states[1]["test"] = states[1]["test"], states[0]["test"]
    mutations.append(swapped)
    for index in range(len(document["stream"]["steps"]) - 1):
        changed = copy.deepcopy(document)
        steps = changed["stream"]["steps"]
        steps[index], steps[-1] = steps[-1], steps[index]
        mutations.append(changed)
    stop = copy.deepcopy(document)
    del stop["stream"]["tags"]["0x0d"]
    mutations.append(stop)
    tag = copy.deepcopy(document)
    tag["stream"]["tags"]["0x01"] = "32-bit unsigned integer, Lua number"
    mutations.append(tag)
    dropped = copy.deepcopy(document)
    dropped["staticChecks"] = [
        item
        for item in dropped["staticChecks"]
        if item["name"] != "apply self sixteen word loop"
    ]
    mutations.append(dropped)
    table = copy.deepcopy(document)
    for item in table["staticChecks"]:
        if item["name"] == "queue state jump table":
            item["values"][2] = "0x00766558"
    mutations.append(table)
    string = copy.deepcopy(document)
    for item in string["staticChecks"]:
        if item["name"] == "string WorldMaster":
            item["text"] = "worldMaster"
    mutations.append(string)
    leaked = copy.deepcopy(document)
    leaked["source"]["path"] = "agent-islands/private-evidence"
    mutations.append(leaked)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]


def verify_executable(path: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    return image_checks.verify_image(path.read_bytes(), document)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exe", type=Path, help="pinned retail executable for byte checks"
    )
    args = parser.parse_args()
    errors = verify() + mutation_test()
    if args.exe:
        errors += verify_executable(args.exe)
    if errors:
        print(f"FAIL: {len(errors)} actor instantiation error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus executable checks" if args.exe else ""
    print(f"PASS: actor instantiation manifest and mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
