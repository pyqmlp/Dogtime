// 静态插件导入头文件 
#ifndef STATIC_PLUGINS_H 
#define STATIC_PLUGINS_H 
 
#include <QtPlugin> 

// 只在使用静态编译时导入插件
#ifdef QT_STATIC
// 只导入Windows集成插件，其他插件按需添加
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

#endif // STATIC_PLUGINS_H 
