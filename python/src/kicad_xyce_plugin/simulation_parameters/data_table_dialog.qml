pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {

    id: root
    anchors.fill: parent

    property var existingTableNames: []
    property string originalTableName: ""
    property string tableName: ""
    property var columnNames: []
    property var rowValues: []
    property string validationError: ""
    property bool initialized: false

    readonly property bool canAccept: validationError === ""

    signal dialogAccepted()
    signal dialogRejected()

    onExistingTableNamesChanged: if (initialized) validate()
    onOriginalTableNameChanged: if (initialized) validate()
    onTableNameChanged: if (initialized) validate()
    onColumnNamesChanged: if (initialized) validate()
    onRowValuesChanged: if (initialized) validate()

    function _normalizedName(value) {
        return String(value).trim()
    }

    function _isReservedName(name) {
        var reservedNames = ["TIME", "FREQ", "HERTZ", "VT", "TEMP", "TEMPER", "GMIN"]
        return reservedNames.indexOf(name.trim().toUpperCase()) !== -1
    }

    function _isIdentifier(name) {
        return /^[A-Za-z_][A-Za-z0-9_]*$/.test(name.trim())
    }

    function _isValidName(name) {
        return _isIdentifier(name) && !_isReservedName(name)
    }

    function _cloneRow(row) {
        return _toArray(row)
    }

    function _toArray(value) {
        if (value === null || value === undefined) {
            return []
        }
        if (Array.isArray(value)) {
            return value.slice()
        }
        if (typeof value.length === "number") {
            var result = []
            for (var i = 0; i < value.length; i++) {
                result.push(value[i])
            }
            return result
        }
        return []
    }

    function initialize(tableNameValue, columnNamesValue, rowValuesValue, existingTableNamesValue, originalTableNameValue) {
        root.initialized = false
        root.tableName = tableNameValue
        root.columnNames = _toArray(columnNamesValue)
        root.rowValues = _toArray(rowValuesValue).map(function(row) { return _cloneRow(row) })
        root.existingTableNames = _toArray(existingTableNamesValue)
        root.originalTableName = originalTableNameValue || ""
        root.initialized = true
        validate()
    }

    function setTableName(value) {
        root.tableName = value
    }

    function setColumnName(columnIndex, value) {
        var updatedColumns = root.columnNames.slice()
        updatedColumns[columnIndex] = value
        root.columnNames = updatedColumns
    }

    function setCellValue(rowIndex, columnIndex, value) {
        var updatedRows = root.rowValues.slice()
        var updatedRow = _cloneRow(updatedRows[rowIndex])
        updatedRow[columnIndex] = value
        updatedRows[rowIndex] = updatedRow
        root.rowValues = updatedRows
    }

    function addColumn() {
        var updatedColumns = root.columnNames.slice()
        var nextIndex = updatedColumns.length + 1
        var candidate = "param" + nextIndex
        while (updatedColumns.indexOf(candidate) !== -1 || _isReservedName(candidate)) {
            nextIndex += 1
            candidate = "param" + nextIndex
        }
        updatedColumns.push(candidate)
        root.columnNames = updatedColumns
        var updatedRows = root.rowValues.slice()
        for (var rowIndex = 0; rowIndex < updatedRows.length; rowIndex++) {
            var row = _cloneRow(updatedRows[rowIndex])
            row.push("")
            updatedRows[rowIndex] = row
        }
        root.rowValues = updatedRows
        validate()
    }

    function removeColumn(columnIndex) {
        var updatedColumns = root.columnNames.slice()
        updatedColumns.splice(columnIndex, 1)
        root.columnNames = updatedColumns
        var updatedRows = root.rowValues.slice()
        for (var rowIndex = 0; rowIndex < updatedRows.length; rowIndex++) {
            var row = _cloneRow(updatedRows[rowIndex])
            row.splice(columnIndex, 1)
            updatedRows[rowIndex] = row
        }
        root.rowValues = updatedRows
        validate()
    }

    function addRow() {
        var updatedRows = root.rowValues.slice()
        var newRow = []
        for (var columnIndex = 0; columnIndex < root.columnNames.length; columnIndex++) {
            newRow.push("")
        }
        updatedRows.push(newRow)
        root.rowValues = updatedRows
        validate()
    }

    function removeRow(rowIndex) {
        var updatedRows = root.rowValues.slice()
        updatedRows.splice(rowIndex, 1)
        root.rowValues = updatedRows
        validate()
    }

    function validate() {
        if (!root.initialized) {
            root.validationError = ""
            return
        }
        var tableName = _normalizedName(root.tableName)
        if (tableName === "") {
            root.validationError = "Table name is required"
            return
        }
        if (!_isValidName(tableName)) {
            root.validationError = "Table name must be a legal identifier"
            return
        }
        for (var i = 0; i < root.existingTableNames.length; i++) {
            var existingName = _normalizedName(root.existingTableNames[i])
            if (existingName !== "" && existingName.toUpperCase() === tableName.toUpperCase() && existingName.toUpperCase() !== _normalizedName(root.originalTableName).toUpperCase()) {
                root.validationError = "Table name must be unique"
                return
            }
        }
        if (root.columnNames.length < 1) {
            root.validationError = "At least one column is required"
            return
        }
        if (root.rowValues.length < 1) {
            root.validationError = "At least one row is required"
            return
        }
        var seenColumns = {}
        for (var columnIndex = 0; columnIndex < root.columnNames.length; columnIndex++) {
            var columnName = _normalizedName(root.columnNames[columnIndex])
            if (columnName === "") {
                root.validationError = "Column header names are required"
                return
            }
            if (!_isValidName(columnName)) {
                root.validationError = "Column header names must be legal identifiers"
                return
            }
            var normalizedColumnName = columnName.toUpperCase()
            if (seenColumns[normalizedColumnName] === true) {
                root.validationError = "Column header names must be unique"
                return
            }
            seenColumns[normalizedColumnName] = true
        }
        for (var rowIndex = 0; rowIndex < root.rowValues.length; rowIndex++) {
            var row = root.rowValues[rowIndex]
            if (!Array.isArray(row) || row.length !== root.columnNames.length) {
                root.validationError = "Each row must contain one value per column"
                return
            }
            for (var valueIndex = 0; valueIndex < row.length; valueIndex++) {
                var cellValue = _normalizedName(row[valueIndex])
                if (cellValue === "") {
                    root.validationError = "Data values are required"
                    return
                }
                if (!/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?$/.test(cellValue) && !/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:meg|mil|f|p|n|u|m|k|g|t)$/.test(cellValue.toLowerCase())) {
                    root.validationError = "Data values must be numeric"
                    return
                }
            }
        }
        root.validationError = ""
    }

    Component.onCompleted: validationError = ""

    Rectangle {
        anchors.fill: parent
        color: "#1a1b1e"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        Text {
            text: "DATA Table Editor"
            color: "#dce8f8"
            font.pixelSize: 18
            font.bold: true
            font.family: "Helvetica"
        }

        Text {
            text: "Edit one .DATA block at a time. The dialog validates the Xyce table shape before accepting."
            color: "#8f98ab"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            font.family: "Helvetica"
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 10
            columnSpacing: 16

            Label { text: "Table Name *"; color: "#8f98ab"; font.pixelSize: 12; font.family: "Helvetica" }
            TextField {
                id: tableNameField
                Layout.fillWidth: true
                placeholderText: "e.g. testTable"
                text: root.tableName
                selectByMouse: true
                onTextEdited: root.setTableName(text)
                font.family: "Helvetica"
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "Columns"
                color: "#8f98ab"
                font.pixelSize: 12
                font.bold: true
                font.family: "Helvetica"
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "+ Add Column"
                onClicked: root.addColumn()
                font.family: "Helvetica"
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                width: parent.width
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "Header"
                        color: "#8f98ab"
                        font.pixelSize: 12
                        Layout.preferredWidth: 90
                        font.family: "Helvetica"
                    }

                    Repeater {
                        model: root.columnNames.length

                        delegate: ColumnLayout {
                            required property int index
                            Layout.preferredWidth: 140
                            spacing: 4

                            TextField {
                                Layout.fillWidth: true
                                placeholderText: "Column name"
                                text: root.columnNames[index] || ""
                                selectByMouse: true
                                onTextEdited: root.setColumnName(index, text)
                                font.family: "Helvetica"
                            }

                            Button {
                                text: "Remove"
                                Layout.fillWidth: true
                                onClicked: root.removeColumn(index)
                                font.family: "Helvetica"
                            }
                        }
                    }
                }

                Repeater {
                    model: root.rowValues.length

                    delegate: RowLayout {
                        required property int index
                        property int rowIndex: index
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Row " + (index + 1)
                            color: "#8f98ab"
                            font.pixelSize: 12
                            Layout.preferredWidth: 90
                            font.family: "Helvetica"
                        }

                        Repeater {
                            model: root.columnNames.length

                            delegate: TextField {
                                required property int index
                                Layout.preferredWidth: 140
                                placeholderText: "Value"
                                text: Array.isArray(root.rowValues[rowIndex]) && root.rowValues[rowIndex][index] !== undefined ? root.rowValues[rowIndex][index] : ""
                                selectByMouse: true
                                onTextEdited: root.setCellValue(rowIndex, index, text)
                                font.family: "Helvetica"
                            }
                        }

                        Button {
                            text: "Remove"
                            onClicked: root.removeRow(index)
                            font.family: "Helvetica"
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Button {
                text: "+ Add Row"
                onClicked: root.addRow()
                font.family: "Helvetica"
            }

            Item { Layout.fillWidth: true }

            Text {
                text: root.validationError
                color: "#c05050"
                font.pixelSize: 12
                visible: root.validationError !== ""
                wrapMode: Text.WordWrap
                Layout.preferredWidth: 340
                font.family: "Helvetica"
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                onClicked: root.dialogRejected()
                font.family: "Helvetica"
            }

            Button {
                text: "OK"
                enabled: root.canAccept
                onClicked: root.dialogAccepted()
                font.family: "Helvetica"
            }
        }
    }
}
