#include "systemtrayicon.h"
#include <QApplication>
#include <QProcess>
#include <QMessageBox>
#include <QStyle>
#include <QFileInfo>
#include <QDateTime>
#include <QTimer>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QInputDialog>
#include <QGuiApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
// 添加Windows.h用于MessageBeep
#ifdef _WIN32
#include <Windows.h>
#endif

SystemTrayIcon::SystemTrayIcon(QObject *parent)
    : QSystemTrayIcon(parent)
    , trayMenu(new QMenu())
    , leftClickMenu(new QMenu())
    , settingsAction(nullptr)
    , helpAction(nullptr)
    , restartAction(nullptr)
    , quitAction(nullptr)
    , timeDisplayMenu(nullptr)
    , showCurrentTimeAction(nullptr)
    , use24HourFormatAction(nullptr)
    , showSecondsAction(nullptr)
    , pomodoroMenu(nullptr)
    , pomodoroTimerAction(nullptr)
    , startCountdownAction(nullptr)
    , pauseCountdownAction(nullptr)
    , stopCountdownAction(nullptr)
    , focusModeMenu(nullptr)
    , focusModeAction(nullptr)
    , weakFocusModeAction(nullptr)
    , strongFocusModeAction(nullptr)
    , countdownTimer(new QTimer(this))
    , countdownSeconds(0)
    , initialCountdownSeconds(0)
    , isCountdownRunning(false)
    , showCountdownClock(true)
    , mainWindowAction(nullptr)
    , mainWindow(nullptr)
    , settingsWindow(nullptr)
    , floatingClock(nullptr)
    , notification(nullptr)
    , focusMode(nullptr)
    , isShowingClock(true)
{
    // 读取配置文件
    loadSettings();
    
    // 尝试使用ico图标，如果不存在则使用png图标
    QIcon appIcon(":/icons/dogtime.ico");
    if (appIcon.isNull()) {
        appIcon = QIcon(":/icons/dogtime.png");
    }
    
    // 如果自定义图标仍然不可用，则使用标准图标
    if (appIcon.isNull()) {
        appIcon = QApplication::style()->standardIcon(QStyle::SP_DesktopIcon);
    }
    
    setIcon(appIcon);
    setToolTip("Dogtime");
    
    // 设置应用程序图标，这将影响任务栏图标
    QApplication::setWindowIcon(appIcon);
    
    // 创建托盘菜单和相关动作
    createActions();
    createTrayIcon();
    createLeftClickMenu();
    createConnections();
    
    // 创建主窗口（但不显示）
    mainWindow = new MainWindow();
    
    // 连接倒计时定时器
    connect(countdownTimer, &QTimer::timeout, this, &SystemTrayIcon::updateCountdown);
    
    // 加载倒计时状态
    loadCountdownState();
    
    // 默认显示悬浮时钟
    QTimer::singleShot(100, this, &SystemTrayIcon::updateFloatingClock);
    
    // 程序退出时保存状态
    connect(qApp, &QApplication::aboutToQuit, this, &SystemTrayIcon::saveCountdownState);
}

SystemTrayIcon::~SystemTrayIcon()
{
    // 保存倒计时状态和设置 - 在阻断信号前执行
    try {
        saveCountdownState();
        saveSettings();
    } catch (const std::exception &e) {
        qWarning("在析构函数中保存状态时发生错误: %s", e.what());
    }
    
    // 现在再阻断信号，保证退出信号能正常处理
    this->blockSignals(true);
    
    // 销毁悬浮时钟 - 使用我们的安全销毁方法
    destroyFloatingClock();
    
    // 处理专注模式窗口
    if (focusMode) {
        focusMode->blockSignals(true);
        QWidget* tempFocusMode = focusMode;
        focusMode = nullptr;
        disconnect(tempFocusMode, nullptr, this, nullptr);
        delete tempFocusMode;
    }
    
    // 处理notification对象
    if (notification) {
        notification->blockSignals(true);
        QWidget* tempNotification = notification;
        notification = nullptr;
        disconnect(tempNotification, nullptr, this, nullptr);
        delete tempNotification;
    }
    
    // 处理settingsWindow对象
    if (settingsWindow) {
        settingsWindow->blockSignals(true);
        QWidget* tempSettings = settingsWindow;
        settingsWindow = nullptr;
        disconnect(tempSettings, nullptr, this, nullptr);
        delete tempSettings;
    }
    
    // 处理mainWindow对象
    if (mainWindow) {
        mainWindow->blockSignals(true);
        QWidget* tempMainWindow = mainWindow;
        mainWindow = nullptr;
        disconnect(tempMainWindow, nullptr, this, nullptr);
        delete tempMainWindow;
    }
    
    // 处理菜单对象
    // 断开菜单项的所有连接，防止在析构过程中触发信号
    if (trayMenu) {
        trayMenu->blockSignals(true);
        QList<QAction*> actions = trayMenu->actions();
        for (QAction* action : actions) {
            if (action) action->blockSignals(true);
        }
        
        delete trayMenu;
        trayMenu = nullptr;
    }
    
    if (leftClickMenu) {
        leftClickMenu->blockSignals(true);
        QList<QAction*> actions = leftClickMenu->actions();
        for (QAction* action : actions) {
            if (action) action->blockSignals(true);
        }
        
        delete leftClickMenu;
        leftClickMenu = nullptr;
    }
    
    // 最后，解除托盘图标的注册
    this->hide();
}

void SystemTrayIcon::createActions()
{
    // 右键菜单项
    settingsAction = new QAction("设置", this);
    helpAction = new QAction("帮助", this);
    restartAction = new QAction("重新启动", this);
    quitAction = new QAction("退出", this);
    
    // 左键菜单项 - 时间显示
    showCurrentTimeAction = new QAction("显示当前时间", this);
    showCurrentTimeAction->setCheckable(true);
    showCurrentTimeAction->setChecked(isShowingClock);
    
    use24HourFormatAction = new QAction("24小时制", this);
    use24HourFormatAction->setCheckable(true);
    use24HourFormatAction->setChecked(true);
    
    showSecondsAction = new QAction("显示秒数", this);
    showSecondsAction->setCheckable(true);
    showSecondsAction->setChecked(false);
    
    // 左键菜单项 - 番茄时钟
    pomodoroTimerAction = new QAction("番茄时钟", this);
    
    // 番茄时钟 - 倒计时控制
    startCountdownAction = new QAction("开始", this);
    pauseCountdownAction = new QAction("暂停", this);
    pauseCountdownAction->setEnabled(false);
    stopCountdownAction = new QAction("取消", this);
    stopCountdownAction->setEnabled(false);
    
    // 左键菜单项 - 专注模式
    focusModeAction = new QAction("专注模式", this);
    weakFocusModeAction = new QAction("弱专注模式", this);
    strongFocusModeAction = new QAction("强专注模式", this);
    
    // 左键菜单项 - 主界面
    mainWindowAction = new QAction("主界面", this);
}

void SystemTrayIcon::createTrayIcon()
{
    trayMenu->addAction(settingsAction);
    trayMenu->addAction(helpAction);
    trayMenu->addSeparator();
    trayMenu->addAction(restartAction);
    trayMenu->addAction(quitAction);
    
    setContextMenu(trayMenu);
}

void SystemTrayIcon::createLeftClickMenu()
{
    // 创建时间显示子菜单
    timeDisplayMenu = new QMenu("时间显示");
    timeDisplayMenu->addAction(showCurrentTimeAction);
    timeDisplayMenu->addAction(use24HourFormatAction);
    timeDisplayMenu->addAction(showSecondsAction);
    
    // 创建番茄时钟子菜单
    pomodoroMenu = new QMenu("番茄钟");
    pomodoroMenu->addAction(startCountdownAction);
    pomodoroMenu->addAction(pauseCountdownAction);
    pomodoroMenu->addAction(stopCountdownAction);
    
    // 添加菜单项到左键菜单
    leftClickMenu->addMenu(timeDisplayMenu);
    
    // 添加番茄钟子菜单
    leftClickMenu->addMenu(pomodoroMenu);
    
    // 创建专注模式子菜单
    focusModeMenu = new QMenu("专注模式");
    focusModeMenu->addAction(weakFocusModeAction);
    focusModeMenu->addAction(strongFocusModeAction);
    
    // 添加专注模式选项
    leftClickMenu->addSeparator();
    leftClickMenu->addMenu(focusModeMenu);
    
    leftClickMenu->addSeparator();
    leftClickMenu->addAction(mainWindowAction);
    
    // 设置弹出模式，使子菜单保持打开状态
    leftClickMenu->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // 确保番茄钟的开始按钮始终可用
    startCountdownAction->setEnabled(true);
}

void SystemTrayIcon::createConnections()
{
    // 连接托盘图标激活信号
    connect(this, &QSystemTrayIcon::activated, this, &SystemTrayIcon::onTrayIconActivated);
    
    // 连接右键菜单动作信号
    connect(settingsAction, &QAction::triggered, this, &SystemTrayIcon::showSettings);
    connect(helpAction, &QAction::triggered, this, &SystemTrayIcon::showHelp);
    connect(restartAction, &QAction::triggered, this, &SystemTrayIcon::restartApp);
    connect(quitAction, &QAction::triggered, QCoreApplication::instance(), &QCoreApplication::quit);
    
    // 连接左键菜单动作信号 - 时间显示
    connect(showCurrentTimeAction, &QAction::triggered, this, &SystemTrayIcon::toggleCurrentTimeDisplay);
    connect(use24HourFormatAction, &QAction::triggered, this, &SystemTrayIcon::toggle24HourFormat);
    connect(showSecondsAction, &QAction::triggered, this, &SystemTrayIcon::toggleShowSeconds);
    
    // 连接左键菜单动作信号 - 番茄时钟
    // pomodoroTimerAction不再使用，直接连接startCountdownAction
    // connect(pomodoroTimerAction, &QAction::triggered, this, &SystemTrayIcon::showPomodoroTimer);
    connect(startCountdownAction, &QAction::triggered, this, &SystemTrayIcon::startCountdown);
    connect(pauseCountdownAction, &QAction::triggered, this, &SystemTrayIcon::pauseCountdown);
    connect(stopCountdownAction, &QAction::triggered, this, &SystemTrayIcon::stopCountdown);
    
    // 连接左键菜单动作信号 - 专注模式
    connect(weakFocusModeAction, &QAction::triggered, this, &SystemTrayIcon::startWeakFocusMode);
    connect(strongFocusModeAction, &QAction::triggered, this, &SystemTrayIcon::startStrongFocusMode);
    
    // 连接左键菜单动作信号 - 主界面
    connect(mainWindowAction, &QAction::triggered, this, &SystemTrayIcon::showMainWindow);
}

void SystemTrayIcon::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        // 清空并重建左键菜单
        leftClickMenu->clear();
        createLeftClickMenu();
        
        // 左键点击，显示左键菜单
        QPoint cursorPos = QCursor::pos();
        leftClickMenu->popup(cursorPos);
    }
}

void SystemTrayIcon::showSettings()
{
    // 创建并显示设置窗口
    if (!settingsWindow) {
        settingsWindow = new SettingsWindow();
    }
    
    // 如果窗口已存在但被隐藏，则显示它
    if (!settingsWindow->isVisible()) {
        settingsWindow->show();
        settingsWindow->activateWindow(); // 确保窗口获得焦点
    } else {
        // 如果窗口已经显示，则将其置为活动窗口
        settingsWindow->activateWindow();
    }
}

void SystemTrayIcon::showHelp()
{
    // 这里可以实现帮助功能，暂时用消息框代替
    QMessageBox::information(nullptr, "帮助", "Dogtime 应用程序帮助信息");
}

void SystemTrayIcon::restartApp()
{
    // 重启应用程序
    QStringList args = QApplication::arguments();
    args.removeFirst(); // 移除程序名
    
    QProcess::startDetached(QApplication::applicationFilePath(), args);
    QApplication::quit();
}

void SystemTrayIcon::updateFloatingClock()
{
    // 增加防止重入的保护，避免在操作过程中重复调用
    static bool isProcessing = false;
    if (isProcessing) return;
    
    isProcessing = true;
    
    try {
        // 如果不需要显示时钟，直接返回
        if (!isShowingClock) {
            if (floatingClock) {
                // 不再显示时钟，安全销毁它
                destroyFloatingClock();
            }
            isProcessing = false;
            return;
        }
        
        // 如果悬浮时钟不存在，则创建
        if (!floatingClock) {
            try {
                floatingClock = new FloatingClock();
                
                // 设置时钟格式 - 增加额外的安全检查
                if (use24HourFormatAction && use24HourFormatAction->isChecked()) {
                    floatingClock->setUse24HourFormat(true);
                } else {
                    floatingClock->setUse24HourFormat(false); // 默认值
                }
                
                if (showSecondsAction && showSecondsAction->isChecked()) {
                    floatingClock->setShowSeconds(true);
                } else {
                    floatingClock->setShowSeconds(false); // 默认值
                }
                
                // 连接设置请求信号
                connect(floatingClock, &FloatingClock::settingsRequested, this, &SystemTrayIcon::showSettings);
                
                // 连接锁定状态变化信号
                connect(floatingClock, &FloatingClock::lockStateChanged, this, [this](bool /*locked*/) {
                    // 这里可以根据锁定状态更新其他UI元素
                });
                
                // 连接关闭信号
                connect(floatingClock, &FloatingClock::closed, this, &SystemTrayIcon::onFloatingClockClosed);
                
                // 检查是否需要显示倒计时
                if (isCountdownRunning || countdownSeconds > 0) {
                    // 通过show()方法显示时会自动播放打开动画
                    floatingClock->show();
                    // 显示倒计时
                    floatingClock->showCountdown(countdownSeconds);
                } else {
                    // 通过show()方法显示时会自动播放打开动画
                    floatingClock->show();
                }
            } catch (const std::exception &e) {
                qWarning("创建悬浮时钟失败: %s", e.what());
                // 确保清理任何可能部分创建的对象
                destroyFloatingClock();
                // 更新UI状态
                isShowingClock = false;
                if (showCurrentTimeAction) {
                    showCurrentTimeAction->blockSignals(true);
                    showCurrentTimeAction->setChecked(false);
                    showCurrentTimeAction->blockSignals(false);
                }
                isProcessing = false;
                return;
            }
        } 
        // 如果悬浮时钟存在，更新其设置
        else if (floatingClock) {
            try {
                // 安全检查：确保动作有效且存在后再使用
                if (use24HourFormatAction) {
                    floatingClock->setUse24HourFormat(use24HourFormatAction->isChecked());
                }
                
                if (showSecondsAction) {
                    floatingClock->setShowSeconds(showSecondsAction->isChecked());
                }
                
                if (!floatingClock->isVisible()) {
                    // 如果之前隐藏了，重新显示时会播放动画
                    floatingClock->show();
                    
                    // 如果有倒计时，同时更新倒计时显示
                    if (isCountdownRunning || countdownSeconds > 0) {
                        floatingClock->showCountdown(countdownSeconds);
                    }
                } else if (isCountdownRunning || countdownSeconds > 0) {
                    // 如果已经显示且有倒计时，确保倒计时模式正确
                    if (!floatingClock->isCountdownMode()) {
                        floatingClock->showCountdown(countdownSeconds);
                    }
                }
            } catch (const std::exception &e) {
                qWarning("更新悬浮时钟设置失败: %s", e.what());
                // 如果更新失败，尝试重新创建时钟
                destroyFloatingClock();
                // 递归调用以重新创建时钟
                isProcessing = false;
                QTimer::singleShot(100, this, &SystemTrayIcon::updateFloatingClock);
                return;
            }
        }
    } catch (const std::exception &e) {
        // 访问悬浮时钟失败，清理指针
        qWarning("更新悬浮时钟失败: %s", e.what());
        destroyFloatingClock();
        
        // 更新菜单状态
        if (showCurrentTimeAction) {
            showCurrentTimeAction->blockSignals(true);
            showCurrentTimeAction->setChecked(false);
            showCurrentTimeAction->blockSignals(false);
        }
        isShowingClock = false;
    } catch (...) {
        // 捕获任何其他异常
        qWarning("更新悬浮时钟时发生未知错误");
        destroyFloatingClock();
        
        // 更新菜单状态
        if (showCurrentTimeAction) {
            showCurrentTimeAction->blockSignals(true);
            showCurrentTimeAction->setChecked(false);
            showCurrentTimeAction->blockSignals(false);
        }
        isShowingClock = false;
    }
    
    isProcessing = false;
}

// 安全销毁悬浮时钟
void SystemTrayIcon::destroyFloatingClock()
{
    // 如果悬浮时钟不存在，直接返回
    if (!floatingClock) {
        return;
    }
    
    // 先断开所有信号连接
    disconnect(floatingClock, nullptr, this, nullptr);
    disconnect(this, nullptr, floatingClock, nullptr);
    
    // 隐藏窗口
    if (floatingClock->isVisible()) {
        floatingClock->hide();
    }
    
    // 安全删除
    FloatingClock* tempClock = floatingClock;
    floatingClock = nullptr;
    tempClock->deleteLater();
}

// 添加处理时钟关闭的方法
void SystemTrayIcon::onFloatingClockClosed()
{
    // 更新菜单项状态
    if (showCurrentTimeAction) {
        showCurrentTimeAction->blockSignals(true);
        showCurrentTimeAction->setChecked(false);
        showCurrentTimeAction->blockSignals(false);
    }
    
    // 更新状态变量
    isShowingClock = false;
    
    // 立即销毁时钟对象
    destroyFloatingClock();
    
    // 保存设置
    saveSettings();
}

// 时间显示菜单操作的实现
void SystemTrayIcon::toggleCurrentTimeDisplay(bool checked)
{
    // 更新状态
    isShowingClock = checked;
    
    // 确保showCurrentTimeAction状态与checked一致
    if (showCurrentTimeAction && showCurrentTimeAction->isChecked() != checked) {
        showCurrentTimeAction->blockSignals(true);
        showCurrentTimeAction->setChecked(checked);
        showCurrentTimeAction->blockSignals(false);
    }
    
    if (checked) {
        // 如果选中了显示，但时钟不存在，则创建它
        updateFloatingClock();
    } else {
        // 如果取消选中，销毁时钟
        destroyFloatingClock();
    }

    // 保存设置
    saveSettings();
}

void SystemTrayIcon::toggle24HourFormat(bool checked)
{
    // 保存设置
    QSettings settings("DogSoft", "Dogtime");
    settings.setValue("floatingclock/use24hour", checked);
    settings.sync();
    
    // 安全地更新浮动时钟，如果它存在的话
    if (floatingClock && isShowingClock) {
        floatingClock->setUse24HourFormat(checked);
    }
}

void SystemTrayIcon::toggleShowSeconds(bool checked)
{
    // 保存设置
    QSettings settings("DogSoft", "Dogtime");
    settings.setValue("floatingclock/showseconds", checked);
    settings.sync();
    
    // 安全地更新浮动时钟，如果它存在的话
    if (floatingClock && isShowingClock) {
        floatingClock->setShowSeconds(checked);
    }
}

// 番茄时钟菜单操作的实现
void SystemTrayIcon::showPomodoroTimer()
{
    // 显示番茄时钟菜单
    QPoint cursorPos = QCursor::pos();
    pomodoroMenu->popup(cursorPos);
}

void SystemTrayIcon::startCountdown()
{
    try {
        // 创建一个对话框来选择倒计时时间
        QDialog dialog;
        dialog.setWindowTitle("设置倒计时");
        dialog.setFixedWidth(320);
        
        QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
        
        // 添加说明标签
        QLabel *label = new QLabel("选择或输入倒计时时间:", &dialog);
        mainLayout->addWidget(label);
        
        // 添加常用番茄时间选项
        QHBoxLayout *presetLayout = new QHBoxLayout();
        QVector<int> presetTimes = {5, 10, 25, 30, 45, 60};
        QVector<QPushButton*> presetButtons;
        
        // 定义选中和未选中的样式
        QString normalButtonStyle = 
            "QPushButton {"
            "   background-color: #FF9800;"
            "   color: white;"
            "   border: none;"
            "   border-radius: 4px;"
            "   padding: 5px 10px;"
            "   font-size: 11pt;"
            "}"
            "QPushButton:hover { background-color: #F57C00; }"
            "QPushButton:pressed { background-color: #E65100; }";
            
        QString selectedButtonStyle = 
            "QPushButton {"
            "   background-color: #E65100;"
            "   color: white;"
            "   border: none;"
            "   border-radius: 4px;"
            "   padding: 5px 10px;"
            "   font-size: 11pt;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #E65100; }"
            "QPushButton:pressed { background-color: #E65100; }";
        
        for (int time : presetTimes) {
            QPushButton *presetBtn = new QPushButton(QString::number(time), &dialog);
            presetBtn->setFixedHeight(40);
            presetBtn->setStyleSheet(normalButtonStyle);
            presetLayout->addWidget(presetBtn);
            presetButtons.append(presetBtn);
        }
        mainLayout->addLayout(presetLayout);
        
        // 添加自定义时分秒输入
        QGroupBox *customGroup = new QGroupBox("自定义时间", &dialog);
        QHBoxLayout *customLayout = new QHBoxLayout(customGroup);
        
        // 小时输入框
        QSpinBox *hoursSpinBox = new QSpinBox(&dialog);
        hoursSpinBox->setRange(0, 23);
        hoursSpinBox->setValue(0);
        hoursSpinBox->setSuffix(" 时");
        hoursSpinBox->setFixedHeight(30);
        hoursSpinBox->setStyleSheet("font-size: 11pt; padding: 2px 5px;");
        customLayout->addWidget(hoursSpinBox);
        
        // 分钟输入框
        QSpinBox *minutesSpinBox = new QSpinBox(&dialog);
        minutesSpinBox->setRange(0, 59);
        minutesSpinBox->setValue(25); // 默认25分钟
        minutesSpinBox->setSuffix(" 分");
        minutesSpinBox->setFixedHeight(30);
        minutesSpinBox->setStyleSheet("font-size: 11pt; padding: 2px 5px;");
        customLayout->addWidget(minutesSpinBox);
        
        // 秒钟输入框
        QSpinBox *secondsSpinBox = new QSpinBox(&dialog);
        secondsSpinBox->setRange(0, 59);
        secondsSpinBox->setValue(0);
        secondsSpinBox->setSuffix(" 秒");
        secondsSpinBox->setFixedHeight(30);
        secondsSpinBox->setStyleSheet("font-size: 11pt; padding: 2px 5px;");
        customLayout->addWidget(secondsSpinBox);
        
        mainLayout->addWidget(customGroup);
        
        // 确认和取消按钮
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *cancelBtn = new QPushButton("取消", &dialog);
        cancelBtn->setStyleSheet(
            "QPushButton {"
            "   background-color: #E0E0E0;"
            "   border: none;"
            "   border-radius: 4px;"
            "   padding: 5px 15px;"
            "   font-size: 11pt;"
            "}"
            "QPushButton:hover { background-color: #BDBDBD; }"
            "QPushButton:pressed { background-color: #9E9E9E; }"
        );
        
        QPushButton *confirmBtn = new QPushButton("确定", &dialog);
        confirmBtn->setStyleSheet(
            "QPushButton {"
            "   background-color: #FF9800;"
            "   color: white;"
            "   border: none;"
            "   border-radius: 4px;"
            "   padding: 5px 15px;"
            "   font-size: 11pt;"
            "}"
            "QPushButton:hover { background-color: #F57C00; }"
            "QPushButton:pressed { background-color: #E65100; }"
        );
        
        buttonLayout->addWidget(cancelBtn);
        buttonLayout->addWidget(confirmBtn);
        
        mainLayout->addLayout(buttonLayout);
        
        // 函数：更新预设按钮状态
        auto updatePresetButtonStyles = [&]() {
            // 获取当前自定义时间（总分钟数）
            int totalMinutes = hoursSpinBox->value() * 60 + minutesSpinBox->value();
            
            // 只有在秒数为0的情况下才匹配预设按钮
            if (secondsSpinBox->value() == 0) {
                // 检查每个预设按钮是否匹配当前时间
                for (int i = 0; i < presetButtons.size(); ++i) {
                    if (presetTimes[i] == totalMinutes) {
                        presetButtons[i]->setStyleSheet(selectedButtonStyle);
                    } else {
                        presetButtons[i]->setStyleSheet(normalButtonStyle);
                    }
                }
            } else {
                // 如果秒数不为0，重置所有按钮样式
                for (auto btn : presetButtons) {
                    btn->setStyleSheet(normalButtonStyle);
                }
            }
        };
        
        // 连接自定义时间输入框的信号
        connect(hoursSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [&]() {
            updatePresetButtonStyles();
        });
        
        connect(minutesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [&]() {
            updatePresetButtonStyles();
        });
        
        connect(secondsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [&]() {
            updatePresetButtonStyles();
        });
        
        // 连接预设按钮的点击信号
        for (int i = 0; i < presetButtons.size(); ++i) {
            connect(presetButtons[i], &QPushButton::clicked, [=, &hoursSpinBox, &minutesSpinBox, &secondsSpinBox, &updatePresetButtonStyles]() {
                // 设置自定义时间为预设时间
                hoursSpinBox->setValue(0);  // 预设时间都是分钟
                minutesSpinBox->setValue(presetTimes[i]);
                secondsSpinBox->setValue(0);
                
                // 更新按钮样式
                updatePresetButtonStyles();
            });
        }
        
        // 初始化按钮状态
        updatePresetButtonStyles();
        
        // 连接确认和取消按钮信号
        connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
        connect(confirmBtn, &QPushButton::clicked, [&dialog, hoursSpinBox, minutesSpinBox, secondsSpinBox]() {
            dialog.setProperty("hours", hoursSpinBox->value());
            dialog.setProperty("minutes", minutesSpinBox->value());
            dialog.setProperty("seconds", secondsSpinBox->value());
            dialog.accept();
        });
        
        if (dialog.exec() == QDialog::Accepted) {
            int hours = dialog.property("hours").toInt();
            int minutes = dialog.property("minutes").toInt();
            int seconds = dialog.property("seconds").toInt();
            
            // 计算总秒数
            int totalSeconds = (hours * 3600) + (minutes * 60) + seconds;
            
            if (totalSeconds > 0) {
                initialCountdownSeconds = totalSeconds;
                countdownSeconds = initialCountdownSeconds;
                
                // 更新状态和UI
                isCountdownRunning = true;
                
                // 安全更新菜单项状态
                if (pauseCountdownAction) {
                    pauseCountdownAction->setEnabled(true);
                }
                if (stopCountdownAction) {
                    stopCountdownAction->setEnabled(true);
                }
                
                // 无需禁用开始按钮，让其始终可用
                // startCountdownAction->setEnabled(false);
                
                // 启动倒计时定时器
                if (countdownTimer) {
                    countdownTimer->start(1000);
                }
                
                // 刷新左键菜单
                if (leftClickMenu) {
                    leftClickMenu->clear();
                    createLeftClickMenu();
                }
                
                // 如果已经有一个浮动时钟，就更新它的显示
                if (floatingClock) {
                    floatingClock->showCountdown(countdownSeconds);
                } else if (isShowingClock) {
                    // 如果没有浮动时钟但设置为显示时钟，则创建一个
                    updateFloatingClock();
                }
                
                // 更新托盘图标提示
                updateTrayIconToolTip();
            } else {
                // 显示错误消息
                QMessageBox::information(nullptr, "倒计时", "请输入有效的倒计时时间（大于0秒）");
            }
        } else {
            // 用户取消，确保按钮状态正确
            if (startCountdownAction) {
                startCountdownAction->setEnabled(true);
            }
        }
    } catch (const std::exception &e) {
        qWarning("启动倒计时时出错: %s", e.what());
        // 尝试恢复到安全状态
        countdownSeconds = 0;
        initialCountdownSeconds = 0;
        isCountdownRunning = false;
        if (countdownTimer && countdownTimer->isActive()) {
            countdownTimer->stop();
        }
        
        // 重置菜单状态
        if (pauseCountdownAction) {
            pauseCountdownAction->setEnabled(false);
        }
        if (stopCountdownAction) {
            stopCountdownAction->setEnabled(false);
        }
        if (startCountdownAction) {
            startCountdownAction->setEnabled(true);
        }
        
        // 更新UI
        if (leftClickMenu) {
            leftClickMenu->clear();
            createLeftClickMenu();
        }
        updateTrayIconToolTip();
        
        // 通知用户
        QMessageBox::warning(nullptr, "错误", QString("启动倒计时失败: %1").arg(e.what()));
    } catch (...) {
        qWarning("启动倒计时时发生未知错误");
        // 同样的恢复代码
        countdownSeconds = 0;
        initialCountdownSeconds = 0;
        isCountdownRunning = false;
        if (countdownTimer && countdownTimer->isActive()) {
            countdownTimer->stop();
        }
        
        // 重置菜单状态
        if (pauseCountdownAction) {
            pauseCountdownAction->setEnabled(false);
        }
        if (stopCountdownAction) {
            stopCountdownAction->setEnabled(false);
        }
        if (startCountdownAction) {
            startCountdownAction->setEnabled(true);
        }
        
        // 更新UI
        if (leftClickMenu) {
            leftClickMenu->clear();
            createLeftClickMenu();
        }
        updateTrayIconToolTip();
        
        // 通知用户
        QMessageBox::warning(nullptr, "错误", "启动倒计时失败，发生未知错误");
    }
}

void SystemTrayIcon::pauseCountdown()
{
    if (isCountdownRunning) {
        // 暂停定时器
        countdownTimer->stop();
        isCountdownRunning = false;
        
        // 更新状态和UI
        pauseCountdownAction->setEnabled(false);
        startCountdownAction->setEnabled(true);
        
        // 更新悬浮时钟显示
        if (floatingClock && floatingClock->isVisible() && floatingClock->isCountdownMode()) {
            try {
                floatingClock->updateCountdownDisplay(countdownSeconds);
            } catch (const std::exception &e) {
                qWarning("更新倒计时显示失败: %s", e.what());
                // 出现异常时清理指针
                if (floatingClock) {
                    floatingClock->blockSignals(true);
                    disconnect(floatingClock, nullptr, this, nullptr);
                    FloatingClock* tempClock = floatingClock;
                    floatingClock = nullptr;
                    tempClock->deleteLater();
                }
            }
        }
        
        // 更新托盘图标提示
        updateTrayIconToolTip();
        
        // 通知用户
        showMessage("倒计时暂停", "倒计时已暂停", QSystemTrayIcon::Information, 3000);
    }
}

void SystemTrayIcon::stopCountdown()
{
    // 停止并重置定时器
    countdownTimer->stop();
    isCountdownRunning = false;
    countdownSeconds = 0;
    initialCountdownSeconds = 0;
    
    // 更新状态和UI
    // 无论何时，startCountdownAction都应该是可用的
    startCountdownAction->setEnabled(true);
    pauseCountdownAction->setEnabled(false);
    stopCountdownAction->setEnabled(false);
    
    // 停止悬浮时钟显示倒计时
    if (floatingClock && floatingClock->isCountdownMode()) {
        try {
            floatingClock->stopCountdown();
        } catch (const std::exception &e) {
            qWarning("停止倒计时显示失败: %s", e.what());
            // 出现异常时清理指针
            if (floatingClock) {
                floatingClock->blockSignals(true);
                disconnect(floatingClock, nullptr, this, nullptr);
                FloatingClock* tempClock = floatingClock;
                floatingClock = nullptr;
                tempClock->deleteLater();
            }
        }
    }
    
    // 恢复正常提示
    setToolTip("Dogtime");
    
    // 通知用户
    showMessage("倒计时停止", "倒计时已停止", QSystemTrayIcon::Information, 3000);
}

void SystemTrayIcon::updateCountdown()
{
    // 倒计时减少
    countdownSeconds--;
    
    // 更新悬浮时钟显示
    if (floatingClock && floatingClock->isVisible() && floatingClock->isCountdownMode()) {
        try {
            floatingClock->updateCountdownDisplay(countdownSeconds);
        } catch (const std::exception &e) {
            qWarning("更新倒计时显示失败: %s", e.what());
            // 出现异常时清理指针
            if (floatingClock) {
                floatingClock->blockSignals(true);
                disconnect(floatingClock, nullptr, this, nullptr);
                FloatingClock* tempClock = floatingClock;
                floatingClock = nullptr;
                tempClock->deleteLater();
            }
        }
    }
    
    // 更新托盘图标提示
    updateTrayIconToolTip();
    
    // 检查是否结束
    if (countdownSeconds <= 0) {
        countdownTimer->stop();
        isCountdownRunning = false;
        
        // 重置UI状态
        startCountdownAction->setEnabled(true);
        pauseCountdownAction->setEnabled(false);
        stopCountdownAction->setEnabled(false);
        
        // 恢复正常提示
        setToolTip("Dogtime");
        
        // 触发倒计时结束信号
        onCountdownFinished();
    }
}

void SystemTrayIcon::onCountdownFinished()
{
    // 恢复悬浮时钟正常显示
    if (floatingClock && floatingClock->isCountdownMode()) {
        try {
            floatingClock->stopCountdown();
        } catch (const std::exception &e) {
            qWarning("停止倒计时显示失败: %s", e.what());
            // 出现异常时清理指针
            if (floatingClock) {
                floatingClock->blockSignals(true);
                disconnect(floatingClock, nullptr, this, nullptr);
                FloatingClock* tempClock = floatingClock;
                floatingClock = nullptr;
                tempClock->deleteLater();
            }
        }
    }
    
    // 显示自定义结束提示窗口
    showCountdownNotification();
}

void SystemTrayIcon::showCountdownNotification()
{
    // 如果通知窗口不存在，则创建它
    if (!notification) {
        notification = new CountdownNotification();
        connect(notification, &CountdownNotification::closed, notification, &CountdownNotification::deleteLater);
        connect(notification, &CountdownNotification::closed, [this]() {
            notification = nullptr;
        });
    }
    
    notification->showNotification("番茄时钟", "时间到了！休息一下吧~");
}

void SystemTrayIcon::updateTrayIconToolTip()
{
    if (isCountdownRunning || (countdownSeconds > 0 && !isCountdownRunning)) {
        // 计算剩余时分秒
        int hours = countdownSeconds / 3600;
        int minutes = (countdownSeconds % 3600) / 60;
        int seconds = countdownSeconds % 60;
        
        // 格式化为 HH:MM:SS 或 MM:SS
        QString timeString;
        if (hours > 0) {
            timeString = QString("%1:%2:%3")
                            .arg(hours, 2, 10, QChar('0'))
                            .arg(minutes, 2, 10, QChar('0'))
                            .arg(seconds, 2, 10, QChar('0'));
        } else {
            timeString = QString("%1:%2")
                            .arg(minutes, 2, 10, QChar('0'))
                            .arg(seconds, 2, 10, QChar('0'));
        }
        
        // 设置提示文本
        QString statusText = isCountdownRunning ? "进行中" : "已暂停";
        setToolTip(QString("Dogtime - 倒计时 %1 (%2)").arg(timeString).arg(statusText));
    } else {
        setToolTip("Dogtime");
    }
}

void SystemTrayIcon::showMainWindow()
{
    if (mainWindow->isVisible()) {
        mainWindow->hide();
    } else {
        mainWindow->show();
        mainWindow->activateWindow();
    }
}

void SystemTrayIcon::loadSettings()
{
    QSettings settings("DogSoft", "Dogtime");
    
    // 加载时钟显示设置
    isShowingClock = settings.value("floatingclock/visible", true).toBool();
    
    // 其他时钟配置
    bool use24HourFormat = settings.value("floatingclock/use24hour", true).toBool();
    bool showSeconds = settings.value("floatingclock/showseconds", false).toBool();
    
    // 创建动作后更新这些值
    QTimer::singleShot(200, this, [this, use24HourFormat, showSeconds]() {
        if (use24HourFormatAction) {
            use24HourFormatAction->setChecked(use24HourFormat);
        }
        if (showSecondsAction) {
            showSecondsAction->setChecked(showSeconds);
        }
        if (showCurrentTimeAction) {
            showCurrentTimeAction->setChecked(isShowingClock);
        }
        updateFloatingClock();
    });
}

void SystemTrayIcon::saveSettings()
{
    QSettings settings("DogSoft", "Dogtime");
    
    // 保存时钟显示设置
    settings.setValue("floatingclock/visible", isShowingClock);
    
    // 保存时钟格式设置
    if (use24HourFormatAction) {
        settings.setValue("floatingclock/use24hour", use24HourFormatAction->isChecked());
    }
    
    if (showSecondsAction) {
        settings.setValue("floatingclock/showseconds", showSecondsAction->isChecked());
    }
    
    settings.sync();
}

void SystemTrayIcon::saveCountdownState()
{
    QSettings settings("DogSoft", "Dogtime");
    
    // 保存倒计时状态
    settings.setValue("countdown/running", isCountdownRunning);
    settings.setValue("countdown/current", countdownSeconds);
    settings.setValue("countdown/initial", initialCountdownSeconds);
    
    // 如果正在运行，记录暂停时间
    if (isCountdownRunning) {
        settings.setValue("countdown/paused_at", QDateTime::currentDateTime());
    } else {
        settings.remove("countdown/paused_at");
    }
    
    settings.sync();
}

void SystemTrayIcon::loadCountdownState()
{
    QSettings settings("DogSoft", "Dogtime");
    
    // 加载倒计时状态
    bool wasRunning = settings.value("countdown/running", false).toBool();
    countdownSeconds = settings.value("countdown/current", 0).toInt();
    initialCountdownSeconds = settings.value("countdown/initial", 0).toInt();
    
    // 如果倒计时之前在运行，考虑时间差
    if (wasRunning && countdownSeconds > 0) {
        QDateTime pausedAt = settings.value("countdown/paused_at").toDateTime();
        
        if (pausedAt.isValid()) {
            // 计算从暂停到现在经过的秒数
            qint64 elapsedSeconds = pausedAt.secsTo(QDateTime::currentDateTime());
            
            // 减去经过的时间
            if (elapsedSeconds > 0) {
                countdownSeconds -= elapsedSeconds;
                
                // 确保计数不会变成负数
                if (countdownSeconds < 0) {
                    countdownSeconds = 0;
                }
            }
        }
        
        // 如果仍有剩余时间，继续倒计时
        if (countdownSeconds > 0) {
            isCountdownRunning = true;
            pauseCountdownAction->setEnabled(true);
            stopCountdownAction->setEnabled(true);
            startCountdownAction->setEnabled(false);
            
            // 更新托盘图标提示
            updateTrayIconToolTip();
            
            // 启动定时器
            countdownTimer->start(1000);
            
            // 显示通知
            showMessage("倒计时恢复", "已恢复上次的倒计时", QSystemTrayIcon::Information, 3000);
        } else {
            // 如果没有剩余时间，重置倒计时
            isCountdownRunning = false;
            countdownSeconds = 0;
            initialCountdownSeconds = 0;
        }
    } else {
        // 不是运行状态或没有倒计时
        isCountdownRunning = false;
    }
    
    // 更新UI状态
    if (pauseCountdownAction) {
        pauseCountdownAction->setEnabled(isCountdownRunning);
    }
    if (startCountdownAction) {
        // 这里修复了startCountdownAction的状态
        // 无论是否有倒计时，都允许开始新的倒计时
        startCountdownAction->setEnabled(true);
    }
    if (stopCountdownAction) {
        stopCountdownAction->setEnabled(isCountdownRunning || countdownSeconds > 0);
    }
}

void SystemTrayIcon::createDefaultSettings(QSettings &settings)
{
    if (!settings.contains("floatingclock/visible")) {
        settings.setValue("floatingclock/visible", true);
    }
    if (!settings.contains("floatingclock/use24hour")) {
        settings.setValue("floatingclock/use24hour", true);
    }
    if (!settings.contains("floatingclock/showseconds")) {
        settings.setValue("floatingclock/showseconds", false);
    }
}

// 专注模式相关的方法实现
void SystemTrayIcon::startFocusMode()
{
    // 该方法保留用于兼容性，现在直接调用弱专注模式
    startWeakFocusMode();
}

void SystemTrayIcon::startWeakFocusMode()
{
    // 如果专注模式窗口不存在，则创建它
    if (!focusMode) {
        focusMode = new FocusMode();
        
        // 连接关闭信号
        connect(focusMode, &FocusMode::closed, this, &SystemTrayIcon::onFocusModeClose);
    }
    
    // 设置为弱强制模式
    focusMode->setFocusMode(FocusMode::WeakFocus);
    
    // 显示专注模式窗口
    focusMode->showFullScreen();
    
    // 通知用户
    showMessage("弱专注模式已启动", "进入全屏专注模式，按ESC键或点击退出按钮可随时退出", QSystemTrayIcon::Information, 3000);
}

void SystemTrayIcon::startStrongFocusMode()
{
    try {
        // 创建一个对话框来选择专注时间
        QDialog dialog;
        dialog.setWindowTitle("设置专注时间");
        dialog.setFixedWidth(320);
        
        QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
        
        // 添加说明标签
        QLabel *label = new QLabel("选择或输入专注时间:", &dialog);
        mainLayout->addWidget(label);
        
        // 添加常用专注时间选项
        QHBoxLayout *presetLayout = new QHBoxLayout();
        QVector<int> presetTimes = {15, 25, 30, 45, 60, 90};
        QVector<QPushButton*> presetButtons;
        
        // 定义选中和未选中的样式
        QString normalButtonStyle = 
            "QPushButton {"
            "   background-color: #4CAF50;"
            "   color: white;"
            "   border: none;"
            "   border-radius: 4px;"
            "   padding: 5px 10px;"
            "   font-size: 11pt;"
            "}"
            "QPushButton:hover { background-color: #388E3C; }"
            "QPushButton:pressed { background-color: #1B5E20; }";
            
        QString selectedButtonStyle = 
            "QPushButton {"
            "   background-color: #1B5E20;"
            "   color: white;"
            "   border: none;"
            "   border-radius: 4px;"
            "   padding: 5px 10px;"
            "   font-size: 11pt;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #1B5E20; }"
            "QPushButton:pressed { background-color: #1B5E20; }";
        
        for (int time : presetTimes) {
            QPushButton *presetBtn = new QPushButton(QString::number(time), &dialog);
            presetBtn->setFixedHeight(40);
            presetBtn->setStyleSheet(normalButtonStyle);
            presetLayout->addWidget(presetBtn);
            presetButtons.append(presetBtn);
        }
        mainLayout->addLayout(presetLayout);
        
        // 添加自定义时分秒输入
        QGroupBox *customGroup = new QGroupBox("自定义时间", &dialog);
        QHBoxLayout *customLayout = new QHBoxLayout(customGroup);
        
        // 小时输入框
        QSpinBox *hoursSpinBox = new QSpinBox(&dialog);
        hoursSpinBox->setRange(0, 23);
        hoursSpinBox->setValue(0);
        hoursSpinBox->setSuffix(" 时");
        hoursSpinBox->setFixedHeight(30);
        hoursSpinBox->setStyleSheet("font-size: 11pt; padding: 2px 5px;");
        customLayout->addWidget(hoursSpinBox);
        
        // 分钟输入框
        QSpinBox *minutesSpinBox = new QSpinBox(&dialog);
        minutesSpinBox->setRange(0, 59);
        minutesSpinBox->setValue(25); // 默认25分钟
        minutesSpinBox->setSuffix(" 分");
        minutesSpinBox->setFixedHeight(30);
        minutesSpinBox->setStyleSheet("font-size: 11pt; padding: 2px 5px;");
        customLayout->addWidget(minutesSpinBox);
        
        // 秒钟输入框
        QSpinBox *secondsSpinBox = new QSpinBox(&dialog);
        secondsSpinBox->setRange(0, 59);
        secondsSpinBox->setValue(0);
        secondsSpinBox->setSuffix(" 秒");
        secondsSpinBox->setFixedHeight(30);
        secondsSpinBox->setStyleSheet("font-size: 11pt; padding: 2px 5px;");
        customLayout->addWidget(secondsSpinBox);
        
        mainLayout->addWidget(customGroup);
        
        // 添加提示说明
        QLabel *warningLabel = new QLabel("注意：在强专注模式下，您将无法在设定的时间结束前退出专注模式。", &dialog);
        warningLabel->setStyleSheet("color: #F44336; font-weight: bold;");
        warningLabel->setWordWrap(true);
        mainLayout->addWidget(warningLabel);
        
        // 确认和取消按钮
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *cancelBtn = new QPushButton("取消", &dialog);
        cancelBtn->setStyleSheet(
            "QPushButton {"
            "   background-color: #E0E0E0;"
            "   border: none;"
            "   border-radius: 4px;"
            "   padding: 5px 15px;"
            "   font-size: 11pt;"
            "}"
            "QPushButton:hover { background-color: #BDBDBD; }"
            "QPushButton:pressed { background-color: #9E9E9E; }"
        );
        
        QPushButton *confirmBtn = new QPushButton("确定", &dialog);
        confirmBtn->setStyleSheet(
            "QPushButton {"
            "   background-color: #4CAF50;"
            "   color: white;"
            "   border: none;"
            "   border-radius: 4px;"
            "   padding: 5px 15px;"
            "   font-size: 11pt;"
            "}"
            "QPushButton:hover { background-color: #388E3C; }"
            "QPushButton:pressed { background-color: #1B5E20; }"
        );
        
        buttonLayout->addWidget(cancelBtn);
        buttonLayout->addWidget(confirmBtn);
        
        mainLayout->addLayout(buttonLayout);
        
        // 函数：更新预设按钮状态
        auto updatePresetButtonStyles = [&]() {
            // 获取当前自定义时间（总分钟数）
            int totalMinutes = hoursSpinBox->value() * 60 + minutesSpinBox->value();
            
            // 只有在秒数为0的情况下才匹配预设按钮
            if (secondsSpinBox->value() == 0) {
                // 检查每个预设按钮是否匹配当前时间
                for (int i = 0; i < presetButtons.size(); ++i) {
                    if (presetTimes[i] == totalMinutes) {
                        presetButtons[i]->setStyleSheet(selectedButtonStyle);
                    } else {
                        presetButtons[i]->setStyleSheet(normalButtonStyle);
                    }
                }
            } else {
                // 如果秒数不为0，重置所有按钮样式
                for (auto btn : presetButtons) {
                    btn->setStyleSheet(normalButtonStyle);
                }
            }
        };
        
        // 连接自定义时间输入框的信号
        connect(hoursSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [&]() {
            updatePresetButtonStyles();
        });
        
        connect(minutesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [&]() {
            updatePresetButtonStyles();
        });
        
        connect(secondsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [&]() {
            updatePresetButtonStyles();
        });
        
        // 连接预设按钮的点击信号
        for (int i = 0; i < presetButtons.size(); ++i) {
            connect(presetButtons[i], &QPushButton::clicked, [=, &hoursSpinBox, &minutesSpinBox, &secondsSpinBox, &updatePresetButtonStyles]() {
                // 设置自定义时间为预设时间
                hoursSpinBox->setValue(0);  // 预设时间都是分钟
                minutesSpinBox->setValue(presetTimes[i]);
                secondsSpinBox->setValue(0);
                
                // 更新按钮样式
                updatePresetButtonStyles();
            });
        }
        
        // 初始化按钮状态
        updatePresetButtonStyles();
        
        // 连接确认和取消按钮信号
        connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
        connect(confirmBtn, &QPushButton::clicked, [&dialog, hoursSpinBox, minutesSpinBox, secondsSpinBox]() {
            dialog.setProperty("hours", hoursSpinBox->value());
            dialog.setProperty("minutes", minutesSpinBox->value());
            dialog.setProperty("seconds", secondsSpinBox->value());
            dialog.accept();
        });
        
        if (dialog.exec() == QDialog::Accepted) {
            int hours = dialog.property("hours").toInt();
            int minutes = dialog.property("minutes").toInt();
            int seconds = dialog.property("seconds").toInt();
            
            // 计算总分钟数
            int totalMinutes = hours * 60 + minutes + (seconds > 0 ? 1 : 0); // 向上取整到分钟
            
            if (totalMinutes > 0) {
                // 如果专注模式窗口不存在，则创建它
                if (!focusMode) {
                    focusMode = new FocusMode();
                    
                    // 连接关闭信号
                    connect(focusMode, &FocusMode::closed, this, &SystemTrayIcon::onFocusModeClose);
                }
                
                // 设置为强强制模式
                focusMode->setFocusMode(FocusMode::StrongFocus, totalMinutes);
                
                // 显示专注模式窗口
                focusMode->showFullScreen();
                
                // 通知用户
                showMessage("强专注模式已启动", QString("进入强制专注模式，将在%1分钟后才能退出").arg(totalMinutes), QSystemTrayIcon::Information, 3000);
            } else {
                // 显示错误消息
                QMessageBox::information(nullptr, "专注模式", "请输入有效的专注时间（大于0分钟）");
            }
        }
    } catch (const std::exception &e) {
        qWarning("启动强专注模式时出错: %s", e.what());
        QMessageBox::warning(nullptr, "错误", QString("启动强专注模式失败: %1").arg(e.what()));
    } catch (...) {
        qWarning("启动强专注模式时发生未知错误");
        QMessageBox::warning(nullptr, "错误", "启动强专注模式失败，发生未知错误");
    }
}

void SystemTrayIcon::onFocusModeClose()
{
    // 如果专注模式窗口存在，则清理资源
    if (focusMode) {
        focusMode->blockSignals(true);
        disconnect(focusMode, nullptr, this, nullptr);
        
        // 保存临时指针
        FocusMode* tempMode = focusMode;
        focusMode = nullptr;
        
        // 安全删除
        tempMode->deleteLater();
    }
    
    // 通知用户
    showMessage("专注模式已退出", "已返回到正常模式", QSystemTrayIcon::Information, 2000);
} 