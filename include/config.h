
/*
 * File:   config.h
 * Author: egladil
 *
 * Created on 15 December 2020, 22:12
 */

#pragma once

#include <stdint.h>
#include <string>
#include <chrono>

#define SerialMon Serial
#define SerialAT  Serial1

#define TINY_GSM_MODEM_SIM800          // Modem is SIM800
#define TINY_GSM_RX_BUFFER      1024   // Set RX buffer to 1Kb

#define ARDUINOJSON_USE_LONG_LONG 1

//#define DUMP_AT_COMMANDS

namespace pin {
    namespace modem {
        constexpr const uint8_t rst = 5;
        constexpr const uint8_t powerKey = 4;
        constexpr const uint8_t powerOn = 23;
        constexpr const uint8_t tx = 27;
        constexpr const uint8_t rx = 26;
        constexpr const uint8_t dtr = 32;
        constexpr const uint8_t ri = 33;
    };

    namespace i2c {
        constexpr const uint8_t sda = 21;
        constexpr const uint8_t scl = 22;
    };

    namespace misc {
        constexpr const uint8_t reset = 0;
        constexpr const uint8_t batterySensor = 12;
        constexpr const uint8_t led = 13;
        constexpr const uint8_t sslRandom = 2;
    };
};


// Server details
namespace server {
    constexpr const char host[] = "net-box.djupfeldt.se";
    constexpr const int port = 8080;
    constexpr const char voltageDataResource[] = "/elasticsearch/mc-voltage/_doc/";
};

namespace gprs {
    constexpr const char simPIN[] = "";
    constexpr const char apn[] = "internet";
    constexpr const char user[] = "";
    constexpr const char password[] = "";
};

namespace data {
    constexpr const std::chrono::minutes collectInterval(60);
    constexpr const std::chrono::hours pushInterval(24);
}

/*
 Defined in secrets.h

namespace server {
    namespace secrets {
        constexpr const char user[] = "";
        constexpr const char password[] = "";
    }
}
 */
#include "secrets.h"
