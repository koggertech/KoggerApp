#pragma once

#include <math.h>
#include <stdint.h>
#include <functional>
#include <QHash>
#include <QObject>
#include <QString>
#include <QUuid>
#include "math_defs.h"
#include "dsp_defs.h"


#if defined(Q_OS_ANDROID) || (defined Q_OS_LINUX)
#define MAKETIME(t) mktime(t)
#define GMTIME(t) gmtime(t)
#else
#define MAKETIME(t) _mkgmtime64(t)
#define GMTIME(t) _gmtime64(&sec);
#endif

#define CONSTANTS_RADIUS_OF_EARTH 6371000 /* meters (m) */
#define M_TWOPI_F 6.28318530717958647692f
#define M_PI_2_F  1.57079632679489661923f
#define M_RAD_TO_DEG 57.29577951308232087679f
#define M_DEG_TO_RAD 0.01745329251994329576f

enum PositionSource {
    PositionSourceNone,
    PositionSourceGNSS,
    PositionSourceRTK,
    PositionSourcePPK,
    PositionSourceInterpolation,
    PositionSourceExtrapolation,
    PositionSourceNED,
    PositionSourceLLA,
};

enum AltitudeSource {
    AltitudeSourceNone,
    AltitudeSourceRTK,
    AltitudeSourcePPK
};

enum class DataType {
    kUndefined = 0,
    kRaw,
    kInterpolated
};

struct ChannelId
{
  ChannelId()
        : uuid(), address(0)
    {
    }

    ChannelId(const QUuid& uuid, uint8_t address)
        : uuid(uuid), address(address)
    {
    }

    bool isValid() const
    {
        return !uuid.isNull();
    }

    QString toShortName() const
    {
        QString uuidPart = uuid.toString(QUuid::WithoutBraces).left(4).toUpper();
        QString addrPart = QString("%1").arg(address, 2, 10, QChar('0'));
        return uuidPart + "|" + addrPart;
    }

    bool operator==(const ChannelId& other) const
    {
        return uuid == other.uuid && address == other.address;
    }

    bool operator!=(const ChannelId& other) const
    {
        return !(*this == other);
    }

    bool operator<(const ChannelId& other) const
    {
        if (uuid < other.uuid) return true;
        if (uuid > other.uuid) return false;
        return address < other.address;
    }

    /*data*/
    QUuid uuid; // usually link ID
    uint8_t address;
};

// std::unordered_map, std::unordered_set
namespace std {
template <>
struct hash<ChannelId>
{
    size_t operator()(const ChannelId& key) const noexcept
    {
        const uint32_t* p = reinterpret_cast<const uint32_t*>(&key.uuid);

        return (hash<uint32_t>()(p[0])) ^
               (hash<uint32_t>()(p[1]) << 1) ^
               (hash<uint32_t>()(p[2]) << 2) ^
               (hash<uint32_t>()(p[3]) << 3) ^
               (hash<uint8_t>()(key.address) << 4);
    }
};
} // namespace std

// QHash, QSet
inline uint qHash(const ChannelId& key, uint seed = 0)
{
    std::size_t stlHash = std::hash<ChannelId>()(key);
    return static_cast<uint>(stlHash ^ (seed * 0x9e3779b9));
}

static const ChannelId CHANNEL_NONE  = ChannelId();

typedef struct NED NED;
typedef struct LLARef LLARef;

typedef struct LLA {
    double latitude = NAN, longitude = NAN;
    double altitude = NAN;

    PositionSource source = PositionSourceNone;
    AltitudeSource altSource = AltitudeSourceNone;

    LLA() {};
    LLA(double lat, double lon, double alt= NAN) {
        latitude = lat;
        longitude = lon;
        altitude = alt;
    }

    LLA(const LLA& other) {
        latitude = other.latitude;
        longitude = other.longitude;
        altitude = other.altitude;
    }

    LLA& operator=(const LLA& other) {
        if (this != &other) {
            latitude = other.latitude;
            longitude = other.longitude;
            altitude = other.altitude;
        }
        return *this;
    }

    inline LLA(const NED* ned, const LLARef* ref, bool spherical = true);

    bool isValid() const {
        return isfinite(latitude) && isfinite(longitude) && isfinite(altitude);
    }
    bool isCoordinatesValid() const {
        return isfinite(latitude) && isfinite(longitude);
    }
} LLA;

typedef struct  LLARef {
    double refLatSin = NAN;
    double refLatCos = NAN;
    double refLatRad = NAN;
    double refLonRad = NAN;
    LLA refLla;
    bool isInit = false;

    LLARef() {}

    LLARef(LLA lla) {
        refLatRad = lla.latitude * M_DEG_TO_RAD;
        refLonRad= lla.longitude * M_DEG_TO_RAD;
        refLatSin = sin(refLatRad);
        refLatCos = cos(refLatRad);
        refLla = lla;
        isInit = true;
    }

    LLARef(const LLARef& other)
        : refLatSin(other.refLatSin),
        refLatCos(other.refLatCos),
        refLatRad(other.refLatRad),
        refLonRad(other.refLonRad),
        refLla(other.refLla),
        isInit(other.isInit) {}

    LLARef& operator=(const LLARef& other) {
        if (this != &other) {
            refLatSin = other.refLatSin;
            refLatCos = other.refLatCos;
            refLatRad = other.refLatRad;
            refLonRad = other.refLonRad;
            refLla = other.refLla;
            isInit = other.isInit;
        }
        return *this;
    }

    friend bool operator==(const LLARef& lhs, const LLARef& rhs) {
        if (!std::isfinite(lhs.refLla.latitude) || !std::isfinite(lhs.refLla.longitude) ||
            !std::isfinite(rhs.refLla.latitude) || !std::isfinite(rhs.refLla.longitude)) {
            return false;
        }
        return qFuzzyCompare(1.0 + lhs.refLla.latitude, 1.0 + rhs.refLla.latitude) &&
               qFuzzyCompare(1.0 + lhs.refLla.longitude, 1.0 + rhs.refLla.longitude);
    }

    friend bool operator!=(const LLARef& lhs, const LLARef& rhs) {
        return !(lhs == rhs);
    }
} LLARef;

typedef struct NED {
    double n = NAN, e = NAN, d = NAN;
    PositionSource source = PositionSourceNone;

    NED() {}
    NED(double _n, double _e, double _d) : n(_n), e(_e), d(_d) { };
    NED(LLA* lla, LLARef* ref, bool spherical = true) {
        if (spherical) {
            double lat_rad = lla->latitude * M_DEG_TO_RAD;
            double lon_rad = lla->longitude * M_DEG_TO_RAD;

            double sin_lat = sin(lat_rad);
            double cos_lat = cos(lat_rad);
            double cos_d_lon = cos(lon_rad - ref->refLonRad);

            double arg = ref->refLatSin * sin_lat + ref->refLatCos * cos_lat * cos_d_lon;

            if (arg > 1.0) {
                arg = 1.0;

            } else if (arg < -1.0) {
                arg = -1.0;
            }

            double c = acos(arg);
            double k = (fabs(c) < kmath::dblEps) ? 1.0 : (c / sin(c));

            n = k * (ref->refLatCos * sin_lat - ref->refLatSin * cos_lat * cos_d_lon) * CONSTANTS_RADIUS_OF_EARTH;
            e = k * cos_lat * sin(lon_rad - ref->refLonRad) * CONSTANTS_RADIUS_OF_EARTH;
            d = -(lla->altitude - ref->refLla.altitude);
        }
        else { // merсator
            double R = CONSTANTS_RADIUS_OF_EARTH;

            double lat_rad = lla->latitude * M_DEG_TO_RAD;
            double ref_lat_rad = ref->refLla.latitude * M_DEG_TO_RAD;

            double y = R * log(tan(M_PI / 4.0 + lat_rad / 2.0));
            double y_ref = R * log(tan(M_PI / 4.0 + ref_lat_rad / 2.0));

            double delta_y = y - y_ref;

            double delta_lon = (lla->longitude - ref->refLla.longitude) * M_DEG_TO_RAD;
            double x = R * delta_lon;

            n = delta_y;
            e = x;
            d = -(lla->altitude - ref->refLla.altitude);
        }
    }


    bool isValid() {
        return isfinite(n) && isfinite(e) && isfinite(d);
    }

    bool isCoordinatesValid() const {
        return isfinite(n) && isfinite(e);
    }
} NED;

LLA::LLA(const NED* ned, const LLARef* ref, bool spherical)
{
    if (spherical) {
        if (!ned || !ref ||
            !std::isfinite(ned->n) ||
            !std::isfinite(ned->e) ||
            !std::isfinite(ned->d)) {
            return;
        }

        double R = CONSTANTS_RADIUS_OF_EARTH;
        double n = ned->n;
        double e = ned->e;

        double c = sqrt(n * n + e * e) / R;

        if (qFuzzyIsNull(c)) {
            latitude = ref->refLla.latitude;
            longitude = ref->refLla.longitude;
            altitude = ref->refLla.altitude - ned->d;
            return;
        }

        double lat0 = ref->refLatRad;
        double lon0 = ref->refLonRad;

        double sin_c = sin(c);
        double cos_c = cos(c);

        double alpha = atan2(e, n);

        double sin_lat = sin(lat0) * cos_c + cos(lat0) * sin_c * cos(alpha);
        double lat_rad = asin(sin_lat);

        double y = sin(alpha) * sin_c * cos(lat0);
        double x = cos_c - sin(lat0) * sin_lat;
        double lon_rad = lon0 + atan2(y, x);

        latitude = lat_rad * 180.0 / M_PI;
        longitude = lon_rad * 180.0 / M_PI;
        altitude = ref->refLla.altitude - ned->d;
    }
    else { // mercator
        if (!ned || !ref ||
            !std::isfinite(ned->n) ||
            !std::isfinite(ned->e) ||
            !std::isfinite(ned->d)) {
            return;
        }

        double R = CONSTANTS_RADIUS_OF_EARTH;

        double ref_lat_rad = ref->refLla.latitude * M_DEG_TO_RAD;
        double y_ref = R * log(tan(M_PI / 4.0 + ref_lat_rad / 2.0));

        double y = y_ref + ned->n;

        double lat_rad = 2.0 * atan(exp(y / R)) - M_PI / 2.0;

        double ref_lon_rad = ref->refLla.longitude * M_DEG_TO_RAD;
        double lon_rad = ref_lon_rad + (ned->e / R);

        latitude = lat_rad * M_RAD_TO_DEG;
        longitude = lon_rad * M_RAD_TO_DEG;
        altitude = ref->refLla.altitude - ned->d;
    }
}

typedef struct XYZ {
    double x = 0, y = 0, z = 0;
} XYZ;

typedef struct DateTime {
    time_t sec = 0;
    int nanoSec = 0;

    DateTime() {}

    DateTime(int64_t unix_sec, int32_t nanosec = 0) {

        sec = unix_sec;
        int s_dif = nanosec/1e9;
        nanosec = nanosec - s_dif*1e9;
        if(nanosec < 0) {
            s_dif--;
            nanosec += 1e9;
        }

        sec += s_dif;
        nanoSec = nanosec;
    }

    DateTime(int year, int month, int day, int hour, int min, int s, int nanosec = 0) {
        //        if(year >= 2000) {
        //            year -= 2000;
        //        }

        tm  t = {};
        t.tm_year = year - 1900;
        t.tm_mon = month - 1;
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = min;
        t.tm_sec = s;

        sec = MAKETIME(&t);

        int s_dif = nanosec/1e9;
        nanosec = nanosec - s_dif*1e9;
        if(nanosec < 0) {
            s_dif--;
            nanosec += 1e9;
        }

        sec += s_dif;
        nanoSec = nanosec;
    }

    tm getDateTime() {
        return *GMTIME(&sec);
    }

    int32_t get_us_frac() {
        return nanoSec/1000;
    }

    int32_t get_ms_frac() {
        return nanoSec/1000000;
    }

    void addSecs(int add_secs) {
        sec += add_secs;
    }
} DateTime;

struct Position { // TODO: refactor all these structs
    DateTime time;
    LLA lla;
    NED ned;

    DataType dataType = DataType::kUndefined;

    void LLA2NED(LLARef* ref) {
        ned = NED(&lla, ref);
    }
};

template <typename T>
inline bool hasIndex(const QVector<T>& vec, int i) {
    return i >= 0 && i < vec.size();
}

typedef struct DatasetChannel {
    Q_GADGET
    // Q_PROPERTY(int channelId  MEMBER channel)

public:
    ChannelId channelId_;
    uint8_t subChannelId_ = 0;
    int count_ = 0;
    double distanceFrom_ = NAN;
    double distanceTo_ = NAN;
    XYZ localPosition_;
    QString portName_;

    DatasetChannel()
    {}

    DatasetChannel(const ChannelId& channelId, uint8_t subChannelId)
        : channelId_(channelId),
        subChannelId_(subChannelId)
    {}

    void counter() {
        count_++;
    }
} DatasetChannel;
Q_DECLARE_METATYPE(DatasetChannel)

enum class BottomTrackPreset {
    BottomTrackOneBeam = 0,
    BottomTrackOneBeamNarrow,
    BottomTrackSideScan
};

struct BottomTrackParam {
    float gainSlope = 1.0;
    float threshold = 1.0;
    float verticalGap = 0;
    float minDistance = 0;
    float maxDistance = 1000;

    int indexFrom = 0;
    int indexTo = 0;
    int windowSize = 1;

    BottomTrackPreset preset = BottomTrackPreset::BottomTrackOneBeam;

    struct {
        float x = 0, y = 0, z = 0;
    } offset;
};

typedef struct ComplexSignal {
    uint32_t globalOffset = 0;
    float sampleRate = 0;
    bool isComplex = true;
    QVector<ComplexF> data;
} ComplexSignal;

struct RecordParameters {
    uint16_t resol      = 0;
    uint16_t count      = 0;
    uint16_t offset     = 0;
    uint16_t freq       = 0;
    uint8_t  pulse      = 0;
    uint8_t  boost      = 0;
    uint32_t soundSpeed = 0;

    bool isNull() const {
        if (resol      == 0 &&
            count      == 0 &&
            offset     == 0 &&
            freq       == 0 &&
            pulse      == 0 &&
            boost      == 0 &&
            soundSpeed == 0) {
            return true;
        }
        return false;
    }
};

typedef QMap<ChannelId, QMap<int, QVector<ComplexSignal>>> ComplexSignals;

static inline double norm360(double a)
{
    a = std::fmod(a, 360.0);
    if (a < 0.0) {
        a += 360.0;
    }

    return a;
}
static inline double norm180(double a)
{
    a = std::fmod(a + 180.0, 360.0);
    if (a < 0.0) {
        a += 360.0;
    }

    return a - 180.0;
}

static inline double distanceMetersLLA(double latBoat, double lonBoat, double latTarget, double lonTarget)
{
    LLARef boatRef(LLA(latBoat, lonBoat, 0.0));
    LLA    tgt(latTarget, lonTarget, 0.0);
    NED    bt(&tgt, &boatRef, /*spherical=*/true);

    return std::hypot(bt.n, bt.e);
}

static inline double angleToTargetDeg(double latBoat, double lonBoat, double latTarget, double lonTarget, double headingDeg)
{
    LLARef boatRef(LLA(latBoat, lonBoat, 0.0));
    LLA    tgt(latTarget, lonTarget, 0.0);
    NED    bt(&tgt, &boatRef, /*spherical=*/true);

    const double dist = std::hypot(bt.n, bt.e);
    if (dist < 1e-6) {
        return 0.0;
    }

    const double bearingTrue = norm360(std::atan2(bt.e, bt.n) * M_RAD_TO_DEG);
    const double headTrue    = norm360(headingDeg);
    return std::abs(norm180(bearingTrue - headTrue));
}

static inline NED fruOffsetToNed(const QVector3D& offFru, double yawDeg)
{
    const double psi = yawDeg * M_DEG_TO_RAD;
    const double c = std::cos(psi);
    const double s = std::sin(psi);

    const double x = offFru.x();
    const double y = offFru.y();
    const double z = offFru.z();

    const double dN =  x * c - y * s;
    const double dE =  x * s + y * c;
    const double dD = -z;

    return NED(dN, dE, dD);
}
