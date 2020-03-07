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

#include "libreswan.h"

#include <KPluginFactory>

#include "libreswanwidget.h"
#include "libreswanauth.h"

K_PLUGIN_CLASS_WITH_JSON(LibreswanUiPlugin, "plasmanetworkmanagement_libreswanui.json")

LibreswanUiPlugin::LibreswanUiPlugin(QObject * parent, const QVariantList &)
    : VpnUiPlugin(parent)
{
}

LibreswanUiPlugin::~LibreswanUiPlugin()
{
}

SettingWidget * LibreswanUiPlugin::widget(const NetworkManager::VpnSetting::Ptr &setting, QWidget * parent)
{
    return new LibreswanWidget(setting, parent);
}

SettingWidget * LibreswanUiPlugin::askUser(const NetworkManager::VpnSetting::Ptr &setting, QWidget * parent)
{
    return new LibreswanAuthDialog(setting, parent);
}

QString LibreswanUiPlugin::suggestedFileName(const NetworkManager::ConnectionSettings::Ptr &connection) const
{
    Q_UNUSED(connection);
    return QString();
}

QString LibreswanUiPlugin::supportedFileExtensions() const
{
    return QString();
}

NMVariantMapMap LibreswanUiPlugin::importConnectionSettings(const QString &fileName)
{
    Q_UNUSED(fileName);

    // TODO : import the Libreswan connection from file and return settings
    mError = VpnUiPlugin::NotImplemented;
    return NMVariantMapMap();
}

bool LibreswanUiPlugin::exportConnectionSettings(const NetworkManager::ConnectionSettings::Ptr &connection, const QString &fileName)
{
    Q_UNUSED(connection);
    Q_UNUSED(fileName);

    // TODO : export Libreswan connection to file
    mError = VpnUiPlugin::NotImplemented;
    return false;
}

#include "libreswan.moc"
