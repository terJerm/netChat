#ifndef STACKEDWIDGETANIMATOR_H
#define STACKEDWIDGETANIMATOR_H

#include <QObject>
#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QParallelAnimationGroup>

enum class SlideDirection {
    Left,
    Right,
    Up,
    Down
};

/**************************************************************/

//   * @file:      stackedwidgetanimator.h
//   * @brife:     自定义动画系统，为了给各界面切换时提供过渡动画效果，效果不好，实际上没有启用
//   * @date:      2025/06/10

/**************************************************************/

class StackedWidgetAnimator : public QObject
{
    Q_OBJECT
public:
    static void slideToIndex(QStackedWidget *stackWidget, int targetIndex, int duration = 500,
                             SlideDirection direction = SlideDirection::Left);

signals:
};

#endif // STACKEDWIDGETANIMATOR_H










