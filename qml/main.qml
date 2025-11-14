import QtQuick 6.0
import QtQuick.Window 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import QtQuick.Dialogs 6.4

import copick3d.qtgui.graphics 1.0

ApplicationWindow {
  id: win
  visible: true
  width: 1280
  height: 800
  title: qsTr("Qt GUI with VTK Point Cloud View")

  Keys.forwardTo: pcview

  menuBar: MenuBar {
    Menu {
      title: qsTr("File")
      MenuItem {
        text: qsTr("Open")
        onTriggered: fileDialog.open()
      }
      MenuItem {
        text: qsTr("Save")
        onTriggered: console.log("Save clicked")
      }
      MenuItem {
        text: qsTr("Exit")
        onTriggered: Qt.quit()
      }
    }
    Menu {
      title: qsTr("Edit")
      MenuItem {
        text: qsTr("Undo")
        onTriggered: console.log("Undo clicked")
      }
      MenuItem {
        text: qsTr("Redo")
        onTriggered: console.log("Redo clicked")
      }
    }
  }

  header: ToolBar {
    id: toolbar
    width: parent.width

    Keys.forwardTo: pcview

    RowLayout {
      anchors.fill: parent
      anchors.margins: 5

      Rectangle {
        width: 20
        height: 20
        color: pcview.backgroundColor
        border.color: "black"
        border.width: 1
        radius: 3
        Layout.leftMargin: 10
      }

      Button {
        text: "Pick Color"
        onClicked: colorDialog.open()
      }

      CheckBox {
        id: fixedPointSizeCheck
        text: "Fixed Point Size"
        checked: false
        onCheckedChanged: {
          pcview.fixedPointSize = checked
        }
      }

      CheckBox {
        id: parallelProjectionCheck
        text: "Parallel Projection"
        checked: false
        onCheckedChanged: {
          pcview.parallelProjection = checked
        }
      }

      CheckBox {
        id: axisGridCheck
        text: "Show Axis/Grid"
        checked: pcview.axisGridVisible
        onCheckedChanged: {
          pcview.axisGridVisible = checked
        }
      }

      Text {
        text: "Point Size:"
        font.bold: true
        Layout.leftMargin: 20
      }

      SpinBox {
        property real factor: Math.pow(10, 1)
        id: spinbox
        stepSize: 1
        value: 10
        to : 50
        from : 1
        validator: DoubleValidator {
            bottom: Math.min(spinbox.from, spinbox.to)*spinbox.factor
            top:  Math.max(spinbox.from, spinbox.to)*spinbox.factor
        }

        textFromValue: function(value, locale) {
            return parseFloat(value*1.0/factor).toFixed(1);
        }

        onValueChanged: {
            pcview.pointSize = value*1.0/factor
        }
      }
      
      Text {
        text: "Color Mode:"
        font.bold: true
      }

      ComboBox {
        id: colorModeCombo
        textRole: "key"
        valueRole: "value"
        padding: 10
        model: ListModel {
          ListElement { key: "Texture"; value: PointCloudView.Texture }
          ListElement { key: "Normal"; value: PointCloudView.Normal }
          ListElement { key: "Depth"; value: PointCloudView.Depth }
          ListElement { key: "Solid Color"; value: PointCloudView.SolidColor }
        }
        currentIndex: 0
      }

      Text {
        text: "Color Map:"
        font.bold: true
        Layout.leftMargin: 20
      }

      ComboBox {
        id: colorMapCombo
        model: ["Gray", "Hot", "Cool", "Jet", "HSV", "Pink"]
        currentIndex: 0
      }
    }
  }

  FileDialog {
    id: fileDialog
    title: "Open Point Cloud File"
    nameFilters: ["Point Cloud Files (*.png)", "All Files (*)"]
    onAccepted: {
      console.log("Selected file:", selectedFile)
      loader.filePath = selectedFile
    }
    onRejected: {
      console.log("File dialog canceled")
    }
  }

  ColorDialog {
    id: colorDialog
    title: "Select a Color"
    selectedColor: pcview.backgroundColor
    onAccepted: {
      pcview.backgroundColor = colorDialog.selectedColor // Update rectangle color on acceptance
    }
    onRejected: {
      console.log("Color selection cancelled.")
    }
  }

  PointCloudLoader {
    id: loader
    filePath: "D:\\data\\copick_images\\binpicking\\multiple_IMG_Texture_8Bit.png"
  }

  PointCloudView {
    id: pcview
    anchors.fill: parent
    anchors.margins: 0
    focus: true
    focusPolicy: Qt.StrongFocus
    colorMode: colorModeCombo.currentValue
    frame: loader.frame

    Timer {
      interval: 10
      running: true
      repeat: true
      onTriggered: pcview.update()
    }
  }

  Text {
    id: pickedPointText
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.margins: 10
    color: "white"
    font.pixelSize: 14
    text: {
      let p = pcview.pickedPoint
      return `Picked Point: (${p.x.toFixed(2)}, ${p.y.toFixed(2)}, ${p.z.toFixed(2)})`
    }
  }
}
