#ifndef MYHOVERBUTTON_H
#define MYHOVERBUTTON_H

#include <QToolButton>
#include <QIcon>

/**************************************************************/

//   * @file:      myhoverbutton.h
//   * @brife:     自定义按钮样式
//   * @date:      2025/06/10

/**************************************************************/

class myHoverButton : public QToolButton
{
    Q_OBJECT
public:
    explicit myHoverButton(QWidget *parent = nullptr);

    void setNormalIcon(const QIcon &icon);
    void setHoverIcon(const QIcon &icon);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QIcon _normalIcon;
    QIcon _hoverIcon;

signals:
};

#endif // MYHOVERBUTTON_H
