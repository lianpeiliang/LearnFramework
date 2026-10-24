#include "DesktopApplication.h"
#include "ApplicationHost.h"

DesktopApplication::DesktopApplication(int &argc, char **argv)
    : QApplication(argc, argv)
{
    ApplicationHost::instance()->show();
}
