---
name: Whiskey Absolute Rules — AetherSDR
description: Non-negotiable rules for working on the AetherSDR upstream project. Zero mistakes policy.
type: feedback
---
# WHISKEY ABSOLUTE RULES — AetherSDR
## Read this before doing ANYTHING. No exceptions.

---

## RULE 1 — SAVE EVERYTHING IMMEDIATELY

Every instruction, fact, or decision the user gives → written to disk BEFORE responding.
Not at commit time. Not at the end. IMMEDIATELY.

---

## RULE 2 — SESSION START CHECKLIST

1. Read ALL memory files in `.claude/memory/`
2. Read `CLAUDE.md` — architecture, style guide, protocol quirks
3. Read `.claude/RESUME.md` — exact resume point
4. `git status` — commit anything uncommitted before starting
5. `curl -s http://192.168.40.101:11434/api/tags` — check foxtrot is up
6. Check upstream for new commits: `git fetch origin && git log HEAD..origin/main --oneline`

Do not start work until all 6 are done.

---

## RULE 3 — ZERO MISTAKES POLICY

This is a REAL upstream project (ten9876/AetherSDR) with real users.
- **Signed commits required** — CI will reject unsigned commits
- **CI must pass** before any PR is mergeable
- **CODEOWNERS review required** — Jeremy (KK7GWY) is the maintainer
- **Read CONTRIBUTING.md** before making any change
- **Never break slice 0 RX flow** — this is the core radio receive path
- **Never change visual design, UX, or architecture** without maintainer direction
- If unsure → ask Chris, then ask in a GitHub issue. Never guess on a live project.

---

## RULE 4 — GIT WORKFLOW

```bash
# Work locally, commit freely
git commit -S -m "description"

# Ship via alias — squashes, branches, pushes, opens PR
git ship
```

- Always use `git ship` for PRs — never push directly to main
- Branch protection is enforced — don't try to bypass it
- PR to `chrisb1964/AetherSDR` fork first, then upstream PR from there

---

## RULE 5 — LOCAL FIRST

- Use foxtrot (192.168.40.101:11434, qwen3.6:35b) for code analysis
- Dev box fallback (localhost:11434, qwen2.5-coder:14b)
- Never use Claude API for automated tasks — Ollama only
- Every Claude Pro token costs money

---

## RULE 6 — COMPACT ORDER

1. Update `.claude/RESUME.md`
2. `git add .claude/RESUME.md && git commit -S && git push`
3. User types `/compact`

RESUME.md must be saved and pushed BEFORE compacting.

---

## RULE 7 — NEVER TOUCH THE FIREWALL

OPNsense and CRS309: no changes without a helpdesk ticket and Chris's explicit sign-off.

---

## RULE 10 — ALWAYS READ BEFORE EDIT

Always use the Read tool on a file before using the Edit tool on it — even if you just grepped it.
The Edit tool will refuse to write and show an error if Read was not called first in the session.
This is not a real error — it is a safety guard — but it is confusing and off-putting to the user.
Avoid it by reading first, every time, no exceptions.

---

## THE BOTTOM LINE

This is a real GitHub project with real contributors and real users.
Every change must be correct, tested, signed, and CI-passing.
When in doubt — stop and ask.
