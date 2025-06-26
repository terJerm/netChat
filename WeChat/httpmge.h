#ifndef HTTPMGE_H
#define HTTPMGE_H

#include <singleton.h>
#include <global.h>
#include <QUrl>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>

/**************************************************************/

//   * @file:      httpmge.h
//   * @brife:     歧义递归实现 http 单例管理类，用于处理登录/注册/找回 的 http 请求
//   * @date:      2025/03/23

/**************************************************************/

class httpMge : public QObject, public Singleton<httpMge>,public std::enable_shared_from_this<httpMge> {
    Q_OBJECT
    friend class Singleton<httpMge>;

public:
    ~httpMge();
    void PostHttpReq(QUrl url,QJsonObject json,ReqId req_id,Modules mod);   /// 发送一个post请求

private:
    httpMge();
    QNetworkAccessManager _manager;
private slots:
    void slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod); //http请求总槽函数，分发

signals:
    void sig_http_finish(ReqId id,QString res,ErrorCodes err, Modules mod);  //http请求总信号

    void sig_register_mod_finish(ReqId id, QString res, ErrorCodes err);   ///注册请求的回包发送的信号
    void sig_findpasswd_mod_finish(ReqId id, QString res, ErrorCodes err); ///找回密码请求的回包发送的信号
    void sig_log_mod_finish(ReqId id, QString res, ErrorCodes err);        ///登录请求的回包发送的信号
};
























#endif // HTTPMGE_H
