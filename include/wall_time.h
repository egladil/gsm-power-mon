
/*
 * File:   wall_time.h
 * Author: egladil
 *
 * Created on 29 December 2020, 13:02
 */

#pragma once

#include <array>
#include <chrono>
#include <sys/time.h>

#include "storage.h"

namespace wall_time {
    namespace detail {
        constexpr const std::array<char[4], 12 > Months{"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    }

    using time_point = std::chrono::system_clock::time_point;
    //    using duration = std::chrono::system_clock::duration;

    constexpr const char CompileTime[] = __DATE__ " " __TIME__;

    inline String format(std::tm* tm) {
        char buf[64];
        buf[sizeof (buf) - 1] = 0;
        strftime(buf, sizeof (buf) - 1, "%F %T", tm);

        return buf;
    }

    inline String format(time_point tp) {
        std::time_t tt = std::chrono::system_clock::to_time_t(tp);
        return format(gmtime(&tt));
    }

    inline bool getModemTime(TinyGsm& modem, time_point& tp) {
        std::tm tm;
        float timeZone = 0;

        memset(&tm, 0, sizeof (tm));
        if (!modem.getNetworkTime(&tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &timeZone)) {
            return false;
        }

        tm.tm_year -= 1900;
        tm.tm_mon -= 1;

        SerialMon.print("Modem time: ");
        SerialMon.println(format(&tm));

        auto tzOffset = std::chrono::minutes((int) (timeZone * 60));

        tp = std::chrono::system_clock::from_time_t(std::mktime(&tm)) - tzOffset;

        return true;
    }

    bool init(TinyGsm& modem) {
        time_point tp;

        if (!getModemTime(modem, tp)) {
            return false;
        }

        timeval tv;
        tv.tv_sec = std::chrono::system_clock::to_time_t(tp);
        tv.tv_usec = 0;

        return settimeofday(&tv, nullptr) == 0;
    }

    time_point getCompileTime() {
        std::tm tm;
        memset(&tm, 0, sizeof (tm));


        tm.tm_mon = -1;
        for (int i = 0; i < detail::Months.size(); i++) {
            if (strncmp(CompileTime, detail::Months[i], 3) == 0) {
                tm.tm_mon = i;
            }
        }

        if (tm.tm_mon < 0) {
            return {};
        }

        if (sscanf(CompileTime + 4, "%i %i %i:%i:%i", &tm.tm_mday, &tm.tm_year, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) != 5) {
            return {};
        }

        tm.tm_year -= 1900;

        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }

    template<typename TDuration>
    void addDelay(time_point& tp, TDuration duration, time_point def) {
        if (tp.time_since_epoch().count() == 0) {
            tp = def;
        }

        tp += duration;
    }
}
