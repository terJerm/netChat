#include "customdialog.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QIcon>
#include <QStyle>
#include <QApplication>

CustomDialog::CustomDialog(const QString &text, StatusEnum status, QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint);  // 无右上角关闭
    setModal(true);
    setupUI(text, status);

    connect(okButton, &QPushButton::clicked, this, [this]() {
        this->accept();  // 关闭
        this->deleteLater();  // 删除
    });
}

CustomDialog::~CustomDialog() {}

void CustomDialog::setupUI(const QString &text, StatusEnum status)
{
    iconLabel = new QLabel(this);
    iconLabel->setFixedSize(50,50);
    iconLabel->setPixmap(getStatusIcon(status).scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    textLabel = new QLabel(text, this);
    textLabel->setWordWrap(true);

    okButton = new QPushButton("确定", this);
    okButton->setFixedWidth(80);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(iconLabel);
    topLayout->addWidget(textLabel);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(okButton, 0, Qt::AlignCenter);

    setLayout(mainLayout);
    setFixedSize(300, 150);
}

QPixmap CustomDialog::getStatusIcon(StatusEnum status)
{
    switch (status) {
    case StatusEnum::SUCCESS:
        return QPixmap(":/res/succ.png");
    case StatusEnum::FAILED:
        return QPixmap(":/res/fail.png");
    case StatusEnum::WARNING:
        return QPixmap(":/res/bg-warning.png");
    default:
        return QPixmap();
    }
}
