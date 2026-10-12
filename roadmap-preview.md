# Mili 2027 roadmap — proposed GitLab changes (preview only)

> **No GitLab work items have been modified.** This document is a reviewable preview derived from `tasks.csv` and `analysis_mili.txt` on 2026-10-12. Every entry below is a proposal, not an approved commitment.

## Scope and safeguards

- Coverage: all 26 open work items in the export (24 top-level issues and 2 child tasks); the export contained no closed items.
- Preserve without change: titles, descriptions, work-item types, current assignees, authors, due dates, existing labels, existing weights, and the #39 → #60/#61 hierarchy.
- Keep every item open during triage. The four consolidation candidates remain open until maintainers confirm scope and traceability.
- Do not replace existing `Priority::*` labels with the report's numeric scores. The scores are planning recommendations and belong in comments and the roadmap tracker.
- Do not assign `Owner TBD` work automatically. Assignment requires an owner accepting the work.
- Treat inferred dependencies as `relates to` links, not blocking links, unless maintainers confirm a hard dependency.
- Treat proposed milestones as dependency-based planning placeholders that require capacity validation.

## Proposed GitLab planning conventions

The following scoped labels would be created if they do not already exist and then added as shown below:

- `Disposition::Schedule for 2027`
- `Disposition::Discovery/Design in 2027`
- `Disposition::Maintain in Backlog`
- `Disposition::Merge/Deduplicate`
- `Disposition::Needs Triage`
- `Roadmap::Q1 2027`, `Roadmap::Q2 2027`, `Roadmap::Q3 2027`, `Roadmap::Q4 2027`
- `Roadmap::Discovery/Design`, `Roadmap::Deferred`
- `Effort::S`, `Effort::M`, `Effort::L`, `Effort::XL`
- `Planning confidence::High`, `Planning confidence::Medium`, `Planning confidence::Low`

Proposed milestones:

- `2027 Q1`, `2027 Q2`, `2027 Q3`, and `2027 Q4` for scheduled delivery work.
- `2027 Discovery and Design` for discovery/design work and design-stage consolidation candidates.
- No milestone for deferred, backlog-only, or needs-triage items.

Each work item would receive one dated comment beginning with `2027 planning proposal — not yet approved`. The proposed comment content is previewed per item below.

## Complete change matrix

Existing labels are retained; the table lists only labels to add. “None” under milestone means leave the current blank milestone unchanged.

| Item | Proposed labels to add | Proposed milestone | Score / confidence | Effort | Other proposed change |
|---|---|---|---:|---:|---|
| [#2](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/2) | `Disposition::Needs Triage`, `Roadmap::Deferred`, `Effort::L`, `Planning confidence::Low` | None | 48 / Low | L | Add planning comment |
| [#5](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/5) | `Disposition::Merge/Deduplicate`, `Roadmap::Discovery/Design`, `Effort::L`, `Planning confidence::Medium` | `2027 Discovery and Design` | 68 / Medium | L | Relate to #59; add planning comment |
| [#7](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/7) | `Disposition::Maintain in Backlog`, `Roadmap::Deferred`, `Effort::XL`, `Planning confidence::Low` | None | 27 / Low | XL | Add planning comment |
| [#8](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/8) | `Disposition::Merge/Deduplicate`, `Roadmap::Q2 2027`, `Effort::S`, `Planning confidence::Medium` | `2027 Q2` pending consolidation | 51 / Medium | S | Relate to #28; mark duplicate only after confirmation; add planning comment |
| [#9](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/9) | `Disposition::Discovery/Design in 2027`, `Roadmap::Discovery/Design`, `Effort::L`, `Planning confidence::Low` | `2027 Discovery and Design` | 55 / Low | L | Relate to #59; add planning comment |
| [#13](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/13) | `Disposition::Discovery/Design in 2027`, `Roadmap::Discovery/Design`, `Effort::M`, `Planning confidence::Medium` | `2027 Discovery and Design` | 72 / Medium | M | Add planning comment |
| [#15](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/15) | `Disposition::Discovery/Design in 2027`, `Roadmap::Discovery/Design`, `Effort::M`, `Planning confidence::Low` | `2027 Discovery and Design` | 49 / Low | M | Relate to #59; add planning comment |
| [#18](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/18) | `Disposition::Schedule for 2027`, `Roadmap::Q4 2027`, `Effort::M`, `Planning confidence::Medium` | `2027 Q4` | 57 / Medium | M | Relate to #33 and implementation work it documents; add planning comment |
| [#26](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/26) | `Disposition::Schedule for 2027`, `Roadmap::Q3 2027`, `Effort::L`, `Planning confidence::Medium` | `2027 Q3` | 79 / Medium | L | Relate to #27, #41, and #54; add planning comment |
| [#27](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/27) | `Disposition::Schedule for 2027`, `Roadmap::Q3 2027`, `Effort::XL`, `Planning confidence::Medium` | `2027 Q3` | 82 / Medium | XL | Relate to #26, #41, and #54; add planning comment |
| [#28](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/28) | `Disposition::Schedule for 2027`, `Roadmap::Q2 2027`, `Effort::S`, `Planning confidence::High` | `2027 Q2` | 63 / High | S | Relate to #8; proposed consolidation target; add planning comment |
| [#30](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/30) | `Disposition::Needs Triage`, `Roadmap::Deferred`, `Effort::M`, `Planning confidence::Low` | None | 58 / Low | M | Relate to #5 and #59; add planning comment |
| [#32](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/32) | `Disposition::Discovery/Design in 2027`, `Roadmap::Discovery/Design`, `Effort::XL`, `Planning confidence::Medium` | `2027 Discovery and Design` | 69 / Medium | XL | Relate to #54 and #59; add planning comment |
| [#33](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/33) | `Disposition::Schedule for 2027`, `Roadmap::Q4 2027`, `Effort::S`, `Planning confidence::Medium` | `2027 Q4` | 52 / Medium | S | Relate to #34, #39, and #61; add planning comment |
| [#34](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/34) | `Disposition::Merge/Deduplicate`, `Roadmap::Deferred`, `Effort::M`, `Planning confidence::Low` | None | 56 / Low | M | Relate to #39/#61; mark duplicate only after confirmation; add planning comment |
| [#35](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/35) | `Disposition::Schedule for 2027`, `Roadmap::Q1 2027`, `Effort::S`, `Planning confidence::High` | `2027 Q1` | 76 / High | S | Add planning comment |
| [#36](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/36) | `Disposition::Needs Triage`, `Roadmap::Deferred`, `Effort::S`, `Planning confidence::Low` | None | 50 / Low | S | Relate to #39; add planning comment |
| [#38](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/38) | `Disposition::Merge/Deduplicate`, `Roadmap::Discovery/Design`, `Effort::L`, `Planning confidence::Low` | `2027 Discovery and Design` | 60 / Low | L | Relate to #59; mark subsumed only after confirmation; add planning comment |
| [#39](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/39) | `Disposition::Schedule for 2027`, `Roadmap::Q1 2027`, `Effort::M`, `Planning confidence::High` | `2027 Q1` | 84 / High | M | Preserve children #60/#61; relate to #33, #34, and #36; add planning comment |
| [#41](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/41) | `Disposition::Schedule for 2027`, `Roadmap::Q2 2027`, `Effort::M`, `Planning confidence::High` | `2027 Q2` | 88 / High | M | Relate to #26, #27, and #54; add planning comment |
| [#42](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/42) | `Disposition::Discovery/Design in 2027`, `Roadmap::Discovery/Design`, `Effort::M`, `Planning confidence::Medium` | `2027 Discovery and Design` | `64 / Medium` | M | Relate to #59; add planning comment |
| [#54](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/54) | `Disposition::Schedule for 2027`, `Roadmap::Q1 2027`, `Effort::M`, `Planning confidence::High` | `2027 Q1` | 78 / High | M | Relate to #2, #26, #27, #32, #41, #59, and #63; add planning comment |
| [#59](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/59) | `Disposition::Discovery/Design in 2027`, `Roadmap::Discovery/Design`, `Effort::XL`, `Planning confidence::High` | `2027 Discovery and Design` | 86 / High | XL | Relate to #5, #9, #15, #30, #38, and #42; preserve weight 5; add planning comment |
| [#60](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/60) | `Disposition::Schedule for 2027`, `Roadmap::Q1 2027`, `Effort::S`, `Planning confidence::Medium` | `2027 Q1` | 84 inherited / Medium | S | Preserve parent #39; add planning comment |
| [#61](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/61) | `Disposition::Schedule for 2027`, `Roadmap::Q1 2027`, `Effort::M`, `Planning confidence::Medium` | `2027 Q1` | 84 inherited / Medium | M | Preserve parent #39; relate to #33/#34; add planning comment |
| [#63](https://rzlc.llnl.gov/gitlab/mdg/mili/mili/-/work_items/63) | `Disposition::Schedule for 2027`, `Roadmap::Q4 2027`, `Effort::L`, `Planning confidence::High` | `2027 Q4` | 70 / High | L | Relate to #54; add planning comment |

## Proposed relationship changes

These are inferred planning relationships and require maintainer review:

1. **Validation cluster:** relate #33, #34, #36, #39, and #61. Preserve the explicit #39 → #60/#61 hierarchy. If scope review confirms complete overlap, mark #34 as a duplicate of #39 or #61 while retaining cross-links.
2. **Metadata/design cluster:** relate #59 to #5, #9, #15, #30, #38, and #42. Use #59 as the proposed design umbrella, but do not reparent or close those issues until the information model and distinct outcomes are confirmed.
3. **I/O diagnostics:** relate #8 and #28. If maintainers approve a single implementation, make #8 a duplicate of #28 and carry #8's `fflush` acceptance criteria into #28 before closing #8.
4. **Recovery program:** relate #26, #27, #41, and #54. These share safety policy, restart behavior, failure-injection, and independent-reader test needs; none is proposed as a duplicate.
5. **Testing dependencies:** relate #54 to #2, #26, #27, #32, #41, #59, and #63 as a shared risk reducer. These are not proposed as hard blockers without maintainer confirmation.
6. **Documentation sequencing:** relate #18 and #33 to the implementation issues they document. Schedule documentation after validation and compatibility decisions stabilize.

## Proposed per-item planning comments

The following text summarizes the substantive payload to add as a comment to each work item. Acceptance criteria remain proposals until accepted by maintainers.

### #2 — Cleanup Mili_family struct in mili_internal.h

**2027 planning proposal — not yet approved.** Recommended disposition: Needs Triage; deferred; score 48/100 (Low confidence); estimated effort L. Proposed acceptance criteria: inventory every field and use site, classify ownership and lifetime, remove or consolidate only proven redundancies, preserve public behavior, and add family-lifecycle regression coverage. Decision needed: Which redundancies are confirmed, and is `Mili_family` used outside the library despite being in an internal header?

### #5 — MILI-11 Add Labeling ability to all Mili types

**2027 planning proposal — not yet approved.** Recommended disposition: Merge/Deduplicate within the #59 design umbrella; score 68/100 (Medium confidence); estimated effort L. Proposed acceptance criteria: define label semantics and compatibility, implement at least the agreed numeric-label scope, round-trip labels, and validate named downstream consumers. Decision needed: Should #5 become the labeling workstream under #59, and is numeric-only still an acceptable first release?

### #7 — MILI-14 Allow addition of new svars/subrecs/mesh topology more dynamically

**2027 planning proposal — not yet approved.** Recommended disposition: Maintain in Backlog; score 27/100 (Low confidence); estimated effort XL. Proposed acceptance criteria: document concrete use cases, define allowed lifecycle transitions, specify compatibility and persistence behavior, and test each supported mutation. Decision needed: What current workflow is blocked, and which dynamic operation is the minimum useful scope?

### #8 — MILI-8 errno info for output

**2027 planning proposal — not yet approved.** Recommended disposition: consolidate with #28 in Q2; score 51/100 (Medium confidence); estimated effort S. Proposed acceptance criteria: preserve `errno` immediately after `fflush` failure, expose a stable diagnostic, and test representative failure behavior. Decision needed: Should #8 and #28 become one consistent file-I/O diagnostics task? Keep #8 open until its criteria are copied and consolidation is approved.

### #9 — MILI-7 Find a way to replace the one offs that are being used for simulations

**2027 planning proposal — not yet approved.** Recommended disposition: Discovery/Design in 2027; score 55/100 (Low confidence); estimated effort L. Proposed acceptance criteria: inventory active one-offs and consumers, group use cases, define a compatible general mechanism, and select one producer/consumer pilot. Decision needed: Which one-off parameters remain active?

### #13 — Xmilics wait and combining issues in batch

**2027 planning proposal — not yet approved.** Recommended disposition: Discovery/Design in 2027; score 72/100 (Medium confidence); estimated effort M. Proposed acceptance criteria: capture the active batch workflows, define `wait`/`restart` state behavior, verify behavior after a run has finished, and add regression scenarios. Decision needed: Are the smart-batch workflows still active, and what behavior is expected for every state?

### #15 — Provide Mechanism to assign partitions to elements

**2027 planning proposal — not yet approved.** Recommended disposition: Discovery/Design in 2027; score 49/100 (Low confidence); estimated effort M. Proposed acceptance criteria: define partition-identifier semantics, preserve provenance through combine, and provide a supported query path. Decision needed: Is partition provenance still requested, and where should it be stored and queried?

### #18 — Review Mili manual

**2027 planning proposal — not yet approved.** Recommended disposition: Schedule for Q4 2027; score 57/100 (Medium confidence); estimated effort M. Proposed acceptance criteria: inventory the API, compare documented signatures and behavior with implementation, correct discrepancies, and record a repeatable review method. Decision needed: Which manual/version is authoritative, and should generated API references be introduced?

### #26 — Mili Recovery

**2027 planning proposal — not yet approved.** Recommended disposition: Schedule for Q3 2027; score 79/100 (Medium confidence); estimated effort L. Proposed acceptance criteria: specify valid time-file invariants, detect representative damage, rebuild without silently losing valid data, support dry-run/diagnostics, and add failure-injection tests. Decision needed: Which corruption cases are recoverable, and must repair be automatic, opt-in, or dry-run first?

### #27 — State File Inconsistancy Recovery

**2027 planning proposal — not yet approved.** Recommended disposition: Schedule for Q3 2027; score 82/100 (Medium confidence); estimated effort XL. Proposed acceptance criteria: define the last valid state boundary, detect inconsistency, mutate or truncate only under an explicit policy, preserve originals or provide recovery safeguards, and test Griz interaction. Decision needed: Who owns the unequal-state policy, and may recovery mutate original files?

### #28 — Better error reporting when file open fails

**2027 planning proposal — not yet approved.** Recommended disposition: Schedule for Q2 2027 and serve as the proposed #8 consolidation target; score 63/100 (High confidence); estimated effort S. Proposed acceptance criteria: preserve `errno`, expose filename, operation, error number, and message through the approved channel, and test representative failures. Decision needed: Which public error-reporting channel should carry the detail?

### #30 — Mili Label Conflict Detection

**2027 planning proposal — not yet approved.** Recommended disposition: Needs Triage; deferred; score 58/100 (Low confidence); estimated effort M. Proposed acceptance criteria: define conflict scope, report or reject conflicts deterministically, preserve valid incremental topology creation, and add positive/negative tests. Decision needed: What constitutes a conflict, and should it be an error or warning?

### #32 — Increased Time Precision

**2027 planning proposal — not yet approved.** Recommended disposition: Discovery/Design in 2027; score 69/100 (Medium confidence); estimated effort XL. Proposed acceptance criteria: quantify required precision and range, design a versioned representation, define old/new read-write compatibility, inventory consumers, and prototype migration tests. Decision needed: Must libraries read and write both versions bidirectionally?

### #33 — Update Mili Format Doc to specify two svars can't exist in the same srec for the same class/label

**2027 planning proposal — not yet approved.** Recommended disposition: Schedule for Q4 2027; score 52/100 (Medium confidence); estimated effort S. Proposed acceptance criteria: confirm the invariant in code and tests, then add normative wording plus valid and invalid examples to the format documentation. Decision needed: Is this an existing invariant or a desired new restriction?

### #34 — Add Option To Check if an Entity has the same svar defined for it more than once

**2027 planning proposal — not yet approved.** Recommended disposition: Merge/Deduplicate with #39/#61 after confirmation; deferred; score 56/100 (Low confidence); estimated effort M. Proposed acceptance criteria: define duplicate scope, decide default and compatibility behavior, expose the check if still distinct, and provide deterministic diagnostics/tests. Decision needed: Is #34 fully covered by #39/#61, and why must the check be optional?

### #35 — rename verbosity function

**2027 planning proposal — not yet approved.** Recommended disposition: Schedule for Q1 2027; score 76/100 (High confidence); estimated effort S. Proposed acceptance criteria: reproduce the `libyogrt` collision, introduce `mili_verbosity`, inventory consumers, preserve or deliberately deprecate the old symbol, and add linkage coverage. Decision needed: Is the old symbol public ABI, and what deprecation window is required?

### #36 — Enforce monotonically increasing MO_ids in an srec

**2027 planning proposal — not yet approved.** Recommended disposition: Needs Triage; deferred; score 50/100 (Low confidence); estimated effort S. Proposed acceptance criteria: define strictness and scope, validate at the correct boundary, produce a deterministic diagnostic, and test historical-data compatibility. Decision needed: Is monotonicity already a documented invariant, and must readers accept historical violations?

### #38 — Arbitrary Groupings

**2027 planning proposal — not yet approved.** Recommended disposition: Merge/Deduplicate within #59 design after confirmation; score 60/100 (Low confidence); estimated effort L. Proposed acceptance criteria: validate workflows, decide whether grouping belongs in application/TI data or the core format, define representation and compatibility, and demonstrate a downstream use case. Decision needed: Does #59 subsume arbitrary groups?

### #39 — Add additional checks for variable declaration

**2027 planning proposal — not yet approved.** Recommended disposition: Schedule for Q1 2027; score 84/100 (High confidence); estimated effort M. Preserve children #60 and #61. Proposed acceptance criteria: enumerate legal and illegal redeclarations by variable type, implement deterministic checks and diagnostics, define compatibility behavior, and add positive/negative tests. Confirm whether #34 and #36 belong in this validation cluster.

### #41 — TFile failure

**2027 planning proposal — not yet approved.** Recommended disposition: Schedule for Q2 2027; score 88/100 (High confidence); estimated effort M. Proposed acceptance criteria: reproduce the zero-length TFile path, define supported underfull-state behavior, prevent corrupt/empty durable output, and test restart plus error paths. Decision needed: Should the underfull-state error remain, and what durable TFile state must exist afterward?

### #42 — Update classes to provide more information for slide lines

**2027 planning proposal — not yet approved.** Recommended disposition: Discovery/Design in 2027; score 64/100 (Medium confidence); estimated effort M. Proposed acceptance criteria: define class metadata, write/read it compatibly, and demonstrate grouping in the named downstream UI. Decision needed: Can #59 represent slide lines without a dedicated special case?

### #54 — Add testing for reading data with mili and outputting curve files directly

**2027 planning proposal — not yet approved.** Recommended disposition: Schedule for Q1 2027; score 78/100 (High confidence); estimated effort M. Proposed acceptance criteria: identify untested reader APIs, build deterministic fixtures, read and verify values independently of `mili-python`, output curves directly, and integrate regression coverage. Decision needed: Which interfaces and database variants are highest priority?

### #59 — Add parts, surfaces into Mili

**2027 planning proposal — not yet approved.** Recommended disposition: Discovery/Design in 2027; score 86/100 (High confidence); estimated effort XL. Preserve current weight 5. Use the issue's explicit checklist to govern structure definition, analyst/simulation input, post-processing requirements, compatibility, and a minimum viable producer/consumer slice. Decision needed: Who is the cross-tool decision owner, and which path defines the minimum viable slice?

### #60 — Check whether Mili enforces naming for M_NODE and remove it if it does

**2027 planning proposal — not yet approved.** Recommended disposition: Schedule for Q1 2027 under parent #39; inherited score 84/100 (Medium confidence); estimated effort S. Proposed acceptance criteria: locate and document the rule, determine intended compatibility, test current behavior, remove only unintended enforcement, and add regression coverage. Decision needed: Which names must be accepted, and do consumers assume the current name?

### #61 — Check whether the same element and variable are added to different subrecords

**2027 planning proposal — not yet approved.** Recommended disposition: Schedule for Q1 2027 under parent #39; inherited score 84/100 (Medium confidence); estimated effort M. Proposed acceptance criteria: define duplicate scope across subrecords, detect the illegal case during declaration, produce deterministic diagnostics, and add positive/negative compatibility tests. Decision needed: Is every repetition illegal, or only repetition for the same class/label/entity?

### #63 — Fix the mili dump code

**2027 planning proposal — not yet approved.** Recommended disposition: Schedule for Q4 2027; score 70/100 (High confidence); estimated effort L. Proposed acceptance criteria: perform the issue's initial validity check, replace fragile low-level access with supported higher-level access, preserve required output modes, and validate against representative databases using #54 fixtures where useful. Decision needed: Which output modes and formats are compatibility requirements, and what databases form the acceptance suite?

## Conditional consolidation actions

No state change is proposed until the named decision is confirmed:

| Candidate | Confirmation required | Then proposed action |
|---|---|---|
| #8 → #28 | Maintainers agree both diagnostics belong in one implementation and #8 criteria are copied to #28 | Mark #8 as duplicate of #28 and close #8 with a traceability comment |
| #34 → #39/#61 | Duplicate scope and optional/default compatibility semantics are confirmed equivalent | Mark #34 as duplicate of the selected target and close it with a traceability comment |
| #5 → #59 workstream | #59's model is confirmed to cover labeling while preserving a separately trackable deliverable | Keep #5 related or make it a child/workstream; do not close merely because it has an umbrella |
| #38 → #59 workstream | #59 is confirmed to subsume arbitrary grouping requirements | Keep #38 related or make it a child/workstream; close only if all acceptance criteria move to #59 |

## Explicit non-changes

- No GitLab API, CLI, browser, or mutation operation was used.
- No work item is created, edited, assigned, closed, retyped, or reparented by this preview.
- No current label is removed and no current `Priority::*` label is changed.
- No due date, start date, time estimate, time spent, or weight is changed.
- No inferred relationship is represented as a hard `blocks`/`is blocked by` dependency yet.
- No quarterly placement is a staffing commitment; milestones require management approval and capacity validation.

## Reconciliation

- Previewed rows: **26 of 26**.
- Top-level issues: **24**.
- Child tasks: **2** (#60 and #61, both already under #39).
- Open items: **26**; closed items in export: **0**.
- Items proposed for scheduled delivery: **10 top-level issues**, plus **2 child tasks**.
- Items proposed for Discovery/Design: **6 top-level issues**.
- Consolidation candidates: **4 top-level issues**.
- Needs Triage: **3 top-level issues**.
- Maintain in Backlog: **1 top-level issue**.

