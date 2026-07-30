## Widescreen and stability fixes

- Fixes a widescreen progression bug that could make X fall through the floor
  at a ladder transition or walk through the wrong wall during a scripted
  sequence.
- Fixes stack corruption during the Doppler boss fight that could cause severe
  slowdown, accelerated audio, visual corruption, or a softlock when the boss
  attacked.
- Updates the pinned snesrecomp runtime to the latest tested `main` revision.

## Notes

- Windows zip only. Extract the full archive, then run
  `MegaManX3SNESRecomp.exe`.
- A verified Mega Man X3 (USA) ROM is required and is not included.
- The bundled **Mega Man X3 Widescreen** package remains optional and is
  disabled by default.
