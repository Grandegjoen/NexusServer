// Handles file transfers

#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <QObject>
#include <QFile>
#include <QDebug>
#include <QTcpSocket>

class FileTransfer{
public:

    bool active_data_transmission = false;
    QString file_name;
    quint64 data_size;
    quint64 data_written;
    quint64 true_size;
    quint16 bytes_to_write = 2048;
    void send_file(QString);
    void receive_file(QByteArray*);
    QFile file;
    QByteArray encrypted_sentence;
    QString password_protected = "false";
    QByteArray file_hash;
    QTcpSocket *ssl_socket;


private:


};

#endif // FILETRANSFER_H
