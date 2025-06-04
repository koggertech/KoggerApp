#include "plot2D.h"
#include "epoch_event.h"


Plot2D::Plot2D()
    : datasetPtr_(nullptr)
    , pendingBtpLambda_(nullptr)
    , isHorizontal_(true)
    , isEnabled_(true)
{
    echogram_.setVisible(true);
    attitude_.setVisible(true);
    encoder_.setVisible(true);
    dvlBeamVelocity_.setVisible(true);
    dvlSolution_.setVisible(true);
    usblSolution_.setVisible(true);
    bottomProcessing_.setVisible(true);
    rangefinder_.setVisible(true);
    grid_.setVisible(true);
    aim_.setVisible(true);
    quadrature_.setVisible(false);
    setDataChannel(false, CHANNEL_NONE, 0, {});
    cursor_.attitude.from = -180;
    cursor_.attitude.to = 180;
    cursor_.distance.set(0, 20);
}

float Plot2D::getCursorDistance() const
{
    if (canvas_.height() <= 0) {
        return 0.0f;
    }

    const float valueRange = cursor_.distance.to - cursor_.distance.from;
    const float valueScale = static_cast<float>(cursor_.mouseY) / static_cast<float>(canvas_.height());

    return valueScale * valueRange + cursor_.distance.from;
}

std::tuple<ChannelId, uint8_t, QString> Plot2D::getSelectedChannelId(float cursorDistance) const
{
    const float dist = qFuzzyIsNull(cursorDistance) ? getCursorDistance() : cursorDistance;
    const bool useChannel1 = qFuzzyIsNull(dist) || dist < 0.0f || cursor_.channel2 == CHANNEL_NONE;

    return useChannel1 ? std::make_tuple(cursor_.channel1, cursor_.subChannel1, cursor_.firstChannelPortName) : std::make_tuple(cursor_.channel2, cursor_.subChannel2, cursor_.secondChannelPortName);
}

void Plot2D::setDataset(Dataset *dataset)
{
    datasetPtr_ = dataset;
    if (pendingBtpLambda_) {
        pendingBtpLambda_();
        pendingBtpLambda_ = nullptr;
    }
}

float Plot2D::getDepthByMousePos(int mouseX, int mouseY, bool isHorizontal) const
{
    int currPos = isHorizontal ? mouseY : mouseX;

    const float valueRange = cursor_.distance.to - cursor_.distance.from;
    const float valueScale = static_cast<float>(currPos) / static_cast<float>(canvas_.height());

    return valueScale * valueRange + cursor_.distance.from;
}

int Plot2D::getEpochIndxByMousePos(int mouseX, int mouseY, bool isHorizontal) const
{
    const int width = canvas_.width();

    if (width == 0 || cursor_.indexes.empty()) {
        return -1;
    }

    int column = isHorizontal ? mouseX : (width - 1 - mouseY);
    int indxsSize = cursor_.indexes.size();
    if (column < 0 || column >= width || column >= indxsSize) {
        return -1;
    }

    return cursor_.indexes[column];
}

QPoint Plot2D::getMousePosByDepthAndEpochIndx(float depth, int epochIndx, bool isHorizontal) const
{
    if (!datasetPtr_ || canvas_.width() <= 0 || canvas_.height() <= 0) {
        return QPoint(-1, -1);
    }

    int column = -1;
    int sizeIndxs = cursor_.indexes.size();
    for (int i = 0; i < sizeIndxs; ++i) {
        if (cursor_.indexes[i] == epochIndx) {
            column = i;
            break;
        }
    }

    if (column == -1) {
        return QPoint(-1, -1);
    }

    const float valueRange = cursor_.distance.to - cursor_.distance.from;
    const float norm = (depth - cursor_.distance.from) / valueRange;
    int depthPix = static_cast<int>(norm * canvas_.height());
    depthPix = std::clamp(depthPix, 0, canvas_.height() - 1);

    if (isHorizontal) {
        return QPoint(column, depthPix);
    }
    else {
        return QPoint(depthPix, canvas_.width() - 1 - column);
    }
}

void Plot2D::addReRenderPlotIndxs(const QSet<int> &indxs)
{
    echogram_.addReRenderPlotIndxs(indxs);
}

void Plot2D::setPlotEnabled(bool state)
{
    isEnabled_ = state;
}

bool Plot2D::plotEnabled() const
{
    return isEnabled_;
}

bool Plot2D::getImage(int width, int height, QPainter* painter, bool is_horizontal)
{
    if (is_horizontal) {
        canvas_.setSize(width, height, painter);
    }
    else {
        canvas_.setSize(height, width, painter);
        painter->rotate(-90);
        painter->translate(-height, 0);
    }

    reindexingCursor();
    reRangeDistance();

    return true;
}

void Plot2D::draw(QPainter *painterPtr)
{
    //    painter->setCompositionMode(QPainter::RasterOp_SourceXorDestination);
    echogram_.draw(this, datasetPtr_);
    attitude_.draw(this, datasetPtr_);
    encoder_.draw(this, datasetPtr_);
    dvlBeamVelocity_.draw(this, datasetPtr_);
    dvlSolution_.draw(this, datasetPtr_);
    usblSolution_.draw(this, datasetPtr_);
    bottomProcessing_.draw(this, datasetPtr_);
    rangefinder_.draw(this, datasetPtr_);
    gnss_.draw(this, datasetPtr_);
    quadrature_.draw(this, datasetPtr_);

    painterPtr->setCompositionMode(QPainter::CompositionMode_Exclusion);
    grid_.draw(this, datasetPtr_);
    aim_.draw(this, datasetPtr_);

    contacts_.draw(this, datasetPtr_);
}

bool Plot2D::isHorizontal()
{
    return isHorizontal_;
}

void Plot2D::setHorizontal(bool is_horizontal)
{
    isHorizontal_ = is_horizontal;
    contacts_.setIsHorizontal(isHorizontal_);
}

void Plot2D::setAimEpochEventState(bool state)
{
    aim_.setEpochEventState(state);
}

void Plot2D::setTimelinePosition(float position)
{
    if (position > 1.0f) {
        position = 1.0f;
    }
    if (position < 0) {
        position = 0;
    }
    if (cursor_.position != position) {
        cursor_.position = position;
        plotUpdate();
    }
}

void Plot2D::resetAim()
{
    cursor_.selectEpochIndx = -1;
}

void Plot2D::setTimelinePositionSec(float position)
{
    if (position > 1.0f) {
        position = 1.0f;
    }
    if (position < 0) {
        position = 0;
    }

    cursor_.position = position;
    plotUpdate();
}

void Plot2D::setTimelinePositionByEpoch(int epochIndx)
{
    float pos = epochIndx == -1 ? cursor_.position : static_cast<float>(epochIndx + cursor_.indexes.size() / 2) / static_cast<float>(datasetPtr_->size());
    cursor_.selectEpochIndx = epochIndx;
    setTimelinePositionSec(pos);
}

void Plot2D::scrollPosition(int columns)
{
    float new_position = timelinePosition() + (1.0f / datasetPtr_->size()) * columns;
    setTimelinePosition(new_position);
}

void Plot2D::setDataChannel(bool fromGui, const ChannelId& channel, uint8_t subChannel1, const QString& portName1, const ChannelId& channel2, uint8_t subChannel2, const QString& portName2)
{
    cursor_.channel1 = channel;
    cursor_.subChannel1 = subChannel1;
    cursor_.firstChannelPortName = fromGui ? portName1 : QString("%1|%2|%3").arg(portName1, QString::number(channel.address), QString::number(subChannel1));
    cursor_.channel2 = channel2;
    cursor_.subChannel2 = subChannel2;
    cursor_.secondChannelPortName = fromGui ? portName2 : QString("%1|%2|%3").arg(portName2, QString::number(channel2.address), QString::number(subChannel2));

    float from = NAN, to = NAN;

    if (datasetPtr_) {
        datasetPtr_->getMaxDistanceRange(&from, &to, channel, subChannel1, channel2, subChannel2);

        if (isfinite(from) && isfinite(to) && (to - from) > 0) {
            cursor_.distance.set(from, to);
        }
    }

    resetCash();
    plotUpdate(); // TODO: this calls from ctr
}

bool Plot2D::getIsContactChanged()
{
    return contacts_.isChanged();
}

QString Plot2D::getContactInfo()
{
    return contacts_.getInfo();
}

void Plot2D::setContactInfo(const QString& str)
{
    contacts_.setInfo(str);
}

bool Plot2D::getContactVisible()
{
    return contacts_.getVisible();
}

void Plot2D::setContactVisible(bool state)
{
    contacts_.setVisible(state);
}

int Plot2D::getContactPositionX()
{
    return contacts_.getPosition().x();
}

int Plot2D::getContactPositionY()
{
    return contacts_.getPosition().y();
}

int Plot2D::getContactIndx()
{
    return contacts_.getIndx();
}

double Plot2D::getContactLat()
{
    return contacts_.getLat();
}

double Plot2D::getContactLon()
{
    return contacts_.getLon();
}

double Plot2D::getContactDepth()
{
    return contacts_.getDepth();
}

float Plot2D::getEchogramLowLevel() const
{
    return echogram_.getLowLevel();
}

float Plot2D::getEchogramHighLevel() const
{
    return echogram_.getHighLevel();
}

int Plot2D::getThemeId() const
{
    return echogram_.getThemeId();
}

void Plot2D::setEchogramLowLevel(float low) {
    echogram_.setLowLevel(low);
    plotUpdate();
}

void Plot2D::setEchogramHightLevel(float high) {
    echogram_.setHightLevel(high);
    plotUpdate();
}

void Plot2D::setEchogramVisible(bool visible) {
    echogram_.setVisible(visible);
    echogram_.resetCash();
    plotUpdate();
}

void Plot2D::setEchogramTheme(int theme_id) {
    echogram_.setThemeId(theme_id);
    plotUpdate();
}

void Plot2D::setEchogramCompensation(int compensation_id) {
    echogram_.setCompensation(compensation_id);
    echogram_.resetCash();
    plotUpdate();
}

void Plot2D::setBottomTrackVisible(bool visible) {
    bottomProcessing_.setVisible(visible);
    plotUpdate();
}

void Plot2D::setBottomTrackTheme(int theme_id) {
    Q_UNUSED(theme_id);
}

void Plot2D::setRangefinderVisible(bool visible) {
    rangefinder_.setVisible(visible);
    grid_.setRangeFinderVisible(visible);
    plotUpdate();
}

void Plot2D::setRangefinderTheme(int theme_id) {
    rangefinder_.setTheme(theme_id);
    plotUpdate();
}

void Plot2D::setAttitudeVisible(bool visible) {
    attitude_.setVisible(visible);
    plotUpdate();
}

void Plot2D::setDopplerBeamVisible(bool visible, int beam_filter) {
    dvlBeamVelocity_.setVisible(visible);
    dvlBeamVelocity_.setBeamFilter(beam_filter);
    plotUpdate();
}

void Plot2D::setDopplerInstrumentVisible(bool visible) {
    dvlSolution_.setVisible(visible);
    plotUpdate();
}

void Plot2D::setGNSSVisible(bool visible, int flags) {
    Q_UNUSED(flags);

    gnss_.setVisible(visible);
    plotUpdate();
}

void Plot2D::setGridVetricalNumber(int grids) {
    grid_.setVisible(grids > 0);
    grid_.setVetricalNumber(grids);
    plotUpdate();
}

void Plot2D::setGridFillWidth(bool state)
{
    grid_.setFillWidth(state);
    plotUpdate();
}

void Plot2D::setAngleVisibility(bool state)
{
    grid_.setAngleVisibility(state);
    plotUpdate();
}

void Plot2D::setAngleRange(int angleRange)
{
    cursor_.attitude.from = static_cast<float>(-angleRange);
    cursor_.attitude.to = static_cast<float>(angleRange);
    plotUpdate();
}

void Plot2D::setVelocityVisible(bool visible) {
    grid_.setVelocityVisible(visible);
    plotUpdate();
}

void Plot2D::setVelocityRange(float velocity) {
    cursor_.velocity.from = -velocity;
    cursor_.velocity.to = velocity;
    plotUpdate();
}

void Plot2D::setDistanceAutoRange(int auto_range_type) {
    cursor_.distance.mode = AutoRangeMode(auto_range_type);
}

void Plot2D::setDistance(float from, float to) {
    cursor_.distance.set(from, to);
}

void Plot2D::zoomDistance(float ratio)
{
    cursor_.distance.mode = AutoRangeNone;

    int  delta = ratio;
    if(delta == 0) return;

    float from = cursor_.distance.from;
    float to = cursor_.distance.to;
    float absrange = abs(to - from);

    float zoom = delta < 0 ? -delta*0.01f : delta*0.01f;
    float delta_range = absrange*zoom;
    float new_range = 0;

    if(delta_range < 0.1) {
        delta_range = 0.1;
    } else if(delta_range > 5) {
        delta_range = 5;
    }

    if(delta > 0) {
        new_range = absrange + delta_range;
    } else {
        new_range = absrange - delta_range;
    }

    if(new_range < 1) {
        new_range = 1;
    } else if(new_range > 500) {
        new_range = 500;
    }


    if (cursor_.isChannelDoubled()) {
        cursor_.distance.from = -ceil(new_range / 2);
        cursor_.distance.to = ceil(new_range / 2);
    }
    else {
       cursor_.distance.to = ceil(cursor_.distance.from + new_range);
    }

    plotUpdate();
}

void Plot2D::scrollDistance(float ratio)
{
    cursor_.distance.mode = AutoRangeNone;

    float from = cursor_.distance.from;
    float to = cursor_.distance.to;
    float absrange = abs(to - from);

    float delta_offset = ((float)absrange*(float)ratio*0.001f);

    if(from < to) {
        float round_cef = 10.0f;

        float from_n = (round((from + delta_offset)*round_cef)/round_cef);
        float to_n = (round((to + delta_offset)*round_cef)/round_cef);

        if(!cursor_.isChannelDoubled()) {
            if(from_n < 0) {
                to_n -= from_n;
                from_n = 0;
            }
        }

        cursor_.distance.from = from_n;
        cursor_.distance.to = to_n;

    } else if(from > to) {
        cursor_.distance.from = (from - delta_offset);
        cursor_.distance.to = (to - delta_offset);
    }

    plotUpdate();
}

void Plot2D::setMousePosition(int x, int y, bool isSync) {

    const int image_width = canvas_.width();
    const int image_height = canvas_.height();
    const int dataset_from = cursor_.getIndex(0);
    Q_UNUSED(dataset_from);

    const float distance_from = cursor_.distance.from;
    const float distance_range = cursor_.distance.to - cursor_.distance.from;
    const float image_distance_ratio = distance_range/(float)image_height;

    struct {
        int x = -1, y = -1;
    } _mouse;

    _mouse.x = cursor_.mouseX;
    _mouse.y = cursor_.mouseY;
    cursor_.setMouse(x, y);


    if(x < -1) { x = -1; }
    if(x >= image_width) { x = image_width - 1; }

    if(y < 0) { y = 0; }
    if(y >= image_height) { x = image_height - 1; }

    if(x == -1) {
        _mouse.x = -1;
        cursor_.selectEpochIndx = -1;
        cursor_.currentEpochIndx = -1;
        //_cursor.lastEpochIndx = -1; // ?
        plotUpdate();
        return;
    }

    int x_start = 0, y_start = 0;
    int x_length = 0;
    float y_scale = 0.0f;
    if(_mouse.x != -1) {
        if(_mouse.x < x) {
            x_length = x - _mouse.x;
            x_start = _mouse.x;
            y_start = _mouse.y;
            y_scale = (float)(y - _mouse.y)/(float)x_length;
        } else if(_mouse.x > x) {
            x_length = _mouse.x - x;
            x_start = x;
            y_start = y;
            y_scale = -(float)(y - _mouse.y)/(float)x_length;
        } else {
            x_length = 1;
            x_start = x;
            y_start = y;
            y_scale = 0;
        }
    } else {
        x_length = 1;
        x_start = x;
        y_start = y;
        y_scale = 0;
    }

//    _mouse.x = x;
//    _mouse.y = y;

    //qDebug() << "Cursor epoch" << cursor_.getIndex(x_start);
    int epoch_index = cursor_.getIndex(x_start);
    cursor_.currentEpochIndx = epoch_index;
    cursor_.lastEpochIndx = cursor_.currentEpochIndx;

    sendSyncEvent(epoch_index, EpochSelected2d);

    if(cursor_.tool() > MouseToolNothing && !isSync) {

        for(int x_ind = 0; x_ind < x_length; x_ind++) {
            int epoch_index = cursor_.getIndex(x_start + x_ind);

            Epoch* epoch = datasetPtr_->fromIndex(epoch_index);

            const ChannelId channel1 = cursor_.channel1;
            const ChannelId channel2 = cursor_.channel2;

            if(epoch != NULL) {

                float image_y_pos = ((float)y_start + (float)x_ind*y_scale);
                float dist = abs(image_y_pos*image_distance_ratio + distance_from);

                if(cursor_.tool() == MouseToolDistanceMin) {
                    epoch->setMinDistProc(channel1, dist);
                    epoch->setMinDistProc(channel2, dist);
                } else if(cursor_.tool() == MouseToolDistance) {
                    epoch->setDistProcessing(channel1, dist);
                    epoch->setDistProcessing(channel2, dist);
                } else if(cursor_.tool()== MouseToolDistanceMax) {
                    epoch->setMaxDistProc(channel1, dist);
                    epoch->setMaxDistProc(channel2, dist);
                } else if(cursor_.tool() == MouseToolDistanceErase) {
                    epoch->clearDistProcessing(channel1);
                    epoch->clearDistProcessing(channel2);
                }
            }
        }

        if (cursor_.tool() == MouseToolDistanceMin || cursor_.tool() == MouseToolDistanceMax) {
            if (auto btp = datasetPtr_->getBottomTrackParamPtr(); btp) {
                btp->indexFrom = cursor_.getIndex(x_start);
                btp->indexTo = cursor_.getIndex(x_start + x_length);
                datasetPtr_->bottomTrackProcessing(cursor_.channel1, cursor_.channel2);
            }
        }

        if(cursor_.tool() == MouseToolDistance || cursor_.tool() == MouseToolDistanceErase) {
            emit datasetPtr_->bottomTrackUpdated(cursor_.channel1, cursor_.getIndex(x_start), cursor_.getIndex(x_start + x_length));
        }
    }

    plotUpdate();
}

void Plot2D::simpleSetMousePosition(int x, int y)
{
    const int image_width = canvas_.width();
    const int image_height = canvas_.height();
    int mouseX = -1;

    if (x < -1) {
        x = -1;
    }
    if (x >= image_width) {
        x = image_width - 1;
    }
    if (y < 0) {
        y = 0;
    }
    if (y >= image_height) {
        x = image_height - 1;
    }

    if (x == -1) {
        //_cursor.selectEpochIndx = -1;
        cursor_.currentEpochIndx = -1;
        //_cursor.lastEpochIndx = -1; // ?
        return;
    }

    cursor_.setContactPos(x, y);

    int x_start = 0;
    if(mouseX != -1) {
        if(mouseX < x) {
            x_start = mouseX;
        }
        else if (mouseX > x) {
            x_start = x;
        }
        else {
            x_start = x;
        }
    }
    else {
        x_start = x;
    }

    cursor_.currentEpochIndx = cursor_.getIndex(x_start);
    cursor_.lastEpochIndx = cursor_.currentEpochIndx;

    //sendSyncEvent(epoch_index);
    //plotUpdate();
}

void Plot2D::setMouseTool(MouseTool tool) {
    cursor_.setTool(tool);
}

bool Plot2D::setContact(int indx, const QString& text)
{
    if (!datasetPtr_) {
        qDebug() << "Plot2D::setContact returned: !_dataset";
        return false;
    }

    if (text.isEmpty()) {
        qDebug() << "Plot2D::setContact returned: text.isEmpty()";
        return false;
    }

    bool primary = indx == -1;
    int currIndx = primary ? cursor_.lastEpochIndx : indx;

    //qDebug() << "indx" << indx << "currIndx" << currIndx << text;

    auto* ep = datasetPtr_->fromIndex(currIndx);
    if (!ep) {
        qDebug() << "Plot2D::setContact returned: !ep";
        return false;
    }

    ep->contact_.info = text;
    //qDebug() << "Plot2D::setContact: setted to epoch:" <<  currIndx << text;


    if (primary) {
        ep->contact_.cursorX = cursor_.contactX;
        ep->contact_.cursorY = cursor_.contactY;

        const float canvas_height = canvas_.height();
        float value_range = cursor_.distance.to - cursor_.distance.from;
        float value_scale = float(cursor_.contactY) / canvas_height;
        float cursor_distance = value_scale * value_range + cursor_.distance.from;

        ep->contact_.distance = cursor_distance;

        auto pos = ep->getPositionGNSS();

        ep->contact_.nedX = pos.ned.n;
        ep->contact_.nedY = pos.ned.e;

        ep->contact_.lat = pos.lla.latitude;
        ep->contact_.lon = pos.lla.longitude;
    }
    else {
        // update rect
    }

    sendSyncEvent(currIndx, ContactCreated);

    plotUpdate();

    return true;
}

bool Plot2D::deleteContact(int indx)
{
    if (!datasetPtr_) {
        qDebug() << "Plot2D::deleteContact returned: !_dataset";
        return false;
    }

    //qDebug() << "indx" << indx << "currIndx" << currIndx << text;

    auto* ep = datasetPtr_->fromIndex(indx);
    if (!ep) {
        qDebug() << "Plot2D::deleteContact returned: !ep";
        return false;
    }

    ep->contact_.clear();

    sendSyncEvent(indx, ContactDeleted);

    plotUpdate();

    return true;
}

void Plot2D::updateContact()
{
    contacts_.setMousePos(-1,-1);
    plotUpdate();
}

void Plot2D::onCursorMoved(int x, int y)
{
    if (isHorizontal_) {
        contacts_.setMousePos(x, y);
    } 
    else {
        const int horX = canvas_.width() - 1 - y;
        const int horY = x;

        const int clampedX = std::clamp(horX, 0, canvas_.width() - 1);
        const int clampedY = std::clamp(horY, 0, canvas_.height() - 1);
        contacts_.setMousePos(clampedX, clampedY);
    }

    plotUpdate();
}

Canvas &Plot2D::canvas() { return canvas_; }

DatasetCursor &Plot2D::cursor() { return cursor_; }

void Plot2D::resetCash() {
    echogram_.resetCash();
}

void Plot2D::plotUpdate() {}

void Plot2D::sendSyncEvent(int epoch_index, QEvent::Type eventType) {
    Q_UNUSED(epoch_index);
    Q_UNUSED(eventType);
}

void Plot2D::reindexingCursor() {
    if(datasetPtr_ == nullptr) { return; }

    const int image_width = canvas_.width();
    const int data_width = datasetPtr_->size();
    const int last_indexes_size = cursor_.indexes.size();

    if(image_width != last_indexes_size) {
        cursor_.indexes.resize(image_width);
    }

    if(cursor_.last_dataset_size > 0) {
        float position = timelinePosition();

        float last_head = round(position*cursor_.last_dataset_size);
        float last_offset_head = float(cursor_.last_dataset_size) - last_head;
        float new_head = data_width - last_offset_head;

        position = float(new_head)/float(data_width);

        setTimelinePosition(position);
    }
    cursor_.last_dataset_size = data_width;

    float hor_ratio = 1.0f;

    float position = timelinePosition();

    int head_data_index = round(position*float(data_width));

    int cntZeros = 0;
    for(int i = 0; i < image_width; i++) {
        int data_index = head_data_index + round((i - image_width)/hor_ratio) - 1;
        if(data_index >= 0 && data_index < data_width) {
             cursor_.indexes[i] = data_index;
        } else {
            ++cntZeros;
             cursor_.indexes[i] = -1;
        }
    }
    cursor_.numZeroEpoch = cntZeros;
}

void Plot2D::reRangeDistance()
{
    if (datasetPtr_ == NULL) {
        return;
    }

    float max_range = NAN;

    if (cursor_.distance.mode == AutoRangeLastData) {
        for (int i = datasetPtr_->endIndex() - 3; i < datasetPtr_->endIndex(); i++) {
            Epoch* epoch = datasetPtr_->fromIndex(i);
            if (epoch != NULL) {
                float epoch_range = epoch->getMaxRange(cursor_.channel1);
                if (!isfinite(max_range) || max_range < epoch_range) {
                    max_range = epoch_range;
                }
            }
        }
    }

    if(cursor_.distance.mode == AutoRangeLastOnScreen) {
        for(unsigned int i = cursor_.indexes.size() - 3; i < cursor_.indexes.size(); i++) {
            Epoch* epoch = datasetPtr_->fromIndex(cursor_.getIndex(i));
            if(epoch != NULL) {
                float epoch_range = epoch->getMaxRange(cursor_.channel1);
                if(!isfinite(max_range) || max_range < epoch_range) {
                    max_range = epoch_range;
                }
            }
        }
    }

    if(cursor_.distance.mode == AutoRangeMaxOnScreen) {
        for(unsigned int i = 0; i < cursor_.indexes.size(); i++) {
            Epoch* epoch = datasetPtr_->fromIndex(cursor_.getIndex(i));
            if(epoch != NULL) {
                float epoch_range = epoch->getMaxRange(cursor_.channel1);
                if(!isfinite(max_range) || max_range < epoch_range) {
                    max_range = epoch_range;
                }
            }
        }
    }

    if (isfinite(max_range)) {
        const float dist = std::round(std::abs(max_range));
        cursor_.distance.to = dist;

        if (cursor_.isChannelDoubled()) {
            cursor_.distance.from = -dist;
        }
        else {
            cursor_.distance.from = 0;
        }
    }
}

float Plot2D::timelinePosition()
{
    return cursor_.position;
}
