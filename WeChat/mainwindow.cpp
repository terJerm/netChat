#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "chatdialog_code.h"
#include <QTimer>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),_chat_dialog_code(nullptr)
{
    ui->setupUi(this);

    _stack = new QStackedWidget(this);

    _login_dialog = new logDialog(this);
    _register_dialog = new registerDialog(this);
    _find_dialog = new findPasswdDialog(this);

    _stack->addWidget(_login_dialog);
    _stack->addWidget(_register_dialog);
    _stack->addWidget(_find_dialog);

    setCentralWidget(_stack);
    this->adjustSize();
    setFixedSize(this->size());

    connect(_login_dialog,&logDialog::switchRgt,this,&MainWindow::changeReg);
    connect(_register_dialog,&registerDialog::changeLogin,this,&MainWindow::changeLog);
    connect(_login_dialog,&logDialog::switchForgetSignal,this,&MainWindow::changeFor);
    connect(_find_dialog,&findPasswdDialog::changeToLog,this,&MainWindow::changeLog);
    connect(_login_dialog,&logDialog::switchChatDialog,this,&MainWindow::changeChat);

    ///测试聊天界面的展示
    ///_login_dialog->emit switchChatDialog();

}

MainWindow::~MainWindow()
{
    delete ui;
    /// if(_chat_dialog) delete _chat_dialog;
    if(_chat_dialog_code)  _chat_dialog_code->deleteLater();
}

void MainWindow::changeReg()
{
    _stack->setCurrentWidget(_register_dialog);
    adjustSize();
    this->setFixedSize(this->size());
}

void MainWindow::changeLog()
{
    _stack->setCurrentWidget(_login_dialog);
    adjustSize();
    setFixedSize(this->size());
}

void MainWindow::changeFor()
{
    _stack->setCurrentWidget(_find_dialog);
    adjustSize();
    setFixedSize(this->size());
}

void MainWindow::changeChat()  ///在此之前已经将 UserMgr 的信息更新好了
{
    /// 休眠两秒钟在跳转
    QTimer::singleShot(1000, this, [this]() {
        this->hide();
        if (!_chat_dialog_code) {      /// 第一次登录
            _chat_dialog_code = new chatDialog_code();
            connect(_chat_dialog_code,&chatDialog_code::sigChatChangeToLogWid,[this](){
                this->slotChatChangeToLogWid();
            });
            _chat_dialog_code->show();

            return ;
        }

        _chat_dialog_code->initMainChatWid();
        _chat_dialog_code->show();

    });

}

void MainWindow::slotChatChangeToLogWid()
{
    _chat_dialog_code->hide();

    _login_dialog->initUI();
    _find_dialog->initUI();
    _register_dialog->initUI();


    this->show();
}
