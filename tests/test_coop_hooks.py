import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("coop_hooks", ROOT / "tools/apply_coop_hooks.py")
HOOKS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(HOOKS)


class CompiledBoundaries(unittest.TestCase):
    def test_controller_death_dispatch_and_terrain_keep_native_boundaries(self):
        for pc in ("8000", "81D0", "B58A"):
            with self.subTest(pc=pc):
                source = (f"RecompReturn bank_04_{pc}_M1X1(CpuState *cpu) {{\n"
                          "  g_cpu_entry_s[g_recomp_stack_top - 1] = _entry_s;\n}\n")
                patched, count = HOOKS.apply(source)
                self.assertEqual(count, 1)
                self.assertIn(f"0x04{pc.lower()}u", patched)
                self.assertEqual(HOOKS.apply(patched), (patched, 1))

    def test_missing_paired_prologue_fails_generation(self):
        with self.assertRaises(ValueError):
            HOOKS.apply("RecompReturn bank_04_8000_M1X1(CpuState *cpu) {\n}\n")


if __name__ == "__main__":
    unittest.main()
