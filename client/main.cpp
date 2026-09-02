#include <QAbstractSocket>
#include <QApplication>
#include <QLabel>
#include <QUrl>
#include <QtWebSockets/QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

int main(int argc, char **argv) {
    QApplication application(argc, argv);

    QLabel window(QStringLiteral("Not connected"));
    window.setWindowTitle(QStringLiteral("Qt Chat Client"));
    window.resize(320, 120);
    window.show();

    QWebSocket socket;
    QString clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QObject::connect(&socket, &QWebSocket::connected, &window, [&window, &socket, &clientId]() {
        window.setText(QStringLiteral("Connected\nID: %1").arg(clientId));

        QJsonObject registration;
        registration[QStringLiteral("type")] = QStringLiteral("register");
        registration[QStringLiteral("clientId")] = clientId;

        const QByteArray json = QJsonDocument(registration).toJson(QJsonDocument::Compact);

        socket.sendTextMessage(QString::fromUtf8(json));
    });

    QObject::connect(&socket, &QWebSocket::disconnected, &window, [&window]() {
        window.setText(QStringLiteral("Disconnected"));
    });

    QObject::connect(&socket, &QWebSocket::errorOccurred, &window, [&window, &socket](QAbstractSocket::SocketError) {
        window.setText(QStringLiteral("Connection error: %1").arg(socket.errorString()));
    });

    QObject::connect(&application, &QCoreApplication::aboutToQuit, &socket, [&socket]() {
        socket.close();
    });

    socket.open(QUrl(QStringLiteral("ws://127.0.0.1:12345")));

    return application.exec();
}