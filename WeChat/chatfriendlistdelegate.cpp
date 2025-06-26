#include "chatfriendlistdelegate.h"
#include "chatfriendlistmodel.h"

chatFriendListDelegate::chatFriendListDelegate(QObject *parent)
    : QStyledItemDelegate{parent}
{}

void chatFriendListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QRect rect = option.rect;
    rect.setWidth(option.widget->width()); // 强制用listView的可视宽度


    // 绘制悬停/选中背景
    if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(rect, QColor(230, 230, 230)); // 鼠标悬停时浅灰背景
    } else if (option.state & QStyle::State_Selected) {
        painter->fillRect(rect, QColor(200, 200, 200)); // 如果还想兼容选中状态，也可以加
    }

    bool isTop = index.data(chatFriendListModel::friendIsTopItemRole).toBool();
    QString name = index.data(chatFriendListModel::friendNameRole).toString();
    QString avatarPath = index.data(chatFriendListModel::friendAvatarRole).toString();


    // 分割线（在固定项后面画一条线）
    if (isTop) {
        QRect lineRect(rect.left(), rect.bottom() - 1, rect.width(), 1);
        painter->fillRect(lineRect, Qt::gray);
    }

    // 绘制头像
    int avatarSize = 40;
    int verticalCenter = option.rect.top() + (option.rect.height() - avatarSize) / 2;
    QRect avatarRect(option.rect.left() + 10, verticalCenter, avatarSize, avatarSize);

    // 加载头像图片
    QPixmap avatar;
    if (!avatarPath.isEmpty()) {
        avatar.load(avatarPath);
    } else {
        // 如果路径为空，可以加载一个默认的占位头像
        avatar = QPixmap(":/res/chat_res/self_1.png"); // 替换为你的默认头像路径
    }
    // 确保头像加载成功并进行缩放
    if (!avatar.isNull()) {
        painter->drawPixmap(avatarRect, avatar.scaled(avatarSize, avatarSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    // 绘制名字
    QRect textRect = rect.adjusted(60, 10, -10, -10);
    QFont nameFont("宋体", 11, QFont::Bold);
    painter->setFont(nameFont);
    painter->setPen(QColor::fromRgb(62,214,209,255));
    QFontMetrics fm(painter->font());
    QString elidedName = fm.elidedText(name, Qt::ElideRight, textRect.width() - 10);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedName);


    painter->restore();
}

QSize chatFriendListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(200, 60);
}
