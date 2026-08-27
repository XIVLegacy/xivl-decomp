#!/usr/bin/env python3
"""Validate the sanitized s2c 0x018D client-consumer contract."""

from __future__ import annotations

import copy
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "s2c_018d_client_consumer.json"
EXPECTED_PROJECTION = [
    ("+0x00", "+0x00"),
    ("+0x08", "+0x08"),
    ("+0x0c", "+0x0c"),
    ("+0x14", "+0x10"),
    ("+0x18", "+0x14"),
    ("+0x1c", "+0x18"),
]
EXPECTED_HEADER = [
    ("+0x00", "+0x08", 4, None),
    ("+0x04", "+0x0c", 4, None),
    ("+0x08", "+0x10", 4, None),
    ("+0x290", "+0x14", 1, "signed-byte widen"),
]
EXPECTED_READERS = [
    ("+0x08,+0x0c,+0x10", ("0x0055cf70",), (), (), "no outward reader in the exact direct, field, data, vtable, or generated-call census"),
    ("+0x14", ("0x0055f830", "0x0055cf70"), ("0x0055d020", "0x0055d0d0"), ("0x00671400", "0x00691f30"), None),
    ("+0x00", (), ("0x0055d090",), ("0x00671400",), None),
    ("+0x08,+0x0c", (), ("0x0055d0b0",), ("0x00671400",), None),
    ("+0x10,+0x18", (), ("0x0055d050",), ("0x00671400",), "+0x14 is projected but has no separate load in the first consumer"),
    ("+0x20", (), ("0x0055d030",), ("0x00671400",), None),
    ("+0x74", (), ("0x0055d070",), ("0x00671400",), None),
    ("remaining tail", (), (), (), "no additional first-consumer load is proven outside the +0x20 helper object and +0x74 scalar"),
    ("+0x798", ("0x0055f830", "0x0055cf70", "0x0055d0f0"), ("0x0055d0d0",), ("0x00691f30",), None),
]


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-s2c-018d-client-consumer-v3":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binarySha256") != "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
        or source.get("ghidraVersion") != "12.1.3"
        or source.get("presentationRunId") != "lane1-018d-presentation-contract-20260827"
        or source.get("followupRunId") != "lane1-018d-followup-keys-20260827"
        or source.get("keyHelperRunId") != "lane1-018d-followup-key-helpers-20260827"
    ):
        errors.append("retail evidence identity changed")
    wire = document.get("wireContract", {})
    if (
        wire.get("opcode") != "0x018D"
        or wire.get("neutralName") != "_0x018D"
        or wire.get("commit") != "67b709d5ffd90b8dc10a699e608fa1216e40660d"
        or wire.get("applicationSize") != "0x298"
        or wire.get("wireRecordOffset") != "0x10"
        or wire.get("wireRecordStride") != "0x28"
        or wire.get("physicalRecordCapacity") != 16
        or wire.get("countOffset") != "0x290"
        or "not a safe runtime count" not in wire.get("countBoundary", "")
    ):
        errors.append("wire contract or unsafe-count boundary changed")
    route = document.get("route", {})
    if (
        route.get("ownerPointerLoadVa") != "0x004dd167"
        or route.get("storageAdjustVa") != "0x004dd18f"
        or route.get("wireArgumentVa") != "0x004dd1a6"
        or route.get("storageThisVa") != "0x004dd1a7"
        or route.get("applyCallVa") != "0x004dd1a9"
        or route.get("applyVa") != "0x0055cf70"
    ):
        errors.append("dispatcher wire/storage alias route changed")
    owner = document.get("owner", {})
    if (
        owner.get("containerClass") != "Application::Main::RaptureElementContainer"
        or owner.get("containerPointerOffset") != "0x4d8"
        or owner.get("elementClass") != "ClientWorkElement"
        or owner.get("elementAllocationSize") != "0x838"
        or owner.get("storageOffset") != "0x98"
        or owner.get("storageSize") != "0x7a0"
        or owner.get("physicalRecordCount") != 16
        or owner.get("recordStride") != "0x78"
        or owner.get("elementConstructorVa") != "0x0055f8b0"
        or owner.get("elementDestructorVa") != "0x0055d100"
        or owner.get("storageConstructorVa") != "0x0055f830"
        or owner.get("storageDestructorVa") != "0x0055cf20"
    ):
        errors.append("owner, lifetime, or storage layout changed")
    context = document.get("helperContext", {})
    if (
        context.get("prepareVa") != "0x00575550"
        or context.get("buildVa") != "0x006c1570"
        or context.get("copyVa") != "0x00573970"
        or context.get("destructorVa") != "0x00573f70"
        or context.get("size") != "0x18"
        or context.get("sourceOwner") != "RaptureElementContainer +0x18 pointee"
        or "application +0x00/+0x04" not in context.get("selector", "")
        or "0x2711" not in context.get("selector", "")
        or "application +0x08" not in context.get("forwardedUnused", "")
        or "not consumed" not in context.get("forwardedUnused", "")
        or "pointee RTTI class is not proven" not in context.get("sourceOwnerBoundary", "")
        or "destroyed immediately after apply" not in context.get("lifetime", "")
    ):
        errors.append("helper context source, selector, or lifetime changed")
    projection = document.get("projection", {})
    projection_map = [
        (row.get("wire"), row.get("storageRecord"))
        for row in projection.get("recordFields", [])
    ]
    header_map = [
        (row.get("wire"), row.get("storage"), row.get("width"), row.get("operation"))
        for row in projection.get("header", [])
    ]
    if (
        projection.get("recordBase") != "+0x18"
        or projection.get("recordStride") != "0x78"
        or projection.get("wireRecordStride") != "0x28"
        or header_map != EXPECTED_HEADER
        or projection_map != EXPECTED_PROJECTION
        or [row.get("storageRecord") for row in projection.get("helperOutputs", [])] != ["+0x20", "+0x74"]
        or [row.get("presentationArgument") for row in projection.get("helperOutputs", [])] != ["Text:String", "Layout:Int"]
        or [row.get("description") for row in projection.get("helperOutputs", [])] != [
            "helper-resolved Sqex::Misc::Utf8String",
            "helper-resolved raw dword copied from the matched tagged referent +0x00",
        ]
        or [row.get("defaulting") for row in projection.get("helperOutputs", [])] != [
            "reset to empty before lookup for each iterated record; lookup failure leaves empty and uniterated slots are not touched",
            "cleared to zero before lookup for each iterated record; success overwrites it, failure leaves zero, and uniterated slots are not touched",
        ]
        or "0x18-byte value context" not in projection.get("helperBoundary", "")
    ):
        errors.append("wire-to-storage projection or helper boundary changed")
    helper = projection.get("helperLookup", {})
    if (
        helper.get("primaryKey") != "+0x00"
        or helper.get("fallbackKey") != "+0x08"
        or helper.get("missingSentinel") != "signed -1"
        or helper.get("unusedAsLookupKey") != "+0x0c"
        or helper.get("entryKeyResolverVa") != "0x006d3fe0"
        or helper.get("entryVectorOffsets") != ["+0x84", "+0x88"]
        or helper.get("entryStride") != "0x10"
        or helper.get("entryKeySource") != "tag-specific referent +0x04"
        or helper.get("stringResolverVa") != "0x006c09e0"
        or helper.get("scalarResolverVa") != "0x006c08a0"
        or "CharaBase" not in helper.get("resolverBoundary", "")
    ):
        errors.append("helper lookup, fallback, or resolver boundary changed")
    keys = document.get("keyDomains", [])
    if (
        [row.get("storageRecord") for row in keys] != ["+0x00", "+0x08", "+0x0c"]
        or "selected RaptureElement +0x88" not in keys[0].get("rules", "")
        or "zero is still looked up" not in keys[0].get("rules", "")
        or "does not prove stable wire identity" not in keys[0].get("stability", "")
        or "signed -1" not in keys[1].get("rules", "")
        or "zero is allowed" not in keys[1].get("stability", "")
        or "never a helper lookup key" not in keys[2].get("rules", "")
        or "no stable identity is proven" not in keys[2].get("stability", "")
    ):
        errors.append("key domains, sentinel rules, or stability boundary changed")
    selected = document.get("selectedObject", {})
    if (
        selected.get("ownerClass") != "Application::Main::RaptureElementContainer"
        or selected.get("activeSlot") != "+0x17838"
        or selected.get("fallbackSlot") != "+0x17834"
        or selected.get("objectBaseClass") != "Application::Main::RaptureElement"
        or selected.get("conditionalCastEvidence") != "Application::Main::Element::Chara::CharaElement"
        or selected.get("derivedCastVa") != "0x004d8f70"
        or selected.get("field") != "+0x88"
        or selected.get("fieldAccessorVa") != "0x004d6750"
        or "constructor argument" not in selected.get("fieldSource", "")
        or selected.get("registryInsertVa") != "0x004da9a0"
        or selected.get("registryLookupVa") != "0x004d9910"
        or selected.get("activeSelectionWriterVa") != "0x004d9980"
        or "0xc0000000-tagged" not in selected.get("nativeKeyEncoding", "")
        or "removal path separately parses" not in selected.get("nativeKeyEncoding", "")
        or "not proven as a wire sentinel" not in selected.get("selectionClearSentinel", "")
        or "container owns the ordered RaptureElement registry" not in selected.get("ownership", "")
        or "cleared when the element is removed" not in selected.get("ownership", "")
        or "stable registry identity" not in selected.get("stability", "")
        or "runtime-selected" not in selected.get("stability", "")
        or "no actor-ID" not in selected.get("stability", "")
    ):
        errors.append("selected-object type, ownership, or identity changed")
    readers = []
    for row in document.get("readerCensus", []):
        location = row.get("storage", row.get("storageRecord"))
        consumers = row.get("consumers")
        if consumers is None:
            consumer = row.get("consumer")
            consumers = [] if consumer is None else [consumer]
        readers.append(
            (
                location,
                tuple(row.get("writers", [])),
                tuple(row.get("readers", [])),
                tuple(consumers),
                row.get("boundary"),
            )
        )
    if readers != EXPECTED_READERS:
        errors.append("reader census changed")
    first = document.get("firstOutwardOperation", {})
    if (
        first.get("applyVa") != "0x0055cf70"
        or first.get("consumerClass") != "MapScreenControl"
        or first.get("consumerVa") != "0x00671400"
        or first.get("retailStrings") != ["MapScreenControl", "group_marker_data", "MapMarkerParty", "Update"]
        or "synchronously" not in first.get("operation", "")
        or "stale suffix" not in first.get("operation", "")
    ):
        errors.append("first outward UI operation changed")
    presentation = document.get("presentationContract", {})
    resource = presentation.get("resourceLookup", {})
    if (
        resource.get("cacheOffset") != "+0x9e8"
        or resource.get("resourceName") != "group_marker_data"
        or resource.get("sourceType") != "Sqwt::ResourceDictionary"
        or resource.get("targetType") != "Sqwt::Data::SqwtXmlDataMaker"
        or resource.get("associatedPath") != "debug/pc_mark_sample.le.spk"
        or "does not prove" not in resource.get("associatedPathBoundary", "")
    ):
        errors.append("presentation resource lookup or package boundary changed")
    count_path = presentation.get("countPath", {})
    if (
        count_path.get("wireType") != "signed byte"
        or count_path.get("storageType") != "signed 32-bit integer"
        or "unsigned" not in count_path.get("applyLoop", "")
        or "signed" not in count_path.get("consumerLoop", "")
        or "negative nonzero" not in count_path.get("boundary", "")
    ):
        errors.append("count comparison or unsafe boundary changed")
    properties = [
        (
            row.get("wire"),
            row.get("storageRecord"),
            row.get("property"),
            row.get("propertyType"),
            row.get("value"),
            row.get("localTransform"),
        )
        for row in presentation.get("properties", [])
    ]
    expected_properties = [
        ("+0x14", "+0x10", "X", "Int", None, "CVTTSS2SI truncation toward zero to signed int32"),
        ("+0x1c", "+0x18", "Z", "Int", None, "CVTTSS2SI truncation toward zero to signed int32"),
        (None, "+0x74", "Layout", "Int", None, None),
        (None, "+0x20", "Text", "String", None, "0x00866010 constructs the literal !!! Utf8String, then 0x0067ac00 finds and erases every occurrence before dispatch"),
        (None, None, "Visibility", "String", "Visible", None),
        (None, None, "SparkleSequence", "String", "m00002", None),
        (None, None, "Template", "String", "MapMarkerParty", None),
    ]
    removal = presentation.get("removal", {})
    if (
        properties != expected_properties
        or len(presentation.get("eligibility", [])) != 3
        or "dense zero-based" not in presentation.get("outputIndex", "")
        or "no separate create branch" not in presentation.get("rowWrite", "")
        or presentation.get("batchOperation") != {
            "operation": "Update",
            "condition": "at least one row was accepted",
            "maximumCallsPerInvocation": 1,
        }
        or removal.get("range") != "inclusive [accepted count, existing count - 1]"
        or removal.get("order") != "descending"
        or removal.get("operation") != "RemoveIndex"
        or "without dispatching Update" not in removal.get("zeroAccepted", "")
        or presentation.get("unusedByThisEffect") != [
            "stored header +0x08", "stored header +0x0c", "stored header +0x10",
            "original application +0x08 forwarded argument", "record +0x14", "storage +0x798"
        ]
    ):
        errors.append("property flow, row lifecycle, or unused-field verdict changed")
    deferred = document.get("deferredGate", {})
    if (
        deferred.get("vtableOwner") != "PcSearchWidgetOperator"
        or deferred.get("vtableSlot") != 29
        or deferred.get("methodVa") != "0x00691f30"
        or deferred.get("condition") != "+0x798 is nonzero and +0x14 equals one"
        or "not the first outward consumer" not in deferred.get("boundary", "")
    ):
        errors.append("deferred refresh gate changed")
    verdict = document.get("verdict", {})
    expected_rejections = [
        "a semantic party-marker packet type is not an evidence-backed packet name",
        "the physical capacity of sixteen records is not a safe runtime count",
        "record +0x00 is not named actor ID and record +0x08 or +0x0c is not named marker type or map ID",
        "the unused middle float is not proven to be Y, and no radius, rotation, icon, label, or color field is proven",
        "X and Z are UI property identities after truncation, not proof of a world-coordinate wire contract",
        "the stored header copies and +0x798 gate do not influence this presentation effect; the original application +0x00/+0x04 pair separately selects the helper context",
        "group_marker_data and MapMarkerParty are UI resource and template literals, not proof of group, party, actor, marker, or map key domains",
        "Layout and Text are presentation property names, not native names for record +0x74 or +0x20",
        "the conditional CharaBase string source does not prove an Application::Scene::Actor state edge or actor identity for a wire selector",
    ]
    if (
        verdict.get("uiConsumers") != ["MapScreenControl::0x00671400"]
        or verdict.get("luaOrNapiConsumers") != []
        or verdict.get("networkEmissions") != []
        or verdict.get("rejectedInterpretations") != expected_rejections
        or "computed or dynamic indirect" not in verdict.get("remainingBoundary", "")
        or "source package" not in verdict.get("remainingBoundary", "")
    ):
        errors.append("consumer verdict or remaining boundary changed")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (
        r"[A-Za-z]:\\\\",
        "/" + "Users/",
        "/" + "home/",
        "agent-islands",
    )
    rejected_packet_name = "PartyMapMarker" + "UpdatePacket"
    if (
        any(re.search(pattern, text, re.I) for pattern in forbidden)
        or rejected_packet_name.lower() in text.lower()
    ):
        errors.append("manifest contains a private path or rejected packet name")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations: list[dict] = []
    for path, value in [
        (("wireContract", "neutralName"), "invented-name"),
        (("wireContract", "countBoundary"), "sixteen rows are safe"),
        (("route", "storageThisVa"), "0x004dd1a6"),
        (("owner", "recordStride"), "0x28"),
        (("projection", "helperBoundary"), "NamedLookupContext"),
        (("helperContext", "size"), "0x10"),
        (("helperContext", "forwardedUnused"), "application +0x08 selects the context"),
        (("helperContext", "lifetime"), "persistent global context"),
        (("helperContext", "sourceOwner"), "party state"),
        (("selectedObject", "objectBaseClass"), "Application::Scene::Actor"),
        (("selectedObject", "fieldAccessorVa"), "0x004d6760"),
        (("selectedObject", "ownership"), "unowned raw pointer"),
        (("selectedObject", "activeSelectionWriterVa"), "0x004d9910"),
        (("firstOutwardOperation", "consumerClass"), "Unknown"),
        (("deferredGate", "vtableSlot"), 28),
        (("verdict", "luaOrNapiConsumers"), ["invented"]),
        (("verdict", "networkEmissions"), ["invented"]),
    ]:
        changed = copy.deepcopy(document)
        changed[path[0]][path[1]] = value
        mutations.append(changed)
    changed_projection = copy.deepcopy(document)
    changed_projection["projection"]["recordFields"][3]["storageRecord"] = "+0x14"
    mutations.append(changed_projection)
    changed_header = copy.deepcopy(document)
    changed_header["projection"]["header"][2]["storage"] = "+0x14"
    mutations.append(changed_header)
    dropped_reader = copy.deepcopy(document)
    dropped_reader["readerCensus"].pop(4)
    mutations.append(dropped_reader)
    changed_writer = copy.deepcopy(document)
    changed_writer["readerCensus"][0]["writers"] = []
    mutations.append(changed_writer)
    changed_boundary = copy.deepcopy(document)
    changed_boundary["readerCensus"][7]["boundary"] = "complete"
    mutations.append(changed_boundary)
    leaked = copy.deepcopy(document)
    leaked["source"]["path"] = "agent-islands/private-evidence"
    mutations.append(leaked)
    rejected_name = copy.deepcopy(document)
    rejected_name["wireContract"]["neutralName"] = "invented-party-marker-packet"
    mutations.append(rejected_name)
    for section, key, value in [
        ("resourceLookup", "resourceName", "invented"),
        ("resourceLookup", "associatedPathBoundary", "proven source package"),
        ("countPath", "applyLoop", "signed safe loop"),
        ("countPath", "boundary", "sixteen rows are clamped"),
        ("removal", "order", "ascending"),
        ("removal", "zeroAccepted", "dispatch Update"),
    ]:
        changed = copy.deepcopy(document)
        changed["presentationContract"][section][key] = value
        mutations.append(changed)
    changed_helper_key = copy.deepcopy(document)
    changed_helper_key["projection"]["helperLookup"]["fallbackKey"] = "+0x0c"
    mutations.append(changed_helper_key)
    changed_helper_default = copy.deepcopy(document)
    changed_helper_default["projection"]["helperOutputs"][0]["defaulting"] = "retain prior value"
    mutations.append(changed_helper_default)
    changed_helper_type = copy.deepcopy(document)
    changed_helper_type["projection"]["helperOutputs"][0]["description"] = "label"
    mutations.append(changed_helper_type)
    changed_entry_key = copy.deepcopy(document)
    changed_entry_key["projection"]["helperLookup"]["entryKeySource"] = "tagged entry +0x00"
    mutations.append(changed_entry_key)
    changed_key_stability = copy.deepcopy(document)
    changed_key_stability["keyDomains"][0]["stability"] = "stable actor identity across updates"
    mutations.append(changed_key_stability)
    changed_x = copy.deepcopy(document)
    changed_x["presentationContract"]["properties"][0]["localTransform"] = "round to nearest"
    mutations.append(changed_x)
    changed_middle = copy.deepcopy(document)
    changed_middle["presentationContract"]["properties"][1]["wire"] = "+0x18"
    mutations.append(changed_middle)
    changed_literal = copy.deepcopy(document)
    changed_literal["presentationContract"]["properties"][5]["value"] = "m00003"
    mutations.append(changed_literal)
    changed_batch = copy.deepcopy(document)
    changed_batch["presentationContract"]["batchOperation"]["maximumCallsPerInvocation"] = 16
    mutations.append(changed_batch)
    changed_unused = copy.deepcopy(document)
    changed_unused["presentationContract"]["unusedByThisEffect"].remove("storage +0x798")
    mutations.append(changed_unused)
    invented_actor_id = copy.deepcopy(document)
    invented_actor_id["verdict"]["rejectedInterpretations"][2] = "record +0x00 is actor ID"
    mutations.append(invented_actor_id)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]


def main() -> int:
    errors = verify() + mutation_test()
    if errors:
        print(f"FAIL: {len(errors)} s2c 0x018D client-consumer error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    print("PASS: s2c 0x018D client-consumer manifest and 41 mutations")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
