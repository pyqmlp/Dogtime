#ifndef FLOATINGCLOCK_H
#define FLOATINGCLOCK_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPoint>
#include <QPushButton>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QPropertyAnimation>
#include <QSettings>

class FloatingClock : public QWidget
{
    Q_OBJECT
    // 使用Qt内置的windowOpacity属性，无需自定义属性
    Q_PROPERTY(QRect geometry READ geometry WRITE setGeometry)

public:
    explicit FloatingClock(QWidget *parent = nullptr);
    ~FloatingClock();

    // 设置时间格式选项
    void setUse24HourFormat(bool use24Hour);
    void setShowSeconds(bool showSeconds);
    
    // 获取当前设置
    bool isUse24HourFormat() const { return use24HourFormat; }
    bool isShowSeconds() const { return showSeconds; }
    bool isLocked() const { return locked; }

    // 倒计时显示功能
    void showCountdown(int seconds);
    void stopCountdown();
    void updateCountdownDisplay(int seconds);
    bool isCountdownMode() const { return countdownMode; }
    
    // 保存和加载窗口位置
    void savePosition();
    void loadPosition();
    
    // 安全销毁前的准备工作
    void prepareForDestruction(bool sendSignal = true);

public slots:
    void setLocked(bool lock);
    void toggleLocked();
    void show(); // 重写show方法以添加动画效果
    void hide(); // 重写hide方法，保存位置
    void close(); // 重写close方法，保存位置

signals:
    void settingsRequested();
    void lockStateChanged(bool locked);
    void closeRequested();
    void closed();
    void countdownFinished();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void updateTime();
    void onSettingsClicked();
    void onCloseClicked();
    void onShowAnimationFinished();

private:
    QLabel *timeLabel;       // 主时间标签
    QTimer *timer;
    
    // 控制栏相关部件
    QWidget *controlBar;
    QPushButton *lockButton;
    QPushButton *settingsButton;
    QPushButton *closeButton;
    
    // 状态变量 - 与初始化列表顺序保持一致
    bool use24HourFormat;
    bool showSeconds;
    bool locked;
    bool mouseOver;
    bool countdownMode;
    bool isDragging;
    
    // 倒计时相关
    int countdownSeconds;
    
    // 用于拖动窗口
    QPoint dragPosition;
    
    // 动画相关成员
    QPropertyAnimation *fadeAnimation;
    QPropertyAnimation *bounceAnimation;
    
    // 位置是否已加载
    bool positionLoaded;
    
    // 关闭状态标志，防止多次触发关闭
    bool isClosing;
    
    void updateTimeFormat();
    void setupUI();
    void setupControlBar();
    void updateControlBarVisibility();
    void updateLockButtonIcon();
    QString formatTime(int seconds) const;
    void setupAnimations();
    void playShowAnimation();
};

#endif // FLOATINGCLOCK_H 