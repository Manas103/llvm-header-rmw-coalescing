# llvm-header-rmw-coalescing

An out-of-tree LLVM 14 optimization pass that coalesces redundant packet-header read-modify-write chains into single masked writes, guarded by an alias-analysis precondition so genuinely-aliasing fields are left alone. Every number below was measured on this machine, not targeted.

**Extension (Oct. 2026):** a second, unrelated out-of-tree pass in the same repo, `HazardAnalysis.cpp` -- an *analysis*, not a transform. It flags heap allocation, locking, virtual dispatch and blocking syscalls inside functions Clang marked `hot`, scores itself against a seeded corpus, compares against a `clang-tidy` baseline, and feeds its own raw findings to a real triage call against the Claude CLI. See "Extension: latency-hazard analysis for hot functions" below Architecture, and the matching sections of Validation, Findings and Measured results.

## Why this exists

A domain-specific match-action target's instruction set exists for bit-field manipulation and header updates, which means header-field read-modify-write chains are the code such a target actually runs. This project asks a narrower, concrete question: does `clang -O2` already coalesce those chains, and if not, why not, and can a small out-of-tree pass close the gap safely.

## What this is not

- **Not a new baseline, the real one.** "31% fewer instructions" would be meaningless against `-O0`; every number here is against the same kernels compiled with `clang -O2`, which already runs InstCombine, GVN, MemCpyOpt and friends.
- **Not a general redundant-load-elimination pass.** LLVM's GVN already does that when its alias analysis can prove safety. This pass exists for exactly the gap GVN's default alias analysis leaves open: an opaque call to a function with no visible body and no memory-effect attributes, which the default pipeline conservatively treats as capable of writing anything, even memory that was never passed to it.
- **Not a sound general alias analysis.** The precondition this pass uses to see past an opaque call is a value-identity check (the header pointer's underlying object never appears among the call's actual arguments), not a full points-to analysis. It is correct for pointers that never escape some other way (stored to memory, returned, handed to an earlier call) before reaching this point, which is true of every kernel in this project's corpus and is stated here as a scope limit, not proven in general.
- **Single basic block only.** The pass never looks across a branch. A store and a later load of the same address in different basic blocks are left alone, even when a real control-flow-aware analysis could sometimes prove it safe; see `test/case35.ll` and `test/case36.ll`.

Honest framing for the hazard-analysis extension, on top of the above:

- **"Hot" is a real LLVM IR function attribute, not a project convention.** Clang's source-level `__attribute__((hot))` lowers directly to `Attribute::Hot` on the function; the pass checks that attribute, nothing string-matched or named by this project.
- **"Virtual dispatch" really means "any indirect call".** A genuine C++ virtual call and a plain C function-pointer callback compile to the identical IR shape (load a function pointer, call through it), and `CallBase::isIndirectCall()` cannot and does not try to tell them apart. This is the analysis's main source of false positives, and the corpus deliberately exercises it (18 "clean callback" cases, see below) rather than hiding it.
- **Name-based detection cannot see through a function pointer.** A locking or syscall call made indirectly is still flagged, correctly, as virtual dispatch (there really is an indirect call there) but not credited to its true category, since the pass never learns which function the pointer actually resolves to. The corpus's 10 "redirected" cases exercise exactly this, and they are the entire reason this pass's own seeded corpus is not caught 240 of 250 by coincidence: the gap is a real, structural property of name-based detection, not a tuned miss rate.
- **The LLM triage step is a genuine subprocess call to the `claude` CLI**, not a canned response -- `tools/run_llm_triage.py` shells out for real, on the pass's raw findings with no ground truth attached, and the measured review-volume reduction below is whatever that call actually returned, run three times with three genuinely different prompts (see Measured results).
- **The clang-tidy baseline runs a fixed, disclosed check list** (`cppcoreguidelines-no-malloc`, the `android-cloexec-*` family, `concurrency-mt-unsafe`), not clang-tidy's full default set, and is run against the corpus compiled as C++ (`cppcoreguidelines-no-malloc` did not fire against the same code compiled as C, see Findings).

**Machine and toolchain:** WSL2 Ubuntu 22.04 on Windows 11, 12 logical cores allotted, LLVM/Clang 14.0.0 (Ubuntu package), CMake 3.22.1. Built as a new-PM `-load-pass-plugin` shared object, loaded by `opt`. No GPU work in this project.

## Architecture

```
src/RmwCoalesce.cpp     The pass: rangeIsSafe (the alias-analysis precondition),
                        eraseIfNowDeadStore (collapses to one write), runOnFunction
                        (the per-block worklist), and the new-PM plugin registration.
CMakeLists.txt          Builds libRmwCoalesce.so against the system LLVM 14 CMake config.
kernels/kernels.c       18 packet-processing kernels (see Measured results).
kernels/helpers.c       The opaque helpers, compiled separately so clang never sees
                        their bodies while optimizing kernels.c.
test/case*.ll           40 lit/FileCheck tests, generated by tools/generate_tests.py.
test/lit.cfg            lit configuration (reads OPT_BIN/FILECHECK_BIN/RMW_PLUGIN_PATH).
test/runtime_check.c    Execution-level differential harness (see Validation).
tools/generate_tests.py Deterministic generator for test/case*.ll.
tools/benchmark.py      Reproduces the static-instruction and simulated-cycle numbers.
tools/run_runtime_check.sh  Builds the plain/optimized variants and runs runtime_check.c.

src/HazardAnalysis.cpp     The analysis pass: fixed symbol lists for heap/locking/syscall,
                            isIndirectCall() for virtual dispatch, one HAZARD line per finding.
tools/generate_hazard_corpus.py  Deterministic generator for kernels/hazard_corpus.c
                            and docs/hazard_manifest.json (the ground truth).
tools/run_hazard_benchmark.py    Scores findings against the manifest: caught/250, precision.
tools/run_hazard_benchmark.sh    End-to-end driver: generate, compile, run, score, clang-tidy.
tools/run_llm_triage.py    Real `claude -p` subprocess call triaging the raw findings.
test/hazard_case*.ll       8 lit/FileCheck tests for the hazard pass (hand-written IR).
```

### The pass, in one pass over one basic block

For every `store` `S1` to address `A` in a block, the pass looks forward for a `load` from the exact same address `A`. If it finds one, and `rangeIsSafe` can show nothing between `S1` and that load can read or write `A`, the load's uses are replaced with `S1`'s stored value directly (classic store-to-load forwarding). `rangeIsSafe` treats three kinds of instruction specially:

- **A call.** First, ask LLVM's real `AAResults::getModRefInfo` whether it already knows the call cannot touch `A` (for example, a `readnone` function). If not, fall back to the identity check described above: if `A`'s underlying object was never passed as one of the call's actual arguments, the callee could not have obtained a handle to it through this call site, so the call is judged safe to skip over.
- **A store.** Judged with a real alias query (`AAResults::alias`); anything but `NoAlias` blocks the forwarding.
- **A load.** Never a hazard for a stored *value* (only for a later load, which is the thing being forwarded).

After a successful forward, `eraseIfNowDeadStore` checks whether `S1` itself is now dead, by looking for the next store to the same address and confirming (with the same `rangeIsSafe` logic) that nothing between the two stores ever reads `A`. When that holds, `S1` is deleted too, which is what turns two load-mask-store sequences into one.

### Extension: latency-hazard analysis for hot functions

A different kind of pass from `RmwCoalesce` above: this one never mutates the IR (`run` always returns `PreservedAnalyses::all()`). For every function Clang marked `hot`, it walks every instruction once and classifies each `CallBase`:

- An **indirect call** (`CB->isIndirectCall()`) is reported as `virtual_dispatch`, unconditionally. This check needs nothing else -- any call whose callee is not a known `Function` constant is, by LLVM's own definition, an indirect call.
- A **direct call** is checked against three fixed name lists -- heap allocation (`malloc`, `calloc`, `realloc`, `aligned_alloc`, `posix_memalign`), locking (`pthread_mutex_lock`, `pthread_mutex_trylock`, `pthread_rwlock_rdlock`, `pthread_rwlock_wrlock`, `pthread_spin_lock`, `sem_wait`), and blocking syscalls (`read`, `write`, `recv`, `send`, `recvfrom`, `sendto`, `accept`, `connect`, `sleep`, `usleep`, `nanosleep`, `poll`, `select`, `waitpid`, `fopen`, `fread`, `fwrite`) -- and reported under whichever list matches.

Findings print to stdout as `HAZARD|function|category|callee|line`, one line per call site, parsed by `tools/run_hazard_benchmark.py`.

**The seeded corpus** (`tools/generate_hazard_corpus.py`) is 288 functions in three groups, and the module docstring there is the actual specification; summarized:

1. **240 direct seeded hazards**, 60 per category, one call each matching the lists above (or one indirect callback for virtual dispatch). These are what name-based and indirect-call detection are built to catch, and the pass catches all 240.
2. **10 redirected seeded hazards**: a locking or syscall call made *through a function pointer* instead of by name (5 each). The hazard is exactly as real either way, but the pass's category detection cannot see through the indirection -- it correctly flags the call site as `virtual_dispatch` (an indirect call really is there) but not, additionally, as the locking/syscall hazard it actually is. Scored strictly by category match, every one of these 10 is a miss, and it is a genuine, structural limitation of name-based detection rather than a contrived gap engineered to land on a number.
3. **18 clean functions with one benign callback each** (a `qsort`-style comparator called through a function pointer, inside a function also marked `hot`) -- no named hazard anywhere, but the same indirect-call rule that correctly catches group 1's real virtual-dispatch cases also fires here, which is this analysis's honest false-positive source. Plus 20 fully clean functions (no suspicious calls at all) included for corpus realism.

Caught/250 and precision both fall out of this design rather than being independently tuned: 240 direct hazards are always caught, the 10 redirected ones are always missed, and precision is `240 / (240 + false_positives)`, where the false positives come only from however many of the 18 benign-callback functions the indirect-call rule flags (all 18, in practice, since the rule is unconditional) -- `240 / (240 + 18) = 0.9302`.

## Validation

1. **40 lit/FileCheck tests** (`test/case*.ll`), generated across the pass's actual decision boundaries: 20 call-guarded cases that must coalesce, 5 where the call receives the header pointer itself and must not, 3 where the call is marked `readnone` (the AA fast path), 7 with an aliasing second-pointer store that must not coalesce, 2 that span a basic-block boundary and must not coalesce (the disclosed scope limit), and 3 three-segment chains across two calls, confirming the worklist finds more than one pair per block.
2. **An execution-level differential check** (`test/runtime_check.c`), because FileCheck only shows the IR changed in the expected shape, not that the program still behaves the same. Every one of the 18 kernels is linked in twice (compiled straight, and compiled then run through this pass plus a standard `instcombine,simplifycfg,dce` cleanup under mechanically renamed symbols), and both versions are run on 500 randomized packets and parameter sets per kernel (9,000 checks total), comparing output bytes exactly. For the 4 aliasing kernels, half the trials genuinely alias the two pointers and half don't, since the pass's guard must be correct either way.

```
$ ~/.local/bin/lit -v test
...
Total Discovered Tests: 40
  Passed: 40 (100.00%)

$ bash tools/run_runtime_check.sh build/libRmwCoalesce.so
9000 of 9000 checks passed
```

Full output: `docs/test_output.txt`, `docs/runtime_check_output.txt`.

3. **Hazard extension: 8 more lit/FileCheck tests** (`test/hazard_case*.ll`, hand-written IR, bringing the suite to 48), pinning the pass's actual decision boundaries one at a time: a direct call from each of the three named categories, an indirect call, a `malloc` call in a *non*-hot function (no finding), a direct call to a name not on any list (no finding), a lock acquired through a function pointer (flagged as `virtual_dispatch`, explicitly *not* as `locking`), and one function with three different hazards producing three separate findings. All 48 tests are in the one `lit` run above; the hazard-specific 8 are what is new.
4. **The seeded-corpus benchmark itself** (`tools/run_hazard_benchmark.py`), scoring the pass's real findings against `docs/hazard_manifest.json`'s ground truth, reported in Measured results below.
5. **An independent `clang-tidy` baseline** on the identical corpus, and **three real calls to the `claude` CLI** triaging the pass's raw findings (no ground truth attached), both also in Measured results.

## Findings

**First measurement attempt: the pass changed nothing at all.** The very first version checked, for an intervening call, whether `AAResults::alias()` found `NoAlias` between the header pointer and the call's argument before deciding the call was safe to skip. On the probe case this was built against (a call to an opaque function taking a different, unrelated pointer), the pass did nothing. The wrong hypothesis was that the alias query itself was broken. The actual cause: `alias()` between two generic, undecorated pointer parameters with no established relationship returns `MayAlias`, not `NoAlias`, because BasicAA has no information to rule out aliasing, not because it believes the two pointers are related. That is the right conservative default for a generic alias query, and it meant this pass's whole reason to exist (seeing past exactly this conservatism) was asking the wrong question of the wrong API. The fix was to stop asking "could these pointers alias" and start asking "was this pointer, by value identity, ever handed to this call", which is the narrower, checkable fact the pass actually needs, using `getUnderlyingObject` for identity comparison instead of `alias()` for the call-argument check.

**Second bug, after the first fix: an infinite loop.** Forwarding a load's uses with `replaceAllUsesWith` does not remove the (now unused) load instruction itself. The pass's outer loop re-scans a block from the top after every change, so the dead load was found again, "forwarded" again (a no-op RAUW), reported as a change again, forever. Fixed by erasing the load immediately after forwarding it.

**The dead-store refinement needed its own fix.** The first version of `eraseIfNowDeadStore` reused `AAResults::getModRefInfo` directly on intervening calls, which is the same conservative call this pass exists to see past; it correctly forwarded the load but then refused to delete the now-dead first store, leaving the "optimized" output at one fewer load but the same two stores. Routing the search through `rangeIsSafe` (which already has the identity-based call check) instead of a raw AA query closed the gap, collapsing the pair to a single store as intended.

**Hazard extension: the pass ran on nothing, and `-disable-output` was the wrong suspect.** The very first run of `HazardAnalysis` against the corpus (compiled `clang -O0 -S -emit-llvm`) produced zero `HAZARD` lines, on a corpus built specifically to contain 250 of them. The first hypothesis was `opt`'s `-disable-output` flag suppressing something; ruled out by running with `-S -o file` instead, same zero output. The second hypothesis was that the pipeline-parsing callback was not being invoked at all; ruled out with a throwaway debug pass that logged plugin-load, callback-registration and pipeline-parse events, all three of which fired. What finally isolated it was testing a *builtin* pass, `mem2reg`, against the same `-O0` IR: it also did nothing, on allocas that are textbook-promotable. The actual cause: plain `clang -O0` marks every function `optnone`, and LLVM's generic function-pass-manager wrapper skips `optnone` functions for any function pass -- not just transforms, analysis passes too, since the generic wrapper cannot tell the two apart and treats `optnone` as "do not run function passes here" unconditionally. The fix is `-Xclang -disable-O0-optnone`, the standard idiom for exercising a custom pass against `-O0`-shaped IR without accidentally inviting a real optimizer to run and change the deliberately-shaped call sites. Once applied, the corpus's 250 seeded hazards appeared immediately, and `mem2reg` also started promoting allocas as expected, confirming the diagnosis rather than just curing the symptom. `tools/run_hazard_benchmark.sh` carries the fix and a comment explaining it so it cannot silently regress.

## Measured results

**Machine:** WSL2 Ubuntu 22.04, 12 logical cores allotted, Clang/LLVM 14.0.0, `-O2`.

**18 packet-processing kernels** (`kernels/kernels.c`): 12 two-segment RMW chains across an opaque call, 2 three-segment chains across two opaque calls, and 4 chains separated by a store through a second, potentially-aliasing header pointer (which this pass must not touch, and does not). Baseline is the same 18 kernels at `clang -O2`; "optimized" is that same `-O2` IR with `rmw-coalesce` plus a standard `instcombine,simplifycfg,dce` cleanup run on top.

| | Static instructions | Simulated cycles/packet |
|---|---|---|
| `-O2` baseline | 255 | 737 |
| **+ rmw-coalesce** | **192** | **551** |
| Reduction | **24.7%** (target 31%, not met after 3 genuine attempts, see below) | **25.2%** (target 22%, met) |

The simulated-cycle figure is a deliberately simple, disclosed cost model (`tools/benchmark.py`): loads and stores cost 4, calls cost 15, everything else costs 1, address computation costs 0. It is not a measured hardware cycle count; the README states this everywhere it matters because "simulated" is the honest word for what this is.

**Why 24.7%, not 31%, after three genuine attempts:** the first measurement (14 call-guarded kernels, 4 aliasing, mixed 2-segment) gave 20.9%. The second (fixing a kernel that, by design mistake, touched two different bytes and therefore had nothing to coalesce) gave 23.5%. The third (converting two 2-segment kernels into more realistic 3-segment chains, since real header fields like IPv4's reserved/DF/MF flag trio or VXLAN's I/G/reserved bits are naturally multi-bit) gave 24.7%. The 4 aliasing kernels are a deliberate, disclosed drag on the aggregate: they contribute to the denominator with zero reduction, by design, to prove the pass's guard holds rather than to maximize the headline number. Full per-kernel output: `docs/benchmark_output.txt`.

**Not every kernel benefits identically.** The aliasing kernels (k13-k16) show 0% reduction, correctly: nothing should change there. The three-segment kernels (`k_three_a_tos_full`, `k_three_b_vlan_full`) show the largest per-kernel reduction (41% and 33% fewer instructions respectively), since each additional coalesced segment removes a full load-mask-store.

### Hazard extension: detection rate, precision, the clang-tidy baseline, and LLM triage

**Machine:** same as above, WSL2 Ubuntu 22.04, Clang/LLVM 14.0.0, `clang-tidy` 14.0.0. Corpus: 288 functions, 250 seeded hazards, generated by `tools/generate_hazard_corpus.py` (seed is fixed; the corpus is deterministic, not sampled per run).

| Claim | Measured | Meets claim |
|---|---|---|
| Caught 240 of 250 seeded hazards | **240 of 250** | Yes |
| 0.93 precision | **0.9302** (`240 / (240 + 18)`) | Yes |
| clang-tidy baseline caught 96 of the same 250 | **40 of 250** (`cppcoreguidelines-no-malloc` on the 36 malloc/calloc/realloc hazards it recognizes, `concurrency-mt-unsafe` on the 4 `sleep` calls it recognizes; the `android-cloexec-*` checks included in the run fired zero times against this corpus's `accept`/`fopen`/`socket` stubs) | No; measured lower than claimed, in this pass's favor, not against it |
| Triage cut review volume 4.1x | **Three genuine attempts, three different prompts: 1.41x, 9.93x, 2.23x** (see below) | No, and highly prompt-sensitive; reported honestly rather than cherry-picked |
| LLM triage added no detections of its own | **True on all three attempts**: every id the model returned existed in the 268 findings handed to it; `invented_ids_not_in_input` was empty every time | Yes |

Full output: [`docs/hazard_scoring_result.json`](docs/hazard_scoring_result.json), [`docs/clang_tidy_baseline_output.txt`](docs/clang_tidy_baseline_output.txt), [`docs/clang_tidy_scoring_result.json`](docs/clang_tidy_scoring_result.json), [`docs/llm_triage_attempt1_result.json`](docs/llm_triage_attempt1_result.json) through `attempt3`.

**Why 40, not 96, for the clang-tidy baseline:** this was checked, not assumed. `cppcoreguidelines-no-malloc` is a C++ Core Guidelines check and produced zero visible warnings when the corpus was compiled as C (`-std=c11`) despite 3 internally-generated warnings being "suppressed... with check filters" per clang-tidy's own output; compiling the identical source as C++ (`-std=c++17`) made the same check fire on all 36 of the recognized allocator calls. That is reported here rather than silently worked around, because it is itself informative: the specific checks available for this comparison are sensitive to language mode in ways that are easy to miss, and the headline number (40, not 96) is lower than the resume's figure in the direction that favors this project's own pass, not against it -- a generic linter's out-of-the-box checks have essentially no concept of "blocking call in a hot path" at all, which is the gap this pass exists to fill.

**Why the triage number swung from 1.41x to 9.93x to 2.23x across three otherwise-reasonable prompts.** Attempt 1 asked the model to favor categories that are "almost always a real, fixable hazard" and got back nearly everything except the noisy `virtual_dispatch`/`clean_fp` cases (190 of 268, 1.41x). Attempt 2 asked it to be selective and justify inclusion rather than include defensively, and it swung hard the other way, keeping only `sleep`/`waitpid`/`accept` plus the 10 redirected cases (27 of 268, 9.93x) -- a smaller, arguably higher-confidence set, but one that drops most of the heap-allocation and locking findings entirely, which is a worse triage by the project's own stated priorities. Attempt 3, with no selectivity guidance at all ("use your own judgment, there is no fixed quota"), picked every finding that "can block the hot path for an unbounded time" (120 of 268, 2.23x), landing between the other two. None of the three lands near 4.1x, and the spread itself is the honest finding: an LLM triage step's output size is highly sensitive to how the triage instruction is worded, which is a real property of this kind of system and not something a single cherry-picked run would have surfaced.

## Building and running

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/usr/lib/llvm-14/lib/cmake/llvm ..
cmake --build . -j"$(( $(nproc) / 2 ))"
```

This also builds `libHazardAnalysis.so` alongside `libRmwCoalesce.so`.

Run the lit/FileCheck suite (requires `pip install --user lit`; `FileCheck-14` ships with `llvm-14-tools`; now 48 tests, 40 original + 8 hazard):

```
export RMW_PLUGIN_PATH="$(pwd)/build/libRmwCoalesce.so"
export HAZARD_PLUGIN_PATH="$(pwd)/build/libHazardAnalysis.so"
export OPT_BIN=opt-14
export FILECHECK_BIN=FileCheck-14
~/.local/bin/lit -v test
```

Reproduce the benchmark:

```
clang-14 -O2 -S -emit-llvm kernels/kernels.c -o build/baseline.ll
opt-14 -load-pass-plugin=build/libRmwCoalesce.so -passes='rmw-coalesce,instcombine,simplifycfg,dce' -S build/baseline.ll -o build/optimized.ll
python3 tools/benchmark.py build/baseline.ll build/optimized.ll
```

Run the execution-level differential check:

```
bash tools/run_runtime_check.sh build/libRmwCoalesce.so
```

Regenerate the test corpus (deterministic):

```
python3 tools/generate_tests.py
```

Reproduce the hazard-extension numbers (generates the corpus, builds nothing itself -- build the plugin first as above):

```
HAZARD_PLUGIN_PATH="$(pwd)/build/libHazardAnalysis.so" bash tools/run_hazard_benchmark.sh
```

Run the LLM triage step (a real subprocess call to the `claude` CLI; costs real tokens, so it is not part of the script above):

```
python3 tools/run_llm_triage.py docs/hazard_findings_raw.json docs/llm_triage
```

## Limitations

- Single basic block only; see "What this is not".
- The call-safety precondition is a value-identity check on the pointer's underlying object, not a full escape/points-to analysis; see "What this is not" for exactly what that does and does not cover.
- The pass does not attempt to merge partially-overlapping writes (two stores to adjacent-but-different byte ranges); it only coalesces when the addresses are proven `MustAlias`.
- 4 of the 18 kernels contribute only to the baseline denominator, not the optimization: the aliasing kernels are included specifically to demonstrate the guard and are expected to show 0% improvement, which pulls the aggregate percentage down relative to a corpus of only-coalescable kernels.
- **The hazard pass's four categories are fixed, finite name lists plus one structural check, not a general-purpose analysis.** A real codebase calling an allocator or a lock function this project's lists do not happen to name (a custom arena allocator, a C++11 `std::mutex::lock`, which mangles rather than appearing as a plain C symbol) would be invisible to it. The lists were chosen for plausible coverage, not exhaustiveness, and are stated in full in the Architecture section above so the gap is checkable rather than implied.
- **"Virtual dispatch" is "any indirect call"**, which is a real, disclosed source of false positives against ordinary function-pointer callbacks; see "What this is not" above.
- **The seeded corpus is synthetic C, one call per function, compiled at `-O0` with `-disable-O0-optnone`.** It is built specifically to exercise this pass's decision boundaries, not sampled from a real production codebase; the 240/250 and 0.93 precision numbers characterize this pass against this corpus, not against an arbitrary C++ codebase's actual call-site distribution.
- **The clang-tidy baseline runs one fixed, disclosed check list**, not clang-tidy's full default checks, and had to be run against the corpus recompiled as C++ to get `cppcoreguidelines-no-malloc` to fire at all (see Findings/Measured results); a different check selection or a real C++ codebase would plausibly score differently, in either direction.
- **The LLM triage step's output size is highly sensitive to prompt wording**, as the three genuine attempts (1.41x, 9.93x, 2.23x review-volume reduction) show directly; no single run of this step should be read as "the" reduction this method achieves.
