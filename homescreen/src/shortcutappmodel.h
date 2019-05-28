#ifndef SHORTCUTAPPMODEL_H
#define SHORTCUTAPPMODEL_H

#include <QtCore/QAbstractListModel>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QFile>
#include <QProcess>
#include <QThread>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <json_object.h>

struct RegisterApp {
    QString id;
    QString name;
    QString icon;
    bool isBlank;
};

class ShortcutAppModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ShortcutAppModel(QObject *parent = nullptr);
    ~ShortcutAppModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QString getId(int index) const;
    Q_INVOKABLE QString getName(int index) const;
    Q_INVOKABLE QString getIcon(int index) const;
    Q_INVOKABLE bool isBlank(int index) const;

    void screenUpdated();

public slots:
    void changeShortcut(QString id, QString name, QString position);

signals:
    void updateShortcut();
    void shortcutUpdated(QString id, struct json_object* object);

private:
    void getAppQueue();
    void setAppQueue();
    bool checkAppFile();
    void setAppQueuePoint(QString id, QString name);
    QString getIconPath(QString id);
    struct json_object* makeAppListJson();

    class Private;
    Private *d;
    RegisterApp app;

};

#endif // SHORTCUTAPPMODEL_H
