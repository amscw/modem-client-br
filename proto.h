#ifndef PROTO_H
#define PROTO_H

#include "common.h"
#include<QTime>

/*
 * | size  | type | QTime | payload
 * |4 bytes|1 byte|4bytes | дохера bytes
 */

struct mailHeader_c
{
    quint32 Size;
    enum class msgType_t : std::uint8_t {MSG_TYPE_TEXT, MSG_TYPE_IMAGE} MsgType;
    QTime Time;
} __attribute__((__packed__));

#endif // PROTO_H
