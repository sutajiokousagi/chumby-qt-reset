/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#define PRESSED_KEY Qt::Key_HomePage

#include "qchumbyresetkb_qws.h"

#include <QSocketNotifier>
#include <QStringList>

#include <qplatformdefs.h>

#include <errno.h>
#include <termios.h>

#include <linux/kd.h>
#include <linux/input.h>

QT_BEGIN_NAMESPACE


class QWSChumbyResetKbPrivate : public QObject
{
    Q_OBJECT
public:
    QWSChumbyResetKbPrivate(QWSChumbyResetKbHandler *, const QString &);
    ~QWSChumbyResetKbPrivate();

private Q_SLOTS:
    void readKeycode();

private:
    QWSChumbyResetKbHandler *m_handler;
    int                           m_fd;
    int                           m_tty_fd;
    struct termios                m_tty_attr;
    int                           m_orig_kbmode;
};

QWSChumbyResetKbHandler::QWSChumbyResetKbHandler(const QString &device)
    : QWSKeyboardHandler(device)
{
    d = new QWSChumbyResetKbPrivate(this, device);
}

QWSChumbyResetKbHandler::~QWSChumbyResetKbHandler()
{
    delete d;
}

bool QWSChumbyResetKbHandler::filterInputEvent(quint16 &, qint32 &)
{
    return false;
}

QWSChumbyResetKbPrivate::QWSChumbyResetKbPrivate(QWSChumbyResetKbHandler *h, const QString &device)
    : m_handler(h), m_fd(-1), m_tty_fd(-1), m_orig_kbmode(K_XLATE)
{
    setObjectName(QLatin1String("chumby Reset-button-to-Keyboard Handler"));

    QString dev = QLatin1String("/dev/input/event0");
    int repeat_delay = -1;
    int repeat_rate = -1;

    QStringList args = device.split(QLatin1Char(':'));
    foreach (const QString &arg, args) {
        if (arg.startsWith(QLatin1String("repeat-delay=")))
            repeat_delay = arg.mid(13).toInt();
        else if (arg.startsWith(QLatin1String("repeat-rate=")))
            repeat_rate = arg.mid(12).toInt();
        else if (arg.startsWith(QLatin1String("/dev/")))
            dev = arg;
    }

    m_fd = QT_OPEN(dev.toLocal8Bit().constData(), O_RDWR, 0);
    if (m_fd >= 0) {
        if (repeat_delay > 0 && repeat_rate > 0) {
            int kbdrep[2] = { repeat_delay, repeat_rate };
            ::ioctl(m_fd, EVIOCSREP, kbdrep);
        }

        QSocketNotifier *notifier;
        notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        connect(notifier, SIGNAL(activated(int)), this, SLOT(readKeycode()));

    } else {
        qWarning("Cannot open keyboard input device '%s': %s", qPrintable(dev), strerror(errno));
        return;
    }
}

QWSChumbyResetKbPrivate::~QWSChumbyResetKbPrivate()
{
    if (m_tty_fd >= 0) {
        ::ioctl(m_tty_fd, KDSKBMODE, m_orig_kbmode);
        tcsetattr(m_tty_fd, TCSANOW, &m_tty_attr);
    }
    if (m_fd >= 0)
        QT_CLOSE(m_fd);
}

void QWSChumbyResetKbPrivate::readKeycode()
{
    struct ::input_event buffer[32];
    int n = 0;

    forever {
        n = QT_READ(m_fd, reinterpret_cast<char *>(buffer) + n, sizeof(buffer) - n);

        if (n == 0) {
            qWarning("Got EOF from the input device.");
            return;
        } else if (n < 0 && (errno != EINTR && errno != EAGAIN)) {
            qWarning("Could not read from input device: %s", strerror(errno));
            return;
        } else if (n % sizeof(buffer[0]) == 0) {
            break;
        }
        else {
            qWarning("Other");
        }
    }

    n /= sizeof(buffer[0]);

    for (int i = 0; i < n; ++i) {
        if (buffer[i].type != EV_KEY)
            continue;

        quint16 code = buffer[i].code;
        qint32 value = buffer[i].value;

        QWSKeyboardHandler::KeycodeAction ka;

        /* Bodge the handler so we always send the same key */
        m_handler->processKeyEvent(code, PRESSED_KEY, Qt::NoModifier, value != 0, value == 2);
    }
}

QT_END_NAMESPACE

#include "qchumbyresetkb_qws.moc"
