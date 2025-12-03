#ifndef DEBUG_HH
#define DEBUG_HH

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace global {

enum class LogLevel { DEBUG, INFO, WARNING, EXCEPTION, FATAL };

inline std::mutex& log_mutex() { static std::mutex m; return m; }

struct LoggerState {
    std::ofstream out;
    LogLevel threshold = LogLevel::DEBUG;
    bool initialized = false;
};

inline LoggerState& logger_state() { static LoggerState s; return s; }

inline void init_log(const std::string& filepath, bool append = false) {
    std::lock_guard<std::mutex> lk(log_mutex());
    auto& s = logger_state();
    if (s.out.is_open()) s.out.close();
    s.out.open(filepath, append ? std::ios::app : std::ios::trunc);
    s.initialized = s.out.good();
}

inline void set_log_level(LogLevel lv) {
    std::lock_guard<std::mutex> lk(log_mutex());
    logger_state().threshold = lv;
}

inline std::string now_string() {
    using namespace std::chrono;
    auto tp = system_clock::now();
    auto t = system_clock::to_time_t(tp);
    auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

inline const char* level_name(LogLevel lv) {
    switch (lv) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::EXCEPTION: return "ERROR";
        default: return "FATAL";
    }
}

inline bool level_enabled(LogLevel msg, LogLevel thr) {
    return static_cast<int>(msg) >= static_cast<int>(thr);
}

inline void log(LogLevel lv, const std::string& msg) {
    std::lock_guard<std::mutex> lk(log_mutex());
    auto& s = logger_state();
    if (!s.initialized) return;
    if (!level_enabled(lv, s.threshold)) return;
    s.out << '[' << now_string() << "] " << level_name(lv) << ": " << msg << '\n';
    s.out.flush();
}

inline void log_debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
inline void log_info(const std::string& msg) { log(LogLevel::INFO, msg); }
inline void log_warning(const std::string& msg) { log(LogLevel::WARNING, msg); }
inline void log_exception(const std::string& msg) { log(LogLevel::EXCEPTION, msg); }
inline void log_fatal(const std::string& msg) { log(LogLevel::FATAL, msg); }

}

#endif

