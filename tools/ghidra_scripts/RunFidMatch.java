// xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
// Copyright (C)  XIVLegacy Dev Team
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Rerun Ghidra's Function ID analyzer after attaching a FidDb. A plain
// `-process` does not rerun an analyzer that completed during import.
// Temporarily disable other analyzers, run analyzeAll(), then restore the
// original analysis options.
//
// Run headless after AttachFidDatabase.java (which attaches the FidDb):
//   analyzeHeadless ... -preScript AttachFidDatabase.java \
//                       -postScript RunFidMatch.java -postScript DumpFunctions.java
//
//@category XIVLegacy

import java.util.HashMap;
import java.util.Map;

import ghidra.app.script.GhidraScript;
import ghidra.framework.options.OptionType;
import ghidra.framework.options.Options;
import ghidra.program.model.listing.Program;

public class RunFidMatch extends GhidraScript {

	private static final String FID = "Function ID";

	@Override
	public void run() throws Exception {
		Options opts = currentProgram.getOptions(Program.ANALYSIS_PROPERTIES);

		// Save + disable every analyzer enable-toggle except Function ID.
		// The enable-toggle is the bare analyzer name (a top-level BOOLEAN
		// option with no '.'); sub-options look like "Analyzer.SubOption".
		Map<String, Boolean> saved = new HashMap<>();
		int disabled = 0;
		for (String name : opts.getOptionNames()) {
			if (name.indexOf('.') >= 0) {
				continue;
			}
			if (opts.getType(name) != OptionType.BOOLEAN_TYPE) {
				continue;
			}
			boolean cur = opts.getBoolean(name, false);
			saved.put(name, cur);
			if (!name.equals(FID) && cur) {
				opts.setBoolean(name, false);
				disabled++;
			}
		}
		if (opts.contains(FID)) {
			opts.setBoolean(FID, true);
		}

		// Diagnostics: print every Function ID sub-option and its value.
		for (String name : opts.getOptionNames()) {
			if (!name.startsWith(FID + ".")) {
				continue;
			}
			println("RunFidMatch: opt " + name + " = " + opts.getValueAsString(name)
				+ " [" + opts.getType(name) + "]");
		}
		println("RunFidMatch: disabled " + disabled + " analyzers; running Function ID over "
			+ currentProgram.getFunctionManager().getFunctionCount() + " functions");

		analyzeAll(currentProgram);

		// Restore original enable-states so the .gpr keeps its analysis config.
		for (Map.Entry<String, Boolean> e : saved.entrySet()) {
			opts.setBoolean(e.getKey(), e.getValue());
		}
		println("RunFidMatch: Function ID pass complete; restored " + saved.size()
			+ " analyzer options");
	}
}
