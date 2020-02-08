#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "common.h"
#include "client.h"
#include "picwgt.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    std::unique_ptr<client_c> client;
    std::unique_ptr<picwgt_c> picwgt;
    void destroyClient() noexcept;

protected:
    void customEvent(QEvent *event);

private slots:
    void on_pbConnect_clicked();
    void on_pbDisconnect_clicked();
    void on_pbSend_clicked();

    void on_pbOpenImg_clicked();

    void on_cbMsgType_currentIndexChanged(int index);

signals:
    void sigSendMsg(const QString &msg);
    void sigQuit();
};
#endif // MAINWINDOW_H
