import QtQuick 6.0
import QtQuick.Window 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import QtQuick.Dialogs 6.2

import copick3d.qtgui.graphics 1.0

ApplicationWindow {
  id: win
  visible: true
  width: 1280
  height: 800
  title: qsTr("Hello World")

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

    RowLayout {
      anchors.fill: parent
      anchors.margins: 5

      CheckBox {
        id: parallelProjectionCheck
        text: "Parallel Projection"
        checked: false
        onCheckedChanged: {
          pcview.parallelProjection = checked
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

  PointCloudLoader {
    id: loader
    filePath: "D:\\data\\copick_images\\binpicking\\multiple_IMG_Texture_8Bit.png"
  }

  PointCloudView {
    id: pcview
    anchors.fill: parent
    anchors.margins: 0
    focus: true
    colorMode: colorModeCombo.currentValue
    frame: loader.frame
  }
}
