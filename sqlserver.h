// Handles connection/communication between server and sql server/database

#ifndef SQLSERVER_H
#define SQLSERVER_H

#include <QObject>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QFile>
#include <QDateTime>

class SqlServer : public QObject
{
    Q_OBJECT
public:
    explicit SqlServer(QObject *parent = nullptr);
    QSqlDatabase db;
    void connect_to_db();
    QByteArray* return_note(QString username, QString note_name);
    QByteArray* return_note_list(QString);
    void update_note_list(QString);
    void clear_note_list(QString);

    // ---------------------------- HANDLES PWM RELATED STUFF ---------------------------------------------------
    QByteArray* return_pwm_list(QString username);
    void add_pwm_item_to_db(QByteArray, QString);
    void update_pwm_item(QByteArray, QString);
    void remove_pwm_item(QString, QString);
    // ---------------------------- HANDLES PWM RELATED STUFF ---------------------------------------------------


    // ---------------------------- HANDLES FTP RELATED STUFF ---------------------------------------------------
    QByteArray* return_ftp_list(QString username);
    void add_file_to_db(QString file_name, QString file_hash, quint64 file_size, QString password_protected, QByteArray sample, QString owner, QString time, quint64 true_size);
    void remove_file_from_db(QString file, QString username);

    // ---------------------------- HANDLES FTP RELATED STUFF ---------------------------------------------------


    // ---------------------------- HANDLES CLIENT RELATED STUFF ------------------------------------------------
    bool verify_login(QString username, QString pw);
    int add_new_user(QString username, QString hashed_pw, QString unique_hash, QString account_role);
    QString return_user_hash(QString username);
    // ---------------------------- HANDLES CLIENT RELATED STUFF ------------------------------------------------




signals:

};

#endif // SQLSERVER_H
