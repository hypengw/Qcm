pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import QtQuick.Effects
import QExtra as QE

Item {
    id: root

    property url source
    property bool active: true
    property color fallbackColor: "#262626"
    property int transitionDuration: 250
    readonly property bool supported: GraphicsInfo.api !== GraphicsInfo.Software && GraphicsInfo.api !== GraphicsInfo.Unknown
    readonly property bool rendering: active && visible && width > 0 && height > 0 && supported && Window.window !== null && Window.window.visible && Window.window.visibility !== Window.Minimized
    readonly property real pixelRatio: Screen.devicePixelRatio
    property BackgroundSlot current: null
    property BackgroundSlot candidate: null

    clip: true

    Rectangle {
        anchors.fill: parent
        color: root.fallbackColor
        visible: root.current === null
    }

    function reset() {
        fade.stop();
        fade.target = null;
        if (candidate) {
            candidate.visible = false;
            candidate.destroy();
            candidate = null;
        }
        if (current) {
            current.visible = false;
            current.destroy();
            current = null;
        }
    }

    function refresh() {
        if (!rendering || source.toString() === "") {
            reset();
            return;
        }
        if (fade.running)
            return;
        if (candidate) {
            if (candidate.source.toString() === source.toString())
                return;
            candidate.visible = false;
            candidate.destroy();
            candidate = null;
        }
        if (current && current.source.toString() === source.toString())
            return;
        candidate = slotComponent.createObject(root, {
            source: source,
            workSide: 64,
            decodeSide: Math.min(512, Math.max(128, Math.ceil(width * pixelRatio / 2)))
        });
    }

    function accept(slot: BackgroundSlot) {
        if (slot !== candidate)
            return;
        if (!rendering || slot.source.toString() !== source.toString()) {
            slot.visible = false;
            slot.destroy();
            candidate = null;
            refresh();
            return;
        }
        if (!current) {
            current = candidate;
            candidate = null;
            current.z = 1;
        } else {
            candidate.z = 2;
            candidate.opacity = 0;
            fade.target = candidate;
            fade.start();
        }
    }

    onSourceChanged: refresh()
    onRenderingChanged: refresh()
    onPixelRatioChanged: {
        reset();
        refresh();
    }
    Component.onCompleted: refresh()

    Connections {
        target: root.Window.window
        function onSceneGraphInvalidated() {
            root.reset();
        }
        function onSceneGraphInitialized() {
            Qt.callLater(root.refresh);
        }
    }

    NumberAnimation {
        id: fade
        property: "opacity"
        from: 0
        to: 1
        duration: root.transitionDuration
        easing.type: Easing.InOutQuad
        onFinished: {
            root.current.visible = false;
            root.current.destroy();
            root.current = root.candidate;
            root.candidate = null;
            root.current.z = 1;
            target = null;
            root.refresh();
        }
    }

    Component {
        id: slotComponent
        BackgroundSlot {
            anchors.fill: parent
            onCompleted: root.accept(this)
            onFailed: {
                if (this === root.candidate) {
                    const latest = source.toString() === root.source.toString();
                    root.reset();
                    if (!latest)
                        root.refresh();
                }
            }
        }
    }

    component BackgroundSlot: Item {
        id: slot
        required property url source
        required property real workSide
        required property int decodeSide
        property bool frozen: false
        property int stage: 0
        signal completed
        signal failed
        z: -1

        Loader {
            id: pipeline
            active: !slot.frozen
            width: slot.workSide
            height: width
            sourceComponent: Item {
                readonly property Item effect: result
                width: slot.workSide
                height: width

                Rectangle {
                    id: input
                    anchors.fill: parent
                    color: "#262626"
                    QE.Image {
                        objectName: "coverInput"
                        anchors.fill: parent
                        source: slot.source
                        sourceSize: Qt.size(slot.decodeSide, slot.decodeSide)
                        maxSize: Qt.size(512, 512)
                        fillMode: QE.Image.PreserveAspectCrop
                        retainWhileLoading: false
                        onStatusChanged: {
                            if (status === QE.Image.Ready) {
                                slot.stage = 1;
                                inputTexture.scheduleUpdate();
                            } else if (status === QE.Image.Error) {
                                slot.failed();
                            }
                        }
                    }
                }
                ShaderEffectSource {
                    id: inputTexture
                    anchors.fill: parent
                    sourceItem: input
                    hideSource: true
                    live: false
                    textureSize: Qt.size(64, 64)
                    onScheduledUpdateCompleted: {
                        if (slot.stage === 1)
                            slot.stage = 2;
                    }
                }
                Item {
                    id: result
                    anchors.fill: parent
                    Rectangle {
                        anchors.fill: parent
                        color: "#262626"
                    }
                    MultiEffect {
                        anchors.fill: parent
                        source: inputTexture
                        blurEnabled: true
                        blurMax: 64
                        blur: 1
                        // Wide sampling offsets produce visible repeated contours.
                        blurMultiplier: 0
                        autoPaddingEnabled: false
                    }
                }
            }
        }
        Loader {
            id: snapshot
            width: slot.workSide
            height: width
            active: slot.stage >= 2
            sourceComponent: ShaderEffectSource {
                visible: false
                sourceItem: pipeline.item ? pipeline.item.effect : null
                sourceRect: Qt.rect(0, 0, slot.workSide, slot.workSide)
                textureSize: Qt.size(256, 256)
                hideSource: true
                live: false
                onScheduledUpdateCompleted: {
                    if (slot.stage === 2) {
                        slot.stage = 3;
                        slot.frozen = true;
                        slot.completed();
                    }
                }
            }
        }
        ShaderEffect {
            // Keep displaying the frozen texture after its source item is released.
            anchors.centerIn: parent
            width: Math.max(slot.width, slot.height)
            height: width
            property var source: snapshot.item
            onStatusChanged: {
                if (status === ShaderEffect.Error)
                    slot.failed();
            }
        }
    }
}
