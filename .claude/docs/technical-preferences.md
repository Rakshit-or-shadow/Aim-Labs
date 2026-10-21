# Technical Preferences

<!-- project.yaml at the repo root is the machine-readable source of truth for
     engine, specialists, naming, performance, platform, and testing.framework.
     This file is the human-readable LEGACY FALLBACK: agents and skills resolve
     each key from project.yaml first and fall back here only when the
     project.yaml key is absent. /setup-engine dual-writes both.
     Forbidden patterns and allowed libraries are NOT migrated — they live only
     in this file. Populated by /setup-engine; updated as decisions are made. -->

## Engine & Language

- **Engine**: Unreal Engine 5.8
- **Language**: C++ (Blueprints for tuning and prototyping)
- **Rendering**: Lumen + Nanite (deferred)
- **Physics**: Chaos (shots use custom ray-sphere math, not physics traces)

## Input & Platform

<!-- Written by /setup-engine. Read by /ux-design, /ux-review, /test-setup, /team-ui, and /dev-story -->
<!-- to scope interaction specs, test helpers, and implementation to the correct input methods. -->

- **Target Platforms**: PC
- **Input Methods**: Keyboard/Mouse
- **Primary Input**: Keyboard/Mouse
- **Gamepad Support**: None
- **Touch Support**: None
- **Platform Notes**: First-person aim trainer; all new C++ types prefixed `GE`.

## Naming Conventions

- **Classes**: Prefixed PascalCase (A/U/F/E); project types add `GE` (e.g. `AGECharacter`)
- **Variables**: PascalCase; booleans `b` prefix
- **Signals/Events**: PascalCase `On<Event>`
- **Files**: Match class without prefix
- **Scenes/Prefabs**: Levels/maps, PascalCase
- **Constants**: SCREAMING_SNAKE (unverified)

## Performance Budgets

- **Target Framerate**: [TO BE CONFIGURED]
- **Frame Budget**: [TO BE CONFIGURED]
- **Draw Calls**: [TO BE CONFIGURED]
- **Memory Ceiling**: [TO BE CONFIGURED]

## Testing

- **Framework**: Unreal Automation Spec
- **Minimum Coverage**: [TO BE CONFIGURED]
- **Required Tests**: Balance formulas, gameplay systems, networking (if applicable)

## Forbidden Patterns

<!-- Add patterns that should never appear in this project's codebase -->
- [None configured yet — add as architectural decisions are made]

## Allowed Libraries / Addons

<!-- Add approved third-party dependencies here -->
- [None configured yet — add as dependencies are approved]

## Architecture Decisions Log

<!-- Quick reference linking to full ADRs in docs/architecture/ -->
- [No ADRs yet — use /architecture-decision to create one]

## Engine Specialists

<!-- Written by /setup-engine when engine is configured. -->
<!-- Read by /code-review, /architecture-decision, /architecture-review, and team skills -->
<!-- to know which specialist to spawn for engine-specific validation. -->

- **Primary**: unreal-specialist
- **Language/Code Specialist**: unreal-specialist (C++); ue-blueprint-specialist (Blueprint graphs)
- **Shader Specialist**: unreal-specialist
- **UI Specialist**: ue-umg-specialist
- **Additional Specialists**: ue-gas-specialist, ue-blueprint-specialist, ue-replication-specialist
- **Routing Notes**: unreal-specialist for C++ architecture; ue-umg-specialist for UMG UI.

### File Extension Routing

<!-- Skills use this table to select the right specialist per file type. -->
<!-- If a row says [TO BE CONFIGURED], fall back to Primary for that file type. -->

| File Extension / Type | Specialist to Spawn |
|-----------------------|---------------------|
| Game code (.cpp, .h) | unreal-specialist |
| Shader / material files | unreal-specialist |
| UI / screen files | ue-umg-specialist |
| Level files (.umap, .uasset) | unreal-specialist |
| Plugin / module files | unreal-specialist |
| General architecture review | Primary |
