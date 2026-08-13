# Work-field evidence index

Client Lua API methods connect to the work-table fields they read through these
durable client-side artifacts:

- [C++-bound Lua declaration census](../script/lpb-corpus.md)
- [Work-table state-field inventory](work-field-inventory.md)
- [`tools/extract_cpp_bindings.py`](../../tools/extract_cpp_bindings.py)
- [`tools/extract_work_fields.py`](../../tools/extract_work_fields.py)

The extraction identified 130 state fields across the client work tables.
Dot-separated paths such as `playerWork.tribe` describe script access.
Slash-separated paths such as `playerWork/journal` are Murmur2-hashed wire
property identifiers; see [MurmurHash2](../resource/murmur2.md).

The former server-coverage comparison and its generated output were not carried
into this knowledge repository. The retained finding is the client field
requirement itself: a field read by a shipped client script must be available
through client state initialization, SetActorProperty, or work-sync behavior.
