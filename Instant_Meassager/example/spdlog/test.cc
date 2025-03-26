#include<spdlog/spdlog.h>
#include<spdlog/sinks/stdout_color_sinks.h>
#include<spdlog/sinks/basic_file_sink.h>
#include<spdlog/async.h>


int main()
{

    spdlog::flush_every(std::chrono::seconds(1));
    spdlog::flush_on(spdlog::level::level_enum::debug);

    auto logger = spdlog::basic_logger_mt("file-logger", "sync.log");
    logger->set_pattern("[%n][%H:%M:%S][%t][%-8l] %v");

    logger->trace("你好！{}", "小明");
    logger->debug("你好！{}", "小明");
    logger->info("你好！{}", "小明");
    logger->warn("你好！{}", "小明");
    logger->error("你好！{}", "小明");
    logger->critical("你好！{}", "小明");
    return 0;
}