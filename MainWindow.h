#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /// Implements Power On Self-Test.
    void selfTest();

public slots:
    void turnOnIndicator(int index);
    void turnOffIndicator(int index);

    // Functions for updating CPR depth compression indicator.
    void setCPRDepth(float depth);

private slots:

    void on_powerBtn_toggled(bool checked);

private:
    Ui::MainWindow *ui;

    void powerOn();
    void powerOff();

    // Keep a list of the indicators shoing current AED operation step.
    QList<QLabel*> stepIndicators;
};
#endif // MAINWINDOW_H
