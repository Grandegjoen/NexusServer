#include "sqlserver.h"

SqlServer::SqlServer(QObject *parent)
    : QObject{parent}{
    connect_to_db();
}


void SqlServer::connect_to_db(){
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("Localhost");
    db.setDatabaseName("Nexus");
    db.setUserName("root");
    db.setPassword("Asdf2435");
    if (!db.open()){
        qDebug() << "Database failed to open. Please try again.";
    } else {
        qDebug() << "Database sucessfully opened!";
    }
}


// ---------------------------- HANDLES CLIENT RELATED STUFF ------------------------------------------------
bool SqlServer::verify_login(QString username, QString pw){
    if (db.isOpen()){
        if (!db.open()){
            qDebug() << "Sql server failed to open. Exiting 293";
            exit(293);
        }
    }
    QSqlQuery query;
    query.prepare("SELECT HashedPW from Users WHERE Username = ?");
    query.addBindValue(username);
    if (query.exec()){
        query.next();
        qDebug() << pw;
        if (query.value(0).toString() == pw){
            qDebug() << "Password checks out!";
            return true;
        }
    }
    qDebug() << "Password doesn't check out!";
    return false;
}


int SqlServer::add_new_user(QString username, QString hashed_pw, QString unique_hash, QString account_role){
    return 0;
}


QString SqlServer::return_user_hash(QString username){
    if (db.isOpen()){
        if (!db.open()){
            qDebug() << "Sql server failed to open. Exiting 293";
            exit(293);
        }
    }
    QSqlQuery query;
    query.prepare("SELECT unique_hash from Users WHERE Username = ?");
    query.addBindValue(username);
    if (query.exec()){
        query.next();
        return query.value(0).toString();
    }
    qDebug() << "Couldn't find unique_hash belonging to user " << username << ". Exiting!";
    exit(492);
}
// ---------------------------- HANDLES CLIENT RELATED STUFF ------------------------------------------------


// ---------------------------- HANDLES NOTE RELATED STUFF ---------------------------------------------------
void SqlServer::update_note_list(QString username){
    clear_note_list(username);
    QFile file("data/" + username.toLower() + "/" + "t_n_l_" + username);
    file.open(QIODevice::ReadOnly);
    QTextStream stream(&file);
    qDebug() << "Test23";
    if (db.isOpen())
        qDebug() << "Yay, open!";
    else
        qDebug() << "No db?!";
    clear_note_list(username);
    while (!stream.atEnd()){
        QString line = stream.readLine();
        QString item_name = line.split('|').at(0);
        QString item_hash = line.split('|').at(1);
        QString parent_hash = line.split('|').at(2);
        QString item_type = line.split('|').at(3);
        QString password_protected = line.split('|').at(4);
        QString item_owner= line.split('|').at(5);
        QString item_index = line.split('|').at(6);
        QSqlQuery query;
        query.prepare("INSERT INTO Notes VALUES (?, ?, ?, ?, ?, ?, ?)");
        query.addBindValue(item_name);
        query.addBindValue(item_hash);
        query.addBindValue(parent_hash);
        query.addBindValue(item_type);
        query.addBindValue(item_owner);
        query.addBindValue(item_index);
        query.addBindValue(password_protected);
        if (!query.exec()){
            qDebug() << query.lastQuery();
        }
    }
}


void SqlServer::clear_note_list(QString username){
    QSqlQuery query;
    query.prepare("DELETE from Notes WHERE owner = ?");
    query.addBindValue(username);
    if (!query.exec())
        qDebug() << "Query to delete from Users clear_note_list failed to execute. 29182";
}


QByteArray* SqlServer::return_note_list(QString username){
    QByteArray *note_list = new QByteArray();
    QSqlQuery query;
    query.prepare("SELECT * FROM Notes WHERE owner = ?");
    query.addBindValue(username);
    qDebug() << "Writing note list...";
    if (!query.exec())
        qDebug() << "Query failed to execute, write_note_list";
    while (query.next()){
        note_list->append(query.value(0).toByteArray() + "|");
        note_list->append(query.value(1).toByteArray() + "|");
        note_list->append(query.value(2).toByteArray() + "|");
        note_list->append(query.value(3).toByteArray() + "|");
        note_list->append(query.value(4).toByteArray() + "|");
        note_list->append(query.value(5).toByteArray() + "|");
        note_list->append(query.value(6).toByteArray() + "|\n");
    }
    return note_list;
}


QByteArray* SqlServer::return_note(QString username, QString note_name){
    QByteArray *note = new QByteArray;
    QFile file("data/" + username.toLower() + "/" + note_name);
    file.open(QIODevice::ReadOnly);
    note->append(file.readAll());
    return note;
}
// ---------------------------- HANDLES NOTE RELATED STUFF ---------------------------------------------------


// ---------------------------- HANDLES PWM RELATED STUFF ---------------------------------------------------
void SqlServer::add_pwm_item_to_db(QByteArray data, QString owner){
    QString unique_hash = data.split('|').at(1);
    QString domain = data.split('|').at(2);
    QString username = data.split('|').at(3);
    QString extra_pw = data.split('|').at(4);
    int begin_length = data.split('|').at(0).size() + data.split('|').at(1).size() + data.split('|').at(2).size()
            + data.split('|').at(3).size() + data.split('|').at(4).size() + 5; // 5 being number of '|'... There's likely a better solution for this
    QByteArray password = data.mid(begin_length, data.size() - begin_length - 6); // 6 being |N-E-X ... Jesus what a shitty solution, but it works...
    QSqlQuery query;
    query.prepare("INSERT INTO Passwords VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(unique_hash);
    query.addBindValue(domain);
    query.addBindValue(username);
    query.addBindValue(password);
    query.addBindValue(extra_pw);
    query.addBindValue(owner);
    if (!query.exec()){
        qDebug() << "Add PWM failed to execute!";

    } else {
        qDebug() << "Add PWM succeeded!";
    }
}


void SqlServer::update_pwm_item(QByteArray data, QString owner){
    QString unique_hash = data.split('|').at(1);
    QByteArray password = data.split('|').at(3);
    QString extra_pw = data.split('|').at(2);
    QSqlQuery query;
    query.prepare("UPDATE Passwords SET Password = ?, Extra_password = ? WHERE owner = ? AND UniqueHash = ?");
    query.addBindValue(password);
    query.addBindValue(extra_pw);
    query.addBindValue(owner);
    query.addBindValue(unique_hash);
    if (query.exec()){
        qDebug() << "Update PWM executed successfully!";
    } else {
        qDebug() << "Update PWM failed to execute!";
    }
}


void SqlServer::remove_pwm_item(QString data, QString owner){
    QSqlQuery query;
    query.prepare("DELETE FROM Passwords WHERE UniqueHash = ? AND owner = ?");
    query.addBindValue(data);
    query.addBindValue(owner);
    if (query.exec()){
        qDebug() << "Update PWM executed successfully!";
    } else {
        qDebug() << "Update PWM failed to execute!";
    }

}


QByteArray* SqlServer::return_pwm_list(QString username){
    QByteArray *data = new QByteArray();
    QSqlQuery query;
    query.prepare("SELECT * FROM Passwords WHERE owner = ?");
    query.addBindValue(username);
    qDebug() << "Writing PWM list...";
    if (!query.exec())
        qDebug() << "Query failed to execute, write_note_list";
    while (query.next()){
        data->append(query.value(0).toByteArray() + "|"); // Unique Hash
        data->append(query.value(1).toByteArray() + "|"); // Domain
        data->append(query.value(2).toByteArray() + "|"); // username
        data->append(query.value(4).toByteArray() + "|"); // extra password
        data->append(query.value(5).toByteArray() + "|"); // owner
        data->append(query.value(3).toByteArray() + "|\n"); // Password
    }
    return data;
}
// ---------------------------- HANDLES PWM RELATED STUFF ---------------------------------------------------

// ---------------------------- HANDLES FTP RELATED STUFF ---------------------------------------------------
QByteArray* SqlServer::return_ftp_list(QString username){
    QByteArray *data = new QByteArray();
    QSqlQuery query;
    query.prepare("SELECT * FROM Files WHERE owner = ?");
    query.addBindValue(username);
    qDebug() << "Writing FTP list...";
    qDebug() << "Step 1;";
    if (!query.exec())
        qDebug() << "Query failed to execute, write_note_list";
    qDebug() << "Step 2;";
    while (query.next()){
        qDebug() << "Step 3;";

        data->append(query.value(0).toByteArray() + "|"); // File name
        data->append(query.value(1).toByteArray() + "|"); // File hash
        data->append(query.value(2).toByteArray() + "|"); // File size
        data->append(query.value(3).toByteArray() + "|"); // Date Added
        data->append(query.value(4).toByteArray() + "|"); // Password protected
        data->append(query.value(5).toByteArray() + "|"); // Sample sentence
        data->append(query.value(6).toByteArray() + "|"); // Owner
        data->append(query.value(7).toByteArray() + "|\n"); // True size

    }
    qDebug() << data;
    return data;
}


void SqlServer::add_file_to_db(QString file_name, QString file_hash, quint64 file_size, QString password_protected,
                               QByteArray sample, QString owner, QString formatted_time, quint64 true_size){
    QSqlQuery query;
    query.prepare("INSERT INTO Files VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(file_name);
    query.addBindValue(file_hash);
    query.addBindValue(file_size);
    query.addBindValue(formatted_time);
    query.addBindValue(password_protected);
    query.addBindValue(sample);
    query.addBindValue(owner);
    query.addBindValue(true_size);
    if (!query.exec()){
        qDebug() << "Add FILE failed to execute!";

    } else {
        qDebug() << "Add FILE succeeded!";
    }

}


void SqlServer::remove_file_from_db(QString file_hash, QString username){
    QSqlQuery query;
    query.prepare("DELETE FROM Files WHERE file_hash = ? AND owner = ?");
    query.addBindValue(file_hash);
    query.addBindValue(username);
    if (!query.exec()){
        qDebug() << "Delete file query failed to exec!";
    } else {
        qDebug() << "Delete file query executed successfully!";
    }
    QFile::remove("data/" + username + "/" + file_hash);

}
// ---------------------------- HANDLES FTP RELATED STUFF ---------------------------------------------------
