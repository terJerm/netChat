#ifndef CUSTOMDIALOG_H
#define CUSTOMDIALOG_H

#include <QDialog>


class QLabel;
class QPushButton;

class CustomDialog : public QDialog
{
    Q_OBJECT

public:
    enum class StatusEnum {
        SUCCESS,
        FAILED,
        WARNING
    };

    explicit CustomDialog(const QString &text, StatusEnum status, QWidget *parent = nullptr);
    ~CustomDialog();

private:
    QLabel *iconLabel;
    QLabel *textLabel;
    QPushButton *okButton;

    void setupUI(const QString &text, StatusEnum status);
    QPixmap getStatusIcon(StatusEnum status);
};

#endif // CUSTOMDIALOG_H
