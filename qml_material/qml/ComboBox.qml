import QtQuick
import QtQuick.Window
import QtQuick.Controls.impl
import QtQuick.Templates as T
import QtQuick.Controls.Material.impl as MDImpl
import Qcm.Material as MD

T.ComboBox {
    id: control

    property int type: MD.Enum.TextFieldOutlined

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: 12
    leftPadding: padding + (!control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    rightPadding: padding + (control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    topPadding: 0
    bottomPadding: 0

    delegate: MD.MenuItem {
        required property var model
        required property int index

        width: ListView.view.width
        text: model[control.textRole]
        // Material.foreground: control.currentIndex === index ? ListView.view.contentItem.Material.accent : ListView.view.contentItem.Material.foreground
        highlighted: control.highlightedIndex === index
        hoverEnabled: true//control.hoverEnabled
    }

    indicator: MD.Icon {
        x: control.mirrored ? control.padding : control.width - width - control.padding
        y: control.topPadding + (control.availableHeight - height) / 2
        name: MD.Token.icon.arrow_drop_down
        size: 24
    }

    contentItem: MD.TextInput {
        typescale: MD.Token.typescale.body_large

        padding: 0
        text: control.editable ? control.editText : control.displayText
        enabled: control.editable
        autoScroll: control.editable
        readOnly: !control.editable
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        selectByMouse: control.selectTextByMouse
        color: item_state.textColor
        selectionColor: MD.Token.color.primary
        selectedTextColor: MD.Token.color.getOn(selectionColor)
        verticalAlignment: TextInput.AlignVCenter
    }
    background: MDImpl.MaterialTextContainer {
        implicitWidth: 64
        implicitHeight: 56

        filled: control.type === MD.Enum.TextFieldFilled
        fillColor: control.MD.MatProp.backgroundColor
        outlineColor: control.outlineColor
        focusedOutlineColor: control.outlineColor
        controlHasActiveFocus: control.activeFocus
        controlHasText: true
        horizontalPadding: 16
    }

    popup: MD.Menu {
        y: control.editable ? control.height - 5 : 0
        height: Math.min(contentItem.implicitHeight + verticalPadding * 2, control.Window.height - topMargin - bottomMargin)
        width: control.width
        transformOrigin: Item.Top
        modal: false
        model: control.delegateModel
        topMargin: 12
        bottomMargin: 12
        verticalPadding: 8
    }

    MD.MatProp.elevation: item_state.elevation
    MD.MatProp.textColor: item_state.textColor
    MD.MatProp.supportTextColor: item_state.supportTextColor
    MD.MatProp.backgroundColor: item_state.backgroundColor
    MD.MatProp.stateLayerColor: item_state.stateLayerColor

    property color outlineColor: item_state.outlineColor

    MD.State {
        id: item_state
        visible: false

        elevation: MD.Token.elevation.level0
        textColor: MD.Token.color.on_surface
        backgroundColor: "transparent"
        supportTextColor: MD.Token.color.on_surface_variant
        property color outlineColor: MD.Token.color.outline

        states: [
            State {
                name: "Disabled"
                when: !enabled
                PropertyChanges {
                    item_state.supportTextColor: MD.Token.color.on_surface
                    control.contentItem.opacity: 0.38
                    control.background.opacity: 0.12
                }
            },
            State {
                name: "Hovered"
                when: control.enabled && control.acceptableInput && control.hovered && !control.focus
                PropertyChanges {
                    item_state.outlineColor: MD.Token.color.on_surface
                }
            },
            State {
                name: "Focused"
                when: control.enabled && control.acceptableInput && control.focus
                PropertyChanges {
                    item_state.outlineColor: MD.Token.color.primary
                }
            },
            State {
                name: "Error"
                when: control.enabled && !control.acceptableInput && !control.hovered
                PropertyChanges {
                    item_state.textColor: MD.Token.color.on_surface
                    item_state.supportTextColor: MD.Token.color.error
                    item_state.outlineColor: MD.Token.color.error
                }
            },
            State {
                name: "ErrorHover"
                when: control.enabled && !control.acceptableInput && control.hovered
                PropertyChanges {
                    item_state.textColor: MD.Token.color.on_surface
                    item_state.supportTextColor: MD.Token.color.error
                    item_state.outlineColor: MD.Token.color.on_error_container
                }
            }
        ]
    }
}
