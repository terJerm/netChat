#ifndef FRIENDINFOWIDGET_H
#define FRIENDINFOWIDGET_H

#include <QWidget>
#include <memory>
#include "global.h"
class myHoverButton;
class QLabel;
class QLineEdit;

/**************************************************************/

//   * @file:      friendinfowidget.h
//   * @brife:     好友资料界面
//   * @date:      2025/06/10

/**************************************************************/

class friendInfoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit friendInfoWidget(QWidget *parent = nullptr);
    void setNewFriendWId(std::shared_ptr<SearchInfo> info, bool isexist);

    QString getUserName();
    int getUserUid();

private:
    void initUI();

    SearchInfo si;
    QLabel *_avatar_label;
    QLabel *_date_label;
    QLabel *_wxid_label;

    QLabel *_remark_title_label;
    QLineEdit *_remark_edit;

    QLabel* _say_say_lab;
    QLabel* _say_say_show;

    QLabel *_source_title_label;
    QLabel *_source_value_label;

    myHoverButton *_msg_button;
    myHoverButton *_audio_button;
    myHoverButton *_video_button;
    myHoverButton *_add_friend_button;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void _apply_friend_wid_signals();
    void _msg_btn_clicked_signals(int uid);
};

#endif // FRIENDINFOWIDGET_H
