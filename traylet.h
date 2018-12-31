#ifndef TRAYLET_H
#define TRAYLET_H

#include <QSystemTrayIcon>

#ifndef QT_NO_SYSTEMTRAYICON

#include <QString>
#include <QVector>
#include <QWidgetAction> // Inherits QAction
#include <QTextStream>
#include <QIcon>
#include <QMenu> // Context menu
#include <QMessageBox> // About dialog
#include <QApplication> // qApp and QCoreApplication
#include <QTimer> // Timer stuff
#include <QDebug> // QDebug
#include <QFile>
#include <QRegularExpression>
#include <QStringList>

class Traylet : public QSystemTrayIcon {
    Q_OBJECT

public:
    enum state {
        GOOD,
        BAD,
        MISSING,
        INIT
    };

    Traylet();

private slots:
    void scan();
    void setIcon();
    void showAbout();
    void notify();
    void setNotifToggle();

private:
    state currState;
    bool blinkState;
    bool notifToggle;
    QString processedInput;
    QString aboutContent;
    QString notifyTitle;
    QString notifyMessage;
    QMessageBox * aboutDialog;
    QMenu * trayMenu;

    QIcon * goodIcon;
    QIcon * badIcon_lit;
    QIcon * badIcon_dim;
    QIcon * missingIcon;

    QTimer * scanTimer;
    QTimer * iconBlinkTimer;
    QTimer * notifyTimer;

    QAction * notifToggleAction;
    QAction * aboutButtonAction;
    QAction * quitButtonAction;

    void createAbout();
    void createNotification();
    void createTrayMenu();
    QString getInput();
    void cleanInput(QString &);
    void setState();
};

#endif // QT_NO_SYSTEMTRAYICON

#endif // TRAYLET_H
