#ifndef STREAMLIST_H
#define STREAMLIST_H

#include "stdint.h"
#include "QByteArray"
#include "QHash"
#include "QMap"
#include "ProtoBinnary.h"
#include "StreamListModel.h"



using namespace Parsers;

class StreamList
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

    typedef struct {
        uint32_t start, end;
    } Fragment;

    typedef struct {
        uint16_t id = 0;
        RecordingState recordingState = RecordingError;
        UploadingState uploadingState = UploadingError;
        uint32_t size = 0;
        uint32_t unix = 0;
        QByteArray data;
        QList<Fragment> gaps;
    } Stream;

    void append(FrameParser* frame) {
        if(frame->completeAsKBP2()) {
            if(frame->isStream()) {

                uint16_t current_id = frame->streamId();

                if(_lastStreamId != current_id) {
                    if(_streams.contains(current_id)) {
                        _lastStreamId = current_id;
                        _lastStream = &_streams[current_id];
                    }

                    if(_lastStreamId != current_id) {
                        createStream(current_id);
                        _lastStream = getStream(current_id);
                        _lastStreamId = current_id;
                    }
                }

                insert(_lastStream, frame->frame(), frame->streamOffset(), frame->frameLen());

            } else {
                _streams[0].data.append((char*)frame->frame(), frame->frameLen());
            }
        } else if(frame->completeAsKBP()) {
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
                uint32_t unix = frame->read<U4>();

                createStream(id);
                Stream* stream = getStream(id);

                if(stream->size < size) { stream->size = size; }
                stream->unix = unix;

                stream->recordingState = (RecordingState)(flags & 0x3);

                _isListChenged = true;
            }
        }
    }

    void createStream(int id) {
        _streams[id].id = id;
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
        _modelList.clear();
        QMapIterator<int, Stream> i(_streams);
        while (i.hasNext()) {
            i.next();
            _modelList.appendEvent(i.value().id, i.value().size, "", i.value().recordingState, i.value().uploadingState);
        }
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
    uint16_t _lastStreamId = 0;
    Stream* _lastStream;
    bool _isListChenged = false;
    StreamListModel _modelList;

    void insert(Stream* stream, uint8_t* frame, uint32_t offset, uint16_t size) {
        uint32_t end = offset + size;
        QList<Fragment>& gaps = stream->gaps;
        QByteArray& data = stream->data;

        if(data.size() < offset) {
            gaps.append({(uint32_t)data.size(), offset});
            debugAddGap(offset, offset - (uint32_t)data.size());
        } else if(data.size() > offset) {
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
                            gaps.insert(i, {end + 1, g_end});
                            i+=1;
                        }
                    }
                    break;
                }
            }
        } else {

        }

        data.replace(offset, size, (char*)frame, size);
    }

    void debugAddGap(uint32_t start, uint32_t size);
    void debugSearchGap(uint32_t start, uint32_t size);
};

#endif // STREAMLIST_H
