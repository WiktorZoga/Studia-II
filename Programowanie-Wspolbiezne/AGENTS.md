# Repository Guidelines

## Structure

`ListaNN/` holds one exercise sheet. C++ sources remain beside its local
`CMakeLists.txt`; explanations and private course material belong in
`ListaNN/docs/`. Use `docs/zadN.md` for one task and `docs/materialy/` for
assignment files. The root `docs/learning-path.md` tracks documentation work.

## Build and test

Use the shared CMake project from this directory:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Exercise targets use `listaNN_zadN` names, for example:

```sh
cmake --build build/debug --target lista13_zad1
```

The project preserves each source file's C++20 or C++23 requirement. Use the
`asan` preset only when investigating runtime problems; normal builds keep
sanitizers disabled.

## Code and documentation

Keep one exercise per `zadN.cpp` (or existing `progN.cpp`) and use four-space
indentation with descriptive `snake_case` names. Do not alter a concurrent
algorithm while doing structural or documentation work. For new task notes,
use these sections: task statement, goal, design, code walkthrough, concurrency
and memory, run/tests, and limitations. Do not infer missing task statements
from code.

## Commits and publication

Use concise messages scoped to a sheet, e.g. `Lista13: document EBR task`.
Before opening a pull request, build affected targets and state the commands
used. Keep PDFs and materials from teaching staff private until their
publication rights are confirmed.
