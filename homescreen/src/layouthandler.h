/*
 * Copyright (C) 2016, 2017 Mentor Graphics Development (Deutschland) GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LAYOUTHANDLER_H
#define LAYOUTHANDLER_H

#include <QObject>
#include "windowmanager_proxy.h"
#include "popup_proxy.h"

class LayoutHandler : public QObject
{
    Q_OBJECT
public:
    explicit LayoutHandler(QObject *parent = 0);
    ~LayoutHandler();

signals:

public slots:
    void showAppLayer(const QString &app_id, int pid);
    void hideAppLayer();
    void makeMeVisible(int pid);
private:
    void checkToDoQueue();
public slots:
    int requestGetSurfaceStatus(int surfaceId);
    void requestRenderSurfaceToArea(int surfaceId, int layoutArea);
    bool requestRenderSurfaceToAreaAllowed(int surfaceId, int layoutArea);
    void requestSurfaceIdToFullScreen(int surfaceId);
    void setLayoutByName(QString layoutName);

    // this will receive the surfaceVisibilityChanged signal of the windowmanager
    void requestSurfaceVisibilityChanged(int surfaceId, bool visible);

Q_SIGNALS: // SIGNALS
    void surfaceVisibilityChanged(int surfaceId, bool visible);

protected:
    void timerEvent(QTimerEvent *e);
private:
    int m_secondsTimerId;
    org::agl::windowmanager *mp_dBusWindowManagerProxy;
    org::agl::popup *mp_dBusPopupProxy;

    QList<int> m_requestsToBeVisiblePids;
    QList<int> m_visibleSurfaces;
    QList<int> m_invisibleSurfaces;
    QList<int> m_requestsToBeVisibleSurfaces;
};

#endif // LAYOUTHANDLER_H
