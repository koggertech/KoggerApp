#ifndef QMLOBJECTNAMES_H
#define QMLOBJECTNAMES_H

#include <QString>

namespace QmlObjectNames{
    inline QString activeObjectParamsMenuLoaderName () { return QStringLiteral("activeObjectParamsMenuLoader"); };
    inline QString bottomTrackControlMenu           () { return QStringLiteral("bottomTrackControlMenu"); };
    inline QString surfaceControlMenu               () { return QStringLiteral("surfaceControlMenu"); };
    inline QString mosaicViewControlMenu            () { return QStringLiteral("mosaicViewControlMenu"); };
    inline QString npdFilterControlMenu             () { return QStringLiteral("npdFilterControlMenu"); };
    inline QString mpcFilterControlMenu             () { return QStringLiteral("mpcFilterControlMenu"); };
    inline QString bottomTrackParamsMenu            () { return QStringLiteral("bottomTrackParamsMenu"); };
    inline QString surfaceParamsMenu                () { return QStringLiteral("surfaceParamsMenu"); };
    inline QString scene3dToolBar                   () { return QStringLiteral("scene3dToolBar"); };
}

#endif // QMLOBJECTNAMES_H
