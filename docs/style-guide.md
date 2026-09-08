# Style guide

Repository style covers authored Python, Ghidra Java, C and C++ headers,
catalogs, and documentation. It does not establish client behavior, symbol or
type identity, evidence strength, or research provenance.

## General

- Prefer existing local patterns once they exist.
- Keep changes scoped to the finding, catalog, header, or tool being changed.
- Use small, explicit functions and modules before adding abstractions.
- Follow the [comment policy](ai_agents/comments-and-prose.md) for source
  comments.
- Do not reformat generated catalogs, evidence records, or imported
  declarations for style alone.

### Documentation

The public [documentation policy](ai_agents/README.md#documentation-policy) is
canonical for authored documentation. The
[evidence policy](ai_agents/evidence-and-claims.md) owns claim wording,
citations, confidence, and provenance.

## Python

- Use 4 spaces for indentation and no tabs.
- Use `lower_snake_case` for modules, functions, and variables,
  `UpperCamelCase` for classes, and `UPPER_SNAKE_CASE` for constants.
- Group imports as standard library, third-party packages, then local modules.
- Prefer `pathlib.Path` for filesystem paths and explicit text encodings.
- Keep command entry points thin; put reusable work in importable functions.
- Raise or report specific failures instead of using broad exception handlers.
- Add type annotations where they clarify catalog records, addresses, paths,
  or public helper contracts.

## Ghidra Java

- Use 4 spaces for indentation, no tabs, and braces around control-flow
  bodies.
- Use `UpperCamelCase` for classes and `lowerCamelCase` for methods, fields,
  parameters, and local variables.
- Keep post-scripts read-only unless their documented contract explicitly owns
  a repository output.
- Keep address parsing, program selection, and output formatting explicit.

## C and C++ headers

- Use Allman braces, 4 spaces for indentation, no tabs, one statement per
  line, and braces around single-statement bodies.
- Use `Type* ptr` pointer declarations.
- Run `clang-format -i` on changed authored headers.
- Keep declarations minimal and include what the header directly needs.
- Preserve evidence-backed names, addresses, offsets, widths, and comments.
- Do not hand-edit generated headers or normalize imported declarations for
  style alone.

The root `.clang-format` encodes mechanical formatting. This guide remains
authoritative when a formatter would alter evidence identity or generated
content.

## Verification

Use the owning commands in [tools/README.md](../tools/README.md). Formatting is
not a substitute for catalog, header, link, or evidence validation.
