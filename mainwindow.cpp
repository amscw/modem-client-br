#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "proto.h"
#include <QFileDialog>

/**
 * @brief MainWindow::MainWindow
 * @param parent
 *
 * Концепция: клиент создается при нажатии на кнопку Connect в соответствующем слоте.
 * Клиент уничтожается по нажатию на кнопку "Disconnect" ИЛИ в случае сбоя. Т.к. удаление имеет место в разных местах, необходим единый метод!
 * Входящие сообщения клиента - это события.
 * Исходящие сообщения доставляются клиенту посредством сигналов или также событий.
 */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->cbMsgType->setCurrentIndex(0);
    ui->pbOpenImg->setEnabled(false);
    ui->leInput->setReadOnly(false);
    ui->leInput->setFocus();
    ui->pbSend->setEnabled(false);
}

MainWindow::~MainWindow()
{

    if (client.get() != nullptr)
    {
        // останавливаем поток клиента
        if (client->isRunning())
        {
            // если идет передача - прервать
            client->StopTx();
            // client->quit();
            emit sigQuit();
            client->wait();
        }

        // удаляем клиента
        client.reset();
    }
        delete ui;
}

void MainWindow::customEvent(QEvent *event)
{
    std::ostringstream oss;
    errEvt_c *errevt;
    txReportEvt_c *txevt;

    switch (static_cast<int>(event->type()))
    {
    case errEvt_c::EVT_TYPE_ERR:
        errevt = dynamic_cast<errEvt_c *>(event);
        ui->teInfo->append(QString::fromStdString(errevt->Errmsg()));
        switch(errevt->Errcode())
        {
        case errEvt_c::errCode_t::ERR_CONN_REFUSED:
        case errEvt_c::errCode_t::ERR_HOST_NOT_FOUND:
        case errEvt_c::errCode_t::ERR_REMOTE_HOST_CLOSED:
            // сбой. Грохнуть клиента
            destroyClient();
            break;

        case errEvt_c::errCode_t::ERR_CONNECTED:
            // изменить функцию кнопки "Connect" --> "Disconnect"
            disconnect(ui->pbConnect, SIGNAL(clicked(bool)), this, SLOT(on_pbConnect_clicked()));
            connect(ui->pbConnect, SIGNAL(clicked(bool)), SLOT(on_pbDisconnect_clicked()));
            ui->pbConnect->setText("Disconnect");
            ui->pbConnect->setEnabled(true);

            // подписываем клиента на события формы
            // connect(this, SIGNAL(sigSendMsg(const QString&)), client.get(), SLOT(slotTx(const QString&)));
            connect(this, SIGNAL(sigQuit()), client.get(), SLOT(quit()));

            // запускаем поток клиента
            client->start();

            // можно слать сообщения
            ui->pbSend->setEnabled(true);
            break;
        }
        event->accept();
        break;

    case txReportEvt_c::EVT_TYPE_TX:
        txevt = dynamic_cast<txReportEvt_c *>(event);
        switch(txevt->Msgtype())
        {
        case mailHeader_c::msgType_t::MSG_TYPE_TEXT:
            oss << "-->" << ui->leInput->text().toStdString() <<  " (" << txevt->Bytes() << " bytes)";
            ui->teInfo->append(QString::fromStdString(oss.str()));
            ui->leInput->setText("");
            break;

        case mailHeader_c::msgType_t::MSG_TYPE_IMAGE:
            switch(txevt->TxStatus())
            {
            case txReportEvt_c::txStatus_t::TX_STAT_IDLE:
                // заголовок отправлен, но передача еще не началась
                picwgt.reset();
                picwgt = nullptr;
                ui->leInput->setText("");
                oss << "-->start image transmitting (total - " << txevt->Total() << " bytes)...";
                ui->teInfo->append(QString::fromStdString(oss.str()));
                ui->prbTxProcess->setValue(0);
                ui->pbSend->setEnabled(false);
                break;

            case txReportEvt_c::txStatus_t::TX_STAT_STARTED:
            case txReportEvt_c::txStatus_t::TX_STAT_CONTINUE:
                ui->prbTxProcess->setValue(txevt->Percent());
                break;

            case txReportEvt_c::txStatus_t::TX_STAT_END:
                ui->prbTxProcess->setValue(txevt->Percent());
                oss << "done! " << txevt->Bytes() << "/" << txevt->Total() << " bytes fired";
                ui->teInfo->append(QString::fromStdString(oss.str()));
                ui->pbSend->setEnabled(true);
                break;

            }

            break;
        }

        event->accept();
        break;
    }
    QMainWindow::customEvent(event);
}

void MainWindow::on_pbConnect_clicked()
{
    // создать клиента
    client = std::make_unique<client_c>(this);

    // отключить кнопку - неопределенная функция кнопки.
    QPushButton *pb = dynamic_cast<QPushButton*>(sender());
    pb->setEnabled(false);

    // попытка связаться с указанным хостом
    client->ConnectTo(ui->leSrvIP->text().toStdString(), static_cast<std::uint16_t>(ui->sbPort->value()));
}

void MainWindow::on_pbDisconnect_clicked()
{
    destroyClient();
}

void MainWindow::on_pbSend_clicked()
{
    // emit sigSendMsg(ui->leInput->text());
    if (client.get() != nullptr && client->isRunning())
    {
        switch (ui->cbMsgType->currentIndex())
        {
            case 0:
                // передать текст
                QApplication::postEvent(client.get(), new txEvt_c(ui->leInput->text().toStdString()));
            break;

            case 1:
                // передать изображение
                if (picwgt.get() != nullptr) {
                    QApplication::postEvent(client.get(), new txEvt_c(picwgt->GetImageArray()));
                } else {
                    ui->teInfo->append("open image first!");
                }
            break;
        }
    } else {
        // не запущен цикл обработки сообщений
        ui->teInfo->append("client is not ready, try connect first");
    }
}

void MainWindow::destroyClient() noexcept
{
    if (client.get() != nullptr)
    {        
        // останавливаем поток клиента
        if (client->isRunning())
        {
            // если идет передача - прервать
            client->StopTx();
            // client->quit();
            emit sigQuit();
            client->wait();
        }

        // отписываем клиента от сигналов формы
        // disconnect(this, SIGNAL(sigSendMsg(const QString&)), client.get(), SLOT(slotTx(const QString&)));
        disconnect(this, SIGNAL(sigQuit()), client.get(), SLOT(quit()));

        // удаляем клиента
        client.reset();
    }

    // изменить функцию кнопки "Disconnect" --> "Connect"
    connect(ui->pbConnect, SIGNAL(clicked(bool)), this, SLOT(on_pbConnect_clicked()));
    disconnect(ui->pbConnect, SIGNAL(clicked(bool)), this, SLOT(on_pbDisconnect_clicked()));
    ui->pbConnect->setText("Connect");
    ui->pbConnect->setEnabled(true);
}

void MainWindow::on_pbOpenImg_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "Открыть изображение", QCoreApplication::applicationDirPath(), "*.jpg"); //*.png
     if (filename != "")
     {
         this->picwgt.reset(new picwgt_c(filename));
         if (picwgt.get())
         {
             picwgt->show();
             this->ui->leInput->setText(filename);
         }
     }
}

void MainWindow::on_cbMsgType_currentIndexChanged(int index)
{
    if (index == 1)
    {
        ui->pbOpenImg->setEnabled((true));
        ui->leInput->setReadOnly(true);
    } else {
        ui->pbOpenImg->setEnabled(false);
        ui->leInput->setReadOnly(false);
    }
}
