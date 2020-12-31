
/*
 * File:   storage.h
 * Author: egladil
 *
 * Created on 29 December 2020, 11:32
 */

#pragma once

#include <EEPROM.h>

#include <stdint.h>
#include <array>
#include <chrono>

#include "config.h"

//#pragma pack(push,1)

struct VoltageData {
    std::chrono::system_clock::time_point timestamp;
    float external;
    uint16_t externalRaw;
    int8_t internalPercent;
};

struct PermanentStorage {

    struct detail {
        constexpr static size_t MaxStorageSize = 0x4000;
        constexpr static uint32_t Magic = 0x4248a16d + 4;
        constexpr static size_t VoltageDataSize = 128;
    };

    uint32_t magic = 0;

    bool clockInitialized = false;

    std::chrono::system_clock::time_point nextCollectTime;
    std::chrono::system_clock::time_point nextPushTime;

    std::array<VoltageData, detail::VoltageDataSize> voltageData;
    uint8_t voltageDataCount = 0;

    bool setup() {
        constexpr const size_t size = sizeof (PermanentStorage);
        static_assert(size <= detail::MaxStorageSize, "Permanent storage too large");

        constexpr const size_t neededVDSize = (size_t) (std::chrono::duration_cast<std::chrono::seconds>(data::pushInterval).count() / std::chrono::duration_cast<std::chrono::seconds>(data::collectInterval).count());
        static_assert(detail::VoltageDataSize > neededVDSize, "Not enough voltage data storage");

        if (!EEPROM.begin(size)) {
            return false;
        }


        read();
        if (magic == detail::Magic) {
            return true;
        }

        *this = PermanentStorage();
        magic = detail::Magic;
        return write();
    }

    void read() {
        EEPROM.get(0, *this);
    }

    bool write() {
        EEPROM.put(0, *this);
        return EEPROM.commit();
    }
};

//#pragma pack(pop)
