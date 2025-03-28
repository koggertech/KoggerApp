#ifndef CONVERTERXTF_H
#define CONVERTERXTF_H

#include "XTFConf.h"
#include "plotcash.h"
#include "vector"

class ConverterXTF {
public:
    ConverterXTF() {}

    QByteArray _lastData;

    XTFFILEHEADER header;

    QByteArray toXTF(Dataset* dataset, int channel1, int channel2 = CHANNEL_NONE) {
        QByteArray xtfdata;
        XTFFILEHEADER fileheader;

        fileheader.ThisFileName[0] = 'k';
        fileheader.RecordingProgramName[0] = 's';
        fileheader.RecordingProgramVersion[0] = '1';

        fileheader.NumberOfSonarChannels = 1;
        if(channel2 != CHANNEL_NONE) {
            fileheader.NumberOfSonarChannels = 2;
        }

        fileheader.ChanInfo[0].SubChannelNumber = 0;
        fileheader.ChanInfo[0].TypeOfChannel = 1;
        fileheader.ChanInfo[0].BytesPerSample = 1;

        if(channel2 != CHANNEL_NONE) {
            fileheader.ChanInfo[1].SubChannelNumber = 1;
            fileheader.ChanInfo[1].TypeOfChannel = 2;
            fileheader.ChanInfo[1].BytesPerSample = fileheader.ChanInfo[0].BytesPerSample;
        } else {
            fileheader.ChanInfo[1].SubChannelNumber = 0;
            fileheader.ChanInfo[1].TypeOfChannel = 0;
            fileheader.ChanInfo[1].BytesPerSample = 0;
        }


        xtfdata.append((char*)&fileheader, sizeof (XTFFILEHEADER));

        int dataset_size = dataset->size();
        int ping_numb = 0;

        int fix_h = 0, fix_m = 0, fix_s = 0, fix_hs = 0;
        int64_t fix_timetag_ms = 0;
        for(int i = 0; i < dataset_size; i++) {
            Epoch* epoch = dataset->fromIndex(i);

            if(epoch != NULL) {
                XTFPINGHEADER pingheader;
                pingheader.NumBytesThisRecord = 0;
                pingheader.NumBytesThisRecord += sizeof (XTFPINGHEADER);
                pingheader.PingNumber = ping_numb;

                Position ext_pos = epoch->getExternalPosition();
                Position int_pos = epoch->getPositionGNSS();

                Position pos;
                if(ext_pos.lla.isCoordinatesValid()) {
                    pos = ext_pos;
                } else if(int_pos.lla.isCoordinatesValid()){
                    pos = int_pos;
                } else {
                    pos.lla.latitude = 0;
                    pos.lla.longitude = 0;
                }

                float yaw = epoch->yaw();
                if(isfinite(yaw)) {
                    pingheader.SensorHeading = yaw;
                    pingheader.Yaw = yaw;
                }

                if(pos.time.sec > 0) {
                    fix_timetag_ms = (pos.time.sec*1e9 + pos.time.nanoSec)/1e6;
                }

                pingheader.ShipYcoordinate = pos.lla.latitude;
                pingheader.ShipXcoordinate = pos.lla.longitude;

                if(pos.time.sec > 0) {
                    tm date_time = pos.time.getDateTime();

                    pingheader.Year = date_time.tm_year+1900;
                    pingheader.Month = date_time.tm_mon+1;
                    pingheader.JulianDay = date_time.tm_yday;
                    pingheader.Day = date_time.tm_mday;
                    pingheader.Hour = date_time.tm_hour;
                    pingheader.Minute = date_time.tm_min;
                    pingheader.Second = date_time.tm_sec;
                    pingheader.HSeconds = pos.time.get_ms_frac()/10;

                    if(pos.lla.isCoordinatesValid()) {
                        fix_h = pingheader.Hour;
                        fix_m = pingheader.Minute;
                        fix_s = pingheader.Second;
                        fix_hs = pingheader.HSeconds;
                        pingheader.FixTimeHour = fix_h;
                        pingheader.FixTimeMinute = fix_m;
                        pingheader.FixTimeSecond = fix_s;
                        pingheader.FixTimeHsecond = fix_hs;
                    }

                    pingheader.AttitudeTimeTag = fix_timetag_ms;
                    pingheader.NavFixMilliseconds = fix_timetag_ms;
                }


                pingheader.SensorYcoordinate = pingheader.ShipYcoordinate;
                pingheader.SensorXcoordinate = pingheader.ShipXcoordinate;

                Epoch::Echogram* chart1 = epoch->chart(channel1);
                Epoch::Echogram* chart2 = epoch->chart(channel2);
                QVector<uint8_t> raw1;
                QVector<uint8_t> raw2;

                XTFPINGCHANHEADER pingch1;
                XTFPINGCHANHEADER pingch2;

                pingch1.ChannelNumber = 0;
                pingch2.ChannelNumber = 1;

                if(chart1 != NULL) {
                    QVector<uint8_t> raw = chart1->amplitude;
                    int constr_size = raw.size();
                    raw1.resize(constr_size);
                    for(int ri = 0; ri < constr_size; ri++) {
                        raw1[raw1.size() - ri - 1] = raw[ri];
                    }
                }

                if(chart2 != NULL && channel2 != CHANNEL_NONE) {
                    QVector<uint8_t> raw = chart2->amplitude;
                    int constr_size = raw.size();
                    raw2.resize(constr_size);
                    for(int ri = 0; ri < constr_size; ri++) {
                        raw2[ri] = raw[ri];
                    }
                }

                if(raw1.size() > 0) {
                    pingheader.NumChansToFollow++;
                    pingheader.NumBytesThisRecord += sizeof (XTFPINGCHANHEADER);
                    pingheader.NumBytesThisRecord += raw1.size();

                    pingch1.NumSamples = raw1.size();
                    pingch1.SlantRange = chart1->resolution*raw1.size();
                    pingch1.TimeDuration = pingch1.SlantRange/750;
                    pingch1.SecondsPerPing = 0.1;
                }

                if(raw2.size() > 0) {
                    pingheader.NumChansToFollow++;
                    pingheader.NumBytesThisRecord += sizeof (XTFPINGCHANHEADER);
                    pingheader.NumBytesThisRecord += raw2.size();

                    pingch2.NumSamples = raw2.size();
                    pingch2.SlantRange = chart2->resolution*raw2.size();
                    pingch2.TimeDuration = pingch2.SlantRange/750;
                    pingch2.SecondsPerPing = 0.1;
                }

                if(pingheader.NumChansToFollow > 0) {
                    //                    if(raw1.size() == raw2.size()) {
                    xtfdata.append((char*)&pingheader, sizeof (pingheader));

                    if(raw1.size() > 0) {
                        xtfdata.append((char*)&pingch1, sizeof (pingch1));
                        xtfdata.append((char*)raw1.constData(), raw1.size());
                    }

                    if(raw2.size() > 0) {
                        xtfdata.append((char*)&pingch2, sizeof (pingch2));
                        xtfdata.append((char*)raw2.constData(), raw2.size());
                    }

                    ping_numb++;
                    //                    }
                }

            }
        }

        return xtfdata;
    }

    bool toDataset(QByteArray data, Dataset* dataset) {
//        if(xtf_data == NULL || dataset == NULL) {
//            return false;
//        }

        _lastData = data;

        uint8_t* cdata = (uint8_t*)data.constData();
        uint8_t* cdata_end = cdata + data.size();

        XTFFILEHEADER* fileheader = (XTFFILEHEADER*)cdata;
        if(fileheader->FileFormat != 123) {
//            consoleInfo("XTF is not valid");
            return false;
        }

        header = *fileheader;

        volatile char* record_prog = fileheader->RecordingProgramName;
        Q_UNUSED(record_prog);

        cdata += sizeof (XTFFILEHEADER);

        while(cdata < cdata_end) {
            XTFPINGHEADER* pingheader = (XTFPINGHEADER*)(cdata);
            if(pingheader->MagicNumber != 0xFACE) {
//                consoleInfo("XTF packet is not valid");
                return false;
            }

            cdata += sizeof (XTFPINGHEADER);

            if(pingheader->HeaderType == 0) {
                uint16_t ch_count = pingheader->NumChansToFollow;
                double lat = pingheader->SensorYcoordinate;
                double lon = pingheader->SensorXcoordinate;
                if(lat != 0 || lat != 0) {
                    dataset->addPosition(lat, lon);
                }

                for(uint16_t chi = 0; chi < ch_count; chi++) {
                    XTFPINGCHANHEADER* pingch = (XTFPINGCHANHEADER*)(cdata);


                    const float range = pingch->SlantRange;
                    const uint16_t sample_count = pingch->NumSamples;
                    const uint16_t sample_bytes = fileheader->ChanInfo[chi].BytesPerSample;
                    const uint16_t sample_format = fileheader->ChanInfo[chi].SampleFormat;

                    if(sample_count == 0 || sample_bytes == 0) {
//                        consoleInfo("XTF samples count is zero");
                        return false;
                    }

                    cdata += sizeof (XTFPINGCHANHEADER);
                    QVector<uint8_t> data;
                    data.resize(sample_count);

                    if((sample_format == 0 && sample_bytes == 1) || sample_format == 8) {
                        if(pingch->ChannelNumber == 0 || pingch->ChannelNumber == 2) {
                            for(uint16_t i = 0; i < sample_count; i++) {
                                data[sample_count-i-1] = *((uint8_t*)cdata);
                                cdata += sample_bytes;
                            }
                        } else {
                            for(uint16_t i = 0; i < sample_count; i++) {
                                data[i] = *((uint8_t*)cdata);
                                cdata += sample_bytes;
                            }
                        }
                    } else if((sample_format == 0 && sample_bytes == 2) || sample_format == 2) {
                        if(pingch->ChannelNumber == 0 || pingch->ChannelNumber == 2) {
                            for(uint16_t i = 0; i < sample_count; i++) {
                                data[sample_count-i-1] = *((uint16_t*)cdata) / 128;
                                cdata += sample_bytes;
                            }
                        } else {
                            for(uint16_t i = 0; i < sample_count; i++) {
                                data[i] = *((uint16_t*)cdata) / 128;
                                cdata += sample_bytes;
                            }
                        }
                    }

                    ChartParameters chartParams(pingch->ChannelNumber, 0, {}, {}, {}, {});
                    dataset->addChart(chartParams, data, range/sample_count, 0); // TODO, address, channel
                }

            } else  if(pingheader->HeaderType == 3) {
                cdata += 64;
            } else {
//                consoleInfo("XTF header type is unknown");
                cdata += pingheader->NumBytesThisRecord - sizeof (XTFPINGHEADER);
            }

            if((uint8_t*)pingheader + pingheader->NumBytesThisRecord != cdata) {
//                consoleInfo("XTF pingchannel offset error");
                return false;
            }
        }

        return true;
    }




};

#endif // CONVERTERXTF_H
