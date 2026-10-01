// SPDX-License-Identifier: AGPL-3.0-or-later
// Read-only instruction decoding for explicit VA:byte-count ranges, including
// callbacks that auto-analysis has not registered as functions.
// @category XIVLegacy

import ghidra.app.script.GhidraScript;
import ghidra.app.util.PseudoDisassembler;
import ghidra.app.util.PseudoInstruction;
import ghidra.program.model.address.Address;

public class DisassembleRanges extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] ranges = getScriptArgs();
        if (ranges.length == 0) {
            throw new IllegalArgumentException("Supply VA:byte-count ranges");
        }
        PseudoDisassembler decoder = new PseudoDisassembler(currentProgram);
        decoder.setRespectExecuteFlag(true);
        for (String range : ranges) {
            String[] parts = range.split(":", -1);
            if (parts.length != 2) {
                throw new IllegalArgumentException("Invalid range: " + range);
            }
            long start = Long.decode(parts[0]);
            int count = Integer.decode(parts[1]);
            if (count <= 0 || count > 4096) {
                throw new IllegalArgumentException("Byte count must be 1..4096");
            }
            Address cursor = toAddr(start);
            Address end = cursor.addNoWrap(count);
            println("===== range " + range + " =====");
            while (cursor.compareTo(end) < 0) {
                monitor.checkCancelled();
                PseudoInstruction instruction = decoder.disassemble(cursor);
                Address next = cursor.addNoWrap(instruction.getLength());
                if (next.compareTo(end) > 0) {
                    println("END: instruction crosses requested boundary at " + cursor);
                    break;
                }
                StringBuilder bytes = new StringBuilder();
                for (byte value : instruction.getBytes()) {
                    bytes.append(String.format("%02x ", value & 0xff));
                }
                println(cursor + "  " + bytes + " " + instruction);
                cursor = next;
            }
        }
    }
}
