#ifndef QMLOBJECTNAMES_H
#define QMLOBJECTNAMES_H

#include <QString>

namespace QmlObjectNames{
    inline QString activeObjectParamsMenuLoaderName () { return QStringLiteral("activeObjectParamsMenuLoader"); };
    inline QString bottomTrackControlMenu           () { return QStringLiteral("bottomTrackControlMenu"); };
    inline QString surfaceControlMenu               () { return QStringLiteral("surfaceControlMenu"); };
    inline QString sideScanViewControlMenu          () { return QStringLiteral("sideScanViewControlMenu"); };
    inline QString imageViewControlMenu             () { return QStringLiteral("imageViewControlMenu"); };
    inline QString mapViewControlMenu               () { return QStringLiteral("mapViewControlMenu"); };
    inline QString npdFilterControlMenu             () { return QStringLiteral("npdFilterControlMenu"); };
    inline QString mpcFilterControlMenu             () { return QStringLiteral("mpcFilterControlMenu"); };
    inline QString bottomTrackParamsMenu            () { return QStringLiteral("bottomTrackParamsMenu"); };
    inline QString surfaceParamsMenu                () { return QStringLiteral("surfaceParamsMenu"); };
    inline QString scene3dToolBar                   () { return QStringLiteral("scene3dToolBar"); };
    inline QString usblViewControlMenu              () { return QStringLiteral("usblViewControlMenu"); };
}

#endif // QMLOBJECTNAMES_H
