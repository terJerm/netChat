#include "chatmessagelistdelegate.h"
#include "chatmessagelistmodel.h"
#include <QTextDocument>
#include <QPixmap>
#include <QEvent>
#include <QMouseEvent>

chatMessageListDelegate::chatMessageListDelegate(QObject *parent)
    : QStyledItemDelegate{parent},avatarSize(40),maxTextWidth(360),minTextWidth(20)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        m_rotateAngle += 10;
        if (m_rotateAngle >= 360) m_rotateAngle = 0;
        emit needUpdate(); // 通知刷新
    });
    m_timer->start(30); // 每50ms刷新一次
}

void chatMessageListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    painter->save();

    // --- 获取数据 ---
    const QString senderId = index.data(chatMessageListMOdel::SenderIdRole).toString();
    const QString text = index.data(chatMessageListMOdel::TextRole).toString();
    const QString avatar = index.data(chatMessageListMOdel::AvatarRole).toString();
    const MessageStatus status = (MessageStatus)index.data(chatMessageListMOdel::StatusRole).toInt();
    const MessageType type = (MessageType)index.data(chatMessageListMOdel::TypeRole).toInt();

    const bool isSelf = (senderId == "self");
    const QRect rect = option.rect;
    const int margin = 10;
    const int bubblePadding = 10;

    // --- 特殊处理：时间气泡 ---
    if (type == TimeItem) {
        painter->setPen(Qt::gray);
        painter->drawText(rect, Qt::AlignCenter, text);
        painter->restore();
        return;
    }
    // --- 特殊处理：加载动画 ---
    if (type == LoadMessage){
        painter->setRenderHint(QPainter::Antialiasing, true);
        QFont font = painter->font();
        font.setPointSize(10);
        painter->setFont(font);
        painter->setPen(Qt::gray);

        QString loadingText = ".....loading.....";

        // 居中绘制
        painter->drawText(rect, Qt::AlignCenter, loadingText);
        painter->restore();
        return;
    }


    /// 不是特使时间气泡就是消息文本或者文件咯，嘿嘿嘿
    // --- 绘制头像 ---
    QRect avatarRect;
    if (isSelf) {
        avatarRect = QRect(rect.right() - avatarSize - margin, rect.top() + margin, avatarSize, avatarSize);
    } else {
        avatarRect = QRect(rect.left() + margin, rect.top() + margin, avatarSize, avatarSize);
    }

    // 加载头像图片
    QPixmap avatarMap;
    if (!avatar.isEmpty()) {
        avatarMap.load(avatar);
    } else {
        avatarMap = QPixmap(":/res/chat_res/self_1.png");
    }
    // 确保头像加载成功并进行缩放
    painter->drawPixmap(avatarRect, avatarMap.scaled(avatarSize, avatarSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));


    if(type == MessageType::NormalMessage){            ///  普通文本样式
        /// --- 绘制气泡 ---
        QTextDocument textDoc;
        QFont fixedFont = painter->font();
        fixedFont.setFamily("Courier New");  // 常见的等宽字体
        textDoc.setDefaultFont(fixedFont);
        textDoc.setPlainText(text);

        /// 测量自然文本宽度
        QFontMetrics fm(painter->font());
        int naturalWidth = fm.horizontalAdvance(text);

        /// 根据文本长度设置布局宽度
        if (naturalWidth > maxTextWidth - bubblePadding * 2) {
            textDoc.setTextWidth(maxTextWidth);
        } else if (naturalWidth < minTextWidth - bubblePadding * 2) {
            textDoc.setTextWidth(minTextWidth);
        } else {
            textDoc.setTextWidth(naturalWidth + bubblePadding * 2 + 10);     ///增加 10 偏移量，防止自动换行
        }

        /// 计算气泡尺寸
        QSizeF textSize = textDoc.size();
        QRect bubbleRect;
        bubbleRect.setSize(QSize(textSize.width() + 2 * bubblePadding, textSize.height() + 2 * bubblePadding));

        /// 设置气泡位置
        if (isSelf) {
            bubbleRect.moveRight(avatarRect.left() - margin);
            bubbleRect.moveTop(avatarRect.top());
        } else {
            bubbleRect.moveLeft(avatarRect.right() + margin);
            bubbleRect.moveTop(avatarRect.top());
        }

        /// 画气泡背景
        painter->setBrush(isSelf ? QColor("#acf") : QColor("#ccc"));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(bubbleRect, 10, 10);

        /// --- 绘制文字 ---
        painter->save();
        painter->translate(bubbleRect.topLeft() + QPoint(bubblePadding, bubblePadding));
        textDoc.drawContents(painter, QRectF(QPointF(0, 0), textSize));
        painter->restore();

        /// --- 绘制发送中五角星 ---
        if (isSelf && status == Sending) {
            painter->save();
            QPointF starCenter = bubbleRect.topLeft() + QPointF(-20, 15);
            painter->translate(starCenter);
            painter->rotate(m_rotateAngle);
            this->drawStar(painter);
            painter->restore();
        }
        painter->restore();
        return ;
    }

    if(type == MessageType::File){                     /// 文件样式

        /// --- 解析 text 字符串 ---
        QStringList parts = text.split(" ");
        QString fileName = parts.value(0, "未知文件");
        QString fileSize = parts.value(1, "未知大小");
        QString iconPath = parts.value(2, ":/res/fileType/icons8-unknown-100.png");

        /// --- 气泡固定大小 ---
        const QSize bubbleFixedSize(260, 120);

        QRect bubbleRect;
        if (isSelf) {
            bubbleRect = QRect(avatarRect.left() - margin - bubbleFixedSize.width(),
                               avatarRect.top(), bubbleFixedSize.width(), bubbleFixedSize.height());
        } else {
            bubbleRect = QRect(avatarRect.right() + margin,
                               avatarRect.top(), bubbleFixedSize.width(), bubbleFixedSize.height());
        }

        /// --- 背景气泡 ---
        painter->setBrush(isSelf ? QColor("#acf") : QColor("#ccc"));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(bubbleRect, 10, 10);

        /// --- 图标区域 ---
        QRect iconRect(bubbleRect.left() + 10, bubbleRect.top() + 10, 60, 60);
        QPixmap fileIcon(iconPath);
        painter->drawPixmap(iconRect, fileIcon.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

        /// --- 设置字体样式：宋体，10px，青蓝色 ---
        QFont font("宋体", 10);
        painter->setFont(font);
        painter->setPen(QColor("#0081a7"));  // 青蓝色

        /// --- 文本区域 ---
        QRect fileNameRect(iconRect.right() + 10, iconRect.top(), bubbleRect.width() - 80, 20);
        QRect fileSizeRect(iconRect.right() + 10, iconRect.top() + 22, bubbleRect.width() - 80, 20);
        painter->drawText(fileNameRect, Qt::AlignLeft | Qt::AlignVCenter, fileName);
        painter->drawText(fileSizeRect, Qt::AlignLeft | Qt::AlignVCenter, fileSize);

        /// --- 下载按钮 ---
        QRect downloadBtnRect(bubbleRect.left() + 10, iconRect.bottom() + 10, (bubbleRect.width() - 30) / 2, 25);
        painter->setBrush(status == MessageStatus::Sending ? QColor("#8ecae6") : QColor("#d3d3d3")); // 未下载高亮，否则灰色
        painter->setPen(QPen(Qt::gray, 1));
        painter->drawRoundedRect(downloadBtnRect, 5, 5);
        painter->setPen(Qt::black);
        painter->drawText(downloadBtnRect, Qt::AlignCenter, "下载");

        /// --- 打开按钮 ---
        QRect openBtnRect(downloadBtnRect.right() + 10, downloadBtnRect.top(), (bubbleRect.width() - 30) / 2, 25);
        painter->setBrush(QColor("#8ecae6"));
        painter->setPen(QPen(Qt::gray, 1));
        painter->drawRoundedRect(openBtnRect, 5, 5);
        painter->setPen(Qt::black);
        painter->drawText(openBtnRect, Qt::AlignCenter, "打开");

        painter->restore();
        return;
    }

}

QSize chatMessageListDelegate::sizeHint(const QStyleOptionViewItem &option,const QModelIndex &index) const
{
    const int margin = 10;
    const int bubblePadding = 10;

    int type = index.data(chatMessageListMOdel::TypeRole).toInt();
    QString text = index.data(chatMessageListMOdel::TextRole).toString();

    if (type == MessageType::File) {
        // 文件消息：固定宽高（保持和 paint 中一致）
        const QSize bubbleFixedSize(240, 120);
        int totalHeight = qMax(avatarSize, bubbleFixedSize.height()) + 2 * margin;
        return QSize(option.rect.width(), totalHeight);
    }

    // 普通文本消息（NormalMessage）
    QFontMetrics fm(option.font);
    int naturalWidth = fm.horizontalAdvance(text);

    int finalTextWidth = 0;
    if (naturalWidth > maxTextWidth - bubblePadding * 2) {
        finalTextWidth = maxTextWidth;
    } else if (naturalWidth < minTextWidth - bubblePadding * 2) {
        finalTextWidth = minTextWidth;
    } else {
        finalTextWidth = naturalWidth + bubblePadding * 2 + 10;  // 防止误换行
    }

    QTextDocument textDoc;
    textDoc.setDefaultFont(option.font);
    textDoc.setPlainText(text);
    textDoc.setTextWidth(finalTextWidth);

    QSizeF textSize = textDoc.size();
    int bubbleHeight = textSize.height() + 2 * bubblePadding;
    int totalHeight = qMax(avatarSize, bubbleHeight) + 2 * margin;

    return QSize(option.rect.width(), totalHeight);
}

void chatMessageListDelegate::drawStar(QPainter *painter) const
{
    // 绘制五角星
    QPen pen(QColor(78, 255, 255), 2); // 深灰色
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    const int r_outer = 8; // 外圈顶点半径
    const int r_inner = 4; // 内圈凹陷半径
    QPolygonF starPolygon;

    for (int i = 0; i < 10; ++i) {
        double angle = i * 36.0; // 每个点36度
        double r = (i % 2 == 0) ? r_outer : r_inner;
        QPointF point(r * cos(qDegreesToRadians(angle)), r * sin(qDegreesToRadians(angle)));
        starPolygon << point;
    }

    painter->drawPolygon(starPolygon);
}

bool chatMessageListDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{

    Q_UNUSED(model);
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QPoint clickPos = mouseEvent->pos();

        int type = index.data(chatMessageListMOdel::TypeRole).toInt();
        if (type != MessageType::File)
            return false;

        QString senderId = index.data(chatMessageListMOdel::SenderIdRole).toString();
        bool isSelf = (senderId == "self");

        const QRect rect = option.rect;
        const int margin = 10;
        const int avatarSize = 40;
        const QSize bubbleFixedSize(260, 120);

        // --- 头像区域 ---
        QRect avatarRect = isSelf
                               ? QRect(rect.right() - avatarSize - margin, rect.top() + margin, avatarSize, avatarSize)
                               : QRect(rect.left() + margin, rect.top() + margin, avatarSize, avatarSize);

        // --- 气泡区域 ---
        QRect bubbleRect = isSelf
                               ? QRect(avatarRect.left() - margin - bubbleFixedSize.width(),
                                       avatarRect.top(), bubbleFixedSize.width(), bubbleFixedSize.height())
                               : QRect(avatarRect.right() + margin,
                                       avatarRect.top(), bubbleFixedSize.width(), bubbleFixedSize.height());

        // --- 按钮区域计算（必须与 paint 中一致） ---
        QRect iconRect(bubbleRect.left() + 10, bubbleRect.top() + 10, 60, 60);
        QRect downloadBtnRect(bubbleRect.left() + 10, iconRect.bottom() + 10, (bubbleRect.width() - 30) / 2, 25);
        QRect openBtnRect(downloadBtnRect.right() + 10, downloadBtnRect.top(), (bubbleRect.width() - 30) / 2, 25);

        // --- 响应点击 ---
        if (downloadBtnRect.contains(clickPos)) {
            emit downloadClicked(index);
            return true;
        } else if (openBtnRect.contains(clickPos)) {
            emit openClicked(index);
            return true;
        }
    }
    return false;
}
