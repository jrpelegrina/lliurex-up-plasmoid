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
#include "LliurexUpIndicator.h"

#include <KLocalizedString>
#include <KFormat>
#include <KNotification>
#include <KRun>
#include <QTimer>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QtDBus/QtDBus>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusConnection>
#include <thread>
#include <chrono>



LliurexUpIndicator::LliurexUpIndicator(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    
{
    

    user=qgetenv("USER");
    uid=qgetenv("KDE_SESSION_UID");
    watch_dir="/var/run/user/"+uid+"/llxup_plasmoid";
    QString update_token_path=watch_dir.absoluteFilePath("new_update_token");
    new_update.setFileName(update_token_path);
    QString llxup_token_path=watch_dir.absoluteFilePath("llxup_run_token");
    llxup_run.setFileName(llxup_token_path);
    QString remote_token_path=watch_dir.absoluteFilePath("remote_update_token");
    remote_update.setFileName(remote_token_path);


    if (!QDBusConnection::sessionBus().isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus session bus.\n"
                "To start it, run:\n"
                "\teval `dbus-launch --auto-syntax`\n");
        
    }
    
      
    QDBusInterface iface("org.debian.apt",  // from list on left
                     "/org/debian/apt", // from first line of screenshot
                     "org.debian.apt",  // from above Methods
                     QDBusConnection::systemBus());


   
    QDBusMessage resultU=iface.call("UpdateCache");
    qDebug()<<"1";
    qDebug()<<resultU;
    QString transactionU=resultU.arguments()[0].toString();
    QDBusInterface ifacesU("org.debian.apt",  // from list on left
                    transactionU, // from first line of screenshot
                    "org.debian.apt.transaction", // from above Methods
                     QDBusConnection::systemBus());

    //QDBusConnection::connect("", "",&ifacesU,"Finished",this,SLOT(listDirectoryContents()));
    //QDBusConnection::connect("", "","org.debian.apt","Finished",this,SLOT(listDirectoryContents()));
   
    ifacesU.call("Run");
    //connect(&ifacesU,SIGNAL(Finished(QString)),this,SLOT(listDirectoryContents()));
    //connect(&ifacesU,SIGNAL(PropertyChanged(QString,QDBusVariant)),this,SLOT(listDirectoryContents()));
    
    qDebug()<<"Waiting";
    QDBusInterface ifaceU1("org.debian.apt",  // from list on left
                    transactionU, // from first line of screenshot
                    "org.freedesktop.DBus.Properties",  // from above Methods
                     QDBusConnection::systemBus());


    std::this_thread::sleep_for(std::chrono::seconds(10));
    //QDBusMessage replyU = ifaceU1.call("Get", "org.debian.apt.transaction", "ExitState");
    //QString state = replyU.arguments()[0].value<QDBusVariant>().variant().toString();
   
    int match=0;
    
    while (match==0){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        QDBusMessage replyU = ifaceU1.call("Get", "org.debian.apt.transaction", "ExitState");
        QString state = replyU.arguments()[0].value<QDBusVariant>().variant().toString();
        if (state != "exit-unfinished"){
            match=1;
        }

    }
    qDebug()<<replyU;


    /*connect(m_timer, &QTimer::timeout, this, &LliurexUpIndicator::listDirectoryContents);
    m_timer->start(3000);*/


    
   
    listDirectoryContents();
   
}


LliurexUpIndicator::TrayStatus LliurexUpIndicator::status() const
{
    return m_status;
}

void LliurexUpIndicator::listDirectoryContents(){

    
    /*QDir watch_dir("/tmp/.llxup_plasmoid");
    QFile new_update("/tmp/.llxup_plasmoid/new_update_token");
    QFile llxup_run("/tmp/.llxup_plasmoid/llxup_run_token");
    QFile remote_update("/tmp/.llxup_plasmoid/remote_update_token");*/
   
     
    bool changeDetect=false;
    int stateToSet=1;

    qDebug()<<"Llamando";

    if (LliurexUpIndicator::watch_dir.exists()){
        if (!LliurexUpIndicator::watch_dir.isEmpty()){
            if (LliurexUpIndicator::new_update.exists()){
                if (m_status!=0){
                    changeDetect=true;
                    stateToSet=0;
                    /*changesTryIconState(0);*/
                }
            }else if (LliurexUpIndicator::llxup_run.exists()){ 
                    if (LliurexUpIndicator::remote_update.exists()){
                        if (m_status!=2){
                            changeDetect=true;
                            stateToSet=2;
                            /*changesTryIconState(2);*/
                        }
                    }else{
                        changeDetect=true;
                        /*changesTryIconState(1);*/
                   } 
            }          
           /* }else{
                changesTryIconState(1);
            }*/
        }else{
            if (m_status!=1){
                changeDetect=true;
                /*changesTryIconState(1);*/
            }
        }
    }else{
        if (m_status!=1){
            changeDetect=true;
            
            /*changesTryIconState(1);*/
         }
    } 

    if (changeDetect){
        changeTryIconState(stateToSet);
    }       
       
}        

void LliurexUpIndicator::changeTryIconState(int state){

    const QString tooltip(i18n("Lliurex-Up"));

    if (state==0){
        setStatus(ActiveStatus);
        const QString subtooltip(i18n("There are new packages ready to be updated or installed"));
        setToolTip(tooltip);
        setSubToolTip(subtooltip);
        setIconName("lliurexupnotifier");
        KNotification *notification = new KNotification(QStringLiteral("notification"),KNotification::CloseOnTimeout | KNotification::DefaultEvent);
        notification->setIconName(QStringLiteral("lliurex-up-indicator"));
        notification->setTitle(tooltip);
        notification->setText(subtooltip);
        const QString name = i18n("Update now");
        notification->setActions({name});
        connect(notification, &KNotification::action1Activated, this, &LliurexUpIndicator::launch_llxup);
        notification->sendEvent();

    }else if (state==2){
        setStatus(NeedsAttentionStatus);
        const QString subtooltip(i18n("Lliurex-Up is being executed"));
        setToolTip(tooltip);
        setSubToolTip(subtooltip);
        setIconName("lliurexupnotifier-running");
        KNotification *notification = new KNotification(QStringLiteral("notification"), KNotification::CloseOnTimeout | KNotification::DefaultEvent);
        notification->setIconName(QStringLiteral("lliurex-up-indicator"));
        notification->setTitle(tooltip);
        notification->setText(subtooltip);
        notification->sendEvent();

    }else{
        setStatus(PassiveStatus);
    }
    


}

void LliurexUpIndicator::launch_llxup()
{
    if (m_status==0){
        KRun::runCommand(QStringLiteral("pkexec lliurex-up"), nullptr);
    }    
   
}


void LliurexUpIndicator::setStatus(LliurexUpIndicator::TrayStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged();
    }
}

QString LliurexUpIndicator::iconName() const
{
    return m_iconName;
}

void LliurexUpIndicator::setIconName(const QString &name)
{
    if (m_iconName != name) {
        m_iconName = name;
        emit iconNameChanged();
    }
}

QString LliurexUpIndicator::toolTip() const
{
    return m_toolTip;
}

void LliurexUpIndicator::setToolTip(const QString &toolTip)
{
    if (m_toolTip != toolTip) {
        m_toolTip = toolTip;
        emit toolTipChanged();
    }
}

QString LliurexUpIndicator::subToolTip() const
{
    return m_subToolTip;
}

void LliurexUpIndicator::setSubToolTip(const QString &subToolTip)
{
    if (m_subToolTip != subToolTip) {
        m_subToolTip = subToolTip;
        emit subToolTipChanged();
    }
}



