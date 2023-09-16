#include "ftpserver.h"
//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
//Need to work on adding files to ftp list and requesting those files.


FTPServer::FTPServer(QObject *parent)
    : QTcpServer{parent}{
    set_keys();
    log.setFileName("data/message_log.nex");
    log.open(QIODevice::Append | QIODevice::WriteOnly);
}

//
void FTPServer::set_keys(){
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


void FTPServer::start_server(){
    if (!listen(QHostAddress::Any, server_port)) {
        qCritical() << "Unable to start the FTP server";
//        exit(192);
    } else {
        qDebug() << "Listening on " << server_port;
    }
    connect(this, &FTPServer::newConnection, this, &FTPServer::link);
    qDebug() << "Connected";
}


void FTPServer::incomingConnection(qintptr socket_descriptor){
    qDebug() << "Incoming FTP connection!";
    QSslSocket *ssl_socket = new QSslSocket(this);

    // Adds ca certificates to socket
    QSslConfiguration ssl_config = QSslConfiguration::defaultConfiguration();
    ssl_config.addCaCertificates(CaCerts_location);
    ssl_socket->setSslConfiguration(ssl_config);

    // Socket descriptor is the client identifier.
    ssl_socket->setSocketDescriptor(socket_descriptor);
    ssl_socket->setPrivateKey(key); // Private key
    ssl_socket->setLocalCertificate(cert);
    ssl_socket->setPeerVerifyMode(QSslSocket::VerifyPeer);
    ssl_socket->startServerEncryption();

    // Goes to link using a ssl_socket
    addPendingConnection(ssl_socket);
}


void FTPServer::link(){
    qDebug() << "Linking FTP...";
    client_without_sessid *new_client = new client_without_sessid();
    new_client->socket = nextPendingConnection();
    clients_without_sessid.append(new_client);
    connect(new_client->socket, &QTcpSocket::readyRead, this, &FTPServer::read_data);
    connect(new_client->socket, &QTcpSocket::disconnected, this, &FTPServer::disconnected);
}
//


void FTPServer::read_data(){
    qDebug() << "Reading data FTP...";
    QTcpSocket *client_socket = qobject_cast<QTcpSocket*>(sender());
    Client *client = return_client(client_socket);
    if (client != nullptr){
        qDebug() << "Client is NOT nullptr FTP";
        client->data.append(client_socket->read(client_socket->bytesAvailable()));
        log.write(client->data);
        log.flush();
        client->handle_data();
        return;
    }
    client_without_sessid *unregistered_client = return_client_without_sessid(client_socket);
    qDebug() << "Unregisted client ptr: " << unregistered_client;
    if (unregistered_client == nullptr){
        qDebug() << "Unregistered client gives nullptr... returning";
        return;
    }
    unregistered_client->data.append(client_socket->read(client_socket->bytesAvailable()));
    if (unregistered_client->data.last(5) == "N-E-X"){
        qDebug() << "Unregistered client last 5 N-E-X";
        quint32 session_id = unregistered_client->data.split('|').at(1).toInt();
        for (auto &x : active_ft_streams)
            if (x->session_id == session_id){
                qDebug() << "Found right if x->session_id == session_id";
                //                client = new Client();
                //                client->ssl_socket = client_socket;
                x->ssl_socket = client_socket;
                x->file_transfer_stream = new FileTransfer();
                x->sql_server = sql_server;
                connect(x, SIGNAL(file_transfer_done(Client*)), this, SLOT(erase_from_active_stream(Client*)));
                //                client->ssl_socket->write("session_accepted|N-E-X");
                x->ssl_socket->write("session_accepted|N-E-X");
                //                client->session_id = session_id;
                delete unregistered_client;
                return;
            }
        unregistered_client->socket->write("session_rejected|N-E-X");
        qDebug() << "Couldn't find the right active_ft_stream ... Exiting! No, returning!";
    }
}


Client* FTPServer::return_client(QTcpSocket* client){
    qDebug() << "In return_client FTP";
    for (auto &x : active_ft_streams){
        if (x->ssl_socket == nullptr)
            continue;
        else if (client->socketDescriptor() == x->ssl_socket->socketDescriptor())
            return x;
    }
    qDebug() << "Returning FTP return_client nullptr";
    return nullptr;
}

client_without_sessid* FTPServer::return_client_without_sessid(QTcpSocket* socket){
    int count = 0;
    for (auto &x : clients_without_sessid){
        if (x->socket->socketDescriptor() == socket->socketDescriptor()){
            clients_without_sessid.removeAt(count);
            return x;
        }
        count += 1;
    }
    log.write("ERROR, COULDN'T FIND CLIENT IN CLIENTS WITHOUT SESSID. QUITTING.");
    log.flush();
    exit(191);
    return nullptr;
}


void FTPServer::erase_from_active_stream(Client *client){
    int count = 0;
    for (auto &x : active_ft_streams){
        qDebug() << x->session_id << " | " << client->session_id;
        if (client->session_id == x->session_id){
            active_ft_streams.removeAt(count);
        }
        count += 1;
        return;
    }
    log.write("ERROR, COULDN'T FIND CLIENT IN ACTIVE STREAMS. QUITTING.");
    log.flush();
    exit(192);

}


void FTPServer::disconnected(){
    qDebug() << "FTP connection disconnected!";
}
