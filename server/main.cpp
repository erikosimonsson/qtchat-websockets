#include <QCoreApplication>
#include <QDebug>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>

int main(int argc, char **argv) {
    QCoreApplication application(argc, argv);
    constexpr quint16 port = 12345;

    QHash<QString, QWebSocket *> clientsById;
    QHash<QWebSocket *, QString> idsByClient;

    QWebSocketServer server(
        QStringLiteral("Qt Chat Server"),
        QWebSocketServer::NonSecureMode
    );

    if (!server.listen(QHostAddress::LocalHost, port)) {
        qCritical() << "Could not start server: " << server.errorString();
        return 1;
    }

    qInfo().noquote() << QStringLiteral("Chat server listening at ws://127.0.0.1:%1").arg(server.serverPort());

    QObject::connect(&server, &QWebSocketServer::newConnection, [&server, &clientsById, &idsByClient]() {
        QWebSocket *client = server.nextPendingConnection();

        if (client == nullptr) {
            return;
        }

        qInfo() << "Client connected from"
                << client->peerAddress().toString()
                << "port"
                << client->peerPort();

        QObject::connect(client, &QWebSocket::textMessageReceived, client, [client, &clientsById, &idsByClient](const QString &message) {
            const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8());
            const QJsonObject request = document.object();
            const QString messageType = request.value(QStringLiteral("type")).toString();
            const QString clientId = request.value(QStringLiteral("clientId")).toString().trimmed();

            QJsonObject response;
            response[QStringLiteral("type")] = QStringLiteral("registration");

            if (!document.isObject() || messageType != QStringLiteral("register") || clientId.isEmpty() || idsByClient.contains(client) || clientsById.contains(clientId)) {
                response[QStringLiteral("status")] = QStringLiteral("rejected");
            } else {
                clientsById.insert(clientId, client);
                idsByClient.insert(client, clientId);

                response[QStringLiteral("status")] = QStringLiteral("accepted");
                qInfo() << "Registered client:" << clientId;
            }

            const QByteArray json = QJsonDocument(response).toJson(QJsonDocument::Compact);

            client->sendTextMessage(QString::fromUtf8(json));
        });
        
        QObject::connect(client, &QWebSocket::disconnected, client, [client, &clientsById, &idsByClient]() {
            const QString clientId = idsByClient.take(client);

            if (!clientId.isEmpty()) {
                const QString clientId = idsByClient.take(client);

                if (!clientId.isEmpty()) {
                    clientsById.remove(clientId);
                    qInfo() << "Removed client:" << clientId;
                }

                qInfo() << "Client disconnected";
                client->deleteLater();
            }
        });
    });

    return application.exec();
}