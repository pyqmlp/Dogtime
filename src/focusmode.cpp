#include "focusmode.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QFont>
#include <QTime>
#include <QDate>
#include <cstdlib>
#include <ctime>
#include <QApplication>
#include <QPainterPath>
#include <QLinearGradient>
#include <QMessageBox>

FocusMode::FocusMode(QWidget *parent)
    : QWidget(parent)
    , timeLabel(nullptr)
    , dateLabel(nullptr)
    , motivationLabel(nullptr)
    , exitButton(nullptr)
    , focusCountdownLabel(nullptr)
    , focusProgressBar(nullptr)
    , timer(new QTimer(this))
    , focusCountdownTimer(new QTimer(this))
    , fadeInAnimation(nullptr)
    , modeType(WeakFocus)
    , focusDurationSeconds(0)
    , remainingFocusSeconds(0)
{
    // 设置窗口特性
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    // 初始化UI
    setupUI();
    setupAnimations();
    loadMotivationalQuotes();
    
    // 连接定时器到更新时间槽函数
    connect(timer, &QTimer::timeout, this, &FocusMode::updateTime);
    
    // 连接强制专注模式计时器
    connect(focusCountdownTimer, &QTimer::timeout, this, &FocusMode::updateFocusCountdown);
    
    // 先更新一次时间
    updateTime();
    
    // 计算到下一秒的毫秒数
    QTime currentTime = QTime::currentTime();
    int msToNextSecond = 1000 - currentTime.msec();
    
    // 首次启动定时器，对准整秒
    timer->singleShot(msToNextSecond, this, [this]() {
        updateTime(); // 整秒时更新时间
        timer->start(1000); // 然后每秒更新一次
    });
    
    // 设置随机种子
    std::srand(std::time(nullptr));
    
    // 显示随机鼓励短语
    setRandomQuote();
}

FocusMode::~FocusMode()
{
    // 停止定时器
    if (timer && timer->isActive()) {
        timer->stop();
        timer->disconnect();
    }
    
    if (focusCountdownTimer && focusCountdownTimer->isActive()) {
        focusCountdownTimer->stop();
        focusCountdownTimer->disconnect();
    }
    
    // 停止动画
    if (fadeInAnimation && fadeInAnimation->state() == QPropertyAnimation::Running) {
        fadeInAnimation->stop();
        fadeInAnimation->disconnect();
    }
}

void FocusMode::setupUI()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(50, 50, 50, 50);
    mainLayout->setSpacing(20);
    
    // 添加顶部空白区域，让内容居中
    mainLayout->addStretch(2);
    
    // 时间标签 - 大而居中
    timeLabel = new QLabel(this);
    timeLabel->setAlignment(Qt::AlignCenter);
    QFont timeFont("Arial", 90, QFont::Bold);
    timeLabel->setFont(timeFont);
    timeLabel->setStyleSheet("QLabel { color: #FFFFFF; }");
    
    // 为时间标签添加阴影效果
    QGraphicsDropShadowEffect *timeShadow = new QGraphicsDropShadowEffect(this);
    timeShadow->setBlurRadius(10);
    timeShadow->setColor(QColor(0, 0, 0, 160));
    timeShadow->setOffset(2, 2);
    timeLabel->setGraphicsEffect(timeShadow);
    
    // 日期标签 - 居中
    dateLabel = new QLabel(this);
    dateLabel->setAlignment(Qt::AlignCenter);
    QFont dateFont("Arial", 24);
    dateLabel->setFont(dateFont);
    dateLabel->setStyleSheet("QLabel { color: #FFFFFF; }");
    
    // 动机鼓励标签 - 居中
    motivationLabel = new QLabel(this);
    motivationLabel->setAlignment(Qt::AlignCenter);
    QFont quoteFont("Arial", 18, QFont::Normal, true);  // 斜体
    motivationLabel->setFont(quoteFont);
    motivationLabel->setStyleSheet("QLabel { color: #FFFFFF; opacity: 0.8; }");
    motivationLabel->setWordWrap(true);
    
    // 强制专注模式倒计时标签
    focusCountdownLabel = new QLabel(this);
    focusCountdownLabel->setAlignment(Qt::AlignCenter);
    QFont countdownFont("Arial", 16);
    focusCountdownLabel->setFont(countdownFont);
    focusCountdownLabel->setStyleSheet("QLabel { color: #FFFFFF; }");
    focusCountdownLabel->hide(); // 默认隐藏
    
    // 强制专注模式进度条
    focusProgressBar = new QProgressBar(this);
    focusProgressBar->setMaximum(100);
    focusProgressBar->setValue(100);
    focusProgressBar->setTextVisible(false);
    focusProgressBar->setStyleSheet(
        "QProgressBar {"
        "   border: none;"
        "   background-color: rgba(255, 255, 255, 50);"
        "   border-radius: 5px;"
        "   height: 10px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #4CAF50;"  // 绿色
        "   border-radius: 5px;"
        "}"
    );
    focusProgressBar->setFixedHeight(10);
    focusProgressBar->hide(); // 默认隐藏
    
    // 退出按钮 - 右下角
    exitButton = new QPushButton("退出专注模式", this);
    exitButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #FF5722;"  // 橙色
        "   color: white;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 8px 16px;"
        "   font-size: 14pt;"
        "}"
        "QPushButton:hover {"
        "   background-color: #E64A19;"  // 深橙色
        "}"
        "QPushButton:pressed {"
        "   background-color: #BF360C;"  // 更深的橙色
        "}"
        "QPushButton:disabled {"
        "   background-color: #BDBDBD;"  // 灰色
        "   color: #757575;"  // 深灰色
        "}"
    );
    
    // 连接退出按钮
    connect(exitButton, &QPushButton::clicked, this, &FocusMode::onExitButtonClicked);
    
    // 将组件添加到布局
    mainLayout->addWidget(timeLabel);
    mainLayout->addWidget(dateLabel);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(motivationLabel);
    mainLayout->addSpacing(20);
    
    // 添加强制专注模式相关控件
    mainLayout->addWidget(focusCountdownLabel);
    mainLayout->addWidget(focusProgressBar);
    
    mainLayout->addStretch(2);
    
    // 使用水平布局让退出按钮靠右
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(exitButton);
    mainLayout->addLayout(buttonLayout);
}

void FocusMode::setupAnimations()
{
    // 设置淡入动画
    fadeInAnimation = new QPropertyAnimation(this, "windowOpacity");
    fadeInAnimation->setDuration(1000);
    fadeInAnimation->setStartValue(0.0);
    fadeInAnimation->setEndValue(1.0);
    fadeInAnimation->setEasingCurve(QEasingCurve::InOutQuad);
}

void FocusMode::showFullScreen()
{
    // 设置初始透明度
    setWindowOpacity(0.0);
    
    // 调用基类的全屏显示方法
    QWidget::showFullScreen();
    
    // 开始淡入动画
    fadeInAnimation->start();
}

void FocusMode::updateTime()
{
    // 获取当前时间和日期
    QDateTime now = QDateTime::currentDateTime();
    
    // 更新时间标签 - 使用大字号，24小时制 HH:MM:SS
    timeLabel->setText(now.toString("HH:mm:ss"));
    
    // 更新日期标签 - 显示年、月、日和星期几
    dateLabel->setText(now.toString("yyyy年MM月dd日 dddd"));
}

void FocusMode::loadMotivationalQuotes()
{
    // 加载一些鼓励和专注的短语
    motivationalQuotes << "专注当下，成就未来";
    motivationalQuotes << "今日的专注，明日的成就";
    motivationalQuotes << "远离干扰，专心致志";
    motivationalQuotes << "每一分钟都是进步的机会";
    motivationalQuotes << "专注是最简单也是最难的事";
    motivationalQuotes << "深呼吸，保持专注";
    motivationalQuotes << "静心，专注，前行";
    motivationalQuotes << "只要开始，就已经成功了一半";
    motivationalQuotes << "一次只做一件事，做到最好";
    motivationalQuotes << "专注不是一种能力，而是一种习惯";
    motivationalQuotes << "成功不是偶然，而是日复一日的专注";
    motivationalQuotes << "专注于过程，而不是结果";
}

void FocusMode::setRandomQuote()
{
    // 从预设短语中随机选择一个
    if (!motivationalQuotes.isEmpty()) {
        int randomIndex = std::rand() % motivationalQuotes.size();
        motivationLabel->setText(motivationalQuotes.at(randomIndex));
    }
}

void FocusMode::onExitButtonClicked()
{
    // 强制专注模式下检查是否可以退出
    if (!canExitFocusMode()) {
        QMessageBox::warning(this, "无法退出", "强制专注模式下无法提前退出，请等待倒计时结束！", QMessageBox::Ok);
        return;
    }
    
    close();
}

void FocusMode::closeEvent(QCloseEvent *event)
{
    // 强制专注模式下检查是否可以退出
    if (!canExitFocusMode()) {
        QMessageBox::warning(this, "无法退出", "强制专注模式下无法提前退出，请等待倒计时结束！", QMessageBox::Ok);
        event->ignore();
        return;
    }
    
    // 停止定时器
    timer->stop();
    if (focusCountdownTimer->isActive()) {
        focusCountdownTimer->stop();
    }
    
    // 发送关闭信号
    emit closed();
    
    // 接受关闭事件
    event->accept();
}

void FocusMode::keyPressEvent(QKeyEvent *event)
{
    // ESC键退出全屏
    if (event->key() == Qt::Key_Escape) {
        // 强制专注模式下检查是否可以退出
        if (!canExitFocusMode()) {
            QMessageBox::warning(this, "无法退出", "强制专注模式下无法提前退出，请等待倒计时结束！", QMessageBox::Ok);
            event->accept();
            return;
        }
        
        close();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void FocusMode::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制半透明背景
    QLinearGradient gradient(0, 0, width(), height());
    gradient.setColorAt(0, QColor(255, 87, 34, 230));    // 橙色 (#FF5722)
    gradient.setColorAt(1, QColor(255, 152, 0, 230));    // 琥珀色 (#FF9800)
    
    painter.fillRect(rect(), gradient);
    
    // 绘制微妙的圆形装饰
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 30));
    
    // 绘制几个半透明圆形作为背景装饰
    int radius = width() / 6;
    painter.drawEllipse(width() / 4 - radius / 2, height() / 5 - radius / 2, radius, radius);
    painter.drawEllipse(3 * width() / 4 - radius / 2, 4 * height() / 5 - radius / 2, radius, radius);
    
    // 减小透明度再绘制一些圆
    painter.setBrush(QColor(255, 255, 255, 15));
    radius = width() / 4;
    painter.drawEllipse(width() / 8 - radius / 2, 2 * height() / 3 - radius / 2, radius, radius);
    painter.drawEllipse(7 * width() / 8 - radius / 2, height() / 3 - radius / 2, radius, radius);
}

void FocusMode::setFocusMode(FocusModeType mode, int focusDurationMinutes)
{
    modeType = mode;
    
    if (mode == StrongFocus && focusDurationMinutes > 0) {
        // 设置强制专注时间
        focusDurationSeconds = focusDurationMinutes * 60;
        remainingFocusSeconds = focusDurationSeconds;
        
        // 更新UI显示
        updateFocusUI();
        
        // 启动倒计时
        focusCountdownTimer->start(1000);
    } else {
        // 弱专注模式或无时间限制
        focusDurationSeconds = 0;
        remainingFocusSeconds = 0;
        
        // 隐藏倒计时UI
        focusCountdownLabel->hide();
        focusProgressBar->hide();
        
        // 确保退出按钮可用
        exitButton->setEnabled(true);
        
        // 确保倒计时停止
        if (focusCountdownTimer->isActive()) {
            focusCountdownTimer->stop();
        }
    }
}

void FocusMode::updateFocusCountdown()
{
    // 减少剩余时间
    remainingFocusSeconds--;
    
    // 更新UI
    updateFocusUI();
    
    // 检查是否结束
    if (remainingFocusSeconds <= 0) {
        // 停止倒计时
        focusCountdownTimer->stop();
        
        // 隐藏倒计时UI
        focusCountdownLabel->hide();
        focusProgressBar->hide();
        
        // 启用退出按钮
        exitButton->setEnabled(true);
        
        // 提示用户
        QMessageBox::information(this, "专注完成", "恭喜！您已完成专注时间，可以休息一下了。", QMessageBox::Ok);
    }
}

void FocusMode::updateFocusUI()
{
    if (modeType == StrongFocus && focusDurationSeconds > 0) {
        // 显示倒计时
        focusCountdownLabel->setText(QString("强制专注倒计时: %1").arg(formatTimeRemaining(remainingFocusSeconds)));
        focusCountdownLabel->show();
        
        // 更新进度条
        int progressValue = (remainingFocusSeconds * 100) / focusDurationSeconds;
        focusProgressBar->setValue(progressValue);
        focusProgressBar->show();
        
        // 禁用退出按钮
        exitButton->setEnabled(remainingFocusSeconds <= 0);
    } else {
        // 隐藏倒计时UI
        focusCountdownLabel->hide();
        focusProgressBar->hide();
        
        // 确保退出按钮可用
        exitButton->setEnabled(true);
    }
}

bool FocusMode::canExitFocusMode()
{
    // 如果是强制专注模式且时间未到，则不允许退出
    if (modeType == StrongFocus && remainingFocusSeconds > 0) {
        return false;
    }
    
    return true;
}

QString FocusMode::formatTimeRemaining(int seconds)
{
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
} 