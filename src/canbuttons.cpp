#include "canbuttons.h"

#include <QCanBus>
#include <QCanBusDevice>
#include <QCanBusFrame>
#include <QHostAddress>
#include <QUdpSocket>

// CUSTOM FIRMWARE: dash buttons on CAN.
//
// id 0x6A1 (dash -> bus), 8 bytes, sent 10 times a second and straight away when a button is tapped:
//   byte 0   CAN button 1 (0 = off, 1 = on)   e.g. map switch: off = map 1, on = map 2
//   byte 1   CAN button 2
//   byte 2   CAN button 3
//   byte 3   CAN button 4
//   byte 4   bit 0 = 1 once the dash has heard the relay box (0x6A2) and shows its states
//   byte 5-6 0
//   byte 7   counter, +1 every frame (lets a receiver tell the dash is still alive)
//
// id 0x6A2 (relay box -> dash), 10 Hz: bytes 0-3 = the states the box has saved and is driving,
// byte 5 = what the box senses on its inputs (bit 0 = "CAN input 1" ... bit 3 = input 4), e.g. whether
// the ECU is running the fan. The inputs read 0 if no 0x6A2 arrives for 1 s.
// The box keeps its states through a power cycle and switches its outputs as soon as it powers up,
// long before the dash has booted. Until the dash has heard 0x6A2 it sends byte 4 = 0 and the box
// ignores it, so the dash starting up with every button off can't undo the saved states. Once
// synced, a tap on the dash changes the state and the box follows (and saves it). Without a box,
// the buttons start off and work as before.
//
// On the dash the frames use can0. In the Windows emulator 0x6A1 goes out as candump-style text
// "6A1#..." to UDP 127.0.0.1:45460 (can_monitor.py), 45461 (fake_me442.py) and 45463
// (fake_relay_box.py), and "6A2#..." is read from UDP 45462 (from fake_relay_box.py).

CanButtons::CanButtons(QObject *parent) : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &CanButtons::send);
    m_timer.start(100);
#ifdef Q_OS_WIN
    m_emuCanIn = new QUdpSocket(this);
    if (m_emuCanIn->bind(QHostAddress::LocalHost, 45462))
        connect(m_emuCanIn, &QUdpSocket::readyRead, this, &CanButtons::readEmuFrames);
#endif
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
    m_lastTap.start();
    setState(button - 1, v);
    send();
    m_timer.start(100);     // keep an even 10 Hz after the immediate frame
}

void CanButtons::setState(int index, int v)
{
    if (m_state[index] == v)
        return;
    m_state[index] = v;
    switch (index) {
    case 0: emit canbutton1Changed(); break;
    case 1: emit canbutton2Changed(); break;
    case 2: emit canbutton3Changed(); break;
    case 3: emit canbutton4Changed(); break;
    }
}

void CanButtons::boxFrame(const QByteArray &payload)
{
    if (payload.size() < Count)
        return;
    m_lastBox.start();
    const int in = payload.size() > 5 ? (quint8(payload.at(5)) & 0x0F) : 0;
    if (in != m_input) {
        m_input = in;
        emit caninputsChanged();
    }
    // Just after a tap the box may still report the old state for a frame or two; don't flick back.
    if (m_lastTap.isValid() && m_lastTap.elapsed() < 500)
        return;
    for (int i = 0; i < Count; ++i)
        setState(i, payload.at(i) ? 1 : 0);
    m_synced = true;
}

void CanButtons::send()
{
    if (m_input && m_lastBox.isValid() && m_lastBox.elapsed() > 1000) {
        m_input = 0;                            // box gone quiet: don't show stale inputs
        emit caninputsChanged();
    }
    QByteArray p(8, 0);
    for (int i = 0; i < Count; ++i)
        p[i] = char(m_state[i]);
    p[4] = char(m_synced ? 1 : 0);
    p[7] = char(m_counter++);
    sendCanFrame(FrameId, p);
}

void CanButtons::readEmuFrames()
{
    while (m_emuCanIn && m_emuCanIn->hasPendingDatagrams()) {
        QByteArray d(int(m_emuCanIn->pendingDatagramSize()), 0);
        m_emuCanIn->readDatagram(d.data(), d.size());
        const int hash = d.indexOf('#');
        if (hash > 0 && d.left(hash).trimmed().toUInt(nullptr, 16) == BoxFrameId)
            boxFrame(QByteArray::fromHex(d.mid(hash + 1).trimmed()));
    }
}

void CanButtons::sendCanFrame(quint32 id, const QByteArray &payload)
{
#ifdef Q_OS_WIN
    if (!m_emuCanOut)
        m_emuCanOut = new QUdpSocket(this);
    const QByteArray text = QByteArray::number(id, 16).toUpper().rightJustified(3, '0') + "#" + payload.toHex().toUpper();
    m_emuCanOut->writeDatagram(text, QHostAddress::LocalHost, 45460);   // can_monitor.py
    m_emuCanOut->writeDatagram(text, QHostAddress::LocalHost, 45461);   // fake_me442.py (the "ECU")
    m_emuCanOut->writeDatagram(text, QHostAddress::LocalHost, 45463);   // fake_relay_box.py
#else
    if (!m_canOut) {
        QString err;
        m_canOut = QCanBus::instance()->createDevice("socketcan", "can0", &err);
        if (!m_canOut)
            return;                     // try again on the next frame
        // Qt queues every received frame until read, and the ECU sends hundreds a second.
        // Let the kernel pass only the relay box's 0x6A2, and read whatever arrives.
        QCanBusDevice::Filter box;
        box.frameId = BoxFrameId;
        box.frameIdMask = 0x7FFu;
        box.type = QCanBusFrame::DataFrame;
        box.format = QCanBusDevice::Filter::MatchBaseFormat;
        m_canOut->setConfigurationParameter(QCanBusDevice::RawFilterKey,
                                            QVariant::fromValue(QList<QCanBusDevice::Filter>{box}));
        connect(m_canOut, &QCanBusDevice::framesReceived, this, [this]() {
            const QVector<QCanBusFrame> frames = m_canOut->readAllFrames();
            for (const QCanBusFrame &f : frames)
                if (f.frameId() == BoxFrameId && f.frameType() == QCanBusFrame::DataFrame)
                    boxFrame(f.payload());
        });
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
