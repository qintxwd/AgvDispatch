
#include "msg.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <cctype>


namespace lynx {
/* * * * * * * * * * * * * * * * * * * *
 * static global
 */
namespace statics {
    static const std::string empty_string;
    static const Msg::object empty_object;
    static const Msg::array  empty_array;
    static const Msg         null_msg;
    static const std::string with_crlf = "";
}


struct NullStruct {};

static bool isword(int c) {
    return std::isdigit(c) || std::isalpha(c) || c == '_';
}

/* * * * * * * * * * * * * * * * * * * *
 * Serialize
 */
static void dump(NullStruct, std::string &out) {
    out += '"';
}

static void dump(const std::string& value, std::string& out) {
    out += "'";
    for (size_t i = 0; i < value.length(); i++) {
        const char ch = value[i];
        if (ch == '\\') {
            out += "\\\\";
        } else if (ch == '"') {
            out += "\\\"";
        } else if (ch == '\b') {
            out += "\\b";
        } else if (ch == '\f') {
            out += "\\f";
        } else if (ch == '\n') {
            out += "\\n";
        } else if (ch == '\r') {
            out += "\\r";
        } else if (ch == '\t') {
            out += "\\t";
        } else if (static_cast<uint8_t>(ch) <= 0x1f) {
            char buf[8];
            snprintf(buf, sizeof buf, "\\u%04x", ch);
            out += buf;
        } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1]) == 0x80
                   && static_cast<uint8_t>(value[i+2]) == 0xa8) {
            out += "\\u2028";
            i += 2;
        } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1]) == 0x80
                   && static_cast<uint8_t>(value[i+2]) == 0xa9) {
            out += "\\u2029";
            i += 2;
        } else {
            out += ch;
        }
    }
    out += "'";
}

static void dump(const Msg::array& values, std::string& out) {
    auto len = values.size();
    out += "L[";
    out += std::to_string(len);
    out += "]";
    out += statics::with_crlf;
    for (size_t i=0; i<len; i++) {
        out += values.at(i).dump();
        if (i != len-1) out += statics::with_crlf;
    }
}

static void dump(const Msg::object& value, std::string& out) {
    int         len;
    std::string key;
    Msg         val;
    std::tie(len, key, val) = value;
    out = out + "<A[" + std::to_string(len) +
            "] " + key + " ID=" + val.dump() + ">";
}

/* * * * * * * * * * * * * * * * * * * *
 * Value wrappers
 */
template <Msg::Type tag, typename T>
class Value : public MsgValue {
protected:
    // Constructors
    explicit Value(const T& value) : m_value(value) {}
    explicit Value(T&& value)      : m_value(std::move(value)) {}

    // Get type tag
    Msg::Type type() const override {
        return tag;
    }

    const T m_value;
    void dump(std::string &out) const override { lynx::dump(m_value, out); }
};

struct MsgNull final: public Value<Msg::NUL, NullStruct> {
    MsgNull() : Value({}) {}
};

class MsgString final: public Value<Msg::STRING, std::string> {
    const std::string &string_value() const override { return m_value; }
public:
    explicit MsgString(const std::string &value) : Value(value) {}
    explicit MsgString(std::string &&value)      : Value(std::move(value)) {}
};

class MsgArray final : public Value<Msg::ARRAY, Msg::array> {
    const Msg::array &array_items() const override { return m_value; }
    const Msg& operator[](size_t i) const override { 
        if (i >= m_value.size()) {
            assert(0);
            return statics::null_msg;
        }
        else return m_value[i];
    }
    const Msg& operator[](const std::string& key) const override { 
        for (const auto& msg: m_value) {
            if (msg.is_object()) {
                std::string k;
                std::tie(std::ignore, k, std::ignore) = msg.object_value();
                if (k == key) 
                    return std::get<2>(msg.object_value());
            }
        }
        assert(0);
        return statics::null_msg;
    }
public:
    explicit MsgArray(const Msg::array &value) : Value(value) {}
    explicit MsgArray(Msg::array &&value)      : Value(std::move(value)) {}
};

class MsgObject final : public Value<Msg::OBJECT, Msg::object> {
    const Msg::object &object_value() const override { return m_value; }
public:
    explicit MsgObject(const Msg::object &value) : Value(value) {}
    explicit MsgObject(Msg::object &&value)      : Value(std::move(value)) {}
};


/* * * * * * * * * * * * * * * * * * * *
 * static global
 */
namespace statics {
    static const std::shared_ptr<MsgValue> null = std::make_shared<MsgNull>();
}

/* * * * * * * * * * * * * * * * * * * *
 * MsgValue
 */
const std::string& MsgValue::string_value() const   {return statics::empty_string;}
const Msg::object& MsgValue::object_value() const   {return statics::empty_object;}
const Msg::array& MsgValue::array_items() const     {return statics::empty_array;}
const Msg& MsgValue::operator[](size_t) const       {return statics::null_msg;}
const Msg& MsgValue::operator[](const std::string&) const     {return statics::null_msg;}

/* * * * * * * * * * * * * * * * * * * *
 * Msg
 */
Msg::Msg()                          :m_ptr(statics::null) {}
Msg::Msg(std::nullptr_t)            :m_ptr(statics::null) {}
Msg::Msg(const std::string& value)  :m_ptr(std::make_shared<MsgString>(value)) {}
Msg::Msg(std::string&& value)       :m_ptr(std::make_shared<MsgString>(std::move(value))) {}
Msg::Msg(const char *value)         :m_ptr(std::make_shared<MsgString>(value)) {}
Msg::Msg(const array& values)       :m_ptr(std::make_shared<MsgArray>(values)) {}
Msg::Msg(array&& values)            :m_ptr(std::make_shared<MsgArray>(std::move(values))) {}
Msg::Msg(const object& value)       :m_ptr(std::make_shared<MsgObject>(value)){}
Msg::Msg(object&& value)            :m_ptr(std::make_shared<MsgObject>(std::move(value))){}

Msg::Type Msg::type() const {return m_ptr->type();}

// getters
const std::string&  Msg::string_value() const   {return m_ptr->string_value();}
const Msg::object&  Msg::object_value() const   {return m_ptr->object_value();}
const Msg::array&   Msg::array_items() const    {return m_ptr->array_items();}
const Msg& Msg::operator[](size_t i) const      {return (*m_ptr)[i];}
const Msg& Msg::operator[](const std::string& key) const  {return (*m_ptr)[key];}

// serialize
std::string Msg::dump() const {
    std::string out;
    m_ptr->dump(out);
    return out;
}

struct MsgParser final {
    // context
    const std::string& str;
    size_t i;
    std::string& err;
    bool failed;

    MsgParser(const std::string& _str,std::string& _err):
        str(_str),
        err(_err),
        i(0),
        failed(false)
    {
    }

    template<class T>
    T fail(std::string&& msg, const T err_ret) {
        if (!failed)
            err = std::move(msg);
        failed = true;
        return err_ret;
    }

    void consume_whitespace() {
        while(std::isspace(str[i])) i++;
    }

    char get_next_token() {
        consume_whitespace();
        if (i == str.size())
            return fail("unexpected end of input", (char)0);

        return str[i++];
    }

    int parse_int() {
        if (get_next_token() != '[')
            return fail("expected L[n]", 0);

        size_t start_pos = i;
        while(std::isdigit(str[i])) i++;
        int n = std::stoi(str.c_str() + start_pos);

        if (get_next_token() != ']')
            return fail("expected L[n]", 0);
        return n;
    }

    bool expect(const std::string& expected) {
        assert(i != 0);
        bool as_expect = (str.compare(i-1, expected.length(), expected) == 0);
        if (as_expect) {
            i += (expected.length() - 1);
        }
        return as_expect;
    }

    //
    Msg parse_msg() {
        char ch = get_next_token();
        if (failed) return statics::null_msg;

        if (ch == '"') {
            return statics::null_msg;
        }

        if (ch == '\'') {
            size_t start_pos = i;
            while (i < str.size() && str[i] != '\'') i++;

            if (i == str.size())
                return fail("expected '<str>'", statics::null_msg);

            // skip '
            i++;
            return Msg(std::string(str, start_pos, i-start_pos-1));
        }

        if (ch == '<') {
            if (get_next_token() != 'A')
                return fail("expected <A[n]", statics::null_msg);

            // parse int
            int n = parse_int();
            if (failed) return statics::null_msg;

            // parse key
            ch = get_next_token();
            size_t start_pos = i - 1;
            while (isword(ch)) {
                ch = str[i++];
            }

            if (!isspace(ch))
                return fail("expected space or object key with error", statics::null_msg);
            std::string key(str, start_pos, i-start_pos-1);

            // parse Msg(string) / Msg()
            ch = get_next_token();
            if (!expect("ID"))
                return fail("expected ID", statics::null_msg);

            ch = get_next_token();
            if (!expect("="))
                return fail("expected =", statics::null_msg);

            Msg msg = parse_msg();
            ch = get_next_token();
            if (ch != '>')
                return fail("expected >", statics::null_msg);

            return Msg(Msg::object{n, key, msg});
        }

        if (ch == 'L') {
            int len = parse_int();
            if (len < 0 && failed) return statics::null_msg;

            Msg::array data;
            for (size_t j=0; j<(size_t)len; j++) {
                data.push_back(parse_msg());
                if (failed) return statics::null_msg;
            }
            return data;
        }

        return statics::null_msg;
    }
};

// deserialize
Msg Msg::parse(const std::string& in, std::string& err) {
    MsgParser parser(in,err);
    Msg msg = parser.parse_msg();

    parser.consume_whitespace();
    if (parser.i != in.size())
        return parser.fail("unexpected trailing", Msg());

    return msg;
}

static std::tuple<int, int, int> parseSF(const std::string& msg)
{
    size_t i=0;
    int S=0, F=0;
    bool succ = false;
    // consume space
    while (i<msg.size() && std::isspace(msg[i])) i++;
    do {
        // get S
        if (i>=msg.size() || msg[i] != 'S') break;
        i++;
        // parse S
        S = std::stoi(msg.c_str()+i);
        while(i < msg.size() && std::isdigit(msg[i])) i++;
        // get F
        if (i>=msg.size() || msg[i] != 'F') break;
        i++;
        // parse F
        F = std::stoi(msg.c_str()+i);
        while(i < msg.size() && std::isdigit(msg[i])) i++;

        succ = true;
    } while (0);

    if (!succ)
        return std::make_tuple(-1, S, F);
    return std::make_tuple((int)i, S, F);
}

std::tuple<int, int, Msg> parseMsg(const std::string& msg, std::string& err)
{
    int i = 0, S, F;
    std::tie(i, S, F) = lynx::parseSF(msg);

    if (i == -1) err = "parse SF error";
    auto msg_data = lynx::Msg::parse(msg.c_str()+i, err);
    return std::make_tuple(S, F, msg_data);
}

std::tuple<int, int, Msg> parseMsg(const char *in, std::string& err)
{
    return lynx::parseMsg(std::string(in), err);
}

} // end of namespace
