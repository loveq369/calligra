import QtQuick 1.1
import DocumentView 1.0

Page {
    id: documentPage

    function openFile(file) {
        fileLabel.text = file

        documentViewItem.openFile(file)

        documentViewItem.scale = 1.0
        documentViewItem.x = 0.0
        documentViewItem.y = 0.0
        flickable.contentX = 0.0
        flickable.contentY = 0.0
    }

    signal openClicked()

    ButtonBar {
        id: buttonBar
        PageStack {
            id: barPageStack
            height: buttonRow.height
            Page {
                id: buttonPage
                Row {
                    id: buttonRow
                    width: parent.width
                    spacing: 5
                    Button {
                        id: fileButton
                        //iconSource: "qrc:///images/file.png"
                        text: "Open"
                        //font.pointSize: 9
                        onClicked: {
                            documentPage.openClicked()
                        }
                    }
                    Label {
                        id: fileLabel
                        elide: Text.ElideLeft
                        verticalAlignment: Text.AlignVCenter
                        height: fileButton.height
                        width: buttonRow.width - buttonRow.spacing - fileButton.width
                    }
                }
            }
            Page {
                id: progressPage
                ProgressBar {
                    id: progressBar
                    height: parent.height
                    value: 0
                }
            }
            Component.onCompleted: {
                barPageStack.push(buttonPage, true)
            }
        }
    }

    Flickable {
        id: flickable
        anchors {
            top: buttonBar.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
            //margins: 5
        }
        contentHeight: documentViewItem.implicitHeight * documentViewItem.scale
        contentWidth: Math.max(documentViewItem.implicitWidth * documentViewItem.scale, parent.width)
        clip: true

        PinchArea {
            id: pinchArea
            width: Math.max(flickable.contentWidth, flickable.width)
            height: Math.max(flickable.contentHeight, flickable.height)

            function distance(p1, p2) {
                var dx = p2.x - p1.x;
                var dy = p2.y - p1.y;
                return Math.sqrt(dx*dx + dy*dy);
            }

            property real initialDistance

            onPinchStarted: {
                initialDistance = distance(pinch.point1, pinch.point2);
            }
            onPinchUpdated: {
                var currentDistance = distance(pinch.point1, pinch.point2);
                var scale = currentDistance / initialDistance;

                var widthChange = documentViewItem.width * scale - documentViewItem.width
                var heightChange = documentViewItem.height * scale - documentViewItem.height

                var vx = (pinch.center.x * scale) - (pinch.center.x - flickable.contentX) + (pinch.previousCenter.x - pinch.center.x)
                var vy = (pinch.center.y * scale) - (pinch.center.y - flickable.contentY) + (pinch.previousCenter.y - pinch.center.y)
                var newContentX = Math.max(0, Math.min(flickable.contentWidth * scale - flickable.width, vx))
                var newContentY = Math.max(0, Math.min(flickable.contentHeight * scale - flickable.height, vy))

                documentViewItem.scale = scale
                documentViewItem.x = widthChange/2
                documentViewItem.y = heightChange/2
                flickable.contentX = newContentX
                flickable.contentY = newContentY
            }
            onPinchFinished: {
                //var finalWidth = Math.max(flickable.contentWidth, flickable.minimumWidth)
                //var finalHeight = Math.max(flickable.contentHeight, flickable.minimumHeight)

                //Reasure the maximum Scale
                //finalWidth = Math.min(finalWidth, image.sourceSize.width)
                //finalHeight = Math.min(finalHeight, image.sourceSize.height)

                //flickable.resizeContent(finalWidth, finalHeight, pinch.center)
                //flickable.returnToBounds()
            }
        }

        DocumentViewItem {
            id: documentViewItem
            onProgressUpdated: {
                if (percent < 0) {
                    if (barPageStack.currentPage == progressPage)
                        barPageStack.pop(buttonPage, true)
                    progressBar.value = 0
                } else {
                    if (barPageStack.currentPage != progressPage)
                        barPageStack.push(progressPage, true)
                    progressBar.value = percent
                }
            }
        }
    }
}
