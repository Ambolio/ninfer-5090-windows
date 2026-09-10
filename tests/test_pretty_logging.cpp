#include "product/logging/logging.h"
#include "product/logging/startup_log.h"

#include <spdlog/logger.h>

#if defined(_WIN32)
// MSVC has no <unistd.h>; the stderr capture below uses the CRT file-descriptor
// equivalents. The capture pipe is binary and stderr is put in binary mode for the
// capture (restored to its default text mode on exit): the assertions compare the
// log text exactly. _getmode no longer exists in modern UCRT, so the default
// _O_TEXT is restored explicitly.
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <stdio.h>

namespace {

inline int capture_pipe(int fd[2]) { return _pipe(fd, 0, _O_BINARY); }
inline int capture_dup(int fd) { return _dup(fd); }
inline int capture_dup2(int from, int to) { return _dup2(from, to); }
inline int capture_close(int fd) { return _close(fd); }
inline int capture_read(int fd, void* buffer, unsigned int count) { return _read(fd, buffer, count); }

const int kStderrFd = _fileno(stderr);

} // namespace
#else
#include <unistd.h>

namespace {

inline int capture_pipe(int fd[2]) { return ::pipe(fd); }
inline int capture_dup(int fd) { return ::dup(fd); }
inline int capture_dup2(int from, int to) { return ::dup2(from, to); }
inline int capture_close(int fd) { return ::close(fd); }
inline int capture_read(int fd, void* buffer, std::size_t count) { return ::read(fd, buffer, count); }

constexpr int kStderrFd = STDERR_FILENO;

} // namespace
#endif

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>

namespace {

class StderrCapture {
public:
    StderrCapture() {
#if defined(_WIN32)
        (void)_setmode(kStderrFd, _O_BINARY);
#endif
        if (capture_pipe(pipe_) != 0) { throw std::runtime_error(std::strerror(errno)); }
        saved_ = capture_dup(kStderrFd);
        if (saved_ < 0 || capture_dup2(pipe_[1], kStderrFd) < 0) {
            throw std::runtime_error(std::strerror(errno));
        }
        capture_close(pipe_[1]);
        pipe_[1] = -1;
    }

    ~StderrCapture() {
        if (saved_ >= 0) {
            (void)capture_dup2(saved_, kStderrFd);
            capture_close(saved_);
        }
        if (pipe_[0] >= 0) { capture_close(pipe_[0]); }
#if defined(_WIN32)
        (void)_setmode(kStderrFd, _O_TEXT);
#endif
    }

    std::string finish() {
        std::fflush(stderr);
        if (capture_dup2(saved_, kStderrFd) < 0) {
            throw std::runtime_error(std::strerror(errno));
        }
        capture_close(saved_);
        saved_ = -1;

        std::string output;
        std::array<char, 4096> buffer{};
        for (;;) {
            const int count =
                capture_read(pipe_[0], buffer.data(), static_cast<unsigned int>(buffer.size()));
            if (count <= 0) {
                if (count < 0 && errno == EINTR) { continue; }
                if (count < 0) { throw std::runtime_error(std::strerror(errno)); }
                break;
            }
            output.append(buffer.data(), static_cast<std::size_t>(count));
        }
        capture_close(pipe_[0]);
        pipe_[0] = -1;
        return output;
    }

private:
    int pipe_[2]{-1, -1};
    int saved_ = -1;
};

int check(bool condition, const char* message) {
    if (condition) { return 0; }
    std::cerr << message << '\n';
    return 1;
}

std::size_t line_count(std::string_view value) {
    return static_cast<std::size_t>(std::count(value.begin(), value.end(), '\n'));
}

} // namespace

int main() {
    int failures = 0;
    std::string service_output;
    {
        StderrCapture capture;
        {
            ninfer::product::LoggingRuntime logging(
                {.logger_name  = "ninfer-serve",
                 .color        = ninfer::product::LogColorMode::Auto,
                 .presentation = ninfer::product::LogPresentation::Service});
            logging.logger()->info("throughput | sample");
            logging.flush();
        }
        service_output = capture.finish();
    }
    failures += check(
        std::regex_match(
            service_output,
            std::regex(
                R"(^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3}  INFO  throughput \| sample\n$)")),
        "service pretty prefix mismatch");
    failures += check(service_output.find("ninfer-serve") == std::string::npos,
                      "service pretty output repeated the executable name");
    failures += check(service_output.find("\x1b[") == std::string::npos,
                      "redirected service output contains ANSI escapes");

    std::string startup_output;
    {
        StderrCapture capture;
        {
            ninfer::product::LoggingRuntime logging(
                {.logger_name  = "ninfer-serve",
                 .color        = ninfer::product::LogColorMode::Never,
                 .presentation = ninfer::product::LogPresentation::Service});
            ninfer::product::StartupLogRenderer startup(logging);
            ninfer::StartupObserver observer = startup.observer();
            observer.callback({.phase  = ninfer::StartupPhase::EngineStartup,
                               .status = ninfer::StartupStatus::Begin});
            observer.callback({.phase  = ninfer::StartupPhase::CudaInitialize,
                               .status = ninfer::StartupStatus::Begin});
            observer.callback({.phase      = ninfer::StartupPhase::CudaInitialize,
                               .status     = ninfer::StartupStatus::Complete,
                               .elapsed_ns = 1'000'000'000});
            observer.callback({.phase         = ninfer::StartupPhase::WeightsMaterialize,
                               .status        = ninfer::StartupStatus::Begin,
                               .progress_unit = ninfer::StartupProgressUnit::Bytes,
                               .total         = 16ULL << 30});
            observer.callback({.phase         = ninfer::StartupPhase::WeightsMaterialize,
                               .status        = ninfer::StartupStatus::Complete,
                               .progress_unit = ninfer::StartupProgressUnit::Bytes,
                               .current       = 16ULL << 30,
                               .total         = 16ULL << 30,
                               .elapsed_ns    = 2'000'000'000});
            observer.callback({.phase      = ninfer::StartupPhase::EngineStartup,
                               .status     = ninfer::StartupStatus::Complete,
                               .elapsed_ns = 3'000'000'000});
            startup.engine_ready({.model_id             = "qwen3.6-27b",
                                  .weights_id           = "groupwise-int",
                                  .host_to_device_bytes = 16ULL << 30});
            logging.flush();
        }
        startup_output = capture.finish();
    }
    failures +=
        check(line_count(startup_output) == 4 &&
                  startup_output.find("starting engine") != std::string::npos &&
                  startup_output.find("loading weights | 16.0 GiB") != std::string::npos &&
                  startup_output.find("weights ready | 16.0 GiB | 2.0s | 8.00 GiB/s") !=
                      std::string::npos &&
                  startup_output.find("engine ready | qwen3.6-27b/groupwise-int | total 3.0s") !=
                      std::string::npos &&
                  startup_output.find("CUDA initialized") == std::string::npos,
              "normal startup pretty output is noisy or incomplete");

    std::string tool_output;
    {
        StderrCapture capture;
        {
            ninfer::product::LoggingRuntime logging(
                {.logger_name  = "ninfer",
                 .color        = ninfer::product::LogColorMode::Never,
                 .presentation = ninfer::product::LogPresentation::Tool});
            logging.logger()->info("engine ready");
            logging.logger()->error("failed");
            logging.flush();
        }
        tool_output = capture.finish();
    }
    failures +=
        check(tool_output == "engine ready\nerror: failed\n", "tool pretty prefix mismatch");
    return failures == 0 ? 0 : 1;
}
