#include <QBuffer>
#include "picwgt.h"

picwgt_c::picwgt_c(const QString &filename, QWidget *pwgt) : QWidget(pwgt),
    m_pImg(std::make_unique<QImage>(filename)) //, m_grayscaleImg(m_pImg->convertToFormat(QImage::Format ::Format_Grayscale8))
{
    setFixedSize(m_pImg->size());
}

picwgt_c::picwgt_c(const QByteArray &imageArray, QWidget *pwgt) : QWidget(pwgt),
    m_pImg(std::make_unique<QImage>(QImage::fromData(imageArray, "JPG")))
{
    setFixedSize(m_pImg->size());
}

void picwgt_c::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.drawImage(0, 0, *m_pImg);
}

const QByteArray &picwgt_c::GetImageArray()
{
    QBuffer buf(&m_imageArray);
    buf.open(QIODevice::WriteOnly);
    m_pImg->save(&buf, "JPG");
    qDebug() << "image size: " << m_imageArray.size() << " bytes";
    return m_imageArray;
}
