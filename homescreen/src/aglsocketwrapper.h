#ifndef AGLSOCKETWRAPPER_H
#define AGLSOCKETWRAPPER_H

#include <QUrl>
#include <QMap>
#include <QObject>
#include <QJsonValue>

#include <functional>

class QWebSocket;
class AglSocketWrapper : public QObject
{
    Q_OBJECT
public:
    explicit AglSocketWrapper(QObject *parent = nullptr);

    void open(const QUrl &url);
    void close();

    using ApiCallback = std::function<void(bool, const QJsonValue&)>;
    void apiCall(const QString &api, const QString &verb, const QJsonValue &args = QJsonValue(),
                 ApiCallback callback = nullptr);

signals:
    void connected();
    void disconnected();
    void eventReceived(const QString &eventName, const QJsonValue &data);

private:
    QWebSocket *m_socket;
    QMap<QString, ApiCallback> m_callbacks;
};

#endif // AGLSOCKETWRAPPER_H
