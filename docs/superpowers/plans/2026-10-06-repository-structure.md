# Repository Structure Implementation Plan

> **For agentic workers:** Execute this plan task by task. Keep the existing course folders and all files under `Modele-Jezykowe/P1` at their current paths.

**Goal:** Make `Studia-II` a clear study hub for its active courses and prepare a respectful, cost-aware AI collaboration workflow.

**Architecture:** Keep every existing course path stable. Improve the root index and add concise course indexes, study folders, and root-level `AGENTS.md` rules; make no changes to exercise content.

**Tech Stack:** Markdown documentation and Git ignore rules.

**Spec:** `docs/superpowers/specs/2026-10-06-repository-structure-design.md`

## Global Constraints

- Preserve every current course directory name and path.
- Preserve every file in `Modele-Jezykowe/P1` without editing it.
- Do not move, delete, or rewrite study material.
- Do not solve or fill in the student's assignment answers.
- Prefer the lightest model and single-agent work that meet the task needs.

## Review Focus

- Existing `P1` files must remain untouched and reachable at their current paths.
- README links must resolve relative to the repository root.
- The AI collaboration rules must guide learning without taking over the student's work.
- The ISO ignore rule must not remove an already-present image.

---

### Task 1: Establish repository guide and collaboration rules

**Files:**
- Modify: `README.md`
- Create: `AGENTS.md`

**Interfaces:**
- Consumes: The existing top-level course directories and the approved design.
- Produces: A root course index and standing instructions for future AI work in this repository.

- [x] Rewrite the root README in Polish with links to `Systemy-Operacyjne/` and `Modele-Jezykowe/`, and list other existing tracked course directories without calling them legacy.
- [x] Document stable-path and naming conventions for courses, lists (`ListaNN` or existing course convention such as `P1`), notes, and projects.
- [x] Add root `AGENTS.md` rules: inspect before editing; preserve user files; ask before broad moves/deletions; explain before changing paths; for study assignments, begin with understanding and hints, review the student's attempt, and do not complete a whole list or write final answers unless explicitly asked.
- [x] Add the model and agent policy: Luna/low for bounded routine work, Sol/low-to-medium when judgment or multi-step implementation is needed, Astra/Max/Ultra only for unusually difficult work, and parallel agents only for independent subtasks.
- [x] Review the rendered Markdown structure and relative links by inspection.

### Task 2: Document active courses and prepare study spaces

**Files:**
- Create: `Systemy-Operacyjne/README.md`
- Create: `Modele-Jezykowe/README.md`
- Create: `Modele-Jezykowe/P1/README.md`
- Create: `Modele-Jezykowe/notatki/README.md`
- Create: `Modele-Jezykowe/projekty/README.md`

**Interfaces:**
- Consumes: Root course-link conventions from Task 1; current `Systemy-Operacyjne/Lista00` and user-created `Modele-Jezykowe/P1` contents.
- Produces: Course entry points, guidance for existing P1 files, and stable homes for general notes and future projects.

- [x] Add a Systemy Operacyjne index linking its current `Lista00`; describe keeping solutions, source material, and code together under each list directory and adding future `ListaNN` directories in the same pattern.
- [x] Add a Modele Językowe index linking P1 and the notes/projects locations; use `P1`, `P2`, etc. for future assignments.
- [x] Describe the existing P1 files by filename and purpose only when clear from their names; do not edit or complete any of them.
- [x] Explain a collaborative study loop: the student first shares the task and their current understanding/attempt; the assistant clarifies concepts, offers one manageable hint at a time, and reviews the student's reasoning.
- [x] Keep `notatki/` for course-wide concepts and `projekty/` for work larger than one assignment; assignment-specific material remains in its `P#` directory.
- [x] Check every new relative link by resolving its target from the containing README.

### Task 3: Ignore local installation images and review the change

**Files:**
- Modify: `.gitignore`

**Interfaces:**
- Consumes: Existing repository ignore rules and local `Systemy-Operacyjne/debian-13.7.0-amd64-netinst.iso` presence in the main checkout.
- Produces: An ignore rule for local ISO images and a reviewed, path-preserving documentation change.

- [x] Add `*.iso` under a clearly labeled local installation-media comment; do not remove or alter any ISO file.
- [x] Inspect `git status` in the main checkout to confirm the user-added `Modele-Jezykowe/P1` files, ISO, nested repository state, and `Programowanie-Wspolbiezne-Cpp` remain untouched.
- [x] Inspect the branch diff for accidental renames/deletions and verify Markdown links and spelling.
