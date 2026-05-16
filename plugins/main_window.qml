pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs

Item {
    id: root
    anchors.fill: parent

    property int _activeChartIndex: -1
    property int _activeChartSeriesCount: 0
    property bool fftVisible: false
    property bool stepToolVisible: false
    property bool smithChartVisible: false

    // simulation log panel
    property bool logVisible: false

    // status bar message shown as an overlay at the bottom of the view
    property string statusText: ""

    signal zoomRegionSelected(int chartIndex, real x0Ratio, real y0Ratio, real x1Ratio, real y1Ratio)
    signal menuZoomToFit(int chartIndex)
    signal menuAutorange(int chartIndex)
    signal menuZoomAbscissaExtent(int chartIndex)
    signal menuAddRemovePlots(int chartIndex)
    signal menuDeleteAllPlots(int chartIndex)
    signal menuAddChart(int chartIndex)
    signal menuDeleteChart(int chartIndex)
    signal menuNewWindow()
    signal menuFft(int chartIndex)
    signal menuStepTool(int chartIndex)
    signal menuSmithChart(int chartIndex)
    signal pointerMoved(int chartIndex, real xRatio)
    signal pointerExited(int chartIndex)

    // exposed log signals
    signal logAppendRequested(string text)
    signal logClearRequested()

    component SimulationLogPanel: Rectangle {
        id: logPanel
        Layout.fillWidth: true
        Layout.preferredHeight: 200
        color: "#efefe8" // Match main window background

        // visual separator from charts
        Rectangle {
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 1
            color: "#d0d0c8"
        }

        ColumnLayout {

            anchors.fill: parent
            anchors.topMargin: 1
            spacing: 0

            Rectangle {

                Layout.fillWidth: true
                Layout.preferredHeight: 24
                color: "#dcdcd4" // Slightly darker header

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 4
                    Label {
                        text: "Simulation Output"
                        color: "#4a5060"
                        font.pixelSize: 11
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    ToolButton {
                        id: closeButton
                        onClicked: root.logVisible = false
                        padding: 0
                        background: Rectangle {
                            color: closeButton.hovered ? "#d0d0c8" : "transparent"
                            radius: 3
                        }
                        contentItem: Image {
                            source: "kicad-icons/cancel_24.png"
                            sourceSize: Qt.size(14, 14)
                            fillMode: Image.PreserveAspectFit
                            opacity: 0.7
                        }
                    }
                }
            }

            ScrollView {

                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                TextArea {
                    id: logTextArea
                    readOnly: true
                    color: "#2a2d35" // Dark text for light background
                    font.family: "Menlo"
                    font.pixelSize: 12
                    textFormat: TextEdit.PlainText
                    padding: 8

                    Connections {
                        target: root
                        function onLogAppendRequested(text) {
                            logTextArea.append(text);
                        }
                        function onLogClearRequested() {
                            logTextArea.clear();
                        }
                    }
                }
            }
        }
    }

    component ChartPanel: Item {
        id: panel

        // index of this panel in the chartsModel - set by the Repeater delegate
        required property int chartIndex

        property int seriesCount: legendModel.count
        property bool legendVisible: false
        readonly property real plotAreaWidth: graphsView.plotArea.width

        signal zoomRegionSelected(real x0Ratio, real y0Ratio, real x1Ratio, real y1Ratio)
        signal menuZoomToFit
        signal menuAutorange
        signal menuZoomAbscissaExtent
        signal menuAddRemovePlots
        signal menuDeleteAllPlots
        signal menuDeleteChart
        // carries panel-local mouse coords so the root can position the shared menu
        signal menuOpenRequested(real localX, real localY, int seriesCount)
        signal pointerMoved(real xRatio)
        signal pointerExited

        // thin divider drawn above every panel except the first
        Rectangle {
            visible: panel.chartIndex > 0
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
            height: 2
            color: "#12131a"
        }

        Timer {
            id: legendRevealTimer
            interval: 150
            repeat: false
            onTriggered: panel.legendVisible = true
        }

        GraphsView {
            id: graphsView
            marginLeft: 30
            marginRight: 30
            marginBottom: 0
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                bottom: panelLegend.top
            }

            property string xScale: "lin"
            property string xUnit: ""

            theme: GraphsTheme {
                colorScheme: GraphsTheme.ColorScheme.Dark
                theme: GraphsTheme.Theme.UserDefined
                backgroundColor: "#1a1b1e"
                plotAreaBackgroundColor: "#0d0e10"
                colorStyle: GraphsTheme.ColorStyle.Uniform
                labelTextColor: "#b0b8c8"
                labelFont.pixelSize: 10
                labelBackgroundVisible: false
                labelBorderVisible: false
                gridVisible: true
                grid.mainColor: "#2a2d35"
                grid.subColor: "#1e2028"
                grid.mainWidth: 1
                grid.subWidth: 1
                borderWidth: 0
            }

            axisX: ValueAxis {
                id: axisX
                min: 0
                max: 1
                lineVisible: true
                labelsVisible: true
                titleVisible: false
                titleText: ""
                alignment: Qt.AlignBottom

                labelDelegate: Item {

                    property string text: ""

                    Text {
                        anchors.fill: parent
                        color: "#b0b8c8"
                        font.pixelSize: 10
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignTop
                        text: {
                            // format the X axis label based on the selected scale
                            switch (graphsView.xScale) {
                            case "dec":
                                return graphsView.decadeValueFormatter(graphsView.xUnit, parent.text);
                            case "oct":
                                return graphsView.octaveValueFormatter(graphsView.xUnit, parent.text);
                            default:
                                return graphsView.linearValueFormatter(graphsView.xUnit, parent.text);
                            }
                        }
                    }
                }
            }

            function applyUnit(unit, text, value) {
                // absolute value for prefix selection
                const absValue = Math.abs(value);
                // giga
                if (absValue >= 1e9)
                    return (value / 1e9).toFixed(1) + " G" + unit;
                // mega
                if (absValue >= 1e6)
                    return (value / 1e6).toFixed(1) + " M" + unit;
                // kilo
                if (absValue >= 1e3)
                    return (value / 1e3).toFixed(1) + " k" + unit;
                // base unit
                if (absValue >= 1.0)
                    return value.toFixed(1) + " " + unit;
                // zero
                if (absValue < 1e-15)
                    return "0" + unit;
                // femto
                if (absValue < 1e-12)
                    return (value * 1e15).toFixed(1) + " f" + unit;
                // pico
                if (absValue < 1e-9)
                    return (value * 1e12).toFixed(1) + " p" + unit;
                // nano
                if (absValue < 1e-6)
                    return (value * 1e9).toFixed(1) + " n" + unit;
                // micro
                if (absValue < 1e-3)
                    return (value * 1e6).toFixed(1) + " µ" + unit;
                // milli
                return (value * 1e3).toFixed(1) + " m" + unit;
            }

            function linearValueFormatter(unit, text) {
                // parse value
                var value = parseFloat(text);
                if (isNaN(value))
                    return text;
                // unit
                return applyUnit(unit, text, value);
            }

            function decadeValueFormatter(unit, text) {
                // parse value
                var value = parseFloat(text);
                if (isNaN(value))
                    return text;
                // calculate actual value from decade exponent
                var actual = Math.pow(10, value);
                // unit
                return applyUnit(unit, text, actual);
            }

            function octaveValueFormatter(unit, text) {
                // parse value
                var value = parseFloat(text);
                if (isNaN(value))
                    return text;
                // calculate actual value from octave exponent
                var actual = Math.pow(2, value);
                // unit
                return applyUnit(unit, text, actual);
            }
        }

        Item {
            id: selectionOverlay
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                bottom: panelLegend.top
            }

            // last mouse X recorded during a pan drag — updated each frame so each delta is incremental
            property bool selectionActive: false
            // rectangle selection start point in overlay coordinates
            property real selectionStartX: 0
            property real selectionStartY: 0
            // rectangle selection current point in overlay coordinates
            property real selectionCurrentX: 0
            property real selectionCurrentY: 0

            // clamp a pixel X to the visible plot area
            function clampPixelX(px) {
                // rectangle
                var r = graphsView.plotArea;
                // clamp and return
                return Math.max(r.x, Math.min(r.x + r.width, px));
            }

            // clamp a pixel Y to the visible plot area
            function clampPixelY(py) {
                // rectangle
                var r = graphsView.plotArea;
                // clamp and return
                return Math.max(r.y, Math.min(r.y + r.height, py));
            }

            // map a pixel X within the overlay to a 0-1 plot-area fraction (0=left, 1=right)
            function pixelToXRatio(px) {
                // rectangle
                var r = graphsView.plotArea;
                // compute ratio of pixel X within the plot area, clamped to [0, 1]
                var ratio = (px - r.x) / r.width;
                // clamp and return
                return Math.max(0, Math.min(1, ratio));
            }

            // map a pixel Y within the overlay to a 0-1 chart fraction (0=bottom, 1=top)
            function pixelToYRatio(py) {
                // rectangle
                var r = graphsView.plotArea;
                // compute ratio of pixel Y within the plot area in screen coordinates
                var screenRatio = (py - r.y) / r.height;
                // invert to chart coordinates and clamp to [0, 1]
                return Math.max(0, Math.min(1, 1.0 - screenRatio));
            }

            Rectangle {
                visible: selectionOverlay.selectionActive
                x: Math.min(selectionOverlay.selectionStartX, selectionOverlay.selectionCurrentX)
                y: Math.min(selectionOverlay.selectionStartY, selectionOverlay.selectionCurrentY)
                width: Math.abs(selectionOverlay.selectionCurrentX - selectionOverlay.selectionStartX)
                height: Math.abs(selectionOverlay.selectionCurrentY - selectionOverlay.selectionStartY)
                color: "#2680eb33"
                border.color: "#56a3ff"
                border.width: 1
                z: 1
            }

            // left-button drag — selects a rectangle and emits normalized chart percentages on release
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                // enable hover so onPositionChanged fires without a button held down
                hoverEnabled: true
                // show a crosshair so rectangle selection is discoverable
                cursorShape: Qt.CrossCursor

                onPressed: mouse => {
                    // clamp the drag start to the visible plot area
                    selectionOverlay.selectionStartX = selectionOverlay.clampPixelX(mouse.x);
                    selectionOverlay.selectionStartY = selectionOverlay.clampPixelY(mouse.y);
                    // initialize current corner from the same point
                    selectionOverlay.selectionCurrentX = selectionOverlay.selectionStartX;
                    selectionOverlay.selectionCurrentY = selectionOverlay.selectionStartY;
                    // mark selection active so the overlay rectangle becomes visible
                    selectionOverlay.selectionActive = true;
                }

                onPositionChanged: mouse => {
                    // only update the selection rectangle during an active drag to avoid jitter while hovering
                    if (selectionOverlay.selectionActive) {
                        // clamp the drag end to the visible plot area
                        selectionOverlay.selectionCurrentX = selectionOverlay.clampPixelX(mouse.x);
                        selectionOverlay.selectionCurrentY = selectionOverlay.clampPixelY(mouse.y);
                        // exit
                        return;
                    }
                    // always report pointer position to the status bar
                    panel.pointerMoved(selectionOverlay.pixelToXRatio(mouse.x));
                }

                onReleased: mouse => {
                    // clamp the drag end before computing normalized coordinates
                    var endX = selectionOverlay.clampPixelX(mouse.x);
                    var endY = selectionOverlay.clampPixelY(mouse.y);
                    // only emit when we have a valid rectangle to avoid spurious events from clicks or tiny drags
                    if (Math.abs(endX - selectionOverlay.selectionStartX) >= 10 && Math.abs(endY - selectionOverlay.selectionStartY) >= 10) {
                        // emit rectangle corners as normalized chart-space percentages
                        panel.zoomRegionSelected(selectionOverlay.pixelToXRatio(selectionOverlay.selectionStartX), selectionOverlay.pixelToYRatio(selectionOverlay.selectionStartY), selectionOverlay.pixelToXRatio(endX), selectionOverlay.pixelToYRatio(endY));
                    }
                    // hide the selection rectangle after completion
                    selectionOverlay.selectionActive = false;
                }

                onExited: panel.pointerExited()

                onCanceled: selectionOverlay.selectionActive = false
            }

            // right-button click — asks root to open the shared context menu at this location
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: mouse => panel.menuOpenRequested(mouse.x, mouse.y, panel.seriesCount)
            }
        }

        ListModel {
            id: legendModel
        }

        Rectangle {
            id: panelLegend
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            color: "#1a1b1e"
            height: panel.legendVisible ? 20 : 0
            visible: panel.legendVisible

            Row {
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 16
                Repeater {
                    model: legendModel
                    delegate: Row {
                        required property string seriesName
                        required property string seriesColor
                        spacing: 6
                        Rectangle {
                            width: 4
                            height: 4
                            color: parent.seriesColor
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: parent.seriesName
                            color: "#b0b8c8"
                            font.pixelSize: 10
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }

        Component {
            id: yAxisComponent

            ValueAxis {
                id: valueAxis

                property string yUnit: ""

                lineVisible: true
                labelsVisible: true
                titleVisible: false
                alignment: Qt.AlignLeft
                labelDelegate: Item {
                    property string text: ""
                    Text {
                        anchors.fill: parent
                        color: "#b0b8c8"
                        font.pixelSize: 10
                        horizontalAlignment: valueAxis.alignment === Qt.AlignLeft ? Text.AlignRight : Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        text: graphsView.linearValueFormatter(valueAxis.yUnit, parent.text)
                    }
                }
            }
        }

        function initialize(axisXTitle, axisXUnit, axisXScale, axisXMinValue, axisXMaxValue) {
            // set X title
            axisX.titleText = axisXTitle;
            // unit and scale
            graphsView.xUnit = axisXUnit;
            graphsView.xScale = axisXScale;
            // one tick per decade/octave in log space; ten equal ticks for linear
            axisX.tickInterval = (axisXScale === "lin") ? (axisXMaxValue - axisXMinValue) / 10 : 1.0;
            axisX.subTickCount = (axisXScale === "lin") ? 10 : 0;
            // set X range
            axisX.min = axisXMinValue;
            axisX.max = axisXMaxValue;
        }

        function createYAxis(alignment, unit) {
            // create and return a new ValueAxis instance from the component
            const axis = yAxisComponent.createObject(graphsView);
            // set property values
            axis.alignment = alignment;
            axis.yUnit = unit;
            // use axis
            return axis;
        }

        function resizeAbscissa(axisXMinValue, axisXMaxValue) {
            // update X axis range after a zoom operation
            axisX.min = axisXMinValue;
            axisX.max = axisXMaxValue;
            // one tick per decade/octave in log space; ten equal ticks for linear
            axisX.tickInterval = (graphsView.xScale === "lin") ? (axisXMaxValue - axisXMinValue) / 10 : 1.0;
        }

        function updateGraphsView(seriesToAdd, seriesToRemove) {
            // skip when there are no updates
            if (seriesToAdd.length === 0 && seriesToRemove.length === 0)
                return;
            // avoid repeated repaints while many series are updated in one event
            const wasVisible = graphsView.visible;
            if (wasVisible)
                graphsView.visible = false;
            // collect removals first so transient add/remove overlap does not churn scenegraph state
            const removeLegendNames = {};
            for (var i = 0; i < seriesToRemove.length; i++) {
                // current remove payload
                const removeCurrent = seriesToRemove[i];
                // extract name and series list
                const removeName = removeCurrent[0];
                const removeData = removeCurrent[1];
                // remove every series in this payload
                for (var j = removeData.length - 1; j >= 0; j--)
                    graphsView.removeSeries(removeData[j]);
                // mark legend entries to remove in a single legend scan later
                if (removeName != null)
                    removeLegendNames[removeName] = true;
            }
            // remove legend entries in one reverse pass
            for (var i = legendModel.count - 1; i >= 0; i--) {
                // current legend label
                const legendName = legendModel.get(i)["seriesName"];
                // remove when this label was marked for deletion
                if (removeLegendNames[legendName] === true)
                    legendModel.remove(i);
            }
            // collect additions after removals to minimize intermediate graph states
            const legendEntriesToAdd = [];
            for (var i = 0; i < seriesToAdd.length; i++) {
                // current add payload
                const addCurrent = seriesToAdd[i];
                // extract name, color and series list
                const addName = addCurrent[0];
                const addColor = addCurrent[1];
                const addData = addCurrent[2];
                // add every series for this payload
                for (var j = 0; j < addData.length; j++) {
                    // current series
                    const addSeries = addData[j];
                    // set line color from the palette
                    addSeries.color = addColor;
                    // append to chart
                    graphsView.addSeries(addSeries);
                }
                // queue legend addition with same color; step additions continue using existing legend entries
                if (addName != null)
                    legendEntriesToAdd.push({ seriesName: addName, seriesColor: addColor });
            }
            // append legend entries in one pass
            for (var i = 0; i < legendEntriesToAdd.length; i++)
                legendModel.append(legendEntriesToAdd[i]);
            // restore visibility after batch updates
            if (wasVisible)
                graphsView.visible = true;
            // reveal the legend the first time we plot series
            if (!panel.legendVisible) {
                // reveal the legend after a short delay so the chart has time to paint first
                legendRevealTimer.restart();
            }
        }

        function removeAllSeries() {
            // loop all series in the chart and remove them
            for (var i = graphsView.seriesList.length - 1; i >= 0; i--)
                graphsView.removeSeries(i);
            // clear the legend
            legendModel.clear();
            // reset default Y axis
            graphsView.axisY = null;
        }
    }

    ListModel {
        id: chartsModel
    }

    ColumnLayout {
        anchors {
            fill: parent
            bottomMargin: root.statusText !== "" ? 22 : 0
        }
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Column {
                id: chartsColumn
                anchors.fill: parent
                spacing: 0

                Repeater {
                    id: chartsRepeater
                    model: chartsModel
                    delegate: ChartPanel {
                        required property int index
                        chartIndex: index
                        width: chartsColumn.width
                        // use full height minus spacing
                        height: (chartsColumn.height - (chartsColumn.spacing * Math.max(0, chartsModel.count - 1))) / Math.max(1, chartsModel.count)
                        // ... event handlers
                        onZoomRegionSelected: (x0, y0, x1, y1) => root.zoomRegionSelected(index, x0, y0, x1, y1)
                        onMenuZoomToFit: root.menuZoomToFit(index)
                        onMenuAutorange: root.menuAutorange(index)
                        onMenuZoomAbscissaExtent: root.menuZoomAbscissaExtent(index)
                        onMenuAddRemovePlots: root.menuAddRemovePlots(index)
                        onMenuDeleteAllPlots: root.menuDeleteAllPlots(index)
                        onMenuDeleteChart: root.menuDeleteChart(index)
                        onPointerMoved: xRatio => root.pointerMoved(index, xRatio)
                        onPointerExited: root.pointerExited(index)
                        onMenuOpenRequested: (localX, localY, sc) => {
                            var pt = mapToItem(root, localX, localY);
                            root._activeChartIndex = index;
                            root._activeChartSeriesCount = sc;
                            contextMenu.x = Math.min(pt.x, root.width - contextMenu.width - 2);
                            contextMenu.y = Math.min(pt.y, root.height - contextMenu.height - 2);
                            contextMenu.visible = true;
                        }
                    }
                }
            }
        }

        Loader {
            Layout.fillWidth: true
            Layout.preferredHeight: root.logVisible ? 200 : 0
            Layout.maximumHeight: root.logVisible ? 200 : 0
            sourceComponent: root.logVisible ? logComponent : undefined
        }
    }

    Component {
        id: logComponent
        SimulationLogPanel {}
    }

    MouseArea {
        anchors.fill: parent
        visible: contextMenu.visible
        z: 998
        acceptedButtons: Qt.AllButtons
        onPressed: contextMenu.visible = false
    }

    // status bar overlay — rendered inside the QML view so Qt widget layout
    // is never affected and no gap can appear
    Rectangle {
        id: statusBar
        visible: root.statusText !== ""
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: 22
        color: "#efefe8"
        z: 997

        Rectangle {
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 1
            color: "#d0d0c8"
        }

        Text {
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
                leftMargin: 8
                rightMargin: 8
            }
            text: root.statusText
            color: "#333333"
            font.pixelSize: 11
            elide: Text.ElideRight
        }
    }

    Rectangle {
        id: contextMenu
        visible: false
        z: 999
        width: 210
        height: menuColumn.implicitHeight + 8
        color: "#252730"
        border.color: "#3a3d4a"
        border.width: 1
        radius: 4

        Column {
            id: menuColumn
            anchors {
                top: parent.top
                left: parent.left
                topMargin: 4
            }

            ContextMenuItem {
                itemText: "Zoom to Fit"
                onTriggered: root.menuZoomToFit(root._activeChartIndex)
            }
            ContextMenuItem {
                itemText: "Autorange"
                onTriggered: root.menuAutorange(root._activeChartIndex)
            }
            ContextMenuItem {
                itemText: "Zoom Abscissa Extent"
                onTriggered: root.menuZoomAbscissaExtent(root._activeChartIndex)
            }
            ContextMenuSeparator {}
            ContextMenuItem {
                itemText: "Add/Remove Plots"
                onTriggered: root.menuAddRemovePlots(root._activeChartIndex)
            }
            ContextMenuItem {
                itemText: "Delete All Plots"
                enabled: root._activeChartSeriesCount > 0
                onTriggered: root.menuDeleteAllPlots(root._activeChartIndex)
            }
            ContextMenuSeparator {}
            ContextMenuItem {
                itemText: "FFT..."
                visible: root.fftVisible && root._activeChartSeriesCount > 0
                onTriggered: root.menuFft(root._activeChartIndex)
            }
            ContextMenuItem {
                itemText: "Step Tool..."
                visible: root.stepToolVisible
                onTriggered: root.menuStepTool(root._activeChartIndex)
            }
            ContextMenuItem {
                itemText: "Smith Chart..."
                visible: root.smithChartVisible
                onTriggered: root.menuSmithChart(root._activeChartIndex)
            }
            ContextMenuSeparator {
                visible: (root.fftVisible && root._activeChartSeriesCount > 0) || root.stepToolVisible || root.smithChartVisible
            }
            ContextMenuItem {
                itemText: "Add Chart"
                onTriggered: root.menuAddChart(root._activeChartIndex)
            }
            ContextMenuItem {
                itemText: "Delete Chart"
                enabled: chartsModel.count > 1
                onTriggered: root.menuDeleteChart(root._activeChartIndex)
            }
            ContextMenuSeparator {}
            ContextMenuItem {
                itemText: "New Window"
                onTriggered: root.menuNewWindow()
            }
        }
    }

    component ContextMenuItem: Rectangle {
        id: itemRoot
        required property string itemText
        signal triggered
        width: contextMenu.width
        implicitHeight: 26
        color: itemMouse.containsMouse ? "#3a3d4a" : "transparent"
        radius: 2
        Text {
            anchors.fill: parent
            text: itemRoot.itemText
            color: itemRoot.enabled ? "#b0b8c8" : "#4a5060"
            font.pixelSize: 12
            leftPadding: 12
            verticalAlignment: Text.AlignVCenter
        }
        MouseArea {
            id: itemMouse
            anchors.fill: parent
            hoverEnabled: true
            enabled: itemRoot.enabled
            onClicked: {
                contextMenu.visible = false;
                itemRoot.triggered();
            }
        }
    }

    // reusable separator
    component ContextMenuSeparator: Rectangle {
        width: contextMenu.width
        implicitHeight: 9
        color: "transparent"
        Rectangle {
            anchors.centerIn: parent
            width: parent.width - 8
            height: 1
            color: "#3a3d4a"
        }
    }

    function addChart() {
        // append a new chart panel entry to the model; the Repeater reacts immediately
        chartsModel.append({});
    }

    function removeChart(chartIndex) {
        // remove the panel at the given index; remaining panels reflow automatically
        chartsModel.remove(chartIndex);
    }

    function getChart(chartIndex) {
        // return the live ChartPanel item so Python can call initialize / addSeries etc.
        return chartsRepeater.itemAt(chartIndex);
    }
}
