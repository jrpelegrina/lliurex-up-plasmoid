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
#include "LliurexUpIndicatorUtils.h"

#include <QFile>
#include <QDateTime>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDebug>
#include <QtDBus/QtDBus>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusConnection>
#include <chrono>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>


LliurexUpIndicatorUtils::LliurexUpIndicatorUtils(QObject *parent)
    : QObject(parent)
       
{
    PKGCACHE.setFileName("/var/cache/apt/pkgcache.bin");
  
}    


bool LliurexUpIndicatorUtils::isClient(){

   
    QProcess process;
    process.start("lliurex-version -v");
    process.waitForFinished(-1);
    QString stdout=process.readAllStandardOutput();
    QString stderr=process.readAllStandardError();
    QStringList flavours=stdout.split(QRegExp("\\W+"), QString::SkipEmptyParts);
  
    if (flavours.size()>0){
        QStringList result = flavours.filter("client");
        if (result.size()>0){
            return true;
        }else{
            return false;
        }
    }else{
        return true;
    }    


}


bool LliurexUpIndicatorUtils::getUserGroups(){
    

    int j, ngroups=32;
    gid_t groups[32];
    struct passwd *pw;
    struct group *gr;


    QString user=qgetenv("USER");
    QByteArray uname = user.toLocal8Bit();
    const char *username = uname.data();
    pw=getpwnam(username);
    getgrouplist(username, pw->pw_gid, groups, &ngroups);
    for (j = 0; j < ngroups; j++) {
        gr = getgrgid(groups[j]);
        if (gr != NULL)
        if ((strcmp(gr->gr_name,"adm")==0)||(strcmp(gr->gr_name,"admins")==0)){
            return true;
        
        }    

    }
    return false;
}    


static bool simulateUpgrade(){


	qDebug()<<"simulando upgrade";

    QDBusInterface iface("org.debian.apt",  // from list on left
                     "/org/debian/apt", // from first line of screenshot
                     "org.debian.apt",  // from above Methods
                     QDBusConnection::systemBus());

    if (iface.isValid()){
        qDebug()<<"IFACE valida";
        QDBusMessage result=iface.call("UpgradeSystem",true);
        QString transaction=result.arguments()[0].toString();
        
        QDBusInterface ifaceS("org.debian.apt",  // from list on left
                        transaction, // from first line of screenshot
                        "org.debian.apt.transaction",  // from above Methods
                         QDBusConnection::systemBus());

         
         if (ifaceS.isValid()){
             qDebug()<<"IFACES VALIDA";
             ifaceS.call("Simulate");
          
             QDBusInterface ifaceR("org.debian.apt",  // from list on left
                            transaction, // from first line of screenshot
                            "org.freedesktop.DBus.Properties",  // from above Methods
                             QDBusConnection::systemBus());

             if (ifaceR.isValid()){
                qDebug()<<"IFACER VALIDA";
                QDBusMessage reply = ifaceR.call("Get", "org.debian.apt.transaction", "Dependencies");
                 
                //QList<QVariant> v = reply.arguments();
                //QVariant first = v.at(0);
                //QDBusVariant dbvFirst = first.value<QDBusVariant>();
                //QVariant dbvFirst=reply.arguments()[0].value<QDBusVariant>().variant();
                //QVariant vFirst = dbvFirst.variant();
                const QDBusArgument& dbusArgs=reply.arguments()[0].value<QDBusVariant>().variant().value<QDBusArgument>();
                //const QDBusArgument& dbusArgs = dbvFirst.value<QDBusArgument>();
                 
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

                qDebug()<<"terminando upgrade"<<pkg_list.size();

                if (pkg_list.size()>0){

                    return true;
                }else{
                    return false;
                } 
            }    
        }            

    }
    qDebug()<<"SIMULATE UPGRADE FALSE";
    return false;    

}


bool LliurexUpIndicatorUtils::runUpdateCache(){

    
    qDebug()<<"IS CACHE UPDATED"<<cacheUpdated;

    if (!cacheUpdated){
    	qDebug()<<"Actualizando cache";
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


        if (iface.isValid()){
            QDBusMessage resultU=iface.call("UpdateCache");
            QString transactionU=resultU.arguments()[0].toString();
            
            QDBusInterface ifaceR("org.debian.apt",  // from list on left
                            transactionU, // from first line of screenshot
                            "org.debian.apt.transaction", // from above Methods
                             QDBusConnection::systemBus());


            if (ifaceR.isValid()){

                ifaceR.asyncCall("Run");

                QDBusInterface ifaceS("org.debian.apt",  // from list on left
                                transactionU, // from first line of screenshot
                                "org.freedesktop.DBus.Properties",  // from above Methods
                                 QDBusConnection::systemBus());

                if (ifaceS.isValid()){
                    std::this_thread::sleep_for(std::chrono::seconds(10));

                    int match=0;
                    int cont=0;
                    int timeout=300;

                    while (match==0){
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        QDBusMessage replyS = ifaceS.call("Get", "org.debian.apt.transaction", "ExitState");
                        QString state = replyS.arguments()[0].value<QDBusVariant>().variant().toString();
                        if (state != "exit-unfinished"){
                            match=1;
                        }else{
                            cont=cont+1;
                            if (cont>timeout){
                                return false;
                            }

                        }

                    }

                   
                }    
            }
        }  

    }
    cacheUpdated=false;
    qDebug()<<"lanzando upgrade";
    return simulateUpgrade();
}


bool LliurexUpIndicatorUtils::checkRemote(){

    int cont=0;

    QStringList remote_pts;
    QProcess process;
    QString cmd="who | grep -v \"(:0\"";

    process.start("/bin/sh", QStringList()<< "-c" 
                       << cmd);
    process.waitForFinished(-1);
    QString stdout=process.readAllStandardOutput();
    QString stderr=process.readAllStandardError();
    QStringList remote_users=stdout.split("\n");
    remote_users.takeLast();

    if (remote_users.size()>0){
        QRegularExpression re("pts/\\d+");
        QRegularExpressionMatch match = re.match(remote_users[0]);
        if (match.hasMatch()) {
            QString matched = match.captured(0);
            remote_pts.push_back(matched);
        }  
    }      

    QProcess process2;
    QString cmd2="ps -ef | grep 'lliurex-upgrade' | grep -v 'grep'";
    process2.start("/bin/sh", QStringList()<< "-c" 
                       << cmd2);
    process2.waitForFinished(-1);
    QString stdout2=process2.readAllStandardOutput();
    QString stderr2=process2.readAllStandardError();
    QStringList remote_process=stdout2.split("\n");
    remote_process.takeLast();

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

    if (cont>0){
        return true;

    }else{
        return false;
    }

}  


bool LliurexUpIndicatorUtils::isCacheUpdated(){


    QDateTime currentTime=QDateTime::currentDateTime();
    QDateTime lastModification=QFileInfo(PKGCACHE).lastModified();    

    qint64 millisecondsDiff = lastModification.msecsTo(currentTime);

    if (millisecondsDiff<APT_FRECUENCY){
        cacheUpdated=true;
    }
    return cacheUpdated;

}

