#include "aglsocketwrapper.h"
#include "constants.h"

#include <QWebSocket>
#include <QUuid>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include <QDebug>

namespace {
enum MessageTypes {
    Call = 2,
    Success = 3,
    Error = 4,
    Event = 5
};
}

AglSocketWrapper::AglSocketWrapper(QObject *parent) :
    QObject(parent)
  , m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
{
    connect(m_socket, &QWebSocket::connected, this, &AglSocketWrapper::connected);
    connect(m_socket, &QWebSocket::disconnected, this, &AglSocketWrapper::disconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            [](QAbstractSocket::SocketError error) -> void {
        qWarning() << "AglSocketWrapper internal socket error" << error;
    });
    connect(m_socket, &QWebSocket::textMessageReceived,
            this, [this](const QString &msg) -> void {
        const QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
        if (doc.isArray()) {
            const QJsonArray msgArray = doc.array();
            if (msgArray.count() >= 3) {
                const int msgType = msgArray.at(0).toInt();
                switch (msgType) {
                case Success:
                case Error: {
                    auto callbackIt = m_callbacks.find( msgArray.at(1).toString());
                    if (callbackIt != m_callbacks.constEnd()) {
                        (*callbackIt)(msgType == Success, msgArray.at(2));
                        m_callbacks.erase(callbackIt);
                    }
                }
                    break;
                case Event: {
                    const QJsonObject eventObj = msgArray.at(2).toObject();
                    emit eventReceived(msgArray.at(1).toString(), eventObj.value(vshl::DATA_TAG));
                }
                    break;
                default:
                    break;
                }
                return;
            }
        }
        qWarning() << "Unsupported message format:" << msg;
    });
}

void AglSocketWrapper::open(const QUrl &url)
{
    m_socket->open(url);
}

void AglSocketWrapper::close()
{
    m_socket->close();
}

void AglSocketWrapper::apiCall(const QString &api, const QString &verb, const QJsonValue &args,
                               AglSocketWrapper::ApiCallback callback)
{
    const QString id = QUuid::createUuid().toString();
    if (callback)
        m_callbacks.insert(id, callback);

    QJsonArray callData;
    callData.append(Call);
    callData.append(id);
    callData.append(api + QLatin1String("/") + verb);
    callData.append(args);

    const QString msg = QLatin1String(QJsonDocument(callData).toJson(QJsonDocument::Compact));
    m_socket->sendTextMessage(msg);

    qDebug() << Q_FUNC_INFO << "Data sent:" << msg;
}
