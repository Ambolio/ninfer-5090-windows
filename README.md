# NInfer 5090 Windows

> Windows port of NInfer for the NVIDIA GeForce RTX 5090 (`sm_120a`, Blackwell). Selected checkpoints. Maximum single-GPU inference performance. **100% Native Windows MSVC (no WSL2 required).**

**[⬇️ Descargar versión precompilada portable v1.0.7 (Windows 11) en GitHub Releases](https://github.com/Ambolio/ninfer-5090-windows/releases/download/v1.0.7-windows/ninfer-5090-windows-v1.0.7.zip)**

> 🖥️ **Companion repository (RTX 4090):** [Ambolio/ninfer-4090-windows](https://github.com/Ambolio/ninfer-4090-windows) — the Ada Lovelace (`sm_89`) sibling branch. Both repos publish the full two-card benchmark tables: see [Benchmarks — v1.0.7 cross-GPU campaign (2026-09-09)](#benchmarks--v107-cross-gpu-campaign-2026-09-09).

NInfer 5090 Windows is a native Windows 11 port of the upstream
[Neroued/ninfer](https://github.com/Neroued/ninfer) C++20/CUDA inference
engine, adapted to Blackwell (`sm_120a`): native DFlash2, NVFP4, FP8 KV, and
the WDDM evictable-budget bypass. It runs text, image, and video prompts
through a local CLI or OpenAI-/Anthropic-compatible HTTP APIs.

The performance numbers in this README were **measured with this build** on a
physical RTX 5090 (section [Measured with this build — RTX 5090](#measured-with-this-build--rtx-5090)).
They are not upstream numbers.

---

## Project Lineage & Credits

This branch stands on the work of the whole NInfer Windows ecosystem. With
gratitude to all of them — in lineage order:

| Contributor | Repository | Contribution |
|---|---|---|
| **Neroued** | [Neroued/ninfer](https://github.com/Neroued/ninfer) | Canonical upstream: C++20/CUDA architecture, Blackwell MMA kernels (`sm_120a`), native DFlash2, ReplaySSM, Paged KV Cache |
| **UDPSendToFailed** | [UDPSendToFailed/ninfer-4090](https://github.com/UDPSendToFailed/ninfer-4090) | **Creator of the original RTX 4090 fork**; pioneer of the WDDM evictable-budget bypass on Windows WDDM and of E8 lattice (Conway-Sloane) geometric quantization, `rk4v4-e8` |
| **sergiuszm** | [sergiuszm/ninfer-4090](https://github.com/sergiuszm/ninfer-4090) | Ada Lovelace `sm_89` kernel optimizations, `rk4v4-e8` adaptation, GDN cooperative-launch fix |
| **natpate** | [natpate/ninfer-windows](https://github.com/natpate/ninfer-windows) | Base Win32/MSVC portability layer, unbuffered asynchronous I/O (`OVERLAPPED`), initial Windows scripts |
| **headpiece747** | [headpiece747/ninfer-5090-windows](https://github.com/headpiece747/ninfer-5090-windows) | **Original RTX 5090 Windows fork**: native MSVC build, C-runtime patches, FFmpeg integration |
| **Don-Chad** | [Don-Chad/ninfer-3090](https://github.com/Don-Chad/ninfer-3090) | Pioneering Ampere work and early compatibility bridges |

Model foundations: **Qwen Team (Alibaba Cloud)** for the foundational model
architectures, **unsloth** for the NVFP4 quantizations, and **z-lab** for the
DFlash2 companion weights.

This branch would not exist without that work. See [NOTICE](NOTICE) for the
full legal attribution (Apache-2.0 §4) and third-party details.

---

## Relationship to Upstream (v1.0.7)

This branch tracks upstream `b88c0f6f` (v1.0.7: 7 commits post-v1.0.6 —
MoE pipeline/prefetch/L2 ×3, NVFP4 W4A4 TMA, open-addressed BPE table,
unicode NFC-skip, host-arena fix) on top of the v1.0.6 sync at `a16b6442`.
The engine core is shared 1:1 with upstream —
the Windows layer (MSVC build, WDDM bypass) does not touch the compute path,
which is why the numbers in this README land on top of the upstream published
RTX 5090 numbers (see [Comparison with the upstream repository](#comparison-with-the-upstream-repository)).

### Shared with upstream

- High-performance C++20/CUDA core and 1:1 compatibility with `.ninfer` artifacts.
- 7-token block speculation with DFlash2 (`--spec dflash2 --draft-tokens 7`) and
  transactional ReplaySSM for linear GDN states.
- HTTP APIs compatible with OpenAI Chat Completions / Responses and Anthropic
  Messages, including streaming, tools, and token counting.
- Low-latency prefix caching with paged Device/Host KV and State retention.

### Added by this fork

- **Native Windows 11 compilation**: CMake + MSVC 2022 + Ninja + CUDA 13.x —
  no WSL2, no virtualization overhead (`build_windows.bat`, `build_v1.0.7.bat`).
- **WDDM bypass (`--wddm-evictable-budget`)**: D3D12/DXGI residency lock that
  budgets runtime memory against total VRAM instead of the WDDM process
  budget (see
  [Windows WDDM and dedicated GPUs](#windows-wddm-and-dedicated-gpus)).
  Concept pioneered in
  [UDPSendToFailed/ninfer-4090](https://github.com/UDPSendToFailed/ninfer-4090).
- Windows C-runtime patches (random generation, thread-safe time, `/FS`),
  automated dependency management, and FFmpeg integration for Vision.

---

## Windows WDDM and dedicated GPUs

**If your GPU is dedicated, enable `--wddm-evictable-budget` to use its VRAM
to the maximum.** On Windows, the WDDM driver model gives every process a
memory *budget* that is a fraction of total VRAM (the OS holds back the rest
for the display compositor and TDR recovery), and a process that exceeds its
budget gets evicted or fails to commit. NInfer on Windows can instead take a
D3D12/DXGI residency lock and budget against **total VRAM**:

- With the flag, this build ran the 27B NVFP4 DFlash2 profile at
  **30,565 MiB resident of 32,607 MiB (93.7%)** — 23.7 GB weights + a
  228,224-token FP8-KV pool + pinned host KV, at C=2. Without the flag the
  same configuration does not start on a 32 GB card.
- **Every measurement in this README was taken with `--wddm-evictable-budget`
  enabled** — the long-context, C=2 profiles only fit on a 32 GB card because
  of it.
- The flag is safe: if a real GPU memory pressure event happens (e.g. a
  fullscreen game, another CUDA process), WDDM still evicts safely — the
  budget just moves from "a fraction of VRAM" to "VRAM minus the hard
  reserves".
- If you still cannot start the server, lower `--max-context` /
  `--kv-capacity` (or use `--kv-capacity auto`) until startup fits. One
  measured case: with the 23.7 GB NVFP4-DFlash2 artifact,
  `--max-context 230000` FATALs at startup (reservation 9.65 GB + 1 GiB
  automatic headroom vs 10.67 GB left after weights — a ~120 MB deficit);
  `--max-context 200000 --kv-capacity auto` starts cleanly with a 228,224
  token pool (the profile in the table below).

The flag is a no-op safety net on multi-GPU systems: pair it with
`CUDA_VISIBLE_DEVICES=<index>` to pin the engine to your card.

---

## Measured with this build (RTX 5090)

Measured **2026-09-08 on a physical RTX 5090 (32 GB, `sm_120a`)** with the
pre-compiled binary of this branch (`CUDA_VISIBLE_DEVICES` pinned to the
5090), `--wddm-evictable-budget` enabled on **every** profile, single
stream, temperature 0.7. Prefill prompts are deterministic (~12.8k and
~56.4k tokens); decode is one 2,048-token free generation. Timings are the
ones the server itself reports (`prompt_per_second`,
`predicted_per_second`); VRAM is `nvidia-smi` on the 5090 (32,607 MiB).

| Profile | Weights | KV pool (resolved) | Prefill 12.8k tok | Prefill 56.4k tok | Decode 2048 tok | Draft acceptance | VRAM peak |
|---|---:|---:|---:|---:|---:|---:|---:|
| Qwen3.8-27B NVFP4 — FP8 KV, MTP3 d3, C=2, 131k ctx | 21.5 GB | 262,144 tok (auto, 9.48 GiB) | **8,754 tok/s** (TTFT 1.5 s) | **6,159 tok/s** (TTFT 9.2 s) | **144.9 tok/s** | 42.4 % (1,145/2,703) | 30,207 MiB (92.6 %) |
| Qwen3.8-27B — groupwise-int, FP8 KV, MTP3 d3, C=2, 131k ctx | 18.2 GB | 262,144 tok (auto, 9.48 GiB) | **3,515 tok/s** (TTFT 3.6 s) | **3,006 tok/s** (TTFT 18.8 s) | **149.3 tok/s** | 42.2 % (1,144/2,709) | 27,079 MiB (83.0 %) |
| Qwen3.8-27B NVFP4 DFlash2 — FP8 KV, dflash2 d7, C=2, 200k ctx | 23.7 GB | 228,224 tok (auto, 8.93 GiB) | **8,063 tok/s** (TTFT 1.6 s) | **6,094 tok/s** (TTFT 9.2 s) | **191.3 tok/s** | 31.7 % (1,410/4,453, 7 tok) | 30,565 MiB (93.7 %) |
| Qwen3.8-27B DFlash2 — groupwise-int, FP8 KV, dflash2 d7, C=2, 131k ctx | 20.4 GB | 262,144 tok (auto, 9.97 GiB) | **3,499 tok/s** (TTFT 3.7 s) | **2,993 tok/s** (TTFT 18.8 s) | **130.2 tok/s** | 26.1 % (1,323/5,065, 7 tok) | 28,505 MiB (87.4 %) |

Engine startup (weights load + CUDA graphs): 8.7 s / 7.8 s / — / 8.5 s
respectively (the NVFP4-DFlash2 row ran on a persistent verification
server, so its startup was not separately instrumented).

Reading the table:

- **NVFP4 vs groupwise-int**: NVFP4 weights cut the bytes read per token, so
  prefill is ~2.5× faster (8,754 vs 3,515 tok/s). Decode is a wash
  (144.9 vs 149.3 tok/s): at C=1 the dense attention path and draft
  acceptance, not weight bandwidth, set the rate.
- **DFlash2 vs MTP3**: on NVFP4, DFlash2 wins by +32% (191.3 vs 144.9
  tok/s) — 7-token drafts at 31.7% per-draft acceptance commit more tokens
  per step than 3-token drafts at 42.4%. On groupwise-int the picture
  inverts (130.2 vs 149.3): the DFlash2 companion head baked into the
  g64 artifact has a lower per-draft acceptance (26.1%), which erases the
  block-length advantage.
- **`--kv-capacity auto`** resolved a 262,144-token pool for the three 131k
  profiles and 228,224 for the 200k DFlash2 profile (see the
  [WDDM note](#windows-wddm-and-dedicated-gpus) for why 230k does not fit).
  All of this is only possible with the WDDM budget; without the flag none
  of the four profiles would start at these capacities.
- All model artifacts are published by Neroued. The base
  [`qwen3_8_27b_nvfp4.ninfer`](https://huggingface.co/neroued/Qwen3.8-27B-nvfp4-NInfer)
  and [`qwen3_8_27b.ninfer`](https://huggingface.co/neroued/Qwen3.8-27B-NInfer)
  artifacts used above are public. The two DFlash2 artifacts
  (`qwen3_8_27b_nvfp4dflash2.ninfer`, `qwen3_8_27b_dflash2.ninfer`) are the
  same public base weights with the DFlash companion merged in via the
  upstream converter pipeline; as of 2026-09-09 the merged artifacts are not
  yet published in Neroued's public HuggingFace repos (verified across all of
  his public NInfer repos).

---

## Comparison with the upstream repository

The upstream project publishes RTX **5090** reference numbers
(docs/performance, v1.0.6, revision `487f8977`, INT8 group-64 KV, auto
capacity). Both sides are now on the **same hardware** (RTX 5090) and share
the same engine core at v1.0.6, so the ratios below measure the remaining
differences — KV dtype (FP8 vs INT8 group-64), artifacts and prompt content —
not the port:

| Metric (single stream) | Upstream 5090 (published) | This build — 5090 (measured) | Ratio |
|---|---:|---:|---:|
| 27B NVFP4 prefill ~7.7k tok (INT8 g64 KV) | 8,340.4 tok/s | 8,754 tok/s (12.8k tok, FP8 KV) | **105 %** |
| 27B NVFP4 MTP3 decode C1 | 143.8 tok/s (48.9 % accept) | 144.9 tok/s (42.4 % accept) | **101 %** |
| 27B g64 prefill ~7.7k tok (INT8 g64 KV) | 3,274.7 tok/s | 3,515 tok/s (12.8k tok, FP8 KV) | **107 %** |
| 27B g64 MTP3 decode (structured output) | 224.4 tok/s | 149.3 tok/s (free generation, 42.2 % accept) | 67 % |

Caveats:

1. **Prefill length**: our prompts are 12.8k tokens, longer than the
   upstream 7,680-token point; the rate is sustained over the longer prefill
   (we also measured the 56.4k point in the table above).
2. **Decode acceptance** is prompt-content-dependent: the upstream C1
   27B-nvfp4 row (48.9%) and our free-generation row (42.4%) use different
   prompt content; the committed-tokens rate is identical (144.9 vs 143.8).
3. **Scenario mismatch on the g64 decode row**: upstream's 224.4 tok/s is a
   *structured-output* MTP3 point (structured outputs draft-accept far
   better); our 149.3 tok/s is free-form generation. Not like-for-like — the
   67% ratio understates parity.
4. **Upstream 260k-prefill points** (2,203.1 / 1,609.7 tok/s): we did not
   run a 260k prefill on the 5090; the closest measured point is the 56.4k
   prefill in the table above.

The full upstream published tables (same revision) are kept below for
reference.

### Upstream published reference (RTX 5090)

**Concurrent MTP3 decode** — saturated decode used INT8 group-64 KV, CUDA
Graphs, MTP3, and one 8,192-token generation per active request. Throughput
uses aggregate committed decode tokens from complete intervals whose actual
decode batch equaled the configured concurrency. Acceptance covers the
complete request wave; these rates are steady decode (tok/s).

| Model profile | C=1 tok/s / accept | C=2 tok/s / accept | C=4 tok/s / accept | C=8 tok/s / accept | C8 / C1 |
|---|---:|---:|---:|---:|---:|
| Qwen3.6-27B `groupwise-int` | 185.8 / 68.2% | 247.0 / 69.0% | 309.5 / 68.4% | 535.0 / 68.3% | 2.88× |
| Qwen3.6-27B `nvfp4` | 202.4 / 69.3% | 399.7 / 71.4% | 699.7 / 69.3% | 1,146.9 / 68.6% | 5.67× |
| Qwen3.6-35B-A3B `groupwise-int` | 642.5 / 68.6% | 907.2 / 66.3% | 1,213.5 / 69.6% | 1,380.7 / 68.0% | 2.15× |
| Qwen3.8-27B `nvfp4` | 143.8 / 48.9% | 267.6 / 48.1% | 461.1 / 45.8% | 766.6 / 46.0% | 5.33× |

**Single-request serving** — the serial serving corpus used INT8 group-64 KV,
CUDA Graphs, a 1,024-token prefill chunk, and five fixed seeds after warm-up.

| Model profile | 7,680-token prefill | 260,096-token prefill | Structured MTP3 decode |
|---|---:|---:|---:|
| Qwen3.6-35B-A3B `groupwise-int` | 17,705.4 tok/s | 5,247.0 tok/s | 779.6 tok/s |
| Qwen3.6-27B `groupwise-int` | 3,218.1 tok/s | 1,614.8 tok/s | 193.0 tok/s |
| Qwen3.6-27B `nvfp4` | 11,191.5 tok/s | 2,510.6 tok/s | 252.2 tok/s |
| Qwen3.8-27B `groupwise-int` | 3,274.7 tok/s | 1,609.7 tok/s | 224.4 tok/s |
| Qwen3.8-27B `nvfp4` | 8,340.4 tok/s | 2,203.1 tok/s | 219.8 tok/s |

### Cross-card: this 5090 vs the 4090 sibling repository

The sibling repository [Ambolio/ninfer-4090-windows](https://github.com/Ambolio/ninfer-4090-windows)
publishes its own fully measured RTX 4090 table (sm_89, `rk4v4-e8` KV). For
the two artifacts present on both cards (same artifacts, same harness,
different card + KV dtype):

| Artifact / profile (single stream) | 4090 (`rk4v4-e8` KV, measured) | 5090 (FP8 KV, measured) | 5090 / 4090 |
|---|---:|---:|---:|
| 27B groupwise-int, MTP3 d3 — decode | 97.8 tok/s | 149.3 tok/s | **+52.7 %** |
| 27B groupwise-int, MTP3 d3 — prefill 12.8k | 2,143 tok/s | 3,515 tok/s | **+64.0 %** |
| 27B DFlash2 d7 — decode | 104.1 tok/s | 130.2 tok/s | **+25.1 %** |
| 27B DFlash2 d7 — prefill 12.8k | 2,079 tok/s | 3,499 tok/s | **+68.3 %** |

(The NVFP4 artifacts and the 35B-A3B v2 artifact were only benched on one
card each in this pass: 35B-A3B on the 4090, NVFP4 on the 5090.)

---

## Benchmarks — v1.0.7 cross-GPU campaign (2026-09-09)

Measured **2026-09-09** in a single back-to-back campaign on one dual-GPU
machine (Windows 11): the **RTX 4090 (24 GB, `sm_89`)** and the **RTX 5090
(32 GB, `sm_120a`)** — the v1.0.7 binaries (sha256-verified byte-identical to
the production binaries), `--wddm-evictable-budget` on every server, and the
exact per-run argv recorded in each point JSON. Same artifacts, same harness,
same day on both cards.

> 🖥️ **Sibling repositories:** the full per-card data — methodology,
> deviations registry (D1–D11), raw point JSONs and campaign logs — live in
> both [Ambolio/ninfer-4090-windows](https://github.com/Ambolio/ninfer-4090-windows)
> and [Ambolio/ninfer-5090-windows](https://github.com/Ambolio/ninfer-5090-windows).
> This section is identical in both repos on purpose, so each page shows the
> numbers for *both* cards.

**Artifacts used in this campaign** (public HuggingFace repos, by Neroued):

| Artifact | Weights | HuggingFace | Used for |
|---|---|---|---|
| `qwen3_6_35b_a3bv2.ninfer` — Qwen3.6-35B-A3B v2 | groupwise-int, 20.6 GiB | [Qwen3.6-35B-A3B-NInfer](https://huggingface.co/neroued/Qwen3.6-35B-A3B-NInfer) | S3 + P0 — both cards |
| `qwen3_8_27b_nvfp4.ninfer` — Qwen3.8-27B | nvfp4, 21.5 GB | [Qwen3.8-27B-nvfp4-NInfer](https://huggingface.co/neroued/Qwen3.8-27B-nvfp4-NInfer) | N0 + NS — 5090 |
| `qwen3_8_27b.ninfer` — Qwen3.8-27B | groupwise-int, 16.7 GiB | [Qwen3.8-27B-NInfer](https://huggingface.co/neroued/Qwen3.8-27B-NInfer) | NS — 4090 |

(The 35B-A3B "v2" is the current production conversion of the public
Qwen3.6-35B-A3B family; the 27B rows use the two public 27B conversions —
NVFP4 on the 32 GB card, groupwise-int on the 24 GB card.)

### S3 — 35B-A3B v2, MTP3 d3, saturated decode (both cards)

Stochastic 8,192-token generation per request (293-token prompt), at
concurrency C = 1/2/4/8; int8 KV, `auto` capacity. Steady-state committed
decode rate:

| C | 4090 — steady (tok/s) | 5090 — steady (tok/s) | 5090 / 4090 |
|---:|---:|---:|---:|
| 1 | 459.5 | 672.9 | **1.46×** |
| 2 | 660.7 | 974.3 | **1.47×** |
| 4 | 914.3 | 1,336.4 | **1.46×** |
| 8 | 1,095.5 ¹ | 1,544.5 | **1.41×** |

Draft acceptance: 4090 67.0–71.1 % · 5090 66.3–68.6 % — 8/8 real concurrent
requests on both cards.

¹ **24 GB wall with int8:** the `auto` pool on the 4090 resolves 59,648
tokens < the 8×8,485 needed for C=8, so that point re-ran with the documented
ladder `--kv-dtype rk4v4-e8 --kv-capacity 131072` (E8-lattice KV, 748 MiB
pool) — still 8/8 real, mean batch 8.0. The 5090 resolves the full
131,072-token pool with int8 `auto` in 32 GB.

### NS — 27B, MTP3 d3, saturated decode (both cards)

Same protocol (335-token prompt + 8,192 decode); int8 `auto` KV pools of
16,384 / 32,768 / 65,536 / 113,216 (4090) and 16,384 / 32,768 / 65,536 /
131,072 (5090):

| C | 4090 — steady (tok/s) | 5090 — steady (tok/s) | 5090 / 4090 |
|---:|---:|---:|---:|
| 1 | 108.5 | 148.1 | 1.37× |
| 2 | 164.6 | 281.2 | 1.71× |
| 4 | 185.9 | 491.8 | 2.65× |
| 8 | 290.5 | 827.5 | 2.85× |

Draft acceptance: 4090 46.1–47.9 % · 5090 44.9–46.2 %.

⚠ **Not like-for-like:** the 4090 ran the **groupwise-int** 27B artifact
(16.7 GiB) and the 5090 the **NVFP4** one (21.5 GB), so the widening ratio
(1.37× → 2.85×) is hardware *plus* weight quantization — NVFP4 reads fewer
bytes per token, and the gap grows with concurrency. The S3 table above is
the same-artifact comparison: ~1.45× with the identical 35B-A3B v2 on both
cards.

### P0 — 35B-A3B v2, MTP0 (no speculation), NIAH context corpus (both cards)

20 serial requests = 5 seeds × {8k, 64k, 128k, 256k} contexts
(2,311,680 prompt tokens total). Prefill and decode rates per context point:

| Context (tok) | 4090 prefill | 5090 prefill | 4090 TTFT | 5090 TTFT | 4090 decode | 5090 decode |
|---:|---:|---:|---:|---:|---:|---:|
| 7,680 | 12,375.1 | 18,699.8 | 624 ms | 414 ms | 240.7 | 363.9 |
| 64,512 | 9,418.1 | 12,092.7 | 6,875 ms | 5,363 ms | 202.2 | 317.9 |
| 130,048 | 7,193.4 | 8,417.1 | 18,127 ms | 15,503 ms | 173.2 | 278.2 |
| 260,096 | 4,927.2 | 5,261.9 | 52,884 ms | 49,530 ms | 136.3 | 225.5 |

(prefill/decode in tok/s; full-corpus makespan: 4090 **393.9 s** · 5090
**355.3 s**.)

### N0 — 27B NVFP4, MTP0, NIAH context (5090 only)

| Context (tok) | Prefill (tok/s) | TTFT (ms) | Decode (tok/s) |
|---:|---:|---:|---:|
| 7,680 | 9,780.6 | 789 | 75.9 |
| 64,512 | 5,778.1 | 11,190 | 69.6 |
| 130,048 | 3,866.0 | 33,692 | 63.7 |
| 260,096 | 2,331.6 | 111,650 | 54.6 |

(The 27B context point ran on the 5090 only — the 4090 27B context run was
outside the campaign's fast profile; its 27B decode-saturation point is the
4090 column of the NS table.)

### Windows vs upstream Linux parity (same card)

The point of the campaign: same models, same commands, same GPU — the
measured delta is the overhead of the Windows port (WDDM), nothing else.

- **RTX 5090 — parity.** Windows matches or slightly exceeds the numbers
  upstream published for the same card, across every point of this campaign:
  S3 steady 104.7–111.9 % of upstream, NS steady 103.0–107.9 %, P0 prefill
  100.3–105.6 %, P0 decode 105.9–107.6 %, N0 prefill 105.9–117.3 %.
- **RTX 4090 — 38–79 % of the upstream *5090* reference** (S3 71.5–79.3 %,
  NS 37.9–75.4 % — with the quantization caveat above —, P0 63.5–93.9 %):
  that is the Ada-vs-Blackwell hardware gap, not port overhead. Within the
  same hardware the port sits at parity (100–117 % on the 5090; the 4090's
  own v1.0.5 → v1.0.6 A/B — int4-KV fix — measured +6.8 %).

### Validation (same campaign)

- **4090:** full ctest suite on the v1.0.7 build — 106/109 passed (3
  documented failures: 2 = MSVC test-binary artifact `0xC0000409`, 1 = known
  deterministic borderline; the v1.0.7 server with real production data
  boots and serves clean, :8091 smoke) + 7 skipped by design. pytest
  75 passed / 3 skipped / 1 failed — the single failure is a Windows
  path-separator artifact in a converter test (`endswith("/model")`), not
  port logic.
- **5090:** ctest pass covered by the 4090 run (byte-identical trees); the
  v1.0.7 5090 deployment additionally validated its suite 103/103 executed
  green (1 `DISABLED` on Windows — the BEX64 test-binary artifact, engine
  verified clean with a production-artifact smoke). pytest 75/3/1 (same
  path-separator artifact).

---

## Running the server

### Generic startup (shipped as `start_5090.bat`)

Minimal configuration — adjust `--max-context`, `--kv-capacity` and
`--max-concurrency` to your VRAM and workload:

```bat
@echo off
set CUDA_VISIBLE_DEVICES=0
ninfer-serve.exe qwen3_8_27b_nvfp4.ninfer ^
 --host 127.0.0.1 --port 8080 ^
 --max-context 131072 --kv-capacity auto --kv-dtype fp8 ^
 --wddm-evictable-budget --max-concurrency 1 --device-state-slots 1 ^
 --spec mtp --draft-tokens 3 --lm-head-draft ^
 --prefill-chunk 1024
pause
```

### Flag notes

- `--kv-dtype fp8` is the default KV storage on this branch (`bf16`/`int8`
  are also accepted; `rk4v4-e8` is an sm_89-only storage).
- `--wddm-evictable-budget` — see [Windows WDDM note](#windows-wddm-and-dedicated-gpus).
- `--kv-capacity auto` resolves the largest pool that fits after weights
  (keeping 1 GiB of sizing headroom); explicit values are fixed for the
  process lifetime and must be at least `--max-context`.
- `--spec mtp --draft-tokens 3` (MTP3) or `--spec dflash2 --draft-tokens 7`
  (DFlash2, needs the merged companion artifact).
- `--max-concurrency` / `--device-state-slots`: one state slot per active
  request; keep them equal for the simplest scheduling.
- `--vision` enables image/video input (FFmpeg DLLs are in the ZIP).
- Multi-GPU: set `CUDA_VISIBLE_DEVICES` to the index of *your* GPU as seen by
  CUDA. Note that CUDA's enumeration order can differ from `nvidia-smi`'s
  order on multi-GPU systems — verify with a short probe run or by watching
  which card's VRAM moves.

### Verified 200k DFlash2 profile (NVFP4, this hardware)

The table row that uses 93.7 % of the 32 GB card — the long-context
production profile:

```bat
@echo off
set CUDA_VISIBLE_DEVICES=0
ninfer-serve.exe qwen3_8_27b_nvfp4dflash2.ninfer ^
 --host 127.0.0.1 --port 8080 ^
 --max-context 200000 --kv-capacity auto --kv-dtype fp8 ^
 --wddm-evictable-budget --max-concurrency 2 --device-state-slots 2 ^
 --spec dflash2 --draft-tokens 7 --lm-head-draft ^
 --prefill-chunk 1024
pause
```

---

## Supported Models

Primary target:

| Model | Weights | Artifact | Download and model card |
|---|---|---|---|
| Qwen3.8-27B NVFP4 | `nvfp4` | `qwen3_8_27b_nvfp4.ninfer` | [Qwen3.8-27B-nvfp4-NInfer](https://huggingface.co/neroued/Qwen3.8-27B-nvfp4-NInfer) |

Also verified on this branch (measured above). Model artifacts: **Neroued**
(NInfer checkpoints on HuggingFace).

| Model | Artifact | Download |
|---|---|---|
| Qwen3.8-27B | `qwen3_8_27b.ninfer` (groupwise-int, FP8 KV, MTP3) | [Qwen3.8-27B-NInfer](https://huggingface.co/neroued/Qwen3.8-27B-NInfer) |
| Qwen3.8-27B DFlash2 (NVFP4) | `qwen3_8_27b_nvfp4dflash2.ninfer` (dflash2, 7 drafts) | the public NVFP4 artifact above + DFlash companion, merged with the upstream converter pipeline; the merged artifact is not yet on Neroued's public HF (2026-09-09) |
| Qwen3.8-27B DFlash2 (g64) | `qwen3_8_27b_dflash2.ninfer` (dflash2, 7 drafts) | the public g64 artifact above + DFlash companion, merged with the upstream converter pipeline; the merged artifact is not yet on Neroued's public HF (2026-09-09) |

---

## Requirements

- 64-bit Windows 11 (Native, **no WSL2 required**)
- NVIDIA GeForce RTX 5090 (`sm_120a`)
- NVIDIA driver with CUDA 13.x support (pre-compiled ZIP)
- Microsoft Visual C++ Redistributable 2015–2022 (x64)
- Source builds only: Visual Studio 2026 or 2022 (Developer Command Prompt / MSVC), CUDA 13.3 Toolkit (or 13.1+), CMake 3.28+, Ninja

---

## Installation (Pre-compiled)

**Download the [ninfer-5090-windows-v1.0.7.zip](https://github.com/Ambolio/ninfer-5090-windows/releases/download/v1.0.7-windows/ninfer-5090-windows-v1.0.7.zip) from the [v1.0.7-windows release](https://github.com/Ambolio/ninfer-5090-windows/releases/tag/v1.0.7-windows).**

The ZIP contains `ninfer-serve.exe` with its runtime DLLs (FFmpeg), a generic
`start_5090.bat`, a `download_model.bat`, and a `LEEME.txt` with instructions
and model links.

1. Extract the ZIP to a folder.
2. Run `download_model.bat` to download the `qwen3_8_27b_nvfp4.ninfer` model
   file (~20 GB, or download manually from
   [HuggingFace](https://huggingface.co/neroued/Qwen3.8-27B-nvfp4-NInfer/resolve/main/qwen3_8_27b_nvfp4.ninfer)).
3. Double-click `start_5090.bat` to launch the server.
4. Point any OpenAI-compatible client at `http://127.0.0.1:8080/v1`.

---

## Building from Source (For Developers)

### 1. Build Automatically

```cmd
build_windows.bat
```

*(The script automatically downloads the required FFmpeg dev package,
locates your MSVC environment, and builds with Ninja.)*

### 2. Manual CMake Build

Open the **x64 Native Tools Command Prompt** and run:

```cmd
cmake -B build -S . -G Ninja -DCMAKE_CUDA_ARCHITECTURES="120a" -DNINFER_ENABLE_AVX2=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## Capabilities and limits

All registered model IDs support:

- text generation with thinking and non-thinking prompt modes;
- image, multi-image, video, and mixed multimodal messages;
- chunked prefill, exact-batch CUDA Graph decode, and startup-bounded batched decode;
- MTP speculative decoding with draft windows from one to five, and DFlash2
  (`--spec dflash2 --draft-tokens 7`, verified e2e on this branch) with draft
  counts 1..15 and either full or optimized proposal heads;
- BF16, INT8, FP8, NVFP4, and K8V4 KV storage;
- offline causal-perplexity scoring;
- private and shared exact-prefix reuse with Device/Host State and KV retention;
- model-aware sampling defaults and explicit sampler overrides;
- OpenAI Responses Core, OpenAI Chat Completions, and Anthropic Messages,
  including streaming, tools, local response state, token counting, and usage
  accounting.

The product boundary remains intentionally small:

- one RTX 5090 and one resident model per Engine;
- a startup-fixed capacity of one to eight active requests with bounded FIFO ingress;
- no request preemption, priority/QoS, active-request swapping, weight offload, multi-GPU, or
  distributed serving;
- one shared startup-fixed KV pool across active requests and retained prefixes;
- no runtime model discovery or unregistered checkpoint fallback;
- parsed tool calls are returned to the client; NInfer does not execute tools;
- the in-tree C++ headers are not distributed as an installed SDK.

`--max-context` is each sequence's logical limit. `--kv-capacity` sizes the
shared Main Text KV pool used by active requests and retained prefixes;
`auto` resolves the largest legal capacity at startup from the memory
remaining after weights while keeping 1 GiB of sizing headroom. Explicit
capacities remain fixed for the process lifetime.

---

## Documentation

- [Documentation index](docs/README.md)
- [CLI](docs/cli.md)
- [HTTP serving](docs/serving.md)
- [Performance](docs/performance.md)
- [Perplexity evaluation](docs/perplexity.md)
- [Resource scheduling and context cache](docs/maintainer/resource-scheduling-and-context-cache.md)
- [Serve TTFT benchmark](tools/bench/ttft/)
- [CLI examples](examples/cli/)
- [Contributing](CONTRIBUTING.md)

Run the relevant `--help` for the exact current option contract.

## Support

NInfer is a personal project that Neroued develops out of interest. If you
find it useful and would like to support its continued development, you can
[support the project on Ko-fi](https://ko-fi.com/neroued).

Support is entirely voluntary. It is not a purchase or investment and does
not come with financial returns, promised services or features, or a role in
project decisions. The project's direction, priorities, technical choices,
and release schedule remain independently determined by the maintainer.

---

## License & Attribution

This project is licensed under the [Apache License 2.0](LICENSE).

This repository is a Windows MSVC adaptation of the upstream
[Neroued/ninfer](https://github.com/Neroued/ninfer) project, originally
authored by **Neroued** and licensed under the Apache License 2.0. In
accordance with Apache License 2.0 Section 4, all original attribution and
copyright notices are retained; the lineage credits above and the
[NOTICE](NOTICE) file are part of the distribution.

The published artifacts are derived from
[Qwen/Qwen3.8-27B](https://huggingface.co/Qwen/Qwen3.8-27B) and the Qwen
3.6 family. The Qwen3.8-27B NVFP4 artifact also uses the fixed mixed
FP8/NVFP4 weights from
[unsloth/Qwen3.8-27B-NVFP4](https://huggingface.co/unsloth/Qwen3.8-27B-NVFP4).
DFlash2 companion weights: [z-lab](https://huggingface.co/z-lab). These
source repositories are distributed under their own licenses. Vendored
dependencies retain their own license files under `third_party/`.

**Third-party binary distribution.** The pre-compiled ZIP packages include
FFmpeg shared libraries (avcodec, avformat, avutil, swresample, swscale) from
the BtbN `ffmpeg-master-latest-win64-gpl-shared` build, distributed under
GPL v2 or later; the full license text ships as `LICENSE-FFMPEG.txt` in the
ZIP and in the repository root. The corresponding source is the FFmpeg
source tree of that build (https://ffmpeg.org,
https://github.com/BtbN/FFmpeg-Builds). NVIDIA, CUDA, and RTX are trademarks
of NVIDIA Corporation. Model weights are **not** redistributed with this
project: users download them directly from HuggingFace under the model
owners' own licenses.

**Disclaimer.** This software is provided "as is" (AS IS), without warranty
of any kind, express or implied. The authors are not liable for hardware
damage, system instability, data loss, or overheating resulting from the use
of these binaries or configurations, including configurations that run the
GPU at or near its full memory and power envelope. You use this software at
your own responsibility.
