# Group member name getter relationship

The retail 1.23b `ffxivgame.exe` member-name getters share a lookup and have
the same first result in the string arm. This does not establish an invariant
for every linkshell member.

The input is the 15,996,808-byte `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses below are VAs with image base `0x00400000`; subtract the base for
RVAs. Ghidra instruction exports were checked
byte-for-byte against the PE section mappings using Python `struct` and
`hashlib`: all 840 instruction encodings in the 14 functions below matched.
The original export invocation and tool version were not
retained, so analysis settings beyond the instruction bytes remain unknown.

The `_getMemberLocalizedDisplayName` registration at `0x0074D400` retains
callback `0x00700110` and literal `0x00FD7DE8`. The
`_getMemberDisplayName` registration at `0x0074D6A0` retains callback
`0x00700280` and literal `0x00FD7E08`. Both callbacks subtract one from the
supplied Lua member index and call `0x006C0D80`, which forwards to
`0x006C09E0` to obtain the same member-name descriptor.

The shared lookup at `0x006C09E0` indexes 16-byte member entries. Its type-3
branch copies an existing descriptor through `0x006BF500`, its type-0 branch
selects an actor's descriptor at `+0x60`, and other types use the empty
constructor `0x006BF420`. The empty constructor writes `-1` to descriptor
`+0` and constructs an empty string. These entry types do not establish a
character-ID allocation domain for the returned value.

The localized callback passes the descriptor's string field at `+4` to
`0x007487C0`. The other callback tests the descriptor's first dword against
`-1` at `0x0070030B-0x0070031A`. For `-1`, it passes the same `+4` string
to the same result helper at `0x0070033E-0x00700343`. Otherwise it returns
the descriptor's first dword through `0x00584F70`. Its numeric branch appends
a nil result, while the string branch appends the byte at descriptor `+0x58`.
`0x007487C0 -> 0x0073DF70` constructs internal string tag 3,
`0x00584F70 -> 0x00583D90` constructs integer tag 0, and
`0x00748920 -> 0x00583F70` constructs nil tag 6. These internal tags are
not the outbound command serializer's tags. The return paths do not
perform a character-ID conversion. The descriptor writer's invariant for
every linkshell member, and equality across concurrent state changes, remain
unestablished. For a fixed receiver, context and member index with unchanged
descriptor `+0 == -1`, the two getters therefore have the same first value.
The integer branch's semantic ID domain remains unresolved.

The general Group update route for clientbound `0x017A` reaches
WorkSyncUpdater and `_onUpdateWork`, as recorded in
[the Group updater finding](../net/sync-writer.md#group-updater-apply-and-notification).
That route is not joined to command 24231 or a master-transfer result.
The generic function-delivery route `0x0130 -> 0x004D8860 -> 0x00575040 ->
0x0076C220` is documented in
[the function receiver finding](start-event-fn-receiver.md).
`SendLogReceiver` at `0x0089FBF0` reaches LuaActorImpl slot 63 at
`0x00759FD0`, as recorded in
[the receiver dispatch inventory](../net/actorimpl-receiver-dispatch.md).
Neither route establishes a transfer-specific callback, text-row binding,
recipient split or success/refusal sequence.

To reproduce the getter observation, disassemble the pinned PE at the two
registration VAs, follow their retained callbacks and the common lookup,
then inspect the string/integer/nil result helpers named above. Preserve
the callback's stack adjustments when locating the descriptor's `+4`
string field. Map each instruction RVA through the PE section table to
verify its bytes against the identified binary. This finding retains the
observation and locators rather than the private instruction exports.
