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
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <QThread>
#include <QRegularExpression>


LliurexUpIndicator::LliurexUpIndicator(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    ,m_timer_run(new QTimer(this))
    ,m_timer_cache(new QTimer(this))
    
{
    

    
    uid=qgetenv("KDE_SESSION_UID");
    watch_dir="/var/run/user/"+uid+"/llxup_plasmoid";
    TARGET_FILE.setFileName("/var/run/lliurexUp.lock");
   
    plasmoidMode();
    //getUserGroups(); .
    qDebug()<<"MODE";
    qDebug()<<updatedInfo;

    QString update_token_path=watch_dir.absoluteFilePath("new_update_token");
    new_update.setFileName(update_token_path);
    QString llxup_token_path=watch_dir.absoluteFilePath("llxup_run_token");
    llxup_run.setFileName(llxup_token_path);
    QString remote_token_path=watch_dir.absoluteFilePath("remote_update_token");
    remote_update.setFileName(remote_token_path);

   /*
    if (LliurexUpIndicator::TARGET_FILE.exists()){
        lliurexUp_running=true;
    }
    */
    
    connect(m_timer, &QTimer::timeout, this, &LliurexUpIndicator::worker);
    m_timer->start(5000);
    worker();
}    


static bool isClient(){

    QProcess process;
    process.start("lliurex-version -v");
    process.waitForFinished(-1);
    QString stdout=process.readAllStandardOutput();
    QString stderr=process.readAllStandardError();
    qDebug()<<stdout;
    qDebug()<<stderr;
    QStringList flavours=stdout.split(QRegExp("\\W+"), QString::SkipEmptyParts);
    qDebug()<<flavours;
    QStringList result = flavours.filter("client");
    qDebug()<<result;
    if (result.size()>0){
        return true;
    }else{
        return false;
    }


}


static bool getUserGroups(){
    

    int j, ngroups=32;
    gid_t groups[32];
    struct passwd *pw;
    struct group *gr;

    QString user=qgetenv("USER");
    QByteArray ba = user.toLocal8Bit();
    const char *username = ba.data();
    pw=getpwnam(username);
    getgrouplist(username, pw->pw_gid, groups, &ngroups);
    for (j = 0; j < ngroups; j++) {
        qDebug()<<groups[j];
        gr = getgrgid(groups[j]);
        if (gr != NULL)
        qDebug()<<gr->gr_name;
        if ((strcmp(gr->gr_name,"adm")==0)||(strcmp(gr->gr_name,"admins")==0)){
            return true;
        
        }    

    }
    return false;
}    


void LliurexUpIndicator::plasmoidMode(){

    qDebug()<<"PlasmoidMode";

    bool adminGroup=getUserGroups();
    qDebug()<<"#####";
    qDebug()<<adminGroup;
    if (adminGroup){
        qDebug()<<"El usurio es admin";
        if(!isClient()){
            qDebug()<<"No es client";
           updatedInfo=true; 
        }

    }
    qDebug()<<"Y el modo es";
    qDebug()<<updatedInfo;
}

void LliurexUpIndicator::worker(){

    qDebug()<<is_working;
    qDebug()<<is_cache_updated;

    if ( !is_working){
        qDebug()<<"No trabajo";
        if (LliurexUpIndicator::TARGET_FILE.exists() ) {
            isAlive();
            qDebug()<<"En marcha";
            //isAlive();
        }else{
            qDebug()<<"Ver cache";
            if (updatedInfo){
                if (!remoteUpdateInfo){
                    if (is_cache_updated){
                        last_check=0;
                        updateCache();
                    }else{
                        last_check=last_check+5;
                        if (last_check>FREQUENCY){
                            last_check=0;
                            updateCache();

                        }
                    }
                }
            }
        }

    }else{
        qDebug()<<"ocupado";
    }


}    


bool LliurexUpIndicator::simulateUpgrade(){


    QDBusInterface iface("org.debian.apt",  // from list on left
                     "/org/debian/apt", // from first line of screenshot
                     "org.debian.apt",  // from above Methods
                     QDBusConnection::systemBus());


    QDBusMessage result=iface.call("UpgradeSystem",true);
    qDebug()<<result;
    QString transaction=result.arguments()[0].toString();
    
    QDBusInterface ifaces("org.debian.apt",  // from list on left
                    transaction, // from first line of screenshot
                    "org.debian.apt.transaction",  // from above Methods
                     QDBusConnection::systemBus());

     
     ifaces.call("Simulate");
  
     QDBusInterface iface3("org.debian.apt",  // from list on left
                    transaction, // from first line of screenshot
                    "org.freedesktop.DBus.Properties",  // from above Methods
                     QDBusConnection::systemBus());


     QDBusMessage reply = iface3.call("Get", "org.debian.apt.transaction", "Dependencies");
     qDebug()<<reply;
     
     QList<QVariant> v = reply.arguments();
     QVariant first = v.at(0);
     QDBusVariant dbvFirst = first.value<QDBusVariant>();
     QVariant vFirst = dbvFirst.variant();
     const QDBusArgument& dbusArgs = vFirst.value<QDBusArgument>();
     qDebug() << "QDBusArgument current type is" << dbusArgs.currentType();
     
     QVariantList pkg_list;

     dbusArgs.beginStructure();

     int cont=0;
     while(!dbusArgs.atEnd()) {
                cont=cont+1;
                if (cont<6){
                    dbusArgs.beginArray();
                    while(!dbusArgs.atEnd()) {
                        QString s;
                        dbusArgs>>s;
                        pkg_list.push_back(s);
                    } 
                 }      

                dbusArgs.endArray();
          
           
        }       
    
    dbusArgs.endStructure();

    qDebug()<<pkg_list;

    qDebug()<<pkg_list.size();

    if (pkg_list.size()>0){
        return true;
    }else{
        return false;
    }         



}


bool LliurexUpIndicator::runUpdateCache(){

    //is_working=true;

    if (!QDBusConnection::sessionBus().isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus session bus.\n"
                "To start it, run:\n"
                "\teval `dbus-launch --auto-syntax`\n");
        return false;
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

    ifacesU.asyncCall("Run");
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


    /*connect(m_timer, &QTimer::timeout, this, &LliurexUpIndicator::listDirectoryContents);
    m_timer->start(3000);*/


    return simulateUpgrade();
         
}

void LliurexUpIndicator::updateCache(){

    is_working=true;

    adbus=new AsyncDbus(this);
    QObject::connect(adbus,SIGNAL(message(bool)),this,SLOT(dbusDone(bool)));
    adbus->start();

}

void LliurexUpIndicator::dbusDone(bool result){

    qDebug()<<"Terminando";
    if (result){

        changeTryIconState(0);
    }

    adbus->exit(0);
    if (adbus->wait()){
        delete adbus;

    }
    is_working=false;
    is_cache_updated=false;


}    


static bool checkRemote(){

    int cont=0;
    QStringList remote_pts;
    QProcess process;
    QString cmd="who | grep -v \"(:0\"";
    process.start("/bin/sh", QStringList()<< "-c" 
                       << cmd);
    process.waitForFinished(-1);
    QString stdout=process.readAllStandardOutput();
    QString stderr=process.readAllStandardError();
    qDebug()<<"Salida";
    qDebug()<<stdout;
    qDebug()<<"error";
    qDebug()<<stderr;
    QStringList remote_users=stdout.split("\n");
    qDebug()<<cmd;
    qDebug()<<"LISTA de usuarios";
    remote_users.takeLast();
    qDebug()<<remote_users;
    if (remote_users.size()>0){
        QRegularExpression re("pts/\\d+");
        QRegularExpressionMatch match = re.match(remote_users[0]);
        if (match.hasMatch()) {

            QString matched = match.captured(0);
            qDebug()<<"encontrado";
            qDebug()<<matched; 
            remote_pts.push_back(matched);
        }    
    }    
    qDebug()<<remote_pts;

    QProcess process2;
    QString cmd2="ps -ef | grep 'lliurex-upgrade' | grep -v 'grep'";
    process2.start("/bin/sh", QStringList()<< "-c" 
                       << cmd2);
    process2.waitForFinished(-1);
    QString stdout2=process2.readAllStandardOutput();
    QString stderr2=process2.readAllStandardError();
    qDebug()<<"Salida";
    qDebug()<<stdout2;
    qDebug()<<"error";
    qDebug()<<stderr2;
    QStringList remote_process=stdout2.split("\n");
    qDebug()<<"LISTA de procesos";
    remote_process.takeLast();
    qDebug()<<remote_process;

    for(int i=0 ; i < remote_process.length() ; i++){

       if (remote_process[i].contains("?")){

           cont=cont+1;
       } // -> True

       for (int j=0; j<remote_pts.length();j++){

            if (remote_process[i].contains(remote_pts[j])){
                cont=cont+1;

            }

       }

    }
    qDebug()<<cont;

    if (cont>0){
        return true;

    }else{
        return false;

    }

}    

void LliurexUpIndicator::isAlive(){

    is_working=true;
    remoteUpdateInfo=true;

    if (checkRemote()){
        changeTryIconState(2);
    }else{
        changeTryIconState(1);

    }    
    connect(m_timer_run, &QTimer::timeout, this, &LliurexUpIndicator::checkLlxUp);
    m_timer_run->start(3000);
    checkLlxUp();


}

void LliurexUpIndicator::checkLlxUp(){

    qDebug()<<"LLX up en marcha";

    if (!LliurexUpIndicator::TARGET_FILE.exists()){
        qDebug()<<"parando";
        m_timer_run->stop();
        changeTryIconState(1);
        is_working=false;
        //lliurexUp_running=false;
        remoteUpdateInfo=false;
        is_cache_updated=false;
       
    } 

}



LliurexUpIndicator::TrayStatus LliurexUpIndicator::status() const
{
    return m_status;
}


void LliurexUpIndicator::changeTryIconState(int state){

    const QString tooltip(i18n("Lliurex-Up"));
    qDebug()<<"Cambiar estado";
    if (state==0){
        qDebug()<<"Listo";
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
        qDebug()<<"Cambiando";
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



