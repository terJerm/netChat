#ifndef PASSWORDLINEEDIT_H
#define PASSWORDLINEEDIT_H

#include <QLineEdit>
#include <QToolButton>
#include <QStyle>
#include <QHBoxLayout>
#include <QResizeEvent>

/// 自定义密码输入框，

class PasswordLineEdit : public QLineEdit
{
    Q_OBJECT
private:
    QToolButton* _btn;
    bool _visible;

public:
    explicit PasswordLineEdit(QWidget *parent = nullptr);
private slots:
    void togglePasswordVisibility();
    void resizeEvent(QResizeEvent* event);

signals:
};

#endif // PASSWORDLINEEDIT_H
