#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>

class SettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    ~SettingsWindow();

private slots:
    void browseLogFile();
    void browseConfigFile();
    void restoreDefaults();
    void applySettings();
    void accept() override;

private:
    // 主要组件
    QTabWidget *tabWidget;
    QWidget *generalTab;
    QWidget *interfaceTab;
    QWidget *hotkeysTab;
    QWidget *aboutTab;
    QDialogButtonBox *buttonBox;

    // 常规选项卡的控件
    QCheckBox *enableLoggingCheckBox;
    QLineEdit *logFilePathEdit;
    QPushButton *browseLogButton;
    QSpinBox *logLevelSpinBox;
    QComboBox *logTypeComboBox;
    
    QLineEdit *configFilePathEdit;
    QPushButton *browseConfigButton;
    QPushButton *restoreDefaultsButton;

    // 方法
    void createTabs();
    void createGeneralTab();
    void createInterfaceTab();
    void createHotkeysTab();
    void createAboutTab();
    
    void saveSettings();
    void loadSettings();
};

#endif // SETTINGSWINDOW_H 