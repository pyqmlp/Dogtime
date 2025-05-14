#include "countdownnotification.h"
#include <QScreen>
#include <QGuiApplication>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>

// Windows系统上使用MessageBeep
#ifdef _WIN32
#include <Windows.h>
#endif

CountdownNotification::CountdownNotification(QWidget *parent)
    : QWidget(parent)
    , titleLabel(nullptr)
    , messageLabel(nullptr)
    , closeButton(nullptr)
    , autoCloseTimer(new QTimer(this))
    , shakeAnimation(nullptr)
    , rotateAnimation(nullptr)
    , animationGroup(nullptr)
    , angle(0)
{
    // 设置窗口为无边框、置顶，但不阻止其他窗口操作
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating); // 显示时不激活窗口（不抢焦点）
    
    // 设置窗口大小
    resize(300, 150);
    
    // 设置阴影效果
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);
    
    // 创建动画组和相关动画
    setupAnimations();
    
    setupUI();
}

CountdownNotification::~CountdownNotification()
{
    // 停止动画
    if (animationGroup && animationGroup->state() == QAbstractAnimation::Running) {
        animationGroup->stop();
    }
}

void CountdownNotification::setupAnimations()
{
    // 创建左右摇晃动画
    shakeAnimation = new QPropertyAnimation(this, "pos");
    shakeAnimation->setDuration(100);
    
    // 创建旋转动画
    rotateAnimation = new QPropertyAnimation(this, "angle");
    rotateAnimation->setDuration(800);
    rotateAnimation->setStartValue(0);
    rotateAnimation->setEndValue(0);
    rotateAnimation->setEasingCurve(QEasingCurve::OutElastic);
    
    // 动画结束后的连接
    connect(rotateAnimation, &QPropertyAnimation::valueChanged, this, [this]() {
        update(); // 角度变化时触发重绘
    });
    
    // 创建动画组
    animationGroup = new QSequentialAnimationGroup(this);
}

void CountdownNotification::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);
    
    // 标题
    titleLabel = new QLabel(this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { color: #FFFFFF; font-size: 16pt; font-weight: bold; }");
    
    // 消息
    messageLabel = new QLabel(this);
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setStyleSheet("QLabel { color: #FFFFFF; font-size: 12pt; }");
    messageLabel->setWordWrap(true);
    
    // 关闭按钮
    closeButton = new QPushButton("确定", this);
    closeButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #FF5722;"  // 更改为橙色，与logo相呼应
        "   color: white;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 5px 15px;"
        "   font-size: 12pt;"
        "}"
        "QPushButton:hover {"
        "   background-color: #E64A19;"  // 深橙色
        "}"
        "QPushButton:pressed {"
        "   background-color: #BF360C;"  // 更深的橙色
        "}"
    );
    
    // 添加部件到布局
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(messageLabel);
    mainLayout->addStretch();
    mainLayout->addWidget(closeButton, 0, Qt::AlignCenter);
    
    // 连接关闭按钮
    connect(closeButton, &QPushButton::clicked, this, &CountdownNotification::onCloseButtonClicked);
}

void CountdownNotification::onCloseButtonClicked()
{
    close();
    emit closed();
}

void CountdownNotification::showNotification(const QString &title, const QString &message)
{
    titleLabel->setText(title);
    messageLabel->setText(message);
    
    // 定位到屏幕中央偏下位置
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2 + 100;
    
    // 移动到起始位置
    move(x, y);
    
    // 显示窗口
    show();
    
    // 使用系统声音代替QSoundEffect
    #ifdef _WIN32
    MessageBeep(MB_ICONEXCLAMATION); // 使用警告音，更像闹钟
    #endif
    
    // 启动闹钟动画
    startAlarmAnimation();
}

void CountdownNotification::startAlarmAnimation()
{
    // 停止正在运行的动画
    if (animationGroup->state() == QAbstractAnimation::Running) {
        animationGroup->stop();
    }
    
    // 清除现有动画
    animationGroup->clear();
    
    // 创建左右摇晃动画序列
    QParallelAnimationGroup *shakeGroup = new QParallelAnimationGroup;
    
    // 当前位置
    QPoint currentPos = pos();
    
    // 创建左右摇晃效果 - 多次左右移动，模拟闹钟振铃
    QPropertyAnimation *shakeLeft = new QPropertyAnimation(this, "pos");
    shakeLeft->setDuration(50);
    shakeLeft->setStartValue(currentPos);
    shakeLeft->setEndValue(QPoint(currentPos.x() - 10, currentPos.y()));
    
    QPropertyAnimation *shakeRight = new QPropertyAnimation(this, "pos");
    shakeRight->setDuration(50);
    shakeRight->setStartValue(QPoint(currentPos.x() - 10, currentPos.y()));
    shakeRight->setEndValue(QPoint(currentPos.x() + 10, currentPos.y()));
    
    QPropertyAnimation *shakeReturn = new QPropertyAnimation(this, "pos");
    shakeReturn->setDuration(50);
    shakeReturn->setStartValue(QPoint(currentPos.x() + 10, currentPos.y()));
    shakeReturn->setEndValue(currentPos);
    
    // 设置震动轨迹，模拟闹钟震动
    for (int i = 0; i < 3; i++) {
        QPropertyAnimation *shake1 = new QPropertyAnimation(this, "pos");
        shake1->setDuration(40);
        shake1->setStartValue(currentPos);
        shake1->setEndValue(QPoint(currentPos.x() - 5, currentPos.y()));
        
        QPropertyAnimation *shake2 = new QPropertyAnimation(this, "pos");
        shake2->setDuration(40);
        shake2->setStartValue(QPoint(currentPos.x() - 5, currentPos.y()));
        shake2->setEndValue(QPoint(currentPos.x() + 5, currentPos.y()));
        
        QPropertyAnimation *shake3 = new QPropertyAnimation(this, "pos");
        shake3->setDuration(40);
        shake3->setStartValue(QPoint(currentPos.x() + 5, currentPos.y()));
        shake3->setEndValue(currentPos);
        
        shakeGroup->addAnimation(shake1);
        shakeGroup->addAnimation(shake2);
        shakeGroup->addAnimation(shake3);
    }
    
    // 添加微小旋转动画
    QPropertyAnimation *rotateAnim = new QPropertyAnimation(this, "angle");
    rotateAnim->setDuration(800);
    rotateAnim->setStartValue(-5);
    rotateAnim->setEndValue(5);
    rotateAnim->setEasingCurve(QEasingCurve::InOutQuad);
    
    QPropertyAnimation *rotateBack = new QPropertyAnimation(this, "angle");
    rotateBack->setDuration(800);
    rotateBack->setStartValue(5);
    rotateBack->setEndValue(0);
    rotateBack->setEasingCurve(QEasingCurve::OutElastic);
    
    // 将所有动画组合在一起
    animationGroup->addAnimation(shakeGroup);
    animationGroup->addAnimation(rotateAnim);
    animationGroup->addAnimation(rotateBack);
    
    // 启动动画
    animationGroup->start();
    
    // 重复动画（每3秒触发一次）
    QTimer::singleShot(3000, this, &CountdownNotification::startAlarmAnimation);
}

// 角度属性的get/set方法
qreal CountdownNotification::getAngle() const
{
    return angle;
}

void CountdownNotification::setAngle(qreal newAngle)
{
    angle = newAngle;
    update(); // 属性变化时触发重绘
}

void CountdownNotification::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 应用旋转
    if (angle != 0) {
        painter.translate(width() / 2, height() / 2);
        painter.rotate(angle);
        painter.translate(-width() / 2, -height() / 2);
    }
    
    // 创建圆角矩形路径
    QPainterPath path;
    path.addRoundedRect(rect(), 10, 10);
    
    // 设置裁剪区域
    painter.setClipPath(path);
    
    // 绘制渐变背景 - 使用与项目logo相呼应的颜色
    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0, QColor(255, 87, 34, 230));    // 橙色 (#FF5722)
    gradient.setColorAt(1, QColor(255, 152, 0, 230));    // 琥珀色 (#FF9800)
    painter.fillPath(path, gradient);
    
    // 添加闹钟图案
    drawAlarmClockPattern(&painter);
    
    // 绘制边框
    painter.setPen(QPen(QColor(255, 152, 0, 100), 2)); // 加粗橙色边框
    painter.drawPath(path);
}

void CountdownNotification::drawAlarmClockPattern(QPainter *painter)
{
    // 在右上角画一个简化的闹钟图案
    int size = 30;
    int x = width() - size - 15;
    int y = 15;
    
    painter->save();
    painter->setPen(QPen(QColor(255, 255, 255, 100), 2));
    
    // 闹钟主体 - 圆形
    painter->drawEllipse(x, y, size, size);
    
    // 闹钟顶部的铃铛
    painter->drawArc(x + size/4, y - size/4, size/2, size/4, 0, 180 * 16);
    
    // 时钟指针
    painter->drawLine(x + size/2, y + size/2, x + size/2, y + size/2 - size/3);
    painter->drawLine(x + size/2, y + size/2, x + size/2 + size/4, y + size/2);
    
    // 闹钟底座
    painter->drawLine(x + size/4, y + size, x + size/4, y + size + size/6);
    painter->drawLine(x + size - size/4, y + size, x + size - size/4, y + size + size/6);
    
    painter->restore();
}

void CountdownNotification::mousePressEvent(QMouseEvent *event)
{
    // 不再处理鼠标点击事件，只有点击关闭按钮才会关闭窗口
    Q_UNUSED(event);
    // 保留原有的事件处理
    QWidget::mousePressEvent(event);
} 