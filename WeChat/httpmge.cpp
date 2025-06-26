#include "httpmge.h"

httpMge::~httpMge(){

}

httpMge::httpMge() {
    connect(this,&httpMge::sig_http_finish,this,&httpMge::slot_http_finish);
}

void httpMge::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod){
    ///qDebug()<<"threr-1";
    ///请求的 url , 请求的数据（json序列化），请求的id,哪个模块发出的请求
    QByteArray data = QJsonDocument(json).toJson();

    ///通过url构造请求
    QNetworkRequest request(url);
    ///请求的数据格式是 json
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    ///请求的数据长度
    request.setHeader(QNetworkRequest::ContentLengthHeader,QByteArray::number(data.length()));

    auto self = shared_from_this();

    ///发送一个 post 请求
    QNetworkReply* reply = _manager.post(request,data);
    connect(reply,&QNetworkReply::finished,[reply,self,req_id,mod](){
        ////qDebug()<<"进入回调1";
        if(reply->error()!=QNetworkReply::NoError){
            ///处理发送错误情况
            emit self->sig_http_finish(req_id,"",ErrorCodes::ERR_NETWORK,mod);
            reply->deleteLater();
            return;
        }
        ///无错误则接收回复
        QString res = reply->readAll();

        ////qDebug()<<"进入回调2";
        ///发送信号通知完成
        emit self->sig_http_finish(req_id,res,ErrorCodes::SUCCESS,mod);
        reply->deleteLater();
        return;
    });
}

void httpMge::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod){
    if(mod == Modules::REGISRERMOD){
        ///注册模块验证码发送的请求
        emit sig_register_mod_finish(id,res,err);
        return ;
    }if(mod == Modules::FINDPASSWOEDMOD){
        ///找回模块
        emit sig_findpasswd_mod_finish(id,res,err);
        return;
    }if(mod == Modules::LOGMOD){
        ///登录模块
        emit sig_log_mod_finish(id,res,err);
    }

}












