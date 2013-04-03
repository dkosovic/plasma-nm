/*
    Copyright 2013 Jan Grulich <jgrulich@redhat.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "connectiondetaileditor.h"
#include "ui_connectiondetaileditor.h"
#include "connectionwidget.h"
#include "wiredconnectionwidget.h"
#include "wificonnectionwidget.h"
#include "ipv4widget.h"
#include "ipv6widget.h"
#include "wifisecurity.h"
#include "wiredsecurity.h"
#include "pppwidget.h"
#include "pppoewidget.h"
#include "gsmwidget.h"
#include "cdmawidget.h"
#include "btwidget.h"
#include "vpnuiplugin.h"

#include <QtNetworkManager/settings.h>
#include <QtNetworkManager/activeconnection.h>
#include <QtNetworkManager/connection.h>
#include <QtNetworkManager/settings/802-11-wireless.h>
#include <QtNetworkManager/settings/vpn.h>
#include <QtNetworkManager/generic-types.h>

#include <KPluginFactory>
#include <KServiceTypeTrader>

using namespace NetworkManager;

ConnectionDetailEditor::ConnectionDetailEditor(Settings::ConnectionSettings::ConnectionType type, QWidget* parent, Qt::WindowFlags f):
    QDialog(parent, f),
    m_detailEditor(new Ui::ConnectionDetailEditor),
    m_connection(new NetworkManager::Settings::ConnectionSettings(type)),
    m_numSecrets(0),
    m_new(true)
{
    m_detailEditor->setupUi(this);

    initEditor();
}


ConnectionDetailEditor::ConnectionDetailEditor(Settings::ConnectionSettings* connection, QWidget* parent, Qt::WindowFlags f):
    QDialog(parent, f),
    m_detailEditor(new Ui::ConnectionDetailEditor),
    m_connection(connection),
    m_numSecrets(0),
    m_new(false)
{
    m_detailEditor->setupUi(this);

    initEditor();
}

ConnectionDetailEditor::~ConnectionDetailEditor()
{
}

void ConnectionDetailEditor::initEditor()
{
    qDBusRegisterMetaType<QStringMap>();

    if (!m_new) {
        NetworkManager::Settings::Connection * connection = NetworkManager::Settings::findConnectionByUuid(m_connection->uuid());
        if (connection) {
            connect(connection, SIGNAL(gotSecrets(QString,bool,QVariantMapMap,QString)),
                    SLOT(gotSecrets(QString,bool, QVariantMapMap, QString)));

            switch (m_connection->connectionType()) {
                case Settings::ConnectionSettings::Adsl:
                    connection->secrets("adsl");
                    m_numSecrets = 1;
                    break;
                case Settings::ConnectionSettings::Bluetooth:
                    connection->secrets("gsm");
                    m_numSecrets = 1;
                    break;
                case Settings::ConnectionSettings::Cdma:
                    connection->secrets("cdma");
                    m_numSecrets = 1;
                    break;
                case Settings::ConnectionSettings::Gsm:
                    connection->secrets("gsm");
                    m_numSecrets = 1;
                    break;
                case Settings::ConnectionSettings::Pppoe:
                    connection->secrets("pppoe");
                    m_numSecrets = 1;
                    break;
                case Settings::ConnectionSettings::Wired:
                    connection->secrets("802-1x");
                    m_numSecrets = 1;
                    break;
                case Settings::ConnectionSettings::Wireless:
                    connection->secrets("802-1x");
                    connection->secrets("802-11-wireless-security");
                    m_numSecrets = 2;
                    break;
                case Settings::ConnectionSettings::Vpn:
                    connection->secrets("vpn");
                    m_numSecrets = 1;
                    break;
                default:
                    initTabs();
                    break;
            }
        }
    } else {
        initTabs();
    }

    if (m_connection->id().isEmpty()) {
        setWindowTitle(i18n("New Connection (%1)", m_connection->typeAsString(m_connection->connectionType())));
        m_detailEditor->connectionName->setText(i18n("New %1 connection", m_connection->typeAsString(m_connection->connectionType())));
    } else {
        setWindowTitle(i18n("Edit Connection '%1'", m_connection->id()));
        m_detailEditor->connectionName->setText(m_connection->id());
    }

    connect(this, SIGNAL(accepted()), SLOT(saveSetting()));
}

void ConnectionDetailEditor::initTabs()
{
    ConnectionWidget * connectionWidget = new ConnectionWidget(m_connection);
    m_detailEditor->tabWidget->addTab(connectionWidget, i18n("General"));

    const NetworkManager::Settings::ConnectionSettings::ConnectionType type = m_connection->connectionType();

    /*Adsl, Cdma, Gsm, Infiniband, Ipv4, Ipv6, Ppp, Pppoe, Security8021x, Serial,
      Vpn, Wired, Wireless, WirelessSecurity, Bluetooth, OlpcMesh, Vlan, Wimax, Bond, Bridge, BridgePort;*/
    if (type == NetworkManager::Settings::ConnectionSettings::Wired) {
        WiredConnectionWidget * wiredWidget = new WiredConnectionWidget(m_connection->setting(NetworkManager::Settings::Setting::Wired), this);
        m_detailEditor->tabWidget->addTab(wiredWidget, i18n("Wired"));
        WiredSecurity * wiredSecurity = new WiredSecurity(static_cast<NetworkManager::Settings::Security8021xSetting *>(m_connection->setting(NetworkManager::Settings::Setting::Security8021x)), this);
        m_detailEditor->tabWidget->addTab(wiredSecurity, i18n("802.1x Security"));
        IPv4Widget * ipv4Widget = new IPv4Widget(m_connection->setting(NetworkManager::Settings::Setting::Ipv4), this);
        m_detailEditor->tabWidget->addTab(ipv4Widget, i18n("IPv4"));
        IPv6Widget * ipv6Widget = new IPv6Widget(m_connection->setting(NetworkManager::Settings::Setting::Ipv6), this);
        m_detailEditor->tabWidget->addTab(ipv6Widget, i18n("IPv6"));
    } else if (type == NetworkManager::Settings::ConnectionSettings::Wireless) {
        WifiConnectionWidget * wifiWidget = new WifiConnectionWidget(m_connection->setting(NetworkManager::Settings::Setting::Wireless), this);
        m_detailEditor->tabWidget->addTab(wifiWidget, i18n("Wireless"));
        WifiSecurity * wifiSecurity = new WifiSecurity(m_connection->setting(NetworkManager::Settings::Setting::WirelessSecurity),
                                                       static_cast<NetworkManager::Settings::Security8021xSetting *>(m_connection->setting(NetworkManager::Settings::Setting::Security8021x)),
                                                       this);
        m_detailEditor->tabWidget->addTab(wifiSecurity, i18n("Wi-Fi Security"));
        IPv4Widget * ipv4Widget = new IPv4Widget(m_connection->setting(NetworkManager::Settings::Setting::Ipv4), this);
        m_detailEditor->tabWidget->addTab(ipv4Widget, i18n("IPv4"));
        IPv6Widget * ipv6Widget = new IPv6Widget(m_connection->setting(NetworkManager::Settings::Setting::Ipv6), this);
        m_detailEditor->tabWidget->addTab(ipv6Widget, i18n("IPv6"));
    } else if (type == NetworkManager::Settings::ConnectionSettings::Pppoe) { // DSL
        PppoeWidget * pppoeWidget = new PppoeWidget(m_connection->setting(NetworkManager::Settings::Setting::Pppoe), this);
        m_detailEditor->tabWidget->addTab(pppoeWidget, i18n("DSL"));
        WiredConnectionWidget * wiredWidget = new WiredConnectionWidget(m_connection->setting(NetworkManager::Settings::Setting::Wired), this);
        m_detailEditor->tabWidget->addTab(wiredWidget, i18n("Wired"));
        PPPWidget * pppWidget = new PPPWidget(m_connection->setting(NetworkManager::Settings::Setting::Ppp), this);
        m_detailEditor->tabWidget->addTab(pppWidget, i18n("PPP"));
        IPv4Widget * ipv4Widget = new IPv4Widget(m_connection->setting(NetworkManager::Settings::Setting::Ipv4), this);
        m_detailEditor->tabWidget->addTab(ipv4Widget, i18n("IPv4"));
    } else if (type == NetworkManager::Settings::ConnectionSettings::Gsm) { // GSM
        GsmWidget * gsmWidget = new GsmWidget(m_connection->setting(NetworkManager::Settings::Setting::Gsm), this);
        m_detailEditor->tabWidget->addTab(gsmWidget, i18n("Mobile Broadband"));
        PPPWidget * pppWidget = new PPPWidget(m_connection->setting(NetworkManager::Settings::Setting::Ppp), this);
        m_detailEditor->tabWidget->addTab(pppWidget, i18n("PPP"));
        IPv4Widget * ipv4Widget = new IPv4Widget(m_connection->setting(NetworkManager::Settings::Setting::Ipv4), this);
        m_detailEditor->tabWidget->addTab(ipv4Widget, i18n("IPv4"));
    } else if (type == NetworkManager::Settings::ConnectionSettings::Cdma) { // CDMA
        CdmaWidget * cdmaWidget = new CdmaWidget(m_connection->setting(NetworkManager::Settings::Setting::Cdma), this);
        m_detailEditor->tabWidget->addTab(cdmaWidget, i18n("Mobile Broadband"));
        PPPWidget * pppWidget = new PPPWidget(m_connection->setting(NetworkManager::Settings::Setting::Ppp), this);
        m_detailEditor->tabWidget->addTab(pppWidget, i18n("PPP"));
        IPv4Widget * ipv4Widget = new IPv4Widget(m_connection->setting(NetworkManager::Settings::Setting::Ipv4), this);
        m_detailEditor->tabWidget->addTab(ipv4Widget, i18n("IPv4"));
    } else if (type == NetworkManager::Settings::ConnectionSettings::Bluetooth) {
        BtWidget * btWidget = new BtWidget(m_connection->setting(NetworkManager::Settings::Setting::Bluetooth), this);
        m_detailEditor->tabWidget->addTab(btWidget, i18n("Bluetooth"));
        NetworkManager::Settings::BluetoothSetting * btSetting = static_cast<NetworkManager::Settings::BluetoothSetting *>(m_connection->setting(NetworkManager::Settings::Setting::Bluetooth));
        if (btSetting->profileType() == NetworkManager::Settings::BluetoothSetting::Dun) {
            GsmWidget * gsmWidget = new GsmWidget(m_connection->setting(NetworkManager::Settings::Setting::Gsm), this);
            m_detailEditor->tabWidget->addTab(gsmWidget, i18n("GSM"));
            PPPWidget * pppWidget = new PPPWidget(m_connection->setting(NetworkManager::Settings::Setting::Ppp), this);
            m_detailEditor->tabWidget->addTab(pppWidget, i18n("PPP"));
            // TODO serial setting?
        }
        IPv4Widget * ipv4Widget = new IPv4Widget(m_connection->setting(NetworkManager::Settings::Setting::Ipv4), this);
        m_detailEditor->tabWidget->addTab(ipv4Widget, i18n("IPv4"));
        IPv6Widget * ipv6Widget = new IPv6Widget(m_connection->setting(NetworkManager::Settings::Setting::Ipv6), this);
        m_detailEditor->tabWidget->addTab(ipv6Widget, i18n("IPv6"));
    } else if (type == NetworkManager::Settings::ConnectionSettings::Vpn) {
        QString error;
        VpnUiPlugin * vpnPlugin = 0;
        NetworkManager::Settings::VpnSetting *vpnSetting =
                static_cast<NetworkManager::Settings::VpnSetting*>(m_connection->setting(NetworkManager::Settings::Setting::Vpn));
        if (!vpnSetting) {
            qDebug() << "Missing VPN setting!";
        } else {
            const QString serviceType = vpnSetting->serviceType();
            qDebug() << "Loading VPN plugin" << serviceType;
            //vpnSetting->printSetting();
            vpnPlugin = KServiceTypeTrader::createInstanceFromQuery<VpnUiPlugin>(QString::fromLatin1("PlasmaNM/VpnUiPlugin"),
                                                                                 QString::fromLatin1("[X-NetworkManager-Services]=='%1'").arg(serviceType),
                                                                                 this, QVariantList(), &error);
            if (vpnPlugin && error.isEmpty()) {
                const QString shortName = serviceType.section('.', -1);
                SettingWidget * vpnWidget = vpnPlugin->widget(vpnSetting, this);
                m_detailEditor->tabWidget->addTab(vpnWidget, i18n("VPN (%1)", shortName));
                IPv4Widget * ipv4Widget = new IPv4Widget(m_connection->setting(NetworkManager::Settings::Setting::Ipv4), this);
                m_detailEditor->tabWidget->addTab(ipv4Widget, i18n("IPv4"));
            } else {
                qDebug() << error << ", serviceType == " << serviceType;
            }
        }
    }
}

void ConnectionDetailEditor::saveSetting()
{
    ConnectionWidget * connectionWidget = static_cast<ConnectionWidget*>(m_detailEditor->tabWidget->widget(0));

    QVariantMapMap settings = connectionWidget->setting();

    for (int i = 1; i < m_detailEditor->tabWidget->count(); ++i) {
        SettingWidget * widget = static_cast<SettingWidget*>(m_detailEditor->tabWidget->widget(i));
        const QString type = widget->type();
        settings.insert(type, widget->setting());

        QVariantMap security8021x;
        if (type == NetworkManager::Settings::Setting::typeAsString(NetworkManager::Settings::Setting::WirelessSecurity)) {
            security8021x = static_cast<WifiSecurity *>(widget)->setting8021x();
        } else if (type == NetworkManager::Settings::Setting::typeAsString(NetworkManager::Settings::Setting::Security8021x)) {
            security8021x = static_cast<WiredSecurity *>(widget)->setting();
        }

        if (!security8021x.isEmpty()) {
            settings.insert(NetworkManager::Settings::Setting::typeAsString(NetworkManager::Settings::Setting::Security8021x), security8021x);
        }
    }

    NetworkManager::Settings::ConnectionSettings * connectionSettings = new NetworkManager::Settings::ConnectionSettings(m_connection->connectionType());

    connectionSettings->fromMap(settings);
    connectionSettings->setId(m_detailEditor->connectionName->text());

    if (connectionSettings->connectionType() == Settings::ConnectionSettings::Wireless) {
        NetworkManager::Settings::WirelessSecuritySetting * securitySetting = static_cast<NetworkManager::Settings::WirelessSecuritySetting*>(connectionSettings->setting(Settings::Setting::WirelessSecurity));
        NetworkManager::Settings::WirelessSetting * wirelessSetting = static_cast<NetworkManager::Settings::WirelessSetting*>(connectionSettings->setting(Settings::Setting::Wireless));

        if (securitySetting && wirelessSetting) {
            if (securitySetting->keyMgmt() != NetworkManager::Settings::WirelessSecuritySetting::WirelessSecuritySetting::Unknown) {
                wirelessSetting->setSecurity("802-11-wireless-security");
            }
        }
    }

    if (m_new) {
        connectionSettings->setUuid(NetworkManager::Settings::ConnectionSettings::createNewUuid());
    } else {
        connectionSettings->setUuid(m_connection->uuid());
    }

    connectionSettings->printSetting();

    if (m_new) {
        connect(NetworkManager::Settings::notifier(), SIGNAL(connectionAddComplete(QString,bool,QString)),
                SLOT(connectionAddComplete(QString,bool,QString)));
        NetworkManager::Settings::addConnection(connectionSettings->toMap());
    } else {
        NetworkManager::Settings::Connection * connection = NetworkManager::Settings::findConnectionByUuid(connectionSettings->uuid());

        if (connection) {
            connection->update(connectionSettings->toMap());
        }
    }
}

void ConnectionDetailEditor::connectionAddComplete(const QString& id, bool success, const QString& msg)
{
    qDebug() << id << " - " << success << " - " << msg;
}

void ConnectionDetailEditor::gotSecrets(const QString& id, bool success, const QVariantMapMap& secrets, const QString& msg)
{
    if (id == m_connection->uuid()) {
        m_numSecrets--;
    } else {
        return;
    }

    if (success) {
        foreach (const QString & key, secrets.keys()) {
            NetworkManager::Settings::Setting * setting = m_connection->setting(NetworkManager::Settings::Setting::typeFromString(key));
            setting->secretsFromMap(secrets.value(key));
        }
    } else {
        qDebug() << msg;
    }

    if (!m_numSecrets) {
        initTabs();
    }
}
