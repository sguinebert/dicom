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

}

}

}

#endif // TAG_TYPE_HPP
