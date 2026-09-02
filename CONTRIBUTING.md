# Contributing to XIVLegacy Decompilation Research

A contribution records or improves a finding about the FINAL FANTASY XIV
1.23b Windows client, or maintains a reproducible tool used to establish one.

## Findings

Follow the claim boundary in
[docs/ai_agents/evidence-and-claims.md](docs/ai_agents/evidence-and-claims.md).
A finding must identify:

- the client build and executable;
- a durable locator such as an RVA, VA, RTTI symbol, opcode, structure offset,
  or file-format field;
- the tool and method used to inspect the client evidence;
- the observation, interpretation, confidence, and uncertainty; and
- any prior-art source used for a name, layout, or research lead.

Make the narrowest claim supported by the cited evidence. Repository code,
tests, catalogs, headers, and agent output can support repository maintenance,
but they do not independently prove client behavior. Preserve every source,
permission, date, and provenance citation exactly.

## Repository changes

Put each finding in one canonical page or catalog and link to it elsewhere.
Keep [docs/README.md](docs/README.md) useful as an entry point. Change generated
artifacts through their retained generator, and keep researcher-supplied
binaries and local analysis output untracked.
Keep original executables outside the checkout or under the ignored `orig/`
path, and keep generated assembly under the ignored `asm/` path. Never commit,
redistribute, or include either kind of artifact in generated output.

Use plain ASCII in authored prose and comments. Do not alter inherited legal,
copyright, SPDX, permission, or stamped-provenance text.

## License

Contributors license their original contributions under
[AGPL-3.0-or-later](LICENSE). This does not license client binaries or
third-party expression represented by a research artifact. Required
attribution is recorded in source provenance and the README
[License](README.md#license) section.
