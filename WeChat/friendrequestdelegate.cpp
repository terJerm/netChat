#include "friendrequestdelegate.h"
#include <QPainter>
#include <QMouseEvent>
#include "FriendRequestModel.h"
#include "tcpmgr.h"
#include "usermgr.h"

FriendRequestDelegate::FriendRequestDelegate(QObject *parent)
    : QStyledItemDelegate{parent}
{
    connect(this,&FriendRequestDelegate::acceptFriendApply,this,&FriendRequestDelegate::slot_acceptFriendApply);

}

void FriendRequestDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QRect rect = option.rect;
    bool accepted = index.data(FriendRequestModel::friendRequestAcceptRole).toBool();
    QString avatar = index.data(FriendRequestModel::friendRequestAvatarRole).toString();
    QString name = index.data(FriendRequestModel::friendRequestNameRole).toString();
    QString message = index.data(FriendRequestModel::friendRequestMessageRole).toString();

    // 背景
    ///painter->fillRect(rect, option.state & QStyle::State_Selected ? QColor("#e6f7ff") : Qt::white);

    // 头像
    int margin = 10;
    int avatarSize = rect.height() - 2 * margin;
    QRect avatarRect(rect.left() + margin, rect.top() + margin + 5, avatarSize - 12, avatarSize -12);
    // 绘制头像
    QPixmap avatarMap;
    if (!avatar.isEmpty()) {
        avatarMap.load(avatar);
    }else{
        avatarMap = QPixmap(":/res/chat_res/self_1.png");
    }
    if (!avatarMap.isNull()) {
        painter->drawPixmap(avatarRect, avatarMap.scaled(avatarRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }

    // 姓名
    painter->setPen(Qt::black);
    painter->setFont(QFont("宋体", 11, QFont::Bold));
    painter->drawText(rect.left() + 70, rect.top() + 34, name);

    // 附言（省略号处理）
    painter->setPen(Qt::gray);
    QFont messageFont("宋体", 10);
    painter->setFont(messageFont);

    QFontMetrics metrics(messageFont);
    int maxWidth = rect.width() - 70 - 100;  // 留出按钮区域
    QString elidedMessage = metrics.elidedText(message, Qt::ElideRight, maxWidth);
    painter->drawText(rect.left() + 70, rect.top() + 60, elidedMessage);

    // 按钮
    QRect btnRect(rect.right() - 80, rect.top() + 20, 60, 30);
    painter->setPen(Qt::NoPen);
    if (accepted) {
        painter->setBrush(QColor("#cccccc"));
        painter->drawRoundedRect(btnRect, 5, 5);
        painter->setPen(Qt::gray);
        painter->drawText(btnRect, Qt::AlignCenter, "已添加");
    } else {
        painter->setBrush(QColor("#00aa00"));
        painter->drawRoundedRect(btnRect, 5, 5);
        painter->setPen(Qt::white);
        painter->drawText(btnRect, Qt::AlignCenter, "接受");
    }

    painter->restore();
}

QSize FriendRequestDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return {400, 80};
}

bool FriendRequestDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    Q_UNUSED(model);
    if (event->type() == QEvent::MouseButtonRelease) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        QRect btnRect(option.rect.right() - 80, option.rect.top() + 20, 60, 30);
        if (btnRect.contains(mouseEvent->pos())) {
            bool accepted = index.data(FriendRequestModel::friendRequestAcceptRole).toBool();
            if (!accepted) {
                ///model->setData(index, true, FriendRequestModel::friendRequestAcceptRole);
                /// 这个应该在同意后的回调中设置 accept 为 true

                int userUid = index.data(FriendRequestModel::friendRequestUidRole).toInt();
                qDebug()<<"同意好友请求按钮,对方 uid is："<<userUid;

                emit acceptFriendApply(userUid);
            }
            return true;
        }
    }
    return false;
}

void FriendRequestDelegate::slot_acceptFriendApply(int uid)
{
    QJsonObject obj;
    obj["fromuid"] = UserMgr::getInstance()->getUid();
    obj["touid"] = uid;
    QJsonDocument docu(obj);
    QByteArray byte = docu.toJson(QJsonDocument::Compact);

    /// 使用 tcpMgr 给服务器发送好友认证请求
    TcpMgr::getInstance()->sig_send_data(ReqId::ID_AUTH_FRIEND_REQ,byte);


}

























