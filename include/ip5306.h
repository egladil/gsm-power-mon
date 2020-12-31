
/*
 * File:   ip5306.h
 * Author: egladil
 *
 * Created on 15 December 2020, 22:12
 */

#pragma once

#include <Wire.h>

#include <stdint.h>

class IP5306 {
private:
    constexpr static uint8_t addr = 0x75;
    constexpr static uint8_t reg_sys_ctl0 = 0x00;
    constexpr static uint8_t reg_sys_ctl1 = 0x01;
    constexpr static uint8_t reg_sys_ctl2 = 0x02;
    constexpr static uint8_t reg_charger_ctl0 = 0x20;
    constexpr static uint8_t reg_charger_ctl1 = 0x21;
    constexpr static uint8_t reg_charger_ctl2 = 0x22;
    constexpr static uint8_t reg_charger_ctl3 = 0x23;
    constexpr static uint8_t reg_dig_ctl0 = 0x24;
    constexpr static uint8_t reg_read0 = 0x70;
    constexpr static uint8_t reg_read1 = 0x71;
    constexpr static uint8_t reg_read2 = 0x72;
    constexpr static uint8_t reg_read3 = 0x77;
    constexpr static uint8_t reg_read4 = 0x78;

    struct SysCtl0 {
        constexpr static uint8_t ButtonShutdownEnable = 1 << 0;
        constexpr static uint8_t BoostOutputNormallyOpenFunction = 1 << 1;
        constexpr static uint8_t PluginLoadAutomaticPoweron = 1 << 2;
        constexpr static uint8_t ChargerEnable = 1 << 4;
        constexpr static uint8_t BoostEnable = 1 << 5;
    };

    TwoWire& wire;

    bool read(uint8_t reg, uint8_t& value) {
        wire.beginTransmission(addr);
        wire.write(reg_read4);

        if (wire.endTransmission(false) == 0 && wire.requestFrom(addr, 1u)) {
            value = wire.read();
            return true;
        } else {
            value = 0;
            return false;
        }
    }

    bool write(uint8_t reg, uint8_t value) {
        wire.beginTransmission(addr);
        wire.write(reg);
        wire.write(value);
        return wire.endTransmission() == 0;
    }

public:

    explicit IP5306(TwoWire& wire) : wire(wire) {
    }

    bool setup() {
        constexpr uint8_t value = SysCtl0::ButtonShutdownEnable | SysCtl0::BoostOutputNormallyOpenFunction | SysCtl0::PluginLoadAutomaticPoweron | SysCtl0::ChargerEnable | SysCtl0::BoostEnable;
        return write(reg_sys_ctl0, value);
    }

    int8_t getBatteryLevel() {
        uint8_t value;
        if (!read(reg_read4, value)) {
            return -1;
        }

        switch (value & 0xF0) {
            case 0xE0: return 25;
            case 0xC0: return 50;
            case 0x80: return 75;
            case 0x00: return 100;
            default: return 0;
        }
    }
};
