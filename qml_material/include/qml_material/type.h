#pragma once
#include <QObject>
#include <QQmlEngine>

namespace qml_material
{
class Enum : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    using QObject::QObject;

    enum class IconStyle
    {
        IconRound = 0,
        IconFilled
    };
    Q_ENUM(IconStyle)

    enum class IconLabelStyle
    {
        IconAndText = 0,
        IconOnly,
        TextOnly
    };
    Q_ENUM(IconLabelStyle)

    enum class ButtonType
    {
        BtElevated = 0,
        BtFilled,
        BtFilledTonal,
        BtOutlined,
        BtText
    };
    Q_ENUM(ButtonType)

    enum class IconButtonType
    {
        IBtFilled = 0,
        IBtFilledTonal,
        IBtOutlined,
        IBtStandard
    };
    Q_ENUM(IconButtonType)

    enum class FABType
    {
        FABSmall = 0,
        FABNormal,
        FABLarge
    };
    Q_ENUM(FABType)

    enum class FABColor
    {
        FABColorPrimary = 0,
        FABColorSurfaec,
        FABColorSecondary,
        FABColorTertiary
    };
    Q_ENUM(FABColor)

    enum class CardType
    {
        CardElevated = 0,
        CardFilled,
        CardOutlined
    };
    Q_ENUM(CardType)

    enum class ListItemHeightMode
    {
        ListItemOneLine = 0,
        ListItemTwoLine,
        ListItemThreeine
    };
    Q_ENUM(ListItemHeightMode)
};
} // namespace qml_material