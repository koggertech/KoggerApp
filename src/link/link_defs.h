#pragma once

#include <stdint.h>
#include <QList>


enum class LinkType {
    kLinkNone = 0,
    kLinkSerial,
    kLinkIPUDP, // also is proxy
    kLinkIPTCP,
};

enum class LinkAttribute {
    kLinkAttributeNone = 0,
    kLinkAttributeBoot = 0xFFFF
};

enum class ControlType {
    kManual = 0,
    kAuto,
    kAutoOnce
};


static const int linkNumTimeoutsBig = 10; // num of updates
static const int linkNumTimeoutsSmall = 2; // num of updates
static const int ghostIgnoreCount = 3; // number of steps to ignore after flashing
static const int requestAllCntBig = 3; // number of steps to requestAll big
static const int requestAllCntSmall = 1; // number of steps to requestAll small
static const int linkCheckingTimeInterval = 100; // msecs

inline const QList<uint32_t> baudrates = { 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1200000, 2000000, 4000000, 5000000, 8000000, 10000000 };
inline const QList<uint32_t> baudrateSearchList = { 115200, 921600, 9600, 38400, 115200, 921600, 460800, 230400 };
