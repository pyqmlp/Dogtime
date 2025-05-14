#ifndef FOCUSMODE_H
#define FOCUSMODE_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QDateTime>
#include <QGraphicsDropShadowEffect>
#include <QCloseEvent>
#include <QPainter>
#include <QKeyEvent>
#include <QSpinBox>
#include <QGroupBox>
#include <QProgressBar>

// 专注模式窗口类
class FocusMode : public QWidget
{
    Q_OBJECT
    
public:
    // 专注模式类型枚举
    enum FocusModeType {
        WeakFocus,   // 弱强制（可随时退出）
        StrongFocus  // 强强制（在设定时间内不可退出）
    };
    
    explicit FocusMode(QWidget *parent = nullptr);
    ~FocusMode();
    
    // 显示全屏专注模式
    void showFullScreen();
    
    // 设置专注模式类型
    void setFocusMode(FocusModeType mode, int focusDurationMinutes = 0);
    
signals:
    void closed();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    
private slots:
    void updateTime();
    void onExitButtonClicked();
    void updateFocusCountdown();
    
private:
    // UI组件
    QLabel *timeLabel;
    QLabel *dateLabel;
    QLabel *motivationLabel;
    QPushButton *exitButton;
    
    // 强制专注模式相关组件
    QLabel *focusCountdownLabel;
    QProgressBar *focusProgressBar;
    
    // 定时器
    QTimer *timer;
    QTimer *focusCountdownTimer;
    
    // 动画
    QPropertyAnimation *fadeInAnimation;
    
    // 专注模式设置
    FocusModeType modeType;
    int focusDurationSeconds;
    int remainingFocusSeconds;
    
    // 内部方法
    void setupUI();
    void setupAnimations();
    void updateFocusUI();
    bool canExitFocusMode();
    
    // 预设鼓励短语
    QStringList motivationalQuotes;
    void loadMotivationalQuotes();
    void setRandomQuote();
    
    // 格式化时间的辅助函数
    QString formatTimeRemaining(int seconds);
};

#endif // FOCUSMODE_H 