/****************************************************************************
**                                MIT License
**
** Copyright (C) 2018-2023 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
** Author: Sérgio Martins <sergio.martins@kdab.com>
**
** This file is part of KDToolBox (https://github.com/KDAB/KDToolBox).
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, ** and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice (including the next paragraph)
** shall be included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF ** CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
****************************************************************************/

#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QString>
#include <QVector>
#include <QtQml/private/qqmlengine_p.h>

extern "C" char *qt_v4StackTrace(void *executionContext);

namespace KDAB
{

QString qmlStackTrace(QV4::ExecutionEngine *engine)
{
    return QString::fromUtf8(qt_v4StackTrace(engine->currentContext()));
}

void printQmlStackTraces()
{
    const auto windows = qApp->topLevelWindows();
    for (QWindow *w : windows)
    {
        if (auto qw = qobject_cast<QQuickWindow *>(w))
        {
            QQuickItem *item = qw->contentItem();
            QQmlContext *context = QQmlEngine::contextForObject(item);
            if (!context)
                continue;
            QQmlEnginePrivate *enginePriv = QQmlEnginePrivate::get(context->engine());
            QV4::ExecutionEngine *v4engine = enginePriv->v4engine();
            qDebug() << "Stack trace for" << qw;
            qDebug().noquote() << qmlStackTrace(v4engine);
            qDebug() << "\n";
        }
    }
}

}