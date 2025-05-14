#ifndef SYSTEMTRAYICON_H
#define SYSTEMTRAYICON_H

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QSettings>
#include <QTimer>
#include <QInputDialog>
#include <QPropertyAnimation>
#include "mainwindow.h"
#include "settingswindow.h"
#include "floatingclock.h"
#include "countdownnotification.h"
#include "focusmode.h"

class SystemTrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    SystemTrayIcon(QObject *parent = nullptr);
    ~SystemTrayIcon();

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void showSettings();
    void showHelp();
    void restartApp();
    
    // 时间显示菜单操作
    void toggleCurrentTimeDisplay(bool checked);
    void toggle24HourFormat(bool checked);
    void toggleShowSeconds(bool checked);
    
    // 番茄时钟菜单操作
    void showPomodoroTimer();
    void startCountdown();
    void pauseCountdown();
    void stopCountdown();
    void updateCountdown();
    void onCountdownFinished();
    
    // 专注模式操作
    void startFocusMode();
    void startWeakFocusMode();
    void startStrongFocusMode();
    void onFocusModeClose();
    
    // 主界面操作
    void showMainWindow();
    
    // 处理悬浮时钟关闭
    void onFloatingClockClosed();

private:
    QMenu *trayMenu;        // 右键菜单
    QMenu *leftClickMenu;   // 左键菜单
    
    // 右键菜单项
    QAction *settingsAction;
    QAction *helpAction;
    QAction *restartAction;
    QAction *quitAction;
    
    // 左键菜单项 - 时间显示
    QMenu *timeDisplayMenu;
    QAction *showCurrentTimeAction;
    QAction *use24HourFormatAction;
    QAction *showSecondsAction;
    
    // 左键菜单项 - 番茄时钟
    QMenu *pomodoroMenu;
    QAction *pomodoroTimerAction;
    QAction *startCountdownAction;
    QAction *pauseCountdownAction;
    QAction *stopCountdownAction;
    
    // 左键菜单项 - 专注模式
    QMenu *focusModeMenu;
    QAction *focusModeAction;
    QAction *weakFocusModeAction;
    QAction *strongFocusModeAction;
    
    // 番茄时钟倒计时相关
    QTimer *countdownTimer;
    int countdownSeconds;
    int initialCountdownSeconds;
    bool isCountdownRunning;
    bool showCountdownClock;
    
    // 左键菜单项 - 主界面
    QAction *mainWindowAction;
    
    MainWindow *mainWindow;
    SettingsWindow *settingsWindow;
    FloatingClock *floatingClock;    // 统一使用这一个时钟窗口
    CountdownNotification *notification; // 倒计时结束提示窗
    FocusMode *focusMode;            // 专注模式窗口
    
    bool isShowingClock;

    void createTrayIcon();
    void createActions();
    void createLeftClickMenu();
    void createConnections();
    void updateFloatingClock();
    void updateTrayIconToolTip();
    void showCountdownNotification();
    
    // 安全销毁悬浮时钟
    void destroyFloatingClock();
    
    // 配置文件管理
    void loadSettings();
    void saveSettings();
    void saveCountdownState();
    void loadCountdownState();
    void createDefaultSettings(QSettings &settings);
};

#endif // SYSTEMTRAYICON_H 