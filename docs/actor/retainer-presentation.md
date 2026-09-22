# Retainer presentation contract

This note records the client-owned retainer presentation recovered from the
FFXIV 1.23b scripts, actor tables, and installed resource corpus. It separates
those presentation paths from server-owned hiring, inventory, naming, and
retainer-lifetime decisions.

## Bell ownership

The recovered `RetainerFurniture.eventRingBell` function calls
`_runCharaScheduler(67522560)` and then `cancelMainTargetCharacter`. The packed
selector is `0x04065000`: category 4, actor-relative common-library bank 0101,
and low selector 0.

Actor classes 1200027 and 1200135 both have class path
`/Chara/Npc/Object/RetainerFurniture`. Their actor-appearance rows resolve to
base 20958 and body 1024, or BG-object family b958/e001. That owner join selects
`b958/lib/0101/main`; the installed b928 bank with the same numeric library ID
is a collision, not a retainer-bell owner.

The b958 bank is 53,264 bytes and contains 13 recursively decoded resources.
Its main SCB is 1,280 bytes with SHA-256
`d2db8131ff54ab669478752f6e897e7fb41c9f609c9cf4a92555d67fb837a6eb`.
The authored block contains four clips, all starting at zero: BindActor,
RaptureSound, Action, and Motion. The block duration is 1.2 seconds; its outer
SCB envelope is 9 seconds and is not the bell animation duration. The motion is
`cbfm_mp_act1`, 120 frames at 30 fps.

The local event flow rings the bell before requesting the selected retainer
spawn. The scheduler is therefore verified client presentation, not evidence
that the widget result authorizes or performs a server transaction.

## Hiring previews

`PopulaceRetainerManager.eventTaklSelectCutSeane` creates a city-specific scene
and calls `startCutScene` with mode `(1, 61, 2, 0)` plus five candidate actor
class IDs. Each installed scene has two
`RaptureCharaActorClassIdClip` records that replace its dynamic Retainer
placeholder from those arguments. The SCB then owns the preview actor, motion,
facial, camera, timing, and cancellation choreography.

| Scene | City | Manager class | Dynamic placeholder | Actors | Authored blocks | Clips | Retainer clips | Retainer motions |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `rtn0l010` | Limsa Lominsa | 1000166 | 1000010 at actor 5 | 25 | 28 | 587 | 126 | 25 |
| `rtn0u010` | Ul'dah | 1000865 | 1000002 at actor 5 | 23 | 28 | 605 | 142 | 30 |
| `rtn0g010` | Gridania | 1001184 | 1000002 at actor 4 | 25 | 28 | 581 | 130 | 29 |

The installed main and SCB identities are:

| Scene | Main SHA-256 | SCB SHA-256 |
| --- | --- | --- |
| `rtn0l010` | `e7510541b450f701078769eeff84956177c2b36a5e8677b9985adb26be6804e1` | `b544a1824cae786e66806096a03c23e735a997777f2ed53ffd202c40a65ebc6b` |
| `rtn0u010` | `10642df5bee3aab050d14f06cd618a2084485d4a8aa7bd52bb199ccd9c86d47f` | `a49fd4f96f40a3502c3f65ad312bf483fe0fa6d9b2869a11ef6abf9cb7e1be2b` |
| `rtn0g010` | `faf8a23098ac8f62f60c3c2217a2960a916b114966509a6b177bbfa08f116254` | `43a7c937bb6a4cee9125e2385878c796414d3be70bdd2e7cb7f6a730aaca89a7` |

All 225 hiring candidate IDs join directly to actor-class path
`/Chara/Npc/Retainer/OrdinaryRetainer` and authoritative appearance rows. The
appearances cover nine PC families, c001 through c009. The three city pools
contain 75 candidates apiece. The preview scenes reference 48 unique motion
names; their family-aware resolution retains 936 bank/motion candidates across
685 physical action files. A scene motion name is not a unique packed
animation selector, so collisions remain unresolved rather than being reduced
to an invented bank choice.

## Ordinary-retainer dialogue

The recovered 205-instruction `OrdinaryRetainer.sayToPlayer` bytecode defines
the exact speech-row selection for all 225 candidates and ten message types.
For the candidate offset `delta`, it computes `q = floor(delta / 5)` and
`r = fmod(delta, 5)`, maps `q` through
`[0, 1, 8, 2, 3, 2, 3, 4, 5, 4, 5, 6, 6, 7, 7]`, and calls:

```text
self:say(self, 9 * r + message_base + mapped_slot, 0, argument)
```

The ten message bases are 2, 102, 202, 302, 402, 502, 602, 702, 802,
and 902. This establishes the client's dialogue-row mapping. No call to
`sayToPlayer` was found in the audited local server retainer scripts, so it
does not establish when retail requested any message type.

## Installed animation boundary

The nine PC families contain both `emp_emp/bid` banks 0000 and 0001. The
decoded corpus comprises 18 banks, 1,584 MTB/MCB/CIBT resources, 144 embedded
SCBs, and 1,215 scheduler clips. These assets establish available character
presentation, not the runtime state selected for a spawned world retainer.

Every candidate appearance has zero weapon fields. No retainer-specific
category-0x13 WSS caller was recovered, and no historical runtime capture
records the generic native BID actor-flag branch for a retainer. A battle
animation, WSS bank, state transition, or equivalent world-event selector must
therefore remain unselected.

## Provenance and evidence boundary

The actor joins use `gamedata_actor_class.sql` SHA-256
`f2fc48bccb6162c923fb33d05ebaeaa2ccc8ed2e2f2bfc2b8ac96a6fd4b8e112`,
`gamedata_actor_appearance.sql` SHA-256
`bf46cc9438b1a2ea1850b0b856519a952b00058973661453a30e06028e21c015`,
and `gamedata_retainer_candidates.sql` SHA-256
`65644001444c4737725a6cfc3e4b370e3846d7f278b323200c98e37477862da9`.
The recovered RetainerFurniture, PopulaceRetainerManager, and OrdinaryRetainer
Lua sources have SHA-256 values
`10dd0741629664c5620d4edff137e063be94460ed52061d266048f1a18f274bf`,
`619b5e158c7b8bb8cdf863fd60cb82804ae42b6d23b57fcd358124d1fa39e287`,
and `53ab90efdd0bc7d05b921b95c762d0b7636f3ee0edcb8e24154cda2c21c1a909`.

The evidence proves the bell owner and scheduler, candidate appearance joins,
hiring-preview choreography, dialogue selector formula, and installed PC
animation corpus. It does not recover retail server authorization, persistence,
inventory or naming validation, world-retainer battle state, WSS selection, or
the runtime occasions on which dynamic speech was requested. Those behaviors
must not be inferred from class or asset existence alone.
