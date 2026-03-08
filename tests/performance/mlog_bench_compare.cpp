#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <misc_log_ex.h>

INITIALIZE_EASYLOGGINGPP

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bench"

namespace
{
struct peer_tag
{
  std::uint64_t high;
  std::uint64_t low;
};

std::ostream& operator<<(std::ostream& out, const peer_tag& value)
{
  out << std::hex << value.high << ':' << value.low << std::dec;
  return out;
}

struct metrics
{
  std::size_t iterations;
  double seconds;
  double ns_per_op;
};

void configure_logging()
{
  el::Configurations config;
  config.setToDefault();
  config.setGlobally(el::ConfigurationType::Enabled, "true");
  config.setGlobally(el::ConfigurationType::ToFile, "true");
  config.setGlobally(el::ConfigurationType::Filename, "/dev/null");
  config.setGlobally(el::ConfigurationType::ToStandardOutput, "false");
  config.setGlobally(el::ConfigurationType::Format, "%msg");
  config.setGlobally(el::ConfigurationType::MaxLogFileSize, "1099511627776");
  el::Loggers::setDefaultConfigurations(config, true);

  el::Loggers::addFlag(el::LoggingFlag::HierarchicalLogging);
  el::Loggers::addFlag(el::LoggingFlag::CreateLoggerAutomatically);
  el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);
}

#ifdef MLOG_BENCH_FORMAT
#define BENCH_LOG(iteration, amount, label, tag) \
  MINFO("height {} amount {} label {} tag {}", iteration, amount, label, tag)
#else
#define BENCH_LOG(iteration, amount, label, tag) \
  MINFO("height " << iteration << " amount " << amount << " label " << label << " tag " << tag)
#endif

template <typename Fn>
metrics run_case(const char* categories, Fn&& fn)
{
  el::Loggers::setCategories(categories, true);

  std::size_t iterations = 1u << 12;
  double elapsed = 0.0;
  for (;;)
  {
    const auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < iterations; ++i)
      fn(i);
    el::Loggers::flushAll();
    const auto end = std::chrono::steady_clock::now();
    elapsed = std::chrono::duration<double>(end - start).count();
    if (elapsed >= 2.5 || iterations >= (1u << 26))
      break;
    iterations *= 2;
  }

  std::array<double, 5> samples{};
  for (double& sample : samples)
  {
    const auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < iterations; ++i)
      fn(i);
    el::Loggers::flushAll();
    const auto end = std::chrono::steady_clock::now();
    sample = std::chrono::duration<double>(end - start).count();
  }

  std::sort(samples.begin(), samples.end());
  const double median = samples[samples.size() / 2];
  return {iterations, median, (median * 1e9) / static_cast<double>(iterations)};
}

} // anonymous namespace

int main()
{
  configure_logging();

  std::uint64_t state = 0x123456789abcdef0ULL;
  peer_tag tag{0x1122334455667788ULL, 0xaabbccddeeff0011ULL};
  const std::string label = "outbound-peer";

  const auto bench_fn = [&](std::size_t i) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    const std::uint64_t amount = state ^ static_cast<std::uint64_t>(i);
    BENCH_LOG(i, amount, label, tag);
  };

  const metrics enabled = run_case("*:INFO", bench_fn);
  const metrics disabled = run_case("*:WARNING", bench_fn);

  std::cout << std::fixed << std::setprecision(3);
  std::cout << "enabled_iterations=" << enabled.iterations << '\n';
  std::cout << "enabled_seconds=" << enabled.seconds << '\n';
  std::cout << "enabled_ns_per_op=" << enabled.ns_per_op << '\n';
  std::cout << "disabled_iterations=" << disabled.iterations << '\n';
  std::cout << "disabled_seconds=" << disabled.seconds << '\n';
  std::cout << "disabled_ns_per_op=" << disabled.ns_per_op << '\n';
  std::cout << "final_state=" << state << '\n';

  return 0;
}
