#include "settingswindow.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <QIcon>
#include <QApplication>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QDialog(parent)
    , tabWidget(new QTabWidget(this))
    , generalTab(new QWidget())
    , interfaceTab(new QWidget())
    , hotkeysTab(new QWidget())
    , aboutTab(new QWidget())
    , buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this))
    , enableLoggingCheckBox(new QCheckBox("启用日志记录", this))
    , logFilePathEdit(new QLineEdit(this))
    , browseLogButton(new QPushButton("浏览...", this))
    , logLevelSpinBox(new QSpinBox(this))
    , logTypeComboBox(new QComboBox(this))
    , configFilePathEdit(new QLineEdit(this))
    , browseConfigButton(new QPushButton("浏览...", this))
    , restoreDefaultsButton(new QPushButton("恢复默认设置", this))
{
    // 设置窗口属性
    setWindowTitle("设置");
    setFixedWidth(450); // 设置固定宽度
    setFixedHeight(450); // 设置固定高度，防止窗口过小
    
    // 设置窗口标志 - 置顶且禁止调整大小
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::MSWindowsFixedSizeDialogHint);
    
    // 确保窗口真的不能调整大小
    setFixedSize(450, 450);
    
    // 设置窗口图标
    QIcon appIcon(":/icons/dogtime.ico");
    if (appIcon.isNull()) {
        appIcon = QIcon(":/icons/dogtime.png");
    }
    setWindowIcon(appIcon);
    
    // 创建布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 创建标签页
    createTabs();
    
    // 添加标签页到布局
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(buttonBox);
    
    // 设置按钮文本为中文
    buttonBox->button(QDialogButtonBox::Ok)->setText("确定");
    buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");
    buttonBox->button(QDialogButtonBox::Apply)->setText("应用");
    
    // 连接信号和槽
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsWindow::applySettings);
    
    connect(browseLogButton, &QPushButton::clicked, this, &SettingsWindow::browseLogFile);
    connect(browseConfigButton, &QPushButton::clicked, this, &SettingsWindow::browseConfigFile);
    connect(restoreDefaultsButton, &QPushButton::clicked, this, &SettingsWindow::restoreDefaults);
    
    // 加载当前设置
    loadSettings();
}

SettingsWindow::~SettingsWindow()
{
    // QDialog会自动删除子部件
}

void SettingsWindow::createTabs()
{
    // 创建各个标签页
    createGeneralTab();
    createInterfaceTab();
    createHotkeysTab();
    createAboutTab();
    
    // 添加标签页到标签页部件
    tabWidget->addTab(generalTab, "常规");
    tabWidget->addTab(interfaceTab, "界面");
    tabWidget->addTab(hotkeysTab, "热键");
    tabWidget->addTab(aboutTab, "关于");
}

void SettingsWindow::createGeneralTab()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(generalTab);
    
    // 日志设置组
    QGroupBox *loggingGroup = new QGroupBox("日志设置", generalTab);
    QVBoxLayout *loggingLayout = new QVBoxLayout(loggingGroup);
    
    loggingLayout->addWidget(enableLoggingCheckBox);
    
    QHBoxLayout *logFileLayout = new QHBoxLayout();
    QLabel *logFileLabel = new QLabel("日志文件路径:", generalTab);
    logFileLayout->addWidget(logFileLabel);
    logFileLayout->addWidget(logFilePathEdit, 1);
    logFileLayout->addWidget(browseLogButton);
    loggingLayout->addLayout(logFileLayout);
    
    QHBoxLayout *logSettingsLayout = new QHBoxLayout();
    QLabel *logLevelLabel = new QLabel("日志级别:", generalTab);
    logLevelSpinBox->setRange(0, 5);
    logLevelSpinBox->setToolTip("0=禁用, 1=错误, 2=警告, 3=信息, 4=调试, 5=详细");
    
    QLabel *logTypeLabel = new QLabel("日志类型:", generalTab);
    logTypeComboBox->addItems(QStringList() << "文本" << "XML" << "JSON");
    
    logSettingsLayout->addWidget(logLevelLabel);
    logSettingsLayout->addWidget(logLevelSpinBox);
    logSettingsLayout->addSpacing(20);
    logSettingsLayout->addWidget(logTypeLabel);
    logSettingsLayout->addWidget(logTypeComboBox);
    loggingLayout->addLayout(logSettingsLayout);
    
    // 配置文件设置组
    QGroupBox *configGroup = new QGroupBox("配置文件", generalTab);
    QVBoxLayout *configLayout = new QVBoxLayout(configGroup);
    
    QHBoxLayout *configFileLayout = new QHBoxLayout();
    QLabel *configFileLabel = new QLabel("配置文件路径:", generalTab);
    configFileLayout->addWidget(configFileLabel);
    configFileLayout->addWidget(configFilePathEdit, 1);
    configFileLayout->addWidget(browseConfigButton);
    configLayout->addLayout(configFileLayout);
    
    // 添加恢复默认按钮
    QHBoxLayout *defaultsLayout = new QHBoxLayout();
    defaultsLayout->addStretch();
    defaultsLayout->addWidget(restoreDefaultsButton);
    
    // 添加所有组件到主布局
    mainLayout->addWidget(loggingGroup);
    mainLayout->addWidget(configGroup);
    mainLayout->addLayout(defaultsLayout);
    mainLayout->addStretch();
}

void SettingsWindow::createInterfaceTab()
{
    QVBoxLayout *layout = new QVBoxLayout(interfaceTab);
    QLabel *label = new QLabel("界面设置选项将在此显示");
    layout->addWidget(label);
    layout->addStretch();
}

void SettingsWindow::createHotkeysTab()
{
    QVBoxLayout *layout = new QVBoxLayout(hotkeysTab);
    QLabel *label = new QLabel("热键设置选项将在此显示");
    layout->addWidget(label);
    layout->addStretch();
}

void SettingsWindow::createAboutTab()
{
    QVBoxLayout *layout = new QVBoxLayout(aboutTab);
    QLabel *label = new QLabel("Dogtime应用程序\n版本: 1.0.0\n© 2023 DogSoft");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    layout->addStretch();
}

void SettingsWindow::browseLogFile()
{
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString filePath = QFileDialog::getSaveFileName(this,
        "选择日志文件位置", defaultDir, "日志文件 (*.log)");
    
    if (!filePath.isEmpty()) {
        logFilePathEdit->setText(filePath);
    }
}

void SettingsWindow::browseConfigFile()
{
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString filePath = QFileDialog::getSaveFileName(this,
        "选择配置文件位置", defaultDir, "配置文件 (*.ini *.conf);;所有文件 (*)");
    
    if (!filePath.isEmpty()) {
        configFilePathEdit->setText(filePath);
    }
}

void SettingsWindow::restoreDefaults()
{
    // 对话框确认
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        "恢复默认设置", "确定要恢复所有设置到默认值吗？\n此操作无法撤销。",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // 设置默认值
        enableLoggingCheckBox->setChecked(true);
        
        QString defaultLogPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + 
                              QDir::separator() + "Dogtime.log";
        logFilePathEdit->setText(defaultLogPath);
        
        logLevelSpinBox->setValue(3); // 信息级别
        logTypeComboBox->setCurrentIndex(0); // 文本
        
        QString defaultConfigPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + 
                                 QDir::separator() + "Dogtime.ini";
        configFilePathEdit->setText(defaultConfigPath);
        
        QMessageBox::information(this, "设置已重置", "所有设置已恢复到默认值。");
    }
}

void SettingsWindow::applySettings()
{
    saveSettings();
    QMessageBox::information(this, "设置已应用", "设置已成功保存。");
}

void SettingsWindow::loadSettings()
{
    QSettings settings("DogSoft", "Dogtime");
    
    // 加载日志设置
    enableLoggingCheckBox->setChecked(settings.value("logging/enabled", true).toBool());
    
    QString defaultLogPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + 
                          QDir::separator() + "Dogtime.log";
    logFilePathEdit->setText(settings.value("logging/filePath", defaultLogPath).toString());
    
    logLevelSpinBox->setValue(settings.value("logging/level", 3).toInt());
    logTypeComboBox->setCurrentIndex(settings.value("logging/type", 0).toInt());
    
    // 加载配置文件设置
    QString defaultConfigPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + 
                             QDir::separator() + "Dogtime.ini";
    configFilePathEdit->setText(settings.value("config/filePath", defaultConfigPath).toString());
}

void SettingsWindow::saveSettings()
{
    QSettings settings("DogSoft", "Dogtime");
    
    // 保存日志设置
    settings.setValue("logging/enabled", enableLoggingCheckBox->isChecked());
    settings.setValue("logging/filePath", logFilePathEdit->text());
    settings.setValue("logging/level", logLevelSpinBox->value());
    settings.setValue("logging/type", logTypeComboBox->currentIndex());
    
    // 保存配置文件设置
    settings.setValue("config/filePath", configFilePathEdit->text());
    
    settings.sync(); // 确保设置被写入
}

// 重写接受事件，确保点击OK时保存设置
void SettingsWindow::accept()
{
    saveSettings();
    QDialog::accept();
} 