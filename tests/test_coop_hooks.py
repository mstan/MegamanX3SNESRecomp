import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("coop_hooks", ROOT / "tools/apply_coop_hooks.py")
HOOKS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(HOOKS)


class CompiledBoundaries(unittest.TestCase):
    def test_native_paired_boundaries_are_observable_in_every_emitted_width(self):
        for bank, targets in HOOKS.TARGETS.items():
            for pc in targets:
                for width in ("M0X0", "M0X1", "M1X0", "M1X1"):
                    with self.subTest(bank=bank, pc=pc, width=width):
                        source = (f"RecompReturn bank_{bank:02X}_{pc:04X}_{width}(CpuState *cpu) {{\n"
                                  "  g_cpu_entry_s[g_recomp_stack_top - 1] = _entry_s;\n}\n")
                        patched, count = HOOKS.apply(source)
                        self.assertEqual(count, 1)
                        self.assertIn(f"0x{bank:02x}{pc:04x}u", patched)
                        self.assertEqual(HOOKS.apply(patched), (patched, 1))

    def test_missing_paired_prologue_fails_generation(self):
        with self.assertRaises(ValueError):
            HOOKS.apply("RecompReturn bank_04_8000_M1X1(CpuState *cpu) {\n}\n")

    def test_retired_boundary_has_no_stale_bridge_or_include(self):
        original = "RecompReturn bank_03_B2A7_M1X1(CpuState *cpu) {\n  g_cpu_entry_s[g_recomp_stack_top - 1] = _entry_s;\n}\n"
        stale = HOOKS.MARKER + ' #include "snes/interp_bridge.h"\n' + original
        self.assertEqual(HOOKS.apply(stale), (original, 0))


if __name__ == "__main__":
    unittest.main()
