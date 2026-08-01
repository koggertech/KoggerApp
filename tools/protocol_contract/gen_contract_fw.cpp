// Emits the KP wire contract as seen by the FIRMWARE, in the same JSON shape as
// gen_contract.cpp emits it for the app.
//
// This is the independent source that makes layout verification real. gen_contract.cpp
// reads src/id_binnary.h, so on its own it can only prove the simulator agrees with
// the app. Compiling the firmware's own PayloadDefines.h gives a second, unrelated
// account of the same wire, and `kpdevtool.py contract-diff` compares them.
//
// Source: EmbedCode/USBL-agent/io/Parser/{FrameParser.hpp,PayloadDefines.h} -- the USBL
// firmware repo itself. (The Bootloader repo vendors a byte-identical copy of the same
// io submodule; the agent repo is the one to point at, because it also carries the
// ParceUSBLControl dispatch that decides which version means which struct.)
// Build:  tools/protocol_contract/gen.ps1 -Firmware

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <type_traits>

#include "FrameParser.hpp"
#include "PayloadDefines.h"

namespace {

template <typename T>
struct Tag {
    static constexpr const char* kind() {
        if constexpr (std::is_enum_v<T>)                return "u";
        else if constexpr (std::is_same_v<T, bool>)      return "u";
        else if constexpr (std::is_floating_point_v<T>)  return "f";
        else if constexpr (std::is_signed_v<T>)          return "i";
        else                                             return "u";
    }
    static constexpr std::size_t esize() { return sizeof(T); }
    static constexpr int count() { return 1; }
};

template <typename T, std::size_t N>
struct Tag<T[N]> {
    static constexpr const char* kind() { return Tag<T>::kind(); }
    static constexpr std::size_t esize() { return sizeof(T); }
    static constexpr int count() { return (int)N; }
};

bool g_firstStruct = true, g_firstField = true, g_firstEnum = true, g_firstVal = true;

void structBegin(const char* name, std::size_t size, const char* id, int ver,
                 const char* dir) {
    printf("%s\n    \"%s\": { \"size\": %zu, \"id\": \"%s\", \"ver\": %d, "
           "\"dir\": \"%s\", \"fields\": [",
           g_firstStruct ? "" : ",", name, size, id, ver, dir);
    g_firstStruct = false;
    g_firstField = true;
}
void structEnd() { printf("\n    ] }"); }

void field(const char* name, std::size_t off, std::size_t esize, int count,
           const char* kind) {
    printf("%s\n      { \"name\": \"%s\", \"offset\": %zu, \"size\": %zu, "
           "\"count\": %d, \"kind\": \"%s\" }",
           g_firstField ? "" : ",", name, off, esize, count, kind);
    g_firstField = false;
}

void enumBegin(const char* name) {
    printf("%s\n    \"%s\": {", g_firstEnum ? "" : ",", name);
    g_firstEnum = false;
    g_firstVal = true;
}
void enumEnd() { printf(" }"); }

void val(const char* name, long long v) {
    printf("%s \"%s\": %lld", g_firstVal ? "" : ",", name, v);
    g_firstVal = false;
}

} // namespace

#define FIELD(S, f)                                                          \
    do {                                                                     \
        using FT = decltype(S::f);                                           \
        field(#f, offsetof(S, f), Tag<FT>::esize(), Tag<FT>::count(),         \
              Tag<FT>::kind());                                              \
    } while (0)

// Every struct here self-reports its ID and version, so the diff can align
// app-vs-firmware by (ID, version, dir) rather than by name -- the two sides renamed
// several structs without changing the wire, and a request payload must never be
// aligned against a reply payload that happens to share a version.
#define STRUCT(S, DIR) \
    structBegin(#S, sizeof(S), idName(S::getId()), (int)S::getVer(), DIR)

static const char* idName(CMD_ID_e id) {
    switch (id) {
        case ID_USBL_SOLUTION:  return "ID_USBL_SOLUTION";
        case ID_MODEM_SOLUTION: return "ID_MODEM_SOLUTION";
        case ID_USBL_CONTROL:   return "ID_USBL_CONTROL";
        case ID_MODEM_CONTROL:  return "ID_MODEM_CONTROL";
        case ID_NAV_DIAG:       return "ID_NAV_DIAG";
        default:                return "ID_OTHER";
    }
}

int main() {
    printf("{\n  \"_generated_by\": \"tools/protocol_contract/gen_contract_fw.cpp\",\n");
    printf("  \"_source\": \"EmbedCode/USBL-agent/io/Parser/PayloadDefines.h\",\n");
    printf("  \"_side\": \"firmware\",\n");
    // Emitted as a hand-picked subset on each side, so absence proves nothing about
    // them -- only a value disagreement does.
    printf("  \"_partial_enums\": [\"ID\", \"Type\", \"Version\", \"BoardVersion\"],\n");
    printf("  \"structs\": {");

    STRUCT(UsblSolution, "content");
    FIELD(UsblSolution, id);
    FIELD(UsblSolution, role);
    FIELD(UsblSolution, cmd_id);
    FIELD(UsblSolution, reserved);
    FIELD(UsblSolution, timestamp_us);
    FIELD(UsblSolution, ping_counter);
    FIELD(UsblSolution, carrier_counter);
    FIELD(UsblSolution, distance_m);
    FIELD(UsblSolution, distance_unc);
    FIELD(UsblSolution, azimuth_deg);
    FIELD(UsblSolution, azimuth_unc);
    FIELD(UsblSolution, elevation_deg);
    FIELD(UsblSolution, elevation_unc);
    FIELD(UsblSolution, snr);
    FIELD(UsblSolution, beacon_x_m);
    FIELD(UsblSolution, beacon_y_m);
    FIELD(UsblSolution, beacon_latitude);
    FIELD(UsblSolution, beacon_longitude);
    FIELD(UsblSolution, beacon_depth);
    FIELD(UsblSolution, usbl_yaw);
    FIELD(UsblSolution, usbl_pitch);
    FIELD(UsblSolution, usbl_roll);
    FIELD(UsblSolution, usbl_latitude);
    FIELD(UsblSolution, usbl_longitude);
    FIELD(UsblSolution, last_iTOW);
    FIELD(UsblSolution, beacon_n_m);
    FIELD(UsblSolution, beacon_e_m);
    FIELD(UsblSolution, code_snr);
    structEnd();

    STRUCT(AcousticNavSolution, "content");
    FIELD(AcousticNavSolution, address);
    FIELD(AcousticNavSolution, cmd_id);
    FIELD(AcousticNavSolution, reserved);
    FIELD(AcousticNavSolution, timestamp_us);
    FIELD(AcousticNavSolution, carrier_us);
    FIELD(AcousticNavSolution, carrier_counter);
    FIELD(AcousticNavSolution, lat);
    FIELD(AcousticNavSolution, lon);
    FIELD(AcousticNavSolution, depth);
    FIELD(AcousticNavSolution, acousticAzimuth);
    FIELD(AcousticNavSolution, geoAzimuth);
    FIELD(AcousticNavSolution, heading);
    FIELD(AcousticNavSolution, distance);
    FIELD(AcousticNavSolution, baseLat);
    FIELD(AcousticNavSolution, baseLon);
    FIELD(AcousticNavSolution, baseDepth);
    structEnd();

    STRUCT(BaseToBeacon, "content");
    FIELD(BaseToBeacon, address);
    FIELD(BaseToBeacon, cmd_id);
    FIELD(BaseToBeacon, reserved);
    FIELD(BaseToBeacon, timestamp_us);
    FIELD(BaseToBeacon, carrier_us);
    FIELD(BaseToBeacon, carrier_counter);
    FIELD(BaseToBeacon, acousticAzimuth);
    FIELD(BaseToBeacon, geoAzimuth);
    FIELD(BaseToBeacon, beaconDistance);
    FIELD(BaseToBeacon, beaconN);
    FIELD(BaseToBeacon, beaconE);
    FIELD(BaseToBeacon, BeaconD);
    FIELD(BaseToBeacon, beaconLat);
    FIELD(BaseToBeacon, beaconLon);
    FIELD(BaseToBeacon, antennaYaw);
    FIELD(BaseToBeacon, antennaDepth);
    FIELD(BaseToBeacon, antennaLat);
    FIELD(BaseToBeacon, antennaLon);
    structEnd();

    STRUCT(USBLTriggerControl, "request");
    FIELD(USBLTriggerControl, txTriggerSource);
    FIELD(USBLTriggerControl, triggerActive);
    FIELD(USBLTriggerControl, clockDriftStatus);
    FIELD(USBLTriggerControl, reserved);
    FIELD(USBLTriggerControl, period_us);
    FIELD(USBLTriggerControl, offset_us);
    structEnd();

    STRUCT(USBLPingRequest, "request");
    FIELD(USBLPingRequest, trigger_timeout_us);
    FIELD(USBLPingRequest, address);
    FIELD(USBLPingRequest, cmd_id);
    FIELD(USBLPingRequest, reply_distance_mm);
    FIELD(USBLPingRequest, function);
    FIELD(USBLPingRequest, payload_bit_length);
    structEnd();

    STRUCT(USBLPingAddresses, "request");
    FIELD(USBLPingAddresses, cur_position);
    FIELD(USBLPingAddresses, max_position);
    FIELD(USBLPingAddresses, address);
    structEnd();

    STRUCT(USBLTransponderEnable, "request");
    FIELD(USBLTransponderEnable, timeout_us);
    structEnd();

    STRUCT(USBLRequestAddressFilter, "request");
    FIELD(USBLRequestAddressFilter, address);
    structEnd();

    STRUCT(USBLResponseAddress, "request");
    FIELD(USBLResponseAddress, address);
    structEnd();

    STRUCT(USBLCmdConfig, "request");
    FIELD(USBLCmdConfig, cmd_id);
    FIELD(USBLCmdConfig, eventFilter);
    FIELD(USBLCmdConfig, cmdIdAction);
    FIELD(USBLCmdConfig, cmd_id_replacement);
    FIELD(USBLCmdConfig, addressAction);
    FIELD(USBLCmdConfig, address_replacement);
    FIELD(USBLCmdConfig, eventAction);
    FIELD(USBLCmdConfig, reserved1);
    FIELD(USBLCmdConfig, reserved2);
    FIELD(USBLCmdConfig, receiver_function);
    FIELD(USBLCmdConfig, receive_bit_length);
    FIELD(USBLCmdConfig, sender_function);
    FIELD(USBLCmdConfig, sending_bit_length);
    structEnd();

    STRUCT(USBLMonitorConfig, "request");
    FIELD(USBLMonitorConfig, suppressSelfResponse_us);
    FIELD(USBLMonitorConfig, suppressSelfRequest_us);
    FIELD(USBLMonitorConfig, receiveResponseInIdle);
    structEnd();

    STRUCT(ModemSolutionHeader, "content");
    FIELD(ModemSolutionHeader, timestamp_us);
    FIELD(ModemSolutionHeader, carrier_us);
    FIELD(ModemSolutionHeader, carrier_counter);
    FIELD(ModemSolutionHeader, reserved1);
    FIELD(ModemSolutionHeader, event);
    FIELD(ModemSolutionHeader, address_from);
    FIELD(ModemSolutionHeader, address_to);
    FIELD(ModemSolutionHeader, slot_id);
    FIELD(ModemSolutionHeader, payload_bit_length);
    structEnd();

    STRUCT(NavDiagnostics, "content");
    FIELD(NavDiagnostics, timestamp_us);
    FIELD(NavDiagnostics, last_iTOW_ms);
    FIELD(NavDiagnostics, gnss_age_ms);
    FIELD(NavDiagnostics, pps_age_ms);
    FIELD(NavDiagnostics, ekf_yaw_cdeg);
    FIELD(NavDiagnostics, ekf_pitch_cdeg);
    FIELD(NavDiagnostics, ekf_roll_cdeg);
    FIELD(NavDiagnostics, ekf_pos_n_mm);
    FIELD(NavDiagnostics, ekf_pos_e_mm);
    FIELD(NavDiagnostics, ekf_pos_d_mm);
    FIELD(NavDiagnostics, ekf_vel_n_cmps);
    FIELD(NavDiagnostics, ekf_vel_e_cmps);
    FIELD(NavDiagnostics, ekf_pos_horiz_std_cm);
    FIELD(NavDiagnostics, ekf_pos_vert_std_cm);
    FIELD(NavDiagnostics, ekf_vel_horiz_std_mmps);
    FIELD(NavDiagnostics, ekf_yaw_std_cdeg);
    FIELD(NavDiagnostics, ekf_control_status);
    FIELD(NavDiagnostics, ekf_fault_status);
    FIELD(NavDiagnostics, imu_acc_norm_mmps2);
    FIELD(NavDiagnostics, imu_gyr_norm_cdps);
    FIELD(NavDiagnostics, imu_temp_cc);
    FIELD(NavDiagnostics, imu_error_flags);
    FIELD(NavDiagnostics, imu_clip_mask);
    FIELD(NavDiagnostics, imu_sensor_id);
    FIELD(NavDiagnostics, gnss_lat_1e7);
    FIELD(NavDiagnostics, gnss_lon_1e7);
    FIELD(NavDiagnostics, gnss_alt_mm);
    FIELD(NavDiagnostics, gnss_hacc_mm);
    FIELD(NavDiagnostics, gnss_fix_type);
    FIELD(NavDiagnostics, gnss_nsats);
    FIELD(NavDiagnostics, gnss_yaw_cdeg);
    FIELD(NavDiagnostics, gnss_yaw_acc_cdeg);
    FIELD(NavDiagnostics, gnss_pdop_x10);
    FIELD(NavDiagnostics, gnss_flags);
    FIELD(NavDiagnostics, sys_flags);
    FIELD(NavDiagnostics, reserved);
    structEnd();

    printf("\n  },\n  \"enums\": {");

    enumBegin("USBLPingRequest::Function");
    val("FunctionDefault", USBLPingRequest::FunctionDefault);
    val("FunctionBitArray", USBLPingRequest::FunctionBitArray);
    val("FunctionLLGeoAzimuth", USBLPingRequest::FunctionLLGeoAzimuth);
    enumEnd();

    enumBegin("USBLCmdConfig::Function");
    val("FunctionDefault", USBLCmdConfig::FunctionDefault);
    val("FunctionBitArray", USBLCmdConfig::FunctionBitArray);
    val("FunctionLLGeoAzimuth", USBLCmdConfig::FunctionLLGeoAzimuth);
    enumEnd();

    enumBegin("USBLCmdConfig::EventFilter");
    val("EventOnReceiveRequest", USBLCmdConfig::EventOnReceiveRequest);
    val("EventOnReceiveResponse", USBLCmdConfig::EventOnReceiveResponse);
    enumEnd();

    enumBegin("USBLCmdConfig::SendBackCmdIdAction");
    val("SendBackCmdIdIncoming", USBLCmdConfig::SendBackCmdIdIncoming);
    val("SendBackCmdIdReplacement", USBLCmdConfig::SendBackCmdIdReplacement);
    enumEnd();

    enumBegin("USBLCmdConfig::SendBackAddressAction");
    val("SendBackAddressIncoming", USBLCmdConfig::SendBackAddressIncoming);
    val("SendBackAddressReplacement", USBLCmdConfig::SendBackAddressReplacement);
    enumEnd();

    enumBegin("USBLCmdConfig::SendBackEventAction");
    val("SendBackEventSwaping", USBLCmdConfig::SendBackEventSwaping);
    val("SendBackEventSame", USBLCmdConfig::SendBackEventSame);
    enumEnd();

    enumBegin("USBLTriggerControl::TxTriggerSource");
    val("TxTriggerSourceCmdOnly", USBLTriggerControl::TxTriggerSourceCmdOnly);
    val("TxTriggerSourcePin", USBLTriggerControl::TxTriggerSourcePin);
    val("TxTriggerSourceBreak", USBLTriggerControl::TxTriggerSourceBreak);
    val("TxTriggerSourcePPS", USBLTriggerControl::TxTriggerSourcePPS);
    val("TxTriggerSourceRegular", USBLTriggerControl::TxTriggerSourceRegular);
    enumEnd();

    enumBegin("ModemSolutionHeader::EventFilter");
    val("EventOnRequest", ModemSolutionHeader::EventOnRequest);
    val("EventOnResponse", ModemSolutionHeader::EventOnResponse);
    enumEnd();

    enumBegin("ID");
    val("ID_VERSION", ID_VERSION);
    val("ID_MARK", ID_MARK);
    val("ID_NAV", ID_NAV);
    val("ID_USBL_SOLUTION", ID_USBL_SOLUTION);
    val("ID_MODEM_SOLUTION", ID_MODEM_SOLUTION);
    val("ID_RESERVED", ID_RESERVED);
    val("ID_USBL_CONTROL", ID_USBL_CONTROL);
    val("ID_MODEM_CONTROL", ID_MODEM_CONTROL);
    val("ID_NAV_DIAG", ID_NAV_DIAG);
    enumEnd();

    printf("\n  }\n}\n");
    return 0;
}
