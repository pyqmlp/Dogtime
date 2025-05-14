#include "floatingclock.h"
#include <QVBoxLayout>
#include <QDateTime>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QStyleOption>
#include <QStyle>
#include <QGraphicsDropShadowEffect>
#include <QEnterEvent>
#include <QApplication>
#include <algorithm>
#include <QPainterPath>
#include <QSequentialAnimationGroup>
#include <QSettings>
#include <QMoveEvent>
#include <QCloseEvent>

FloatingClock::FloatingClock(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
    , timeLabel(nullptr)
    , timer(new QTimer(this))
    , controlBar(nullptr)
    , lockButton(nullptr)
    , settingsButton(nullptr)
    , closeButton(nullptr)
    , use24HourFormat(true)
    , showSeconds(true)
    , locked(false)
    , mouseOver(false)
    , countdownMode(false)
    , isDragging(false)
    , countdownSeconds(0)
    , dragPosition()
    , fadeAnimation(nullptr)
    , bounceAnimation(nullptr)
    , positionLoaded(false)
    , isClosing(false)
{
    setupUI();
    setupControlBar();
    setupAnimations();
    
    // 使用高精度定时器
    timer->setTimerType(Qt::PreciseTimer);
    
    // 设置定时器为精确时间更新
    connect(timer, &QTimer::timeout, this, &FloatingClock::updateTime);
    
    // 立即更新当前时间
    updateTime();
    
    // 计算到下一秒的毫秒数
    QTime currentTime = QTime::currentTime();
    int msToNextSecond = 1000 - currentTime.msec();
    
    // 首次启动定时器，对准整秒
    timer->singleShot(msToNextSecond, this, [this]() {
        updateTime(); // 整秒时更新时间
        timer->start(1000); // 然后每秒更新一次
    });
}

FloatingClock::~FloatingClock()
{
    // 安全销毁前的准备工作，但不发送信号
    prepareForDestruction(false);
    
    // 对子部件资源的清理由Qt自动处理
}

void FloatingClock::setupUI()
{
    // 设置窗口无边框，保持在顶部
    setAttribute(Qt::WA_TranslucentBackground);
    
    // 设置窗口大小，调整为更宽的矩形形状，类似歌词样式
    resize(280, 80);
    
    // 从设置中加载位置
    loadPosition();
    
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 30, 10, 10); // 顶部留出空间给控制栏
    mainLayout->setSpacing(0);
    
    // 创建主时间标签，居中对齐
    timeLabel = new QLabel(this);
    timeLabel->setAlignment(Qt::AlignCenter); // 水平和垂直居中
    // 添加文字阴影使其在任何背景下都清晰可见
    timeLabel->setStyleSheet("QLabel { color: #0066CC; font-size: 20pt; font-weight: bold; text-shadow: 1px 1px 2px rgba(0, 0, 0, 0.7); }");
    
    // 将时间标签添加到布局
    mainLayout->addWidget(timeLabel);
    mainLayout->addStretch();
}

void FloatingClock::setupControlBar()
{
    // 创建控制栏
    controlBar = new QWidget(this);
    controlBar->setObjectName("controlBar");
    controlBar->setStyleSheet("QWidget#controlBar { background-color: transparent; }");
    
    // 设置控制栏布局
    QHBoxLayout *controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setContentsMargins(5, 3, 5, 3);
    controlLayout->setSpacing(8);
    
    // 添加弹性空间(左侧)，使按钮居中
    controlLayout->addStretch();
    
    // 创建按钮
    lockButton = new QPushButton(controlBar);
    settingsButton = new QPushButton(controlBar);
    closeButton = new QPushButton(controlBar);
    
    // 设置按钮样式
    QString buttonStyle = "QPushButton { background: transparent; border: none; width: 18px; height: 18px; color: white; } "
                         "QPushButton:hover { background-color: rgba(255, 255, 255, 0.3); border-radius: 3px; }";
    
    lockButton->setStyleSheet(buttonStyle);
    settingsButton->setStyleSheet(buttonStyle);
    closeButton->setStyleSheet(buttonStyle);
    
    // 设置图标
    lockButton->setIcon(QIcon(":/icons/floatingclock/lock.svg"));
    settingsButton->setIcon(QIcon(":/icons/floatingclock/settings.svg"));
    closeButton->setIcon(QIcon(":/icons/floatingclock/close.svg"));
    
    // 设置图标大小
    QSize iconSize(16, 16);
    lockButton->setIconSize(iconSize);
    settingsButton->setIconSize(iconSize);
    closeButton->setIconSize(iconSize);
    
    // 设置按钮大小
    lockButton->setFixedSize(22, 22);
    settingsButton->setFixedSize(22, 22);
    closeButton->setFixedSize(22, 22);
    
    // 设置工具提示
    lockButton->setToolTip("锁定悬浮时钟");
    settingsButton->setToolTip("设置");
    closeButton->setToolTip("关闭");
    
    // 添加按钮到布局
    controlLayout->addWidget(lockButton);
    controlLayout->addWidget(settingsButton);
    controlLayout->addWidget(closeButton);
    
    // 添加弹性空间(右侧)，使按钮居中
    controlLayout->addStretch();
    
    // 设置控制栏大小和位置
    controlBar->setFixedHeight(24);
    controlBar->setGeometry(0, 0, width(), 24);
    
    // 连接信号槽
    connect(lockButton, &QPushButton::clicked, this, &FloatingClock::toggleLocked);
    connect(settingsButton, &QPushButton::clicked, this, &FloatingClock::onSettingsClicked);
    connect(closeButton, &QPushButton::clicked, this, &FloatingClock::onCloseClicked);
    
    // 隐藏控制栏，初始状态不显示
    controlBar->hide();
}

void FloatingClock::setupAnimations()
{
    // 设置透明度动画
    fadeAnimation = new QPropertyAnimation(this, "windowOpacity", this);
    fadeAnimation->setDuration(300); // 300毫秒
    fadeAnimation->setStartValue(0.0);
    fadeAnimation->setEndValue(1.0);
    fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);
    
    // 设置弹跳动画
    bounceAnimation = new QPropertyAnimation(this, "geometry", this);
    bounceAnimation->setDuration(500); // 500毫秒
    bounceAnimation->setEasingCurve(QEasingCurve::OutBounce);
    
    // 连接动画完成信号
    connect(fadeAnimation, &QPropertyAnimation::finished, this, &FloatingClock::onShowAnimationFinished);
}

void FloatingClock::show()
{
    // 显示窗口但先设置透明
    setWindowOpacity(0.0);
    QWidget::show();
    
    // 播放显示动画
    playShowAnimation();
}

void FloatingClock::playShowAnimation()
{
    // 如果动画正在运行，先停止
    if (fadeAnimation->state() == QPropertyAnimation::Running) {
        fadeAnimation->stop();
    }
    if (bounceAnimation->state() == QPropertyAnimation::Running) {
        bounceAnimation->stop();
    }
    
    // 设置动画的初始和结束几何形状
    QRect currentGeometry = geometry();
    QRect startGeometry = currentGeometry;
    startGeometry.moveTop(startGeometry.top() - 20); // 从上方20像素开始
    
    bounceAnimation->setStartValue(startGeometry);
    bounceAnimation->setEndValue(currentGeometry);
    
    // 创建一个动画组，先执行弹跳动画，然后同时执行淡入动画
    QSequentialAnimationGroup *animGroup = new QSequentialAnimationGroup(this);
    animGroup->addAnimation(bounceAnimation);
    
    // 设置动画组完成后的清理
    connect(animGroup, &QSequentialAnimationGroup::finished, animGroup, &QObject::deleteLater);
    
    // 开始执行动画序列和淡入动画
    animGroup->start();
    fadeAnimation->start();
}

void FloatingClock::onShowAnimationFinished()
{
    // 确保窗口完全不透明（这里不需要做什么，因为动画已经完成）
}

void FloatingClock::updateTime()
{
    if (countdownMode) {
        // 在倒计时模式下，由SystemTrayIcon会调用updateCountdownDisplay
        return;
    }
    
    // 获取当前系统时间（使用currentDateTime获取最新时间）
    QDateTime now = QDateTime::currentDateTime();
    
    // 根据设置选择格式
    QString format = use24HourFormat ? "HH:mm" : "hh:mm AP";
    
    if (showSeconds) {
        format += ":ss";
    }
    
    // 如果不是整秒，重新计算到下一秒的时间并重设定时器
    // 这可以避免长时间运行导致的漂移
    QTime currentTime = now.time();
    if (timer->isActive() && currentTime.msec() > 20) { // 允许有20ms的误差
        int msToNextSecond = 1000 - currentTime.msec();
        timer->stop();
        timer->singleShot(msToNextSecond, this, [this]() {
            updateTime(); // 整秒时更新时间
            timer->start(1000); // 然后每秒更新一次
        });
    }
    
    // 更新主时间标签
    QString timeStr = now.toString(format);
    if (!use24HourFormat) {
        // 如果是12小时制，将AM/PM转换为小写
        timeStr = timeStr.replace("AM", "am").replace("PM", "pm");
    }
    timeLabel->setText(timeStr);
}

void FloatingClock::setUse24HourFormat(bool use24Hour)
{
    if (use24HourFormat != use24Hour) {
        use24HourFormat = use24Hour;
        updateTime();
    }
}

void FloatingClock::setShowSeconds(bool show)
{
    if (showSeconds != show) {
        showSeconds = show;
        updateTime();
    }
}

void FloatingClock::setLocked(bool lock)
{
    if (locked != lock) {
        locked = lock;
        updateLockButtonIcon();
        updateControlBarVisibility();
        emit lockStateChanged(locked);
    }
}

void FloatingClock::toggleLocked()
{
    setLocked(!locked);
}

void FloatingClock::updateLockButtonIcon()
{
    if (locked) {
        lockButton->setIcon(QIcon(":/icons/floatingclock/unlock.svg"));
        lockButton->setToolTip("解锁悬浮时钟");
    } else {
        lockButton->setIcon(QIcon(":/icons/floatingclock/lock.svg"));
        lockButton->setToolTip("锁定悬浮时钟");
    }
}

void FloatingClock::updateControlBarVisibility()
{
    if (locked) {
        // 锁定状态
        controlBar->hide();
        
        // 锁定状态下只显示解锁按钮
        // 解锁按钮作为主窗口的子部件并置于顶层
        lockButton->setParent(this);
        lockButton->raise();
        
        // 改为白色图标
        lockButton->setStyleSheet("QPushButton { background: transparent; border: none; width: 16px; height: 16px; } "
                               "QPushButton:hover { background-color: rgba(255, 255, 255, 0.3); border-radius: 3px; }");
        
        // 使用白色解锁图标
        QIcon unlockIcon(":/icons/floatingclock/unlock.svg");
        QPixmap pixmap = unlockIcon.pixmap(QSize(16, 16));
        
        // 创建一个白色图标
        QPixmap whitePix = pixmap;
        whitePix.fill(Qt::transparent);
        QPainter painter(&whitePix);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.setPen(Qt::white);
        painter.drawPixmap(0, 0, pixmap);
        
        lockButton->setIcon(QIcon(whitePix));
        lockButton->setIconSize(QSize(16, 16));
        
        // 调整位置，锁定按钮位于窗口中央
        lockButton->setGeometry((width() - lockButton->width()) / 2, 
                              (height() - lockButton->height()) / 2, 
                              lockButton->width(), 
                              lockButton->height());
        
        // 只在鼠标悬浮时显示
        lockButton->setVisible(mouseOver);
    } else {
        // 解锁状态
        
        // 将锁定按钮放回控制栏
        if (lockButton->parent() != controlBar) {
            // 保存锁定按钮的指针
            QPushButton* tempLockButton = lockButton;
            
            // 将锁定按钮从其父部件中移除，但不删除对象
            tempLockButton->setParent(nullptr);
            
            // 重新设置父部件为控制栏
            tempLockButton->setParent(controlBar);
            
            // 重新添加到控制栏布局中
            QHBoxLayout* controlLayout = qobject_cast<QHBoxLayout*>(controlBar->layout());
            if (controlLayout) {
                // 找到锁定按钮在布局中的位置
                int index = 1; // 锁定按钮通常是第一个按钮
                
                // 如果控制布局第一个部件不是伸缩器，需要调整索引
                if (controlLayout->itemAt(0) && 
                    controlLayout->itemAt(0)->widget() != nullptr) {
                    index = 0;
                }
                
                // 插入锁定按钮到布局中
                controlLayout->insertWidget(index, tempLockButton);
            }
            
            // 更新样式和图标
            tempLockButton->setStyleSheet("QPushButton { background: transparent; border: none; width: 18px; height: 18px; color: white; } "
                                  "QPushButton:hover { background-color: rgba(255, 255, 255, 0.3); border-radius: 3px; }");
            
            // 恢复原始图标
            tempLockButton->setIcon(QIcon(":/icons/floatingclock/lock.svg"));
            tempLockButton->setIconSize(QSize(16, 16));
            tempLockButton->show();
        }
        
        // 控制栏位于窗口顶部，宽度与窗口相同
        controlBar->setGeometry(0, 0, width(), 24);
        
        // 只在鼠标悬浮时显示控制栏
        controlBar->setVisible(mouseOver);
    }
}

void FloatingClock::onSettingsClicked()
{
    emit settingsRequested();
}

void FloatingClock::prepareForDestruction(bool sendSignal)
{
    // 避免重入
    static bool isProcessing = false;
    if (isProcessing) return;
    isProcessing = true;
    
    try {
        // 停止所有动画和计时器
        if (timer) {
            if (timer->isActive()) {
                timer->stop();
            }
            timer->disconnect();
        }
        
        if (fadeAnimation) {
            if (fadeAnimation->state() == QPropertyAnimation::Running) {
                fadeAnimation->stop();
            }
            fadeAnimation->disconnect();
        }
        
        if (bounceAnimation) {
            if (bounceAnimation->state() == QPropertyAnimation::Running) {
                bounceAnimation->stop();
            }
            bounceAnimation->disconnect();
        }
        
        // 断开所有信号连接，阻止回调
        blockSignals(true);
        
        // 确保窗口不再获得事件
        setAttribute(Qt::WA_DeleteOnClose, false);
        setEnabled(false);
        
        // 保存位置
        try {
            if (isVisible()) {
                savePosition();
            }
        } catch(...) {
            // 忽略保存位置过程中的错误
        }
        
        // 如果需要发送信号，且没有设置关闭标志
        if (sendSignal && !isClosing) {
            isClosing = true;
            // 使用一次性延迟信号发送，避免在当前调用栈中处理
            QMetaObject::invokeMethod(this, [this]() {
                if (!this) return; // 防止使用已删除的对象
                emit closeRequested();
            }, Qt::QueuedConnection);
        }
    } catch (const std::exception &e) {
        qWarning("准备销毁浮动时钟时出错: %s", e.what());
        // 确保blockSignals被调用，避免后续信号触发崩溃
        blockSignals(true);
    } catch (...) {
        qWarning("准备销毁浮动时钟时出现未知错误");
        // A安全检查，确保信号被阻断
        blockSignals(true);
    }
    
    isProcessing = false;
}

void FloatingClock::onCloseClicked()
{
    // 简单关闭窗口，会触发closeEvent
    close();
}

void FloatingClock::enterEvent(QEnterEvent *event)
{
    mouseOver = true;
    updateControlBarVisibility();
    // 触发重绘以显示/隐藏背景
    update();
    event->accept();
}

void FloatingClock::leaveEvent(QEvent *event)
{
    mouseOver = false;
    updateControlBarVisibility();
    // 触发重绘以显示/隐藏背景
    update();
    event->accept();
}

void FloatingClock::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (!locked) {
            isDragging = true;
            dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        }
        event->accept();
    }
}

void FloatingClock::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && isDragging && !locked) {
        move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
    }
}

void FloatingClock::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        event->accept();
    }
}

void FloatingClock::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 只在解锁状态且鼠标悬浮时显示半透明背景
    if (mouseOver && !locked) {
        // 绘制主窗口背景（灰色半透明）
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(128, 128, 128, 200));
        painter.drawRoundedRect(rect(), 8, 8);
    }
    
    // 绘制子部件
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
}

// 添加窗口大小变化事件处理
void FloatingClock::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // 更新控制栏位置和大小
    if (controlBar && !locked) {
        controlBar->setGeometry(0, 0, width(), 24);
    }
    
    // 锁定状态下，更新锁定按钮位置
    if (locked && lockButton && lockButton->parent() == this) {
        lockButton->setGeometry((width() - lockButton->width()) / 2, 
                              (height() - lockButton->height()) / 2, 
                              lockButton->width(), 
                              lockButton->height());
    }
}

// 倒计时相关功能实现
void FloatingClock::showCountdown(int seconds)
{
    countdownMode = true;
    countdownSeconds = seconds;
    updateCountdownDisplay(seconds);
    
    // 如果不可见，则显示
    if (!isVisible()) {
        show();
    } else {
        // 如果已经可见，播放一个轻微的注意动画
        QRect currentGeometry = geometry();
        QRect startGeometry = currentGeometry;
        
        // 轻微缩放窗口
        startGeometry.adjust(5, 5, -5, -5); // 向内收缩5个像素
        
        // 设置动画
        QPropertyAnimation *pulseAnimation = new QPropertyAnimation(this, "geometry", this);
        pulseAnimation->setDuration(300);
        pulseAnimation->setStartValue(startGeometry);
        pulseAnimation->setEndValue(currentGeometry);
        pulseAnimation->setEasingCurve(QEasingCurve::OutElastic);
        
        // 连接动画完成信号以删除动画对象
        connect(pulseAnimation, &QPropertyAnimation::finished, pulseAnimation, &QObject::deleteLater);
        
        // 开始动画
        pulseAnimation->start();
    }
}

void FloatingClock::stopCountdown()
{
    countdownMode = false;
    countdownSeconds = 0;
    
    // 恢复显示当前时间
    updateTime();
}

void FloatingClock::updateCountdownDisplay(int seconds)
{
    if (!countdownMode) {
        return;
    }
    
    countdownSeconds = seconds;
    if (seconds <= 0) {
        emit countdownFinished();
        stopCountdown();
        return;
    }
    
    // 格式化并显示倒计时时间
    QString timeStr = formatTime(seconds);
    timeLabel->setText(timeStr);
}

QString FloatingClock::formatTime(int seconds) const
{
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    QString timeStr;
    
    if (hours > 0) {
        // 时分秒格式 HH:MM:SS
        timeStr = QString("%1:%2:%3")
                        .arg(hours, 2, 10, QChar('0'))
                        .arg(minutes, 2, 10, QChar('0'))
                        .arg(secs, 2, 10, QChar('0'));
    } else {
        // 分秒格式 MM:SS
        timeStr = QString("%1:%2")
                        .arg(minutes, 2, 10, QChar('0'))
                        .arg(secs, 2, 10, QChar('0'));
    }
    
    return timeStr;
}

void FloatingClock::savePosition()
{
    if (isVisible()) {
        QSettings settings("DogSoft", "Dogtime");
        settings.setValue("floatingclock/position", pos());
        settings.sync();
    }
}

void FloatingClock::loadPosition()
{
    if (!positionLoaded) {
        QSettings settings("DogSoft", "Dogtime");
        QPoint savedPos = settings.value("floatingclock/position").toPoint();
        
        if (!savedPos.isNull()) {
            // 检查保存的位置是否在任何可见屏幕上
            bool onScreen = false;
            const QList<QScreen *> screens = QGuiApplication::screens();
            
            for (const QScreen *screen : screens) {
                if (screen->geometry().contains(savedPos)) {
                    onScreen = true;
                    break;
                }
            }
            
            if (onScreen) {
                move(savedPos);
                positionLoaded = true;
                return;
            }
        }
        
        // 如果没有保存位置或位置不在屏幕上，则使用默认位置
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->geometry();
        move(screenGeometry.width() - width() - 20, 20);
        positionLoaded = true;
    }
}

void FloatingClock::hide()
{
    // 如果已经隐藏，则不执行
    if (!isVisible()) return;
    
    try {
        // 停止任何正在运行的动画
        if (fadeAnimation && fadeAnimation->state() == QPropertyAnimation::Running) {
            fadeAnimation->stop();
        }
        
        if (bounceAnimation && bounceAnimation->state() == QPropertyAnimation::Running) {
            bounceAnimation->stop();
        }
        
        // 保存当前位置
        savePosition();
        
        // 调用基类的hide方法
        QWidget::hide();
        
        // 如果是由关闭按钮触发的，发送关闭信号
        if (isClosing) {
            // 确保只发送一次，且在下一个事件循环中发送
            QMetaObject::invokeMethod(this, [this]() {
                if (!this) return; // 防止使用已删除的对象
                if (!isClosing) return; // 检查状态是否改变
                
                // 发出信号前先断开所有连接
                disconnect(this, &FloatingClock::closeRequested, nullptr, nullptr);
                emit closeRequested();
            }, Qt::QueuedConnection);
        }
    } catch (const std::exception& e) {
        qWarning("隐藏窗口时出错: %s", e.what());
        // 即使出错也要尝试正常隐藏
        blockSignals(true); // 阻止任何可能的信号
        QWidget::hide();
    } catch (...) {
        qWarning("隐藏窗口时出现未知错误");
        // 即使出错也要尝试正常隐藏
        blockSignals(true); // 阻止任何可能的信号
        QWidget::hide();
    }
}

void FloatingClock::close()
{
    if (isClosing) {
        QWidget::close();
        return;
    }
    
    // 设置关闭标志
    isClosing = true;
    
    // 保存位置
    if (isVisible()) {
        try {
            savePosition();
        } catch (...) {
            // 忽略保存位置过程中的错误
        }
    }
    
    // 停止定时器和动画
    if (timer && timer->isActive()) {
        timer->stop();
    }
    
    // 发送信号
    emit closed();
    
    // 隐藏窗口
    QWidget::hide();
    
    // 调用基类close
    QWidget::close();
}

void FloatingClock::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    // 不在拖动时保存位置，避免频繁写入
    if (!isDragging && isVisible()) {
        // 使用延迟保存，减少频繁写入
        QTimer::singleShot(500, this, &FloatingClock::savePosition);
    }
}

void FloatingClock::closeEvent(QCloseEvent *event)
{
    // 简单接受关闭事件
    event->accept();
    
    // 如果是通过X按钮关闭的，确保发送关闭信号
    if (!isClosing) {
        isClosing = true;
        emit closed();
    }
} 