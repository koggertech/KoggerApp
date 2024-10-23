#ifndef MAVLINKCONF_H
#define MAVLINKCONF_H
#include "stdint.h"
#include "math.h"

const uint8_t MAVLinkCRCExtra[] = {50, 124, 137, 0, 237, // keep lines, lines are indexes
217,
104,
119,
117,
0,
0,      // id 10
89,
0,
0,
0,
0,
0,
0,
0,
0,
214,    // id 20
159,
220,
168,
24,
23,
170,
144,
67,
115,
39,    // id 30
246,
185,
104,
237,
244,
222,
212,
9,
254,
230,     // id 40
28,
28,
132,
221,
232,
11,
153,
41,
39,
78,     // id 50
196,
0,
0,
15,
3,
0,
0,
0,
0,
0,     // id 60
153,
183,
51,
59,
118,
148,
21,
0,
243,
124,    // id 70
0,
0,
38,
20,
158,
152,
143,
0,
0,
14,     // id 80
106,
49,
22,
143,
140,
5,
150,
0,
231,
183,    // id 90
63,
54,
47,
0,
0,
0,
0,
0,
0,
175,    // id 100
102,
158,
208,
56,
93,
138,
108,
32,
185,
84,    // id 110
34,
174,
124,
237,
4,
76,
128,
56,
116,
134,    // id 120
237,
203,
250,
87,
203,
220,
25,
226,
46,
29,    // id 130
223,
85,
6,
229,
203,
1,
195,
109,
168,
181,    // id 140
47,
72,
131,
127,
0,
103,
154,
178,
200,
134,    // id 150
219,
208,
188,
84,
22,
19,
21,
134,
0,
78,      // id 160
68,
189,
127,    // name="AHRS"
154,
21,     // name="HWSTATUS"
21,
144,
1,
234,
73,      // id 170
181,
22,
83,
167,
138,
234,
240,
47,     // name="AHRS2"
189,
52,     // id 180
174,
229,    // name="AHRS3"
85,
159,
186,
72,
0,
0,
0,
0,      // id 190
92,
36,
71,      // name="EKF_STATUS_REPORT"
98,
120,
0,
0,
0,
0,
134,    // id 200
205,
0,
0,
0,
0,
0,
0,
0,
0,
0,    // id 210
0,
0,
0,
69,
101,
50,
202,
17,
162,
34,    // id 220
71,
15,
0,
0,
208,
207,
0,
0,
0,
163,    // id 230
105,
151,
35,
150,
179,
0,
0,
0,
0,
0,    // id 240
90,
104,
85,
95,
130,
184,
81,
8,
204,
49,    // id 250
170,
44,
83,
46,
0,
71,
131,
187,
92,
146,
179,
12,
133,
49  ,
26,
193,
35,
14,
109,
59,
22,
0,
0,
0,
126,
18,
0,
0,
0,
70,
48,
123,
74,
99,
137,
210,
1,
20,
0,
251,
10,
0,
0,
0,
0,
0,
0,
0,
19,
217,    // id 300
243,
0,
0,
0,
0,
0,
0,
0,
0,
28,
95,
0,
0,
0,
0,
0,
0,
0,
0,
243,
88,
243,
78,
132,
0,
0,
0,
0,
0,
23,
91,
236,
231,
72,
225,
245,
0,
0,
199,
99,
0,
0,
0,
0,
0,
0,
0,
0,
0,
232,    // id 350
0,
0,
0,
0,
0,
0,
0,
0,
0,
11,
0,
0,
0,
0,
0,
0,
0,
0,
0,
75,
0,
0,
117,
0,
251,
0,
0,
0,
0,
232,
0,
0,
0,
0,
147,
132,
4,
8,
0,
156,
0,
0,
0,
0,
0,
0,
182,
0,
0,
110,    // id 400
183,
0,
0,
0,
0,
0,
0,
0,
0,
160,
106,
33,
77,
};

typedef struct {uint16_t id; uint8_t crc;} MAVLinkCRCExtra2Type;
const MAVLinkCRCExtra2Type MAVLinkCRCExtra2[] = {
    {9000, 113},
    {9005, 117},
    {10001, 209},
    {10002, 186},
    {10003, 4},
    {11000, 134},
    {11001, 15},
    {11002, 234},
    {11003, 64},
    {11010, 46},
    {11011, 106},
    {11020, 205},
    {11030, 144},
    {11031, 133},
    {11032, 85},
    {11033, 195},
    {11034, 79},
    {11035, 128},
    {11036, 177},
    {11037, 130},
    {11038, 47},
    {11039, 142},
    {12900, 114},
    {12901, 254},
    {12902, 140},
    {12903, 249},
    {12904, 77},
    {12905, 49},
    {12915, 94},
    {12918, 139},
    {12919, 7},
    {12920, 20},
    {42000, 227},
    {42001, 239},
    {50001, 246},
    {50002, 181},
    {50003, 62},
    {50004, 240},
    {50005, 152}
};

inline uint8_t getMAVLinkExtra(uint32_t msgid) {
    if(msgid < sizeof(MAVLinkCRCExtra)) {
        return MAVLinkCRCExtra[msgid];
    } else {
        const uint16_t cnt_extra2 = sizeof(MAVLinkCRCExtra2)/sizeof(MAVLinkCRCExtra2[0]);
        for(uint16_t i = 0; i < cnt_extra2; i++) {
            if(MAVLinkCRCExtra2[i].id == msgid) {
                return MAVLinkCRCExtra2[i].crc;
            }
        }
    }

    return 0;
}


typedef struct   __attribute__((packed)) {
    uint32_t time_boot_ms; // Timestamp (time since system boot), ms
    int32_t	lat; // Latitude, expressed, degE7
    int32_t	 lon; // Longitude, expressed, degE7
    int32_t	alt; // mm
    int32_t relative_alt; // mm
    int16_t vx; // cm/s
    int16_t vy; // cm/s
    int16_t vz; // cm/s
    uint16_t hdg; // cdeg

    static uint32_t getID() {
        return 33;
    }

    double latitude() {
        return double(lat)/1.0e7;
    }

    double longitude() {
        return double(lon)/1.0e7;
    }

    uint32_t time_boot_msec() {
        return time_boot_ms;
    }

    bool isValid() {
        return lat != 0 || lon != 0;
    }

    float velocityX() {
        return float(vx)*0.01f;
    }

    float velocityY() {
        return float(vy)*0.01f;
    }

    float velocityZ() {
        return float(vz)*0.01f;
    }

    float velocityH() {
        return sqrtf(velocityX()*velocityX() + velocityY()*velocityY());
    }

} MAVLink_MSG_GLOBAL_POSITION_INT;

typedef struct   __attribute__((packed)) {
    uint64_t time_usec; // Timestamp (UNIX Epoch time or time since system boot).

    int32_t	lat; // Latitude, expressed, degE7
    int32_t	 lon; // Longitude, expressed, degE7
    int32_t	alt; // mm // Altitude (MSL). Positive for up. Note that virtually all GPS modules provide the MSL altitude in addition to the WGS84 altitude.
    uint16_t eph = UINT16_MAX; // GPS HDOP horizontal dilution of position (unitless * 100). If unknown, set to: UINT16_MAX
    uint16_t epv = UINT16_MAX; // GPS VDOP vertical dilution of position (unitless * 100). If unknown, set to: UINT16_MAX
    uint16_t vel = UINT16_MAX; // cm/s // GPS ground speed. If unknown, set to: UINT16_MAX
    uint16_t cog = UINT16_MAX; // Course over ground (NOT heading, but direction of movement) in degrees * 100, 0.0..359.99 degrees. If unknown, set to: UINT16_MAX
    uint8_t fix_type;
    uint8_t satellites_visible = UINT8_MAX; // Number of satellites visible. If unknown, set to UINT8_MAX

    double latitude() {
        return double(lat)/1.0e7;
    }

    double longitude() {
        return double(lon)/1.0e7;
    }

    uint32_t time_boot_msec() {
        return time_usec;
    }

    bool isValid() {
        return lat != 0 || lon != 0;
    }

    float velocityH() {
        if(vel == UINT16_MAX) { return NAN; }
        return float(vel)*0.01f;
    }

} MAVLink_MSG_GPS_RAW_INT;


typedef struct   __attribute__((packed)) {
    float airspeed; // m/s
    float groundspeed; // m/s
    int16_t heading; // deg
    uint16_t throttle; // %
    uint16_t alt; // m
    float climb; // m/s
} MAVLink_MSG_VFR_HUD;



typedef struct   __attribute__((packed)) {
    typedef enum {
        Manual = 0,
        Acro,
        Steering,
        Hold,
        Loiter,
        Follow,
        Simple,
        Dock,
        Circle,
        Auto,
        RTL,
        SmartRTL,
        Guided
    } MAVLink_CustomMode;

//    union {
//        MAVLink_CustomMode custom_mode_enum;
//        uint32_t custom_mode;
//    };
    uint32_t custom_mode;

    uint8_t type;
    uint8_t autopilot;
    uint8_t base_mode;
    uint8_t system_status;
    uint8_t mavlink_version;

    bool isArmed() { return (base_mode & 128) != 0; }
    bool isRemoteControl() { return (base_mode & 64) != 0; }
    bool isCustomMode() { return (base_mode & 1) != 0; }
    uint32_t customMode() { return custom_mode; }

} MAVLink_MSG_HEARTBEAT;

typedef struct   __attribute__((packed)) {
    uint32_t onboard_control_sensors_present; // MAV_SYS_STATUS_SENSOR	Bitmap showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present.
    uint32_t onboard_control_sensors_enabled; // MAV_SYS_STATUS_SENSOR	Bitmap showing which onboard controllers and sensors are enabled: Value of 0: not enabled. Value of 1: enabled.
    uint32_t onboard_control_sensors_health; // MAV_SYS_STATUS_SENSOR	Bitmap showing which onboard controllers and sensors have an error (or are operational). Value of 0: error. Value of 1: healthy.
    uint16_t load; // d%		Maximum usage in percent of the mainloop time. Values: [0-1000] - should always be below 1000
    uint16_t voltage_battery; // mV		Battery voltage, UINT16_MAX: Voltage not sent by autopilot
    int16_t current_battery; // cA		Battery current, -1: Current not sent by autopilot
    int8_t battery_remaining; // %		Battery energy remaining, -1: Battery remaining energy not sent by autopilot
    uint16_t drop_rate_comm; // c%		Communication drop rate, (UART, I2C, SPI, CAN), dropped packets on all links (packets that were corrupted on reception on the MAV)
    uint16_t errors_comm; //				Communication errors (UART, I2C, SPI, CAN), dropped packets on all links (packets that were corrupted on reception on the MAV)

    float batteryVoltage() {
        if(voltage_battery == UINT16_MAX) {
            return 0;
        } else {
            return 0.001f*voltage_battery;
        }
    }

    float batteryCurrent() {
        if(current_battery == -1) {
            return 0;
        } else {
            return 0.001f*current_battery;
        }
    }
} MAVLink_MSG_SYS_STATUS;

typedef struct   __attribute__((packed)) {
    int32_t current_consumed; /*< [mAh] Consumed charge, -1: autopilot does not provide consumption estimate*/
     int32_t energy_consumed; /*< [hJ] Consumed energy, -1: autopilot does not provide energy consumption estimate*/
     int16_t temperature; /*< [cdegC] Temperature of the battery. INT16_MAX for unknown temperature.*/
     uint16_t voltages[10]; /*< [mV] Battery voltage of cells 1 to 10 (see voltages_ext for cells 11-14). Cells in this field above the valid cell count for this battery should have the UINT16_MAX value. If individual cell voltages are unknown or not measured for this battery, then the overall battery voltage should be filled in cell 0, with all others set to UINT16_MAX. If the voltage of the battery is greater than (UINT16_MAX - 1), then cell 0 should be set to (UINT16_MAX - 1), and cell 1 to the remaining voltage. This can be extended to multiple cells if the total voltage is greater than 2 * (UINT16_MAX - 1).*/
     int16_t current_battery; /*< [cA] Battery current, -1: autopilot does not measure the current*/
     uint8_t id; /*<  Battery ID*/
     uint8_t battery_function; /*<  Function of the battery*/
     uint8_t type; /*<  Type (chemistry) of the battery*/
     int8_t battery_remaining; /*< [%] Remaining battery energy. Values: [0-100], -1: autopilot does not estimate the remaining battery.*/
     int32_t time_remaining; /*< [s] Remaining battery time, 0: autopilot does not provide remaining battery time estimate*/
     uint8_t charge_state; /*<  State for extent of discharge, provided by autopilot for warning or external reactions*/
     uint16_t voltages_ext[4]; /*< [mV] Battery voltages for cells 11 to 14. Cells above the valid cell count for this battery should have a value of 0, where zero indicates not supported (note, this is different than for the voltages field and allows empty byte truncation). If the measured value is 0 then 1 should be sent instead.*/
     uint8_t mode; /*<  Battery mode. Default (0) is that battery mode reporting is not supported or battery is in normal-use mode.*/
     uint32_t fault_bitmask; /*<  Fault/health indications. These should be set when charge_state is MAV_BATTERY_CHARGE_STATE_FAILED or MAV_BATTERY_CHARGE_STATE_UNHEALTHY (if not, fault reporting is not supported).*/

    float voltage() {
        float voltage = 0;
        for(uint16_t i = 0; i < 10; i++) {
            if(voltages[i] != UINT16_MAX) {
                voltage += voltages[i];
            } else {
                break;
            }
        }
        if(voltages[0] == UINT16_MAX) {
            return 0;
        } else {
            return 0.001f*voltage;
        }
    }

    float current() {
        if(current_battery == -1) {
            return 0;
        } else {
            return 0.01f*current_battery;
        }
    }
} MAVLink_MSG_BATTERY_STATUS;


typedef struct __attribute__((packed)) {
 uint32_t time_boot_ms; /*< [ms] Timestamp (time since system boot).*/
 float roll; /*< [rad] Roll angle (-pi..+pi)*/
 float pitch; /*< [rad] Pitch angle (-pi..+pi)*/
 float yaw; /*< [rad] Yaw angle (-pi..+pi)*/
 float rollspeed; /*< [rad/s] Roll angular speed*/
 float pitchspeed; /*< [rad/s] Pitch angular speed*/
 float yawspeed; /*< [rad/s] Yaw angular speed*/

 float yawDeg() { return yaw*57.2957795;}
 float pitchDeg() { return pitch*57.2957795;}
 float rollDeg() { return roll*57.2957795;}

} MAVLink_MSG_ATTITUDE;








#endif // MAVLINKCONF_H
