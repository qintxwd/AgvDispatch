
#pragma once

#include <string>
#include <vector>

namespace lynx {

    namespace elevator {
        enum CMD {
            TestEleCmd = 0x00,
            CallEleENQ = 0x01,
            CallEleACK = 0x02,
            TakeEleENQ = 0x03,
            TakeEleACK = 0x04,
            IntoEleENQ = 0x05,
            IntoEleACK = 0x06,
            IntoEleSet = 0x0D,
            LeftEleENQ = 0x07,
            LeftEleCMD = 0x0B,
            LeftCMDSet = 0x0E,
            LeftEleACK = 0x08,
            LeftEleSet = 0x0F,
            FullEleENQ = 0x09,
            FullEleACK = 0x0A,
            FloorElErr = 0x0C,
            InitEleENQ = 0x10,
            InitEleACK = 0x11,
            StaEleENQ = 0x12,
            StaEleACK = 0x13,
            EleLightON = 0x14,
            EleLitOnSet = 0x15,
            EleLightOFF = 0x16, 
            EleLitOffSet = 0x17,
            ExComENQ = 0x20,
            ExComACK = 0x21,
            InComENQRobot = 0x22,
            InComENQServ = 0x23,
            PassEleACK = 0x1D,
            PassEleSet = 0x1E,
            AlignEleENQ = 0x24,
            AlignEleACK = 0x25,
        };    

        struct Param {
            typedef unsigned char byte_t;

            byte_t src_floor;
            byte_t dst_floor;
            CMD    cmd;
            byte_t elevator_no;
            byte_t robot_no;

            std::vector<byte_t> serialize() const;
            static Param parse(const byte_t *in, size_t len, std::string& err);
            static Param parse(const std::vector<byte_t>& in, std::string& err) {
                return Param::parse(in.data(), in.size(), err);
            }

            // helper for debug
            std::string debug() const;
            inline bool operator==(const Param& rp) {
                return cmd == rp.cmd && src_floor == rp.src_floor &&
                    dst_floor == rp.dst_floor && elevator_no == rp.elevator_no &&
                    robot_no == rp.robot_no;
            }
        };

    } // namespace elevator

} // namespace lynx


