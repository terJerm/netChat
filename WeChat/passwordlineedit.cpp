#include "passwordlineedit.h"

PasswordLineEdit::PasswordLineEdit(QWidget *parent)
    : QLineEdit{parent},_visible(false)
{
    setEchoMode(QLineEdit::Password);

    _btn = new QToolButton(this);
    _btn->setCursor(Qt::ArrowCursor);             // 设置鼠标指针样式
    _btn->setFocusPolicy(Qt::NoFocus);            // 不获取焦点
    _btn->setStyleSheet("QToolButton { border: none; padding: 0px; }");

    /// 设置初始图标（可替换为眼睛图标）
    _btn->setIcon(QIcon(":/res/nuable.png"));

    /// 获取按钮的宽度用于设置右侧 padding，使按钮不会覆盖文本
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    QSize buttonSize = _btn->sizeHint();

    /// 设置 QLineEdit 的右边内边距以避免文字遮挡按钮
    setStyleSheet(QString("QLineEdit { padding-right: %1px; }")
                      .arg(buttonSize.width() + frameWidth + 1));

    /// 设置控件最小尺寸，确保按钮不被裁剪
    QSize msz = minimumSizeHint();
    setMinimumSize(qMax(msz.width(), buttonSize.width() + frameWidth * 2 + 2),
                   qMax(msz.height(), buttonSize.height() + frameWidth * 2 + 2));

    /// 点击按钮时切换密码可见性
    connect(_btn, &QToolButton::clicked, this, &PasswordLineEdit::togglePasswordVisibility);

    /// 设置按钮大小与位置
    int height = this->sizeHint().height();
    _btn->setFixedSize(height - 1, height - 1);
    _btn->move(rect().right() - _btn->width() - frameWidth, (height - _btn->height()) / 2);

}

void PasswordLineEdit::togglePasswordVisibility(){
    _visible = !_visible;

    /// 切换 echoMode（Normal 显示明文，Password 显示密文）
    setEchoMode(_visible ? QLineEdit::Normal : QLineEdit::Password);

    /// 更换按钮图标（你可以替换为更直观的图标，如眼睛/遮眼图标）
    _btn->setIcon(_visible?QIcon(":/res/able.png"):QIcon(":/res/nuable.png"));
}

void PasswordLineEdit::resizeEvent(QResizeEvent *event){
    QLineEdit::resizeEvent(event);

    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    QSize buttonSize = _btn->size();

    _btn->move(rect().right() - buttonSize.width() - frameWidth,
                       (rect().height() - buttonSize.height()) / 2);
}














