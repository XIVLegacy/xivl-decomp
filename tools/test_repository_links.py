"""Public links must not depend on a maintainer's ignored exports."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import validate_repo


class RepositoryLinkTests(unittest.TestCase):
    def test_existing_ignored_export_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            (root / "asm").mkdir()
            (root / "asm" / "function.s").write_text("local export", encoding="utf-8")
            (root / "README.md").write_text(
                "[assembly](asm/function.s)", encoding="utf-8"
            )
            with patch.object(validate_repo, "ROOT", root):
                errors = validate_repo.markdown_link_errors(["README.md"])
            self.assertEqual(
                errors, ["untracked relative link: README.md:1 -> asm/function.s"]
            )

    def test_tracked_files_and_directories_are_resolvable(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            (root / "docs").mkdir()
            (root / "docs" / "finding.md").write_text("Observation", encoding="utf-8")
            (root / "README.md").write_text(
                "[finding](docs/finding.md) [docs](docs/) [root](.)", encoding="utf-8"
            )
            with patch.object(validate_repo, "ROOT", root):
                self.assertEqual(
                    validate_repo.markdown_link_errors(
                        ["README.md", "docs/finding.md"]
                    ),
                    [],
                )

    def test_hash_checked_generated_catalog_is_resolvable(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            relative = "config/ffxivgame.vtable_method_names.json"
            (root / "config").mkdir()
            (root / relative).write_text("{}", encoding="utf-8")
            (root / "README.md").write_text(f"[catalog]({relative})", encoding="utf-8")
            with patch.object(validate_repo, "ROOT", root):
                self.assertEqual(validate_repo.markdown_link_errors(["README.md"]), [])


if __name__ == "__main__":
    unittest.main()
