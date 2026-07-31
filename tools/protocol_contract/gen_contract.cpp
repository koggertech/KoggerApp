// Emits the KP wire contract as JSON, straight from the compiler.
//
// Struct sizes, field offsets and enum values are the compiler's numbers, never
// hand-derived — three of my hand-derived sizes were wrong the last time this was
// done by hand, and __attribute__((packed)) makes the mistakes invisible.
//
// Consumed by kptools/kpdev. The generated JSON is committed, so a change to
// id_binnary.h shows up as a reviewable diff in the contract rather than
// propagating silently into the simulator.
//
// Build (no Qt link needed — nothing here is instantiated):
//   clang++ -std=c++23 -fsyntax-only ... no; see tools/protocol_contract/build.ps1

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <type_traits>

#include "id_binnary.h"

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

bool g_firstStruct = true;
bool g_firstField = true;
bool g_firstEnum = true;
bool g_firstVal = true;

void structBegin(const char* name, std::size_t size, const char* id, int ver,
                 const char* dir) {
    printf("%s\n    \"%s\": { \"size\": %zu, \"id\": \"%s\", \"ver\": %d, "
           "\"dir\": \"%s\", \"fields\": [",
           g_firstStruct ? "" : ",", name, size, id, ver, dir);
    g_firstStruct = false;
    g_firstField = true;
}

void structEnd() {
    printf("\n    ] }");
}

void field(const char* name, std::size_t off, std::size_t esize, int count, const char* kind) {
    printf("%s\n      { \"name\": \"%s\", \"offset\": %zu, \"size\": %zu, \"count\": %d, \"kind\": \"%s\" }",
           g_firstField ? "" : ",", name, off, esize, count, kind);
    g_firstField = false;
}

void enumBegin(const char* name) {
    printf("%s\n    \"%s\": {", g_firstEnum ? "" : ",", name);
    g_firstEnum = false;
    g_firstVal = true;
}

void enumEnd() {
    printf(" }");
}

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

// dir: "content" = device -> host payload, "request" = host -> device payload.
#define STRUCT(S, ID, VER, DIR) structBegin(#S, sizeof(S), ID, VER, DIR)

using Usbl = IDBinUsblSolution;
using Ctl  = IDBinUsblControl;
using Modem = IDBinModemSolution;

int main() {
    printf("{\n  \"_generated_by\": \"tools/protocol_contract/gen_contract.cpp\",\n");
    printf("  \"_source\": \"src/id_binnary.h + src/proto_binnary.h\",\n");
    printf("  \"_note\": \"Compiler-emitted sizes/offsets, derived from the APP headers: this "
           "pins app-vs-simulator agreement on usage, NOT app-vs-firmware layout.\",\n");

    // ---- structs -----------------------------------------------------------
    // These four are emitted as a hand-picked subset on each side, so a member being
    // absent proves nothing about them -- only a value disagreement does.
    printf("  \"_partial_enums\": [\"ID\", \"Type\", \"Version\", \"BoardVersion\"],\n");
    printf("  \"structs\": {");

    STRUCT(Usbl::UsblSolution, "ID_USBL_SOLUTION", 0, "content");
    FIELD(Usbl::UsblSolution, id);
    FIELD(Usbl::UsblSolution, role);
    FIELD(Usbl::UsblSolution, cmd_id);
    FIELD(Usbl::UsblSolution, reserved);
    FIELD(Usbl::UsblSolution, timestamp_us);
    FIELD(Usbl::UsblSolution, ping_counter);
    FIELD(Usbl::UsblSolution, carrier_counter);
    FIELD(Usbl::UsblSolution, distance_m);
    FIELD(Usbl::UsblSolution, distance_unc);
    FIELD(Usbl::UsblSolution, azimuth_deg);
    FIELD(Usbl::UsblSolution, azimuth_unc);
    FIELD(Usbl::UsblSolution, elevation_deg);
    FIELD(Usbl::UsblSolution, elevation_unc);
    FIELD(Usbl::UsblSolution, snr);
    FIELD(Usbl::UsblSolution, beacon_x_m);
    FIELD(Usbl::UsblSolution, beacon_y_m);
    FIELD(Usbl::UsblSolution, beacon_latitude);
    FIELD(Usbl::UsblSolution, beacon_longitude);
    FIELD(Usbl::UsblSolution, beacon_depth);
    FIELD(Usbl::UsblSolution, usbl_yaw);
    FIELD(Usbl::UsblSolution, usbl_pitch);
    FIELD(Usbl::UsblSolution, usbl_roll);
    FIELD(Usbl::UsblSolution, usbl_latitude);
    FIELD(Usbl::UsblSolution, usbl_longitude);
    FIELD(Usbl::UsblSolution, last_iTOW);
    FIELD(Usbl::UsblSolution, beacon_n_m);
    FIELD(Usbl::UsblSolution, beacon_e_m);
    FIELD(Usbl::UsblSolution, code_snr);
    structEnd();

    STRUCT(Usbl::AcousticNavSolution, "ID_USBL_SOLUTION", 1, "content");
    FIELD(Usbl::AcousticNavSolution, address);
    FIELD(Usbl::AcousticNavSolution, cmd_id);
    FIELD(Usbl::AcousticNavSolution, reserved);
    FIELD(Usbl::AcousticNavSolution, timestamp_us);
    FIELD(Usbl::AcousticNavSolution, carrier_us);
    FIELD(Usbl::AcousticNavSolution, carrier_counter);
    FIELD(Usbl::AcousticNavSolution, lat);
    FIELD(Usbl::AcousticNavSolution, lon);
    FIELD(Usbl::AcousticNavSolution, depth);
    FIELD(Usbl::AcousticNavSolution, acousticAzimuth);
    FIELD(Usbl::AcousticNavSolution, geoAzimuth);
    FIELD(Usbl::AcousticNavSolution, heading);
    FIELD(Usbl::AcousticNavSolution, distance);
    FIELD(Usbl::AcousticNavSolution, baseLat);
    FIELD(Usbl::AcousticNavSolution, baseLon);
    FIELD(Usbl::AcousticNavSolution, baseDepth);
    structEnd();

    STRUCT(Usbl::BaseToBeacon, "ID_USBL_SOLUTION", 2, "content");
    FIELD(Usbl::BaseToBeacon, address);
    FIELD(Usbl::BaseToBeacon, cmd_id);
    FIELD(Usbl::BaseToBeacon, reserved);
    FIELD(Usbl::BaseToBeacon, timestamp_us);
    FIELD(Usbl::BaseToBeacon, carrier_us);
    FIELD(Usbl::BaseToBeacon, carrier_counter);
    FIELD(Usbl::BaseToBeacon, acousticAzimuth);
    FIELD(Usbl::BaseToBeacon, geoAzimuth);
    FIELD(Usbl::BaseToBeacon, beaconDistance);
    FIELD(Usbl::BaseToBeacon, beaconN);
    FIELD(Usbl::BaseToBeacon, beaconE);
    FIELD(Usbl::BaseToBeacon, beaconD);
    FIELD(Usbl::BaseToBeacon, beaconLat);
    FIELD(Usbl::BaseToBeacon, beaconLon);
    FIELD(Usbl::BaseToBeacon, antennaYaw);
    FIELD(Usbl::BaseToBeacon, antennaDepth);
    FIELD(Usbl::BaseToBeacon, antennaLat);
    FIELD(Usbl::BaseToBeacon, antennaLon);
    structEnd();

    STRUCT(Usbl::USBLRequestBeacon, "ID_USBL_SOLUTION", 0, "request");
    FIELD(Usbl::USBLRequestBeacon, id);
    FIELD(Usbl::USBLRequestBeacon, reserved);
    FIELD(Usbl::USBLRequestBeacon, watermark);
    FIELD(Usbl::USBLRequestBeacon, latitude_deg);
    FIELD(Usbl::USBLRequestBeacon, longitude_deg);
    FIELD(Usbl::USBLRequestBeacon, external_heading_deg);
    FIELD(Usbl::USBLRequestBeacon, force_beacon_depth_m);
    FIELD(Usbl::USBLRequestBeacon, external_pitch);
    FIELD(Usbl::USBLRequestBeacon, external_roll);
    structEnd();

    STRUCT(Usbl::BeaconActivationResponce, "ID_USBL_SOLUTION", 1, "content");
    FIELD(Usbl::BeaconActivationResponce, id);
    FIELD(Usbl::BeaconActivationResponce, reserved);
    FIELD(Usbl::BeaconActivationResponce, reserved1);
    structEnd();

    STRUCT(Usbl::BeaconActivate, "ID_USBL_SOLUTION", 1, "request");
    FIELD(Usbl::BeaconActivate, timeout_s);
    structEnd();

    STRUCT(Ctl::USBLPingRequest, "ID_USBL_CONTROL", 1, "request");
    FIELD(Ctl::USBLPingRequest, trigger_timeout_us);
    FIELD(Ctl::USBLPingRequest, address);
    FIELD(Ctl::USBLPingRequest, cmd_id);
    FIELD(Ctl::USBLPingRequest, reply_distance_mm);
    FIELD(Ctl::USBLPingRequest, function);
    FIELD(Ctl::USBLPingRequest, payload_bit_length);
    structEnd();

    STRUCT(Ctl::USBLPingAddresses, "ID_USBL_CONTROL", 2, "request");
    FIELD(Ctl::USBLPingAddresses, cur_position);
    FIELD(Ctl::USBLPingAddresses, max_position);
    FIELD(Ctl::USBLPingAddresses, address);
    structEnd();

    STRUCT(Ctl::USBLResponseTimeout, "ID_USBL_CONTROL", 3, "request");
    FIELD(Ctl::USBLResponseTimeout, timeout_us);
    structEnd();

    STRUCT(Ctl::USBLMonitorConfig, "ID_USBL_CONTROL", 7, "request");
    FIELD(Ctl::USBLMonitorConfig, suppressSelfResponse_us);
    FIELD(Ctl::USBLMonitorConfig, suppressSelfRequest_us);
    FIELD(Ctl::USBLMonitorConfig, receiveResponseInIdle);
    structEnd();

    STRUCT(Ctl::USBLResponseAddressFilter, "ID_USBL_CONTROL", 4, "request");
    FIELD(Ctl::USBLResponseAddressFilter, address);
    structEnd();

    STRUCT(Ctl::USBLCmdSlotConfig, "ID_USBL_CONTROL", 6, "request");
    FIELD(Ctl::USBLCmdSlotConfig, eventFilter);
    FIELD(Ctl::USBLCmdSlotConfig, type);
    FIELD(Ctl::USBLCmdSlotConfig, function);
    FIELD(Ctl::USBLCmdSlotConfig, cmdAction);
    FIELD(Ctl::USBLCmdSlotConfig, addressAction);
    FIELD(Ctl::USBLCmdSlotConfig, eventAction);
    FIELD(Ctl::USBLCmdSlotConfig, reserved1);
    FIELD(Ctl::USBLCmdSlotConfig, cmd_id);
    FIELD(Ctl::USBLCmdSlotConfig, cmd_id_next);
    FIELD(Ctl::USBLCmdSlotConfig, address_next);
    FIELD(Ctl::USBLCmdSlotConfig, reserved2);
    FIELD(Ctl::USBLCmdSlotConfig, bit_length);
    structEnd();

    STRUCT(Ctl::USBLCmdConfig, "ID_USBL_CONTROL", 6, "request");
    FIELD(Ctl::USBLCmdConfig, cmd_id);
    FIELD(Ctl::USBLCmdConfig, eventFilter);
    FIELD(Ctl::USBLCmdConfig, cmdIdAction);
    FIELD(Ctl::USBLCmdConfig, cmd_id_replacement);
    FIELD(Ctl::USBLCmdConfig, addressAction);
    FIELD(Ctl::USBLCmdConfig, address_replacement);
    FIELD(Ctl::USBLCmdConfig, eventAction);
    FIELD(Ctl::USBLCmdConfig, reserved1);
    FIELD(Ctl::USBLCmdConfig, reserved2);
    FIELD(Ctl::USBLCmdConfig, receiver_function);
    FIELD(Ctl::USBLCmdConfig, receive_bit_length);
    FIELD(Ctl::USBLCmdConfig, sender_function);
    FIELD(Ctl::USBLCmdConfig, sending_bit_length);
    structEnd();

    STRUCT(Modem::ModemSolutionHeader, "ID_MODEM_SOLUTION", 0, "content");
    FIELD(Modem::ModemSolutionHeader, timestamp_us);
    FIELD(Modem::ModemSolutionHeader, carrier_us);
    FIELD(Modem::ModemSolutionHeader, carrier_counter);
    FIELD(Modem::ModemSolutionHeader, reserved1);
    FIELD(Modem::ModemSolutionHeader, event);
    FIELD(Modem::ModemSolutionHeader, address_from);
    FIELD(Modem::ModemSolutionHeader, address_to);
    FIELD(Modem::ModemSolutionHeader, cmd_id_from);
    FIELD(Modem::ModemSolutionHeader, bit_length);
    structEnd();

    printf("\n  },\n");

    // ---- enums -------------------------------------------------------------
    // Emitted by name so the simulator asserts against real values instead of
    // literals. USBLPingRequest::FunctionBitArray == 1 while
    // USBLCmdSlotConfig::FunctionBitArray == 3 — both legal on the wire, which is
    // why a shared control corrupts frames in silence.
    printf("  \"enums\": {");

    enumBegin("USBLPingRequest::Function");
    val("FunctionDefault", Ctl::USBLPingRequest::FunctionDefault);
    val("FunctionBitArray", Ctl::USBLPingRequest::FunctionBitArray);
    val("FunctionLLGeoAzimuth", Ctl::USBLPingRequest::FunctionLLGeoAzimuth);
    enumEnd();

    enumBegin("USBLCmdSlotConfig::Function");
    val("FunctionDisabled", Ctl::USBLCmdSlotConfig::FunctionDisabled);
    val("FunctionSilent", Ctl::USBLCmdSlotConfig::FunctionSilent);
    val("FunctionNothing", Ctl::USBLCmdSlotConfig::FunctionNothing);
    val("FunctionBitArray", Ctl::USBLCmdSlotConfig::FunctionBitArray);
    val("FunctionLLGeoAzimuth", Ctl::USBLCmdSlotConfig::FunctionLLGeoAzimuth);
    enumEnd();

    enumBegin("USBLCmdSlotConfig::EventFilter");
    val("EventOnRequest", Ctl::USBLCmdSlotConfig::EventOnRequest);
    val("EventOnResponse", Ctl::USBLCmdSlotConfig::EventOnResponse);
    enumEnd();

    enumBegin("USBLCmdSlotConfig::Type");
    val("PayloadContainer", Ctl::USBLCmdSlotConfig::PayloadContainer);
    val("PayloadRequest", Ctl::USBLCmdSlotConfig::PayloadRequest);
    enumEnd();

    enumBegin("USBLCmdSlotConfig::CmdAction");
    val("CmdActionRepeat", Ctl::USBLCmdSlotConfig::CmdActionRepeat);
    val("CmdActionUseNext", Ctl::USBLCmdSlotConfig::CmdActionUseNext);
    enumEnd();

    enumBegin("USBLCmdSlotConfig::AddressAction");
    val("AddressActionRepeat", Ctl::USBLCmdSlotConfig::AddressActionRepeat);
    val("AddressActionUseNext", Ctl::USBLCmdSlotConfig::AddressActionUseNext);
    enumEnd();

    enumBegin("USBLCmdSlotConfig::EventAction");
    val("EventActionSwap", Ctl::USBLCmdSlotConfig::EventActionSwap);
    val("EventActionSame", Ctl::USBLCmdSlotConfig::EventActionSame);
    enumEnd();

    enumBegin("USBLCmdConfig::Function");
    val("FunctionDefault", Ctl::USBLCmdConfig::FunctionDefault);
    val("FunctionBitArray", Ctl::USBLCmdConfig::FunctionBitArray);
    val("FunctionLLGeoAzimuth", Ctl::USBLCmdConfig::FunctionLLGeoAzimuth);
    enumEnd();

    enumBegin("USBLCmdConfig::EventFilter");
    val("EventOnReceiveRequest", Ctl::USBLCmdConfig::EventOnReceiveRequest);
    val("EventOnReceiveResponse", Ctl::USBLCmdConfig::EventOnReceiveResponse);
    enumEnd();

    enumBegin("USBLCmdConfig::SendBackCmdIdAction");
    val("SendBackCmdIdIncoming", Ctl::USBLCmdConfig::SendBackCmdIdIncoming);
    val("SendBackCmdIdReplacement", Ctl::USBLCmdConfig::SendBackCmdIdReplacement);
    enumEnd();

    enumBegin("USBLCmdConfig::SendBackAddressAction");
    val("SendBackAddressIncoming", Ctl::USBLCmdConfig::SendBackAddressIncoming);
    val("SendBackAddressReplacement", Ctl::USBLCmdConfig::SendBackAddressReplacement);
    enumEnd();

    enumBegin("USBLCmdConfig::SendBackEventAction");
    val("SendBackEventSwaping", Ctl::USBLCmdConfig::SendBackEventSwaping);
    val("SendBackEventSame", Ctl::USBLCmdConfig::SendBackEventSame);
    enumEnd();

    enumBegin("ModemSolutionHeader::EventFilter");
    val("EventOnRequest", Modem::ModemSolutionHeader::EventOnRequest);
    val("EventOnResponse", Modem::ModemSolutionHeader::EventOnResponse);
    enumEnd();

    enumBegin("PayloadKind");
    val("None", (long long)Usbl::PayloadKind::None);
    val("Solution", (long long)Usbl::PayloadKind::Solution);
    val("AcousticNav", (long long)Usbl::PayloadKind::AcousticNav);
    val("BaseToBeacon", (long long)Usbl::PayloadKind::BaseToBeacon);
    val("BeaconActivation", (long long)Usbl::PayloadKind::BeaconActivation);
    enumEnd();

    enumBegin("ID");
    val("ID_VERSION", Parsers::ID_VERSION);
    val("ID_MARK", Parsers::ID_MARK);
    val("ID_DIST", Parsers::ID_DIST);
    val("ID_CHART", Parsers::ID_CHART);
    val("ID_DATASET", Parsers::ID_DATASET);
    val("ID_TIMESTAMP", Parsers::ID_TIMESTAMP);
    val("ID_NAV", Parsers::ID_NAV);
    val("ID_USBL_SOLUTION", Parsers::ID_USBL_SOLUTION);
    val("ID_MODEM_SOLUTION", Parsers::ID_MODEM_SOLUTION);
    val("ID_USBL_CONTROL", Parsers::ID_USBL_CONTROL);
    enumEnd();

    enumBegin("Type");
    val("CONTENT", Parsers::CONTENT);
    val("SETTING", Parsers::SETTING);
    val("GETTING", Parsers::GETTING);
    enumEnd();

    enumBegin("Version");
    val("v0", Parsers::v0);
    val("v1", Parsers::v1);
    val("v2", Parsers::v2);
    val("v3", Parsers::v3);
    val("v4", Parsers::v4);
    val("v5", Parsers::v5);
    val("v6", Parsers::v6);
    val("v7", Parsers::v7);
    enumEnd();

    enumBegin("BoardVersion");
    val("BoardNone", BoardNone);
    val("BoardEnhanced", BoardEnhanced);
    val("BoardBase", BoardBase);
    val("BoardNBase", BoardNBase);
    val("BoardChirp", BoardChirp);
    val("BoardAssist", BoardAssist);
    val("BoardDVL", BoardDVL);
    val("BoardUSBL", BoardUSBL);
    val("BoardUSBLBeacon", BoardUSBLBeacon);
    enumEnd();

    printf("\n  }\n}\n");
    return 0;
}
