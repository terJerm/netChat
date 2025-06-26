#include "myhoverbutton.h"

myHoverButton::myHoverButton(QWidget *parent)
    : QToolButton{parent}
{
    setStyleSheet(R"(
        QToolButton {
            border: none;
            background: transparent;
            border-radius: 8px; /* 圆角大小可调 */
        }
        QToolButton:pressed {
            background-color: rgba(0, 0, 0, 10); /* 半透明黑色 */
            border-radius: 8px; /* 保持按下时也圆角 */
        }
    )");
}

void myHoverButton::setNormalIcon(const QIcon &icon)
{
    _normalIcon = icon;
    setIcon(_normalIcon);
}

void myHoverButton::setHoverIcon(const QIcon &icon)
{
    _hoverIcon = icon;
}

void myHoverButton::enterEvent(QEnterEvent *event)
{
    QToolButton::enterEvent(event);
    if (!_hoverIcon.isNull())setIcon(_hoverIcon);
}

void myHoverButton::leaveEvent(QEvent *event)
{
    QToolButton::leaveEvent(event);
    if (!_normalIcon.isNull())
        setIcon(_normalIcon);
}
