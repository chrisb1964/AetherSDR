# RESUME — AetherSDR Project

## Last completed work

- **PR #2250 merged** — Fix TGXL presence when detected via direct TCP only (Fixes #2174)
- **Project memory created** — `.claude/memory/` set up with rules, user profile, project state

## Next work

1. Review `.claude/research/aethersdr-bug-audit.md` — 4 bugs identified, draft fixes ready
2. Pick the easiest bug (TGXL forward power reset) and raise a GitHub issue first
3. Implement fix, test locally, use `git ship` to open PR

## Key reminders

- Always `git fetch origin` at session start to check for upstream commits
- Signed commits required: `git commit -S`
- CI must pass before PR is mergeable
- Read CONTRIBUTING.md before any change
- Jeremy (KK7GWY) is the maintainer — design decisions need his review
