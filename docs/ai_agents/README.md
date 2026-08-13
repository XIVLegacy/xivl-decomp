# AI-assisted contributions

AI-assisted work follows the same contribution and evidence contract as any
other work. The contributor owns every submitted claim and must be able to
explain its source, scope, uncertainty, and verification.

Agent output is not decompilation evidence. A durable client claim starts from
disassembly or decompiled output produced from a contributor-supplied retail
FINAL FANTASY XIV 1.23b binary. Read
[evidence-and-claims.md](evidence-and-claims.md) before promoting an
observation into a document, catalog, header, or symbol name.

## Public contract

Tracked documentation describes current client knowledge and the reproducible
method used to recover it. Keep branch state, session narration, and private
maintainer context out of the public tier.

The repository publishes findings, catalogs, evidence-preserving headers, and
analysis tools. It does not publish retail binaries, generated assembly, Ghidra
project state, or the local source reconstruction. The ignored `orig/` and
`asm/` paths are local-only inputs and outputs, not tracked placeholders.

Byte-identical recompilation is retired. Do not introduce match percentages,
matching queues, per-function build rules, or verification claims based on the
old matching workflow.

## Documentation policy

The root [README.md](../../README.md) defines project scope. The
[documentation index](../README.md) lists every tracked page under `docs/`.
This policy tier owns evidence and claims, comments and prose, and repository
verification.

Use short paragraphs, ASCII punctuation, and concrete names. Put each client
fact in one canonical home and link to it elsewhere. Preserve binary names,
addresses, RTTI symbols, tool names, confidence labels, uncertainty, and
provenance citations when they carry the claim.

The README [License](../../README.md#license), source provenance, and cited
research references are the authoritative attribution record.
Never trim, paraphrase, relocate, or normalize a provenance citation. A date
inside an external citation is part of that citation and stays.

## Policy shelf

| Question | Page |
|---|---|
| Public docs entry point | [docs/README.md](../README.md) |
| Evidence and claim boundaries | [evidence-and-claims.md](evidence-and-claims.md) |
| Comments and public prose | [comments-and-prose.md](comments-and-prose.md) |
| Repository checks and their limits | [verification.md](verification.md) |
