/*
 * Copyright (C) 2015 Dominik Haumann <dhaumann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef PLASMA_LLIUREX_UP_INDICATOR_H
#define PLASMA_LLIUREX_UP_INDICATOR_H

#include <QObject>
#include <QProcess>
#include <QPointer>
#include <KNotification>
#include <QDir>
#include <QFile>
#include <QDBusConnection>
#include <QThread>


class QTimer;
class KNotification;
class AsyncDbus;


class LliurexUpIndicator : public QObject
{
    Q_OBJECT


    Q_PROPERTY(TrayStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString toolTip READ toolTip NOTIFY toolTipChanged)
    Q_PROPERTY(QString subToolTip READ subToolTip NOTIFY subToolTipChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY iconNameChanged)
    Q_ENUMS(TrayStatus)

public:
    /**
     * System tray icon states.
     */
    enum TrayStatus {
        ActiveStatus = 0,
        PassiveStatus,
        NeedsAttentionStatus
    };

    LliurexUpIndicator(QObject *parent = nullptr);

    TrayStatus status() const;
    void changeTryIconState (int state);
    void setStatus(TrayStatus status);

    QString toolTip() const;
    void setToolTip(const QString &toolTip);

    QString subToolTip() const;
    void setSubToolTip(const QString &subToolTip);

    QString iconName() const;
    void setIconName(const QString &name);

    bool runUpdateCache();
    bool simulateUpgrade();
    //void isAlive();
    //void updateCache();
   
    void isAlive();

    /**
     * Getter function for the model that is used in QML.
     */

public slots:
    /**
     * Called every timer timeout to update the data model.
     * Launches an asynchronous 'quota' process to obtain data,
     * and finally calls quotaProcessFinished().
     */
    void worker();
    void checkLlxUp();
    void launch_llxup();
    //void UpgradeSystem();

signals:
   
    void statusChanged();
    void toolTipChanged();
    void subToolTipChanged();
    void iconNameChanged();

private:

    AsyncDbus* adbus;
    void plasmoidMode();
    QTimer *m_timer = nullptr;
    QTimer *m_timer_run=nullptr;
    QTimer *m_timer_cache=nullptr;
    TrayStatus m_status = PassiveStatus;
    QString m_iconName = QStringLiteral("lliurexupnotifier");
    QString m_toolTip;
    QString m_subToolTip;
    //QString user;
    QString uid;
    QDir watch_dir;
    QFile new_update;
    QFile llxup_run;
    QFile remote_update;
    QFile TARGET_FILE;
    int FREQUENCY=3600;
    int APT_FRECUENCY=1200;
    bool updatedInfo=false;
    bool remoteUpdateInfo=false;
    bool is_working=false;
    bool is_cache_updated=true;
    bool lliurexUp_running=false;
    bool new_pkg=false;
    int last_check=0;

private slots:

     void updateCache();
     void dbusDone(bool status);
     
};

/**
 * Class monitoring the file system quota.
 * The monitoring is performed through a timer, running the 'quota'
 * command line tool.
 */

class AsyncDbus: public QThread

{

    Q_OBJECT

public:
    
    LliurexUpIndicator* llxindicator;
    
    AsyncDbus(LliurexUpIndicator* lliurexupindicator)
     {
        llxindicator = lliurexupindicator;
     }

     void run()
     {      

        bool result=llxindicator->runUpdateCache();
        emit message(result);

     }
     
signals:

    void message(bool);



};


#endif // PLASMA_LLIUREX_DISK_QUOTA_H
