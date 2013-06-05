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

#include "modelitem.h"

#include <KLocalizedString>

#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Settings>
#include <NetworkManagerQt/BluetoothDevice>
#include <NetworkManagerQt/ModemDevice>
#include <NetworkManagerQt/WimaxDevice>
#include <NetworkManagerQt/WiredDevice>
#include <NetworkManagerQt/WirelessDevice>
#include <NetworkManagerQt/VpnConnection>

#include <NetworkManagerQt/Utils>
#include <NetworkManagerQt/VpnSetting>
#include <NetworkManagerQt/WirelessSetting>

#include <ModemManagerQt/modemgsmnetworkinterface.h>

#include <KGlobal>
#include <KLocale>

#include "model.h"
#include "uiutils.h"
#include "applet/globalconfig.h"

#include "debug.h"

ModelItem::ModelItem(const QString& device, QObject * parent):
    QObject(parent),
    m_connected(false),
    m_connecting(false),
    m_secure(false),
    m_signal(0),
    m_sectionType(ModelItem::Unknown),
    m_type(NetworkManager::ConnectionSettings::Unknown)
{
    if (!device.isEmpty()) {
        setDevice(device);
    }
}

ModelItem::~ModelItem()
{
}

bool ModelItem::connected() const
{
    return m_connected;
}

bool ModelItem::connecting() const
{
    return m_connecting;
}

bool ModelItem::secure() const
{
    return m_secure;
}

QString ModelItem::details() const
{
    return m_details;
}

QString ModelItem::deviceName() const
{
    return m_device;
}

QString ModelItem::icon() const
{
    switch (m_type) {
        case NetworkManager::ConnectionSettings::Adsl:
            return "modem";
            break;
        case NetworkManager::ConnectionSettings::Bluetooth:
            return "preferences-system-bluetooth";
            break;
        case NetworkManager::ConnectionSettings::Bond:
            break;
        case NetworkManager::ConnectionSettings::Bridge:
            break;
        case NetworkManager::ConnectionSettings::Cdma:
            return "phone";
            break;
        case NetworkManager::ConnectionSettings::Gsm:
            return "phone";
            break;
        case NetworkManager::ConnectionSettings::Infiniband:
            break;
        case NetworkManager::ConnectionSettings::OLPCMesh:
            break;
        case NetworkManager::ConnectionSettings::Pppoe:
            return "modem";
            break;
        case NetworkManager::ConnectionSettings::Vlan:
            break;
        case NetworkManager::ConnectionSettings::Vpn:
            return "secure-card";
            break;
        case NetworkManager::ConnectionSettings::Wimax:
            break;
        case NetworkManager::ConnectionSettings::Wired:
            if (connected()) {
                return "network-wired-activated";
            } else {
                return "network-wired";
            }
            break;
        case NetworkManager::ConnectionSettings::Wireless:
            if (m_signal < 13) {
                return "network-wireless-connected-00";
            } else if (m_signal < 38) {
                return "network-wireless-connected-25";
            } else if (m_signal < 63) {
                return "network-wireless-connected-50";
            } else if (m_signal < 88) {
                return "network-wireless-connected-75";
            } else {
                return "network-wireless-connected-100";
            }
            break;
        default:
            return "network-wired";
            break;
    }

    return "network-wired";
}

QString ModelItem::name() const
{
    return m_name;
}

QString ModelItem::nspPath() const
{
    return m_nspPath;
}

QString ModelItem::uuid() const
{
    return m_uuid;
}

QString ModelItem::ssid() const
{
    return m_ssid;
}

QString ModelItem::sectionType() const
{
    if (m_connected) {
        return i18n("Active connections");
    } else if (!m_uuid.isEmpty()) {
        return i18n("Previous connections");
    } else {
        return i18n("Unknown connections");
    }
}

QString ModelItem::originalName() const
{
    return name() + " (" + deviceName() + ')';
}

QString ModelItem::activeConnectionPath() const
{
    return m_activePath;
}

QString ModelItem::connectionPath() const
{
    return m_connectionPath;
}

QString ModelItem::devicePath() const
{
    return m_devicePath;
}

QString ModelItem::specificPath() const
{
    if (type() == NetworkManager::ConnectionSettings::Wimax) {
        return m_nspPath;
    } else if (type() == NetworkManager::ConnectionSettings::Wireless) {
        return m_accessPointPath;
    }

    return QString();
}

int ModelItem::signal() const
{
    return m_signal;
}

NetworkManager::ConnectionSettings::ConnectionType ModelItem::type() const
{
    return m_type;
}

void ModelItem::updateDetails()
{
    QString format = "<tr><td align=\"right\" width=\"50%\"><b>%1</b></td><td align=\"left\" width=\"50%\">&nbsp;%2</td></tr>";
    m_details = "<qt><table>";

    QStringList detailKeys = GlobalConfig().detailKeys();
    GlobalConfig::NetworkSpeedUnit unit = GlobalConfig().networkSpeedUnit();

    // Initialize objects
    NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(m_devicePath);

    foreach (const QString& key, detailKeys) {
        if (key == "interface:type") {
            if (m_type != NetworkManager::ConnectionSettings::Unknown) {
                m_details += QString(format).arg(i18nc("type of network device", "Type:"), NetworkManager::ConnectionSettings::typeAsString(m_type));
            }
        } else if (key == "interface:status") {
            QString status = i18n("Disconnected");
            if (m_connecting) {
                status = i18n("Connecting");
            } else if (m_connected) {
                status = i18n("Connected");
            }
            m_details += QString(format).arg(i18n("Status:"), status);
        } else if (key == "interface:name") {
            if (device) {
                QString name;
                if (device->ipInterfaceName().isEmpty()) {
                    name = device->interfaceName();
                } else {
                    name = device->ipInterfaceName();
                }
                m_details += QString(format).arg(i18n("System name:"), name);
            }
        } else if (key == "ipv4:address") {
            if (device && device->ipV4Config().isValid() && m_connected) {
                if (device->ipV4Config().addresses().isEmpty()) {
                    continue;
                }
                QHostAddress addr = device->ipV4Config().addresses().first().ip();
                m_details += QString(format).arg(i18n("IPv4 Address:"), addr.toString());
            }
        } else if (key == "ipv4:gateway") {
            if (device && device->ipV4Config().isValid() && m_connected) {
                if (device->ipV4Config().addresses().isEmpty()) {
                    continue;
                }
                QHostAddress addr = device->ipV4Config().addresses().first().gateway();
                m_details += QString(format).arg(i18n("IPv4 Gateway:"), addr.toString());
            }
        } else if (key == "ipv6:address") {
            if (device && device->ipV6Config().isValid() && m_connected) {
                if (device->ipV6Config().addresses().isEmpty()) {
                    continue;
                }
                QHostAddress addr = device->ipV6Config().addresses().first().ip();
                m_details += QString(format).arg(i18n("IPv6 Address:"), addr.toString());
            }
        } else if (key == "ipv6:gateway") {
            if (device && device->ipV6Config().isValid() && m_connected) {
                if (device->ipV6Config().addresses().isEmpty()) {
                    continue;
                }
                QHostAddress addr = device->ipV6Config().addresses().first().gateway();
                m_details += QString(format).arg(i18n("IPv6 Gateway:"), addr.toString());
            }
        } else if (key == "interface:driver") {
            if (device) {
                m_details += QString(format).arg(i18n("Driver:"), device->driver());
            }
        }
    }

    if (m_type == NetworkManager::ConnectionSettings::Bluetooth) {
        NetworkManager::BluetoothDevice::Ptr btDevice;
        if (device) {
            btDevice = device.objectCast<NetworkManager::BluetoothDevice>();
        }

        foreach (const QString& key, detailKeys) {
            if (key == "bluetooth:name") {
                if (btDevice) {
                    m_details += QString(format).arg(i18n("Name:"), btDevice->name());
                }
            } else if (key == "interface:hardwareAddress") {
                if (btDevice) {
                    m_details += QString(format).arg(i18n("MAC Address:"), btDevice->hardwareAddress());
                }
            } else if (key == "interface:driver") {
                if (btDevice) {
                    m_details += QString(format).arg(i18n("Driver:"), device->driver());
                }
            }
        }
    } else if (m_type == NetworkManager::ConnectionSettings::Gsm) {
        NetworkManager::ModemDevice::Ptr modemDevice;
        ModemManager::ModemGsmNetworkInterface::Ptr modemNetwork;
        if (device) {
            modemDevice = device.objectCast<NetworkManager::ModemDevice>();
        }
        if (modemDevice) {
            modemNetwork = modemDevice->getModemNetworkIface().objectCast<ModemManager::ModemGsmNetworkInterface>();
        }

        foreach (const QString& key, detailKeys) {
            if (key == "mobile:operator") {
                if (modemNetwork) {
                    m_details += QString(format).arg(i18n("Operator:"), modemNetwork->getRegistrationInfo().operatorName);
                }
            } else if (key == "mobile:quality") {
                if (modemNetwork) {
                    m_details += QString(format).arg(i18n("Signal quality:"), QString("%1%").arg(modemNetwork->getSignalQuality()));
                }
            } else if (key == "mobile:technology") {
                if (modemNetwork) {
                    m_details += QString(format).arg(i18n("Access technology:"), QString("%1/%2").arg(UiUtils::convertTypeToString(modemNetwork->type()), UiUtils::convertAccessTechnologyToString(modemNetwork->getAccessTechnology())));
                }
            } else if (key == "mobile:mode") {
                if (modemNetwork) {
                    m_details += QString(format).arg(i18n("Allowed Mode"), UiUtils::convertAllowedModeToString(modemNetwork->getAllowedMode()));
                }
            } else if (key == "mobile:band") {
                if (modemNetwork) {
                    m_details += QString(format).arg(i18n("Frequency Band:"), UiUtils::convertBandToString(modemNetwork->getBand()));
                }
            } else if (key == "mobile:unlock") {
                if (modemNetwork) {
                    m_details += QString(format).arg(i18n("Unlock Required:"), modemNetwork->unlockRequired());
                }
            } else if (key == "mobile:imei") {
                if (modemDevice) {
                    ModemManager::ModemGsmCardInterface::Ptr modemCard;
                    modemCard = modemDevice->getModemCardIface();
                    if (modemCard) {
                        detailKeys += QString(format).arg(i18n("IMEI:"), modemCard->getImei());
                    }
                }
            } else if (key == "mobile:imsi") {
                if (modemDevice) {
                    ModemManager::ModemGsmCardInterface::Ptr modemCard;
                    modemCard = modemDevice->getModemCardIface();
                    if (modemCard) {
                        detailKeys += QString(format).arg(i18n("IMSI:"), modemCard->getImsi());
                    }
                }
            }
        }
    } else if (m_type == NetworkManager::ConnectionSettings::Wimax) {
        NetworkManager::WimaxDevice::Ptr wimaxDevice;
        NetworkManager::WimaxNsp::Ptr wimaxNsp;
        if (device) {
            wimaxDevice = device.objectCast<NetworkManager::WimaxDevice>();
        }
        if (wimaxDevice) {
            wimaxNsp = wimaxDevice->findNsp(m_nspPath);
        }

        foreach (const QString& key, detailKeys) {
            if (key == "wimax:bsid") {
                if (m_connected && wimaxDevice) {
                    m_details += QString(format).arg(i18n("Bsid:"), wimaxDevice->bsid());
                }
            } else if (key == "wimax:nsp") {
                if (wimaxNsp) {
                    m_details += QString(format).arg(i18n("NSP Name:"), wimaxNsp->name());
                }
            } else if (key == "wimax:signal") {
                if (wimaxNsp) {
                    m_details += QString(format).arg(i18n("Signal Quality:"), i18n("%1%", wimaxNsp->signalQuality()));
                }
            } else if (key == "wimax:type") {
                if (wimaxNsp) {
                    m_details += QString(format).arg(i18n("Network Type:"), UiUtils::convertNspTypeToString(wimaxNsp->networkType()));
                }
            }
        }
    } else if (m_type == NetworkManager::ConnectionSettings::Wired) {
        NetworkManager::WiredDevice::Ptr wiredDevice;
        if (device) {
            wiredDevice = device.objectCast<NetworkManager::WiredDevice>();
        }

        foreach (const QString& key, detailKeys) {
            if (key == "interface:bitrate") {
                if (wiredDevice && m_connected) {
                    if (unit == GlobalConfig::KBits) {
                        m_details += QString(format).arg(i18n("Connection speed:"), UiUtils::connectionSpeed(wiredDevice->bitRate()));
                    } else {
                        m_details += QString(format).arg(i18n("Connection speed:"), KGlobal::locale()->formatByteSize(wiredDevice->bitRate() * 128));
                    }
                }
            } else if (key == "interface:hardwareaddress") {
                if (wiredDevice) {
                    m_details += QString(format).arg(i18n("MAC Address:"), wiredDevice->permanentHardwareAddress());
                }
            }
        }
    } else if (m_type == NetworkManager::ConnectionSettings::Wireless) {
        NetworkManager::WirelessDevice::Ptr wirelessDevice;
        if (device) {
            wirelessDevice = device.objectCast<NetworkManager::WirelessDevice>();
        }
        NetworkManager::WirelessNetwork::Ptr network;
        if (wirelessDevice) {
            network = wirelessDevice->findNetwork(m_ssid);
        }
        NetworkManager::AccessPoint::Ptr ap;
        if (network) {
            ap = network->referenceAccessPoint();
        }

        foreach (const QString& key, detailKeys) {
            if (key == "interface:bitrate") {
                if (wirelessDevice && m_connected) {
                    if (unit == GlobalConfig::KBits) {
                        m_details += QString(format).arg(i18n("Connection speed:"), UiUtils::connectionSpeed(wirelessDevice->bitRate()));
                    } else {
                        m_details += QString(format).arg(i18n("Connection speed:"), KGlobal::locale()->formatByteSize(wirelessDevice->bitRate() * 128));
                    }
                }
            } else if (key == "interface:hardwareaddress") {
                if (wirelessDevice) {
                    m_details += QString(format).arg(i18n("MAC Address:"), wirelessDevice->permanentHardwareAddress());
                }
            } else if (key == "wireless:mode") {
                if (wirelessDevice) {
                    m_details += QString(format).arg(i18n("Mode:"), UiUtils::operationModeToString(wirelessDevice->mode()));
                }
            } else if (key == "wireless:signal") {
                if (network) {
                    m_details += QString(format).arg(i18n("Signal strength:"), i18n("%1%", network->signalStrength()));
                }
            } else if (key == "wireless:ssid") {
                if (network) {
                    m_details += QString(format).arg(i18n("Access point (SSID):"), network->ssid());
                }
            } else if (key == "wireless:accesspoint") {
                if (ap) {
                    m_details += QString(format).arg(i18n("Access point (BSSID):"), ap->hardwareAddress());
                }
            } else if (key == "wireless:channel") {
                if (ap) {
                    m_details += QString(format).arg(i18nc("Wifi AP channel and frequency", "Channel:"), i18n("%1 (%2 MHz)", NetworkManager::Utils::findChannel(ap->frequency()), ap->frequency()));
                }
            } else if (key == "wireless:security") {
                if (ap) {
                    NetworkManager::Utils::WirelessSecurityType security = NetworkManager::Utils::findBestWirelessSecurity(wirelessDevice->wirelessCapabilities(), true, (wirelessDevice->mode() == NetworkManager::WirelessDevice::Adhoc),
                                                                                                                           ap->capabilities(), ap->wpaFlags(), ap->rsnFlags());
                    m_details += QString(format).arg(i18n("Security:"), UiUtils::labelFromWirelessSecurity(security));
                }
            } else if (key == "wireless:band") {
                if (ap) {
                    m_details += QString(format).arg(i18n("Frequency band:"), UiUtils::wirelessBandToString(NetworkManager::Utils::findFrequencyBand(ap->frequency())));
                }
            }
        }
    } else if (m_type == NetworkManager::ConnectionSettings::Vpn) {
        NetworkManager::ActiveConnection::Ptr active = NetworkManager::findActiveConnection(m_activePath);
        NetworkManager::Connection::Ptr connection = NetworkManager::findConnection(m_connectionPath);
        NetworkManager::ConnectionSettings::Ptr connectionSettings;
        NetworkManager::VpnSetting::Ptr vpnSetting;
        NetworkManager::VpnConnection::Ptr vpnConnection;

        if (connection) {
            connectionSettings = connection->settings();
        }
        if (connectionSettings) {
            vpnSetting = connectionSettings->setting(NetworkManager::Setting::Vpn).dynamicCast<NetworkManager::VpnSetting>();
        }

        if (active) {
            vpnConnection = NetworkManager::VpnConnection::Ptr(new NetworkManager::VpnConnection(active->path()), &QObject::deleteLater);
        }

        foreach (const QString& key, detailKeys) {
            if (key == "vpn:plugin") {
                if (vpnSetting) {
                    m_details += QString(format).arg(i18n("VPN plugin:"), vpnSetting->serviceType().section('.', -1));
                }
            } else if (key == "vpn:banner") {
                if (vpnConnection) {
                    m_details += QString(format).arg(i18n("Banner:"), vpnConnection->banner().simplified());
                }
            }
        }
    }

    m_details += "</table></qt>";
}

bool ModelItem::operator==(const ModelItem* item) const
{
    if (((item->uuid() == uuid() && !item->uuid().isEmpty() && !uuid().isEmpty()) ||
         (item->name() == name() && !item->name().isEmpty() && !name().isEmpty() && item->type() == type()) ||
         (item->ssid() == ssid() && !item->ssid().isEmpty() && !ssid().isEmpty()) ||
         (item->nspPath() == nspPath() && !item->nspPath().isEmpty() && !nspPath().isEmpty())) &&
         ((item->devicePath() == devicePath() && !item->devicePath().isEmpty() && !devicePath().isEmpty()) ||
          (item->type() == NetworkManager::ConnectionSettings::Vpn && type() == NetworkManager::ConnectionSettings::Vpn))) {
        return true;
    }
    return false;
}

void ModelItem::setActiveConnection(const QString& active)
{
    m_activePath = active;

    NetworkManager::ActiveConnection::Ptr activeConnection = NetworkManager::findActiveConnection(active);

    if (activeConnection) {
        if (activeConnection->state() == NetworkManager::ActiveConnection::Activating) {
            m_connecting = true;
            m_connected = false;
            NMItemDebug() << name() << ": activating";
        } else if (activeConnection->state() == NetworkManager::ActiveConnection::Activated) {
            NMItemDebug() << name() << ": activated";
            m_connected = true;
            m_connecting = false;
        }
    } else {
        m_connecting = false;
        m_connected = false;
    }

    updateDetails();
}

void ModelItem::setDevice(const QString& device)
{
    m_devicePath = device;
    NetworkManager::Device::Ptr dev = NetworkManager::findNetworkInterface(device);

    if (dev) {
        if (dev->ipInterfaceName().isEmpty()) {
            m_device = dev->interfaceName();
        } else {
            m_device = dev->ipInterfaceName();
        }
        m_devicePath = dev->uni();
        updateDetails();
    } else {
        m_devicePath.clear();
    }
}

void ModelItem::setConnection(const QString& connection)
{
    m_connectionPath = connection;
    NetworkManager::Connection::Ptr con = NetworkManager::findConnection(m_connectionPath);

    if (con && !con->uuid().isEmpty() && !con->name().isEmpty()) {
        setConnectionSettings(con->settings());
    } else {
        m_connectionPath.clear();
        m_name.clear();
        m_uuid.clear();
        m_activePath.clear();
        m_connected = false;
        m_connecting = false;

        if (!m_ssid.isEmpty()) {
            m_name = m_ssid;
        } else if (!m_nsp.isEmpty()) {
            m_name = m_nsp;
        }
    }
}

void ModelItem::setConnectionSettings(const NetworkManager::ConnectionSettings::Ptr& settings)
{
    m_uuid = settings->uuid();
    m_name = settings->id();
    m_type = settings->connectionType();

    if (type() == NetworkManager::ConnectionSettings::Wireless) {
        bool changed = false;
        QString previousSsid;
        NetworkManager::WirelessSetting::Ptr wirelessSetting;
        wirelessSetting = settings->setting(NetworkManager::Setting::Wireless).dynamicCast<NetworkManager::WirelessSetting>();
        if (m_ssid != wirelessSetting->ssid()) {
            if (!m_ssid.isEmpty()) {
                changed = true;
            }
            previousSsid = m_ssid;
            m_ssid = wirelessSetting->ssid();
            if (!wirelessSetting->security().isEmpty()) {
                m_secure = true;
            }
        }

        if (!changed) {
            return;
        }

        NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(devicePath());
        if (!device) {
            return;
        }
        NetworkManager::WirelessDevice::Ptr wifiDevice = device.objectCast<NetworkManager::WirelessDevice>();
        if (!wifiDevice) {
            return;
        }
        NetworkManager::WirelessNetwork::Ptr newWifiNetwork = wifiDevice->findNetwork(m_ssid);

        if (!newWifiNetwork) {
            setConnection(QString());
            NetworkManager::WirelessNetwork::Ptr wifiNetwork = wifiDevice->findNetwork(previousSsid);
            if (wifiNetwork) {
                setWirelessNetwork(previousSsid);
            }
        } else {
            setWirelessNetwork(m_ssid);
        }
    }

    updateDetails();
}

void ModelItem::setNsp(const QString& nsp)
{
    NetworkManager::WimaxNsp::Ptr wimaxNsp;
    NetworkManager::WimaxDevice::Ptr wimaxDevice = NetworkManager::findNetworkInterface(m_devicePath).objectCast<NetworkManager::WimaxDevice>();

    if (wimaxDevice) {
        wimaxNsp = wimaxDevice->findNsp(nsp);
    }

    if (wimaxNsp) {
        m_nspPath = wimaxNsp->uni();
        m_nsp = wimaxNsp->name();
        m_signal = wimaxNsp->signalQuality();
        m_type = NetworkManager::ConnectionSettings::Wimax;

        if (m_name.isEmpty() || m_connectionPath.isEmpty()) {
            m_name = m_nsp;
        }
    } else {
        m_nsp.clear();
        m_signal = 0;
        m_type = NetworkManager::ConnectionSettings::Unknown;
    }

    updateDetails();
}

void ModelItem::setWirelessNetwork(const QString& ssid)
{
    NetworkManager::WirelessNetwork::Ptr network;
    NetworkManager::WirelessDevice::Ptr wifiDevice = NetworkManager::findNetworkInterface(m_devicePath).objectCast<NetworkManager::WirelessDevice>();

    if (wifiDevice) {
        network = wifiDevice->findNetwork(ssid);
    }

    if (network) {
        m_accessPointPath = network->referenceAccessPoint()->uni();
        m_ssid = network->ssid();
        m_signal = network->signalStrength();
        m_type = NetworkManager::ConnectionSettings::Wireless;

        if (m_name.isEmpty() || m_connectionPath.isEmpty()) {
            m_name = m_ssid;
        }

        NetworkManager::AccessPoint::Ptr ap = wifiDevice->findAccessPoint(m_accessPointPath);

        if (ap && ap->capabilities() & NetworkManager::AccessPoint::Privacy) {
            m_secure = true;
        }
    } else {
        m_ssid.clear();
        m_signal = 0;
        m_type = NetworkManager::ConnectionSettings::Unknown;
        m_secure = false;
    }

    updateDetails();
}

void ModelItem::updateActiveConnectionState(NetworkManager::ActiveConnection::State state)
{
    if (state == NetworkManager::ActiveConnection::Deactivated ||
        state == NetworkManager::ActiveConnection::Deactivating) {
        NMItemDebug() << name() << ": disconnected";
        m_connecting = false;
        m_connected = false;
    } else if (state == NetworkManager::ActiveConnection::Activated) {
        NMItemDebug() << name() << ": activated";
        m_connecting = false;
        m_connected = true;
    } else if (state == NetworkManager::ActiveConnection::Activating) {
        NMItemDebug() << name() << ": activating";
        m_connecting = true;
        m_connected = false;
    }

    updateDetails();

    NMItemDebug() << name() << ": state has been changed to " << state;
}

void ModelItem::updateAccessPoint(const QString& ap)
{
    m_accessPointPath = ap;

    updateDetails();

    NMItemDebug() << name() << ": access point changed to " << m_accessPointPath;
}

void ModelItem::updateSignalStrenght(int strength)
{
    m_signal = strength;

    updateDetails();

    NMItemDebug() << name() << ": signal strength changed to " << m_signal;
}
