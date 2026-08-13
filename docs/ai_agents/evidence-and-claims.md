# Evidence and claims

This repository records facts recovered from the retail FINAL FANTASY XIV
1.23b Windows client binaries. A contributor supplies those binaries from a
legitimate retail installation. The binaries themselves are never committed.

## What counts as evidence

Disassembly and decompiled output count as direct client evidence when the
record identifies all of these:

- the retail 1.23b binary, such as `ffxivgame.exe`
- a durable locator, normally an RVA or VA, or an RTTI symbol with its locator
- the tool that produced the disassembly, decompilation, RTTI walk, xref set, or
  extraction result
- the observation made from that output
- the interpretation, confidence, and unresolved ambiguity

The tool is an instrument, not the authority. A decompiler's inferred type,
signature, variable name, or control-flow rendering remains an interpretation
until the underlying instructions, data, or independent client evidence
supports it.

Repository code, tests, catalogs, headers, and validation results establish
repository contracts. They do not prove a client behavior by themselves.
Agent output, summaries, search snippets, and uncited notes are leads, not
evidence. Inspect the contributor-supplied binary output before promoting a
fact.

## Claim boundary

Make the narrowest claim supported by the cited output.

| Observation | Supported claim | Unsupported leap |
|---|---|---|
| Instructions at a cited function address | Reads, writes, calls, constants, and branches visible there | A semantic function name with no independent anchor |
| RTTI data and its cited xrefs | Type identity, inheritance evidence, vtable relationships, and use sites visible in the binary | Complete object layout or behavior not reached by the evidence |
| A cited vtable and callsite set | Slot ordering and observed dispatch relationships | The purpose of untraced slots |
| A cited decompiler rendering | A reviewable interpretation of the underlying function | Source-level authorship, exact original C++, or retail source code |
| A repository extractor result | The deterministic output of that tool for the stated input | Correctness beyond the input and algorithm |

Keep observation and interpretation separate. Preserve placeholder names when
the evidence does not establish a stable identity. Record conflicts instead of
merging candidates into one assertion.

An address must state enough context to be reproducible. Distinguish VA from
RVA, name the executable, and retain the RTTI mangled symbol when it is the
identity anchor. Name the producing tool or repository script. If tool settings
or an image base affect the result, record them with the claim.

## Local evidence boundary

The public repository retains durable conclusions and reproducible analysis
tools, not the retail inputs or bulk local analysis state. Original
executables, generated assembly, Ghidra projects, and local decompiled output
stay outside the tracked tree. The ignored `orig/` and `asm/` paths may hold
local inputs and outputs, but never tracked artifacts.

Do not use the retired byte-identical recompilation workflow as an evidence
class or verification method. A historical match note does not replace a
current binary locator and tool-backed observation.

A claim that leaves this repository must be citable without it. Because the
local decompiled output is untracked, a consumer cannot resolve a citation
that points into it, so any finding intended for promotion elsewhere is first
stated in a tracked artifact here: a docs page, a `config/` catalog entry, or
a header. State the binary, the locator, the producing tool, and the
observation, so the claim carries its own derivation. If a finding cannot be
stated that way without losing the argument, say so rather than citing
untracked output, and raise it - the function may need to ship as a tracked
exhibit instead.

## Prior art and attribution

An imported statement from an external project is a research lead, never direct
evidence, until independently established against the contributor's binary.

Preserve every provenance citation exactly. Do not trim it, reword it for
brevity, relocate it, or remove dates that belong to the external citation.
Keep the source identity and the local binary observation distinct so a reader
can tell which part was incorporated and which part was independently
confirmed.

## Numbers in prose

Keep a number when it carries the claim: an address, offset, size, slot,
opcode, image base, byte sequence, hash, or exact extraction result. Preserve
it verbatim with its locator and method.

Omit an incidental count when the sentence's meaning survives without it. Do
not hedge an evidence value with "approximately", "roughly", "about", or a
leading `~`. Make it exact, qualify the unresolved boundary, or remove it.

A figure inside an external citation or preserved source text stays verbatim,
including its original date or approximation.
