#include "DesktopApplication.h"
#include "ApplicationHost.h"
#include <QTimer>

DesktopApplication::DesktopApplication(int &argc, char **argv)
    : QApplication(argc, argv)
{
    if (!ApplicationHost::instance()->show())
        QTimer::singleShot(0, this, [] { QCoreApplication::exit(1); });
}
