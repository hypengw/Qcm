import QtQuick

Canvas {
    id: canvas

    required property Image image
    readonly property int status: canvas.image.status

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
    onPaint: {
        if (image.paintedHeight === 0)
            return ;

        const ctx = getContext("2d");
        const c = canvas;
        const ratio = Screen.devicePixelRatio;
        ctx.beginPath();
        ctx.arc(c.width / 2, c.height / 2, Math.min(c.width, c.height) / 2, 0, Math.PI * 2, false);
        ctx.closePath();
        ctx.clip();
        ctx.scale(1 / Screen.devicePixelRatio, 1 / Screen.devicePixelRatio);
        ctx.drawImage(image, image.x, image.y, c.width * ratio, c.width * ratio);
        ctx.scale(Screen.devicePixelRatio, Screen.devicePixelRatio);
    }
    onImageLoaded: {
        requestPaint();
    }
}
