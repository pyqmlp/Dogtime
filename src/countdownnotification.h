#ifndef COUNTDOWNNOTIFICATION_H
#define COUNTDOWNNOTIFICATION_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QPropertyAnimation>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QSequentialAnimationGroup>

// 自定义倒计时结束提示窗
class CountdownNotification : public QWidget
{
    Q_OBJECT
    // 为旋转动画添加属性
    Q_PROPERTY(qreal angle READ getAngle WRITE setAngle)
    
public:
    explicit CountdownNotification(QWidget *parent = nullptr);
    virtual ~CountdownNotification();
    
    void showNotification(const QString &title, const QString &message);
    
    // 角度属性访问器
    qreal getAngle() const;
    void setAngle(qreal newAngle);
    
signals:
    void closed();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    
private slots:
    void onCloseButtonClicked();
    void startAlarmAnimation();
    
private:
    QLabel *titleLabel;
    QLabel *messageLabel;
    QPushButton *closeButton;
    QTimer *autoCloseTimer;
    
    // 动画相关成员
    QPropertyAnimation *shakeAnimation;
    QPropertyAnimation *rotateAnimation;
    QSequentialAnimationGroup *animationGroup;
    qreal angle;
    
    void setupUI();
    void setupAnimations();
    void drawAlarmClockPattern(QPainter *painter);
};

#endif // COUNTDOWNNOTIFICATION_H 