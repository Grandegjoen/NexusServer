#ifndef SSLSERVER_H
#define SSLSERVER_H

#include <QTcpServer>
#include <QSsl>
#include <QSslKey>
#include <QSslCertificate>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QFile>
#include <QDataStream>
#include <QDir>

class SslServer : public QTcpServer
{
public:
    explicit SslServer(QObject *parent = nullptr);
    ~SslServer();

public slots:
    void ssl_errors(const QList<QSslError> &errors);
    void link();
    void read_data();
    void disconnected();


private:
    QSslKey key;
    QSslCertificate cert;

    void incomingConnection(qintptr socket_descriptor);
    void send_file_info();
    void send_notes_info();
    void send_file();
    void receive_file();
    void set_keys();

    QTcpSocket* client_socket;

    QString cert_location;
    QString key_location;
    QString CaCerts_location;
};

#endif // SSLSERVER_H
