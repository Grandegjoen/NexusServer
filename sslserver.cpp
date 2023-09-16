#include "sslserver.h"

// Initialization of server
SslServer::SslServer(QObject *parent)
    : QTcpServer{parent}{
    set_keys();
    start_server();
    sql_server = new SqlServer();
    // Read port number and such from server config file ...

    log.setFileName("data/message_log.nex");
    log.open(QIODevice::Append | QIODevice::WriteOnly);

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
// Initialization of server

// ---------------------------- HANDLES START OF SERVER AND ALL CONNECTIONS TO IT ----------------------------
void SslServer::start_server(){
    if (!listen(QHostAddress::Any, server_port)){
        qCritical() << "Unable to start the server";
    } else {
        qDebug() << "Listening on " << server_port;
        ftp_server = new FTPServer();
        ftp_server->start_server();
        ftp_server->sql_server = sql_server;
    }
    connect(this, &SslServer::newConnection, this, &SslServer::link);
    qDebug() << "Connected";
}


void SslServer::incomingConnection(qintptr socket_descriptor){
    qDebug() << "Incoming connection!";
    QSslSocket *ssl_socket = new QSslSocket(this);

    qDebug() << ssl_socket->socketDescriptor();
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


void SslServer::link(){

    qDebug() << "Link!";

    Client *client = new Client(this);
    client->ssl_socket = nextPendingConnection();
    qDebug() << "Writing after next_pending...";

    clients.append(client);
    connect(client->ssl_socket, &QTcpSocket::readyRead, this, &SslServer::read_data);
    connect(client->ssl_socket, &QTcpSocket::disconnected, this, &SslServer::disconnected);
}
// ---------------------------- HANDLES START OF SERVER AND ALL CONNECTIONS TO IT ----------------------------


// ---------------------------- HANDLES INCOMING DATA AND DISCONNECTION --------------------------------------
void SslServer::read_data(){
    // Back to the basics: Experimenting!
    QTcpSocket *client_socket = qobject_cast<QTcpSocket*>(sender());
    if (client_socket->bytesAvailable() < 5){} /////////////////////////////////////////////////////////////////////////////////////////
    Client *client = return_client(client_socket);
    if (client == nullptr){
        qDebug() << "Nullptr client? Exiting...";
        exit(19282);
    }
    // Here begins the experiment
    if (client->active_data_transmission){
        client->data.append(client->ssl_socket->read(client->ssl_socket->bytesAvailable()));
        qDebug() << "Client data ADS: " << client->data;
        if (client->data_size == client->data.size()){
            receive_complete_data(client);
        }
        return;
    }
    client->data.append(client->ssl_socket->read(client->ssl_socket->bytesAvailable()));
    qDebug() << "Client data N: " << client->data;
    if (client->data.last(5) == "N-E-X"){
        receive_message(client);
        client->data.clear();
    }
}


void SslServer::receive_message(Client* client){
    qDebug() << client->data;
    log.write(client->data + "\n");
    log.flush();
    QString message_type = client->data.split('|').at(0);
    if (message_type == "login"){
        login(client);
    }
    else if (message_type == "received_data"){
        client->ssl_socket->write("received_data|N-E-X");
    }
    else if (message_type == "server_receive_note"){
        sync_note_with_server(client);
    }
    else if (message_type == "server_send_note"){
        if (client->data.split('|').at(1) == "receiving"){
            send_note_to_client(client, true);
        } else
            send_note_to_client(client, false);
    }
    else if (message_type == "server_delete_note"){
        delete_note(client);
        client->ssl_socket->write("received_data|N-E-X");

    }
    else if (message_type == "server_receive_notelist"){
        sync_note_list_with_server(client);
    }
    else if (message_type == "server_send_notelist"){
        if (client->data.split('|').at(1) == "receiving"){
            send_note_list_to_client(client, true);
        } else
            send_note_list_to_client(client, false);
    }
    else if (message_type == "server_encrypt_note"){
        encrypt_note(client);
    }
    else if (message_type == "server_add_pwm"){
        sql_server->add_pwm_item_to_db(client->data, client->username);
    }
    else if (message_type == "server_update_pwm"){
        sql_server->update_pwm_item(client->data, client->username);
    }
    else if (message_type == "server_remove_pwm"){
        sql_server->remove_pwm_item(client->data.split('|').at(1), client->username);
    }
    else if (message_type == "server_send_pwm"){
        if (client->data.split('|').at(1) == "receiving"){
            send_data(client->pwm_list, client);
        }
        else {
            client->pwm_list = sql_server->return_pwm_list(client->username);
            QByteArray message = "server_send_pwm|" + QString::number(client->pwm_list->size()).toUtf8() + "|N-E-X";
            client->ssl_socket->write(message);
        }
    }
    else if (message_type == "server_send_ftp"){
        if (client->data.split('|').at(1) == "receiving"){
            qDebug() << "Step 50;";
            send_data(client->file_list, client);
            qDebug() << "Step 60;";
        }
        else {
            qDebug() << "Step 10;";
            qDebug() << "Returning FTP list";
            client->file_list = sql_server->return_ftp_list(client->username);
            qDebug() << "Step 20;";
            QByteArray message = "server_send_ftp|" + QString::number(client->file_list->size()).toUtf8() + "|N-E-X";
            qDebug() << "Step 30;";
            client->ssl_socket->write(message);
            qDebug() << "Step 40;";

        }
    }
    else if (message_type == "ftp_connection"){
        establish_ftp_connection(client);

    }
    else if (message_type == "file_deletion"){
        QString file_to_delete = client->data.split('|').at(1);
        sql_server->remove_file_from_db(file_to_delete, client->username);
        client->ssl_socket->write("received_data|N-E-X");
    }
    else if (message_type == "data_received"){
        client->ssl_socket->write("server_ready|N-E-X");
    }
}

void SslServer::receive_complete_data(Client* client){
    qDebug() << "Receiving complete data...";
    qDebug() << client->data_type;
    QFile file;
    if (client->data_type == "note"){
        file.setFileName("data/" + client->username.toLower() + "/" + client->note_name);
        file.open(QIODevice::WriteOnly);
        file.write(client->data);
        file.close();
    }
    else if (client->data_type == "notelist"){
        qDebug() << "Notelist";
        file.setFileName("data/" + client->username.toLower() + "/" + "t_n_l_" + client->username);
        file.open(QIODevice::WriteOnly);
        file.write(client->data);
        file.close();
        sql_server->update_note_list(client->username);
    }
    client->data.clear();
    client->ssl_socket->write("received_data|N-E-X");
    client->active_data_transmission = false;
}


void SslServer::disconnected(){
    QTcpSocket *client_socket = qobject_cast<QTcpSocket*>(sender());
    Client *client = return_client(client_socket);
    int count = 0;
    for (auto x : clients){
        if (client == x){
            clients.takeAt(count);
            delete client;
            qDebug() << "Client disconnected and deleted!";
            return;
        }
        count += 1;
    }
    qDebug() << "Couldn't find client. Exiting, bug 192";
    exit(192);
}
// ---------------------------- HANDLES INCOMING DATA AND DISCONNECTION --------------------------------------


// ---------------------------- HANDLES NOTE RELATED STUFF ---------------------------------------------------
void SslServer::sync_note_with_server(Client* client){
    client->data_size = client->data.split('|').at(1).toInt();
    client->note_name = client->data.split('|').at(2);
    client->data_type = "note";
    client->ssl_socket->write("server_receive_note|receiving|N-E-X");
    client->active_data_transmission = true;
}


void SslServer::delete_note(Client* client){
    QFile note("data/" + client->username.toLower() + "/" + client->data.split('|').at(1));
    note.remove();
}


void SslServer::sync_note_list_with_server(Client* client){
    client->data_size = client->data.split('|').at(1).toInt();
    client->note_name = client->data.split('|').at(2);
    client->data_type = "notelist";
    if (client->data_size == 0){
        client->ssl_socket->write("|N-E-X");
        sql_server->clear_note_list(client->username);
        return;
    }
    client->ssl_socket->write("server_receive_notelist|receiving|N-E-X");
    client->active_data_transmission = true;
}


void SslServer::send_note_to_client(Client* client, bool data){
    if (data){
        if (client->note->size() == 0){
            return;
        }
        send_data(client->note, client);
    }
    else {
        QString note_name = client->data.split('|').at(1);
        qDebug() << "Note name: " << note_name;
        client->note = sql_server->return_note(client->username, note_name);
        QByteArray note_size = QString::number(client->note->size()).toUtf8();
        qDebug() << "note size: " << client->note->size();
        client->ssl_socket->write("server_send_note|" + note_size + "|N-E-X");
    }
}


void SslServer::send_note_list_to_client(Client* client, bool data){
    qDebug() << "Data: " << data;
    if (data){
        send_data(client->note_list, client);
        qDebug() << "Sending note list...";
    } else {
        client->note_list = sql_server->return_note_list(client->username);
        client->ssl_socket->write("server_send_notelist|" + QString::number(client->note_list->size()).toUtf8() + "|N-E-X");
        qDebug() << "Made it here;";
    }

}

//QString message = "server_encrypt_note|" + note + "|" + password + "|N-E-X";
void SslServer::encrypt_note(Client* client){
    QString path = "data/" + client->username.toLower() + "/" + client->data.split('|').at(1);
    QString password = client->data.split('|').at(2);
    client->encrypt_note(path, password);
}

// ---------------------------- HANDLES NOTE RELATED STUFF ---------------------------------------------------


// ---------------------------- HANDLES CLIENT RELATED STUFF AND REQUESTS ------------------------------------
Client* SslServer::return_client(QTcpSocket *client_socket){ // Returns client from clients vector if exists.
    for (auto x : clients){
        if (client_socket->socketDescriptor() == x->ssl_socket->socketDescriptor()){
            return x;
        }
    }
    return nullptr;
}


void SslServer::send_data(QByteArray* data, Client* client, QFile *file){
    quint32 bytes_written = 0;
    quint32 bytes_to_write = data->size();
    while(bytes_written != bytes_to_write){
        if (bytes_to_write - bytes_written < max_packet_size){
            client->ssl_socket->write(data->mid(bytes_written, bytes_to_write - bytes_written));
            qDebug() << "send_data SENDING 1: " << data->mid(bytes_written, bytes_to_write - bytes_written);
            bytes_written += bytes_to_write - bytes_written;
        }
        else {
            client->ssl_socket->write(data->mid(bytes_written, max_packet_size));
            qDebug() << "send_data SENDING 2: " << data->mid(bytes_written, max_packet_size);
            bytes_written += max_packet_size;
        }
    }
    data->clear();
}


void SslServer::login(Client *client){
    qDebug() << "Login!";
    QString username = client->data.split('|').at(1);
    QString password = client->data.split('|').at(2);
    if (sql_server->verify_login(username, password)){
        QByteArray message = "login|success|";
        client->unique_user_hash = sql_server->return_user_hash(username);
        message += client->unique_user_hash.toUtf8();
        message += "|N-E-X";
        client->username = username.toLower();
        client->create_folders();
        client->logged_in = true;
        client->ssl_socket->write(message);
    }
    else {
        client->ssl_socket->write("login|failure|N-E-X");
    }
}


void SslServer::establish_ftp_connection(Client* client){
    QString message = "ftp_connection|";
    if (!client->logged_in)
        message.append("00000|N-E-X");
    else {
        Client *new_ft_stream = new Client();
        ftp_server->active_ft_streams.append(new_ft_stream);
        quint32 session_id = 12345;
        new_ft_stream->session_id = session_id;
        new_ft_stream->ssl_socket = nullptr;
        new_ft_stream->username = client->username.toLower();
        message.append("12345|N-E-X"); // Create rng session_id later on instead of magic number
    }
    qDebug() << "Is client logged in? " << client->logged_in;
    client->ssl_socket->write(message.toUtf8());
}

// ---------------------------- HANDLES CLIENT RELATED STUFF AND REQUESTS ------------------------------------
