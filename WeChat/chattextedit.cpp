#include "chattextedit.h"

ChatTextEdit::ChatTextEdit(QWidget *parent)
    : QTextEdit{parent}
{
    setAcceptDrops(true);
    setDefaultStyle();
}

void ChatTextEdit::keyPressEvent(QKeyEvent *event){
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (event->modifiers() & Qt::ShiftModifier) {
            /// 如果按了Shift，则正常换行
            QTextEdit::keyPressEvent(event);
        } else {
            /// 否则，清空并发出信号
            if(this->toPlainText() == "") return ;
            emit sendMessageSignals(this->toPlainText());
            this->clear();
        }
    } else {
        QTextEdit::keyPressEvent(event);
    }

}

void ChatTextEdit::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        setDragOverStyle();                           /// 高亮
    }
}

void ChatTextEdit::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    setDefaultStyle();                                /// 恢复样式
}

void ChatTextEdit::dropEvent(QDropEvent *event)
{
    setDefaultStyle(); // 恢复样式

    QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        QString filePath = urls.first().toLocalFile();
        qDebug()<<"=============filePath is :"<<filePath;
        if (!filePath.isEmpty()){
            /// qDebug()<<"拖动的文件路径是： "<<filePath;
            emit fileDropped(filePath);
        }
    }else qDebug()<<"inter the else...";
}

void ChatTextEdit::setDefaultStyle()
{
    setStyleSheet(R"(
            QTextEdit {
                border: 1px solid white;
                background: transparent;
            }
        )");
}

void ChatTextEdit::setDragOverStyle()
{
    setStyleSheet(R"(
            QTextEdit {
                border: 3px solid #1E90FF;
                background-color: #e6f7ff;
                border-radius: 8px;
            }
        )");
}
