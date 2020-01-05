#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStringList>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    if (a.arguments().size() == 1) {
        QLocalServer server;
        server.listen("RazerControlDaemon");
        while (server.waitForNewConnection(-1)) {
            QLocalSocket *socket = server.nextPendingConnection();
            socket->waitForReadyRead();
            qDebug() << "received message" << socket->readAll();
            delete socket;
        }
    }

    return a.exec();
}
