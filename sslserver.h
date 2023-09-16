// Handles connections between clients and the server.

#ifndef SSLSERVER_H
#define SSLSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSsl>
#include <QSslKey>
#include <QSslCertificate>
#include <QFile>
#include "filetransfer.h"
#include "sqlserver.h"
#include "client.h"
#include "ftpserver.h"


class SslServer : public QTcpServer{
    Q_OBJECT
public:
    explicit SslServer(QObject *parent = nullptr);
    void start_server(); // Starts the server
//    QVector <Client*> active_ft_streams;
    FTPServer *ftp_server;

public slots:
    void link();
    void read_data();
    void disconnected();

signals:


private:
    quint16 server_port = 13579;
    QString server_domain = "127.0.0.1";
    SqlServer *sql_server;

    QFile log;

    QVector <Client*> clients;

    Client* return_client(QTcpSocket *client_socket);

    void incomingConnection(qintptr socket_descriptor); // Server connection
    void set_keys();
    void send_data(QByteArray *data, Client*, QFile* = nullptr);
    void receive_message(Client*);
    void receive_complete_data(Client*);
    void login(Client*);
    bool password_checker(QString username, QString pw);
    quint16 max_packet_size = 1024;

    // Note related variables
    void sync_note_with_server(Client*);
    void sync_note_list_with_server(Client*);
    void send_note_to_client(Client*, bool);
    void send_note_list_to_client(Client*, bool);
    void encrypt_note(Client*);
    void delete_note(Client* client);
    QByteArray note_to_encrypt;


    // SSL related variables
    QString cert_location;
    QString key_location;
    QString CaCerts_location;
    QSslKey key;
    QSslCertificate cert;

    // FTP Related stuff
    void establish_ftp_connection(Client*);
};

#endif // SSLSERVER_H
