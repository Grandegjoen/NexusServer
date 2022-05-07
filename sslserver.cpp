#include "sslserver.h"

SslServer::SslServer(QObject *parent)
    : QTcpServer{parent}{

    // Reads key and cert files into cert and key variables.
    QFile key_file("/home/admin/keys/grandegjoen_server_keys/red_local.key");
    key_file.open(QIODevice::ReadOnly);
    key = QSslKey(key_file.readAll(), QSsl::Rsa);
    key_file.close();

    QFile cert_file("/home/admin/keys/grandegjoen_server_keys/red_local.pem");
    cert_file.open(QIODevice::ReadOnly);
    cert = QSslCertificate(cert_file.readAll());
    cert_file.close();

    // Starts listening for connections
    if (!listen(QHostAddress::Any, 12345)) {
        qCritical() << "Unable to start the TCP server";
        exit(0);
    }

    // Calls link whenever a new connection is established.
    // newConnection is emitted once incomingConnection (overwritten below) is changed.
    connect(this, &SslServer::newConnection, this, &SslServer::link);

}

void SslServer::link(){
    // nextPendingConnection returns ssl_socket from incomingConnection. Secure connection!
//    QTcpSocket *client_socket;
    client_socket = nextPendingConnection();

    // ReadyRead is data from server. Can write to server using client_socket->write.
    connect(client_socket, &QTcpSocket::readyRead, this, &SslServer::read_data);
    connect(client_socket, &QTcpSocket::disconnected, this, &SslServer::disconnected);
}

void SslServer::read_data(){
    // client_socket pointer is assigned to client_socket_data to avoid the right client socket being overwritten? Need to test.
    QTcpSocket *client_socket_data = qobject_cast<QTcpSocket*>(sender());

    // Need to assign protocols for different functions.
    // Read/Assign first 400 bytes for metadata

    client_socket_data->readAll();
    client_socket_data->write("Server says Hello");
}

void SslServer::disconnected(){
    qDebug("Client Disconnected");
    QTcpSocket* client_socket = qobject_cast<QTcpSocket*>(sender());
    client_socket->deleteLater();
}

void SslServer::incomingConnection(qintptr socket_descriptor){
    QSslSocket *ssl_socket = new QSslSocket(this);

    // Adds ca certificates to socket
    QSslConfiguration ssl_config = QSslConfiguration::defaultConfiguration();
    ssl_config.addCaCertificates("/home/admin/keys/grandegjoen_server_keys/blue_ca.pem");
    ssl_socket->setSslConfiguration(ssl_config);

    // Sends errors to ssl_errors slot
    connect(ssl_socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ssl_errors(QList<QSslError>)));

    // Socket descriptor is the client identifier.
    ssl_socket->setSocketDescriptor(socket_descriptor);
    ssl_socket->setPrivateKey(key); // Private key
    ssl_socket->setLocalCertificate(cert);
    ssl_socket->setPeerVerifyMode(QSslSocket::VerifyPeer);
    ssl_socket->startServerEncryption();
    return;

    // Goes to link using a ssl_socket
    addPendingConnection(ssl_socket);
}

void SslServer::ssl_errors(const QList<QSslError> &errors){
    foreach (const QSslError &error, errors)
        qDebug() << error.errorString();
}

SslServer::~SslServer(){
    qDebug() << "End of server!";
    exit(0);
}
