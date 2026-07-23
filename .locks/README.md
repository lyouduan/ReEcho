# Local locks

Lock files in this directory are machine-local coordination signals and are ignored by git. `unreal-editor.lock` represents exclusive ownership of the ReEcho editor instance, not ownership of UE binary assets; asset ownership is recorded in `shared/PLANNER_EXCHANGE.md`.
