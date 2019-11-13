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
#include "LliurexUpIndicatorUtils.h"

#include <KLocalizedString>
#include <KFormat>
#include <KNotification>
#include <KRun>
#include <QTimer>
#include <QStandardPaths>
#include <QDebug>
#include <QFile>
#include <QThread>


LliurexUpIndicator::LliurexUpIndicator(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    ,m_timer_run(new QTimer(this))
    ,m_timer_cache(new QTimer(this))
    ,m_utils(new LliurexUpIndicatorUtils(this))
    
{
    

    TARGET_FILE.setFileName("/var/run/lliurexUp.lock");
   
    plasmoidMode();

    connect(m_timer, &QTimer::timeout, this, &LliurexUpIndicator::worker);
    m_timer->start(5000);
    worker();
}    


void LliurexUpIndicator::plasmoidMode(){


   if (m_utils->getUserGroups()){
        if(!m_utils->isClient()){
           updatedInfo=true; 
        }

    }
}

void LliurexUpIndicator::worker(){

    qDebug()<<"LAST UPDATE"<<last_update;
    qDebug()<<"LAST CHECK"<<last_check;

    if (!is_working){
        if (LliurexUpIndicator::TARGET_FILE.exists() ) {
            isAlive();
            //isAlive();
        }else{
            if (updatedInfo){
                if (!remoteUpdateInfo){
                    last_update=last_update+5;
                    last_check=last_check+5;
                    if (last_update>FREQUENCY){
                       qDebug()<<"LAST_UPDATE >FRECUENCY";
                       last_update=0;
                       last_check=0;
                       updateCache();
                    }else{   
                        if (last_check>1200){
                            last_check=0;
                            if (m_utils->isCacheUpdated()){
                                qDebug()<<"last_check >1200";
                                last_update=0;
                                updateCache();
                            }

                        }    
                    }
                       
                }
            }
        }
    }

    

}    

void LliurexUpIndicator::updateCache(){

    is_working=true;

    adbus=new AsyncDbus(this);
    QObject::connect(adbus,SIGNAL(message(bool)),this,SLOT(dbusDone(bool)));
    adbus->start();

}

bool LliurexUpIndicator::runUpdateCache(){

    return m_utils->runUpdateCache();

}

void LliurexUpIndicator::dbusDone(bool result){

    is_working=false;
        
    adbus->exit(0);
    if (adbus->wait()){
        delete adbus;
    }
    qDebug()<< "RESULT UPGRADE"<<result;
    if (result){
        changeTryIconState(0);
    }

}    


void LliurexUpIndicator::isAlive(){

    is_working=true;
    remoteUpdateInfo=true;

    if (m_utils->checkRemote()){
        changeTryIconState(2);
    }else{
        changeTryIconState(1);
    }

    connect(m_timer_run, &QTimer::timeout, this, &LliurexUpIndicator::checkLlxUp);
    m_timer_run->start(5000);
    checkLlxUp();


}

void LliurexUpIndicator::checkLlxUp(){


    if (!LliurexUpIndicator::TARGET_FILE.exists()){
        m_timer_run->stop();
        is_working=false;
        remoteUpdateInfo=false;
        changeTryIconState(1);
          
    } 

}



LliurexUpIndicator::TrayStatus LliurexUpIndicator::status() const
{
    return m_status;
}


void LliurexUpIndicator::changeTryIconState(int state){

    const QString tooltip(i18n("Lliurex-Up"));
    if (state==0){
        setStatus(ActiveStatus);
        const QString subtooltip(i18n("There are new packages ready to be updated or installed"));
        setToolTip(tooltip);
        setSubToolTip(subtooltip);
        setIconName("lliurexupnotifier");
        /*
        KNotification *notification = new KNotification(QStringLiteral("notification"), KNotification::CloseOnTimeout | KNotification::DefaultEvent);
        notification->setIconName(QStringLiteral("lliurex-up-indicator"));
        notification->setTitle(tooltip);
        notification->setText(subtooltip);
        const QString name = i18n("Update now");
        notification->setActions({name});
        connect(notification, &KNotification::action1Activated, this, &LliurexUpIndicator::launch_llxup);
        notification->sendEvent();
        */
        m_updatesAvailableNotification = KNotification::event(QStringLiteral("Update"), subtooltip, {}, "lliurex-up-indicator", nullptr, KNotification::CloseOnTimeout , QStringLiteral("llxupnotifier"));
        const QString name = i18n("Update now");
        m_updatesAvailableNotification->setDefaultAction(name);
        m_updatesAvailableNotification->setActions({name});
        connect(m_updatesAvailableNotification, QOverload<unsigned int>::of(&KNotification::activated), this, &LliurexUpIndicator::launch_llxup);


    }else if (state==2){
        setStatus(NeedsAttentionStatus);
        const QString subtooltip(i18n("Lliurex-Up is being executed"));
        setToolTip(tooltip);
        setSubToolTip(subtooltip);
        setIconName("lliurexupnotifier-running");
        /*
        KNotification *notification = new KNotification(QStringLiteral("remoteUpdate"), KNotification::CloseOnTimeout | KNotification::DefaultEvent);
        notification->setIconName(QStringLiteral("lliurex-up-indicator"));
        notification->setTitle(tooltip);
        notification->setText(subtooltip);
        notification->addContext( "llxupabstractnotifier");
        notification->sendEvent();*/
        m_remoteUpdateNotification = KNotification::event(QStringLiteral("remoteUpdate"), subtooltip, {}, "lliurex-up-indicator", nullptr, KNotification::CloseOnTimeout , QStringLiteral("llxupnotifier"));
        //const QString name = i18n("Update now");
        //m_updatesAvailableNotification->setDefaultAction(name);
        //m_updatesAvailableNotification->setActions({name});
        //connect(m_updatesAvailableNotification, QOverload<unsigned int>::of(&KNotification::activated), this, &LliurexUpIndicator::launch_llxup);
        //notification->sendEvent();

    }else{
        setStatus(PassiveStatus);
    }
    


}

void LliurexUpIndicator::launch_llxup()
{
    if (m_status==0){
        KRun::runCommand(QStringLiteral("pkexec lliurex-up"), nullptr);
        if (m_updatesAvailableNotification) { m_updatesAvailableNotification->close(); }
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



