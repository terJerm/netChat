#ifndef SENDFILEDIALOG_H
#define SENDFILEDIALOG_H

#include <QDialog>

namespace Ui {
class sendFileDialog;
}

/**************************************************************/

//   * @file:      sendfiledialog.h
//   * @brife:     上传/接收文件的界面弹窗
//   * @date:      2025/06/10

/**************************************************************/

class sendFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit sendFileDialog(QWidget *parent = nullptr);
    ~sendFileDialog();

    void SendsetUserInfo( QString icon, QString name); /// 发送文件时设置界面信息
    void RecvsetUserInfo( QString icon, QString name); /// 接收文件时设置界面信息


    void setRecvFileInfo(const QString& filename,QString& suffix,QString& fileicon,QString filemarking);
    void setFileInfo(const QString& filename,const QString& suffix,const qint64& size,const QString& sizeStr,QString);

    void setProgressBar(bool b);   /// 设置进度条

    void setUIEnable(bool);        /// 设置界面上组件状态

    void updateProgress(qint64 currentChunk, qint64 totalChunks);     ///更新进度条

public slots:
    void sendFileSlot();  ///发送文件槽函数
    void recvFileSlot();  ///接收文件槽函数

private:
    Ui::sendFileDialog *ui;
    QString filePath;           /// 上传文件存的是本地文件路径，下载文件存的是文件的 marking 标识

    bool is_send;
};

#endif // SENDFILEDIALOG_H












