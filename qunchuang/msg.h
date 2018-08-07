#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <sstream>


namespace lynx{

class MsgValue;

class Msg final
{
public:
    // Types
    enum Type {
        NUL, STRING, ARRAY, OBJECT
    };

    // typedef
    typedef std::vector<Msg> array;
    typedef std::tuple<int, std::string, Msg> object;

    // Construtor
    Msg();
    Msg(std::nullptr_t);
    Msg(const std::string& value);
    Msg(std::string&& value);
    Msg(const char *value);
    Msg(const array& values);
    Msg(array&& values);
    Msg(const object& value);
    Msg(object&& value);

    Type type() const;
    bool is_null() const        {return type() == NUL;}
    bool is_string() const      {return type() == STRING;}
    bool is_array() const       {return type() == ARRAY;}
    bool is_object() const      {return type() == OBJECT;}

    // getters
    const std::string&  string_value() const;
    const object&       object_value() const;
    const array&        array_items() const;
    // Return a reference to arr[i] if this is an array, Msg() otherwise.
    const Msg & operator[](size_t i) const;
    const Msg &operator[](const std::string& key) const;

    // serialize
    std::string dump() const;
    // deserialize
    static Msg parse(const std::string& in, std::string& err);
    static Msg parse(const char *in, std::string& err) {
        if (in) {
            return Msg::parse(std::string(in), err);
        } else {
            err = "null input";
            return nullptr;
        }
    }

private:
    std::shared_ptr<MsgValue> m_ptr;
};

// Internal class hierarchy
class MsgValue {
protected:
    friend class Msg;
    virtual Msg::Type type() const = 0;
    virtual void dump(std::string& out) const = 0;
    virtual const std::string& string_value() const;
    virtual const Msg::object& object_value() const;
    virtual const Msg::array& array_items() const;
    virtual const Msg &operator[](size_t i) const;
    virtual const Msg &operator[](const std::string& key) const;
    virtual ~MsgValue() {}
};
//
//template<class T>
//static Msg make_object(int n, const std::string& key, const T& val) {
//    std::ostringstream oss;
//    oss << val;
//    return Msg({n, key, Msg(oss.str())});
//}
//
//template<>
//static Msg make_object(int n, const std::string& key, const Msg& val) {
//    return Msg({n, key, val});
//}
//
//    template<>
//    static Msg make_object(int n, const std::string& key, const object& val) {
//        return Msg({n, key, Msg(val)});
//    }
//
//    template<>
//    static Msg make_object(int n, const std::string& key, const array& val) {
//        return Msg({n, key, Msg(val)});
//    }


std::tuple<int, int, Msg> parseMsg(const std::string& msg, std::string& err);
std::tuple<int, int, Msg> parseMsg(const char *in, std::string& err);

} // end of namespace

