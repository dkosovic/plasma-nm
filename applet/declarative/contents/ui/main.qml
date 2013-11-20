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
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.networkmanagement 0.1 as PlasmaNM

Item {
    id: mainWindow;

    property int minimumWidth: 300;
    property int minimumHeight: 300;
    property bool autoHideOptions: true;
    property Component compactRepresentation: CompactRepresentation {
        Component.onCompleted: {
            plasmoid.addEventListener('configChanged', mainWindow.configChanged)
        }
    }

    signal sectionChanged();

//     function hideOptions() {
//         if (autoHideOptions) {
//             toolbar.expanded = false;
//         }
//     }

    PlasmaNM.GlobalConfig {
        id: globalConfig;

        onDetailKeysChanged: {
            connectionModel.updateItems();
        }

        onNetworkSpeedUnitChanged: {
            connectionModel.updateItems();
        }
    }

    PlasmaNM.Handler {
            id: handler;
    }

    PlasmaNM.Model {
        id: connectionModel;
    }

    PlasmaNM.SortModel {
        id: connectionSortModel;

        sourceModel: connectionModel;
    }

    PlasmaCore.FrameSvgItem {
        id: padding
        imagePath: "widgets/viewitem"
        prefix: "hover"
        opacity: 0
        anchors.fill: parent
    }

    Item {
        id: sizes;

        property int iconSize: theme.iconSizes.toolbar;
        property int itemSize: iconSize + padding.margins.top + padding.margins.bottom;
    }

    ListView {
        id: connectionView;

//         property bool expandedItem: false;
//         property variant previouslyExpandedItems: [];
//
//         property bool activeExpanded: true;
//         property bool previousExpanded: true;
//         property bool unknownExpanded: true;

//         anchors {
//             left: parent.left;
//             right: parent.right;
//             top: parent.top;
//             bottom: toolbarSeparator.top;
//             topMargin: padding.margins.top;
//             bottomMargin: padding.margins.bottom
//         }
        anchors.fill: parent;

        clip: true
        model: connectionSortModel;
        currentIndex: -1;
        interactive: true;
        boundsBehavior: Flickable.StopAtBounds;
        section.property: "itemSection";
        section.delegate: SectionHeader {
            onHideSection: {
                if (section == i18n("Active connections")) {
                    connectionView.activeExpanded = false;
                } else if (section == i18n("Previous connections")) {
                    connectionView.previousExpanded = false;
                } else {
                    connectionView.unknownExpanded = false;
                }
            }

            onShowSection: {
                if (section == i18n("Active connections")) {
                    connectionView.activeExpanded = true;
                } else if (section == i18n("Previous connections")) {
                    connectionView.previousExpanded = true;
                } else {
                    connectionView.unknownExpanded = true;
                }
            }
        }

        delegate: ConnectionItem {
//             expanded: connectionView.expandedItem && connectionView.containItem(itemUni);
//             onItemExpanded: {
//                 if (itemExpanded) {
//                     connectionView.expandedItem = true;
//                     connectionView.addItem(itemUni);
//                     connectionView.currentIndex = index;
//                 } else {
//                     connectionView.removeItem(itemUni);
//                     if (connectionView.previouslyExpandedItems.length == 0) {
//                         connectionView.expandedItem = false;
//                     }
//                 }
//             }
//
//             ListView.onRemove: {
//                 if (ListView.isCurrentItem) {
//                     connectionView.removeItem(itemUni);
//                 }
//             }
        }

//         function addItem(item) {
//             var currentItems = previouslyExpandedItems;
//             currentItems[currentItems.length] = item;
//             previouslyExpandedItems = currentItems;
//         }
//
//         function removeItem(item) {
//             var tmpItems = [];
//             for (var i = 0; i < previouslyExpandedItems.length; i++) {
//                 if (previouslyExpandedItems[i] != item) {
//                     tmpItems[tmpItems.length] = previouslyExpandedItems[i];
//                 }
//             }
//             previouslyExpandedItems = tmpItems;
//         }
//
//         function containItem(item) {
//             for (var i = 0; i < previouslyExpandedItems.length; i++) {
//                 if (previouslyExpandedItems[i] == item) {
//                     return true;
//                 }
//             }
//             return false;
//         }
    }

//     PlasmaCore.SvgItem {
//         id: toolbarSeparator
//
//         height: lineSvg.elementSize("horizontal-line").height;
//         width: parent.width;
//         anchors {
//             left: parent.left;
//             right: parent.right;
//             bottom: toolbar.top;
//             bottomMargin: padding.margins.bottom/2;
//             leftMargin: padding.margins.left;
//             rightMargin: padding.margins.right;
//         }
//         elementId: "horizontal-line";
//
//         svg: PlasmaCore.Svg {
//             id: lineSvg;
//             imagePath: "widgets/line";
//         }
//     }
//
//     Toolbar {
//         id: toolbar;
//
//         anchors {
//             left: parent.left;
//             right: parent.right;
//             bottom: parent.bottom;
//         }
//     }

    Component.onCompleted: {
        configChanged();
        plasmoid.popupEvent.connect(mainWindow.hideOptions);
        plasmoid.addEventListener('configChanged', mainWindow.configChanged)
    }

    function configChanged() {
        var keys;
        keys = plasmoid.readConfig("detailKeys");
        globalConfig.setDetailKeys(keys);
        var speedUnit;
        speedUnit = plasmoid.readConfig("networkSpeedUnit");
        globalConfig.setNetworkSpeedUnit(speedUnit);
//         autoHideOptions = plasmoid.readConfig("autoHideOptions");
    }
}
