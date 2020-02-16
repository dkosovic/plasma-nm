/*
    Copyright (c) 2013 Lukas Tinkl <ltinkl@redhat.com>

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

#include "ipv6widget.h"
#include "ui_ipv6.h"
#include "ipv6delegate.h"
#include "intdelegate.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QStandardItemModel>
#include <QItemSelection>
#include <QNetworkAddressEntry>

#include <KEditListWidget>
#include <KLocalizedString>

quint32 suggestNetmask(Q_IPV6ADDR ip)
{
    Q_UNUSED(ip);

    /*
    TODO: find out common IPv6-netmasks and make a complete function

    */
    quint32 netmask = 64;

    return netmask;
}

class IPv6Widget::Private
{
public:
    Private() : model(0,3)
    {
        QStandardItem * headerItem = new QStandardItem(i18nc("Header text for IPv6 address", "Address"));
        model.setHorizontalHeaderItem(0, headerItem);
        headerItem = new QStandardItem(i18nc("Header text for IPv6 prefix", "Prefix"));
        model.setHorizontalHeaderItem(1, headerItem);
        headerItem = new QStandardItem(i18nc("Header text for IPv6 gateway", "Gateway"));
        model.setHorizontalHeaderItem(2, headerItem);
    }
    QStandardItemModel model;
};


IPv6Widget::IPv6Widget(const NetworkManager::Setting::Ptr &setting, QWidget* parent, Qt::WindowFlags f):
    SettingWidget(setting, parent, f),
    m_ui(new Ui::IPv6Widget),
    d(new IPv6Widget::Private())
{
    m_ui->setupUi(this);

    m_ui->tableViewAddresses->setModel(&d->model);
    m_ui->tableViewAddresses->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_ui->tableViewAddresses->horizontalHeader()->setStretchLastSection(true);

    IpV6Delegate *ipDelegate = new IpV6Delegate(this);
    IntDelegate *prefixDelegate = new IntDelegate (0, 128, this);
    m_ui->tableViewAddresses->setItemDelegateForColumn(0, ipDelegate);
    m_ui->tableViewAddresses->setItemDelegateForColumn(1, prefixDelegate);
    m_ui->tableViewAddresses->setItemDelegateForColumn(2, ipDelegate);

    connect(m_ui->btnAdd, &QPushButton::clicked, this, &IPv6Widget::slotAddIPAddress);
    connect(m_ui->btnRemove, &QPushButton::clicked, this, &IPv6Widget::slotRemoveIPAddress);

    connect(m_ui->dnsMorePushButton, &QPushButton::clicked, this, &IPv6Widget::slotDnsServers);
    connect(m_ui->dnsSearchMorePushButton, &QPushButton::clicked, this, &IPv6Widget::slotDnsDomains);

    connect(m_ui->tableViewAddresses->selectionModel(), &QItemSelectionModel::selectionChanged, this, &IPv6Widget::selectionChanged);


    connect(&d->model, &QStandardItemModel::itemChanged, this, &IPv6Widget::tableViewItemChanged);

    if (setting) {
        loadConfig(setting);
    }

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    connect(m_ui->method, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &IPv6Widget::slotModeComboChanged);
#else
    connect(m_ui->method, QOverload<int, const QString &>::of(&KComboBox::currentIndexChanged), this, &IPv6Widget::slotModeComboChanged);
#endif
    slotModeComboChanged(m_ui->method->currentIndex());

    connect(m_ui->btnRoutes, &QPushButton::clicked, this, &IPv6Widget::slotRoutesDialog);

    // Connect for setting check
    watchChangedSetting();

    // Connect for validity check
    connect(m_ui->dns, &KLineEdit::textChanged, this, &IPv6Widget::slotWidgetChanged);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    connect(m_ui->method, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &IPv6Widget::slotWidgetChanged);
#else
    connect(m_ui->method, QOverload<int, const QString &>::of(&KComboBox::currentIndexChanged), this, &IPv6Widget::slotWidgetChanged);
#endif
    connect(&d->model, &QStandardItemModel::dataChanged, this, &IPv6Widget::slotWidgetChanged);
    connect(&d->model, &QStandardItemModel::rowsRemoved, this, &IPv6Widget::slotWidgetChanged);

    KAcceleratorManager::manage(this);
}

IPv6Widget::~IPv6Widget()
{
    delete d;
    delete m_ui;
}

void IPv6Widget::loadConfig(const NetworkManager::Setting::Ptr &setting)
{
    NetworkManager::Ipv6Setting::Ptr ipv6Setting = setting.staticCast<NetworkManager::Ipv6Setting>();

    // BUG:406118
    // We don't have route-metric in the UI, maybe even won't have for now, but that doesn't mean we
    // want to loose it when it's configured manually in a config file
    m_tmpIpv6Setting.setRouteMetric(ipv6Setting->routeMetric());

    m_tmpIpv6Setting.setRoutes(ipv6Setting->routes());
    m_tmpIpv6Setting.setNeverDefault(ipv6Setting->neverDefault());
    m_tmpIpv6Setting.setIgnoreAutoRoutes(ipv6Setting->ignoreAutoRoutes());

    // method
    switch (ipv6Setting->method()) {
        case NetworkManager::Ipv6Setting::Automatic:
            if (ipv6Setting->ignoreAutoDns()) {
                m_ui->method->setCurrentIndex(AutomaticOnlyIP);
            } else {
                m_ui->method->setCurrentIndex(Automatic);
            }
            break;
        case NetworkManager::Ipv6Setting::Dhcp:
            m_ui->method->setCurrentIndex(AutomaticOnlyDHCP);
            break;
        case NetworkManager::Ipv6Setting::Manual:
            m_ui->method->setCurrentIndex(Manual);
            break;
        case NetworkManager::Ipv6Setting::LinkLocal:
            m_ui->method->setCurrentIndex(LinkLocal);
            break;
        case NetworkManager::Ipv6Setting::Ignored:
            m_ui->method->setCurrentIndex(Disabled);
            break;
    }

    // dns
    QStringList tmp;
    for (const QHostAddress &addr : ipv6Setting->dns()) {
        tmp.append(addr.toString());
    }
    m_ui->dns->setText(tmp.join(","));
    m_ui->dnsSearch->setText(ipv6Setting->dnsSearch().join(","));

    // addresses
    for (const NetworkManager::IpAddress &address : ipv6Setting->addresses()) {
        QList<QStandardItem *> item;

        item << new QStandardItem(address.ip().toString())
             << new QStandardItem(QString::number(address.prefixLength(),10))
             << new QStandardItem(address.gateway().toString());

        d->model.appendRow(item);
    }

    // may-fail
    m_ui->ipv6RequiredCB->setChecked(!ipv6Setting->mayFail());

    // privacy
    if (ipv6Setting->privacy() != NetworkManager::Ipv6Setting::Unknown) {
        m_ui->privacyCombo->setCurrentIndex(static_cast<int>(ipv6Setting->privacy()) + 1);
    }
}

QVariantMap IPv6Widget::setting() const
{
    NetworkManager::Ipv6Setting ipv6Setting;

    // BUG:406118
    // We don't have route-metric in the UI, maybe even won't have for now, but that doesn't mean we
    // want to loose it when it's configured manually in a config file
    ipv6Setting.setRouteMetric(m_tmpIpv6Setting.routeMetric());

    ipv6Setting.setRoutes(m_tmpIpv6Setting.routes());
    ipv6Setting.setNeverDefault(m_tmpIpv6Setting.neverDefault());
    ipv6Setting.setIgnoreAutoRoutes(m_tmpIpv6Setting.ignoreAutoRoutes());

    // method
    switch ((MethodIndex)m_ui->method->currentIndex()) {
        case Automatic:
            ipv6Setting.setMethod(NetworkManager::Ipv6Setting::Automatic);
            break;
        case AutomaticOnlyIP:
            ipv6Setting.setMethod(NetworkManager::Ipv6Setting::Automatic);
            ipv6Setting.setIgnoreAutoDns(true);
            break;
        case IPv6Widget::AutomaticOnlyDHCP:
            ipv6Setting.setMethod(NetworkManager::Ipv6Setting::Dhcp);
            break;
        case Manual:
            ipv6Setting.setMethod(NetworkManager::Ipv6Setting::Manual);
            break;
        case LinkLocal:
            ipv6Setting.setMethod(NetworkManager::Ipv6Setting::LinkLocal);
            break;
        case Disabled:
            ipv6Setting.setMethod(NetworkManager::Ipv6Setting::Ignored);
            break;
    }

    // dns
    if (m_ui->dns->isEnabled() && !m_ui->dns->text().isEmpty()) {
        QStringList tmp = m_ui->dns->text().split(',');
        QList<QHostAddress> tmpAddrList;
        for (const QString &str : tmp) {
            QHostAddress addr(str);
            if (!addr.isNull())
                tmpAddrList.append(addr);
        }
        ipv6Setting.setDns(tmpAddrList);
    }
    if (m_ui->dnsSearch->isEnabled() && !m_ui->dnsSearch->text().isEmpty()) {
        ipv6Setting.setDnsSearch(m_ui->dnsSearch->text().split(','));
    }

    // addresses
    if (m_ui->tableViewAddresses->isEnabled()) {
        QList<NetworkManager::IpAddress> list;
        for (int i = 0, rowCount = d->model.rowCount(); i < rowCount; i++) {
            NetworkManager::IpAddress address;
            address.setIp(QHostAddress(d->model.item(i, 0)->text()));
            address.setPrefixLength(d->model.item(i, 1)->text().toInt());
            address.setGateway(QHostAddress(d->model.item(i, 2)->text()));

            list << address;
        }
        ipv6Setting.setAddresses(list);
    }

    // may-fail
    if (m_ui->ipv6RequiredCB->isEnabled()) {
        ipv6Setting.setMayFail(!m_ui->ipv6RequiredCB->isChecked());
    }

    // privacy
    if (m_ui->privacyCombo->isEnabled() && m_ui->privacyCombo->currentIndex()) {
        ipv6Setting.setPrivacy(static_cast<NetworkManager::Ipv6Setting::IPv6Privacy>(m_ui->privacyCombo->currentIndex() - 1));
    }

    return ipv6Setting.toMap();
}

void IPv6Widget::slotModeComboChanged(int index)
{
    if (index == Automatic) {  // Automatic
        m_ui->dnsLabel->setText(i18n("Other DNS Servers:"));
        m_ui->dns->setEnabled(true);
        m_ui->dnsMorePushButton->setEnabled(true);
        m_ui->dnsSearch->setEnabled(true);
        m_ui->dnsSearchMorePushButton->setEnabled(true);
        m_ui->ipv6RequiredCB->setEnabled(true);
        m_ui->privacyCombo->setEnabled(true);
        m_ui->btnRoutes->setEnabled(true);
        m_ui->tableViewAddresses->setEnabled(false);
        m_ui->btnAdd->setEnabled(false);
        m_ui->btnRemove->setEnabled(false);
    } else if (index == AutomaticOnlyIP) {
        m_ui->dnsLabel->setText(i18n("DNS Servers:"));
        m_ui->dns->setEnabled(true);
        m_ui->dnsMorePushButton->setEnabled(true);
        m_ui->dnsSearch->setEnabled(true);
        m_ui->dnsSearchMorePushButton->setEnabled(true);
        m_ui->ipv6RequiredCB->setEnabled(true);
        m_ui->privacyCombo->setEnabled(true);
        m_ui->btnRoutes->setEnabled(true);
        m_ui->tableViewAddresses->setEnabled(false);
        m_ui->btnAdd->setEnabled(false);
        m_ui->btnRemove->setEnabled(false);
    } else if (index == Manual) {  // Manual
        m_ui->dnsLabel->setText(i18n("DNS Servers:"));
        m_ui->dns->setEnabled(true);
        m_ui->dnsMorePushButton->setEnabled(true);
        m_ui->dnsSearch->setEnabled(true);
        m_ui->dnsSearchMorePushButton->setEnabled(true);
        m_ui->ipv6RequiredCB->setEnabled(true);
        m_ui->privacyCombo->setEnabled(true);
        m_ui->btnRoutes->setEnabled(true);
        m_ui->tableViewAddresses->setEnabled(true);
        m_ui->btnAdd->setEnabled(true);
        m_ui->btnRemove->setEnabled(true);
    } else if (index == AutomaticOnlyDHCP || index == LinkLocal) {  // Link-local or DHCP
        m_ui->dnsLabel->setText(i18n("DNS Servers:"));
        m_ui->dns->setEnabled(false);
        m_ui->dnsMorePushButton->setEnabled(false);
        m_ui->dnsSearch->setEnabled(false);
        m_ui->dnsSearchMorePushButton->setEnabled(false);
        m_ui->ipv6RequiredCB->setEnabled(true);
        m_ui->privacyCombo->setEnabled(true);
        m_ui->btnRoutes->setEnabled(false);
        m_ui->tableViewAddresses->setEnabled(false);
        m_ui->btnAdd->setEnabled(false);
        m_ui->btnRemove->setEnabled(false);
    } else if (index == Disabled) {  // Ignored
        m_ui->dnsLabel->setText(i18n("DNS Servers:"));
        m_ui->dns->setEnabled(false);
        m_ui->dnsMorePushButton->setEnabled(false);
        m_ui->dnsSearch->setEnabled(false);
        m_ui->dnsSearchMorePushButton->setEnabled(false);
        m_ui->ipv6RequiredCB->setEnabled(false);
        m_ui->privacyCombo->setEnabled(false);
        m_ui->btnRoutes->setEnabled(false);
        m_ui->tableViewAddresses->setEnabled(false);
        m_ui->btnAdd->setEnabled(false);
        m_ui->btnRemove->setEnabled(false);
    }
}

void IPv6Widget::slotAddIPAddress()
{
    QList<QStandardItem *> item;
    item << new QStandardItem << new QStandardItem << new QStandardItem;
    d->model.appendRow(item);

    const int rowCount = d->model.rowCount();
    if (rowCount > 0) {
        m_ui->tableViewAddresses->selectRow(rowCount - 1);

        QItemSelectionModel * selectionModel = m_ui->tableViewAddresses->selectionModel();
        QModelIndexList list = selectionModel->selectedIndexes();
        if (list.size()) {
            // QTableView is configured to select only rows.
            // So, list[0] - IP address.
            m_ui->tableViewAddresses->edit(list[0]);
        }
    }
}

void IPv6Widget::slotRemoveIPAddress()
{
    QItemSelectionModel * selectionModel = m_ui->tableViewAddresses->selectionModel();
    if (selectionModel->hasSelection()) {
        QModelIndexList indexes = selectionModel->selectedIndexes();
        d->model.takeRow(indexes[0].row());
    }
    m_ui->btnRemove->setEnabled(m_ui->tableViewAddresses->selectionModel()->hasSelection());
}

void IPv6Widget::selectionChanged(const QItemSelection & selected)
{
    m_ui->btnRemove->setEnabled(!selected.isEmpty());
}

void IPv6Widget::tableViewItemChanged(QStandardItem *item)
{
    if (item->text().isEmpty()) {
        return;
    }

    const int column = item->column();
    if (column == 0) { // ip
        int row = item->row();

        QStandardItem *netmaskItem = d->model.item(row, column + 1); // netmask
        if (netmaskItem && netmaskItem->text().isEmpty()) {
            QHostAddress addr(item->text());
            const quint32 netmask = suggestNetmask(addr.toIPv6Address());
            if (netmask) {
                netmaskItem->setText(QString::number(netmask,10));
            }
        }
    }
}

void IPv6Widget::slotRoutesDialog()
{
    QPointer<IpV6RoutesWidget> dlg = new IpV6RoutesWidget(this);

    dlg->setRoutes(m_tmpIpv6Setting.routes());
    dlg->setNeverDefault(m_tmpIpv6Setting.neverDefault());
    if (m_ui->method->currentIndex() == 3) {  // manual
        dlg->setIgnoreAutoRoutesCheckboxEnabled(false);
    } else {
        dlg->setIgnoreAutoRoutes(m_tmpIpv6Setting.ignoreAutoRoutes());
    }

    connect(dlg.data(), &QDialog::accepted,
            [dlg, this] () {
                m_tmpIpv6Setting.setRoutes(dlg->routes());
                m_tmpIpv6Setting.setNeverDefault(dlg->neverDefault());
                m_tmpIpv6Setting.setIgnoreAutoRoutes(dlg->ignoreautoroutes());
            });
    connect(dlg.data(), &QDialog::finished,
            [dlg] () {
                if (dlg) {
                    dlg->deleteLater();
                }
            });
    dlg->setModal(true);
    dlg->show();
}

void IPv6Widget::slotDnsServers()
{
    QPointer<QDialog> dialog = new QDialog(this);
    dialog->setWindowTitle(i18n("Edit DNS servers"));
    dialog->setLayout(new QVBoxLayout);
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, dialog);
    connect(buttons, &QDialogButtonBox::accepted, dialog.data(), &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dialog.data(), &QDialog::reject);
    KEditListWidget * listWidget = new KEditListWidget(dialog);
    listWidget->setItems(m_ui->dns->text().split(',').replaceInStrings(" ", ""));
    listWidget->lineEdit()->setFocus(Qt::OtherFocusReason);
    dialog->layout()->addWidget(listWidget);
    dialog->layout()->addWidget(buttons);
    connect(dialog.data(), &QDialog::accepted,
            [listWidget, this] () {
                QString text = listWidget->items().join(",");
                if (text.endsWith(',')) {
                    text.chop(1);
                }
                m_ui->dns->setText(text);
            });
    connect(dialog.data(), &QDialog::finished,
            [dialog] () {
                if (dialog) {
                    dialog->deleteLater();
                }
            });
    dialog->setModal(true);
    dialog->show();
}

void IPv6Widget::slotDnsDomains()
{
    QPointer<QDialog> dialog = new QDialog(this);
    dialog->setWindowTitle(i18n("Edit DNS search domains"));
    dialog->setLayout(new QVBoxLayout);
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, dialog);
    connect(buttons, &QDialogButtonBox::accepted, dialog.data(), &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dialog.data(), &QDialog::reject);
    KEditListWidget * listWidget = new KEditListWidget(dialog);
    listWidget->setItems(m_ui->dnsSearch->text().split(',').replaceInStrings(" ", ""));
    listWidget->lineEdit()->setFocus(Qt::OtherFocusReason);
    dialog->layout()->addWidget(listWidget);
    dialog->layout()->addWidget(buttons);
    connect(dialog.data(), &QDialog::accepted,
            [listWidget, this] () {
                QString text = listWidget->items().join(",");
                if (text.endsWith(',')) {
                    text.chop(1);
                }
                m_ui->dnsSearch->setText(text);
            });
    connect(dialog.data(), &QDialog::finished,
            [dialog] () {
                if (dialog) {
                    dialog->deleteLater();
                }
            });
    dialog->setModal(true);
    dialog->show();
}

bool IPv6Widget::isValid() const
{
    if (m_ui->method->currentIndex() == Manual) {
        if (!d->model.rowCount()) {
            return false;
        }

        for (int i = 0, rowCount = d->model.rowCount(); i < rowCount; i++) {
            QHostAddress ip = QHostAddress(d->model.item(i, 0)->text());
            const int prefix = d->model.item(i,1)->text().toInt();
            QHostAddress gateway = QHostAddress(d->model.item(i, 2)->text());

            if (ip.isNull() || !(prefix >= 1 && prefix <= 128) || (gateway.isNull() && !d->model.item(i, 2)->text().isEmpty())) {
                return false;
            }
        }
    }

    if (!m_ui->dns->text().isEmpty() && (m_ui->method->currentIndex() == Automatic || m_ui->method->currentIndex() == Manual || m_ui->method->currentIndex() == AutomaticOnlyIP)) {
        const QStringList tmp = m_ui->dns->text().split(',');
        for (const QString &str : tmp) {
            QHostAddress addr(str);
            if (addr.isNull()) {
                return false;
            }
        }
    }

    return true;
}
