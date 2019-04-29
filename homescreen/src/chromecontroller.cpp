#include "chromecontroller.h"
#include "aglsocketwrapper.h"
#include "constants.h"

#include <QTimer>
#include <QDebug>
#include <QJsonDocument>

ChromeController::ChromeController(const QUrl &bindingUrl, QObject *parent) :
    QObject(parent)
  , m_aglSocket(new AglSocketWrapper(this))
{
    //Alexa voice agent subscription----------------------------------------------------------------
    {
    connect(m_aglSocket, &AglSocketWrapper::connected,
            this, [this]() -> void {
                m_aglSocket->apiCall(vshl::API, vshl::VOICE_AGENT_ENUMERATION_VERB, QJsonValue(),
                [this](bool result, const QJsonValue &data) -> void {
                    if (!result) {
                        qWarning() << "Failed to enumerate voice agents";
                        return;
                    }

                    QJsonObject dataObj = data.toObject();
                    auto objIt = dataObj.find(vshl::RESPONSE_TAG);
                    if (objIt == dataObj.constEnd()) {
                        qWarning() << "Voice agent enumeration response tag missing."
                                   << dataObj;
                        return;
                    }

                    dataObj = objIt.value().toObject();
                    objIt = dataObj.find(vshl::AGENTS_TAG);
                    if (objIt == dataObj.constEnd()) {
                        qWarning() << "Voice agent enumeration agents tag missing."
                                   << dataObj;
                        return;
                    }

                    const QJsonArray agents = objIt.value().toArray();
                    QString alexaAgentId;
                    for (const QJsonValue &agent : agents) {
                        const QJsonObject agentObj = agent.toObject();
                        auto agentIt = agentObj.find(vshl::NAME_TAG);
                        if (agentIt == agentObj.constEnd())
                            continue;
                        const QString agentName = agentIt.value().toString();
                        agentIt = agentObj.find(vshl::ID_TAG);
                        if (agentIt == agentObj.constEnd())
                            continue;
                        if (agentName.compare(vshl::ALEXA_AGENT_NAME) == 0) {
                            alexaAgentId = agentIt.value().toString();
                            break;
                        }
                    }
                    if (alexaAgentId.isEmpty()) {
                        qWarning() << "Alexa voice agent not found";
                        return;
                    }

                    //Voice agent subscription------------------------------------------------------
                    {
                        m_voiceAgentId = alexaAgentId;
                        const QJsonObject args {
                            { vshl::VOICE_AGENT_ID_ARG, alexaAgentId },
                            { vshl::VOICE_AGENT_EVENTS_ARG, vshl::ALEXA_EVENTS_ARRAY }
                        };
                        m_aglSocket->apiCall(vshl::API, vshl::SUBSCRIBE_VERB, args,
                        [](bool result, const QJsonValue &data) -> void {
                            qDebug() << (vshl::API + QLatin1String(":") + vshl::SUBSCRIBE_VERB)
                                     << "result: " << result << " val: " << data;
                        });
                    }
                    //------------------------------------------------------------------------------
                });
            });
    }
    //----------------------------------------------------------------------------------------------<

    //Socket connection management------------------------------------------------------------------
    {
    auto connectToBinding = [bindingUrl, this]() -> void {
        m_aglSocket->open(bindingUrl);
        qDebug() << "Connecting to:" << bindingUrl;
    };
    connect(m_aglSocket, &AglSocketWrapper::disconnected, this, [connectToBinding]() -> void {
                QTimer::singleShot(2500, connectToBinding);
            });
    connectToBinding();
    }
    //----------------------------------------------------------------------------------------------

    //Speech chrome state change event handling-----------------------------------------------------
    {
    connect(m_aglSocket, &AglSocketWrapper::eventReceived,
            this, [this](const QString &eventName, const QJsonValue &data) -> void {
        if (eventName.compare(vshl::VOICE_DIALOG_STATE_EVENT + m_voiceAgentId) == 0) {
            const QJsonObject dataObj = QJsonDocument::fromJson(data.toString().toUtf8()).object();
            auto objIt = dataObj.find(vshl::STATE_TAG);
            if (objIt == dataObj.constEnd()) {
                qWarning() << "Voice dialog state event state missing.";
                return;
            }
            const QString stateStr = objIt.value().toString();
            if (stateStr.compare(vshl::VOICE_DIALOG_IDLE) == 0) {
                setChromeState(Idle);
            } else if (stateStr.compare(vshl::VOICE_DIALOG_LISTENING) == 0) {
                setChromeState(Listening);
            } else if (stateStr.compare(vshl::VOICE_DIALOG_THINKING) == 0) {
                setChromeState(Thinking);
            } else if (stateStr.compare(vshl::VOICE_DIALOG_SPEAKING) == 0) {
                setChromeState(Speaking);
            } else if (stateStr.compare(vshl::VOICE_DIALOG_MICROPHONEOFF) == 0) {
                setChromeState(MicrophoneOff);
            }
        }
    });
    }
    //----------------------------------------------------------------------------------------------
}

void ChromeController::pushToTalk()
{
    m_aglSocket->apiCall(vshl::API, vshl::TAP_TO_TALK_VERB, QJsonValue(),
                         [](bool result, const QJsonValue &data) -> void {
        qDebug() << (vshl::API + QLatin1String(":") + vshl::TAP_TO_TALK_VERB)
                 << "result: " << result << " val: " << data;
    });
}

void ChromeController::setChromeState(ChromeController::ChromeState state)
{
    if (m_chromeState != state) {
        m_chromeState = state;
        emit chromeStateChanged();
    }
}
