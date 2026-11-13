#ifndef CANBUTTONS_H
#define CANBUTTONS_H

// CUSTOM FIRMWARE: on-screen buttons that send their state to the ECU on CAN.
// A "State gauge" on a dash page with source "CAN button 1..4" becomes a toggle button:
// tap it to switch the button on/off; it shows its OFF / ON picture like any state gauge.

#include <QObject>
#include <QTimer>
#include <QByteArray>

class QCanBusDevice;
class QUdpSocket;

class CanButtons : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int canbutton1 READ canbutton1 NOTIFY canbutton1Changed)
    Q_PROPERTY(int canbutton2 READ canbutton2 NOTIFY canbutton2Changed)
    Q_PROPERTY(int canbutton3 READ canbutton3 NOTIFY canbutton3Changed)
    Q_PROPERTY(int canbutton4 READ canbutton4 NOTIFY canbutton4Changed)

public:
    static const int Count = 4;
    static const quint32 FrameId = 0x6A1;

    explicit CanButtons(QObject *parent = nullptr);

    int canbutton1() const { return m_state[0]; }
    int canbutton2() const { return m_state[1]; }
    int canbutton3() const { return m_state[2]; }
    int canbutton4() const { return m_state[3]; }

    Q_INVOKABLE void toggle(int button);          // button 1..4
    Q_INVOKABLE void set(int button, int on);

signals:
    void canbutton1Changed();
    void canbutton2Changed();
    void canbutton3Changed();
    void canbutton4Changed();

private:
    void send();
    void sendCanFrame(quint32 id, const QByteArray &payload);

    int m_state[Count] = {0, 0, 0, 0};
    quint8 m_counter = 0;
    QTimer m_timer;
    QCanBusDevice *m_canOut = nullptr;
    QUdpSocket *m_emuCanOut = nullptr;
};

#endif // CANBUTTONS_H
