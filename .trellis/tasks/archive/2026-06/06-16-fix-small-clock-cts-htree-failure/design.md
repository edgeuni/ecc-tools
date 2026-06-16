# Design: 修复小规模时钟 CTS HTree 失败

## Boundary

The change targets the ECC/iCTS CTS topology build path that handles downstream HTree construction. The benchmark assets and generated 2026-06-16 benchmark outputs are inputs for diagnosis only and must remain read-only.

## Data Flow

1. Read LEF/Liberty/DEF/Verilog/SDC through the existing iCTS bench Tcl and config path.
2. Build CTS clock metadata, including sink count and clock net.
3. Build topology, including downstream HTree.
4. Emit CTS DEF/Verilog into a project-local output directory during validation.

## Expected Failure Shape

The two reported cases have very small sink counts, 5 and 6. A likely defect class is an HTree partitioning or branch construction assumption that requires an even split, enough sinks per quadrant, non-empty child branches, or a valid geometric span. The implementation should gracefully handle small and uneven sink populations.

## Compatibility

- Existing user configuration keys remain unchanged.
- The benchmark Tcl and generated config shape remain unchanged.
- The fix should prefer local CTS fallback logic over changing external flow scripts unless investigation proves config generation is responsible.
- If HTree is unsuitable for a small sink population, the code should select an existing simpler topology path or build a legal degenerate HTree rather than aborting.

## Validation Strategy

- Use project-local copies of the provided DB/flow config with output directories redirected inside this repository.
- Run the local `ecc_bin` from this repository when possible; otherwise use the provided benchmark binary only as a read-only baseline comparator.
- Validate both `ascon` and `s1488`.
- Run focused unit tests or add one near the topology implementation when a stable internal seam exists.

## Rollback

The change should be small enough to revert as one code patch. If validation reveals broader CTS quality regression risk, keep the fallback guarded by the small or degenerate sink-tree condition discovered during debugging.
