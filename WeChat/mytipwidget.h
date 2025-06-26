#ifndef MYTIPWIDGET_H
#define MYTIPWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QColor>


/**************************************************************/

//   * @file:      mytipwidget.h
//   * @brife:     自定义提示弹框，
//   * @date:      2025/06/10

/**************************************************************/

class TipWidget : public QWidget
{
    Q_OBJECT
public:
    enum StatusEnum{
        SUCCESS,
        FAILED,
        WARNING,
    };

    explicit TipWidget(const QString &text,
                       const QColor &bgColor,
                       TipWidget::StatusEnum type,int time,   ///时间过后自动销毁
                       QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QLabel *iconLabel;
    QLabel *textLabel;
    QColor m_bgColor;
    QTimer *m_timer;
};

#endif // MYTIPWIDGET_H
