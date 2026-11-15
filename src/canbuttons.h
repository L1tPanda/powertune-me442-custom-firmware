#ifndef CANBUTTONS_H
#define CANBUTTONS_H

// CUSTOM FIRMWARE: on-screen buttons that send their state to the ECU on CAN.
// A "State gauge" on a dash page with source "CAN button 1..4" becomes a toggle button:
// tap it to switch the button on/off; it shows its OFF / ON picture like any state gauge.
// When a CAN relay box is on the bus, the box keeps the states and the dash shows the box's.

#include <QObject>
#include <QTimer>
#include <QByteArray>
#include <QElapsedTimer>

class QCanBusDevice;
class QUdpSocket;

class CanButtons : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int canbutton1 READ canbutton1 NOTIFY canbutton1Changed)
    Q_PROPERTY(int canbutton2 READ canbutton2 NOTIFY canbutton2Changed)
    Q_PROPERTY(int canbutton3 READ canbutton3 NOTIFY canbutton3Changed)
    Q_PROPERTY(int canbutton4 READ canbutton4 NOTIFY canbutton4Changed)
    Q_PROPERTY(int caninput1 READ caninput1 NOTIFY caninputsChanged)
    Q_PROPERTY(int caninput2 READ caninput2 NOTIFY caninputsChanged)
    Q_PROPERTY(int caninput3 READ caninput3 NOTIFY caninputsChanged)
    Q_PROPERTY(int caninput4 READ caninput4 NOTIFY caninputsChanged)

public:
    static const int Count = 4;
    static const quint32 FrameId = 0x6A1;       // dash -> bus: button states
    static const quint32 BoxFrameId = 0x6A2;    // relay box -> dash: the box's saved states

    explicit CanButtons(QObject *parent = nullptr);

    int canbutton1() const { return m_state[0]; }
    int canbutton2() const { return m_state[1]; }
    int canbutton3() const { return m_state[2]; }
    int canbutton4() const { return m_state[3]; }
    int caninput1() const { return m_input & 1 ? 1 : 0; }   // sensed by the relay box (0x6A2 byte 5)
    int caninput2() const { return m_input & 2 ? 1 : 0; }
    int caninput3() const { return m_input & 4 ? 1 : 0; }
    int caninput4() const { return m_input & 8 ? 1 : 0; }

    Q_INVOKABLE void toggle(int button);          // button 1..4
    Q_INVOKABLE void set(int button, int on);

signals:
    void canbutton1Changed();
    void canbutton2Changed();
    void canbutton3Changed();
    void canbutton4Changed();
    void caninputsChanged();

private:
    void send();
    void sendCanFrame(quint32 id, const QByteArray &payload);
    void boxFrame(const QByteArray &payload);
    void setState(int index, int v);
    void readEmuFrames();

    int m_state[Count] = {0, 0, 0, 0};
    bool m_synced = false;          // heard the relay box at least once
    QElapsedTimer m_lastTap;        // ignore the box's echo for a moment after a tap
    QElapsedTimer m_lastBox;        // last 0x6A2; inputs read 0 if the box goes quiet
    int m_input = 0;                // sensed inputs, bit 0 = CAN input 1
    quint8 m_counter = 0;
    QTimer m_timer;
    QCanBusDevice *m_canOut = nullptr;
    QUdpSocket *m_emuCanOut = nullptr;
    QUdpSocket *m_emuCanIn = nullptr;
};

#endif // CANBUTTONS_H
