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
import org.sailfishosopen 1.0

// The first start: Android is not set up yet. Setup (C++) runs Waydroid's
// own setup, which downloads Android's images, checks and unpacks them; then
// the runner starts Android.
SilicaFlickable {
    id: view

    readonly property bool running: setup.state == Setup.Running
    readonly property bool failed: setup.state == Setup.Failed
    readonly property bool cancelled: setup.state == Setup.Cancelled
    readonly property bool done: setup.state == Setup.Done
    readonly property bool enoughSpace: setup.bytesFree < 0 || setup.bytesFree >= setup.bytesNeeded

    function gigabytes(bytes) {
        return (bytes / 1e9).toLocaleString(Qt.locale(), 'f', 1)
    }

    contentHeight: column.height + Theme.paddingLarge

    Column {
        id: column
        width: parent.width
        spacing: Theme.paddingLarge

        PageHeader {
            title: qsTr("Android apps")
        }

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: Qt.resolvedUrl("icons/waydroid.png")
            sourceSize.width: Theme.iconSizeExtraLarge
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            text: qsTr("Waydroid runs Android and its apps on this phone. Android is set up once: about 1 GB is downloaded and unpacked, which needs about 3 GB of free space.")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryHighlightColor
            visible: !view.running && !view.done
            text: qsTr("This takes a few minutes; use Wi-Fi if your mobile data is limited. This Android has no Google services.")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            wrapMode: Text.Wrap
            color: Theme.errorColor
            visible: !view.enoughSpace && !view.running && !view.done
            text: qsTr("Only %1 GB are free. Free some space first.").arg(view.gigabytes(setup.bytesFree))
        }

        // Progress
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            color: Theme.highlightColor
            visible: view.running || view.done
            text: setup.step
        }

        ProgressBar {
            width: parent.width
            visible: view.running
            minimumValue: 0
            maximumValue: 1
            value: Math.max(0, setup.progress)
            indeterminate: setup.progress < 0
            valueText: setup.progress < 0 ? "" : Math.round(100 * setup.progress) + " %"
            label: setup.detail
        }

        // The end of a setup that did not succeed
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            wrapMode: Text.Wrap
            color: Theme.errorColor
            visible: view.failed
            text: setup.error
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryHighlightColor
            visible: view.failed && setup.errorDetail.length > 0
            text: setup.errorDetail
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            visible: view.cancelled
            text: qsTr("The setup was cancelled.")
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !view.done
            enabled: view.running || view.enoughSpace
            text: view.running ? qsTr("Cancel")
                               : (view.failed || view.cancelled ? qsTr("Try again") : qsTr("Set up Android"))
            onClicked: {
                if (view.running)
                    setup.cancel()
                else
                    setup.start()
            }
        }
    }

    VerticalScrollDecorator {}

    onVisibleChanged: {
        if (visible)
            setup.updateSpace()
    }
}
