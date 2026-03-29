# Code Audit Report — 2026-03-26

## Scope
- Core C modules in `src/` with emphasis on memory-safety, API robustness, and parser correctness.
- Existing CI workflow in `.github/workflows/esp-idf-compat.yml`.
- New host-side regression tests in `tests/host/test_core.c`.

## Method
1. Manual review of core low-level modules (`bitset`, `base64`, `query_params`) for arithmetic, bounds, and null handling.
2. Added host-executable regression tests to exercise code paths independently of ESP-IDF.
3. Added sanitizer-backed CI gate (`ASan` + `UBSan`) to catch latent memory/UB issues early.

## Findings

### 1) Critical: heap under-allocation in `bitset_new`
**File:** `src/bitset.c`

- **Issue:** allocation used `sizeof(bitset_t) + (size + 7 / 8)` where `7 / 8` is integer `0`.
- **Impact:** only `size` bytes were reserved for bit storage instead of `ceil(size/8)`, leading to invalid memory writes for many sizes and hard-to-debug corruption.
- **Fix:** changed to `sizeof(bitset_t) + ((size + 7) / 8)` and added `NULL` handling after `malloc`.

### 2) High: out-of-bounds read in `bitset_isset`
**File:** `src/bitset.c`

- **Issue:** no bounds check for `index` before reading bit array.
- **Impact:** potential out-of-bounds read and undefined behavior on malformed input paths.
- **Fix:** return `false` when `index >= bs->size`.

### 3) Medium: public headers rely on transitive includes
**Files:** `src/bitset.h`, `src/query_params.h`

- **Issue:** `bool` and `size_t` were used without including canonical headers (`<stdbool.h>`, `<stddef.h>`).
- **Impact:** fragile compilation behavior depending on include order.
- **Fix:** added explicit includes in both headers.

## CI hardening added

- Added `host-sanity (asan)` job to `.github/workflows/esp-idf-compat.yml`.
- Job compiles and runs host regression tests with:
  - `-Wall -Wextra -Werror -pedantic`
  - `-fsanitize=address,undefined -fno-omit-frame-pointer`
- This catches memory bugs independently from embedded toolchains and before long ESP-IDF matrix builds.

## Residual risk / next recommendations

1. Add fuzz targets for parser/decoder modules (`query_params`, `tlv`, `json`) and run nightly.
2. Enable static analysis (`clang-tidy`/`cppcheck`) as non-blocking first phase.
3. Expand host tests to cover malformed base64 inputs and boundary behavior for zero-sized bitsets.

## Status summary
- Critical/high findings in reviewed modules were remediated in this change set.
- CI now includes a fast, deterministic safety gate for regressions.
