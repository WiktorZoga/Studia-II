# Concurrent Programming in Modern C++

Coursework repository for **Programowanie Współbieżne**. It collects C++20 and
C++23 exercises covering threads, the C++ memory model, non-blocking data
structures, and safe memory reclamation.

The repository is currently intended to remain private. Assignment PDFs are
kept as private course materials and must not be made public without checking
their publication rights.

## Requirements

- CMake 3.20 or newer
- A C++ compiler with C++20 and C++23 support
- Ninja (used by the supplied presets)

## Build and test

Configure and build every available exercise:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Build one exercise after configuring the project:

```sh
cmake --build build/debug --target lista13_zad1
```

Sanitizers are optional and intentionally disabled by default:

```sh
cmake --preset asan
cmake --build --preset asan
```

## Repository layout

- `Lista01`–`Lista14` — exercise sheets and their source code.
- `ListaNN/docs/` — Polish learning notes and source material for a sheet.
- `docs/learning-path.md` — topic map and documentation progress.
- `cmake/` — shared CMake helper for consistently named targets.

## Exercise index

| Sheet | Language level | Status | Documentation |
| --- | --- | --- | --- |
| [Lista01](Lista01/docs/README.md) | C++20 | In progress | Template ready |
| [Lista02](Lista02/docs/README.md) | C++20 | In progress | Template ready |
| [Lista03](Lista03/docs/README.md) | C++20 | In progress | Template ready |
| [Lista04](Lista04/docs/README.md) | C++20 | In progress | Template ready |
| [Lista05](Lista05/docs/README.md) | C++23 | In progress | Template ready |
| [Lista06](Lista06/docs/README.md) | C++20 | In progress | Partial notes available |
| [Lista07](Lista07/docs/README.md) | C++20 | In progress | Partial notes available |
| [Lista08](Lista08/docs/README.md) | C++23 | In progress | Partial notes available |
| [Lista09](Lista09/docs/README.md) | C++23 | In progress | Partial notes available |
| [Lista10](Lista10/docs/README.md) | C++23 | In progress | Template ready |
| [Lista11](Lista11/docs/README.md) | C++23 | In progress | Template ready |
| [Lista12](Lista12/docs/README.md) | C++23 | In progress | Partial notes available |
| [Lista13](Lista13/docs/README.md) | C++23 | Notes available | Four task notes available |
| [Lista14](Lista14/docs/README.md) | — | Awaiting implementation | Source material only |

The status describes repository organisation, not a correctness guarantee for
the algorithms. Code review and deeper validation will be performed later,
exercise by exercise.
