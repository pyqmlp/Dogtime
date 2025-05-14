QT += core gui widgets

# 保持简单，不添加不必要的模块
TARGET = Dogtime
TEMPLATE = app
CONFIG += c++17

# 确保MOC处理正确
CONFIG += debug_and_release

# 源文件
SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/systemtrayicon.cpp \
    src/settingswindow.cpp \
    src/floatingclock.cpp \
    src/countdownnotification.cpp \
    src/focusmode.cpp

# 头文件
HEADERS += \
    src/mainwindow.h \
    src/systemtrayicon.h \
    src/settingswindow.h \
    src/floatingclock.h \
    src/countdownnotification.h \
    src/focusmode.h \
    src/static_plugins.h

# 资源文件
RESOURCES += \
    resources/dogtime.qrc

# 简化构建目录配置
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/bin/debug
    TARGET = $${TARGET}d
} else {
    DESTDIR = $$PWD/bin/release
}

OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc
RCC_DIR = $$PWD/build/rcc
UI_DIR = $$PWD/build/ui

# 平台特定配置
win32 {
    RC_ICONS = resources/icons/dogtime.ico
    
    # 添加Windows库
    LIBS += -lUser32 -lshell32 -lwinmm
    
    # MinGW编译器设置
    QMAKE_CXXFLAGS_DEBUG += -g3 -O0
}

# 安装配置
target.path = /usr/local/bin
INSTALLS += target

# 移除Windows静态编译特定设置
# win32:CONFIG(static) {
#     # 确保包含所有必要的插件
#     QT += widgets-private
#     
#     # 添加必要的静态库
#     LIBS += -lopengl32 -lglu32 -ld3d11 -ldxgi -ldwrite -lwinspool
#     
#     # 额外添加可能需要的系统库
#     LIBS += -ladvapi32 -lws2_32 -limm32 -luuid -loleaut32 -lole32 -lgdi32 -lcomdlg32
#     
#     # Qt 6特定库
#     greaterThan(QT_MAJOR_VERSION, 5): {
#         LIBS += -lqtharfbuzzng -lqtlibpng -lqtfreetype -lqtpcre2
#     } else {
#         # Qt 5特定库
#         LIBS += -lharfbuzz -lpng -lfreetype -lpcre2
#     }
# } 