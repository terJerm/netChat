#include "stackedwidgetanimator.h"
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QLabel>





void StackedWidgetAnimator::slideToIndex(QStackedWidget *stackWidget, int targetIndex, int duration, SlideDirection direction)
{
    if (!stackWidget || targetIndex < 0 || targetIndex >= stackWidget->count())
        return;

    QWidget *currentWidget = stackWidget->currentWidget();
    QWidget *targetWidget = stackWidget->widget(targetIndex);

    if (currentWidget == targetWidget) return;

    int width = stackWidget->width();
    int height = stackWidget->height();

    QRect currentStart, currentEnd, targetStart, targetEnd;

    switch (direction) {
    case SlideDirection::Left:
        currentStart = QRect(0, 0, width, height);
        currentEnd = QRect(-width, 0, width, height);
        targetStart = QRect(width, 0, width, height);
        targetEnd = QRect(0, 0, width, height);
        break;
    case SlideDirection::Right:
        currentStart = QRect(0, 0, width, height);
        currentEnd = QRect(width, 0, width, height);
        targetStart = QRect(-width, 0, width, height);
        targetEnd = QRect(0, 0, width, height);
        break;
    case SlideDirection::Up:
        currentStart = QRect(0, 0, width, height);
        currentEnd = QRect(0, -height, width, height);
        targetStart = QRect(0, height, width, height);
        targetEnd = QRect(0, 0, width, height);
        break;
    case SlideDirection::Down:
        currentStart = QRect(0, 0, width, height);
        currentEnd = QRect(0, height, width, height);
        targetStart = QRect(0, -height, width, height);
        targetEnd = QRect(0, 0, width, height);
        break;
    }

    targetWidget->setGeometry(targetStart);
    targetWidget->show();

    QPropertyAnimation *currentAnimation = new QPropertyAnimation(currentWidget, "geometry");
    currentAnimation->setDuration(duration);
    currentAnimation->setStartValue(currentStart);
    currentAnimation->setEndValue(currentEnd);

    QPropertyAnimation *targetAnimation = new QPropertyAnimation(targetWidget, "geometry");
    targetAnimation->setDuration(duration);
    targetAnimation->setStartValue(targetStart);
    targetAnimation->setEndValue(targetEnd);

    // Fade In/Out Effect with Custom Curve
    QGraphicsOpacityEffect *currentOpacity = new QGraphicsOpacityEffect(currentWidget);
    QGraphicsOpacityEffect *targetOpacity = new QGraphicsOpacityEffect(targetWidget);
    currentWidget->setGraphicsEffect(currentOpacity);
    targetWidget->setGraphicsEffect(targetOpacity);

    QPropertyAnimation *fadeOut = new QPropertyAnimation(currentOpacity, "opacity");
    fadeOut->setDuration(duration / 2);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InQuad); // y = x * x 左半段

    QPropertyAnimation *fadeIn = new QPropertyAnimation(targetOpacity, "opacity");
    fadeIn->setDuration(duration / 2);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutQuad); // y = x * x 右半段

    QParallelAnimationGroup *group = new QParallelAnimationGroup;
    group->addAnimation(currentAnimation);
    group->addAnimation(targetAnimation);
    group->addAnimation(fadeOut);
    group->addAnimation(fadeIn);

    QObject::connect(group, &QParallelAnimationGroup::finished, [=]() {
        stackWidget->setCurrentIndex(targetIndex);
        targetWidget->setGeometry(0, 0, width, height);
        currentWidget->hide();
        currentWidget->setGraphicsEffect(nullptr);
        targetWidget->setGraphicsEffect(nullptr);
        group->deleteLater();
    });

    // Handle the logic to decide if the animation should continue or interrupt
    if (currentAnimation->state() == QAbstractAnimation::Running) {
        // If directions are opposite, cancel the current animation and start the new one
        if ((direction == SlideDirection::Left && targetStart == QRect(width, 0, width, height)) ||
            (direction == SlideDirection::Right && targetStart == QRect(-width, 0, width, height)) ||
            (direction == SlideDirection::Up && targetStart == QRect(0, height, width, height)) ||
            (direction == SlideDirection::Down && targetStart == QRect(0, -height, width, height))) {
            currentAnimation->stop();
            targetWidget->setGeometry(targetStart); // start from the current position
            group->start(); // start new animation immediately
        }
    } else {
        group->start();
    }
}
