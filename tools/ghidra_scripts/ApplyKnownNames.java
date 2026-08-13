// xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
// Copyright (C) 2026  XIVLegacy Dev Team
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Apply a JSON list of {rva, name} entries onto the functions in the current
// program so locally generated neutral vtable-method names can be applied to
// a disposable analysis project without changing the clean export project.
//
// Input JSON is a list of objects with at least `rva` (int, offset from the
// program image base) and `name` (the symbol to apply). `rva_hex` is accepted
// as a fallback. Names containing "::" are applied verbatim (Ghidra keeps them
// as readable flat symbols). Only functions are renamed; an entry with no
// function at its address gets a primary label instead so the name still shows.
//
// Path comes from env var APPLY_NAMES_JSON (one or more, ';'-separated), else
// defaults to config/<program-name>.vtable_method_names.json under
// XIVL_DECOMP_ROOT. Idempotent and re-runnable.
//
//@category XIVLegacy

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolTable;

public class ApplyKnownNames extends GhidraScript {

	// Minimal object scanner: pull "rva", "rva_hex" and "name" out of each
	// top-level {...} object in the JSON array. Avoids a JSON-lib dependency,
	// matching the other repository post-scripts' hand-rolled parsing.
	private static final Pattern OBJ = Pattern.compile("\\{[^{}]*\\}");
	private static final Pattern RVA = Pattern.compile("\"rva\"\\s*:\\s*(\\d+)");
	private static final Pattern RVA_HEX =
		Pattern.compile("\"rva_hex\"\\s*:\\s*\"0x([0-9a-fA-F]+)\"");
	private static final Pattern NAME =
		Pattern.compile("\"name\"\\s*:\\s*\"((?:[^\"\\\\]|\\\\.)*)\"");
	private static String unescapeJson(String value) {
		StringBuilder out = new StringBuilder(value.length());
		for (int i = 0; i < value.length(); i++) {
			char c = value.charAt(i);
			if (c != '\\') {
				out.append(c);
				continue;
			}
			if (++i >= value.length()) {
				throw new IllegalArgumentException("trailing JSON escape");
			}
			char escaped = value.charAt(i);
			switch (escaped) {
			case '"': out.append('"'); break;
			case '\\': out.append('\\'); break;
			case '/': out.append('/'); break;
			case 'b': out.append('\b'); break;
			case 'f': out.append('\f'); break;
			case 'n': out.append('\n'); break;
			case 'r': out.append('\r'); break;
			case 't': out.append('\t'); break;
			case 'u':
				if (i + 4 >= value.length()) {
					throw new IllegalArgumentException("short JSON unicode escape");
				}
				int code = 0;
				for (int j = 1; j <= 4; j++) {
					int digit = Character.digit(value.charAt(i + j), 16);
					if (digit < 0) {
						throw new IllegalArgumentException("invalid JSON unicode escape");
					}
					code = (code << 4) | digit;
				}
				out.append((char) code);
				i += 4;
				break;
			default:
				throw new IllegalArgumentException("invalid JSON escape: \\" + escaped);
			}
		}
		return out.toString();
	}

	@Override
	public void run() throws Exception {
		String root = System.getenv("XIVL_DECOMP_ROOT");
		if (root == null || root.isEmpty()) {
			root = Paths.get("").toAbsolutePath().toString();
		}
		String env = System.getenv("APPLY_NAMES_JSON");
		String[] files;
		if (env != null && !env.isEmpty()) {
			files = env.split(";");
		} else {
			String bin = currentProgram.getName().replaceAll("\\.exe$", "");
			files = new String[] {
				Paths.get(root, "config", bin + ".vtable_method_names.json").toString()
			};
		}

		long imageBase = currentProgram.getImageBase().getOffset();
		SymbolTable symtab = currentProgram.getSymbolTable();
		int applied = 0, labelled = 0, missing = 0, unchanged = 0, failed = 0;

		for (String f : files) {
			Path p = Paths.get(f.trim());
			if (!Files.exists(p)) {
				println("ApplyKnownNames: SKIP (missing) " + p);
				continue;
			}
			String json = new String(Files.readAllBytes(p));
			Matcher om = OBJ.matcher(json);
			while (om.find()) {
				String obj = om.group();
				Matcher nm = NAME.matcher(obj);
				if (!nm.find()) {
					continue;
				}
				String name;
				try {
					name = unescapeJson(nm.group(1));
				} catch (IllegalArgumentException e) {
					failed++;
					continue;
				}

				long rva = -1;
				Matcher rm = RVA.matcher(obj);
				if (rm.find()) {
					rva = Long.parseLong(rm.group(1));
				} else {
					Matcher rh = RVA_HEX.matcher(obj);
					if (rh.find()) {
						rva = Long.parseLong(rh.group(1), 16);
					}
				}
				if (rva < 0) {
					continue;
				}

				Address addr;
				try {
					addr = toAddr(imageBase + rva);
				} catch (Exception e) {
					failed++;
					continue;
				}

				Function fn = getFunctionAt(addr);
				try {
					if (fn != null) {
						if (name.equals(fn.getName())) {
							unchanged++;
						} else {
							Symbol symbol = symtab.getPrimarySymbol(fn.getEntryPoint());
							if (symbol != null && symbol.getSource() != SourceType.DEFAULT) {
								unchanged++;
							} else {
								fn.setName(name, SourceType.USER_DEFINED);
								applied++;
							}
						}
					} else {
						missing++;
						symtab.createLabel(addr, name, SourceType.USER_DEFINED);
						labelled++;
					}
				} catch (Exception e) {
					failed++;
				}
			}
			println("ApplyKnownNames: processed " + p.getFileName());
		}
		println("ApplyKnownNames DONE: renamed=" + applied + " labelled=" + labelled
			+ " already-named=" + unchanged + " no-func=" + missing
			+ " failed=" + failed);
	}
}
