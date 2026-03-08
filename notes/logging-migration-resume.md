# Logging Migration Resume ## Goal Continue the Monero logging migration from stream-style 
`<<` logging to format-based logging while keeping `mlog` and `easylogging++`. Primary goals: 
- remove remaining `<<`-style logging dependencies from the codebase - keep logging 
correctness first - keep `mlog` - keep `easylogging++` - preserve runtime log 
categories/behavior - measure whether the migration is worth finishing ## Current status A 
large part of the migration is already done. Completed so far: - `misc_log_ex.h` was 
redesigned toward format-based logging - direct `MINFO/MERROR/...` formatting support exists - 
many direct logging and assert callsites were converted from `<<` to `{}` format style - 
benchmark code was added - benchmark comparing clean `master` vs current patch was already run 
Important: - `master` and current branch started from the same commit:
  - `ea9be68fb66f66605d857e1ee13e92fb1d50423b` - benchmark comparison was done against a clean 
exported tree at:
  - `/tmp/monero-master-bench` ## Benchmark file Benchmark source: - 
`/Users/nvr/dev/monero/tests/performance/mlog_bench_compare.cpp` It benchmarks: - enabled 
`MINFO(...)` hot path - disabled logging hot path - clean `master` vs current modified tree - 
multi-second runs with median sampling ## Benchmark build commands used Current tree: ```bash 
env SDKROOT="$(xcrun --sdk macosx --show-sdk-path)" /usr/bin/clang++ -std=c++17 -O3 -DNDEBUG 
-DMLOG_BENCH_FORMAT=1 \
  -I/Users/nvr/dev/monero/contrib/epee/include \ 
  -I/Users/nvr/dev/monero/external/easylogging++ \ -I/Users/nvr/dev/monero \ 
  /Users/nvr/dev/monero/tests/performance/mlog_bench_compare.cpp \ 
  /Users/nvr/dev/monero/external/easylogging++/easylogging++.cc \ -o /tmp/mlog_bench_current
