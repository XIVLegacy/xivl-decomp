// xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
// Copyright (C) 2026  XIVLegacy Dev Team
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// For each VA in CALLER_VAS (comma-separated), print every reference TO it and
// the function that contains each reference - i.e. "who calls / points at this
// function". Distinguishes direct call sites from data references (vtable slots
// are data refs from a vftable, so this also surfaces which vtable a function
// sits in).
//
//   CALLER_VAS="0x6e32f0,..."  analyzeHeadless ... -process <bin> -noanalysis \
//       -postScript FindCallers.java
//
// Set XIVL_RETAIL_OBSERVATIONS_OUT for the bounded retail-evidence mode. That
// mode requires exactly one target, emits only sorted unique direct-caller
// function entry VAs, and does not use the text mode's display truncation.
//
//@category XIVLegacy

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.AtomicMoveNotSupportedException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.nio.file.StandardOpenOption;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.TreeSet;

public class FindCallers extends GhidraScript {
	@Override
	public void run() throws Exception {
		String vas = System.getenv("CALLER_VAS");
		if (vas == null || vas.isEmpty()) {
			throw new IllegalArgumentException("FindCallers: set CALLER_VAS=0x..,0x..");
		}
		List<String> targets = parseTargets(vas);
		String structuredOutput = System.getenv("XIVL_RETAIL_OBSERVATIONS_OUT");
		if (structuredOutput != null && !structuredOutput.trim().isEmpty()) {
			if (targets.size() != 1) {
				throw new IllegalArgumentException(
					"structured FindCallers mode requires exactly one target");
			}
			writeStructuredObservation(targets.get(0), structuredOutput.trim());
			return;
		}

		for (String tok : targets) {
			long va = Long.parseLong(tok.replaceFirst("^0x", ""), 16);
			Address target = toAddr(va);
			println("===== refs to " + tok + " =====");
			ReferenceIterator it = currentProgram.getReferenceManager().getReferencesTo(target);
			int n = 0;
			while (it.hasNext()) {
				monitor.checkCancelled();
				Reference r = it.next();
				Address from = r.getFromAddress();
				Function host = getFunctionContaining(from);
				String hostName = (host == null) ? "(not in a function - data/vtable)"
					: host.getName() + " @" + host.getEntryPoint();
				println("  " + r.getReferenceType() + " from " + from + "  in " + hostName);
				if (++n > 60) { println("  ... (truncated)"); break; }
			}
			if (n == 0) println("  (no references found)");
		}
	}

	private List<String> parseTargets(String source) {
		List<String> targets = new ArrayList<>();
		for (String raw : source.split(",", -1)) {
			String token = raw.trim().toLowerCase(Locale.ROOT);
			if (!token.matches("0x[0-9a-f]{1,8}")) {
				throw new IllegalArgumentException("invalid CALLER_VAS entry");
			}
			targets.add(String.format("0x%08x",
				Long.parseLong(token.substring(2), 16)));
		}
		if (targets.isEmpty()) {
			throw new IllegalArgumentException("CALLER_VAS is empty");
		}
		return targets;
	}

	private void writeStructuredObservation(String token, String outputSource)
			throws Exception {
		long va = Long.parseLong(token.substring(2), 16);
		Address target = toAddr(va);
		if (!currentProgram.getMemory().contains(target)) {
			throw new IllegalArgumentException("target is outside the program");
		}

		TreeSet<Long> callerEntries = new TreeSet<>();
		ReferenceIterator references =
			currentProgram.getReferenceManager().getReferencesTo(target);
		while (references.hasNext()) {
			monitor.checkCancelled();
			Reference reference = references.next();
			if (!reference.getReferenceType().isCall()) {
				continue;
			}
			Function caller = getFunctionContaining(reference.getFromAddress());
			if (caller == null) {
				throw new IllegalStateException(
					"direct call reference has no containing function");
			}
			callerEntries.add(caller.getEntryPoint().getOffset());
		}

		StringBuilder json = new StringBuilder();
		json.append("{\"check_id\":\"protocol-0x0135-single-direct-caller-v1\",");
		json.append("\"direct_caller_entry_vas\":[");
		boolean first = true;
		for (long callerEntry : callerEntries) {
			if (!first) json.append(',');
			json.append(String.format("\"0x%08x\"", callerEntry));
			first = false;
		}
		json.append("],\"input_id\":\"ffxivgame-1.23b\",");
		json.append("\"schema_version\":1,");
		json.append(String.format("\"target_va\":\"0x%08x\"}", va));

		Path output = Paths.get(outputSource).toAbsolutePath().normalize();
		Path parent = output.getParent();
		if (parent == null || !Files.isDirectory(parent)) {
			throw new IOException("structured output parent does not exist");
		}
		if (Files.exists(output)) {
			throw new IOException("structured output already exists");
		}
		Path temporary = Files.createTempFile(parent,
			output.getFileName().toString() + ".", ".tmp");
		try {
			Files.write(temporary, (json.toString() + "\n").getBytes(StandardCharsets.US_ASCII),
				StandardOpenOption.TRUNCATE_EXISTING);
			try {
				Files.move(temporary, output, StandardCopyOption.ATOMIC_MOVE);
			}
			catch (AtomicMoveNotSupportedException exception) {
				Files.move(temporary, output);
			}
		}
		finally {
			Files.deleteIfExists(temporary);
		}
		println("FindCallers: wrote bounded structured observation");
	}
}
