#ifndef NON_COPY_ABLE_H
#define NON_COPY_ABLE_H

class noncopyable
{
protected:
    noncopyable() = default;
    ~noncopyable() = default;
    noncopyable( const noncopyable& ) = delete;
    noncopyable& operator=( const noncopyable& ) = delete;

};

#endif //end NON_COPY_ABLE_H
