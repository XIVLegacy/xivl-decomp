# Comments and prose

Deletion is the default for explanatory comments. Keep a comment only when it
records a current fact or constraint that names and types cannot show.

Keep comments for:

- a binary, address, RVA, RTTI, vtable, ABI, or wire fact
- the disassembler, decompiler, or repository tool that produced an observation
- an evidence or provenance citation
- a safety or distribution boundary
- a non-obvious extraction, regeneration, or API contract
- ownership of generated output when the file cannot make it clear

Compress a survivor to one to three lines at the use site. Move a longer
contract to a public policy, tool, or research page and leave a short pointer.
Remove branch-time narration, matching progress, agent assignments, and notes
that only explain the next statement.

## Repository surfaces

- In `tools/`, keep comments for PE assumptions, address conversion, Ghidra
  interfaces, extraction boundaries, and input safety. Do not narrate control
  flow or a command that is already clear.
- In `include/`, keep layout offsets, ABI constraints, symbol locators, and
  evidence citations. Do not turn a header into a research diary.
- In `config/`, preserve evidence fields and source metadata exactly. JSON has
  no ordinary comment syntax, so do not encode disposable commentary as data.
- In `docs/`, state the observation, its locator, the tool,
  its interpretation, and remaining uncertainty. Prefer dated past-tense facts
  when time matters.

Python docstrings and command help are runtime text. Treat them as public tool
contracts. Tighten inaccurate text, but retain required inputs, outputs,
failure modes, and evidence boundaries.

Generated output belongs to its owning catalog or tool. Change the canonical
input or generator and regenerate it. Do not hand-edit generated prose or
headers to hide drift.

## Provenance text

Source, reference, permission, and provenance citations are immutable text for
editing purposes. Never shorten them, rewrite them for house style, move them
to a less visible location, or remove a date embedded in an external citation.
Inherited legal, copyright, SPDX, permission, and stamped-provenance blocks
also remain verbatim even when their punctuation differs from current style.

## Examples

Keep a client locator and method:

```cpp
// ffxivgame.exe RVA 0x00385bf0, recovered by Ghidra decompilation.
```

Keep a safety boundary:

```python
# Read the caller-supplied executable; never copy it into the repository.
```

Delete narration:

```python
# Increment the index.
index += 1
```

## Authored public prose

Public prose uses a plain, direct register.

- Use ASCII punctuation and short declarative sentences.
- Avoid over-hyphenation and invented compound modifiers. Established
  technical terms keep their hyphens.
- Use semicolons sparingly, preferring periods, commas, or short lists.
- Prefer exact executable names and locators over vague references.
- Separate observed instructions or data from the interpretation they support.
- State uncertainty directly. Do not promote a plausible name into a confirmed
  identity through confident wording.
- Omit progress counts and volatile inventories unless the number is the claim
  and its source is cited.
- Do not describe the retired byte-matching workflow as current policy.

Internal working notes are outside this public policy tier.
