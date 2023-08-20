import QtQuick

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
    property alias paintedHeight: image.paintedHeight
    property alias paintedWidth: image.paintedWidth
    property alias progress: image.progress
    property alias smooth: image.smooth
    property alias source: image.source
    property alias sourceClipRect: image.sourceClipRect
    property alias sourceSize: image.sourceSize
    property alias status: image.status
    property alias verticalAlignment: image.verticalAlignment

    implicitHeight: image.implicitHeight
    implicitWidth: image.implicitWidth

    onStatusChanged: {
        if (status === Image.Ready)
            canvas.imageLoaded();
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
        if (image.paintedHeight === 0)
            return;
        const ctx = getContext("2d");
        const c = canvas;
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
    }
}
