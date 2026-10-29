# Sqwt ProgressBar template style and indicator sizing

A ProgressBar without a Template can construct a native default template with a track and indicator. The default indicator sizing path reads that ProgressBar own Value, Minimum and Maximum. Template-root parenting supplies a conditional native path for inheriting the owner SqwtStyle into a child without a local style. These are static native mechanisms, rather than evidence that a particular widget rendered.

## Input and method

The input is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build `2012.09.19.0001`, size `15996808`, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. All addresses are VA with PE32 image base `0x00400000`.

The instruments are read-only focused Ghidra CLI queries on the analyzed executable, Node Buffer PE section mapping and RTTI/table reads, and a separate Python standard-library comparison of retained instruction bytes with the exact PE. The Python check also independently recalculates relative CALL targets. Decompiled views guide navigation. Instructions and referenced data carry the claims below. No client execution, image rendering or fresh-project reproduction is established.

## Template selection and parts

Constructor `0x009B6640` installs ProgressBar tables at the receiver, receiver+0xB4 and receiver+0x194. The primary table is `0x0107E9D4`. Its locator `0x011813F4` names type descriptor `0x012BD800`, whose RTTI name is `.?AVProgressBar@Controls@Sqwt@@`.

The primary table slot 11 targets `0x00937190`. This entry tests receiver+0x318 and calls virtual offset 0x110 only when that field is zero. Descriptor initializer `0x00F22640..0x00F22659` names descriptor `0x01359118` Template with index 17. The constructor association at `0x00938E94..0x00938EA8` joins it to +0x318. ProgressBar slot 68 at virtual offset 0x110 targets `0x009B64A0`. Thus the checked initialization entry creates the default template only when Template is null at that time. It does not prove the state after every possible late-template replacement.

The creator allocates 0x6C0 bytes, calls `0x009B5CB0` with the new allocation as ECX and the ProgressBar owner as its stack argument, then stores the result at owner+0x378. These operations are at `0x009B64C5..0x009B64F8`. The result is passed to the Template setter `0x00938140` at `0x009B6507`.

The helper constructor installs table `0x0107E854`. Its RTTI name is `.?AVProgressBarDefaultTemplate@ProgressBar@Controls@Sqwt@@`. It constructs a Bar at helper+0xD0 and a Border at helper+0x374, then retains that Border as FrameworkTemplate root +0xB0. The Border retains the Bar pointer at Border+0x33C. The relevant constructor instructions are `0x009B5CDC..0x009B5D18` and `0x009B5B0F`.

| Role | Object | Primary table | RTTI type descriptor |
| --- | --- | --- | --- |
| Track | helper+0x374 Border | `0x0107E734` | `0x012E8850` |
| Indicator | helper+0xD0 Bar | `0x0107E4EC` | `0x012E8808` |

The corresponding RTTI names are `.?AVBorder@ProgressBarDefaultTemplate@ProgressBar@Controls@Sqwt@@` and `.?AVBar@ProgressBarDefaultTemplate@ProgressBar@Controls@Sqwt@@`. Constructor naming at `0x009B5D25..0x009B5D89` assigns PART_Track to the Border and PART_Indicator to the Bar.

For a zero orientation input, `0x009B5DD0` requests ProgressBar:HorizontalBackground on the track Border and ProgressBar:HorizontalIndicator on the indicator Bar. The callsites are `0x009B5E57` and `0x009B5ECE`, with receiver adjustments at `0x009B5E46` and `0x009B5EC0`. Nonzero orientation takes the corresponding Vertical branch. The assignments go through `0x00930BA0`, whose unnamed-target branch writes SqwtStyleControlName descriptor `0x01358838`. Its name initializer is `0x00F220A0..0x00F220B9`. These native keys correspond to style parts, separately from the style name.

## Conditional style inheritance

SqwtStyle descriptor `0x01358958` has name string `0x0106AA00`, index 22 and flags 0x3F. SqwtStyleFile descriptor `0x01358A78` has name string `0x0106AA0C`, index 21 and flags 0x3F. The initializers are `0x00F220C0..0x00F22113`. The FrameworkElement property array at `0x012E0B78` contains those descriptors at elements 20 and 19. Initializer `0x00F22160..0x00F221B6` passes the array and count 23 to the property-info registration constructor.

When startup collection is enabled, `0x0098B250` collects these descriptor arrays. Registry `0x0098B310`, at `0x0098B387..0x0098B39F`, selects descriptors whose +0x18 flags include 0x20 and passes them to the inherited-name list at `0x0135F700`. Registry population is a condition of the following path.

Template application `0x00936DF0`, at `0x00936EB8..0x00936EDB`, obtains the root from Template+0xB0. When that root is nonnull and root+0x240 is null, it calls `0x00940B30` and `0x009313C0` with the template owner as ECX and the root as argument. The latter is the checked parent attachment path. It conditionally stores the owner in child+0x240 and calls `0x00912C10` on the child at `0x00931489`. The default Border constructor attaches its Bar through the same entry at `0x009B5B64`.

Inherited refresh `0x00912C10` obtains the parent through receiver virtual offset 0x0C. The checked ProgressBar, Border and Bar primary tables use `0x0092B5D0`, which returns receiver+0x240. The refresh reads each inherited property from the parent through `0x00911020` with flag 1 at `0x00912CAF`. Only AL=1 reaches the child write through `0x00912850`, also with flag 1, at `0x00912CC2`. The read path checks descriptor mask 0x10. The write path checks mask 0x20 before property-info dispatch. Per-instance metadata overrides remain conditions.

SqwtStyle index 22 maps to receiver+0x228. The getter dispatch table `0x0092BFAC` has index-20 slot `0x0092BFFC` pointing to `0x0092BF08`, which reads that field. The setter table `0x00930A50` has index-20 slot `0x00930AA0` pointing to `0x00930962`. Its branch supplies descriptor `0x01358958` and wrapper receiver+0x228 at `0x00930969..0x00930983`.

The inherited string store reaches `0x00983FE0`. It rejects a flag-1 write when wrapper bit 1 is clear and bit 0 is clear. A local flag-0 write clears bit 0 at `0x0098400D..0x00984010`. An unoverridden string wrapper initialized by `0x009B2E50` from the checked flags has bit 0 set and bit 1 clear. This establishes inherited admission and preservation of a local override on the checked path.

Skin-part entry `0x0092A5F0` passes source +0x290, direct skin override +0x238, part/control name +0x220 and style name +0x228 to `0x00934210` at `0x0092A630..0x0092A65E`. That function uses the [published part resolver](sqwt-tab-scroll-text-parts.md#style-part-resolution). The inherited style name and explicit ProgressBar part key are distinct native inputs.

This supports the following conditional inference: a template root ProgressBar without a local style can inherit its owner style, and its native default-template track and indicator can inherit that style through their subsequent parent links. It does not establish that a particular declared template was accepted, instantiated or drawn.

## Value and indicator layout consumer

When owner+0x378 is nonzero, `0x009B60A0..0x009B6140` reads owner+0x35C, owner+0x34C and owner+0x344. The RangeBase associations identify these as Value, Minimum and Maximum. See [RangeBase Value production](sqwt-range-value-changed.md).

At `0x009B60EC..0x009B6112`, the code computes `(Value-Minimum)/(Maximum-Minimum)`. It compares that result with the Border fraction at Border+0x340, which is helper+0x6B4. On the changed-float path it stores the result at `0x009B612D` and tails the layout invalidation entry `0x0093B250` with the Border receiver. No explicit denominator-zero guard or fraction clamp appears in this checked span.

The same method compares Value with Minimum and requests indicator visibility 0 or 2 through `0x0093C2A0` at `0x009B60E7`. This is a conditional property request. Floating-point predicates do not establish all NaN, exception or precision behavior.

The Border table slot 54 at `0x0107E80C` targets arrange entry `0x009B6180..0x009B62C7`. It combines two four-float inset inputs obtained through virtual offsets 0xC8 and 0xCC, subtracts their horizontal and vertical sums from the supplied final size, and reads Border+0x340 as the fraction. With orientation byte Border+0x344 equal to zero, it multiplies the inner width by the fraction and requests child Width through `0x0092ED80` at `0x009B627A`. Otherwise it requests child Height through `0x0092ED10` at `0x009B6290`, using fraction times inner height. Both calls pass flags 0 and 1. Their receiver is the child pointer at Border+0x33C. The method then requests arrangement of that child through `0x0093A870` at `0x009B62A0`.

The dimension identities are independently anchored by initializers `0x00F21DE0..0x00F21E33`: Height descriptor `0x01358934` names string `0x0106A918`, and Width descriptor `0x01358A54` names string `0x0106A920`.

For an outer ProgressBar whose custom Template is present at initialization and whose template root is an inner ProgressBar with no Template, this path makes the inner ProgressBar own range values the default indicator geometry inputs. It does not make the outer Value an automatic geometry input for the inner control. A binding or animation must supply the inner Value separately.

## Reproduction and limits

Authenticate the PE, map each cited VA through its file-backed section, and compare the constructor, initialization, parent, inheritance and fraction instructions. Recover the RTTI and vtable slots from the original file. Recover both index-22 dispatch slots rather than inferring them from nearby branch layout. Check every receiver adjustment from ProgressBar to helper, helper to Border and Border to indicator.

The native mechanism establishes conditional property inheritance and a range-driven layout consumer. Exact template assignment order, accepted live instances, lookup and storyboard delivery, animation clock, current skin state, clipping, GPU submission and rendered pixels remain unverified. Literal NULL skin handling and the common/default.dds to default.win32.gtex alias are outside this selected path.
