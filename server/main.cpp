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
            
            if (!document.isObject()) {
                return;
            }

            const QJsonObject request = document.object();
            const QString messageType = request.value(QStringLiteral("type")).toString();
            
            if (messageType == QStringLiteral("register")) {
                const QString requestedId = request.value(QStringLiteral("clientId")).toString().trimmed();
                QJsonObject response;
                response[QStringLiteral("type")] = QStringLiteral("registration");

                if (requestedId.isEmpty() || idsByClient.contains(client) || clientsById.contains(requestedId)) {
                    response[QStringLiteral("status")] = QStringLiteral("rejected");
                } else {
                    clientsById.insert(requestedId, client);
                    idsByClient.insert(client, requestedId);

                    response[QStringLiteral("status")] = QStringLiteral("accepted");

                    qInfo() << "Registered client:" << requestedId;
                }

                const QByteArray json = QJsonDocument(response).toJson(QJsonDocument::Compact);
                client->sendTextMessage(QString::fromUtf8(json));
                return;
            }

            if (messageType == QStringLiteral("chat")) {
                if (!idsByClient.contains(client)) {
                    qWarning() << "Ignoring message from unregistered client";
                    return;
                }

                const QString chatText = request.value(QStringLiteral("message")).toString().trimmed();
                if (chatText.isEmpty()) {
                    return;
                }

                const QString senderId = idsByClient.value(client);

                QJsonObject outgoingMessage;
                outgoingMessage[QStringLiteral("type")] = QStringLiteral("chat");
                outgoingMessage[QStringLiteral("senderId")] = senderId;
                outgoingMessage[QStringLiteral("message")] = chatText;

                const QString outgoingJson = QString::fromUtf8(QJsonDocument(outgoingMessage).toJson(QJsonDocument::Compact));

                for (QWebSocket *recipient : clientsById) {
                    recipient->sendTextMessage(outgoingJson);
                }

                qInfo() << "Broadcast message from:" << senderId;
            }
        });
        
        QObject::connect(client, &QWebSocket::disconnected, client, [client, &clientsById, &idsByClient]() {
            const QString clientId = idsByClient.take(client);

            if (!clientId.isEmpty()) {
                clientsById.remove(clientId);
                qInfo() << "Removed client:" << clientId;
            }

            qInfo() << "Client disconnected";
            client->deleteLater();
        });
    });

    return application.exec();
}