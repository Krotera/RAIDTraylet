#include <QApplication>

#if defined(Q_OS_LINUX) && !defined(QT_NO_SYSTEMTRAYICON)

#include <QMessageBox>

#include "traylet.h"

int main(int argc, char *argv[]) {
    Q_INIT_RESOURCE(resources);

    QApplication a(argc, argv);
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(0, QObject::tr("Systray"),
                              QObject::tr("A system tray was not detected "
                                          "on this system."));
        return 1;
    }
    QApplication::setQuitOnLastWindowClosed(false);
    Traylet w;
    w.show();

    return a.exec();
}

#else

#include <QLabel>
#include <QDebug>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QString text("RAIDTraylet is only supported on Linux systems with system trays.");

    QLabel *label = new QLabel(text);
    label->setWordWrap(true);

    label->show();
    qDebug() << text;

    app.exec();
}

#endif
