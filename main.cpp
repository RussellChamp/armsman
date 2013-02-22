#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}

/*
 * Known issues:
 * - attack bonuses show a '+' even when the bonus is a '-'
 */
