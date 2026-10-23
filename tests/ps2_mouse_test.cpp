#include "ps2_mouse_report.hpp"
#include <cassert>
#include <climits>
#include <cstdio>
using namespace esp32_ps2dev;
int main()
{
    const int scaled[] = {0,1,1,3,6,9,12,14};
    for (int i=0; i<8; ++i) {
        assert(scale_mouse_axis(i,true)==scaled[i]);
        assert(scale_mouse_axis(-i,true)==-scaled[i]);
        assert(scale_mouse_axis(i,false)==i);
    }
    auto report=encode_mouse_report(-256,255,0,7,false,false,false);
    assert(report.packet.len==3 && report.packet.data[0]==0x1F);
    assert(report.packet.data[1]==0 && report.packet.data[2]==255);
    report=encode_mouse_report(INT32_MIN,INT32_MAX,100,0,true,true,false);
    assert((report.packet.data[0]&0xD0)==0xD0 && report.wheel_consumed==7);
    report=encode_mouse_report(0,0,-100,0x18,false,true,true);
    assert(report.packet.len==4 && report.packet.data[3]==0x38 && report.wheel_consumed==-8);
    report=encode_mouse_report(0,0,-1,0,false,true,false);
    assert(report.packet.data[3]==0xFF);
    report=encode_mouse_report(4,-5,0,0,true,false,false);
    assert(report.packet.data[1]==6 && report.packet.data[2]==uint8_t(-9));
    // READ_DATA bypasses 2:1 scaling (its caller passes two_one=false).
    report=encode_mouse_report(4,-5,0,0,false,false,false);
    assert(report.packet.data[1]==4 && report.packet.data[2]==uint8_t(-5));
    puts("PS/2 mouse scaling, signed limits, wheel and extra button encoding passed");
}
