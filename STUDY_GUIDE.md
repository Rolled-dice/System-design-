# Study Guide - How to Get the Most from This Repo

This is a workbook, not a textbook. The structure assumes **active practice over passive reading**.

## Daily Loop (90-180 min)

```
[15 min]  Read day README - theory section
[45 min]  Code-along: type out one .cpp file from scratch (don't copy)
          Run it. Modify it. Break it. Re-run.
[30 min]  Answer 3-5 interview questions from day README aloud or in writing
[30 min]  Daily assignment - implement in your own .cpp file
          (commit it next to mine to compare)
[15 min]  Update weekN-notes.md with what clicked / what didn't
```

If you only have 90 min, drop the daily assignment. If you have 3 hr, add a related external article from `interview-bank/resources.md`.

## Weekly Loop

- **End of each week:** fill in `weekN-notes.md` reflection sections completely
- **Saturday:** rebuild one project from scratch with no reference (e.g., LRU cache, observer chat room) - tests retention
- **Sunday:** rest or pick one external case study from System Design Interview Vol 1 / ByteByteGo

## Active Recall Techniques

1. **Pattern flashcards** - one card per pattern: name + intent + when-to-use + C++ snippet. Use Anki.
2. **Diagram from memory** - close the README, draw the pattern's UML on paper.
3. **Teach to rubber duck** - explain the pattern out loud to an inanimate object. If you stumble, re-read.
4. **Rewrite the example** - don't copy. Type from scratch even if you peek.

## Mock Interview Schedule (Week 4 onwards)

| Day | Type | Source |
|-----|------|--------|
| 22 | LLD self-mock | Parking Lot - record yourself for 45 min |
| 23 | LLD self-mock | Splitwise |
| 24 | LLD with peer | BookMyShow on Pramp/Interviewing.io |
| 25 | LLD self-mock | TicTacToe + Snake & Ladder |
| 26 | HLD self-mock | URL Shortener - draw on paper, then verify |
| 27 | HLD with peer | Twitter |
| 28 | HLD with peer | Uber or WhatsApp |

After Day 28, schedule **2-3 paid mock interviews** with senior engineers. Track scores in `interview-bank/final-checklist.md`.

## Common Pitfalls

- **Reading without coding** - LLD patterns don't stick without finger-memory in C++.
- **Skipping assignments** - the daily assignment is where the actual learning happens.
- **Copy-pasting code** - type it. Yes, every line. Yes, even the boilerplate.
- **Skipping HLD math** - capacity estimation is a separate skill from pattern knowledge. Practice the arithmetic.
- **Avoiding mocks** - you cannot self-assess HLD/LLD without a partner or recorded session.

## What "Done" Looks Like

After 28 days, you should be able to:

- [ ] Implement 23 GoF design patterns in C++ from memory in <15 min each
- [ ] Identify SOLID violations in a code sample within 60 seconds
- [ ] Whiteboard URL Shortener, Twitter, Uber HLD in 45 min from blank paper
- [ ] Code Parking Lot or BookMyShow LLD in 45 min from blank IDE
- [ ] Cite latency numbers (memory, SSD, network, disk seek) without lookup
- [ ] Justify CAP/PACELC choices for a given scenario
- [ ] Tick all boxes in `interview-bank/final-checklist.md`

## When You Get Stuck

1. **Stuck on a pattern?** -> Read [resources.md](interview-bank/resources.md), find the same pattern in `donnemartin/system-design-primer` or `Head First Design Patterns` for an alternate angle.
2. **C++ syntax issue?** -> cppreference.com - bookmark it.
3. **Concurrency confusion?** -> Anthony Williams *C++ Concurrency in Action* chapter 1-3.
4. **HLD trade-off unclear?** -> Find the same problem solved on bytebytego.com or highscalability.com - compare.
5. **Pattern feels artificial?** -> Search for it in real OSS code (e.g., LLVM, Boost, your favorite framework). It's used somewhere.

## Spaced Review Schedule

| When | Review |
|------|--------|
| End of Day N | Day N (active recall, not re-read) |
| End of Day N | Day N-1 (1-day gap) |
| End of week | Days 1-7 of that week (skim notes) |
| Day 28 | Whole repo - skim every README |
| Day 35 | Re-do 2 LLD case studies + 2 HLD case studies blind |
| Before interview | Re-skim final-checklist + resources cheat-sheets |
