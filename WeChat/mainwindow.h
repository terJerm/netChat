#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "logdialog.h"
#include "registerdialog.h"
#include "findpasswddialog.h"

class ChatDialog;
class chatDialog_code;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**************************************************************/

//   * @file:      mainwindow.h
//   * @brife:     main window 主界面，调度登录、注册、找回界面以及聊天主界面
//   * @date:      202503/23

/**************************************************************/

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QStackedWidget*       _stack;
    logDialog*            _login_dialog;
    registerDialog*       _register_dialog;
    findPasswdDialog*     _find_dialog;
    ///ChatDialog*           _chat_dialog;
    chatDialog_code*      _chat_dialog_code;

private slots:
    void changeReg();      ///跳转到注册界面的槽函数
    void changeLog();    ///跳转到登录界面的槽函数
    void changeFor();     ///跳转到找回界面的槽函数
    void changeChat();    ///跳转到聊天主界面的槽函数

    void slotChatChangeToLogWid();
};
#endif // MAINWINDOW_H
