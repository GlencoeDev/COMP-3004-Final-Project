#include "MainWindow.h"
#include "AED.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));

    MainWindow w;

    // Create AED device.
    AED* device = new AED();

    w.addAED(device);

    // TODO: Put AED class into a separate thread.

    w.show();

    return a.exec();
}
