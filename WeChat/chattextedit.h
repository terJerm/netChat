#ifndef CHATTEXTEDIT_H
#define CHATTEXTEDIT_H

#include <QTextEdit>
#include <QMimeData>
#include <QDropEvent>
#include <QStyle>
#include <QKeyEvent>

/**************************************************************/

//   * @file:      chattextedit.h
//   * @brife:     自定义输入框，右下角的输入框
//   * @date:      2025/06/10

/**************************************************************/
class ChatTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit ChatTextEdit(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;        ///鼠标按下事件

    void dragEnterEvent(QDragEnterEvent* event) override;  /// 拖动事件，处理文件拖入上传
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void setDefaultStyle();   ///文件拖入/释放的样式设置
    void setDragOverStyle();

signals:
    void sendMessageSignals(QString message);

    void fileDropped(const QString& filePath);
};

#endif // CHATTEXTEDIT_H
