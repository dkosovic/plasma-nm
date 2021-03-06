/*
    Copyright 2013 Jan Grulich <jgrulich@redhat.com>
    Copyright 2020 Douglas Kosovic <doug@uq.edu.au>

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

#ifndef PLASMA_NM_L2TP_PPP_WIDGET_H
#define PLASMA_NM_L2TP_PPP_WIDGET_H

#include <QDialog>

#include <NetworkManagerQt/VpnSetting>

namespace Ui
{
class L2tpPppWidget;
}

class L2tpPPPWidget : public QDialog
{
    Q_OBJECT
public:
    explicit L2tpPPPWidget(const NetworkManager::VpnSetting::Ptr &setting, QWidget *parent = nullptr, bool need_peer_eap = false);
    ~L2tpPPPWidget() override;

    NMStringMap setting() const;

private:
    void loadConfig(const NetworkManager::VpnSetting::Ptr &setting);
    Ui::L2tpPppWidget * m_ui;
    bool m_need_peer_eap;
};

#endif // PLASMA_NM_L2TP_PPP_WIDGET_H
