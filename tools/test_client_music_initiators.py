#!/usr/bin/env python3
"""Tests for the client music initiator verifier."""

from __future__ import annotations

import copy
import hashlib
import json
import stat
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_client_music_initiators as verifier  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []
ENTRY = 0x00401020
LONE = 0x00401030
CHAIN = 0x00401038


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def build_image(text: bytes, data: bytes = b"\0" * 16) -> bytes:
    """A two-section PE32 image: raw .text at 0x401000 and .rdata at 0x402000."""
    header = bytearray(0x200)
    header[0:2] = b"MZ"
    struct.pack_into("<I", header, 0x3C, 0x80)
    header[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", header, 0x80 + 6, 2)
    struct.pack_into("<H", header, 0x80 + 20, 0xE0)
    base = 0x80 + 24 + 0xE0
    header[base : base + 5] = b".text"
    struct.pack_into("<IIII", header, base + 8, len(text), 0x1000, len(text), 0x200)
    header[base + 40 : base + 46] = b".rdata"
    struct.pack_into(
        "<IIII", header, base + 48, len(data), 0x2000, len(data), 0x200 + len(text)
    )
    return bytes(header) + text + data


def text_bytes(
    extra_call_at: int | None = None,
    jump_at: int | None = None,
    chain_jump_at: int | None = None,
) -> bytes:
    """Calls to ENTRY and CHAIN, an immediate, and ENTRY, LONE and CHAIN as bare returns."""
    text = bytearray(b"\x90" * 0x40)
    text[0:5] = b"\xe8" + struct.pack("<i", ENTRY - 0x00401005)
    text[5] = 0xC3
    text[8:13] = b"\x05" + struct.pack("<I", 0xDEADBEEF)
    text[0x22:0x27] = b"\xe8" + struct.pack("<i", CHAIN - 0x00401027)
    text[ENTRY - 0x00401000] = 0xC3
    text[LONE - 0x00401000] = 0xC3
    text[CHAIN - 0x00401000] = 0xC3
    if extra_call_at is not None:
        at = extra_call_at - 0x00401000
        target = LONE if extra_call_at == 0x00401014 else ENTRY
        text[at : at + 5] = b"\xe8" + struct.pack("<i", target - (extra_call_at + 5))
    for site, target in ((jump_at, ENTRY), (chain_jump_at, CHAIN)):
        if site is not None:
            at = site - 0x00401000
            text[at : at + 5] = b"\xe9" + struct.pack("<i", target - (site + 5))
    return bytes(text)


def fixture() -> tuple[bytes, dict]:
    document = {
        "entries": [
            {
                "name": "entry",
                "va": f"{ENTRY:#010x}",
                "sites": [{"site": "0x00401000", "role": "caller"}],
                "dataRefs": [],
            }
        ],
        "unreferenced": [{"va": f"{LONE:#010x}"}],
        "cutChain": [
            {
                "name": "chain",
                "va": f"{CHAIN:#010x}",
                "sites": [{"site": "0x00401022", "role": "caller"}],
                "dataRefs": [],
            }
        ],
        "immediateScans": [{"value": "0xdeadbeef", "sites": ["0x00401009"]}],
        "staticChecks": [
            {
                "name": "ret",
                "kind": "signature",
                "va": f"{ENTRY:#010x}",
                "pattern": "c3",
                "mask": "ff",
            }
        ],
    }
    return build_image(text_bytes()), document


def real_manifest() -> dict:
    return json.loads(verifier.MANIFEST.read_text(encoding="ascii"))


def run(image: bytes, document: dict) -> list[str]:
    return verifier.verify_image(image, document, require_identity=False)


def put(root: Path, resource: int, data: bytes) -> None:
    path = root / verifier.scans.resource_path(resource)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def container(streams: int = 1) -> bytes:
    body = bytearray(0x60)
    body[0:8] = b"SEDBSSCF"
    struct.pack_into("<H", body, 0x34, streams)
    return bytes(body)


SHEET_XML = (
    chr(0xFEFF)
    + '<?xml version="1.0" encoding="utf-8"?>\n'
    + '<ssd version="0.1"><sheet name="_layout" mode="client" column_max="4" '
    + 'column_count="3" cache="-1" type="none"><type><param>u8</param>'
    + "<param>s32</param><param>u16</param></type><index><param>1</param>"
    + '<param>2</param><param>3</param></index><block count="1">'
    + '<file begin="1" count="8085" offset="3" enable="2">1</file></block>'
    + "</sheet></ssd>"
)


def title_rows(root: Path) -> list[dict]:
    rows = []
    for index in (21, 22, 23):
        data = container() + bytes([index]) * 8
        put(root, verifier.TITLE_BASE + index, data)
        rows.append(
            {
                "index": index,
                "resource": f"{verifier.TITLE_BASE + index:#010x}",
                "size": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
                "streams": 1,
            }
        )
    return rows


def main() -> int:
    manifest = real_manifest()
    check("manifest verifies", verifier.verify() == [])
    check("mutations are rejected", verifier.mutation_test() == [])
    check(
        "claim tiers are pinned",
        {c["id"]: c["tier"] for c in manifest["claims"]} == verifier.EXPECTED_CLAIMS,
    )
    external = {
        s["site"]
        for s in manifest["entries"][1]["sites"]
        if not 0x004BB000 <= int(s["site"], 16) < 0x004BC000
    }
    check(
        "every external request site is an initiator or the unreferenced helper",
        external == {i["site"] for i in manifest["initiators"]} | {"0x00687c57"},
    )

    image, document = fixture()
    check("matching image passes", run(image, document) == [])
    check(
        "an unlisted call site is reported",
        len(run(build_image(text_bytes(0x00401010)), document)) == 1,
    )
    gone = copy.deepcopy(document)
    gone["entries"][0]["sites"] = []
    check("a missing listed site is reported", len(run(image, gone)) == 1)
    check(
        "an unlisted jump to an entry is reported",
        len(run(build_image(text_bytes(jump_at=0x00401010)), document)) == 1,
    )
    check(
        "an unlisted jump to a cut chain routine is reported",
        len(run(build_image(text_bytes(chain_jump_at=0x00401010)), document)) == 1,
    )
    gone_chain = copy.deepcopy(document)
    gone_chain["cutChain"][0]["sites"] = []
    check("a missing cut chain site is reported", len(run(image, gone_chain)) == 1)
    chain_pointer = build_image(text_bytes(), struct.pack("<I", CHAIN) + b"\0" * 12)
    check(
        "a dword reference to a cut chain routine is reported",
        any("dword references to chain" in e for e in run(chain_pointer, document)),
    )
    check(
        "a call to an unreferenced function is reported",
        len(run(build_image(text_bytes(0x00401014)), document)) == 1,
    )
    pointer = build_image(text_bytes(), struct.pack("<I", ENTRY) + b"\0" * 12)
    errors = run(pointer, document)
    check(
        "a dword reference to an entry is reported",
        len(errors) == 1 and "dword references" in errors[0],
    )
    lone_pointer = build_image(
        text_bytes(), b"\0" * 4 + struct.pack("<I", LONE) + b"\0" * 8
    )
    check(
        "a dword reference to an unreferenced function is reported",
        len(run(lone_pointer, document)) == 1,
    )
    check(
        "dword references are found in every section",
        verifier.image_dword_refs(pointer, ENTRY) == [0x00402000],
    )
    moved = copy.deepcopy(document)
    moved["immediateScans"][0]["sites"] = ["0x0040100a"]
    check("a changed immediate site is reported", len(run(image, moved)) == 1)
    flipped = bytearray(image)
    flipped[0x200 + (ENTRY - 0x00401000)] = 0x90
    check("a changed pinned byte is reported", len(run(bytes(flipped), document)) >= 1)
    conditioned = copy.deepcopy(manifest)
    conditioned["reset"]["sites"][2]["condition"] = "always"
    check(
        "the opcode 0x0004 reset condition is pinned",
        "reset site conditions changed" in verifier.verify(conditioned),
    )
    check("identity is enforced", len(verifier.verify_image(image, document)) >= 2)

    with tempfile.TemporaryDirectory() as raw:
        root = Path(raw)
        install = {"install": {"titleTrackFiles": title_rows(root)}}
        check(
            "install files match themselves",
            verifier.verify_install(root, install) == [],
        )
        put(root, verifier.TITLE_BASE + 22, container() + b"\x00" * 8)
        check(
            "a changed file is reported",
            len(verifier.verify_install(root, install)) == 1,
        )
        put(root, verifier.TITLE_BASE + 22, b"SEDBSSCX" + bytes(0x58) + bytes([22]) * 8)
        check(
            "a missing magic is reported",
            any("magic" in e for e in verifier.verify_install(root, install)),
        )
        (root / verifier.scans.resource_path(verifier.TITLE_BASE + 23)).unlink()
        check(
            "a missing file is reported",
            any("missing" in e for e in verifier.verify_install(root, install)),
        )

    with tempfile.TemporaryDirectory() as raw:
        root = Path(raw)
        install = {"install": {"titleTrackFiles": title_rows(root)}}
        two = container(2) + bytes([23]) * 8
        put(root, verifier.TITLE_BASE + 23, two)
        install["install"]["titleTrackFiles"][2]["size"] = len(two)
        install["install"]["titleTrackFiles"][2]["sha256"] = hashlib.sha256(
            two
        ).hexdigest()
        check(
            "a stream count difference is reported",
            any("stream count" in e for e in verifier.verify_install(root, install)),
        )

    found = verifier.parse_sheet(SHEET_XML)
    check(
        "a sheet definition is parsed past its byte order mark",
        found
        == {
            "name": "_layout",
            "columnCount": 3,
            "columnMax": 4,
            "types": ["u8", "s32", "u16"],
            "index": [1, 2, 3],
            "blockBegin": 1,
            "blockCount": 8085,
        },
    )
    try:
        verifier.parse_sheet('<ssd version="0.1"/>')
        rejected = False
    except ValueError:
        rejected = True
    check("a document without a sheet is rejected", rejected)

    with tempfile.TemporaryDirectory() as raw:
        root = Path(raw)
        tool = root / "fake_xivl.py"
        tool.write_text(
            "#!/usr/bin/env python3\n"
            "import sys\n"
            "from pathlib import Path\n"
            "out = Path(sys.argv[sys.argv.index('--output') + 1]) / 'payloads'\n"
            "out.mkdir(parents=True)\n"
            f"(out / 'decoded.xml').write_text({SHEET_XML!r}, encoding='utf-8')\n",
            encoding="ascii",
        )
        tool.chmod(tool.stat().st_mode | stat.S_IXUSR)
        sheet = {
            "name": "_layout",
            "resource": "0x0103045f",
            "columnCount": 3,
            "columnMax": 4,
            "types": ["u8", "s32", "u16"],
            "index": [1, 2, 3],
            "blockBegin": 1,
            "blockCount": 8085,
        }
        shaped = {"install": {"sheets": [sheet]}}
        put(root, 0x0103045F, b"x")
        check(
            "a decoded sheet matches",
            verifier.verify_sheets(root, tool, shaped) == [],
        )
        off = copy.deepcopy(shaped)
        off["install"]["sheets"][0]["types"] = ["u8", "s32", "s32"]
        check(
            "a sheet type difference is reported",
            len(verifier.verify_sheets(root, tool, off)) == 1,
        )
        off = copy.deepcopy(shaped)
        off["install"]["sheets"][0]["blockCount"] = 8084
        check(
            "a sheet slot count difference is reported",
            len(verifier.verify_sheets(root, tool, off)) == 1,
        )
        check(
            "a failing decoder is reported",
            len(verifier.verify_sheets(root, root / "absent", shaped)) >= 1,
        )

    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(FAILED) + len(PASSED)}")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} client music initiator tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
