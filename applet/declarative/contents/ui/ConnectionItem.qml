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

import QtQuick 1.1
import org.kde.qtextracomponents 0.1
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasmanm 0.1 as PlasmaNM

Item {
    id: connectionItem;

    property bool isWireless: (itemType == 14) ? true : false;
    property bool expanded: false;

//     Item {
//         id: priv;
//         property Item detailWidget;
//     }

    signal itemExpanded(string connectionPath, bool itemExpanded);
//     signal removeConnectionItem(string connectionName, string connectionPath);

    height: 35;
    anchors { left: parent.left; right: parent.right }

    QIconItem {
        id: connectionTypeIcon;

        height: 30; width: 25;
        anchors { left: parent.left; top: parent.top; leftMargin: 5 }
        icon: QIcon(itemConnectionIcon);

        QIconItem {
            id: connectionSecurityIcon;
            width: 15; height: 15;
            anchors { bottom: parent.bottom; right: parent.right }
            icon: QIcon("object-locked");
            visible: itemSecure;
        }
    }

    PlasmaComponents.Label {
        id: connectionNameLabel;

        height: 30;
        anchors { left: connectionTypeIcon.right; right: parent.right; top: parent.top; leftMargin: 5; rightMargin: 30 }
        text: itemName;
        elide: Text.ElideRight;
        font.weight: itemConnected ? Font.DemiBold : Font.Normal;
        font.italic: itemConnecting ? true : false;
    }

//     PlasmaComponents.BusyIndicator {
//         id: connectingIndicator;
//
//         anchors { right: parent.right; top: parent.top; rightMargin: 5 }
//         width: 30; height: 30;
//         running: itemConnecting;
//         visible: running;
//     }

    MouseArea {
        id: connectionItemMouseArea;

        anchors.fill: parent;
        hoverEnabled: true;

//         PlasmaCore.IconItem {
//             id: configureButton;
//
//             width: 30; height: 30;
//             anchors { right: parent.right; top: parent.top; rightMargin: 5 }
//             source: "configure";
//             visible: false;
//
//             MouseArea {
//                 id: configureButtonMouseArea;
//
//                 anchors.fill: parent;
//
//                 onClicked: {
//                     if (!expanded) {
//                         itemExpanded();
//                         expanded = !expanded;
//                     // Item may be set as expanded, but was closed from the toolbar
//                     } else if (expanded && connectionView.itemExpandable == false && toolbar.toolbarExpandable == true) {
//                         itemExpanded();
//                     } else {
//                         expanded = !expanded;
//                     }
//                 }
//             }
//         }

        onClicked: {
//             if (!itemConnected && !itemConnecting) {
//                 if (itemUuid) {
//                     handler.activateConnection(itemConnectionPath, itemDevicePath, itemSpecificPath);
//                 } else {
//                     handler.addAndActivateConnection(itemDevicePath, itemSpecificPath);
//                 }
//             } else {
//                 handler.deactivateConnection(itemConnectionPath);
//             }
//
//             expanded = !expanded;
            console.log(expanded);
            if (itemUuid != "") {
                itemExpanded(itemConnectionPath, !expanded);
            } else {
                itemExpanded(itemName, !expanded);
            }
        }

        onEntered: {
            connectionView.currentIndex = index;
//             configureButton.visible = true;
        }
//
//         onExited: {
//             configureButton.visible = false;
//         }
    }

//     Component {
//         id: detailWidgetComponent;
//         DetailsWidget {
//
//             anchors {
//                 left: parent.left;
//                 right: parent.right;
//                 top: connectionTypeIcon.bottom;
//                 bottom: parent.bottom;
//                 topMargin: 5;
//                 leftMargin: 10;
//                 rightMargin: 10
//                 bottomMargin: 5;
//             }
//             text: itemDetails;
//             editable: itemUuid == "" ? false : true;
//             enableTraffic: {
//                 if (itemDevicePath != "" && itemConnected && itemType != 11) {
//                     true;
//                 } else {
//                     false;
//                 }
//             }
//             device: itemDevicePath;
//
//             onHideDetails: {
//                 expanded = false;
//                 configureButton.visible = false;
//             }
//
//             onEditConnection: {
//                 handler.editConnection(itemUuid);
//             }
//
//             onRemoveConnection: {
//                 connectionItem.removeConnectionItem(itemName, itemConnectionPath);
//             }
//         }
//     }

    //  For active connection
    Item {
        id: activeConnectionItem;
        property Item activeConnectionSettings;
    }

    Component {
        id: activeConnectionComponent;

        Item {
            anchors { left: parent.left; right: parent.right; top: connectionTypeIcon.bottom; topMargin: 5 }

            PlasmaComponents.Button {
                id: disconnectButton;

                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }
                text: i18n("Disconnect");

                onClicked: {
                    itemExpanded(itemConnectionPath, false);
                    handler.deactivateConnection(itemConnectionPath);
                }
            }

            PlasmaNM.TrafficMonitor {
                id: trafficMonitor;

                visible: (itemDevicePath != "" && itemConnected && itemType != 1)
                height: visible ? 100 : 0;
                device: itemDevicePath;
                anchors { top: disconnectButton.bottom; left: parent.left; right: parent.right; topMargin: 5 }
            }
        }
    }

    //  For previous connection
    Item {
        id: previousConnectionItem;
        property Item previousConnectionSettings;
    }

    Component {
        id: previousConnectionComponent;

        PlasmaComponents.Button {
            anchors { horizontalCenter: parent.horizontalCenter; top: connectionTypeIcon.bottom; topMargin: 5 }
            text: i18n("Connect");

            onClicked: {
                itemExpanded(itemConnectionPath, false);
                handler.activateConnection(itemConnectionPath, itemDevicePath, itemSpecificPath);
            }
        }
    }

    //  For unknown connection
    Item {
        id: unknownConnectionItem;
        property Item unknownConnectionSettings;
    }

    Component {
        id: unknownConnectionComponent;

        PlasmaComponents.Button {
            anchors { horizontalCenter: parent.horizontalCenter; top: connectionTypeIcon.bottom; topMargin: 5 }
            text: i18n("Connect");

            onClicked: {
                itemExpanded(itemName, false);
                handler.addAndActivateConnection(itemDevicePath, itemSpecificPath);
            }
        }
    }

    states: [
        State {
            name: "Collapsed";
            when: /*(!expanded || !connectionView.itemExpandable) &&*/
                  !expanded && !sectionHidden();
//             StateChangeScript { script: if (priv.detailWidget) {priv.detailWidget.destroy()} }
                StateChangeScript { script: if (activeConnectionItem.activeConnectionSettings) {activeConnectionItem.activeConnectionSettings.destroy()} }
                StateChangeScript { script: if (previousConnectionItem.previousConnectionSettings) {previousConnectionItem.previousConnectionSettings.destroy()} }
                StateChangeScript { script: if (unknownConnectionItem.unknownConnectionSettings) {unknownConnectionItem.unknownConnectionSettings.destroy()} }
        },

        State {
            name: "CollapsedHidden";
            when: /*(!expanded || !connectionView.itemExpandable) &&*/ sectionHidden();
//             StateChangeScript { script: if (priv.detailWidget) {priv.detailWidget.destroy()} }
            StateChangeScript { script: if (activeConnectionItem.activeConnectionSettings) {activeConnectionItem.activeConnectionSettings.destroy()} }
            StateChangeScript { script: if (previousConnectionItem.previousConnectionSettings) {previousConnectionItem.previousConnectionSettings.destroy()} }
            StateChangeScript { script: if (unknownConnectionItem.unknownConnectionSettings) {unknownConnectionItem.unknownConnectionSettings.destroy()} }
            PropertyChanges { target: connectionItem; height: 0; }
            PropertyChanges { target: connectionItem; visible: false; }
        },

        State {
            name: "ActiveConnectionExpanded";
            when: expanded && (itemConnected || itemConnecting) && !sectionHidden();
            StateChangeScript { script: activeConnectionItem.activeConnectionSettings = activeConnectionComponent.createObject(connectionItem); }
            PropertyChanges { target: connectionItem; height: (itemDevicePath != "" && itemConnected && itemType != 1) ? 220 : 60; }
            StateChangeScript { script: if (previousConnectionItem.previousConnectionSettings) {previousConnectionItem.previousConnectionSettings.destroy()} }
            StateChangeScript { script: if (unknownConnectionItem.unknownConnectionSettings) {unknownConnectionItem.unknownConnectionSettings.destroy()} }                StateChangeScript { script: console.log("Active") }
        },

        State {
            name: "PreviousConnectionExpanded";
            when: expanded && !itemConnected && itemUuid != "" && !sectionHidden();
            StateChangeScript { script: previousConnectionItem.previousConnectionSettings = previousConnectionComponent.createObject(connectionItem); }
            PropertyChanges { target: connectionItem; height: 60; }
            StateChangeScript { script: if (activeConnectionItem.activeConnectionSettings) {activeConnectionItem.activeConnectionSettings.destroy()} }
            StateChangeScript { script: if (unknownConnectionItem.unknownConnectionSettings) {unknownConnectionItem.unknownConnectionSettings.destroy()} }                StateChangeScript { script: console.log("Previous") }
        },

        State {
            name: "UknownConnectionExpanded";
            when: expanded && !itemConnected && itemUuid == "" && !sectionHidden();
            StateChangeScript { script: unknownConnectionItem.unknownConnectionSettings = unknownConnectionComponent.createObject(connectionItem); }
            StateChangeScript { script: if (activeConnectionItem.activeConnectionSettings) {activeConnectionItem.activeConnectionSettings.destroy()} }
            StateChangeScript { script: if (previousConnectionItem.previousConnectionSettings) {previousConnectionItem.previousConnectionSettings.destroy()} }
            PropertyChanges { target: connectionItem; height: 60; }
        }

//         State {
//             name: "Details";
//             when: (expanded && connectionView.itemExpandable);
//             PropertyChanges { target: connectionItem; height: connectionItem.ListView.view.height }
//             PropertyChanges { target: connectionItem.ListView.view; interactive: false }
//             PropertyChanges { target: connectionItem.ListView.view; contentY: connectionItem.y }
//             PropertyChanges { target: connectionItem.ListView.view; currentIndex: -1 }
//             PropertyChanges { target: connectionItemMouseArea; hoverEnabled: false }
//             StateChangeScript { script: priv.detailWidget = detailWidgetComponent.createObject(connectionItem); }
//         },


    ]

    transitions: Transition {
        NumberAnimation { duration: 300; properties: "height, contentY" }
    }

    function sectionHidden() {
        return ((!connectionView.activeExpanded && itemSection == i18n("Active connections")) ||
                (!connectionView.previousExpanded && itemSection == i18n("Previous connections")) ||
                (!connectionView.unknownExpanded && itemSection == i18n("Unknown connections")))
    }
}
