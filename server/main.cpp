#include <QCoreApplication>
#include <QDebug>
#include <QHostAddress>
#include <QtWebSockets/QWebSocketServer>

int main(int argc, char **argv) {
    QCoreApplication application(argc, argv);
    constexpr quint16 port = 12345;

    QWebSocketServer server(
        QStringLiteral("Qt Chat Server"),
        QWebSocketServer::NonSecureMode
    );

    if (!server.listen(QHostAddress::LocalHost, port)) {
        qCritical() << "Could not start server: " << server.errorString();
        return 1;
    }

    qInfo().noquote() << QStringLiteral("Chat server listening at ws://127.0.0.1:%1").arg(server.serverPort());

    return application.exec();
}