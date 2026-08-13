# Public headers

This directory contains C++ headers that another project can include when it
needs a client layout, protocol declaration, registry, or recovered interface.
The conventional `include/` name is intentional.

The headers under `structs/ffxivgame/` are generated layout snapshots. Their
generator and provenance caveats are recorded in each file. The headers under
`actor/`, `install/`, `net/`, and `sqex/` contain curated headers for interfaces
and catalogs supported by evidence. Preserve their offsets, ABI constraints, locators,
and provenance when changing them.

This repository has no build system and does not produce a library. Consumers
select the headers they need and provide any implementations required by
declaration-only interfaces. A header being compilable establishes its C++
interface contract; it does not independently prove the documented client
behavior.
