# NInfer — port Windows v1.0.6

Este árbol es el **port Windows de `Neroued/ninfer` actualizado a upstream `master`
(`a16b6442`, 2026-09-07)**: equivale al port v1.0.5 (sincronizado en `ad0f3d38`)
**más los 88 commits** del rango `ad0f3d38..a16b6442`.

## Contenido del delta (88 commits)

| Área | Contenido |
|---|---|
| **DFlash2** (~70 commits) | Nuevo backend de decodificación especulativa: backbone de draft de 5 capas, selector top-16, rechazo sparse. `--spec dflash2 --draft-tokens K` (K=1..15), Text/Vision, eager/Graph, ambos proposal heads. Requiere artifact con pesos compañeros DFlash2 (p. ej. `gpillon/Qwen3.8-27B-nvfp4full-dflash2-NInfer`). |
| `385b30ce` | Integración de dflash2 en el engine; unifica `--spec mtp\|dflash\|dflash2` + `--draft-tokens`. |
| `4df5e0b4` | Conversor: pesos dflash2 para qwen3.8. |
| `03177b91` | fix(runtime): preservar coverage KV en terminal settlement especulativo. |
| `487f8977` | perf(sparse_moe): one CTA per token en S2 small-T + warp merge (reemplaza 8 rondas dependientes). |
| `22d8a1d3`, `aa429f28`, `1c8f8acc`, `4b0eb36c`, `6d1da9ce` | perf(ops): w8 vocabulary t64 route (elimina el schedule de vocabulario clásico; launchers por capacidad 8..40 con `TiledColumns`), gdn input t16 vía grouped MMA, swiglu nvfp4 fusionado t96, verify attention h24 small-t, q5 linear add. |
| docs/bench | Informes de rendimiento reorganizados (`docs/performance/*.md`), bench corpus dflash2, fix ttft staging. |

## CLI

- `--mtp-draft-tokens` ya no existe: el contrato es `--spec <mtp|dflash|dflash2> --draft-tokens N`
  (MTP N=1..5; DFlash/DFlash2 N=1..15). `--lm-head-draft` sigue eligiendo el proposal head optimizado.
- Los `.bat` existentes con `--spec mtp --draft-tokens 3` siguen funcionando sin cambios.

## Qué se conservó (adaptaciones MSVC/Windows)

Igual que v1.0.5 — sin cambios nuevos en este port:
- `unsigned __int128` donde aplica, headers POSIX → Windows con `#ifdef _WIN32`
  (`process.h`, `<random>`, `intrin.h`, `localtime_s` con args invertidos).
- `src/CMakeLists.txt`: FFmpeg/LibCurl condicionales (`NINFER_BUILD_MEDIA_ACQUIRE`).
- `nvfp4_w4a4_tma.cuh`: `alignas(64)`.

Los 88 commits no introdujeron constructos nuevos incompatibles con MSVC
(escaneado: sin `__int128` nuevo, sin includes POSIX en archivos nuevos,
sin `localtime_r`/`pthread`/`gettimeofday` fuera de `#ifdef`).

## Estado de alineación (verificado 2026-09-07)

- **39 archivos** difieren de `upstream/master` (`a16b6442`): tooling Windows
  (bat, CMake root, workflows, READMEs localizados, eval scripts), 13 archivos
  de código con la adaptación MSVC, y `PORT_v1.0.5.md`.
- El resto del árbol (~2000 archivos, incluidos todos los kernels dflash2) es
  **idéntico byte a byte** a upstream `a16b6442`.
- README principal: fusión manual — secciones Windows (Installation/Building/
  Running/OpenCode) + secciones nuevas de upstream (Performance con tablas
  saturación/corpus, Capabilities and limits, Documentation, Support, License).

## Compilar

Script autocontenido en el árbol: **`build_v1.0.6.bat`** (sm_120a, visión,
Release). Solo necesita este árbol + MSVC BuildTools + CUDA 13.3 + Ninja
(rutas ya puestas en el script). Por defecto compila en
`_build_5090new` (relativo al árbol del repo); acepta un build dir alternativo como
primer argumento: `build_v1.0.6.bat <build_dir>`. Sin `pause` (background).
Copia exes + DLLs FFmpeg (de `ffmpeg\bin\`) a la raíz del build dir.

```bat
build_v1.0.6.bat
```

Verificado: build Release completo sin errores (el 2026-09-07, MSVC + CUDA 13.3).

- Binarios de esta verificación: `_build_5090new\`
  (`ninfer-serve.exe` + DLLs FFmpeg; el resto del árbol de build se movió a
  `_build_5090new-delete\`).
- Smoke test: `ninfer-serve.exe --help` muestra `--spec mtp|dflash|dflash2`;
  `--spec dflash2 --draft-tokens 7` parsea y arranca el engine (falla solo
  sin artifact).

## Uso DFlash2

- `start_ninfer_5090_vision_optimized_dflash2.bat` (en la raíz del repo):
  artifact `qwen3_8_27b_nvfp4full-v2.ninfer` + `--spec dflash2 --draft-tokens 7 --lm-head-draft`.
- Cifras upstream (Qwen3.8-27B nvfp4, RTX 5090, C=1, `docs/performance/qwen3.8-27b.md`):
  DFlash2 K=7 vs MTP3 — Code 194→266 tok/s (+37%), Structured 220→357 (+62%),
  razonamiento corto 195→321 (+65%), 65k 151→183 (+21%), Story ~0%.
