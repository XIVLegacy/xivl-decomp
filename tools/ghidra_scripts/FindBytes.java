// xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
// Copyright (C) 2026  XIVLegacy Dev Team
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Find an exact byte sequence in the current program without modifying it.
//
//   SEARCH_BYTES="20 83 b8 ed" analyzeHeadless ... -readOnly -noanalysis \
//       -postScript FindBytes.java
//
//@category XIVLegacy

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.Memory;

public class FindBytes extends GhidraScript {
    @Override
    public void run() throws Exception {
        String source = System.getenv("SEARCH_BYTES");
        if (source == null || source.trim().isEmpty()) {
            println("FindBytes: set SEARCH_BYTES to space-separated hex bytes");
            return;
        }

        String[] tokens = source.trim().split("\\s+");
        byte[] pattern = new byte[tokens.length];
        for (int i = 0; i < tokens.length; i++) {
            pattern[i] = (byte)Integer.parseInt(tokens[i], 16);
        }

        Memory memory = currentProgram.getMemory();
        Address cursor = memory.getMinAddress();
        int matches = 0;
        while (cursor != null && cursor.compareTo(memory.getMaxAddress()) <= 0) {
            Address found = memory.findBytes(cursor, pattern, null, true, monitor);
            if (found == null) {
                break;
            }
            Function function = getFunctionContaining(found);
            String owner = function == null
                ? "data"
                : function.getName() + " @" + function.getEntryPoint();
            println("match " + found + " in " + owner);
            matches++;
            cursor = found.next();
        }
        println("FindBytes: " + matches + " match(es) for " + source.trim());
    }
}
