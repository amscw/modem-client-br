#include "client.h"
#include <QApplication>
#include <QTime>

std::string clientExc_c::strErrorMessages[] = {
        "can't open connection",
};

std::string errEvt_c::errMsg[] = {
        "ok",
        "the host was not found",
        "the remote host is closed",
        "The connection was refused",
        "Received the connected() signal",
        "tx fail",
        "disconnected from host",
};

QByteArray client_c::buf;

client_c::client_c(const QObject *observer) noexcept : observer(observer), tcpSocket(std::make_unique<QTcpSocket>())
{
    // хз пока зачем это. Требуется для обработки сигналов сокета в другом потоке
    qRegisterMetaType<QAbstractSocket::SocketError>();

    // подключаемся к сигналам сокета
    // ошибка сокета
    connect(tcpSocket.get(), SIGNAL(error(QAbstractSocket::SocketError)), SLOT(slotError(QAbstractSocket::SocketError)));
    // сокет подключился к серверу
    connect(tcpSocket.get(), SIGNAL(connected()), SLOT(slotConnected()));
    // сокет отключился от сервера
    connect(tcpSocket.get(), SIGNAL(disconnected()), SLOT(slotDisconnected()));
}

client_c::~client_c() noexcept
{
    if (tcpSocket.get() != nullptr)
    {
        // отключаемся от слотов передачи
        disconnect(this, SIGNAL(sigTxMsg()), this, SLOT(slotTxMsg()));
        disconnect(this, SIGNAL(sigTxMsg()), this, SLOT(slotTxMsg()));

        // разрываем все связи с сокетом
        disconnect(tcpSocket.get(), SIGNAL(disconnected()), this, SLOT(slotDisconnected()));
        disconnect(tcpSocket.get(), SIGNAL(connected()), this, SLOT(slotConnected()));
        disconnect(tcpSocket.get(), SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(slotError(QAbstractSocket::SocketError)));

        // отключаем сокет от хоста
        if (tcpSocket->state() == QTcpSocket::SocketState::ConnectedState)
        {
            tcpSocket->disconnectFromHost();
            QApplication::postEvent(const_cast<QObject*>(observer), new errEvt_c(errEvt_c::errCode_t::ERR_DISCONNECTED));
        }
    }
}

void client_c::ConnectTo(const std::string &to, std::uint16_t port)
{
    tcpSocket->connectToHost(QString::fromStdString(to), port);
}

void client_c::slotError(QAbstractSocket::SocketError se)
{
    errEvt_c::errCode_t errCode;

    switch (se)
    {
    case QAbstractSocket::HostNotFoundError:
        errCode = errEvt_c::errCode_t::ERR_HOST_NOT_FOUND;
        break;

    case QAbstractSocket::RemoteHostClosedError:
        errCode = errEvt_c::errCode_t::ERR_REMOTE_HOST_CLOSED;

        // отключаемся от слотов передачи
        disconnect(this, SIGNAL(sigTxMsg()), this, SLOT(slotTxMsg()));
        disconnect(this, SIGNAL(sigTxMsg()), this, SLOT(slotTxMsg()));
        break;

    case QAbstractSocket::ConnectionRefusedError:
        errCode = errEvt_c::errCode_t::ERR_CONN_REFUSED;
        break;

    default:
        errCode = errEvt_c::errCode_t::ERR_DEFAULT;
    }

    QApplication::postEvent(const_cast<QObject*>(observer), new errEvt_c(errCode, tcpSocket->errorString().toStdString()));
}

void client_c::slotConnected()
{
    QApplication::postEvent(const_cast<QObject*>(observer), new errEvt_c(errEvt_c::errCode_t::ERR_CONNECTED));

    // !!!WARNING!!!
    // после данного вызова события сокета не будут обработаны до запуска потока!
    // поток запускается снаружи
    moveToThread(this);

    // сокет был создан в главном потоке, чтобы ловить его сигналы, требуется его перенести бэк
    tcpSocket->moveToThread(this);

    // подключемся к слотам передачи информации
    connect(this, SIGNAL(sigTxMsg()), SLOT(slotTxMsg()));
    connect(this, SIGNAL(sigTxImg()), SLOT(slotTxImg()));
}

void client_c::slotDisconnected()
{
    // отключаемся от слотов передачи
    disconnect(this, SIGNAL(sigTxMsg()), this, SLOT(slotTxMsg()));
    disconnect(this, SIGNAL(sigTxMsg()), this, SLOT(slotTxMsg()));

    // сообщаем наблюдателю о разрыве соединения
    QApplication::postEvent(const_cast<QObject*>(observer), new errEvt_c(errEvt_c::errCode_t::ERR_DISCONNECTED));
}

void client_c::customEvent(QEvent *event)
{
    txEvt_c *txevt;
    //QDataStream out(&buf, QIODevice::WriteOnly);
    mailHeader_c *h;
    std::ostringstream oss;

    switch (static_cast<int>(event->type()))
    {        
        case txEvt_c::EVT_TYPE_TX:
            // событие запроса на передачу
            txevt = dynamic_cast<txEvt_c*>(event);

            // готовим заголовок
            buf.clear();
            buf.append(sizeof (mailHeader_c), 0);
            h = reinterpret_cast<mailHeader_c*>(buf.data());
            h->Time = QTime::currentTime();
            h->MsgType = txevt->Msgtype();


            // в зависимости от типа сообщения используются разные механизмы передачи
            switch(txevt->Msgtype())
            {
                case mailHeader_c::msgType_t::MSG_TYPE_TEXT:
                    // добавляем payload
                    buf.append(txevt->Msg().c_str(), static_cast<int>(txevt->Msg().length()));

                    // адрес данных в куче мог измениться после перераспределения, так что переопределить h
                    h = reinterpret_cast<mailHeader_c*>(buf.data());

                    // установить размер
                    h->Size = buf.size();

                    TRACE(oss);
                    emit sigTxMsg();
                    break;

                case mailHeader_c::msgType_t::MSG_TYPE_IMAGE:
                    // добавляем payload
                    buf.append(txevt->Imagebytes());

                    // адрес данных в куче мог измениться после перераспределения, так что переопределить h
                    h = reinterpret_cast<mailHeader_c*>(buf.data());

                    // установить размер
                    h->Size = buf.size();
                    oss << "set sizeof mail = " << h->Size;

                    TRACE(oss);
                    emit sigTxImg();
                    break;
            }
            event->accept();
            break;
    }
    QThread::customEvent(event);
}

void client_c::slotTxMsg()
{
    qint64 err;
    QEvent *evt;
    std::ostringstream oss;
    std::uint32_t *p = (std::uint32_t*)buf.data();

    oss << "mail size = " << *p ;
    TRACE(oss);

    // отправить посылку в сокет
    err = tcpSocket->write(buf);

    // шлем отчет о передаче
    if (err == -1) {
        evt = new errEvt_c(errEvt_c::errCode_t::ERR_TX_FAIL);
    } else {
        evt = new txReportEvt_c(err);
    }
    QApplication::postEvent(const_cast<QObject*>(observer), evt);
}

void client_c::slotTxImg()
{
    qint64 err = -1, bytes, last;
    QEvent *evt;
    QByteArray::const_iterator it;
    bool fail;

     needStopTx = false;

     // сообщить о начале передачи
     evt = new txReportEvt_c(0, buf.size());
     QApplication::postEvent(const_cast<QObject*>(observer), evt);


    // шлем блоки, размером CHUNK_SIZE
    bytes = 0;
    for (it = buf.begin(); it <= (buf.end() - CHUNK_SIZE); it += CHUNK_SIZE)
    {
        // отправка очередной посылки
        err = tcpSocket->write(&(*it), CHUNK_SIZE);
        fail = !tcpSocket->waitForBytesWritten(2000);

        QThread::msleep(200);
        if (err == -1 || fail) {
            evt = new errEvt_c(errEvt_c::errCode_t::ERR_TX_FAIL);
            QApplication::postEvent(const_cast<QObject*>(observer), evt);
            break;
        } else {
            bytes += err;
            txReportEvt_c::txStatus_t st =
                    (bytes == err) ? txReportEvt_c::txStatus_t::TX_STAT_STARTED :
                    (bytes < buf.size()) ? txReportEvt_c::txStatus_t::TX_STAT_CONTINUE :
                    txReportEvt_c::txStatus_t::TX_STAT_END;
            evt = new txReportEvt_c(bytes, buf.size(), st);
            QApplication::postEvent(const_cast<QObject*>(observer), evt);
        }

        // передача может идти долго, за это время могли прийти новые события или сигналы
        // пользователь мог прервать передачу
        // TODO: прежде, чем разрешить это, надо быть уверенным, что пользователь не запустит еще одну передачу!
        // QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents); // не работает, блять!
        mutex.lock();
        if (needStopTx)
        {
            evt = new txReportEvt_c(bytes, buf.size(), txReportEvt_c::txStatus_t::TX_STAT_END);
            QApplication::postEvent(const_cast<QObject*>(observer), evt);
            mutex.unlock();
            return;
        }
        mutex.unlock();
    }

    // хвост досылаем
    last = buf.end() - it;
    if (err != -1 && last > 0)
    {
        err = tcpSocket->write(&(*it), last);
        fail = !tcpSocket->waitForBytesWritten(2000);
        if (err == -1 || fail) {
            evt = new errEvt_c(errEvt_c::errCode_t::ERR_TX_FAIL);
        } else {
            bytes += err;
            evt = new txReportEvt_c(bytes, buf.size(), txReportEvt_c::txStatus_t::TX_STAT_END);
        }
        QApplication::postEvent(const_cast<QObject*>(observer), evt);
    }
}

void client_c::StopTx() noexcept
{
    mutex.lock();
    needStopTx = true;
    mutex.unlock();
}
