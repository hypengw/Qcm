import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.Material.impl as MDImpl
import Qcm.Material as MD

MD.TextFieldEmbed {
    id: control

    property int type: MD.Enum.TextFieldOutlined
    property string leading_icon
    property string trailing_icon

    font.capitalization: Font.MixedCase

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, Math.max(contentWidth, placeholder.implicitWidth) + leftPadding + rightPadding + (leading.visible ? leading.implicitWidth : 0) + (trailing.visible ? trailing.implicitWidth : 0))
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    // If we're clipped, set topInset to half the height of the placeholder text to avoid it being clipped.
    topInset: clip ? placeholder.largestHeight / 2 : 0
    bottomInset: 0

    leftPadding: 16
    rightPadding: 16
    topPadding: 0
    bottomPadding: 0

    // Need to account for the placeholder text when it's sitting on top.
    //topPadding: Material.containerStyle === Material.Filled
    //    ? placeholderText.length > 0 && (activeFocus || length > 0)
    //        ? Material.textFieldVerticalPadding + placeholder.largestHeight
    //        : Material.textFieldVerticalPadding
    //    // Account for any topInset (used to avoid floating placeholder text being clipped),
    //    // otherwise the text will be too close to the background.
    //    : Material.textFieldVerticalPadding + topInset

    MDImpl.FloatingPlaceholderText {
        id: placeholder
        x: control.leftPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        elide: Text.ElideRight
        renderType: control.renderType

        filled: control.type === MD.Enum.TextFieldFilled
        verticalPadding: 8
        controlHasActiveFocus: control.activeFocus
        controlHasText: control.length > 0
        controlImplicitBackgroundHeight: control.implicitBackgroundHeight
        controlHeight: control.height
    }

    property Item leading: MD.Icon {
        anchors.left: parent?.left
        anchors.verticalCenter: parent?.verticalCenter
        anchors.leftMargin: 12
        name: control.leading_icon
        visible: name
        size: 24
    }

    property Item trailing: MD.Icon {
        anchors.right: parent?.right
        anchors.verticalCenter: parent?.verticalCenter
        anchors.rightMargin: 12
        visible: name
        name: control.trailing_icon
        size: 24
    }

    data: [placeholder, leading, trailing]

    background: MDImpl.MaterialTextContainer {
        implicitWidth: 64
        implicitHeight: 56

        filled: control.type === MD.Enum.TextFieldFilled
        fillColor: control.MD.MatProp.backgroundColor
        outlineColor: control.outlineColor
        focusedOutlineColor: control.outlineColor
        placeholderTextWidth: Math.min(placeholder.width, placeholder.implicitWidth) * placeholder.scale
        controlHasActiveFocus: control.activeFocus
        controlHasText: control.length > 0
        placeholderHasText: placeholder.text.length > 0
        horizontalPadding: 16
    }

    MD.MatProp.elevation: item_state.elevation
    MD.MatProp.textColor: item_state.textColor
    MD.MatProp.supportTextColor: item_state.supportTextColor
    MD.MatProp.backgroundColor: item_state.backgroundColor
    MD.MatProp.stateLayerColor: item_state.stateLayerColor

    color: item_state.textColor
    placeholderTextColor: item_state.placeholderColor

    property color outlineColor: item_state.outlineColor

    MD.State {
        id: item_state
        item: control

        elevation: MD.Token.elevation.level0
        textColor: item_state.ctx.color.on_surface
        backgroundColor: "transparent"
        supportTextColor: item_state.ctx.color.on_surface_variant
        property color placeholderColor: item_state.ctx.color.on_surface_variant
        property color outlineColor: item_state.ctx.color.outline

        states: [
            State {
                name: "Disabled"
                when: !enabled
                PropertyChanges {
                    item_state.placeholderColor: item_state.ctx.color.on_surface
                    item_state.supportTextColor: item_state.ctx.color.on_surface
                    placeholder.opacity: 0.38
                    control.background.opacity: 0.12
                }
            },
            State {
                name: "ErrorHover"
                when: !control.acceptableInput && control.hovered
                PropertyChanges {
                    item_state.textColor: item_state.ctx.color.on_surface
                    item_state.supportTextColor: item_state.ctx.color.error
                    item_state.placeholderColor: item_state.ctx.color.on_error_container
                    item_state.outlineColor: item_state.ctx.color.on_error_container
                }
            },
            State {
                name: "Error"
                when: !control.acceptableInput
                PropertyChanges {
                    item_state.textColor: item_state.ctx.color.on_surface
                    item_state.supportTextColor: item_state.ctx.color.error
                    item_state.placeholderColor: item_state.ctx.color.error
                    item_state.outlineColor: item_state.ctx.color.error
                }
            },
            State {
                name: "Focused"
                when: control.focus
                PropertyChanges {
                    item_state.placeholderColor: item_state.ctx.color.primary
                    item_state.outlineColor: item_state.ctx.color.primary
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        const c = item_state.ctx.color.primary;
                        return MD.Util.transparent(c, MD.Token.state.pressed.state_layer_opacity);
                    }
                }
            },
            State {
                name: "Hovered"
                when: control.hovered
                PropertyChanges {
                    item_state.placeholderColor: item_state.ctx.color.on_surface
                    item_state.outlineColor: item_state.ctx.color.on_surface
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        const c = item_state.ctx.color.primary;
                        return MD.Util.transparent(c, MD.Token.state.hover.state_layer_opacity);
                    }
                }
            }
        ]
    }
}
