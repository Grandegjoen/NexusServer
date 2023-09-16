#ifndef FTPSERVER_H
#define FTPSERVER_H

#include <QTcpServer>
#include <QObject>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSsl>
#include <QSslKey>
#include <QSslCertificate>
#include <QFile>
#include "filetransfer.h"
#include "client.h"
#include "sqlserver.h"

struct client_without_sessid{
    QTcpSocket* socket;
    QByteArray data;
};

class FTPServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit FTPServer(QObject *parent = nullptr);
    void start_server(); // Starts the server
    QFile log;
    QVector <Client*> active_ft_streams;
    SqlServer *sql_server;

public slots:
    void link();
    void read_data();
    void disconnected();
    void erase_from_active_stream(Client *client);

private:


    quint16 server_port = 13580;
    QString server_domain = "127.0.0.1";
    QVector <client_without_sessid*> clients_without_sessid;


    void incomingConnection(qintptr socket_descriptor); // Server connection
    void set_keys();
    void send_data(QByteArray *data, QFile* = nullptr);

    Client* return_client(QTcpSocket*);

    // SSL related variables
    QString cert_location;
    QString key_location;
    QString CaCerts_location;
    QSslKey key;
    QSslCertificate cert;



    client_without_sessid* return_client_without_sessid(QTcpSocket*);

};

#endif // FTPSERVER_H
