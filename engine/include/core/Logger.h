#pragma once

#include <cstdio>
#include <cstdarg>

// ============================================================================
// 轻量宏日志系统 (Logger)
// ============================================================================
// 设计铁律：所有调试、日志、警告、错误信息必须且只能输出到 stderr。
// stdout 是引擎与 Node.js 网关之间的神圣 NDJSON 数据通道，绝对不允许被污染。
//
// 使用方式：
//   LOG_INFO("World genesis complete. Map: %dx%d", width, height);
//   LOG_WARN("Entity %d stepped out of biome bounds", entity_id);
//   LOG_ERROR("Cannot open map file: %s", path.c_str());
// ============================================================================

namespace fsb {
namespace core {
namespace logger {

// 日志级别枚举 (用于未来扩展过滤等级)
enum class LogLevel {
    INFO,
    WARN,
    ERROR
};

// 级别标签的人类可读映射
inline const char* levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default:              return "???";
    }
}

// 核心输出函数：格式化后写入 stderr，附带源文件与行号定位
inline void log(LogLevel level, const char* file, int line, const char* fmt, ...) {
    // 提取文件名（剥离完整路径，只保留 basename，节省 stderr 带宽）
    const char* basename = file;
    for (const char* p = file; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            basename = p + 1;
        }
    }

    // 写入级别标签与源码定位
    fprintf(stderr, "[%s] [%s:%d] ", levelToString(level), basename, line);

    // 写入用户格式化内容
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    // 换行并强制冲刷，防止 stderr 缓冲导致日志延迟
    fprintf(stderr, "\n");
    fflush(stderr);
}

} // namespace logger
} // namespace core
} // namespace fsb

// ============================================================================
// 便捷宏：自动注入 __FILE__ 与 __LINE__，调用者无需关心
// ============================================================================
#define LOG_INFO(fmt, ...)  fsb::core::logger::log(fsb::core::logger::LogLevel::INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  fsb::core::logger::log(fsb::core::logger::LogLevel::WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fsb::core::logger::log(fsb::core::logger::LogLevel::ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
