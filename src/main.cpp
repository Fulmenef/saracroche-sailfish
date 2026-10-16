#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QQmlContext>
#include <QQuickView>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>

#include <MDConfItem>
#include <sailfishapp.h>

#include <memory>

#include "patterns.h"

static const QString ApiHost = QStringLiteral("https://app.saracroche.org");
static const QString ApiBase = ApiHost + QStringLiteral("/api/v2");
static const QString RejectedKey = QStringLiteral("/sailfish/voicecall/filter/rejected-numbers");
static const QString WhitelistKey = QStringLiteral("/sailfish/voicecall/filter/whitelist");
static const QString Anonymous = QStringLiteral("anonymous");
static const int TimeoutMs = 30000;

class Saracroche : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QDateTime lastSync READ lastSync NOTIFY changed)
    Q_PROPERTY(int entryCount READ entryCount NOTIFY changed)
    Q_PROPERTY(int blockedTotal READ blockedTotal NOTIFY changed)
    Q_PROPERTY(bool blockAnonymous READ blockAnonymous WRITE setBlockAnonymous NOTIFY changed)

public:
    Saracroche()
        : m_settings(QStringLiteral("saracroche-sailfish"), QStringLiteral("saracroche-sailfish"))
        , m_rejected(RejectedKey)
        , m_whitelist(WhitelistKey)
    {
        connect(&m_rejected, &MDConfItem::valueChanged, this, &Saracroche::changed);
    }

    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    QDateTime lastSync() const { return m_settings.value(QStringLiteral("lastSync")).toDateTime(); }
    int entryCount() const { return m_settings.value(QStringLiteral("ownedRejected")).toStringList().size(); }
    int blockedTotal() const { return m_settings.value(QStringLiteral("blockedTotal"), 0).toInt(); }
    bool blockAnonymous() const { return m_rejected.value().toStringList().contains(Anonymous); }

    void setBlockAnonymous(bool block)
    {
        QStringList list = m_rejected.value().toStringList();
        list.removeAll(Anonymous);
        if (block)
            list.prepend(Anonymous);
        m_rejected.set(list);
        m_rejected.sync();
        emit changed();
    }

    Q_INVOKABLE void start()
    {
        countBlocked();
        if (!m_settings.contains(QStringLiteral("lastSync")))
            sync();
    }

    Q_INVOKABLE void sync()
    {
        if (m_busy)
            return;
        setState(true, QString());
        setTimerEnabled(true);

        QNetworkReply *reply = get(QUrl(ApiBase + QStringLiteral("/lists")));
        connect(reply, &QNetworkReply::finished, this, [this, reply] {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError)
                return fail(reply->errorString());
            downloadLists(QJsonDocument::fromJson(reply->readAll()).array());
        });
    }

    Q_INVOKABLE void removeAll()
    {
        setTimerEnabled(false);
        writeList(m_rejected, QStringLiteral("ownedRejected"), QStringList());
        writeList(m_whitelist, QStringLiteral("ownedWhitelist"), QStringList());
        emit changed();
    }

    Q_INVOKABLE void report(const QString &number, bool isGood)
    {
        QString deviceId = m_settings.value(QStringLiteral("deviceId")).toString();
        if (deviceId.isEmpty()) {
            deviceId = QUuid::createUuid().toString().mid(1, 36);
            m_settings.setValue(QStringLiteral("deviceId"), deviceId);
        }

        QNetworkRequest request = makeRequest(QUrl(ApiBase + QStringLiteral("/reports")));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        request.setRawHeader("X-Device-Id", deviceId.toUtf8());
        const QJsonObject body { { QStringLiteral("phone"), number.trimmed() }, { QStringLiteral("is_good"), isGood } };
        QNetworkReply *reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
        QTimer::singleShot(TimeoutMs, reply, &QNetworkReply::abort);

        connect(reply, &QNetworkReply::finished, this, [this, reply] {
            reply->deleteLater();
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status >= 200 && status < 300)
                emit reportFinished(true, QStringLiteral("Merci, signalement envoyé."));
            else if (status == 422)
                emit reportFinished(false, QStringLiteral("Numéro invalide."));
            else if (status == 429)
                emit reportFinished(false, QStringLiteral("Trop de signalements, réessayez plus tard."));
            else
                emit reportFinished(false, QStringLiteral("Échec de l'envoi : ") + reply->errorString());
        });
    }

signals:
    void changed();
    void syncFinished(bool ok);
    void reportFinished(bool ok, const QString &message);

private:
    QNetworkRequest makeRequest(const QUrl &url) const
    {
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        request.setRawHeader("X-Country-Code", "FR");
        request.setRawHeader("User-Agent", "saracroche-sailfish");
        return request;
    }

    QNetworkReply *get(const QUrl &url)
    {
        QNetworkReply *reply = m_network.get(makeRequest(url));
        QTimer::singleShot(TimeoutMs, reply, &QNetworkReply::abort);
        return reply;
    }

    void downloadLists(const QJsonArray &lists)
    {
        // Saracroche applies the list with the lowest priority number first, so an allow
        // list ranked after every block list never changes a decision.
        int worstBlockPriority = -1;
        for (const QJsonValue &value : lists) {
            const QJsonObject list = value.toObject();
            if (list.value(QStringLiteral("type")).toString() == QLatin1String("block"))
                worstBlockPriority = qMax(worstBlockPriority, list.value(QStringLiteral("priority")).toInt());
        }
        if (worstBlockPriority < 0)
            return fail(QStringLiteral("Aucune liste de blocage reçue."));

        QList<QJsonObject> selected;
        for (const QJsonValue &value : lists) {
            const QJsonObject list = value.toObject();
            const QString type = list.value(QStringLiteral("type")).toString();
            if (list.value(QStringLiteral("channel")).toString() == QLatin1String("phone")
                && (type == QLatin1String("block")
                    || (type == QLatin1String("allow") && list.value(QStringLiteral("priority")).toInt() < worstBlockPriority)))
                selected.append(list);
        }

        auto pending = std::make_shared<int>(selected.size());
        auto rejected = std::make_shared<QStringList>();
        auto whitelist = std::make_shared<QStringList>();
        auto failure = std::make_shared<QString>();

        for (const QJsonObject &list : selected) {
            const bool block = list.value(QStringLiteral("type")).toString() == QLatin1String("block");
            const QUrl url = QUrl(ApiHost).resolved(QUrl(list.value(QStringLiteral("download_url")).toString()));
            QNetworkReply *reply = get(url);
            connect(reply, &QNetworkReply::finished, this,
                    [this, reply, block, pending, rejected, whitelist, failure] {
                reply->deleteLater();
                if (reply->error() != QNetworkReply::NoError) {
                    *failure = reply->errorString();
                } else {
                    QStringList &target = block ? *rejected : *whitelist;
                    for (const QByteArray &line : reply->readAll().split('\n')) {
                        const QString pattern = QJsonDocument::fromJson(line).object()
                                .value(QStringLiteral("pattern")).toString();
                        const QString entry = toFilterEntry(pattern);
                        if (!entry.isEmpty())
                            target.append(entry);
                    }
                }
                if (--*pending > 0)
                    return;
                // Never write partial lists: that would unblock the numbers of the missing list.
                if (!failure->isEmpty())
                    return fail(*failure);
                if (rejected->isEmpty())
                    return fail(QStringLiteral("Listes de blocage vides."));
                apply(*rejected, *whitelist);
            });
        }
    }

    void apply(const QStringList &rejected, const QStringList &whitelist)
    {
        writeList(m_rejected, QStringLiteral("ownedRejected"), rejected);
        writeList(m_whitelist, QStringLiteral("ownedWhitelist"), whitelist);
        m_settings.setValue(QStringLiteral("lastSync"), QDateTime::currentDateTime());
        m_settings.sync();
        countBlocked();
        setState(false, QString());
        emit syncFinished(true);
    }

    void writeList(MDConfItem &item, const QString &ownedKey, const QStringList &fresh)
    {
        const QStringList owned = m_settings.value(ownedKey).toStringList();
        item.set(merge(item.value().toStringList(), owned, fresh));
        item.sync();
        m_settings.setValue(ownedKey, fresh);
    }

    void countBlocked()
    {
        const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                + QStringLiteral("/commhistory/commhistory.db");
        if (!QFile::exists(path))
            return;

        const QString connection = QStringLiteral("commhistory");
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
            db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
            db.setDatabaseName(path);
            if (db.open()) {
                // shortcut: counts every rejected call, including the user's own manual blocks;
                // match remoteUid against the owned entries if that matters
                QSqlQuery query(db);
                query.prepare(QStringLiteral(
                    "SELECT COUNT(*), MAX(e.id) FROM Events e JOIN EventProperties p ON p.eventId = e.id "
                    "WHERE p.key = 'incomingStatus' AND CAST(p.value AS TEXT) = 'rejected' AND e.id > ?"));
                query.addBindValue(m_settings.value(QStringLiteral("lastEventId"), 0).toLongLong());
                if (query.exec() && query.next() && query.value(0).toInt() > 0) {
                    m_settings.setValue(QStringLiteral("blockedTotal"), blockedTotal() + query.value(0).toInt());
                    m_settings.setValue(QStringLiteral("lastEventId"), query.value(1).toLongLong());
                }
            }
        }
        QSqlDatabase::removeDatabase(connection);
        emit changed();
    }

    void setTimerEnabled(bool enabled)
    {
        // shortcut: systemctl errors are ignored, the next manual update shows if blocking still works
        QProcess::startDetached(QStringLiteral("systemctl"),
                                { QStringLiteral("--user"),
                                  enabled ? QStringLiteral("enable") : QStringLiteral("disable"),
                                  QStringLiteral("--now"), QStringLiteral("saracroche-sync.timer") });
    }

    void setState(bool busy, const QString &error)
    {
        m_busy = busy;
        m_error = error;
        emit changed();
    }

    void fail(const QString &error)
    {
        setState(false, error);
        emit syncFinished(false);
    }

    QSettings m_settings;
    MDConfItem m_rejected;
    MDConfItem m_whitelist;
    QNetworkAccessManager m_network;
    bool m_busy = false;
    QString m_error;
};

int main(int argc, char *argv[])
{
    // Headless mode used by the daily systemd timer.
    if (argc > 1 && QByteArray(argv[1]) == "--sync") {
        QCoreApplication app(argc, argv);
        Saracroche saracroche;
        QObject::connect(&saracroche, &Saracroche::syncFinished, &app, [&app, &saracroche](bool ok) {
            if (!ok)
                qWarning("%s", qPrintable(saracroche.error()));
            app.exit(ok ? 0 : 1);
        });
        saracroche.sync();
        return app.exec();
    }

    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    Saracroche saracroche;
    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty(QStringLiteral("saracroche"), &saracroche);
    view->setSource(SailfishApp::pathToMainQml());
    view->show();
    saracroche.start();
    return app->exec();
}

#include "main.moc"
