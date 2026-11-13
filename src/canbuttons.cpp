#include "canbuttons.h"

#include <QCanBus>
#include <QCanBusDevice>
#include <QCanBusFrame>
#include <QHostAddress>
#include <QUdpSocket>

// CUSTOM FIRMWARE: dash buttons on CAN.
//
// id 0x6A1, 8 bytes, sent 10 times a second and straight away when a button is tapped:
//   byte 0   CAN button 1 (0 = off, 1 = on)   e.g. map switch: off = map 1, on = map 2
//   byte 1   CAN button 2
//   byte 2   CAN button 3
//   byte 3   CAN button 4
//   byte 4-6 0
//   byte 7   counter, +1 every frame (lets the ECU tell the dash is still alive)
//
// Every button starts OFF when the dash starts.
// On the dash the frame goes out on can0; in the Windows emulator it is sent as candump-style
// text "6A1#..." to UDP 127.0.0.1:45460 (can_monitor.py) and 45461 (fake_me442.py).

CanButtons::CanButtons(QObject *parent) : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &CanButtons::send);
    m_timer.start(100);
}

void CanButtons::toggle(int button)
{
    if (button >= 1 && button <= Count)
        set(button, m_state[button - 1] ? 0 : 1);
}

void CanButtons::set(int button, int on)
{
    if (button < 1 || button > Count)
        return;
    const int v = on ? 1 : 0;
    if (m_state[button - 1] == v)
        return;
    m_state[button - 1] = v;
    switch (button) {
    case 1: emit canbutton1Changed(); break;
    case 2: emit canbutton2Changed(); break;
    case 3: emit canbutton3Changed(); break;
    case 4: emit canbutton4Changed(); break;
    }
    send();
    m_timer.start(100);     // keep an even 10 Hz after the immediate frame
}

void CanButtons::send()
{
    QByteArray p(8, 0);
    for (int i = 0; i < Count; ++i)
        p[i] = char(m_state[i]);
    p[7] = char(m_counter++);
    sendCanFrame(FrameId, p);
}

void CanButtons::sendCanFrame(quint32 id, const QByteArray &payload)
{
#ifdef Q_OS_WIN
    if (!m_emuCanOut)
        m_emuCanOut = new QUdpSocket(this);
    const QByteArray text = QByteArray::number(id, 16).toUpper().rightJustified(3, '0') + "#" + payload.toHex().toUpper();
    m_emuCanOut->writeDatagram(text, QHostAddress::LocalHost, 45460);   // can_monitor.py
    m_emuCanOut->writeDatagram(text, QHostAddress::LocalHost, 45461);   // fake_me442.py (the "ECU")
#else
    if (!m_canOut) {
        QString err;
        m_canOut = QCanBus::instance()->createDevice("socketcan", "can0", &err);
        if (!m_canOut)
            return;                     // try again on the next frame
        // Send-only: Qt queues every received frame until read, and the ECU sends hundreds a
        // second. Accept only an extended id nothing uses, and drain whatever still arrives.
        QCanBusDevice::Filter none;
        none.frameId = 0x1FFFFFFFu;
        none.frameIdMask = 0x1FFFFFFFu;
        none.type = QCanBusFrame::DataFrame;
        none.format = QCanBusDevice::Filter::MatchExtendedFormat;
        m_canOut->setConfigurationParameter(QCanBusDevice::RawFilterKey,
                                            QVariant::fromValue(QList<QCanBusDevice::Filter>{none}));
        connect(m_canOut, &QCanBusDevice::framesReceived, this, [this]() { m_canOut->readAllFrames(); });
        if (!m_canOut->connectDevice()) {
            delete m_canOut;
            m_canOut = nullptr;
            return;
        }
    }
    if (m_canOut->state() == QCanBusDevice::ConnectedState)
        m_canOut->writeFrame(QCanBusFrame(id, payload));
#endif
}
