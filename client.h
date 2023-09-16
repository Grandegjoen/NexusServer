// Object that represents each client connected to the server and other necessary values

#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QFile>
#include <QDir>
#include <QCryptographicHash>
#include <cryptopp/aes.h>
#include <cryptopp/rijndael.h>
#include <cryptopp/hex.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/modes.h>
#include <cryptopp/files.h>
#include <cryptopp/osrng.h>
#include "sqlserver.h"
#include "filetransfer.h"

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = nullptr);

    // SSL server related variables
    QString note_name;
    QByteArray *note;
    QByteArray *note_list;
    QByteArray *pwm_list;
    QByteArray *file_list;
    void encrypt_note(QString, QString);
    QString unique_user_hash;
    void create_folders();

    // FTP server related variables
    quint32 session_id;
    void handle_data();
    FileTransfer *file_transfer_stream;

    // Shared variables
    QFile file;
    QByteArray data;
    QString data_type;
    quint64 data_size;
    bool active_data_transmission = false;
    QTcpSocket *ssl_socket;
    QString username;
    bool logged_in = false;


    // Sql stuff
    SqlServer *sql_server;


signals:
    void file_transfer_done(Client *client);
};

#endif // CLIENT_H
