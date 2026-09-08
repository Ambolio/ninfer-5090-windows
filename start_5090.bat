@echo off
rem ============================================================
rem NInfer v1.0.6 - RTX 5090 (sm_120a) - generic startup.
rem
rem Model file expected next to this script:
rem   qwen3_8_27b_nvfp4.ninfer   (see download_model.bat)
rem
rem If you have more than one GPU, set CUDA_VISIBLE_DEVICES to
rem the index of your GPU as seen by CUDA.
rem
rem With --wddm-evictable-budget the engine budgets against TOTAL
rem VRAM instead of the WDDM process budget: up to 230k tokens at
rem C=2 (255k at C=1) fit on 32 GB (see README.md). DFlash2 needs
rem the companion artifact; with the base artifact below, MTP3 is
rem used. Add --vision to enable image/video input.
rem ============================================================
set CUDA_VISIBLE_DEVICES=0
ninfer-serve.exe qwen3_8_27b_nvfp4.ninfer ^
 --host 127.0.0.1 --port 8080 ^
 --max-context 131072 --kv-capacity auto --kv-dtype fp8 ^
 --wddm-evictable-budget --max-concurrency 1 --device-state-slots 1 ^
 --spec mtp --draft-tokens 3 --lm-head-draft ^
 --prefill-chunk 1024
pause
