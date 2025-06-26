#include "sendfiledialog.h"
#include "ui_sendfiledialog.h"
#include "tcpfilemgr.h"
#include <QCryptographicHash>
#include "tcpmgr.h"
#include <QThread>
#include "usermgr.h"

sendFileDialog::sendFileDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::sendFileDialog), is_send(false)
{
    ui->setupUi(this);

    setWindowTitle("SendFile");
    this->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);


    connect(ui->no,&QPushButton::clicked,[this]{
        this->hide();
    });

    connect(ui->ok,&QPushButton::clicked,[this]{
        if(is_send){
            this->sendFileSlot();
        }
        else{
            this->recvFileSlot();
        }
    });

}

sendFileDialog::~sendFileDialog()
{
    delete ui;
}

void sendFileDialog::SendsetUserInfo(QString icon, QString name)
{
    QPixmap pix(icon);
    if (pix.isNull()) {
        pix.load(":/res/chat_res/self_1.png");  // 替换为你项目中的默认头像路径
    }
    is_send = true;
    ui->label->setText("发送给：");
    ui->userIcon->setPixmap(pix.scaled(38, 38, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->userName->setText(name);
    ui->ok->setText("发送");
}

void sendFileDialog::RecvsetUserInfo(QString icon, QString name)
{
    QPixmap pix(icon);
    if (pix.isNull()) {
        pix.load(":/res/chat_res/self_1.png");  // 替换为你项目中的默认头像路径
    }
    ui->label->setText("来自于：");
    ui->userIcon->setPixmap(pix.scaled(38, 38, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->userName->setText(name);

    is_send = false;
    ui->ok->setText("接收");
}

void sendFileDialog::setRecvFileInfo(const QString &filename, QString &suffix, QString &fileicon,QString filemarking)
{
    ui->fileIcon->setPixmap(QPixmap(fileicon).scaled(62, 62, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->fileName->setText(filename);
    ui->fileSize->setText(suffix);

    filePath = filemarking;
}

void sendFileDialog::setFileInfo(const QString &filename, const QString &suffix, const qint64 &size,
                                 const QString &sizeStr,QString filepath)
{
    Q_UNUSED(size);
    ui->fileName->setText(filename);
    ui->fileSize->setText(sizeStr);

    /// 根据后缀设置文件图片:QLabel
    QString iconPath;
    QString s = suffix.toLower();
    if (s == "7z" || s == "7zip" || s == "rar") {
        iconPath = ":/res/fileType/icons8-7zip-100.png";
    } else if (s == "ai") {
        iconPath = ":/res/fileType/icons8-ai-100.png";
    } else if (s == "avi") {
        iconPath = ":/res/fileType/icons8-avi-100.png";
    } else if (s == "csv") {
        iconPath = ":/res/fileType/icons8-csv-100.png";
    } else if (s == "dll") {
        iconPath = ":/res/fileType/icons8-dll-100.png";
    } else if (s == "doc" || s == "docx") {
        iconPath = ":/res/fileType/icons8-doc-100.png";
    } else if (s == "exe") {
        iconPath = ":/res/fileType/icons8-exe-100.png";
    } else if (s == "gif") {
        iconPath = ":/res/fileType/icons8-gif-100.png";
    } else if (s == "html" || s == "htm") {
        iconPath = ":/res/fileType/icons8-html-100.png";
    } else if (s == "jpg" || s == "jpeg") {
        iconPath = ":/res/fileType/icons8-jpg-100.png";
    } else if (s == "js") {
        iconPath = ":/res/fileType/icons8-js-100.png";
    } else if (s == "mov") {
        iconPath = ":/res/fileType/icons8-mov-100.png";
    } else if (s == "mp3") {
        iconPath = ":/res/fileType/icons8-mp3-100.png";
    } else if (s == "pdf") {
        iconPath = ":/res/fileType/icons8-pdf-2-100.png";
    } else if (s == "png") {
        iconPath = ":/res/fileType/icons8-png-100.png";
    } else if (s == "ppt" || s == "pptx") {
        iconPath = ":/res/fileType/icons8-ppt-100.png";
    } else if (s == "psd") {
        iconPath = ":/res/fileType/icons8-psd-100.png";
    } else if (s == "raw") {
        iconPath = ":/res/fileType/icons8-raw-100.png";
    } else if (s == "sql") {
        iconPath = ":/res/fileType/icons8-sql-100.png";
    } else if (s == "txt") {
        iconPath = ":/res/fileType/icons8-txt-100.png";
    } else if (s == "w") {
        iconPath = ":/res/fileType/icons8-word-100.png";
    } else if (s == "xls" || s == "xlsx") {
        iconPath = ":/res/fileType/icons8-xls-100.png";
    } else if (s == "xml") {
        iconPath = ":/res/fileType/icons8-xml-100.png";
    } else if (s == "zip") {
        iconPath = ":/res/fileType/icons8-zip-100.png";
    } else {
        iconPath = ":/res/fileType/icons8-unknown-100.png"; // 默认图标
    }

    // 设置图标
    ui->fileIcon->setPixmap(QPixmap(iconPath).scaled(62, 62, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    this->filePath = filepath;


}

void sendFileDialog::setProgressBar(bool b)
{
    if(!b) {
        ui->bar->hide();
        return ;
    }

    ui->bar->show();
    ui->bar->setValue(0);

}

void sendFileDialog::setUIEnable(bool i)
{
    if(i){
        ui->no->setEnabled(true);
        ui->ok->setEnabled(true);
        return ;
    }
    ui->no->setEnabled(false);
    ui->ok->setEnabled(false);
}


void sendFileDialog::updateProgress(qint64 bytesSent, qint64 totalBytes)
{
    if (!ui->bar)
        return;

    // QProgressBar 的最大值是 int，所以这里限制最大值（防止溢出）
    const int progressMax = 10000;  // 用 10000 表示 100%，更精细
    int percent = 0;

    if (totalBytes > 0) {
        percent = static_cast<int>((static_cast<double>(bytesSent) / totalBytes) * progressMax);
    }

    ui->bar->setRange(0, progressMax);
    ui->bar->setValue(percent);
    ui->bar->setTextVisible(true);

    // 显示为百分比
    ui->bar->setFormat(QString("%1%").arg(percent / 100.0, 0, 'f', 1)); // 显示如 75.3%
}




void sendFileDialog::sendFileSlot()          /// 确定发送按钮点击后的槽函数
{
    ///qDebug()<<"filepath is : "<<filePath;

    setUIEnable(false);
    setProgressBar(true);

    /// 1. 检查 文件tcp单例 是否实例化了，如果没有则先获取 fileserver 的服务接口
    bool isconnect = tcpFilemgr::getInstance()->isFinishConnect();
    if(isconnect){
        ///qDebug()<<"fileServer socket is connected ...";

        /// 开始上传文件
        ///
        // 打开文件
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "无法打开文件:" << file.errorString();
            return;
        }

        // 计算文件 MD5
        QCryptographicHash hash(QCryptographicHash::Md5);
        if (!hash.addData(&file)) {
            qWarning() << "读取文件数据失败:" << filePath;
            file.close();
            return;
        }
        QString fileMd5 = hash.result().toHex(); // 16 进制字符串

        // 重置文件指针
        file.seek(0);

        // 准备文件信息
        QFileInfo fileInfo(filePath);
        QString fileName = fileInfo.fileName();
        qint64 totalSize = fileInfo.size();
        int chunkSize = MAX_FILE_LEN;
        int totalChunks = (totalSize + chunkSize - 1) / chunkSize;

        qDebug() << "开始上传文件:" << fileName << "     大小:" << totalSize << "      MD5:" << fileMd5;

        QByteArray buffer;
        int seq = 0;

        while (!file.atEnd()) {

            ///qDebug()<<"send filebag work, the total bagSize is : "<<totalChunks<<" , this is the "<<seq<<" bag...";

            buffer = file.read(chunkSize);
            ++seq;


            QJsonObject jsonObj;
            jsonObj["uid"] = UserMgr::getInstance()->getUid();
            jsonObj["md5"] = fileMd5;                                         /// MD5
            jsonObj["name"] = fileName;                                       /// 文件名
            jsonObj["seq"] = seq;                                             /// 本条片段
            jsonObj["last_seq"] = totalChunks;                                /// 总片段
            jsonObj["trans_size"] = QString::number(file.pos(), 10);          /// 当前发送的长度
            jsonObj["total_size"] = QString::number(totalSize, 10);           /// 文件总长度
            jsonObj["last"] = (seq == totalChunks) ? 1 : 0;                   /// 是否传输完整

            // Base64 编码
            QString base64Data = buffer.toBase64();
            jsonObj["data"] = base64Data;                                     /// 本个文件包的 数据

            // 序列化为 JSON 发送
            QJsonDocument doc(jsonObj);
            QByteArray sendData = doc.toJson(QJsonDocument::Compact);
            tcpFilemgr::getInstance()->sendMsg(ReqId::ID_UPLOAD_FILE_REQ, sendData);
        }



        file.close();
        qDebug() << "文件上传完毕。共发送分片:" << seq;
        return ;
    }else{
        /// 没连接上，发送一个请求获取 fileserver 接口
        qDebug()<<"fileServer socket not connect , try to conenct ...";
        QJsonObject obj;
        obj["is_send"] = 1;
        QJsonDocument docu(obj);
        QByteArray byte = docu.toJson(QJsonDocument::Compact);

        /// 使用 tcpMgr 给服务器发送请求
        TcpMgr::getInstance()->sig_send_data(ReqId::ID_GET_FILESERVER_PATH_REQ,byte);
        return;
    }


}

void sendFileDialog::recvFileSlot()
{
    qDebug()<<"start to receive the file slot, the file marking is : "<<filePath;

    setUIEnable(false);
    setProgressBar(true);

    bool isconnect = tcpFilemgr::getInstance()->isFinishConnect();
    if(!isconnect){
        /// 没连接上，发送一个请求获取 fileserver 接口
        qDebug()<<"fileServer socket not connect , try to conenct ...";
        QJsonObject obj;
        obj["is_send"] = 0;
        QJsonDocument docu(obj);
        QByteArray byte = docu.toJson(QJsonDocument::Compact);

        /// 使用 tcpMgr 给服务器发送请求
        TcpMgr::getInstance()->sig_send_data(ReqId::ID_GET_FILESERVER_PATH_REQ,byte);
        return;
    }
    qDebug()<<"fileserver is connected ...";

    QJsonObject obj;
    obj["marking"] = filePath;

    QJsonDocument docu(obj);
    QByteArray byte = docu.toJson(QJsonDocument::Compact);

    /// 使用 tcpMgr 给服务器发送 下载文件请求
    tcpFilemgr::getInstance()->sendMsg(ReqId::iD_GET_FILE_FROM_MARKING_REQ,byte);


}





















