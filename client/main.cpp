#include <QAbstractSocket>
#include <QApplication>
#include <QLabel>
#include <QUrl>
#include <QtWebSockets/QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char **argv) {
    QApplication application(argc, argv);

    QString clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QWidget window;
    window.setWindowTitle(QStringLiteral("QtChat Client [%1]").arg(clientId));
    window.resize(600, 400);

    auto *statusLabel = new QLabel(QStringLiteral("Not connected"), &window);
    
    auto *messageHistory = new QPlainTextEdit(&window);
    messageHistory->setReadOnly(true);
    
    auto *messageInput = new QLineEdit(&window);
    messageInput->setPlaceholderText(QStringLiteral("Type a message..."));
    messageInput->setEnabled(false);

    auto *sendButton = new QPushButton(QStringLiteral("Send"), &window);
    sendButton->setEnabled(false);

    auto *inputLayout = new QHBoxLayout;
    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);

    auto *mainLayout = new QVBoxLayout(&window);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(messageHistory, 1);
    mainLayout->addLayout(inputLayout);

    window.show();

    QWebSocket socket;

    auto sendRegistration = [&socket, &clientId]() {
        QJsonObject registration;
        registration[QStringLiteral("type")] = QStringLiteral("register");
        registration[QStringLiteral("clientId")] = clientId;

        const QByteArray json = QJsonDocument(registration).toJson(QJsonDocument::Compact);

        socket.sendTextMessage(QString::fromUtf8(json));
    };

    auto sendChatMessage = [&socket, messageInput]() {
        if (socket.state() != QAbstractSocket::ConnectedState) {
            return;
        }

        const QString chatText = messageInput->text().trimmed();

        if (chatText.isEmpty()) {
            return;
        }

        QJsonObject chatMessage;
        chatMessage[QStringLiteral("type")] = QStringLiteral("chat");
        chatMessage[QStringLiteral("message")] = chatText;

        const QByteArray json = QJsonDocument(chatMessage).toJson(QJsonDocument::Compact);

        socket.sendTextMessage(QString::fromUtf8(json));

        messageInput->clear();
        messageInput->setFocus();
    };

    QObject::connect(sendButton, &QPushButton::clicked, &window, sendChatMessage);
    QObject::connect(messageInput, &QLineEdit::returnPressed, &window, sendChatMessage);

    QObject::connect(&socket, &QWebSocket::connected, &window, [&window, statusLabel, &clientId, &sendRegistration]() {
        statusLabel->setText(QStringLiteral("Connected; registering..."));
        sendRegistration();
    });

    QObject::connect(&socket, &QWebSocket::disconnected, &window, [&window, statusLabel, messageInput, sendButton]() {
        statusLabel->setText(QStringLiteral("Disconnected"));
        messageInput->setEnabled(false);
        sendButton->setEnabled(false);
    });

    QObject::connect(&socket, &QWebSocket::errorOccurred, &window, [&window, statusLabel, &socket](QAbstractSocket::SocketError) {
        statusLabel->setText(QStringLiteral("Connection error: %1").arg(socket.errorString()));
    });

    QObject::connect(&application, &QCoreApplication::aboutToQuit, &socket, [&socket]() {
        socket.close();
    });

    QObject::connect(&socket, &QWebSocket::textMessageReceived, &window, [&window, statusLabel, messageHistory, messageInput, sendButton, &clientId, &sendRegistration](const QString &message) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);

        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            statusLabel->setText(QStringLiteral("Invalid response from server"));
            return;
        }

        const QJsonObject payload = document.object();
        const QString messageType = payload.value(QStringLiteral("type")).toString();

        if (messageType == QStringLiteral("registration")) {
            const QString status = payload.value(QStringLiteral("status")).toString();

            if (status == QStringLiteral("accepted")) {
                statusLabel->setText(QStringLiteral("Registered"));
                messageInput->setEnabled(true);
                sendButton->setEnabled(true);
                messageInput->setFocus();
            } else {
                clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                window.setWindowTitle(QStringLiteral("QtChat Client [%1]").arg(clientId));
                statusLabel->setText(QStringLiteral("Retrying registration"));
                sendRegistration();
            }

            return;
        }

        if (messageType == QStringLiteral("chat")) {
            const QString senderId = payload.value(QStringLiteral("senderId")).toString();
            const QString chatText = payload.value(QStringLiteral("message")).toString();
            messageHistory->appendPlainText(QStringLiteral("%1: %2").arg(senderId, chatText));
            return;
        }
    });

    socket.open(QUrl(QStringLiteral("ws://127.0.0.1:12345")));

    return application.exec();
}