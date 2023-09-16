#include "filetransfer.h"

void FileTransfer::send_file(QString file){
    QFile file_to_send(file);
    if (!file_to_send.open(QIODevice::ReadOnly)){
        qDebug() << "File failed to open, exiting!";
        exit(20);
    }
    quint64 file_size = file_to_send.size();
    quint64 data_written = 0;
    while (data_written != file_size){
        QByteArray data;
        if (data_written + bytes_to_write > file_size){
            data = file_to_send.read(file_size - data_written);
            data_written += file_size - data_written;
        } else {
            data = file_to_send.read(bytes_to_write);
            data_written += bytes_to_write;
        }
        ssl_socket->write(data);
    }
    qDebug() << "File successfully sent!";

}


void FileTransfer::receive_file(QByteArray *data){
    file.open(QIODevice::Append);
    file.write(*data);
}
