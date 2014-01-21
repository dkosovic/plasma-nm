/*
    Copyright 2013-2014 Jan Grulich <jgrulich@redhat.com>

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

#include "debug.h"
#include "networkmodel.h"
#include "networkmodelitem.h"
#include "uiutils.h"

#if WITH_MODEMMANAGER_SUPPORT
#ifdef MODEMMANAGERQT_ONE
#include <ModemManagerQt/manager.h>
#include <ModemManagerQt/modem.h>
#endif
#endif
#include <NetworkManagerQt/Settings>

NetworkModel::NetworkModel(QObject* parent)
    : QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles = roleNames();
    roles[AvailableRole] = "Available";
    roles[ConnectionDetailsRole] = "ConnectionDetails";
    roles[ConnectionIconRole] = "ConnectionIcon";
    roles[ConnectionPathRole] = "ConnectionPath";
    roles[ConnectionStateRole] = "ConnectionState";
    roles[DevicePathRole] = "DevicePath";
    roles[LastUsedRole] = "LastUsed";
    roles[NameRole] = "Name";
    roles[SectionRole] = "Section";
    roles[SignalRole] = "Signal";
    roles[SsidRole] = "Ssid";
    roles[SpecificPathRole] = "SpecificPath";
    roles[SpeedRole] = "Speed";
    roles[SecurityTypeRole] = "SecurityType";
    roles[SecurityTypeStringRole] = "SecurityTypeString";
    roles[TypeRole] = "Type";
    roles[UuidRole] = "Uuid";
    roles[UniRole] = "Uni";
    setRoleNames(roles);

    initialize();
}

NetworkModel::~NetworkModel()
{
}

void NetworkModel::initialize()
{
    // Initialize existing connections
    foreach (const NetworkManager::Connection::Ptr& connection, NetworkManager::listConnections()) {
        addConnection(connection);
    }

    // Initialize existing active connections
    foreach (const NetworkManager::ActiveConnection::Ptr& active, NetworkManager::activeConnections()) {
        addActiveConnection(active);
    }

    // Initialize existing devices
    foreach (const NetworkManager::Device::Ptr& dev, NetworkManager::networkInterfaces()) {
        addDevice(dev);
    }

    initializeSignals();
}

void NetworkModel::initializeSignals()
{
    connect(NetworkManager::notifier(), SIGNAL(activeConnectionAdded(QString)),
            SLOT(activeConnectionAdded(QString)), Qt::UniqueConnection);
    connect(NetworkManager::notifier(), SIGNAL(activeConnectionRemoved(QString)),
            SLOT(activeConnectionRemoved(QString)), Qt::UniqueConnection);
    connect(NetworkManager::settingsNotifier(), SIGNAL(connectionAdded(QString)),
            SLOT(connectionAdded(QString)), Qt::UniqueConnection);
    connect(NetworkManager::settingsNotifier(), SIGNAL(connectionRemoved(QString)),
            SLOT(connectionRemoved(QString)), Qt::UniqueConnection);
    connect(NetworkManager::notifier(), SIGNAL(deviceAdded(QString)),
            SLOT(deviceAdded(QString)), Qt::UniqueConnection);
    connect(NetworkManager::notifier(), SIGNAL(deviceRemoved(QString)),
            SLOT(deviceRemoved(QString)), Qt::UniqueConnection);
    connect(NetworkManager::notifier(), SIGNAL(statusChanged(NetworkManager::Status)),
            SLOT(statusChanged(NetworkManager::Status)), Qt::UniqueConnection);
    connect(NetworkManager::notifier(), SIGNAL(serviceAppeared()),
            SLOT(initialize()), Qt::UniqueConnection);
}

void NetworkModel::initializeSignals(const NetworkManager::ActiveConnection::Ptr& activeConnection)
{
    if (activeConnection->vpn()) {
        NetworkManager::VpnConnection::Ptr vpnConnection = activeConnection.objectCast<NetworkManager::VpnConnection>();
        if (vpnConnection) {
            connect(vpnConnection.data(), SIGNAL(stateChanged(NetworkManager::VpnConnection::State,NetworkManager::VpnConnection::StateChangeReason)),
                    SLOT(activeVpnConnectionStateChanged(NetworkManager::VpnConnection::State,NetworkManager::VpnConnection::StateChangeReason)));
        }
    } else {
        connect(activeConnection.data(), SIGNAL(stateChanged(NetworkManager::ActiveConnection::State)),
                SLOT(activeConnectionStateChanged(NetworkManager::ActiveConnection::State)), Qt::UniqueConnection);
    }
}

void NetworkModel::initializeSignals(const NetworkManager::Connection::Ptr& connection)
{
    connect(connection.data(), SIGNAL(updated()), SLOT(connectionUpdated()), Qt::UniqueConnection);
}

void NetworkModel::initializeSignals(const NetworkManager::Device::Ptr& device)
{
    connect(device.data(), SIGNAL(availableConnectionAppeared(QString)),
            SLOT(availableConnectionAppeared(QString)), Qt::UniqueConnection);
    connect(device.data(), SIGNAL(availableConnectionDisappeared(QString)),
            SLOT(availableConnectionDisappeared(QString)), Qt::UniqueConnection);

    if (device->type() == NetworkManager::Device::Ethernet) {
        NetworkManager::WiredDevice::Ptr wiredDev = device.objectCast<NetworkManager::WiredDevice>();
        connect(wiredDev.data(), SIGNAL(bitRateChanged(int)),
                SLOT(bitrateChanged(int)));
    } else if (device->type() == NetworkManager::Device::Wifi) {
        NetworkManager::WirelessDevice::Ptr wifiDev = device.objectCast<NetworkManager::WirelessDevice>();
        connect(wifiDev.data(), SIGNAL(bitRateChanged(int)),
                SLOT(bitrateChanged(int)));
        connect(wifiDev.data(), SIGNAL(networkAppeared(QString)),
                SLOT(wirelessNetworkAppeared(QString)), Qt::UniqueConnection);
        connect(wifiDev.data(), SIGNAL(networkDisappeared(QString)),
                SLOT(wirelessNetworkDisappeared(QString)), Qt::UniqueConnection);
    }
#if WITH_MODEMMANAGER_SUPPORT
    else if (device->type() == NetworkManager::Device::Modem) {
        NetworkManager::ModemDevice::Ptr modemDev = device.objectCast<NetworkManager::ModemDevice>();
#ifdef MODEMMANAGERQT_ONE
        ModemManager::Modem::Ptr modemNetwork = modemDev->getModemNetworkIface();
        if (modemDev->isValid()) {
            connect(modemNetwork.data(), SIGNAL(signalQualityChanged(uint)),
                    SLOT(gsmNetworkSignalQualityChanged(uint)), Qt::UniqueConnection);
            connect(modemNetwork.data(), SIGNAL(accessTechnologyChanged(ModemManager::Modem::AccessTechnologies)),
                    SLOT(gsmNetworkAccessTechnologyChanged(ModemManager::Modem::AccessTechnologies)), Qt::UniqueConnection);
            connect(modemNetwork.data(), SIGNAL(currentModesChanged()),
                    SLOT(gsmNetworkCurrentModesChanged()), Qt::UniqueConnection);
        }
#else
        ModemManager::ModemGsmNetworkInterface::Ptr modemNetwork = modemDev->getModemNetworkIface().objectCast<ModemManager::ModemGsmNetworkInterface>();
        if (modemNetwork) {
            connect(modemNetwork.data(), SIGNAL(signalQualityChanged(uint)),
                    SLOT(gsmNetworkSignalQualityChanged(uint)), Qt::UniqueConnection);
            connect(modemNetwork.data(), SIGNAL(accessTechnologyChanged(ModemManager::ModemInterface::AccessTechnology)),
                    SLOT(gsmNetworkAccessTechnologyChanged(ModemManager::ModemInterface::AccessTechnology)), Qt::UniqueConnection);
            connect(modemNetwork.data(), SIGNAL(allowedModeChanged(ModemManager::ModemInterface::AllowedMode)),
                    SLOT(gsmNetworkAllowedModeChanged(ModemManager::ModemInterface::AllowedMode)), Qt::UniqueConnection);
        }
#endif
    }
#endif
}

void NetworkModel::initializeSignals(const NetworkManager::WirelessNetwork::Ptr& network)
{
    connect(network.data(), SIGNAL(signalStrengthChanged(int)),
            SLOT(wirelessNetworkSignalChanged(int)), Qt::UniqueConnection);
    connect(network.data(), SIGNAL(referenceAccessPointChanged(QString)),
            SLOT(wirelessNetworkReferenceApChanged(QString)), Qt::UniqueConnection);
}

QVariant NetworkModel::data(const QModelIndex& index, int role) const
{
    const int row = index.row();

    if (row >= 0 && row < m_list.count()) {
        NetworkModelItem * item = m_list.itemAt(row);

        switch (role) {
            case AvailableRole:
                return item->available();
            case ConnectionDetailsRole:
                return item->details();
            case ConnectionIconRole:
                return item->icon();
            case ConnectionPathRole:
                return item->connectionPath();
            case ConnectionStateRole:
                return item->connectionState();
            case DevicePathRole:
                return item->devicePath();
            case LastUsedRole:
                return item->lastUsed();
            case NameRole:
                // TODO duplicate items (i.e two wireless cards)??
//                 if (m_list.returnItems(NetworkItemsList::Name, item->name()).count() > 1) {
//                     return item->originalName();
//                 } else {
                return item->name();
//                 }
            case SectionRole:
                return item->sectionType();
            case SignalRole:
                return item->signal();
            case SsidRole:
                return item->ssid();
            case SpecificPathRole:
                return item->specificPath();
            case SpeedRole:
                return item->speed();
            case SecurityTypeRole:
                return item->securityType();
            case SecurityTypeStringRole:
                return UiUtils::labelFromWirelessSecurity(item->securityType());
            case TypeRole:
                return item->type();
            case UuidRole:
                return item->uuid();
            case UniRole:
                return item->uni();
            default:
                break;
        }
    }

    return QVariant();
}

int NetworkModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_list.count();
}

void NetworkModel::addActiveConnection(const NetworkManager::ActiveConnection::Ptr& activeConnection)
{
    qDebug() << "Add active connection - " << activeConnection->connection()->name();

    initializeSignals(activeConnection);

    NetworkManager::Connection::Ptr connection = activeConnection->connection();
    NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(activeConnection->devices().first());

    // Check whether we have a base connection
    if (m_list.returnItems(NetworkItemsList::Uuid, connection->uuid()).isEmpty()) {
        qDebug() << "Base connection doesn't exist, let's add it";
        // Active connection appeared before a base connection, so we have to add its base connection first
        addConnection(connection);
    }

    foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::NetworkItemsList::Uuid, connection->uuid())) {
        item->setActiveConnectionPath(activeConnection->path());
        item->setConnectionState(activeConnection->state());
        // Some properties make sense to display only for active connections, like connection speed etc.
        // TODO: device state
        if (device && device->type() == NetworkManager::Device::Ethernet) {
            NetworkManager::WiredDevice::Ptr wiredDev = device.objectCast<NetworkManager::WiredDevice>();
            if (wiredDev) {
                item->setSpeed(wiredDev->bitRate());
            }
        } else if (device && device->type() == NetworkManager::Device::Wifi) {
            NetworkManager::WirelessDevice::Ptr wirelessDev = device.objectCast<NetworkManager::WirelessDevice>();
            if (wirelessDev) {
                item->setSpeed(wirelessDev->bitRate());
            }
        }
#if WITH_MODEMMANAGER_SUPPORT
#ifdef MODEMMANAGERQT_ONE
        if (device && device->type() == NetworkManager::Device::Modem) {
            ModemManager::ModemDevice::Ptr modemDevice = ModemManager::findModemDevice(device->udi());
            if (modemDevice) {
                ModemManager::Modem::Ptr modemInterface = modemDevice->interface(ModemManager::ModemDevice::ModemInterface).objectCast<ModemManager::Modem>();
                if (modemInterface) {
                    item->setSignal(modemInterface->signalQuality().signal);
                }
            }
        }
#endif
#endif
        updateItem(item);
    }
}

void NetworkModel::addConnection(const NetworkManager::Connection::Ptr& connection)
{
    //TODO remove debug
    qDebug() << "Adding connection - " << connection->name();

    // Can't add a connection without name or uuid
    if (connection->name().isEmpty() || connection->uuid().isEmpty()) {
        return;
    }

    initializeSignals(connection);

    NetworkManager::ConnectionSettings::Ptr settings = connection->settings();
    NetworkManager::WirelessSetting::Ptr wirelessSetting;
    if (settings->connectionType() == NetworkManager::ConnectionSettings::Wireless) {
        wirelessSetting = settings->setting(NetworkManager::Setting::Wireless).dynamicCast<NetworkManager::WirelessSetting>();
        // Check whether is there already an access point in the model for this connection, so we can only extend it
        if (m_list.contains(NetworkItemsList::Ssid, wirelessSetting->ssid())) {
            foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::Ssid, wirelessSetting->ssid())) {
                item->setConnectionPath(connection->path());
                item->setLastUsed(settings->timestamp());
                item->setName(settings->id());
                item->setType(settings->connectionType());
                item->setUuid(settings->uuid());
                updateItem(item);
            }
            return;
        }
    }

    // Check whether the connection is already in the model to avoid duplicates, but this shouldn't happen
    if (!m_list.contains(NetworkItemsList::Connection, connection->path())) {
        NetworkModelItem * item = new NetworkModelItem();
        item->setConnectionPath(connection->path());
        item->setLastUsed(settings->timestamp());
        item->setName(settings->id());
        item->setType(settings->connectionType());
        item->setUuid(settings->uuid());

        if (item->type() == NetworkManager::ConnectionSettings::Wireless) {
            item->setSecurityType(NetworkManager::Utils::securityTypeFromConnectionSetting(settings));
            item->setSsid(wirelessSetting->ssid());
            if (wirelessSetting->mode() == NetworkManager::WirelessSetting::Adhoc ||
                wirelessSetting->mode() == NetworkManager::WirelessSetting::Ap) {
                item->setShared(true);
            }
        }
        item->updateDetails();
        const int index = m_list.count();
        beginInsertRows(QModelIndex(), index, index);
        m_list.insertItem(item);
        endInsertRows();
    }
}

void NetworkModel::addDevice(const NetworkManager::Device::Ptr& device)
{
    //TODO remove debug
    qDebug() << "Adding device - " << device->udi();

    initializeSignals(device);

    // Add available wireless networks
    if (device->type() == NetworkManager::Device::Wifi) {
        NetworkManager::WirelessDevice::Ptr wifiDev = device.objectCast<NetworkManager::WirelessDevice>();
        foreach (const NetworkManager::WirelessNetwork::Ptr& wifiNetwork, wifiDev->networks()) {
            addWirelessNetwork(wifiNetwork, wifiDev);
        }
    }

    // Add device to make connections available
    foreach (const NetworkManager::Connection::Ptr & connection, device->availableConnections()) {
        foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::Uuid, connection->uuid())) {
            if (device->ipInterfaceName().isEmpty()) {
                item->setDeviceName(device->interfaceName());
            } else {
                item->setDeviceName(device->ipInterfaceName());
            }
            item->setDevicePath(device->uni());
            updateItem(item);
        }
    }
}

void NetworkModel::addWirelessNetwork(const NetworkManager::WirelessNetwork::Ptr& network, const NetworkManager::WirelessDevice::Ptr& device)
{
    //TODO remove debug
    qDebug() << "Adding wireless network - " << network->ssid();

    initializeSignals(network);

    bool connectionFound = false;
    // Check whether there is an available connection for this network, because it's possible that some
    // connection has same SSID, but it might not be available due to specified BSSID or device
    foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::Ssid, network->ssid())) {
        foreach (const NetworkManager::Connection::Ptr connection, device->availableConnections()) {
            if (item->connectionPath() == connection->path()) {
                connectionFound = true;
                if (device->ipInterfaceName().isEmpty()) {
                    item->setDeviceName(device->interfaceName());
                } else {
                    item->setDeviceName(device->ipInterfaceName());
                }
                item->setDevicePath(device->uni());
                item->setSignal(network->signalStrength());
                item->setSpecificPath(network->referenceAccessPoint()->uni());
                updateItem(item);
            }
        }
    }

    if (!connectionFound) {
        NetworkModelItem * item = new NetworkModelItem();
        if (device->ipInterfaceName().isEmpty()) {
            item->setDeviceName(device->interfaceName());
        } else {
            item->setDeviceName(device->ipInterfaceName());
        }
        item->setDevicePath(device->uni());
        item->setName(network->ssid());
        item->setSignal(network->signalStrength());
        item->setSpecificPath(network->referenceAccessPoint()->uni());
        item->setSsid(network->ssid());
        item->setType(NetworkManager::ConnectionSettings::Wireless);

        NetworkManager::AccessPoint::Ptr ap = network->referenceAccessPoint();
        if (ap && ap->capabilities() & NetworkManager::AccessPoint::Privacy) {
            item->setSecurityType(NetworkManager::Utils::findBestWirelessSecurity(device->wirelessCapabilities(), true, (device->mode() == NetworkManager::WirelessDevice::Adhoc),
                                                                                  ap->capabilities(), ap->wpaFlags(), ap->rsnFlags()));
        }

        item->updateDetails();
        const int index = m_list.count();
        beginInsertRows(QModelIndex(), index, index);
        m_list.insertItem(item);
        endInsertRows();
    }
}

void NetworkModel::updateItems()
{
    foreach (NetworkModelItem * item, m_list.items()) {
        updateItem(item);
    }
}

void NetworkModel::updateItem(NetworkModelItem * item)
{
    const int row = m_list.indexOf(item);

    if (row >= 0) {
        item->updateDetails();
        QModelIndex index = createIndex(row, 0);
        emit dataChanged(index, index);
    }
}

void NetworkModel::activeConnectionAdded(const QString& activeConnection)
{
    //TODO remove debug
    qDebug() << "Adding active connection " << activeConnection;

    NetworkManager::ActiveConnection::Ptr activeCon = NetworkManager::findActiveConnection(activeConnection);

    if (activeCon) {
        addActiveConnection(activeCon);
    }
}

void NetworkModel::activeConnectionRemoved(const QString& activeConnection)
{
    //TODO remove debug
    qDebug() << "Removing active connection " << activeConnection;

    foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::ActiveConnection, activeConnection)) {
        item->setActiveConnectionPath(QString());
        item->setConnectionState(NetworkManager::ActiveConnection::Deactivated);
        if (item->type() == NetworkManager::ConnectionSettings::Wired || item->type() == NetworkManager::ConnectionSettings::Wireless) {
            item->setSpeed(0);
        } else if (item->type() == NetworkManager::ConnectionSettings::Gsm) {
            item->setSignal(0);
        }
        updateItem(item);
    }
}

void NetworkModel::activeConnectionStateChanged(NetworkManager::ActiveConnection::State state)
{
    NetworkManager::ActiveConnection * activePtr = qobject_cast<NetworkManager::ActiveConnection*>(sender());
    if (activePtr) {
        foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::ActiveConnection, activePtr->path())) {
            item->setConnectionState(state);
            updateItem(item);
        }
    }
}

void NetworkModel::activeVpnConnectionStateChanged(NetworkManager::VpnConnection::State state, NetworkManager::VpnConnection::StateChangeReason reason)
{
    Q_UNUSED(reason)
    NetworkManager::ActiveConnection *activePtr = qobject_cast<NetworkManager::ActiveConnection*>(sender());

    if (activePtr) {
        foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::ActiveConnection, activePtr->path())) {
            if (state == NetworkManager::VpnConnection::Prepare ||
                state == NetworkManager::VpnConnection::NeedAuth ||
                state == NetworkManager::VpnConnection::Connecting ||
                state == NetworkManager::VpnConnection::GettingIpConfig) {
                item->setConnectionState(NetworkManager::ActiveConnection::Activating);
            } else if (state == NetworkManager::VpnConnection::Activated) {
                item->setConnectionState(NetworkManager::ActiveConnection::Activated);
            } else {
                item->setConnectionState(NetworkManager::ActiveConnection::Deactivated);
            }
            updateItem(item);
        }
    }
}

void NetworkModel::availableConnectionAppeared(const QString& connection)
{
    //TODO remove debug
    qDebug() << "Adding available connection " << connection;

    NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(qobject_cast<NetworkManager::Device*>(sender())->uni());

    foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::Connection, connection)) {
        if (device->ipInterfaceName().isEmpty()) {
            item->setDeviceName(device->interfaceName());
        } else {
            item->setDeviceName(device->ipInterfaceName());
        }
        item->setDevicePath(device->uni());
        updateItem(item);
    }
}

void NetworkModel::availableConnectionDisappeared(const QString& connection)
{
    //TODO remove debug
    qDebug() << "Removing available connection " << connection;

    foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::Connection, connection)) {
        item->setDeviceName(QString());
        item->setDevicePath(QString());
        item->setSignal(0);
        item->setSpecificPath(QString());
        updateItem(item);
    }

    // TODO split the item to a connection and access point ???
}

void NetworkModel::bitrateChanged(int bitrate)
{
    NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(qobject_cast<NetworkManager::Device*>(sender())->uni());

    foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::Device, device->uni())) {
        if ((item->connectionState() == NetworkManager::ActiveConnection::Activated || item->connectionState() == NetworkManager::ActiveConnection::Activating) &&
            (item->type() == NetworkManager::ConnectionSettings::Wired || item->type() == NetworkManager::ConnectionSettings::Wireless)) {
            item->setSpeed(bitrate);
            updateItem(item);
        }
    }
}

void NetworkModel::connectionAdded(const QString& connection)
{
    // TODO remove debug
    qDebug() << "Connectin added " << connection;

    NetworkManager::Connection::Ptr newConnection = NetworkManager::findConnection(connection);
    if (newConnection) {
        addConnection(newConnection);
    }
}

void NetworkModel::connectionRemoved(const QString& connection)
{
    // TODO remove debug
    qDebug() << "Removing connection " << connection;

    foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::Connection, connection)) {
        // When the item type is wireless, we can remove only the connection and leave it as an available access point
        if (item->type() == NetworkManager::ConnectionSettings::Wireless && !item->devicePath().isEmpty()) {
            item->setConnectionPath(QString());
            item->setLastUsed(QDateTime());
            item->setName(item->ssid());
            item->setUuid(QString());
            updateItem(item);
        } else {
            const int row = m_list.indexOf(item);
            if (row >= 0) {
                beginRemoveRows(QModelIndex(), row, row);
                m_list.removeItem(item);
                item->deleteLater();
                endRemoveRows();
            }
        }
    }
}

void NetworkModel::connectionUpdated()
{
    NetworkManager::Connection * connectionPtr = qobject_cast<NetworkManager::Connection*>(sender());
    if (connectionPtr) {
        NetworkManager::ConnectionSettings::Ptr settings = connectionPtr->settings();
        foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::Connection, connectionPtr->path())) {
            item->setConnectionPath(connectionPtr->path());
            item->setLastUsed(settings->timestamp());
            item->setName(settings->id());
            item->setType(settings->connectionType());
            item->setUuid(settings->uuid());

            if (item->type() == NetworkManager::ConnectionSettings::Wireless) {
                NetworkManager::WirelessSetting::Ptr wirelessSetting;
                wirelessSetting = settings->setting(NetworkManager::Setting::Wireless).dynamicCast<NetworkManager::WirelessSetting>();
                item->setSecurityType(NetworkManager::Utils::securityTypeFromConnectionSetting(settings));
                if (wirelessSetting->mode() == NetworkManager::WirelessSetting::Adhoc ||
                    wirelessSetting->mode() == NetworkManager::WirelessSetting::Ap) {
                    item->setShared(true);
                } else {
                    item->setShared(false);
                }
                item->setSsid(wirelessSetting->ssid());
            }
            updateItem(item);
        }
    }
}

void NetworkModel::deviceAdded(const QString& device)
{
    // TODO: remove debug
    qDebug() << "Device added " << device;

    NetworkManager::Device::Ptr dev = NetworkManager::findNetworkInterface(device);
    if (dev) {
        addDevice(dev);
    }
}

void NetworkModel::deviceRemoved(const QString& device)
{
    // TODO: remove debug
    qDebug() << "Device removed " << device;

    // Make all items unavailable
    foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::Device, device)) {
        item->setDeviceName(QString());
        item->setDevicePath(QString());
        updateItem(item);
    }
}

#if WITH_MODEMMANAGER_SUPPORT
#ifdef MODEMMANAGERQT_ONE
void NetworkModel::gsmNetworkAccessTechnologyChanged(ModemManager::Modem::AccessTechnologies technology)
#else
void NetworkModel::gsmNetworkAccessTechnologyChanged(ModemManager::ModemInterface::AccessTechnology technology)
#endif
{
#ifdef MODEMMANAGERQT_ONE
#else
#endif
    // TODO
    Q_UNUSED(technology);
}

#ifdef MODEMMANAGERQT_ONE
void NetworkModel::gsmNetworkCurrentModesChanged()
#else
void NetworkModel::gsmNetworkAllowedModeChanged(ModemManager::ModemInterface::AllowedMode mode)
#endif
{
#ifdef MODEMMANAGERQT_ONE
#else
#endif
    // TODO
}

void NetworkModel::gsmNetworkSignalQualityChanged(uint signal)
{
#ifdef MODEMMANAGERQT_ONE
#else
#endif

#ifdef MODEMMANAGERQT_ONE
#else
#endif
    // TODO
    Q_UNUSED(signal);
}
#endif

void NetworkModel::statusChanged(NetworkManager::Status status)
{
    Q_UNUSED(status);

    // This have probably effect only for VPN connections
    foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::Type, NetworkManager::ConnectionSettings::Vpn)) {
        updateItem(item);
    }
}

void NetworkModel::wirelessNetworkAppeared(const QString& ssid)
{
    // TODO: remove debug
    qDebug() << "Wireless network appeared " << ssid;

    NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(qobject_cast<NetworkManager::Device*>(sender())->uni());
    if (device && device->type() == NetworkManager::Device::Wifi) {
        NetworkManager::WirelessDevice::Ptr wirelessDevice = device.objectCast<NetworkManager::WirelessDevice>();
        NetworkManager::WirelessNetwork::Ptr network = wirelessDevice->findNetwork(ssid);
        addWirelessNetwork(network, wirelessDevice);
    }
}

void NetworkModel::wirelessNetworkDisappeared(const QString& ssid)
{
    // TODO: remove debug
    qDebug() << "Wireless network disappeared " << ssid;

    NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(qobject_cast<NetworkManager::Device*>(sender())->uni());
    if (device) {
        foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::Ssid, ssid, device->uni())) {
            // Remove the entire item, because it's only AP
            if (item->connectionPath().isEmpty()) {
                const int row = m_list.indexOf(item);
                if (row >= 0) {
                    beginRemoveRows(QModelIndex(), row, row);
                    m_list.removeItem(item);
                    item->deleteLater();
                    endRemoveRows();
                }
            // Remove only AP and device from the item and leave it as an unavailable connection
            } else {
                item->setDeviceName(QString());
                item->setDevicePath(QString());
                item->setSignal(0);
                item->setSpecificPath(QString());
            }
        }
    }
}

void NetworkModel::wirelessNetworkReferenceApChanged(const QString& accessPoint)
{
    NetworkManager::WirelessNetwork * networkPtr = qobject_cast<NetworkManager::WirelessNetwork*>(sender());

    if (networkPtr) {
        // FIXME not sure about it, might be wrong when using more wireless cards
        foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::Ssid, networkPtr->ssid())) {
            item->setSpecificPath(accessPoint);
            updateItem(item);
        }
    }
}

void NetworkModel::wirelessNetworkSignalChanged(int signal)
{
    NetworkManager::WirelessNetwork * networkPtr = qobject_cast<NetworkManager::WirelessNetwork*>(sender());
    if (networkPtr) {
        // TODO remove debug
        qDebug() << "Wireless network " << networkPtr->ssid() << " signal changed - " << signal;

        // FIXME not sure about it, might be wrong when using more wireless cards
        foreach (NetworkModelItem * item, m_list.returnItems(NetworkItemsList::Ssid, networkPtr->ssid())) {
            item->setSignal(signal);
            updateItem(item);
        }
    }
}