#include "traylet.h"

#ifndef QT_NO_SYSTEMTRAYICON

#define VERSION "0.1"
#define PROJECT_REPO_PAGE "https://github.com/Krotera/RAIDTraylet"

/***
 * Default ctor that initializes the Traylet's members and UI
 ***/
Traylet::Traylet() {
    // Init data and UI
    currState = INIT;
    blinkState = 0;
    processedInput = "NONE";
    /*
     * For the icons, add images to the .qrc file per the format specified at
     * https://doc.qt.io/qt-5/resources.html
     *
     * The warnings
     *      "libpng warning: iCCP: known incorrect sRGB profile"
     * can probably be ignored but can be fixed by optimizing the PNGs with imagemagick
     *      "convert red.png red_opt.png"
     * which, if nothing else, will also make 'em smaller.
     */
    goodIcon = new QIcon(":/images/green.png");
    badIcon_lit = new QIcon(":/images/red_cbt.png");
    badIcon_dim = new QIcon(":/images/red_dim_cbt.png");
    missingIcon = new QIcon(":/images/green_dim_cbt.png");
    QSystemTrayIcon::setIcon(*missingIcon); // Initial icon
    scanTimer = new QTimer(this);
    iconBlinkTimer = new QTimer(this);
    notifyTimer = new QTimer(this);
    createAbout();
    createNotification();
    createTrayMenu();
    this->setContextMenu(trayMenu); // Set the created trayMenu to this
    this->show(); // Shows the tray icon
    // Connect timers' timeout() signal to the corresponding slots
    connect(scanTimer, &QTimer::timeout, this, &Traylet::scan);
    connect(iconBlinkTimer, &QTimer::timeout, this, &Traylet::setIcon);
    connect(notifyTimer, &QTimer::timeout, this, &Traylet::notify);
    // Set timers
    scanTimer->start(60000); // Set scan intervals to 1 min
    iconBlinkTimer->setInterval(500); // Set icon blink intervals to 0.5 sec, stopped initially
    notifyTimer->setInterval(900000); // Set notification intervals to 15 mins, stopped initially
    scan(); // Initial startup scan
}

/***
 * This is a slot connected to the QTimer scanTimer, which emits its timeout() signal
 * at some pre-established frequency, encoded in scanTimer->interval. That signal is
 * connected to scan(), which is thus called at that frequency.
 *
 * This acquires an updated RAID state via setState(),
 * and, if the new state differs from the current state, updates the current state and
 * either calls setIcon() if the new state is not BAD or initiates repeated tray
 * icon blinking and notifications for the duration of the BAD state
 ***/
void Traylet::scan() {
    state lastState = currState;
    QString input = getInput();

    cleanInput(input);
    setState();
    // Act on state change
    if (currState != lastState) {
        if (currState == BAD) { // Entered BAD state (trigger blinking and notifications)
            notify();
            notifyTimer->start();
            iconBlinkTimer->start();
        }
        else {
            if (lastState == BAD) { // Exited BAD state (stop blinking and notifications)
                notifyTimer->stop();
                iconBlinkTimer->stop();
            }
            setIcon(); // Entered GOOD or MISSING state (just set icon)
        }
    }
}

/***
 * Sets the tray icon based on currentState
 ***/
void Traylet::setIcon() {
    if (currState == GOOD) {
        this->QSystemTrayIcon::setIcon(*goodIcon);
    }
    else if (currState == BAD) {
        // Since checking if QIcon == QIcon is hard, the bool blinkState guides blinking
        if (blinkState == 0) {
            blinkState = 1;
            this->QSystemTrayIcon::setIcon(*badIcon_lit);
        }
        else {
            blinkState = 0;
            this->QSystemTrayIcon::setIcon(*badIcon_dim);
        }
    }
    else if (currState == MISSING) {
        this->QSystemTrayIcon::setIcon(*missingIcon);
    }
}

/***
 * Shows the About dialog
 ***/
void Traylet::showAbout() {
    aboutDialog->about(0, "About", aboutContent);
}

/***
 * Shows the BAD state notification
 ***/
void Traylet::notify() {
    if (notifToggle) {
        // Try to make notifications endure for 5 mins
        this->showMessage(notifyTitle, notifyMessage, QSystemTrayIcon::Critical, 300000);
    }
}

/***
 * Sets the bool notifToggle based on the checkable QAction notifToggleAction
 ***/
void Traylet::setNotifToggle() {
    if (!notifToggleAction->isChecked() && notifToggle) {
        notifToggle = false;
    }
    else if (notifToggleAction->isChecked() && !notifToggle) {
        notifToggle = true;
    }
}

/***
 * Initializes About dialog
 ***/
void Traylet::createAbout() {
    QTextStream in(&aboutContent);

    in << "<center><big><b>RAIDTraylet</b></big><br><small>Version " << VERSION <<
           "<br>(c) 2018 Krotera</small></center><br>Source: <a href=\"" << PROJECT_REPO_PAGE
        << "\">" << PROJECT_REPO_PAGE << "</a><br>Built against Qt 5.11.0<br>License: "
           "<a href=\"https://www.gnu.org/licenses/gpl.html\">GPLv3</a>";
}

/***
 * Initializes BAD state notification
 ***/
void Traylet::createNotification() {
    notifToggle = true; // Notifications on by default
    notifyTitle = "RAID failed or degraded";
    notifyMessage = "Uncheck \"Notify\" to turn off notifications.";
}

/***
 * Initializes the tray icon's context menu and its actions
 ***/
void Traylet::createTrayMenu() {
    trayMenu = new QMenu();

    // Init notification toggle button
    notifToggleAction = new QAction(tr("&Notify"), 0);
    notifToggleAction->setCheckable(true); // Make this a checkbox action
    notifToggleAction->setChecked(true); // Necessary since default state is unchecked
    connect(notifToggleAction, &QAction::toggled, this, &Traylet::setNotifToggle);

    // Init About dialog and About button
    aboutDialog = new QMessageBox();
    // Note: the "&" in "&About" sets the shortcut key to the action when in the menu
    aboutButtonAction = new QAction(tr("&About"), 0);
    connect(aboutButtonAction, &QAction::triggered, this, &Traylet::showAbout);

    // Init Quit button
    quitButtonAction = new QAction(tr("&Quit"), 0);
    connect(quitButtonAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    // Add notification toggle button
    trayMenu->addAction(notifToggleAction);

    // Add About button
    trayMenu->addAction(aboutButtonAction);

    trayMenu->addSeparator();

    // Add Quit button
    trayMenu->addAction(quitButtonAction);
}

/***
 * Attempts to read and return the /proc/mdstat file's content
 ***/
QString Traylet::getInput() {
    QFile mdstat("/proc/mdstat");

    if (mdstat.exists() && mdstat.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&mdstat);

        return in.readAll();
    }
    else if (!mdstat.exists()) { // Failed to find
        qDebug() << "ERROR: /proc/mdstat was not found.\n";
    }
    else if (!mdstat.open(QFile::ReadOnly | QFile::Text)) { // Failed to open
        qDebug() << "ERROR: /proc/mdstat could not be opened.\n";
    }
    return "NONE";
}

/***
 * Processes the QString rawInput and writes the QString processedInput with the
 * deduced state of the RAIDs found in /proc/mdstat along with the faulty RAIDs found
 *
 * Assume there are three RAIDs listed in /proc/mdstat: md0, md1, and md2.
 * If md1 and md2 appear faulty, processedInput will be "BAD,md1,md2".
 * If none of the RAIDs appear faulty, processedInput will be "GOOD".
 * If there are no RAIDs listed, processedInput will be "MISSING".
 ***/
void Traylet::cleanInput(QString & rawInput) {
    /*
     * /proc/mdstat's appearance when there are no RAIDs:
     *
     *      Personalities :
     *      unused devices: <none>
     *
     * When there are several RAIDs of various types and states:
     *
     *      Personalities : [raid0] [raid1] [raid6] [raid5] [raid4]
     *      md0 : active raid0 sdc1[1] sdb1[0]
     *            136448 blocks super 1.2 512k chunks
     *
     *      md1 : active raid1 sdb3[1] sda3[0](F)
     *            129596288 blocks [2/1] [U_]
     *
     *      md5 : active raid5 sdl1[9] sdk1[8] sdj1[7] sdi1[6] sdh1[5] sdg1[4] sdf1[3] sde1[2] sdd1[1] sd. . .
     *            1318680576 blocks level 5, 1024k chunk, algorithm 2 [10/10] [UUUUUUUUUU]
     *
     *      md127 : active raid5 sdv1[6] sdn1[4] sdm1[3] sdz1[2] sdy1[1] sdz1[0]
     *            1464725760 blocks level 5, 64k chunk, algorithm 2 [6/5] [UUUUU_]
     *            [==>..................]  recovery = 12.6% (37043392/292945152) finish=127.5min speed=33. . .
     *
     *      unused devices: <none>
     *
     * Based on this, there are several data fields we can key on to detect faulty RAIDs:
     *
     * 0. The "active/inactive" one is unreliable. Often, faulty RAIDs seem marked "active".
     *
     * 1. Similarly, "(F)/(E)"s may NOT be present on a failed RAID's members (the failed disk
     * may not show up at all if they're removed via mdadm or too damaged to be recognized by
     * the system).
     * Also, usually, it's "(F)". "(E)" seems unique to Synology systems, but it is supported here.
     * Legends^[a] whisper of "(S)", "(W)", and "(R)" flags, but I've found no documentation that
     * describes any of those. :C
     *
     *      [a]: https://raid.wiki.kernel.org/index.php/Talk:Mdstat
     *
     * 2. "[UU]/[_U]"s can appear even in recoveries, and these don't appear at all for RAID0s.
     *
     * 3. Similarly, "[n/m]"s can appear even in recoveries, and these don't appear at all for RAID0s.
     * Here, n is how many devices the RAID needs and m is how many are currently sync'd members of the RAID.
     * If n > m, the RAID is in a failed state because it doesn't have sufficient members.
     *
     * This function will examine each "chunk" of lines, checking each RAID device for failure. The resulting
     * state will be at the beginning of processedInput followed by all faulty RAIDs, if any, all delimited by
     * ","s. Also, since any RAID with the "recovery =" is recovering, it isn't "faulty" in the sense intended
     * here, so it will not be listed as bad nor trigger the BAD state.
     */
    QString result_str;
    QTextStream result(&result_str);

    // Remove first (personalities) line
    int index = rawInput.indexOf("\n");
    rawInput.remove(0, index + 1);
    // Remove last (unused devices) line
    index = rawInput.indexOf("unused devices");
    rawInput.remove(index, rawInput.size());
    // Split string into device chunk strings
    QStringList deviceChunks = rawInput.split("\n\n");
    // If there are no RAID devices (deviceChunks only has the original string, hence size of 1),
    // declare MISSING state.
    if (deviceChunks.size() == 1) {
        result << "MISSING";
    }
    else {
        // Declare state and identify bad devices
        bool anyDeviceFailed = false;

        /*
         * NOTE: it seems mdstat prepends "unused devices" with a second "\n" when there is at least one RAID.
         * Since we split rawInput by "\n\n", deviceChunks will contain a "" last element. This is probably
         * not a problem, probably.
         */
        for (QString s : deviceChunks) {
            // Hold device name
            index = s.indexOf(" : ");
            QString name = s.left(index);
            // Look for fail considering RAID type
            bool currDeviceFailed = false;

            // With RAID0, we can only rely on "inactive" and "(F)" or "(E)".
            if (s.contains("inactive") || s.contains("(F)") || s.contains("(E)")) {
                currDeviceFailed = true;
            }
            /*
             * With other types, fields like "[_U]" and "[6/5]"
             * are also available to key on, BUT these should not
             * indicate an emergency when the RAID is recovering.
             */
            if (!currDeviceFailed && !s.contains("raid0") && !s.contains("recovery =")) {
                //QRegularExpression numbers;
                QRegularExpression Us;
                //QRegularExpressionMatch matchNumbers;
                QRegularExpressionMatch matchUs;

                //numbers.setPattern("\\[\\d+\\/\\d+\\]");
                Us.setPattern("\\[[_]+[U]*|[U]*[_]+|[U]*[_]+[U]*\\]");
                //matchNumbers = numbers.match(s);
                matchUs = Us.match(s);
                // The "[_U]"s are easier to check, where an "_" anywhere indicates failure.
                if (matchUs.hasMatch()) {
                    currDeviceFailed = true;
                }
                /*
                 * However, per mdstat's syntax, we'll probably never have a RAID listing that
                 * has a "[2/2]" field but NOT a "[UU]" field right next to it. They're always
                 * paired together. Therefore, checking the numbers is unnecessary.
                 * Still, the code to do it was tested and left here just in case.
                 *
                else {
                    if (matchNumbers.hasMatch()) {
                        // The "[6/5]"s require numeric comparison, so extract numbers.
                        QString wholeStr, leftStr, rightStr;
                        int slashIndex, leftInt, rightInt;
                        bool conversionSuccessful;

                        wholeStr = matchNumbers.captured(0);
                        // Get left number
                        leftStr = wholeStr.remove(0, 1); // Remove "["
                        slashIndex = leftStr.indexOf("/");
                        leftStr.truncate(slashIndex);
                        leftInt = leftStr.toInt(&conversionSuccessful, 10);
                        // It's only worth continuing if we could interpret an int.
                        if (conversionSuccessful) {
                            // Get right number
                            rightStr = wholeStr.remove(wholeStr.size() - 1, 1); // Remove "]"
                            slashIndex = rightStr.indexOf("/");
                            rightStr = wholeStr.right(wholeStr.size() - 1 - slashIndex);
                            rightInt = rightStr.toInt(&conversionSuccessful, 10);
                            if (conversionSuccessful) {
                                // Finally, compare the left and right ints [n/m]
                                if (leftInt > rightInt) {
                                    currDeviceFailed = true;
                                }
                            }
                        }
                    }
                }
                 */
            }
            // Catch failure
            if (currDeviceFailed) {
                // If catching a failure for the first time this round, declare BAD state
                if (!anyDeviceFailed) {
                    anyDeviceFailed = true;
                    result << "BAD";
                }
                // List faulty RAID
                result << "," << name;
            }
        }
        // If no failures were caught, declare GOOD state
        if (!anyDeviceFailed) {
            result << "GOOD";
        }
    }
    processedInput = result.readAll();
}

/***
 * Sets the enum state currState based on processedInput and lists any faulty
 * devices within the notifications displayed in a BAD state
 ***/
void Traylet::setState() {
    if (processedInput.startsWith("GOOD")) {
        currState = GOOD;
    }
    else if (processedInput.startsWith("BAD")) {
        currState = BAD;
        // Specify faulty devices in notification body
        QStringList badDevices = processedInput.split(",");
        // List the bad device(s) (first item is state header, hence size
        // 2 implies ONE faulty device)
        notifyMessage.clear();
        if (badDevices.size() == 2) {
            notifyMessage = "Device: ";
            notifyMessage += badDevices.at(1).toLocal8Bit().constData();
        }
        else if (badDevices.size() > 2) {
            notifyMessage = "Devices: ";
            notifyMessage += badDevices.at(1).toLocal8Bit().constData();
            for (int i = 2; i < badDevices.size(); ++i) {
                notifyMessage += ", ";
                notifyMessage += badDevices.at(i).toLocal8Bit().constData();
            }
        }
        notifyMessage += "\n(Uncheck \"Notify\" to turn off these notifications.)";
    }
    else {
        currState = MISSING;
    }
}

#endif
