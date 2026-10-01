#pragma once

#include <cstdint>

#include "mav_link_conf.h"

constexpr uint8_t kMavCompIdAutopilot1 = 1;
constexpr uint8_t kMavGcsSystemId = 255;
constexpr uint8_t kMavGcsComponentId = 190;
constexpr uint8_t kMavModeFlagCustomModeEnabled = 1;
constexpr float kMavArmForceMagic = 21196.0f;

enum MavCommandId : uint16_t {
    MavCmdDoSetMode = 176,
    MavCmdMissionStart = 300,
    MavCmdComponentArmDisarm = 400,
    MavCmdRequestMessage = 512
};

enum MavResult : int {
    MavResultNotSent = -1,
    MavResultAccepted = 0,
    MavResultTemporarilyRejected = 1,
    MavResultDenied = 2,
    MavResultUnsupported = 3,
    MavResultFailed = 4,
    MavResultInProgress = 5,
    MavResultCancelled = 6
};

enum MavMissionType : uint8_t {
    MavMissionTypeMission = 0,
    MavMissionTypeFence = 1,
    MavMissionTypeRally = 2
};

enum MavMissionResult : int {
    MavMissionAccepted = 0,
    MavMissionError = 1,
    MavMissionUnsupportedFrame = 2,
    MavMissionUnsupported = 3,
    MavMissionNoSpace = 4,
    MavMissionInvalid = 5,
    MavMissionInvalidParam1 = 6,
    MavMissionInvalidParam7 = 12,
    MavMissionInvalidSequence = 13,
    MavMissionDenied = 14,
    MavMissionOperationCancelled = 15
};

struct __attribute__((packed)) MAVLink_MSG_COMMAND_LONG
{
    float param1 = 0.0f;
    float param2 = 0.0f;
    float param3 = 0.0f;
    float param4 = 0.0f;
    float param5 = 0.0f;
    float param6 = 0.0f;
    float param7 = 0.0f;
    uint16_t command = 0;
    uint8_t target_system = 0;
    uint8_t target_component = 0;
    uint8_t confirmation = 0;

    static uint32_t getID() { return 76; }
    static int v1Length() { return 33; }
};

struct __attribute__((packed)) MAVLink_MSG_COMMAND_ACK
{
    uint16_t command = 0;
    uint8_t result = 0;
    uint8_t progress = 0;
    int32_t result_param2 = 0;
    uint8_t target_system = 0;
    uint8_t target_component = 0;

    static uint32_t getID() { return 77; }
};

struct __attribute__((packed)) MAVLink_MSG_MISSION_REQUEST
{
    uint16_t seq = 0;
    uint8_t target_system = 0;
    uint8_t target_component = 0;
    uint8_t mission_type = 0;

    static uint32_t getID() { return 40; }
};

struct __attribute__((packed)) MAVLink_MSG_RADIO_STATUS
{
    uint16_t rxerrors = 0;
    uint16_t fixed = 0;
    uint8_t rssi = 0;
    uint8_t remrssi = 0;
    uint8_t txbuf = 0;
    uint8_t noise = 0;
    uint8_t remnoise = 0;

    static uint32_t getID() { return 109; }
};

struct __attribute__((packed)) MAVLink_MSG_HOME_POSITION
{
    int32_t latitude = 0;
    int32_t longitude = 0;
    int32_t altitude = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float q[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float approach_x = 0.0f;
    float approach_y = 0.0f;
    float approach_z = 0.0f;
    uint64_t time_usec = 0;

    static uint32_t getID() { return 242; }
};

struct __attribute__((packed)) MAVLink_MSG_MISSION_REQUEST_LIST
{
    uint8_t target_system = 0;
    uint8_t target_component = 0;
    uint8_t mission_type = 0;

    static uint32_t getID() { return 43; }
    static int v1Length() { return 2; }
};

struct __attribute__((packed)) MAVLink_MSG_MISSION_COUNT
{
    uint16_t count = 0;
    uint8_t target_system = 0;
    uint8_t target_component = 0;
    uint8_t mission_type = 0;

    static uint32_t getID() { return 44; }
    static int v1Length() { return 4; }
};

struct __attribute__((packed)) MAVLink_MSG_MISSION_ACK
{
    uint8_t target_system = 0;
    uint8_t target_component = 0;
    uint8_t type = 0;
    uint8_t mission_type = 0;
    uint32_t opaque_id = 0;

    static uint32_t getID() { return 47; }
    static int v1Length() { return 3; }
};

struct __attribute__((packed)) MAVLink_MSG_MISSION_REQUEST_INT
{
    uint16_t seq = 0;
    uint8_t target_system = 0;
    uint8_t target_component = 0;
    uint8_t mission_type = 0;

    static uint32_t getID() { return 51; }
    static int v1Length() { return 4; }
};

struct __attribute__((packed)) MAVLink_MSG_MISSION_ITEM_INT
{
    float param1 = 0.0f;
    float param2 = 0.0f;
    float param3 = 0.0f;
    float param4 = 0.0f;
    int32_t x = 0;
    int32_t y = 0;
    float z = 0.0f;
    uint16_t seq = 0;
    uint16_t command = 0;
    uint8_t target_system = 0;
    uint8_t target_component = 0;
    uint8_t frame = 0;
    uint8_t current = 0;
    uint8_t autocontinue = 1;
    uint8_t mission_type = 0;

    static uint32_t getID() { return 73; }
    static int v1Length() { return 37; }
};

static_assert(sizeof(MAVLink_MSG_COMMAND_LONG) == 33);
static_assert(sizeof(MAVLink_MSG_RADIO_STATUS) == 9);
static_assert(sizeof(MAVLink_MSG_MISSION_REQUEST_LIST) == 3);
static_assert(sizeof(MAVLink_MSG_MISSION_COUNT) == 5);
static_assert(sizeof(MAVLink_MSG_MISSION_REQUEST_INT) == 5);
static_assert(sizeof(MAVLink_MSG_MISSION_ACK) == 8);
static_assert(sizeof(MAVLink_MSG_MISSION_ITEM_INT) == 38);
