#ifndef QYHBUFFER_H
#define QYHBUFFER_H

#include <vector>
#include <memory>

#define QYH_BUFFER_DEFAULT_SIZE 1500

class QyhBuffer
{
public:
    QyhBuffer(int size = QYH_BUFFER_DEFAULT_SIZE);
    QyhBuffer(const QyhBuffer &b);
    QyhBuffer(const std::vector<char> &b);
    QyhBuffer(const char *data,int len);
    ~QyhBuffer() = default;

    std::unique_ptr<QyhBuffer> clone();

    uint32_t size(); // Size of internal vector

    int find(char key,int start = 0);

    //如果不够一个int，返回一个-1
    int getInt32(int start = 0) const;

    const char *data(int start) const;

    void append(const char *data,int len);

    QyhBuffer &operator=(const QyhBuffer &other);

    QyhBuffer &operator+=(const QyhBuffer &other);

    bool operator == (const QyhBuffer &other);

    const std::vector<char> &buffer() const { return buf; }

    void clear();

    int length() const { return (int)buf.size(); }

    bool empty() const { return buf.empty(); }

    void removeFront(int len);

public:
    std::vector<char> buf;

};

#endif // QYHBUFFER_H
