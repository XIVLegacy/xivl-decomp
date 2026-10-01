// SPDX-License-Identifier: AGPL-3.0-or-later
// Read-only, bounded reads of printable ASCII strings at explicit VAs.
// @category XIVLegacy

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;

public class ReadAsciiStrings extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] requests = getScriptArgs();
        if (requests.length == 0) {
            throw new IllegalArgumentException("Supply VA:max-bytes requests");
        }
        for (String request : requests) {
            String[] parts = request.split(":", -1);
            if (parts.length != 2) {
                throw new IllegalArgumentException("Invalid request: " + request);
            }
            long va = Long.decode(parts[0]);
            int limit = Integer.decode(parts[1]);
            if (limit < 1 || limit > 256) {
                throw new IllegalArgumentException("Byte limit must be 1..256");
            }
            Address start = toAddr(va);
            StringBuilder value = new StringBuilder();
            boolean terminated = false;
            for (int offset = 0; offset < limit; offset++) {
                monitor.checkCancelled();
                int next = currentProgram.getMemory().getByte(start.addNoWrap(offset)) & 0xff;
                if (next == 0) {
                    terminated = true;
                    break;
                }
                if (next < 0x20 || next > 0x7e) {
                    throw new IllegalArgumentException("Non-printable ASCII at " + request);
                }
                value.append((char) next);
            }
            if (!terminated) {
                throw new IllegalArgumentException("No terminator within " + request);
            }
            String quoted = value.toString().replace("\\", "\\\\").replace("\"", "\\\"");
            println(start + " = \"" + quoted + "\"");
        }
    }
}
