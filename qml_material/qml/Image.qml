import QtQuick
import Qcm.Material as MD

Item {
    id: root
    // tl,tr,bl,br
    property var radius: [0]
    property alias asynchronous: image.asynchronous
    property alias autoTransform: image.autoTransform
    property alias cache: image.cache
    property alias currentFrame: image.currentFrame
    property alias fillMode: image.fillMode
    property alias frameCount: image.frameCount
    property alias horizontalAlignment: image.horizontalAlignment
    property alias mipmap: image.mipmap
    property alias mirror: image.mirror
    property alias mirrorVertically: image.mirrorVertically
    property alias progress: image.progress
    property alias source: image.source
    property alias sourceClipRect: image.sourceClipRect
    property alias sourceSize: image.sourceSize
    property alias status: image.status
    property alias verticalAlignment: image.verticalAlignment

    property alias paintedHeight: image.paintedHeight
    property alias paintedWidth: image.paintedWidth

    implicitHeight: {
        return sourceSize.height;
    }
    implicitWidth: {
        return sourceSize.width;
    }

    MD.MatProp.elevation: MD.Token.elevation.level0
    layer.enabled: root.status === Image.Ready && root.paintedHeight > 0 && root.MD.MatProp.elevation != MD.Token.elevation.level0
    layer.effect: MD.RoundedElevationEffect {
        elevation: root.MD.MatProp.elevation
    }

    Image {
        id: image
        anchors.fill: parent

        fillMode: Image.PreserveAspectFit

        layer.enabled: true
        layer.effect: ShaderEffect {
            property var radius: MD.Util.corner(root.radius).toVector4D()
            property var size: root.height
            fragmentShader: 'qrc:/Qcm/Material/assets/shader/round.frag.qsb'
        }
    }
}
/*
Canvas {
    id: canvas

    property var radius: [0]
    property alias asynchronous: image.asynchronous
    property alias autoTransform: image.autoTransform
    property alias cache: image.cache
    property alias currentFrame: image.currentFrame
    property alias fillMode: image.fillMode
    property alias frameCount: image.frameCount
    property alias horizontalAlignment: image.horizontalAlignment
    property alias mipmap: image.mipmap
    property alias mirror: image.mirror
    property alias mirrorVertically: image.mirrorVertically
    property alias progress: image.progress
    property alias source: image.source
    property alias sourceClipRect: image.sourceClipRect
    property alias sourceSize: image.sourceSize
    property alias status: image.status
    property alias verticalAlignment: image.verticalAlignment

    property alias paintedHeight: image.paintedHeight
    property alias paintedWidth: image.paintedWidth

    implicitHeight: {
        return sourceSize.width;
    }
    implicitWidth: {
        return sourceSize.height;
    }

    onStatusChanged: {
        if (status === Image.Ready) {
            requestPaint();
        }
    }
    Component.onCompleted: {
        image.visible = false;
        image.parent = canvas;
    }
    function roundRect(ctx, x, y, width, height, radius) {
        if (typeof radius === 'number') {
            radius = [radius];
        }
        if (radius.length === 1)
            radius = {
                "tl": radius[0],
                "tr": radius[0],
                "br": radius[0],
                "bl": radius[0]
            };
        else if (radius.length === 2)
            radius = {
                "tl": radius[0],
                "tr": radius[0],
                "br": radius[1],
                "bl": radius[1]
            };
        else if (radius.length === 3)
            radius = {
                "tl": radius[0],
                "tr": radius[1],
                "br": radius[2],
                "bl": radius[1]
            };
        else
            radius = {
                "tl": radius[0],
                "tr": radius[1],
                "br": radius[2],
                "bl": radius[3]
            };
        ctx.moveTo(x + radius.tl, y);
        ctx.lineTo(x + width - radius.tr, y);
        ctx.quadraticCurveTo(x + width, y, x + width, y + radius.tr);
        ctx.lineTo(x + width, y + height - radius.br);
        ctx.quadraticCurveTo(x + width, y + height, x + width - radius.br, y + height);
        ctx.lineTo(x + radius.bl, y + height);
        ctx.quadraticCurveTo(x, y + height, x, y + height - radius.bl);
        ctx.lineTo(x, y + radius.tl);
        ctx.quadraticCurveTo(x, y, x + radius.tl, y);
    }
    onPaint: {
        const c = canvas;
        if (c.paintedHeight === 0)
            return;
        const ctx = getContext("2d");
        const ratio = Screen.devicePixelRatio;
        ctx.beginPath();
        roundRect(ctx, 0, 0, c.width, c.height, c.radius);
        ctx.closePath();
        ctx.clip();
        ctx.scale(1 / Screen.devicePixelRatio, 1 / Screen.devicePixelRatio);
        ctx.drawImage(image, image.x, image.y, c.width * ratio, c.width * ratio);
        ctx.scale(Screen.devicePixelRatio, Screen.devicePixelRatio);
    }
    onImageLoaded: {
        requestPaint();
    }

    Image {
        id: image
        visible: false
        smooth: root.smooth
    }

    MD.MatProp.elevation: MD.Token.elevation.level0
    layer.enabled: canvas.status === Image.Ready && canvas.paintedHeight > 0 && canvas.MD.MatProp.elevation != MD.Token.elevation.level0
    layer.effect: MD.RoundedElevationEffect {
        elevation: canvas.MD.MatProp.elevation
    }
}

*/
