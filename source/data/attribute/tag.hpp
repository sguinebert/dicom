#ifndef TAG_TYPE_HPP
#define TAG_TYPE_HPP

#include <ostream>
#include <iomanip>

namespace dicom
{

namespace data
{

namespace attribute
{

/**
 * @brief The tag_type struct represents a tag used as the identifier in the
 *        attribute and as a value field type for AT VRs.
 */
struct tag_type
{
      unsigned short group_id;
      unsigned short element_id;

      tag_type(unsigned short gid = 0, unsigned short eid = 0):
          group_id {gid},
          element_id {eid}
      {
      }
};

std::ostream& operator<<(std::ostream& os, const tag_type tag)
{
    std::ios state(nullptr);
    state.copyfmt(os);

    os << "(" << std::hex << std::setw(4) << std::setfill('0') << tag.group_id
       << "," << std::hex << std::setw(4) << std::setfill('0') << tag.element_id
       << ")";

    os.copyfmt(state);
    return os;
}



bool operator==(const tag_type& lhs, const tag_type& rhs)
{
    return lhs.group_id == rhs.group_id &&
           rhs.element_id == lhs.element_id;
}
bool operator!=(const tag_type& lhs, const tag_type& rhs)
{
    return !(lhs == rhs);
}

bool operator<(const tag_type& lhs, const tag_type& rhs)
{
    // Item tag should be placed first in a (nested) set
    // but prevent that index operator for map inserts
    // dupe Items
    tag_type dump{0xFFFE, 0xE000};
    //if (lhs == Item && rhs == Item) return false;
    if (lhs == dump && rhs != dump) return true;
    if (rhs == dump && lhs != dump) return false;
    return lhs.group_id == rhs.group_id ?
               lhs.element_id < rhs.element_id :
               lhs.group_id < rhs.group_id;
}

std::size_t byte_length(tag_type)
{
    return 4;
}


/**
 * @brief encode_tag_little_endian converts the element tag into a little endian
 *        representation of 8 bytes
 * @param tag tag to be encoded
 * @return vector of bytes representing the tag in little endian
 */
std::vector<unsigned char> encode_tag_little_endian(tag_type tag)
{
    std::vector<unsigned char> data;
    auto group_le = convhelper::integral_to_little_endian(tag.group_id, 2);
    auto elem_le = convhelper::integral_to_little_endian(tag.element_id, 2);
    data.insert(data.begin(), elem_le.begin(), elem_le.end());
    data.insert(data.begin(), group_le.begin(), group_le.end());
    return data;
}

/**
 * @brief encode_tag_big_endian converts the element tag into a big endian
 *        representation of 8 bytes
 * @param tag tag to be encoded
 * @return vector of bytes representing the tag in big endian
 */
std::vector<unsigned char> encode_tag_big_endian(tag_type tag)
{
    std::vector<unsigned char> data;
    auto group_le = convhelper::integral_to_big_endian(tag.group_id, 2);
    auto elem_le = convhelper::integral_to_big_endian(tag.element_id, 2);
    data.insert(data.begin(), elem_le.begin(), elem_le.end());
    data.insert(data.begin(), group_le.begin(), group_le.end());
    return data;
}

/**
 * @brief encode_tag converts the element tag into serialized representation of
 *        8 bytes
 * @param tag tag to be encoded
 * @param endianness endianness of the encoded stream
 * @return vector of bytes representing the tag
 */
std::vector<unsigned char> encode_tag(tag_type tag, ENDIANNESS endianness)
{
    if (endianness == ENDIANNESS::LITTLE) {
        return encode_tag_little_endian(tag);
    } else {
        return encode_tag_big_endian(tag);
    }
}

static attribute::vmtype<tag_type> decode_tags(const std::vector<unsigned char>& strdata, std::string vm, ENDIANNESS endianness, std::size_t begin, std::size_t len)
{
    const std::size_t tag_length = 4;
    std::vector<tag_type> tags;
    tags.reserve(len / tag_length);
    for (std::size_t i=0; i<len; i+=tag_length) {
        tags.emplace_back(decode_tag(strdata, begin+i, endianness));
    }
    return attribute::vmtype<tag_type>(vm, tags.begin(), tags.end());
}

}

}

}

#endif // TAG_TYPE_HPP
