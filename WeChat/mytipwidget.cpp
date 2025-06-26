#include "mytipwidget.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QPixmap>
#include <QEvent>

TipWidget::TipWidget(const QString &text,
                     const QColor &bgColor,
                     TipWidget::StatusEnum type,int time,
                     QWidget *parent)

    : QWidget(parent), m_bgColor(bgColor)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setFixedSize(280, 100);

    // 图标
    iconLabel = new QLabel(this);
    QPixmap icon;
    switch (type) {
    case SUCCESS:
        icon = QPixmap(":/res/succ.png");
        break;
    case FAILED:
        icon = QPixmap(":/res/fail.png");
        break;
    case WARNING:
        icon = QPixmap(":/res/bg-warning.png");
        break;
    }
    iconLabel->setPixmap(icon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setFixedSize(36, 36);
    iconLabel->setAlignment(Qt::AlignCenter);

    // 文本
    textLabel = new QLabel(text, this);
    textLabel->setStyleSheet("color: white; font-size: 15px;");
    textLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    // 布局
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(iconLabel);
    layout->addSpacing(10);
    layout->addWidget(textLabel);
    layout->setContentsMargins(20, 10, 20, 10);
    setLayout(layout);

    // 自动居中于 parent
    if (parent) {
        QPoint pos = parent->mapToGlobal(QPoint(
            (parent->width() - width()) / 2,
            (parent->height() - height()) / 2
            ));
        move(pos);

        // 监听父窗口状态变化
        parent->installEventFilter(this);
    }

    // 自动关闭
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &TipWidget::close);
    m_timer->start(time);

    show();
}

void TipWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(m_bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 12, 12);
}

bool TipWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parent() && event->type() == QEvent::WindowStateChange) {
        QWidget *parentWidget = qobject_cast<QWidget *>(parent());
        if (parentWidget && parentWidget->isMinimized()) {
            // 父窗口最小化，立即关闭 TipWidget（会触发析构释放）
            this->close();
        }
    }
    return QWidget::eventFilter(watched, event);
}
