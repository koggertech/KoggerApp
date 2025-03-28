#ifndef STREAMLIST_H
#define STREAMLIST_H

#include "stdint.h"
#include "QByteArray"
#include "QHash"
#include "QMap"
#include "proto_binnary.h"
#include "stream_list_model.h"
#include "QTime"
#include "QTimer"


using namespace Parsers;

class StreamList : public QObject
{
public:
    StreamList();

    typedef enum {
        RecordingError,
        RecordingIdle,
        RecordingPause,
        Recording
    } RecordingState;

    typedef enum {
        UploadingError,
        UploadingIdle,
        UploadingPause,
        Uploading
    } UploadingState;

    typedef enum {
        FragmentNone,
        FragmentNew,
        FragmentWait,
        FragmentProcessing
    } FragmentStatus;

    typedef struct {
        uint32_t start, end;
        uint64_t timestamp;
        FragmentStatus status;
    } Fragment;

    typedef struct {
        uint16_t id = 0;
        RecordingState recordingState = RecordingError;
        UploadingState uploadingState = UploadingError;
        uint32_t size = 0;
        uint32_t unixt = 0;
        QByteArray data;
        QList<Fragment> gaps;
        struct {
            uint32_t _fragments;
            uint32_t _lostFragments = 0;
            uint32_t _fillFragments = 0;
        } _counter;
        int modelIndex = -1;
    } Stream;

    void append(FrameParser* frame) {
        if(frame->isStream()) {
            uint16_t current_id = frame->streamId();

            if(_lastStreamId != current_id) {
                if(_streams.contains(current_id)) {
                    _lastStreamId = current_id;
                    _lastStream = &_streams[current_id];
                }

                if(_lastStreamId != current_id) {
                    updateStream(current_id);
                    _lastStream = getStream(current_id);
                    _lastStreamId = current_id;
                }
            }

            insert(_lastStream, frame->frame(), frame->streamOffset(), frame->frameLen());
        } else {
             _streams[0].data.append((char*)frame->frame(), frame->frameLen());
        }
    }

    void parse(FrameParser* frame) {
        if(frame->id() == ID_STREAM && frame->type() == CONTENT && frame->ver() == v0 && frame->resp() == false) {
            int item_cnt = frame->payloadLen()/12;
            while(item_cnt--) {
                int id = frame->read<U2>();
                uint16_t flags = frame->read<U2>();
                uint32_t size = frame->read<U4>();
                uint32_t unixt = frame->read<U4>();

                updateStream(id);
                Stream* stream = getStream(id);

                if(stream->size < size) { stream->size = size; }
                stream->unixt = unixt;

                stream->recordingState = (RecordingState)(flags & 0x3);

                updateStream(id);

                _isListChenged = true;
            }
        }
    }

    void updateStream(int id) {
        _streams[id].id = id;
        _modelList.appendEvent(_streams[id].id, _streams[id].size, _streams[id].data.size(), "", _streams[id].recordingState, _streams[id].uploadingState);
    }

    Stream* getStream(int id) {
        if(isStreamExist(id)) {
            return &_streams[id];
        } else {
            return NULL;
        }
    }

    bool isStreamExist(int id) {
        return _streams.contains(id);
    }

    StreamListModel* streamsList() {
        return &_modelList;
    }

    bool isListChenged() {
        if(_isListChenged) {
            _isListChenged = false;
            return true;
        }
        return false;
    }

protected:
    QMap<int, Stream> _streams;
    uint16_t _lastStreamId = 0xFFFF;
    Stream* _lastStream;
    bool _isListChenged = false;
    StreamListModel _modelList;
    QTimer _updater;
    uint64_t _timeLastGapsUpdate = 0;
    uint64_t _timeLastGapsInsert = 0;
    bool _isInserting = false;



    void insert(Stream* stream, uint8_t* frame, uint32_t offset, uint16_t size) {
        uint32_t end = offset + size;
        QList<Fragment>& gaps = stream->gaps;
        QByteArray& data = stream->data;

        _timeLastGapsInsert = timestamp();
        _isInserting = true;

        if(stream->size < end) {
            stream->size = end;
        }

        if (static_cast<uint32_t>(data.size()) < offset) {
            Fragment new_fragment = {
                .start = (uint32_t)data.size(),
                .end = offset,
                .timestamp = timestamp(),
                .status = FragmentStatus::FragmentNew
            };
            gaps.append(new_fragment);
            stream->_counter._lostFragments++;
            debugAddGap(offset, offset - (uint32_t)data.size());
        }
        else if (static_cast<uint32_t>(data.size()) > offset) {
            debugSearchGap(offset, size);

            for(int32_t i = 0; i < gaps.size(); i++) {
                uint32_t g_start = gaps[i].start;
                uint32_t g_end = gaps[i].end;

                if(g_start <= offset && g_end > offset) {
                    if(g_start == offset) {
                        if(g_end > end) {
                            gaps[i].start = end + 1;
                        } else {
                            gaps.removeAt(i);
                            i-=1;
                        }
                    } else {
                        gaps[i].end = offset - 1;
                        if(g_end >= end) {
                            Fragment new_fragment = {
                                .start = end + 1,
                                .end = g_end,
                                .timestamp = timestamp(),
                                .status = FragmentStatus::FragmentNew
                            };

                            gaps.insert(i, new_fragment);
                            i+=1;
                        }
                    }

                    gaps[i].timestamp = timestamp();
                    gaps[i].status = FragmentStatus::FragmentProcessing;
                    break;
                }
            }

            stream->_counter._fillFragments++;
        } else {
            stream->_counter._fragments++;
        }

        data.replace(offset, size, (char*)frame, size);
        updateStream(stream->id);

        _isInserting = false;
//        process();
    }

    uint64_t timestamp() {
        return QDateTime::currentMSecsSinceEpoch();
    }

    void debugAddGap(uint32_t start, uint32_t size);
    void debugSearchGap(uint32_t start, uint32_t size);

protected slots:
    void process();
};

#endif // STREAMLIST_H
