#include "client.h"

Client::Client(QObject *parent)
    : QObject{parent}{

}

QString username_based_hash(QString username){
    QString hash;
    QByteArray result = QCryptographicHash::hash(username.toLower().toUtf8() + "AOLEOSKAOEJANE!", QCryptographicHash::Sha1);
    hash = QLatin1String(result.toHex());
    qDebug() << hash;
    return hash;
}


void decrypter(QByteArray* note, QString aes_secret_key){
    CryptoPP::SHA256 hash;
    CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
    std::string message = aes_secret_key.toStdString() + "R@nd0m!Sh17$yS4!t!";

    hash.CalculateDigest(digest, (CryptoPP::byte*)message.c_str(), message.length());

    CryptoPP::HexEncoder encoder;
    std::string sKey;
    encoder.Attach(new CryptoPP::StringSink(sKey));
    encoder.Put(digest, sizeof(digest));
    encoder.MessageEnd();

    CryptoPP::byte key[CryptoPP::AES::MAX_KEYLENGTH]; //16 Bytes MAXKEYLENGTH 32 BYTES(SHA 256)
    CryptoPP::byte  iv[CryptoPP::AES::BLOCKSIZE];
    memcpy(key, sKey.c_str(), CryptoPP::AES::MAX_KEYLENGTH);;
    memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE);

    CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);

    std::string cipher_text = note->toStdString();
    std::string decrypted_text;

    try {
        CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decrypted_text));
        stfDecryptor.Put(reinterpret_cast<const unsigned char*>(cipher_text.c_str()), cipher_text.size());
        stfDecryptor.MessageEnd();
    } catch (const CryptoPP::Exception& e){
        qDebug() << e.what();
    }

    *note = QByteArray::fromStdString(decrypted_text);
}


void encrypter(QByteArray* note, QString aes_secret_key){
    CryptoPP::SHA256 hash;
    CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
    std::string message = aes_secret_key.toStdString() + "R@nd0m!Sh17$yS4!t!";

    hash.CalculateDigest(digest, (CryptoPP::byte*)message.c_str(), message.length());

    CryptoPP::HexEncoder encoder;
    std::string sKey;
    encoder.Attach(new CryptoPP::StringSink(sKey));
    encoder.Put(digest, sizeof(digest));
    encoder.MessageEnd();

    CryptoPP::byte key[CryptoPP::AES::MAX_KEYLENGTH]; //16 Bytes MAXKEYLENGTH 32 BYTES(SHA 256)
    CryptoPP::byte  iv[CryptoPP::AES::BLOCKSIZE];
    memcpy(key, sKey.c_str(), CryptoPP::AES::MAX_KEYLENGTH);;
    memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE);

    CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);

    std::string plain_text_std = note->toStdString();
    std::string cipher_text;

    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(cipher_text));
    stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plain_text_std.c_str()), plain_text_std.length());
    stfEncryptor.MessageEnd();

    *note = QByteArray::fromStdString(cipher_text);
}


void Client::encrypt_note(QString path, QString password){
    QFile data(path);
    data.open(QIODevice::ReadOnly);
    QByteArray note = data.readAll();
    if (note == ""){
        note = "Empty note";
    }
    data.close();
    decrypter(&note, username_based_hash(username));
    encrypter(&note, username_based_hash(password));
    data.open(QIODevice::WriteOnly);
    data.write(note);
}


void Client::handle_data(){
    qDebug() << data;
    qDebug() << "Handling data...";
    if (file_transfer_stream->active_data_transmission){
        qDebug() << "Active data transmission!";
        file_transfer_stream->receive_file(&data);
        data.clear();
        if (file_transfer_stream->file.size() != file_transfer_stream->data_size)
            return;
        active_data_transmission = false;
        QDateTime date = QDateTime::currentDateTime();
        QString formatted_time = date.toString("dd.MM.yyyy hh:mm:ss");
        sql_server->add_file_to_db(file_transfer_stream->file_name, file_transfer_stream->file_hash, file_transfer_stream->file.size(), file_transfer_stream->password_protected,
                                   file_transfer_stream->encrypted_sentence, username, formatted_time, file_transfer_stream->true_size);
        ssl_socket->write("data_received|" + formatted_time.toUtf8() + "|N-E-X");
        emit file_transfer_done(this);
        qDebug() << "Emitting file transfer done SENDING";
//        delete file_transfer_stream;
//        delete this;
        return;
    }
    qDebug() << "Checking if last 5 are N-E-X!";
    if (data.last(5) != "N-E-X")
        return;
    qDebug() << "Last 5 are N-E-X!";
    QString message = data.split('|').at(0);
    if (message == "file_info"){
        // Write data size and such to file transfer stream... Server is receiving here. Ready to receive afterwards.
        qDebug() << "Receiving file info...";
        file_transfer_stream->file_name = QString::fromUtf8(data.split('|').at(1));
        file_transfer_stream->data_size = data.split('|').at(2).toInt();
        file_transfer_stream->encrypted_sentence= data.split('|').at(3);
        file_transfer_stream->file_hash = data.split('|').at(4);
        file_transfer_stream->active_data_transmission = true;
        file_transfer_stream->password_protected = data.split('|').at(5);
        file_transfer_stream->true_size = data.split('|').at(6).toInt();


        file_transfer_stream->file.setFileName("data/" + username + "/" + file_transfer_stream->file_hash);
        file_transfer_stream->file.open(QIODevice::Append);
        file_transfer_stream->active_data_transmission = true;
        ssl_socket->write("receiving_data|N-E-X");
    }
    else if (message == "client_receive"){
        // Receiving 1: "client_receive|file_hash|encryption_key(default or custom)|N-E-X" Client
    }
//    else if (message == "send_file_info"){
//        // Write data size and such to client
//    }
    else if (message == "receiving_data"){
        // Receiving 3: "receiving|N-E-X" Client
        QString file_path = "data/" + username.toLower() + "/" + data.split('|').at(1);
        file_transfer_stream->ssl_socket = ssl_socket;
        file_transfer_stream->send_file(file_path);
    }
    else if (message == "data_received"){
        // FINAL 5: "data_received|N-E-X" client
        emit file_transfer_done(this);
//        delete file_transfer_stream;
//        delete this;
    }
    data.clear();
}

void Client::create_folders(){
    QDir dir;
    if (!dir.exists("data")){
        dir.mkdir("data");
    }
    if (!dir.exists("data/" + username)){
        dir.mkdir("data/" + username);
    }
}
