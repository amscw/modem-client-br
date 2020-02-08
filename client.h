#ifndef CLIENT_H
#define CLIENT_H

#include <QTcpSocket>
#include <QWidget>
#include <QThread>
#include <QEvent>
#include <QMutex>
#include "proto.h"

#include "common.h"

struct clientExc_c : public exc_c
{
    enum class errCode_t : std::uint32_t {
        ERROR_OPEN,
    } m_errCode;

    clientExc_c(enum errCode_t code, const std::string &strFile, const std::string &strFunction, const std::string &strErrorDescription = "") noexcept :
            exc_c(strFile, strFunction, strErrorDescription), m_errCode(code)
    {}

    const std::string &Msg() const noexcept override { return strErrorMessages[static_cast<int>(m_errCode)]; }
    void ToStderr() const noexcept override
    {
        std::cerr << "WTF:" << m_strFile << "(" << m_strFunction << "):" << strErrorMessages[static_cast<int>(m_errCode)] << "-" << m_strErrorDescription << std::endl;
    }

    std::string ToString() noexcept override
    {
        oss.str("");
        oss.clear();
        oss << "WTF:" << m_strFile << "(" << m_strFunction << "):" << strErrorMessages[static_cast<int>(m_errCode)] << "-" << m_strErrorDescription;
        return oss.str();
    }

    const errCode_t &Errcode() const noexcept{ return m_errCode; }

    private:
        static std::string strErrorMessages[];
        std::ostringstream oss;
};



class client_c : public QThread
{
    Q_OBJECT

private:
     const int CHUNK_SIZE = 1024;
     const QObject *observer;
     std::unique_ptr<QTcpSocket> tcpSocket = nullptr;
     static QByteArray buf;
     QMutex mutex;
     bool needStopTx{false};

public:

     client_c(const QObject *observer) noexcept;
     virtual ~client_c() noexcept override;
     void ConnectTo(const std::string &to, std::uint16_t port);
     void StopTx() noexcept;
     void run() override
     {
        exec();
     }

protected:
     void customEvent(QEvent *event) override;

signals:
     void sigTxMsg();
     void sigTxImg();

private slots:
    void slotError(QAbstractSocket::SocketError);
    void slotConnected();
    void slotDisconnected();
    void slotTxMsg();
    void slotTxImg();
};

/**
 * Events
 */
class errEvt_c : public QEvent
{
    static std::string errMsg[];

public:
    enum {EVT_TYPE_ERR = QEvent::User + 1}; // need to pass it into c-tor
    enum class errCode_t : std::int32_t {
        ERR_DEFAULT = -1,
        ERR_OK = 0,
        ERR_HOST_NOT_FOUND,
        ERR_REMOTE_HOST_CLOSED,
        ERR_CONN_REFUSED,
        ERR_CONNECTED,
        ERR_TX_FAIL,
        ERR_DISCONNECTED,
    };


private:
    errCode_t errCode{errCode_t::ERR_DEFAULT};
    std::string defaultErrMsg;

public:
    errEvt_c(errCode_t err, const std::string dem = "") : QEvent(static_cast<Type>(EVT_TYPE_ERR)), errCode(err), defaultErrMsg(dem) {}
    const errCode_t &Errcode() const noexcept { return errCode; }
    const std::string &Errmsg() const noexcept { return errCode == errCode_t::ERR_DEFAULT ? defaultErrMsg : errMsg[static_cast<int>(errCode)]; }
};

/**
 * @brief The clientTxEvt_c class
 * репорт о состоянии передачи
 */
class txReportEvt_c : public QEvent
{
    qint64 bytes, total;    // ушло байтов, всего байтов

public:
    enum {EVT_TYPE_TX = QEvent::User + 2}; // need to pass it into c-tor
    enum class txStatus_t : std::uint32_t {TX_STAT_IDLE, TX_STAT_STARTED, TX_STAT_CONTINUE, TX_STAT_END };

    // конструктор, используемый при передаче текста
    txReportEvt_c(qint64 n) : QEvent(static_cast<Type>(EVT_TYPE_TX)),
        bytes(n), total(n), msgType(mailHeader_c::msgType_t::MSG_TYPE_TEXT), txStatus(txStatus_t::TX_STAT_END){}

    // конструктор, используемый для передачи изображений
    txReportEvt_c(qint64 n, qint64 tb, txStatus_t status = txStatus_t::TX_STAT_IDLE) : QEvent(static_cast<Type>(EVT_TYPE_TX)),
        bytes(n), total(tb), msgType(mailHeader_c::msgType_t::MSG_TYPE_IMAGE), txStatus(status){}

    // деструктор
    virtual ~txReportEvt_c(){}


    // аксессоры
    inline const qint64& Bytes() const noexcept { return bytes; }
    inline const qint64& Total() const noexcept {return total; }
    inline const txStatus_t & TxStatus() const noexcept { return txStatus; }
    inline const mailHeader_c::msgType_t &Msgtype() const noexcept {return msgType; }
    inline int Percent() const noexcept { return static_cast<int>(qRound( static_cast<double>(bytes) * 100.0 / total)); }

private:
    typename mailHeader_c::msgType_t msgType;
    txStatus_t txStatus {txStatus_t::TX_STAT_IDLE};
};

/**
 * @brief The hostTxEvt_c class
 * запрос на передачу клиентом
 */
class txEvt_c : public QEvent
{
    typename mailHeader_c::msgType_t msgtype;
    QByteArray img;
    std::string str;

public:
    enum {EVT_TYPE_TX = QEvent::User + 3};
    txEvt_c(const std::string &s) noexcept : QEvent(static_cast<Type>(EVT_TYPE_TX)), msgtype(mailHeader_c::msgType_t::MSG_TYPE_TEXT), str(s) {}
    txEvt_c(const QByteArray &ba) noexcept : QEvent(static_cast<Type>(EVT_TYPE_TX)), msgtype(mailHeader_c::msgType_t::MSG_TYPE_IMAGE), img(ba) {}
    inline const mailHeader_c::msgType_t &Msgtype() const noexcept { return msgtype; }
    inline const std::string &Msg() const noexcept { return str; }
    inline const QByteArray &Imagebytes() const noexcept { return img; }
};

#endif // CLIENT_H
