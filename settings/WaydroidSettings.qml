/****************************************************************************
**
** Copyright (C) 2026 Johannes Meyer zum Alten Borgloh
**
** This file is part of Waydroid Runner.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of the copyright holder nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import com.jolla.settings 1.0
import org.sailfishosopen.waydroidrunner 1.0

// Settings > Apps > Waydroid. The running instance follows a change at once
// (Service::closeActionKey). Android can be removed here (Android, the
// package's QML plugin): Waydroid's packages are part of the image.
ApplicationSettings {
    id: page

    function gigabytes(bytes) {
        return (bytes / 1e9).toLocaleString(Qt.locale(), 'f', 1)
    }

    onStatusChanged: {
        if (status === PageStatus.Activating)
            android.refresh()
    }

    Android {
        id: android
    }

    ConfigurationValue {
        id: closeAction
        key: "/apps/waydroid-runner/close_action"
        defaultValue: "keep-running"
    }

    ComboBox {
        label: qsTr("Closing the window")
        currentIndex: closeAction.value === "stop" ? 1 : 0
        description: currentIndex === 1
                     ? qsTr("Stops Android. Opening the window starts it again, which takes about 15 seconds. No notifications while it is stopped.")
                     : qsTr("Android keeps running in the background: its apps can show notifications, and the window opens again at once.")

        menu: ContextMenu {
            MenuItem {
                text: qsTr("Keeps Android running")
                onClicked: closeAction.value = "keep-running"
            }
            MenuItem {
                text: qsTr("Stops Android")
                onClicked: closeAction.value = "stop"
            }
        }
    }

    SectionHeader {
        text: qsTr("Android")
    }

    Column {
        width: parent.width
        spacing: Theme.paddingLarge

        readonly property bool removing: android.state === Android.Removing

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            text: {
                if (parent.removing)
                    return qsTr("Removing Android")
                if (android.setUp)
                    return qsTr("Android is set up. Its images take %1 GB, its apps and their data come on top.")
                           .arg(page.gigabytes(android.bytesUsed))
                if (android.bytesUsed > 0)
                    return qsTr("An unfinished setup of Android left %1 GB of files.")
                           .arg(page.gigabytes(android.bytesUsed))
                if (android.state === Android.Removed)
                    return qsTr("Android has been removed. Waydroid offers to set it up again when you open it.")
                return qsTr("Android is not set up. Waydroid offers to set it up when you open it.")
            }
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            wrapMode: Text.Wrap
            color: Theme.errorColor
            visible: android.state === Android.Failed
            text: android.error
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryHighlightColor
            visible: android.state === Android.Failed && android.errorDetail.length > 0
            text: android.errorDetail
        }

        BusyIndicator {
            anchors.horizontalCenter: parent.horizontalCenter
            size: BusyIndicatorSize.Medium
            running: parent.removing
            visible: running
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !parent.removing && (android.setUp || android.bytesUsed > 0)
            text: qsTr("Remove Android")
            onClicked: pageStack.push(removeDialog)
        }
    }

    Component {
        id: removeDialog

        Dialog {
            onAccepted: android.remove()

            Column {
                width: parent.width
                spacing: Theme.paddingLarge

                DialogHeader {
                    acceptText: qsTr("Remove")
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2*x
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeExtraLarge
                    color: Theme.highlightColor
                    text: qsTr("Remove Android?")
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2*x
                    wrapMode: Text.Wrap
                    color: Theme.highlightColor
                    text: qsTr("Android stops, and its apps, their data and Android itself are deleted from this phone. This cannot be undone.")
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2*x
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryHighlightColor
                    text: qsTr("Waydroid can set Android up again afterwards, which downloads about 1 GB.")
                }
            }
        }
    }
}
