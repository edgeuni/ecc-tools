# Design: Commercial CTS Phase 4 Alignment

## Strategy

Phase 1 locked the timing/RC model. Phase 2 proved the final gap is structural. Phase 3 removed a hard H-tree depth window and added shape-aware global selection, but most metrics did not move. Phase 4 focused on algorithm behavior over all 23 cases:

```text
23-case Innovus/ECC distribution analysis
  -> first-principles algorithm hypothesis
  -> production iCTS patch
  -> 23-case CTS-only bench
  -> 23-case Innovus detail-route evaluation
  -> final/structural metric report
  -> keep/refine/revert/split
```

Timing remains a guard. Phase 4 does not retune RC, Liberty delay, or arrival trace calibration.

## Inputs

Phase 4 reads:

- Phase 1 ECC detail-run summary and reports under this task
- Phase 3 candidate summaries and reports under this task
- Innovus commercial strict summary and reports under `/home/liweiguo/project/DAC-27-CTS`
- all 23 DAC synthesized CTS cases

DAC-27-CTS remains read-only. All scripts, summaries, and reports are written under this task.

## First-Principles Model

Phase 4 algorithm decisions should be derived from:

- sink geometry, sink count, and load-cap distribution
- sink clustering compactness versus balance
- H-tree depth candidate legality and selected depth
- local sink-load-region fanout/split feasibility
- trunk/leaf wire length, cap, slew, and power distributions
- source-trunk selected buffering and source-to-root delay
- library drive distribution and transition/cap constraints
- final Innovus detail-route skew/latency/DRV metrics

If a change needs a boundary, it must be derived from an existing design/library constraint such as max fanout, max cap, sink cap distribution, or route RC. Do not introduce fixed global margins or per-case constants.

## Analysis Surfaces

Phase 4 extends the existing Phase 2/3 report parsing:

- `clock_trees.rpt`
  - depth, trunk/leaf/top length and cap
  - buffer count/area/cell histogram
  - transition and fanout summaries
- `timing_paths.rpt`
  - clock stage count and clock-buffer sequence
- CTS logs
  - H-tree selected policy, selected depth, depth candidate count
  - monotone/sink-load-region failure signatures
  - source-trunk selected buffer count and candidate policy
- summary CSV
  - latency/skew/power/cap/DRV
- Phase 1 L3 timing summary
  - row-delay guard status

## Iteration Records

Each of the required 10 Phase 4 iterations records:

- hypothesis
- all-23 evidence
- code/action
- validation command or reason for rejecting before code
- final/structural impact if evaluated
- decision: keep, refine, revert, or split

Rejected hypotheses count only when the all-23 evidence is strong enough to show the change would be unsound before implementation.

## Candidate Repair Families

### Sink Clustering

Investigate whether current sink clustering spreads sinks to satisfy internal balance objectives but diverges from Innovus compact/local distributions. Candidate actions include disabling, reducing, or replacing the micro-adjustment behavior only when all-23 evidence supports tighter clustering and DRV remains controlled.

### Sink-Load Region Legality

Investigate failures where shallower H-tree candidates are blocked by local load groups such as `fanout_violation load_count > max_fanout` with infeasible one-stage split remediation. Candidate actions should improve local split/local buffering topology rather than raising max fanout or using case constants.

### H-Tree Candidate Scoring

Extend scoring only from candidate facts: depth, buffered levels, delay, power, cap/load distribution, and legality. Do not reintroduce delay margins or period-fraction targets.

### Source-Trunk And Segment Selection

Compare source-trunk selected buffer counts, trunk cap/slew, and route lengths against Innovus. Candidate actions should adjust segment scoring or source-trunk construction from route/electrical facts, not from fixed buffer-count targets.

## Validation Loop

Use this loop for promoted candidates:

1. build `ecc_bin` and targeted iCTS tests
2. run all-23 DAC CTS-only bench
3. run all-23 Innovus detail-route evaluation
4. regenerate Phase 4 scoreboards
5. compare against Phase 1, Phase 3, and Innovus
6. update iteration record

During the normal loop do not run broad `ecc dev`; reserve it for finish-work.

## Phase 4 Outcome

Phase 4 promoted Iter09 as the final candidate:

- k-ary H-tree construction derived from max fanout and 2D quadrant topology
- correct k-ary target-depth tree height
- branching-factor-aware fanout legality and weighted buffer accounting
- local split cost visible to global candidate selection
- physical candidate ordering by actual buffer count before physical depth
- boundary polish kept, because the no-boundary control worsened structure

The final evidence lives at:

- `summary/phase4_iterations/phase4_iteration_log.md`
- `summary/phase4_final/phase4_final_report.md`
- `summary/phase4_iter09_kary_buffer_first_score/phase4_candidate_score_report.md`

Phase 4 closes a meaningful part of latency/topology/buffer alignment. It intentionally leaves skew spread and leaf/load balance for the next stage.
