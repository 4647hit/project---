#pragma once
#include <iostream>
#include <time.h>
#include <pthread.h>
#include <cstdio>
#include <jsoncpp/json/json.h>
#include <memory>
#include <sstream>
#include <string>
#include <chrono>
#include <random>
#include <atomic>
#include <iomanip>

namespace RPC
{
#define DEBUG 1
#define INFO 0
#define ERROR 2
#define DEFAULTLEVEL 0
    bool _is_save = false;
    pthread_mutex_t glock = PTHREAD_MUTEX_INITIALIZER;
    const char *path_name = "./Log.txt";
#define LOG(issave, level, format, ...)                                                                                 \
    {                                                                                                                   \
        if (level >= DEFAULTLEVEL)                                                                                      \
        {                                                                                                               \
            time_t _time = time(NULL);                                                                                  \
            struct tm *time_ptr = localtime(&_time);                                                                    \
            char timeblock[32] = {0};                                                                                   \
            strftime(timeblock, sizeof(timeblock), "%Y-%m-%d %T", time_ptr);                                            \
            if (issave)                                                                                                 \
            {                                                                                                           \
                FILE *fptr = fopen(path_name, "a+");                                                                    \
                if (fptr)                                                                                               \
                {                                                                                                       \
                    fprintf(fptr, "[%s][file: %s-line: %d]" format "\n", timeblock, __FILE__, __LINE__, ##__VA_ARGS__); \
                    fclose(fptr);                                                                                       \
                }                                                                                                       \
            }                                                                                                           \
            else                                                                                                        \
            {                                                                                                           \
                pthread_mutex_lock(&glock);                                                                             \
                printf("[%s][file: %s-line: %d]:" format "\n", timeblock, __FILE__, __LINE__, ##__VA_ARGS__);           \
                pthread_mutex_unlock(&glock);                                                                           \
            }                                                                                                           \
        }                                                                                                               \
    }
#define EnSave_Code()    \
    {                    \
        _is_save = true; \
    }
#define UnSave_Code()     \
    {                     \
        _is_save = false; \
    }
#define ILOG(format, ...) LOG(_is_save, INFO, format, ##__VA_ARGS__)
#define DLOG(format, ...) LOG(_is_save, DEBUG, format, ##__VA_ARGS__)
#define ELOG(format, ...) LOG(_is_save, ERROR, format, ##__VA_ARGS__)
    class JsonTool
    {
    public:
         static bool serialize(const Json::Value &val,  std::string &body) {
                std::stringstream ss;
                //先实例化一个工厂类对象
                Json::StreamWriterBuilder swb;
                //通过工厂类对象来生产派生类对象
                std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
                int ret = sw->write(val, &ss);
                if (ret != 0) {
                    ELOG("json serialize failed!");
                    return false;
                }
                body = ss.str();
                return true;
            }

            static bool unserialize( Json::Value &val,const std::string &body) {
                Json::CharReaderBuilder crb;
                std::string errs;
                std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
                bool ret = cr->parse(body.c_str(), body.c_str() + body.size(), &val, &errs);
                if (ret == false) {
                    ELOG("json unserialize failed : %s", errs.c_str());
                    return false;
                }
                return true;
            }
    };
    class UUID
    {
    public:
        static std::string uuid()
        {
            std::stringstream ss;
            // 1、构造随机数种子
            std::random_device rdm;
            // 2 、以该种子为构造随机数对象
            std::mt19937 generator(rdm());
            // 3、构造指定范围内的数
            std::uniform_int_distribution<int> distribution(0, 255);
            // 4、构成16进制数的字符串
            for (int i = 0; i < 8; i++)
            {
                if (i == 4 | i == 6)
                    ss << "-";
                ss << std::setw(2) << std::setfill('0') << std::hex << distribution(generator);
            }
            ss << "-";
            // 5、定义一个8字节的序号，逐字节组织成为16进制数字字符的字符串
            static std::atomic<size_t> seq(1);
            size_t cur = seq.fetch_add(1);
            for (int i = 7; i >= 0; i--)
            {
                if (i == 5)
                    ss << "-";
                ss << std::setw(2) << std::setfill('0') << std::hex << ((cur >> (8 * i)) & 0xFF);
            }
            return ss.str();
        }
    };
}