// xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
// Copyright (C) 2026  XIVLegacy Dev Team
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Walk MSVC RTTI recovered by Ghidra's Microsoft RTTI analyzer and emit a
// deterministic class/vtable catalog plus a streaming vtable-slot catalog.
//
//@category XIVLegacy

import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.TreeMap;

import ghidra.app.script.GhidraScript;
import ghidra.framework.Application;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Program;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolTable;

public class DumpRtti extends GhidraScript {

    private static class VtableRecord {
        long rva;
        String className;
        String symbolName;
        int slotCount;
        long locatorRva;
        int hierarchyDepth;
        int baseClassCount;
    }

    private static class SlotRecord {
        long vtableRva;
        String className;
        int slot;
        long functionRva;
        String functionName;
    }

    @Override
    public void run() throws Exception {
        Program prog = currentProgram;
        long imageBase = prog.getImageBase().getOffset();
        String sourceSha256 = prog.getExecutableSHA256();
        String ghidraVersion = Application.getApplicationVersion();
        if (sourceSha256 == null || !sourceSha256.matches("[0-9a-fA-F]{64}")) {
            throw new IllegalStateException("program has no usable executable SHA-256");
        }

        String repoRoot = System.getenv("XIVL_DECOMP_ROOT");
        if (repoRoot == null) repoRoot = ".";
        String binary = prog.getName().toLowerCase(Locale.ROOT);
        String stem = binary.replace(".exe", "");
        File configDir = new File(repoRoot, "config");
        configDir.mkdirs();
        File out = new File(configDir, stem + ".rtti.json");
        File slotsFile = new File(configDir, stem + ".vtable_slots.jsonl");

        SymbolTable symbols = prog.getSymbolTable();
        Memory memory = prog.getMemory();

        // Ghidra's global symbol iteration order is not an output contract.
        // Deduplicate aliases by vtable RVA and choose the lexical first name.
        TreeMap<Long, Symbol> vtables = new TreeMap<>();
        Iterator<Symbol> iter = symbols.getAllSymbols(true).iterator();
        while (iter.hasNext() && !monitor.isCancelled()) {
            Symbol symbol = iter.next();
            String name = symbol.getName(true);
            if (!isVtableSymbol(name)) continue;
            long rva = symbol.getAddress().getOffset() - imageBase;
            Symbol prior = vtables.get(rva);
            if (prior == null || name.compareTo(prior.getName(true)) < 0) {
                vtables.put(rva, symbol);
            }
        }

        List<VtableRecord> records = new ArrayList<>();
        List<SlotRecord> slotRecords = new ArrayList<>();
        Set<Long> locatorRvas = new HashSet<>();
        int unknownLocatorCount = 0;
        int unknownDepthCount = 0;
        int minDepth = Integer.MAX_VALUE;
        int maxDepth = Integer.MIN_VALUE;

        for (Symbol symbol : vtables.values()) {
            if (monitor.isCancelled()) break;
            Address address = symbol.getAddress();
            long rva = address.getOffset() - imageBase;
            String name = symbol.getName(true);
            String className = stripVtableSuffix(name);
            HierarchyInfo hierarchy = readHierarchy(prog, memory, imageBase, address);

            int slotIndex = 0;
            try {
                while (slotIndex < 1024) {
                    Address slotAddress = address.add(slotIndex * 4L);
                    long pointer = ((long) memory.getInt(slotAddress)) & 0xffffffffL;
                    if (pointer == 0) break;
                    Address target = prog.getAddressFactory().getDefaultAddressSpace()
                        .getAddress(pointer);
                    MemoryBlock block = memory.getBlock(target);
                    if (block == null || !block.isExecute()) break;
                    SlotRecord slot = new SlotRecord();
                    slot.vtableRva = rva;
                    slot.className = className;
                    slot.slot = slotIndex;
                    slot.functionRva = pointer - imageBase;
                    Function function = prog.getFunctionManager().getFunctionAt(target);
                    Symbol targetSymbol = function == null
                        ? symbols.getPrimarySymbol(target) : function.getSymbol();
                    slot.functionName = targetSymbol == null ? "" : targetSymbol.getName();
                    slotRecords.add(slot);
                    slotIndex++;
                }
            } catch (Exception e) {
                // An unreadable address or non-function pointer terminates the table.
            }

            VtableRecord record = new VtableRecord();
            record.rva = rva;
            record.className = className;
            record.symbolName = name;
            record.slotCount = slotIndex;
            record.locatorRva = hierarchy.locator == null
                ? -1 : hierarchy.locator.getOffset() - imageBase;
            record.hierarchyDepth = hierarchy.depth;
            record.baseClassCount = hierarchy.baseClassCount;
            records.add(record);

            if (record.locatorRva < 0) unknownLocatorCount++;
            else locatorRvas.add(record.locatorRva);
            if (record.hierarchyDepth < 0) unknownDepthCount++;
            else {
                minDepth = Math.min(minDepth, record.hierarchyDepth);
                maxDepth = Math.max(maxDepth, record.hierarchyDepth);
            }
        }

        try (PrintWriter writer = new PrintWriter(Files.newBufferedWriter(
                out.toPath(), StandardCharsets.UTF_8))) {
            writer.print("{\n");
            writer.printf("  \"schema_version\": 1,\n");
            writeMetadata(writer, binary, sourceSha256, ghidraVersion, imageBase, "  ");
            writer.printf("  \"stats\": {\"vtable_records\": %d, \"unique_locator_rvas\": %d, \"vtable_slots\": %d, \"hierarchy_depth_min\": %s, \"hierarchy_depth_max\": %s, \"unknown_locator_count\": %d, \"unknown_hierarchy_depth_count\": %d},\n",
                records.size(), locatorRvas.size(), slotRecords.size(),
                minDepth == Integer.MAX_VALUE ? "null" : Integer.toString(minDepth),
                maxDepth == Integer.MIN_VALUE ? "null" : Integer.toString(maxDepth),
                unknownLocatorCount, unknownDepthCount);
            writer.print("  \"classes\": [\n");
            for (int i = 0; i < records.size(); i++) {
                VtableRecord record = records.get(i);
                writer.printf("    {\"rva\": %d, \"rva_hex\": \"0x%x\", \"class\": %s, \"vtable_symbol\": %s, \"slot_count\": %d, \"locator_rva\": %s, \"locator_rva_hex\": %s, \"hierarchy_depth\": %d, \"base_class_count\": %d}%s\n",
                    record.rva, record.rva, jsonStr(record.className),
                    jsonStr(record.symbolName), record.slotCount,
                    record.locatorRva < 0 ? "null" : Long.toString(record.locatorRva),
                    record.locatorRva < 0 ? "null" : jsonStr(String.format("0x%x", record.locatorRva)),
                    record.hierarchyDepth, record.baseClassCount,
                    i + 1 == records.size() ? "" : ",");
            }
            writer.print("  ]\n}\n");
        }

        try (PrintWriter writer = new PrintWriter(Files.newBufferedWriter(
                slotsFile.toPath(), StandardCharsets.UTF_8))) {
            writer.printf("{\"record_type\": \"metadata\", \"schema_version\": 1, \"binary\": %s, \"retail_version\": \"1.23b\", \"source_sha256\": %s, \"ghidra_version\": %s, \"producer\": \"tools/ghidra_scripts/DumpRtti.java\", \"address_kind\": \"RVA\", \"image_base\": %d, \"image_base_hex\": \"0x%x\", \"observation\": \"Consecutive executable pointers and current target symbols recovered from a clean auto-analyzed Ghidra program.\", \"confidence\": \"direct structural observation\", \"ambiguity\": \"A slot run ends at the first null or non-executable pointer; target names are Ghidra auto-analysis symbols and slot semantics are not inferred.\"}\n",
                jsonStr(binary), jsonStr(sourceSha256.toLowerCase(Locale.ROOT)),
                jsonStr(ghidraVersion), imageBase, imageBase);
            for (SlotRecord slot : slotRecords) {
                writer.printf("{\"record_type\": \"vtable_slot\", \"vtable_rva\": %d, \"vtable_rva_hex\": \"0x%x\", \"class\": %s, \"slot\": %d, \"fn_rva\": %d, \"fn_rva_hex\": \"0x%x\", \"fn_name\": %s}\n",
                    slot.vtableRva, slot.vtableRva, jsonStr(slot.className),
                    slot.slot, slot.functionRva, slot.functionRva,
                    jsonStr(slot.functionName));
            }
        }

        println(String.format("DumpRtti: %s - %d vtable records, %d unique locators, %d total slots -> %s + %s",
            stem, records.size(), locatorRvas.size(), slotRecords.size(), out, slotsFile));
    }

    private static void writeMetadata(PrintWriter writer, String binary,
            String sourceSha256, String ghidraVersion, long imageBase, String indent) {
        writer.printf("%s\"metadata\": {\"binary\": %s, \"retail_version\": \"1.23b\", \"source_sha256\": %s, \"ghidra_version\": %s, \"producer\": \"tools/ghidra_scripts/DumpRtti.java\", \"address_kind\": \"RVA\", \"image_base\": %d, \"image_base_hex\": \"0x%x\", \"observation\": \"Complete object locators, class names, hierarchy metrics, and vtables recovered from the contributor-supplied retail binary by Ghidra's Microsoft RTTI analysis.\", \"confidence\": \"direct structural observation\", \"ambiguity\": \"Class names are Ghidra demangler interpretations; hierarchy metrics are unknown when an RTTI record cannot be decoded.\"},\n",
            indent, jsonStr(binary), jsonStr(sourceSha256.toLowerCase(Locale.ROOT)),
            jsonStr(ghidraVersion), imageBase, imageBase);
    }

    private static boolean isVtableSymbol(String name) {
        if (name == null) return false;
        String lower = name.toLowerCase(Locale.ROOT);
        if (lower.contains("complete_object_locator")) return false;
        return name.endsWith("_vftable")
            || name.endsWith("`vftable'")
            || name.endsWith("::vftable");
    }

    private static String stripVtableSuffix(String name) {
        for (String suffix : new String[] {"_vftable", "::`vftable'", "::vftable"}) {
            if (name.endsWith(suffix)) return name.substring(0, name.length() - suffix.length());
        }
        return name;
    }

    private static class HierarchyInfo {
        Address locator;
        int depth = -1;
        int baseClassCount = -1;
    }

    private static HierarchyInfo readHierarchy(Program prog, Memory mem,
            long imageBase, Address vtable) {
        HierarchyInfo result = new HierarchyInfo();
        try {
            int locatorPtr = mem.getInt(vtable.subtract(4));
            result.locator = resolvePointer(prog, mem, imageBase, locatorPtr);
            if (result.locator == null) return result;

            int chdPtr = mem.getInt(result.locator.add(16));
            Address chd = resolvePointer(prog, mem, imageBase, chdPtr);
            if (chd == null) return result;

            result.baseClassCount = mem.getInt(chd.add(8));
            if (result.baseClassCount <= 0 || result.baseClassCount > 4096) return result;
            int bcaPtr = mem.getInt(chd.add(12));
            Address bca = resolvePointer(prog, mem, imageBase, bcaPtr);
            if (bca == null) return result;
            result.depth = hierarchyDepth(prog, mem, imageBase, bca,
                result.baseClassCount);
        } catch (Exception e) {
            // A malformed or truncated RTTI record has no trustworthy depth.
        }
        return result;
    }

    private static int hierarchyDepth(Program prog, Memory mem, long imageBase,
            Address bca, int count) throws Exception {
        int[] subtreeEnds = new int[count];
        int stackSize = 0;
        int maxDepth = -1;
        for (int i = 0; i < count; i++) {
            while (stackSize > 0 && i >= subtreeEnds[stackSize - 1]) stackSize--;
            maxDepth = Math.max(maxDepth, stackSize);

            int bcdPtr = mem.getInt(bca.add(i * 4L));
            Address bcd = resolvePointer(prog, mem, imageBase, bcdPtr);
            if (bcd == null) return -1;
            int contained = mem.getInt(bcd.add(4));
            if (contained < 0 || contained >= count - i) return -1;
            subtreeEnds[stackSize++] = i + contained + 1;
        }
        return maxDepth;
    }

    private static Address resolvePointer(Program prog, Memory mem,
            long imageBase, int raw) {
        long value = ((long) raw) & 0xffffffffL;
        if (value == 0) return null;
        try {
            Address absolute = prog.getAddressFactory().getDefaultAddressSpace()
                .getAddress(value);
            if (mem.getBlock(absolute) != null) return absolute;
            Address rebased = prog.getAddressFactory().getDefaultAddressSpace()
                .getAddress(imageBase + value);
            if (mem.getBlock(rebased) != null) return rebased;
        } catch (Exception e) {
            // The caller records an unknown hierarchy for an invalid pointer.
        }
        return null;
    }

    private static String jsonStr(String value) {
        StringBuilder out = new StringBuilder("\"");
        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);
            switch (c) {
                case '"': out.append("\\\""); break;
                case '\\': out.append("\\\\"); break;
                case '\n': out.append("\\n"); break;
                case '\r': out.append("\\r"); break;
                case '\t': out.append("\\t"); break;
                default:
                    if (c < 0x20 || c > 0x7e) out.append(String.format("\\u%04x", (int) c));
                    else out.append(c);
            }
        }
        out.append('"');
        return out.toString();
    }
}
