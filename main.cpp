#include "MainWindow.h"
#include "AED.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    // Create AED device.
    AED* device = new AED();
    w.addAED(device);

    w.show();

    return a.exec();
}
