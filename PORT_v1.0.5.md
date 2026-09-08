# NInfer — port Windows v1.0.5

Este árbol es el **port Windows de `Neroued/ninfer` actualizado a upstream `master`
(2026-09-03)**: equivale al port v1.0.4 (sincronizado en `5973313d`, 2026-09-02)
**más los 4 commits** que faltaban:

| Commit | Cambio |
|---|---|
| `550d0ac3` | feat(serve): timing + progreso de llama.cpp (prompt progress) |
| `6e2786c5` | fix(logging): logs operativos legibles (rework + `pretty_format`) |
| `719d56ef` | fix(frontend): preservar intención de tool-call estructurado |
| `e3aeaf8c` | fix(serve): preservar *anthropic thinking* tras reinicios |

## Qué se conservó (adaptaciones MSVC/Windows)
- `unsigned __int128` → `std::uint64_t` (MSVC no soporta `__int128`)
- Headers POSIX → Windows con `#ifdef _WIN32` (`process.h`, `<random>`, `intrin.h`)
- `src/CMakeLists.txt`: FFmpeg/LibCurl condicionales
- `nvfp4_w4a4_tma.cuh`: `alignas(64)`
- READMEs localizados (raíz / `tests` / `tools/bench`) conservados tal cual v1.0.4

## Nota sobre `e3aeaf8c`
Borra `src/serve/anthropic_thinking_signature.{cpp,h}`; la lógica de firma migró a
`src/serve/anthropic_messages_response.cpp` usando `std::random_device` (seguro en MSVC).

## Estado de alineación (verificado)
- 48 archivos del delta = **idénticos a upstream master** (sin desfase).
- 13 archivos de código difieren de master **solo** por la adaptación Windows.
- 3 archivos nuevos + 2 eliminados según upstream.

## Compilar
Igual que v1.0.4: `build_windows.bat` (texto) o `build_vision_windows.bat` (visión).
No requiere cambios: los fixes no añaden dependencias nuevas.
