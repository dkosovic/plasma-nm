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

#include "networksettings.h"
#include "networksettingsmodel.h"
#include "uiutils.h"

#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Settings>
#include <NetworkManagerQt/Connection>

#include <KDebug>
#include <KLocale>
#include <KConfig>
#include <KConfigGroup>

class NetworkSettingsPrivate {
public:
    NetworkSettings *q;
    QObject *networkSettingsModel;
//     QString details;
    QString icon;
    QString settingName;
    QString status;

    NetworkManager::ConnectionSettings::ConnectionType connectionType;
//     QStringList detailKeys;
    QString path;
    NetworkSettingModelItem::NetworkType type;

    void initNetwork();
};

NetworkSettings::NetworkSettings()
{
    d = new NetworkSettingsPrivate;
    d->q = this;
    d->connectionType = NetworkManager::ConnectionSettings::Unknown;
    d->networkSettingsModel = 0;
    d->settingName = i18n("Network Settings");
    d->status = i18n("Network status and control");
    d->type = NetworkSettingModelItem::Undefined;

    d->initNetwork();

//     QStringList detailKeys;
//     detailKeys << "interface:status" << "interface:bitrate" << "interface:hardwareaddress" << "ipv4:address" << "ipv6:address" << "wireless:ssid" << "wireless:signal";
//     detailKeys << "wireless:security" << "mobile:operator" << "mobile:quality" << "mobile:technology" << "vpn:plugin" << "vpn:banner";

    KConfig config("networkactivesettingrc");
    KConfigGroup generalGroup(&config, "General");

//     setDetailKeys(generalGroup.readEntry("DetailKeys", detailKeys));

    kDebug() << "NetworkSettings module loaded.";
}

NetworkSettings::~NetworkSettings()
{
    kDebug() << "========================== NetworkSettings destroyed";
    delete d;
}

void NetworkSettingsPrivate::initNetwork()
{
    QAbstractItemModel * _networkSettingsModel = new NetworkSettingsModel(q);
    networkSettingsModel = _networkSettingsModel;
}

QObject* NetworkSettings::networkSettingsModel()
{
    return d->networkSettingsModel;
}

void NetworkSettings::setNetworkSettingsModel(QObject *networkSettingsModel)
{
    if ( d->networkSettingsModel != networkSettingsModel) {
        d->networkSettingsModel = networkSettingsModel;
        emit networkSettingsModelChanged();
    }
}

int NetworkSettings::connectionType() const
{
    return (int) d->connectionType;
}

void NetworkSettings::setConnectionType(int type)
{
    NetworkManager::ConnectionSettings::ConnectionType newType = (NetworkManager::ConnectionSettings::ConnectionType) type;

    if (d->connectionType != newType) {
        d->connectionType = newType;
        emit connectionTypeChanged();
    }
}

// QString NetworkSettings::details() const
// {
//     return d->details;
// }
//
// void NetworkSettings::setDetails(const QString &details)
// {
//     if (d->details != details) {
//         d->details = details;
//         emit detailsChanged();
//     }
// }
//
// QStringList NetworkSettings::detailKeys() const
// {
//     return d->detailKeys;
// }
//
// void NetworkSettings::setDetailKeys(const QStringList &keys)
// {
//     if (d->detailKeys != keys) {
//         d->detailKeys = keys;
//         updateDetails();
//         emit detailKeysChanged();
//     }
// }

QString NetworkSettings::settingName() const
{
    return d->settingName;
}

void NetworkSettings::setSettingName(const QString &name)
{
    if (d->settingName != name) {
        d->settingName = name;
        emit settingNameChanged();
    }
}

QString NetworkSettings::status() const
{
    return d->status;
}

void NetworkSettings::setStatus(const QString &status)
{
    if (d->status != status) {
        d->status = status;
        emit statusChanged();
    }
}

void NetworkSettings::setNetworkSetting(uint type, const QString &path)
{
    if (d->type == NetworkSettingModelItem::Vpn) {
        disconnect(NetworkManager::notifier(), 0, this, 0);
    } else if (d->type != NetworkSettingModelItem::Undefined) {
        NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(path);
        if (device) {
            disconnect(device.data(), 0, this, 0);
        }
    }

    d->type = (NetworkSettingModelItem::NetworkType) type;
    d->path = path;

    if (d->type == NetworkSettingModelItem::Ethernet ||
        d->type == NetworkSettingModelItem::Modem ||
        d->type == NetworkSettingModelItem::Wifi) {
        NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(path);

        if (device) {
            connect(device.data(), SIGNAL(connectionStateChanged()),
                    SLOT(updateStatus()), Qt::UniqueConnection);
            connect(device.data(), SIGNAL(activeConnectionChanged()),
                    SLOT(updateStatus()), Qt::UniqueConnection);
//             connect(device.data(), SIGNAL(connectionStateChanged()),
//                     SLOT(updateDetails()), Qt::UniqueConnection);
//             connect(device.data(), SIGNAL(activeConnectionChanged()),
//                     SLOT(updateDetails()), Qt::UniqueConnection);
        }
    } else if (d->type == NetworkSettingModelItem::Vpn) {
        foreach (const NetworkManager::ActiveConnection::Ptr & activeConnection, NetworkManager::activeConnections()) {
            if (activeConnection && activeConnection->vpn()) {
                connect(activeConnection.data(), SIGNAL(stateChanged(NetworkManager::ActiveConnection::State)),
                        SLOT(updateStatus()), Qt::UniqueConnection);
//                 connect(activeConnection.data(), SIGNAL(stateChanged(NetworkManager::ActiveConnection::State)),
//                         SLOT(updateDetails()), Qt::UniqueConnection);
            }
        }

        connect(NetworkManager::notifier(), SIGNAL(activeConnectionAdded(QString)),
                SLOT(activeConnectionAdded(QString)), Qt::UniqueConnection);
    }

    updateConnectionType();
//     updateDetails();
    updateSettingName();
    updateStatus();
}

void NetworkSettings::activeConnectionAdded(const QString &active)
{
    NetworkManager::ActiveConnection::Ptr activeConnection = NetworkManager::findActiveConnection(active);

    if (activeConnection && activeConnection->vpn()) {
        connect(activeConnection.data(), SIGNAL(stateChanged(NetworkManager::ActiveConnection::State)),
                SLOT(updateStatus()), Qt::UniqueConnection);
//         connect(activeConnection.data(), SIGNAL(stateChanged(NetworkManager::ActiveConnection::State)),
//                 SLOT(updateDetails()), Qt::UniqueConnection);
//         updateDetails();
        updateStatus();
    }
}

void NetworkSettings::updateConnectionType()
{
    if (d->type == NetworkSettingModelItem::Ethernet) {
        setConnectionType(NetworkManager::ConnectionSettings::Wired);
    } else if (d->type == NetworkSettingModelItem::Modem) {
        setConnectionType(NetworkManager::ConnectionSettings::Gsm);
    } else if (d->type == NetworkSettingModelItem::Wifi) {
        setConnectionType(NetworkManager::ConnectionSettings::Wireless);
    } else if (d->type == NetworkSettingModelItem::Vpn) {
        setConnectionType(NetworkManager::ConnectionSettings::Vpn);
    }
}

// void NetworkSettings::updateDetails()
// {
//     const QString format = "<tr><td align=\"right\" width=\"50%\"><b>%1</b></td><td align=\"left\" width=\"50%\">&nbsp;%2</td></tr>";
//     QString details = "<qt><table>";
//
//     if (d->type == NetworkSettingModelItem::Vpn) {
//         NetworkManager::ActiveConnection::Ptr active;
//         foreach (const NetworkManager::ActiveConnection::Ptr & activeConnection, NetworkManager::activeConnections()) {
//             if (activeConnection && activeConnection->vpn() &&
//                 (activeConnection->state() == NetworkManager::ActiveConnection::Activated || activeConnection->state() == NetworkManager::ActiveConnection::Activating)) {
//                 active = activeConnection;
//             }
//         }
//
//         if (active) {
//             NetworkManager::Connection::Ptr connection = NetworkManager::findConnection(active->connection()->path());
//             NetworkManager::ConnectionSettings::Ptr connectionSettings;
//             NetworkManager::VpnSetting::Ptr vpnSetting;
//             NetworkManager::VpnConnection::Ptr vpnConnection;
//
//             if (connection) {
//                 connectionSettings = connection->settings();
//             }
//             if (connectionSettings) {
//                 vpnSetting = connectionSettings->setting(NetworkManager::Setting::Vpn).dynamicCast<NetworkManager::VpnSetting>();
//             }
//
//             if (active) {
//                 vpnConnection = NetworkManager::VpnConnection::Ptr(new NetworkManager::VpnConnection(active->path()), &QObject::deleteLater);
//             }
//             details += UiUtils::vpnDetails(vpnConnection, vpnSetting, d->detailKeys, format);
//         } else {
//             // TODO
//         }
//     } else {
//         NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(d->path);
//         if (device) {
//             const NetworkManager::Device::State state = device->state();
//             const bool connected = (state == NetworkManager::Device::Activated);
//             const bool connecting = (state >= NetworkManager::Device::CheckingIp && state <= NetworkManager::Device::Preparing);
//             if (d->type == NetworkSettingModelItem::Ethernet) {
//                 details += UiUtils::connectionDetails(device, NetworkManager::ConnectionSettings::Wired, connected, connecting, d->detailKeys, format);
//                 NetworkManager::WiredDevice::Ptr wiredDevice;
//                 wiredDevice = device.objectCast<NetworkManager::WiredDevice>();
//                 details += UiUtils::wiredDetails(wiredDevice, connected, d->detailKeys, format);
//             } else if (d->type == NetworkSettingModelItem::Modem) {
//                 details += UiUtils::deviceDetails(device, NetworkManager::ConnectionSettings::Gsm, connected, connecting, d->detailKeys, format);
//                 NetworkManager::ModemDevice::Ptr modemDevice = device.objectCast<NetworkManager::ModemDevice>();
//                 details += UiUtils::modemDetails(modemDevice, d->detailKeys, format);
//             } else if (d->type == NetworkSettingModelItem::Wifi) {
//                 details += UiUtils::deviceDetails(device, NetworkManager::ConnectionSettings::Wireless, connected, connecting, d->detailKeys, format);
//                 NetworkManager::WirelessDevice::Ptr wirelessDevice;
//                 wirelessDevice = device.objectCast<NetworkManager::WirelessDevice>();
//                 QString ssid;
//                 NetworkManager::AccessPoint::Ptr ap = wirelessDevice->activeAccessPoint();
//                 if (ap) {
//                     ssid = wirelessDevice->activeAccessPoint()->ssid();
//                 }
//                 NetworkManager::WirelessNetwork::Ptr network;
//                 if (wirelessDevice) {
//                     network = wirelessDevice->findNetwork(ssid);
//                 }
//                 details += UiUtils::wirelessDetails(wirelessDevice, network, ap, connected, d->detailKeys, format);
//             }
//         }
//     }
//     details += "</table></qt>";
//
//     setDetails(details);
// }

void NetworkSettings::updateSettingName()
{
    switch (d->type) {
    case NetworkSettingModelItem::Ethernet:
        setSettingName(i18n("Ethernet Settings"));
        break;
    case NetworkSettingModelItem::Modem:
        setSettingName(i18n("Modem Settings"));
        break;
    case NetworkSettingModelItem::Vpn:
        setSettingName(i18n("VPN Settings"));
        break;
    case NetworkSettingModelItem::Wifi:
        setSettingName(i18n("Wireless Settings"));
        break;
    default:
        break;
    }
}

void NetworkSettings::updateStatus()
{
    if (d->type == NetworkSettingModelItem::Vpn) {
        // TODO: maybe check for the case when there are two active VPN connections
        bool vpnFound = false;
        foreach (const NetworkManager::ActiveConnection::Ptr & activeConnection, NetworkManager::activeConnections()) {
            if (activeConnection && activeConnection->vpn()) {
                vpnFound = true;
                if (activeConnection->state() == NetworkManager::ActiveConnection::Unknown) {
                    setStatus(UiUtils::connectionStateToString(NetworkManager::Device::UnknownState));
                } else if (activeConnection->state() == NetworkManager::ActiveConnection::Activating) {
                    setStatus(i18n("Connecting"));
                } else if (activeConnection->state() == NetworkManager::ActiveConnection::Activated) {
                    setStatus(UiUtils::connectionStateToString(NetworkManager::Device::Activated) % i18n(" to %1").arg(activeConnection->connection()->name()));
                } else if (activeConnection->state() == NetworkManager::ActiveConnection::Deactivating) {
                    setStatus(UiUtils::connectionStateToString(NetworkManager::Device::Deactivating));
                } else if (activeConnection->state() == NetworkManager::ActiveConnection::Deactivated) {
                    setStatus(UiUtils::connectionStateToString(NetworkManager::Device::Disconnected));
                }
            }
        }

        if (!vpnFound) {
            setStatus(UiUtils::connectionStateToString(NetworkManager::Device::Disconnected));
        }
    } else if (d->type != NetworkSettingModelItem::Undefined) {
        NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(d->path);
        if (device) {
            NetworkManager::ActiveConnection::Ptr activeConnection = device->activeConnection();
            if (activeConnection) {
                setStatus(UiUtils::connectionStateToString(device->state(), activeConnection->connection()->name()));
            } else {
                setStatus(UiUtils::connectionStateToString(device->state()));
            }
        }
    } else {
        setStatus(UiUtils::connectionStateToString(NetworkManager::Device::UnknownState));
    }
}

#include "networksettings.moc"
