#ifndef GPS_H_
#define GPS_H_
#include <QElapsedTimer>
#include <QTimer>
#include <QCanBusDevice>
#include <QUdpSocket>

#include "serialport.h"

class DashBoard;
class Serialport;

class GPS : public QObject
{
    Q_OBJECT

 public:
    explicit GPS(QObject *parent = 0);
    explicit GPS(DashBoard *dashboard, QObject *parent = 0);
    Q_INVOKABLE void defineFinishLine(const qreal & Y1, const qreal & X1, const qreal & Y2, const qreal & X2);
    Q_INVOKABLE void defineFinishLine2(const qreal & Y1, const qreal & X1, const qreal & Y2, const qreal & X2);
    Q_INVOKABLE void resetLaptimer();

 private:
    DashBoard *m_dashboard;
    SerialPort *m_serialport;
    // CUSTOM FIRMWARE: GPS speed/time out on CAN (see sendGpsCan in gps.cpp)
    QCanBusDevice *m_canOut = nullptr;
    QUdpSocket *m_emuCanOut = nullptr;   // emulator (Windows): frames go to UDP 45460 instead of can0
    QUdpSocket *m_emuGpsIn = nullptr;    // emulator (Windows): NMEA arrives on UDP 45455
    void sendGpsCan(double kph, const QString &utc, bool valid);
    QByteArray gpsFrame(double kph, const QString &utc, bool valid);
    void sendCanFrame(quint32 id, const QByteArray &payload);
    QByteArray  m_readData;
    QByteArray  m_buffer;
    QElapsedTimer m_timer;
    QTimer m_timeouttimer;
    QTimer m_reconnecttimer;
    float convertToFloat(const QString & coord, const QString & dir);
    void processGPRMC(const QString &line);
    void checklinecrossed();
    void linecrossed();
    void processGPGGA(const QString &line);
    void processGPVTG(const QString & line);
    void checknewLap();



 public slots:
    // void delaytimer();
    void openConnection(const QString &portName, const QString &Baud);
    void ProcessMessage(QByteArray messageline);
    void removeNMEAmsg();
    void setGPSBAUD115();
    void setGPS10HZ();
   // void setGPSOnly();
    void closeConnection();
    void clear();

 private slots:
    void readyToRead();
    void handleTimeout();
    void handleReconnect();
    void handleError(QSerialPort::SerialPortError error);
    void initSerialPort();
    void readEmulatorGps();
    void logNMEA(const QString & line);

 signals:
    void sig_linecrossed();
};

#endif  // GPS_H_
