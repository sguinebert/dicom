#ifndef TOOLS_H
#define TOOLS_H

#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <ostream>
#include <cassert>

using uchar = unsigned char;

std::size_t be_char_to_16b(std::array<uchar, 2> bs)
{
    //assert(bs.size() == 2);
    std::size_t sz = 0;
    sz |= (static_cast<std::size_t>(bs[0]) << 8);
    sz |= (static_cast<std::size_t>(bs[1]));
    return sz;
}
std::size_t be_char_to_32b(std::array<uchar, 4> bs)
{
    //assert(bs.size() == 4);
    std::size_t sz = 0;
    sz |= (static_cast<std::size_t>(bs[0]) << 24);
    sz |= (static_cast<std::size_t>(bs[1]) << 16);
    sz |= (static_cast<std::size_t>(bs[2]) << 8);
    sz |= (static_cast<std::size_t>(bs[3]));
    return sz;
}
std::array<uchar, 2> ui_to_16b_be(unsigned val)
{
    std::array<unsigned char, 2> be_val;
    //std::vector<unsigned char> be_val(2);
    be_val[0] = ((val & 0xFF00) >> 8);
    be_val[1] = (val & 0xFF);
    return be_val;
}
std::array<uchar, 4> ui_to_32b_be(unsigned val)
{
    std::array<uchar, 4> be_val;
    //std::vector<unsigned char> be_val(4);
    be_val[0] = ((val & 0xFF000000) >> 24);
    be_val[1] = ((val & 0xFF0000)) >> 16;
    be_val[2] = ((val & 0xFF00) >> 8);
    be_val[3] = (val & 0xFF);
    return be_val;
}


#endif // TOOLS_H
