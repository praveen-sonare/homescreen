#pragma once

#include <QObject>
#include <QUrl>

class AglSocketWrapper;
class ChromeController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool agentPresent READ agentPresent NOTIFY agentPresentChanged)
    Q_PROPERTY(int chromeState READ chromeState NOTIFY chromeStateChanged)

public:
    enum ChromeState {
        Idle = 0,
        Listening,
        Thinking,
        Speaking,
        MicrophoneOff
    };
    Q_ENUM(ChromeState)

    explicit ChromeController(const QUrl &bindingUrl, QObject *parent = nullptr);
    bool agentPresent() const { return m_agentPresent; }
    int chromeState() const { return m_chromeState; }

public slots:
    void pushToTalk();

signals:
    void agentPresentChanged();
    void chromeStateChanged();

private:
    void setChromeState(ChromeState state);

    AglSocketWrapper *m_aglSocket;
    QString m_voiceAgentId;
    bool m_agentPresent = false;
    ChromeState m_chromeState = Idle;
};
