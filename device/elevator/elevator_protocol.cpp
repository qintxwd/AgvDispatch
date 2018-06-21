
#include "elevator_protocol.h"
#include <numeric>
#include <vector>
#include <sstream>
#include <iomanip>
#include <string.h>

namespace lynx {

    namespace elevator {
        using byte_t = Param::byte_t;

        namespace statics {
            // 空参数 全局 用于错误返回 (no const)
            // [refer](https://stackoverflow.com/questions/7411515/why-does-c-require-a-user-provided-default-constructor-to-default-construct-a)
            static Param  null_param;
            static const int MSG_SIZE = 8;
            // 协议帧头
            static const byte_t header[2] = {0xAA, 0x55};
        } // namespace statics

        #define HEADER_SIZE  \
            (sizeof(statics::header)/sizeof(statics::header[0]))

        static byte_t check_sum(const std::vector<byte_t>& bytes) {
            int retval = 0;
            retval = std::accumulate(bytes.begin(), bytes.end(), retval);
            return static_cast<byte_t>(retval & 0xff);
        }

        // 调试用, 输出字符串
        std::string Param::debug() const {
            auto&& result = serialize();
            std::ostringstream oss;

            for (auto b: result) {
                oss << std::setfill('0') << std::setw(2)
                    << std::setiosflags(std::ios::uppercase)
                    << std::hex << (int)b << " ";
            }
            return oss.str();
        }

        // 打包信息
        std::vector<byte_t> Param::serialize() const {
            // headers
            std::vector<byte_t> result(statics::header, 
                            statics::header+HEADER_SIZE);

            // parameters
            result.push_back(src_floor);
            result.push_back(dst_floor);
            result.push_back((byte_t)cmd);
            result.push_back(elevator_no);
            result.push_back(robot_no);

            // check sum
            result.push_back(check_sum(result));

            return result;
        }

        // 从字符流解析, err为解析错误返回信息
        Param Param::parse(const byte_t *in, size_t len, std::string& err) {
            // find header
            int i=0;
            for(i=0; i<len; i++) {
                if (strncmp((char *)in+i, (char *)statics::header, HEADER_SIZE) == 0) break;
            }

            // 未找到消息头
            if (i == len) {
                err = "can't find msg header";
                return statics::null_param;
            }
            
            // 消息的长度不足
            if (len-i < statics::MSG_SIZE) {
                err = "wrong msg size";
                return statics::null_param;
            }

            const byte_t *begin = in + i;
            byte_t src_floor   = begin[2];
            byte_t dst_floor   = begin[3];
            CMD    cmd         = (CMD)begin[4];
            byte_t elevator_no = begin[5];
            byte_t robot_no    = begin[6];
            byte_t checksum    = begin[7];

            // 校验消息
            if (checksum != 
                check_sum(std::vector<byte_t>(begin, begin+7))) {
                err = "check sum error";
                return statics::null_param;
            }

            return Param {src_floor, dst_floor, cmd, elevator_no, robot_no};
        }
        
    } // namespace elevator
    
} // namespace lynx


