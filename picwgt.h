#ifndef PICWGT_H
#define PICWGT_H

#include <memory>
#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QDebug>
/**
 * @brief The picwgt class
 * QPainter - отрисовщик. С помощью него можно рисовать на любом QPaintDevice, используя указатель на объект, унаследованный
 * от QPaintDevice в качестве параметра.
 */
class picwgt_c : public QWidget
{
    Q_OBJECT

private:
    std::unique_ptr<QImage> m_pImg;
    // QImage m_grayscaleImg;
    QByteArray m_imageArray;

public:
     picwgt_c(const QString &filename, QWidget* pwgt=0);
     picwgt_c(const QByteArray &imageArray, QWidget* pwgt=0);
     ~picwgt_c(){ qDebug() << "picwgt destroyed"; }

    const QByteArray &GetImageArray();

protected:
     virtual void paintEvent(QPaintEvent *event);
};

#endif // PICWGT_H
