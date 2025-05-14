#include <QApplication>
#include "systemtrayicon.h"
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序名称和组织信息
    app.setApplicationName("Dogtime");
    app.setOrganizationName("DogSoft");
    app.setQuitOnLastWindowClosed(false); // 关闭窗口时不退出应用程序，由用户手动退出
    
    // 设置应用程序图标
    QIcon appIcon(":/icons/dogtime.ico");
    if (appIcon.isNull()) {
        appIcon = QIcon(":/icons/dogtime.png");
    }
    app.setWindowIcon(appIcon);
    
    // 创建系统托盘图标
    SystemTrayIcon trayIcon;
    trayIcon.show();
    
    return app.exec();
} 