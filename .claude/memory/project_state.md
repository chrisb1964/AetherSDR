---
name: AetherSDR Project State
description: Current contribution status, last merged PR, open work
type: project
---
## Contribution Status

- **Fork:** chrisb1964/AetherSDR (remote: `fork`)
- **Upstream:** ten9876/AetherSDR (remote: `origin`)
- **Current version:** 0.9.4

## Last Merged PR

- **PR #2250** — "Fix TGXL presence when detected via direct TCP only (Fixes #2174)"
- **Commit:** 8c856e3
- **Status:** Merged to upstream main ✅

## Known Bugs in AetherSDR (from bug audit)

See `.claude/research/aethersdr-bug-audit.md` for full details.

1. **TGXL forward power not resetting after TX ends** — `pttA` transition 1→0 doesn't zero `m_forwardPower`. Draft fix in research file.
2. **PGXL power level not resetting after TX ends** — `setState()` doesn't zero `m_outputPower` on TRANSMIT→non-TRANSMIT. Draft fix in research file.
3. **PGXL fan mode buttons** — commands exist (FAN:NORMAL/CONTEST/BROADCAST on port 9008), UI bindings partially implemented.
4. **EFF/meffa label** — clarity issue in TunerControl display.

## Contribution Opportunities

See `.claude/research/aethersdr-contribution-opportunities.md` — 12 candidate issues mapped, 9 easy/medium complexity.

## CI/CD

- Docker image: `ghcr.io/ten9876/aethersdr-ci:latest` (~3.5 min builds)
- Signed commits required on main
- `git ship` alias for PR workflow
- CODEOWNERS review required (Jeremy/KK7GWY)
