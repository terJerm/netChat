#ifndef USERMGR_H
#define USERMGR_H

#include <QObject>
#include "singleton.h"

/**************************************************************/

//   * @file:      usermgr.h
//   * @brife:     管理本次登录的账号信息
//   * @date:      2025/06/10

/**************************************************************/

class UserMgr : public QObject,public std::enable_shared_from_this<UserMgr>,
                public Singleton<UserMgr>
{
    Q_OBJECT
    friend class Singleton<UserMgr>;
public:
    explicit UserMgr();
    ~UserMgr();

    void setName(QString name);
    void setToken(QString token);
    void SetUid(int uid);
    void setSex(int sex);
    void setIcon(QString icon);
    void setEmail(QString email);
    void setDesc(QString desc);

    QString getName();
    QString getToken();
    int getUid();
    int getSex();
    QString getIcon();
    QString getEmail();
    QString getDesc();

private:
    QString _name;
    QString _token;
    int _uid;
    int _sex;
    QString _icon;
    QString _email;
    QString _desc;

signals:
};

#endif // USERMGR_H
