#include "mainwindow.h"
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , centralLabel(new QLabel())
{
    // 设置窗口属性
    setWindowTitle("Dogtime");
    setMinimumSize(400, 300);
    
    // 创建中央部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // 创建布局
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    
    // 设置标签内容
    centralLabel->setText("欢迎使用 Dogtime 应用程序!");
    centralLabel->setAlignment(Qt::AlignCenter);
    
    // 添加标签到布局
    layout->addWidget(centralLabel);
}

MainWindow::~MainWindow()
{
    // QMainWindow 会自动删除中央部件及其子部件
} 