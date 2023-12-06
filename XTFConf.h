#ifndef XTFCONF_H
#define XTFCONF_H

#include "stdint.h"

typedef uint8_t XTFBYTE;
typedef uint16_t XTFWORD;
typedef uint32_t XTFDWORD;
typedef char XTFCHAR;
typedef int16_t XTFSHORT;
typedef int32_t XTFINT;
typedef int32_t XTFLONG;

typedef struct   __attribute__((packed)) {
    XTFBYTE TypeOfChannel = 1; // SUBBOTTOM=0, PORT=1, STBD=2, BATHYMETRY=3
    XTFBYTE SubChannelNumber = 0; // Index for which CHANINFO structure this is.
    XTFWORD CorrectionFlags = 1; // 1=sonar imagery stored as slant-range, 2=sonar imagery stored as ground range (corrected)
    XTFWORD UniPolar = 0; // 0=data is polar, 1=data is unipolar
    XTFWORD BytesPerSample = 1; // 1 (8-bit data) or 2 (16-bit data) or 4 (32-bit)
    XTFDWORD Reserved = 0;
    XTFCHAR ChannelName[16] = {};
    float VoltScale = 5.0f;
    float Frequency = 350;
    float HorizBeamAngle = 1;
    float TiltAngle = 30;
    float BeamWidth = 50;
    float OffsetX = 0;
    float OffsetY = 0;
    float OffsetZ = 0;
    float OffsetYaw = 0;
    float OffsetPitch = 0;
    float OffsetRoll = 0;
    XTFWORD BeamsPerArray = 0;
    XTFBYTE SampleFormat = 0; // 0 = Legacy, 1 = 4-byte IBM float, 2 = 4-byte integer, 3 = 2-byte integer4 = unused, 5 = 4-byte IEEE float, 6 = unused, 7 = unused, 8 = 1-byte integer
    XTFCHAR ReservedArea2[53] = {};

} XTFCHANINFO;

typedef struct   __attribute__((packed)) {
    XTFBYTE FileFormat = 123;
    XTFBYTE SystemType = 1;
    XTFCHAR RecordingProgramName[8] = {};
    XTFCHAR RecordingProgramVersion[8] = {};
    XTFCHAR SonarName[16] = {};
    XTFWORD SonarType = 44; // 44 = NONE_SIDESCAN

    XTFCHAR NoteString[64] = {};
    XTFCHAR ThisFileName[64] = {};
    XTFWORD NavUnits = 3; // 0=Meters (i.e., UTM) or 3=Lat/Long
    XTFWORD NumberOfSonarChannels = 2;
    XTFWORD NumberOfBathymetryChannels = 0;
    XTFBYTE NumberOfSnippetChannels = 0;
    XTFBYTE NumberOfForwardLookArrays = 0;
    XTFWORD NumberOfEchoStrengthChannels = 0;
    XTFBYTE NumberOfInterferometryChannels = 0;
    XTFBYTE Reserved1 = 0;
    XTFWORD Reserved2 = 0;
    float ReferencePointHeight = 0; // Height of reference point above water line (m)

    // Navigation System Parameters
    XTFBYTE ProjectionType[12] = {};
    XTFBYTE SpheriodType[10] = {};
    XTFLONG NavigationLatency = 0; // ms

    float OriginY = 0;
    float OriginX = 0;
    float NavOffsetY = 0;
    float NavOffsetX = 0;
    float NavOffsetZ = 0;
    float NavOffsetYaw = 0;
    float MRUOffsetY = 0;
    float MRUOffsetX = 0;
    float MRUOffsetZ = 0;
    float MRUOffsetYaw = 0;
    float MRUOffsetPitch = 0;
    float MRUOffsetRoll = 0;

    XTFCHANINFO ChanInfo[6];

} XTFFILEHEADER;

typedef struct   __attribute__((packed)) {
    XTFWORD MagicNumber = 0xFACE;
    XTFBYTE HeaderType = 0; // 0 = XTF_HEADER_SONAR (Sidescan data), 3 = XTF_HEADER_ATTITUDE (attitude packet)
    XTFBYTE SubChannelNumber = 0; // If HeaderType is bathymetry, this indicates which head; if HeaderType is forward-looking sonar, and then this indicates which array.
    XTFWORD NumChansToFollow = 0; // If HeaderType is sonar, number of channels to follow.
    XTFWORD Reserved1[2] = {};
    XTFDWORD NumBytesThisRecord = 0; // Total byte count for this ping including this ping header.
    XTFWORD Year = 0;
    XTFBYTE Month = 0;
    XTFBYTE Day = 0;
    XTFBYTE Hour = 0;
    XTFBYTE Minute = 0;
    XTFBYTE Second = 0;
    XTFBYTE HSeconds = 0;
    XTFWORD JulianDay = 0;
    XTFDWORD EventNumber = 0;
    XTFDWORD PingNumber = 0; // Counts consecutively (usually from 0) and increments for each update.
    float SoundVelocity = 750; // m/s in one or two way form.
    float OceanTide = 0; // Altitude above Geoide (from RTK), if present;
    XTFDWORD Reserved2 = 0;
    float ConductivityFreq = 0;
    float TemperatureFreq = 0;
    float PressureFreq = 0;
    float PressureTemp = 0;
    float Conductivity = 0;
    float WaterTemperature = 0; // Water temperature in Celsius
    float Pressure = 0; // Water pressure in psia
    float ComputedSoundVelocity = 0; // m/s

    float MagX = 0; // mgauss
    float MagY = 0;
    float MagZ = 0;

    float AuxVal1 = 0;
    float AuxVal2 = 0;
    float AuxVal3 = 0;
    float AuxVal4 = 0;
    float AuxVal5 = 0;
    float AuxVal6 = 0;

    float SpeedLog = 0;
    float Turbidity = 0;

    float ShipSpeed = 0;
    float ShipGyro = 0;
    double ShipYcoordinate = 0;
    double ShipXcoordinate = 0;

    XTFWORD ShipAltitude = 0; // in decimeters
    XTFWORD ShipDepth = 0; // in decimeters
    XTFBYTE FixTimeHour = 0;
    XTFBYTE FixTimeMinute = 0;
    XTFBYTE FixTimeSecond = 0;
    XTFBYTE FixTimeHsecond = 0;
    float SensorSpeed = 0;
    float KP = 0;
    double SensorYcoordinate = 0; // Sensor Navigation information. Sensor latitude or northing; nav interface template token = E.
    double SensorXcoordinate = 0; // Sensor Navigation information. Sensor longitude or easting; nav interface template token = N.
    XTFWORD SonarStatus = 0;
    XTFWORD RangeToFish = 0; // Slant range to sensor in decimeters
    XTFWORD BearingToFish = 0; // Bearing to towfish from ship, stored in degrees multiplied by 100
    XTFWORD CableOut = 0; // Tow Cable information. Amount of cable payed out in meters
    float Layback = 0; // Tow Cable information. Distance over ground from ship to fish
    float CableTension = 0;
    float SensorDepth = 0; // Sensor Attitude information. Distance (m) from sea surface to sensor.
    float SensorPrimaryAltitude = 0; // Sensor Attitude information. Distance from towfish to the sea floor
    float SensorAuxAltitude = 0; // Sensor Attitude information. Auxiliary altitude;
    float SensorPitch = 0; // Sensor Attitude information. Pitch in degrees (positive=nose up);
    float SensorRoll = 0; // Sensor Attitude information. Roll in degrees (positive=roll to starboard);
    float SensorHeading = 0; // Sensor Attitude information. Sensor heading in degrees;
    float Heave = 0; // Attitude information. Sensors heave at start of ping. Positive value means sensor moved up.
    float Yaw = 0; // Attitude information. Sensor yaw. Positive means turn to right.
    XTFDWORD AttitudeTimeTag = 0; // Attitude information. In milliseconds - used to coordinate with millisecond time value in Attitude packets
    float DOT = 0; // Misc. Distance Off Track
    XTFDWORD NavFixMilliseconds = 0; // Misc. millisecond clock value when nav received.
    XTFBYTE ComputerClockHour = 0;
    XTFBYTE ComputerClockMinute = 0;
    XTFBYTE ComputerClockSecond = 0;
    XTFBYTE ComputerClockHsec = 0;
    XTFWORD FishPositionDeltaX = 0;
    XTFWORD FishPositionDeltaY = 0;
    XTFBYTE FishPositionErrorCode = 0; // Additional Tow Cable and Fish information from Trackpoint. Error code for FishPosition delta x,y.
    XTFDWORD OptionalOffsey = 0;
    XTFBYTE CableOutHundredths = 0;
    XTFBYTE ReservedSpace2[6] = {};
} XTFPINGHEADER;

typedef struct   __attribute__((packed)) {
    XTFWORD ChannelNumber = 0; // 0=port (low frequency), 1=stbd (low frequency), 2=port (high frequency), 3=stbd (high frequency)
    XTFWORD DownsampleMethod = 0; // 2 = MAX; 4 = RMS
    float SlantRange = 0; // Slant range of the data in meters
    float GroundRange = 0; // Ground range of the data; in meters
    float TimeDelay = 0; // Amount of time, in seconds, to the start of recorded data. (almost always 0.0).
    float TimeDuration = 0; // Amount of time, in seconds, recorded (typically SlantRange/750)
    float SecondsPerPing = 0; // Amount of time, in seconds, from ping to ping. (SlantRange/750)
    XTFWORD ProcessingFlags = 0; // 4 = TVG; 8 = BAC&GAC; 16 = filter, etc. (almost always zero)
    XTFWORD Frequency = 350; // Ccenter transmit frequency for this channel.
    XTFWORD InitialGainCode = 0; // Settings as transmitted by sonar
    XTFWORD GainCode = 0;
    XTFWORD BandWidth = 0;
    XTFDWORD ContactNumber = 0;
    XTFWORD ContactClassification = 0;
    XTFBYTE ContactSubNumber = 0;
    XTFBYTE ContactType = 0;
    XTFDWORD NumSamples = 0; // Number of samples that will follow this structure. The number of bytes will be this value multiplied by the number of bytes per sample. BytesPerSample found in CHANINFO structure (given in the file header).
    XTFWORD MillivoltScale = 0; // Maximum voltage, in mv, represented by a full-scale value in the data.If zero, then the value stored in the VoltScale should be used instead. VoltScale can be found in the XTF file header
    float ContactTimeOffTrack = 0; // Time off track to this contact (stored in milliseconds)
    XTFBYTE ContactCloseNumber = 0;
    XTFBYTE Reserved2 = 0;
    float FixedVSOP = 0; // This is the fixed, along-track size of each ping, stored in centimeters.
    short Weight = 0; // Weighting factor passed by some sonars
    XTFBYTE ReservedSpace[4] = {};
} XTFPINGCHANHEADER;

#endif // XTFCONF_H
