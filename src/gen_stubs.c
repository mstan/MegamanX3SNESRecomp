/* Handwritten bodies for functions the recompiler could not synthesise.
 *
 * Empty at scaffold time. Mega Man X3 boots LLE-first: absent AOT coverage runs
 * the real ROM bytes on the interpreter tier, so nothing needs stubbing to get
 * a picture. Add entries here only when a regen reports a genuine gap and the
 * root cause is in the ROM's structure, not in the recompiler.
 *
 * RAM-routine guards live in src/gen/dispatch_v2.c once a regen emits
 * ram_routines[].
 */
