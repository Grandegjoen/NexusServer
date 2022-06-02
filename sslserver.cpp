#include "sslserver.h"

SslServer::SslServer(QObject *parent)
    : QTcpServer{parent}{
    qDebug() << "Starting server...";

    // Loads keys from files into server.
    set_keys();
    // Starts listening for connections
    if (!listen(QHostAddress::Any, 12345)) {
        qCritical() << "Unable to start the TCP server";
        exit(0);
    } else {
        qDebug() << "Listening!";
    }

    // Calls link whenever a new connection is established.
    // newConnection is emitted once incomingConnection (overwritten below) is changed.
    connect(this, &SslServer::newConnection, this, &SslServer::link);

}

void SslServer::set_keys(){
    cert_location = "/home/admin/keys/grandegjoen_server_keys/red_local.pem";
    key_location = "/home/admin/keys/grandegjoen_server_keys/red_local.key";
    CaCerts_location = "/home/admin/keys/grandegjoen_server_keys/blue_ca.pem";
    QFile key_file(key_location);
    key_file.open(QIODevice::ReadOnly);
    key = QSslKey(key_file.readAll(), QSsl::Rsa);
    key_file.close();

    QFile cert_file(cert_location);
    cert_file.open(QIODevice::ReadOnly);
    cert = QSslCertificate(cert_file.readAll());
    cert_file.close();
}

void SslServer::link(){
    // nextPendingConnection returns ssl_socket from incomingConnection. Secure connection!
    //    QTcpSocket *client_socket;
    qDebug() << "IncomingLink!";

    client_socket = nextPendingConnection();
    qDebug() << "Link has been established. Sending file info soon!";
    // ReadyRead is data from server. Can write to server using client_socket->write.
    connect(client_socket, &QTcpSocket::readyRead, this, &SslServer::read_data);
    connect(client_socket, &QTcpSocket::disconnected, this, &SslServer::disconnected);
    send_file();
}

void SslServer::read_data(){
    // client_socket pointer is assigned to client_socket_data to avoid the right client socket being overwritten? Need to test.
    QTcpSocket *client_socket_data = qobject_cast<QTcpSocket*>(sender());

    // Need to assign protocols for different functions.
    // Read/Assign first 400 bytes for metadata

    QString command = client_socket_data->readAll();

    // Do different things depending on command. For now I just temporarily send some files...



}

void SslServer::disconnected(){
    qDebug("Client Disconnected");
    QTcpSocket* client_socket = qobject_cast<QTcpSocket*>(sender());
    client_socket->deleteLater();
}

void SslServer::incomingConnection(qintptr socket_descriptor){
    qDebug() << "IncomingConnection!";
    QSslSocket *ssl_socket = new QSslSocket(this);

    // Adds ca certificates to socket
    QSslConfiguration ssl_config = QSslConfiguration::defaultConfiguration();
    ssl_config.addCaCertificates(CaCerts_location);
    ssl_socket->setSslConfiguration(ssl_config);

    // Sends errors to ssl_errors slot
    connect(ssl_socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ssl_errors(QList<QSslError>)));

    // Socket descriptor is the client identifier.
    ssl_socket->setSocketDescriptor(socket_descriptor);
    ssl_socket->setPrivateKey(key); // Private key
    ssl_socket->setLocalCertificate(cert);
    ssl_socket->setPeerVerifyMode(QSslSocket::VerifyPeer);
    ssl_socket->startServerEncryption();

    // Goes to link using a ssl_socket
    addPendingConnection(ssl_socket);
}

void SslServer::send_file_info(){
    // Sql Query to receive file info

    QByteArray header;
//    header.fill(0, 500);
    QString path = QDir::currentPath() + "/test.txt";
    QFile file_to_send(path);
    file_to_send.open(QIODevice::ReadOnly);
    QFileInfo fi(path);
    QByteArray data_type = "File";
    QByteArray file_name = fi.baseName().toUtf8();
    QByteArray data_size;
    data_size = data_size.setNum(file_to_send.size() + 500, 10);
    header = data_type + "|" + file_name + "|" + data_size + "|";
    qDebug() << header.size();
    qDebug() << data_type.size();
    qDebug() << file_name.size();
    qDebug() << data_size;
    qDebug() << data_size.size();
    while (header.size() < 500){
        header.append(0x1);
    }

    header.append(file_to_send.readAll());
    qDebug() << header.size();
    qDebug() << header;


    return;
//    qDebug() << "Test";
//    QString path = QDir::currentPath() + "/test.txt";
//    QFile file_to_send(path);
//    QFileInfo fi(path);
//    QByteArray file_name = fi.baseName().toUtf8(); // Stores filename in QByteArray
//    QByteArray file_name_size;
//    file_name_size = file_name_size.setNum(file_name.size(), 2); // Converts size to binary
//    client_socket->write("File|");
//    client_socket->write(file_name_size);
//    client_socket->write("|");
//    client_socket->write(file_name);
//    client_socket->write("|");
//    file_to_send.open(QIODevice::ReadOnly);
//    QByteArray file = file_to_send.readAll();
//    QByteArray file_size;
//    file_size = file_size.setNum(file.size(), 2); // Converts size to binary
//    client_socket->write(file_size);
//    client_socket->write("|");
//    client_socket->write("Test1");
//    client_socket->write("Test2");

//    client_socket->write(file); // Create loop to send in multiple packages?
//    client_socket->write(file_size);
//    while(!file.atEnd()){
//        QByteArray block;
//        QDataStream out(&block, QIODevice::WriteOnly);
//        out << file_to_send.fileName();

//        block.append(file_to_send.read(2048));
//        client_socket->write(block);
//    }
    qDebug() << "REACHED END!";

    return;

    //    QString path = QDir::currentPath() + "/Neon City.png";
    //    QFile image(path);
    //    image.open(QIODevice::ReadOnly);

    //    QByteArray block;
    //    QDataStream out(&block, QIODevice::WriteOnly);

    //    out << (quint32)0 << image.fileName();

    //    QByteArray q = image.readAll();
    //    block.append(q);
    //    image.close();

    //    out.device()->seek(0);
    //    out << (quint32)(block.size() - sizeof(quint32));

    //    qint64 x = 0;
    //    while (x < block.size()) {
    //        qint64 y = client_socket->write(block);
    //        x += y;
    //        qDebug() << x;
    //    }

    //    qDebug() << "Sending...";
    //    QString path = QDir::currentPath() + "/Neon City.png";
    //    QString path2 = QDir::currentPath() + "/testtest.png";
    //    client_socket->write("GIDANGIADFUJGI9UDFSJGIODSFJNVBFSDUIJJGVBNPXI HJQ3T4UNIPDF");
    //    QFile image(path);
    //    image.open(QIODevice::ReadOnly);
    //    QFile file(path2);
    //    file.open(QIODevice::ReadWrite);
    //    QDataStream out(&file);

    //    out << QString("Neon City.png");
    //    out << (qint32)49213;
    //    out << image.readAll();
    //    client_socket->write(file.readAll());

    //    out.device()->seek(0);
    //    out << (quint64)(QBA.size() - sizeof(quint64));
    //    QString data_type = "File_info";
    //    QString file_one = "Account.txt\n43.2mb\n05.05.2022\nContains DB username and PW\n";
    //    QString file_two = "Rarry.txt\n52.9mb\n03.04.2021\nSome notes about the origins of the universe.\n";
    //    QDataStream stream(&file);
    //    stream << data_type;
    //    stream << file_one;
    //    stream << file_two;
    //    for (int i = 0; i < 3; ++i){
    //        QString test;
    //        out >> test;
    //        qDebug() << test;
    //    }


}

void SslServer::send_notes_info(){

}

void SslServer::send_file(){
    QByteArray header;
    QString path = QDir::currentPath() + "/test.mp4";
    QFile file_to_send(path);
    file_to_send.open(QIODevice::ReadOnly);
    QFileInfo fi(path); //
    QByteArray data_type = "file_transfer";
    QByteArray file_name = fi.fileName().toUtf8();
    QByteArray data_size;
    data_size = data_size.setNum(file_to_send.size() + 500, 10);
//    data_size = 500 + file_to_send.size();
    header = data_type + "|" + file_name + "|" + data_size + "|";
    while (header.size() < 500){
        header.append(0x1);
    }
//    header.append(file_to_send.readAll());
//    client_socket->write(header);
    qint64 data_written = 0;
    qint64 total_data = data_size.toInt();
    qint32 bytes_to_write = 2048;
    qint32 count = 0;
    header.append(file_to_send.readAll());
    while (data_written != total_data){
        if (data_written + 2048 > total_data){
            qDebug() << "Done";
            bytes_to_write = total_data - data_written;
            client_socket->write(header.mid(data_written, bytes_to_write));
            data_written += bytes_to_write;
            qDebug() << "Data Written: " << data_written;
            return;
        } else if (data_written >= total_data){
            qDebug() << "Equals";
            return;
        }
        client_socket->write(header.mid(data_written, 2048));
        data_written += 2048;
    }
}

void SslServer::receive_file(){

}

void SslServer::ssl_errors(const QList<QSslError> &errors){
    foreach (const QSslError &error, errors)
        qDebug() << error.errorString();
}

SslServer::~SslServer(){
    qDebug() << "End of server!";
    exit(0);
}
