#include "MainWindow.h"
#include "AED.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    // Create AED device.
    AED* device = new AED();
    w.addAED(device);


    a.setStyle(QStyleFactory::create("Fusion"));

    w.show();

    return a.exec();
}
