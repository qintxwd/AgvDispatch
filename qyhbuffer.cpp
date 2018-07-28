#include "qyhbuffer.h"
#include <memory.h>

QyhBuffer::QyhBuffer(int size)
{
    buf.reserve(size);
    clear();
}

QyhBuffer::QyhBuffer(const QyhBuffer &b)
{
    buf = b.buf;
}
QyhBuffer::QyhBuffer(const char *data,int len)
{
    if (data == NULL) {
        buf.reserve(len);
        clear();
    } else { // Consume the provided array
        buf.reserve(len);
        clear();
        append(data, len);
    }
}

QyhBuffer::QyhBuffer(const std::vector<char> &b)
{
    buf = b;
}

std::unique_ptr<QyhBuffer> QyhBuffer::clone()
{
    std::unique_ptr<QyhBuffer> ret = std::make_unique<QyhBuffer>(buf);
    return ret;
}

uint32_t QyhBuffer::size()
{
    return length();
}

int QyhBuffer::find(char key,int start)
{
    int ret = -1;
    uint32_t len = buf.size();
    for (uint32_t i = start; i < len; i++) {
        if (buf[i] == key) {
            ret = (int) i;
            break;
        }
    }
    return ret;
}

int QyhBuffer::getInt32(int start) const
{
    int ret;
    if(start + sizeof(int32_t)>buf.size())return -1;

    memcpy(&ret,&buf[start],sizeof(int32_t));
    return ret;

//    return *((int32_t*) &buf[start]);
}

const char *QyhBuffer::data(int start) const
{
    if(empty()) return nullptr;
    if(start>buf.size())return nullptr;
    return &buf[start];
}

void QyhBuffer::append(const char *data,int len)
{
    if(data == NULL || len == 0) return;
    buf.resize(length() + len,0);
    memcpy(&buf[0] + length() - len,data,len);
}

QyhBuffer &QyhBuffer::operator=(const QyhBuffer &other)
{
    buf = other.buf;
    return *this;
}

QyhBuffer &QyhBuffer::operator+=(const QyhBuffer &other)
{
    if(!other.empty()){
        buf.insert(buf.end(), other.buf.begin(), other.buf.end());
    }
    return *this;
}

bool QyhBuffer::operator == (const QyhBuffer &other)
{
    return buf == other.buf;
}

void QyhBuffer::clear()
{
    buf.clear();
}

void QyhBuffer::removeFront(int len)
{
    if(len<=0)return ;
    if(len > length())
    {
        clear();
    }else{
        buf.erase(buf.begin(),buf.begin()+len);
    }
}
