#include "encapsulated.hpp"

namespace dicom
{

namespace data
{

namespace attribute
{

// std::size_t byte_length(const ob_type& data)
// {
//    byte_size sz;
//    return boost::apply_visitor(sz, data);
// }


// std::size_t byte_size::operator()(const encapsulated&) const
// {
//    return 0xffffffff;
// }


// std::size_t byte_size::operator()(const std::vector<unsigned char>& image_data) const
// {
//    return byte_length(image_data);
// }




// std::ostream& printer::operator()(const encapsulated& encapsulated_data) const
// {
//    if (encapsulated_data.have_compressed_frame_info()) {
//       os << "encapsulated data (with compressed frame info)\n";
//    } else {
//       os << "encapsulated data (no compressed frame info)\n";
//    }
//    os << encapsulated_data.fragment_count() << " fragments\n";

//    for (std::size_t i = 0; i < encapsulated_data.fragment_count(); ++i) {
//       const auto& fragment = encapsulated_data.get_fragment(i);
//       os << "fragment " << i << ": " << fragment.size() << " bytes";
//       if (encapsulated_data.have_compressed_frame_info() &&
//           encapsulated_data.marks_frame_start(i)) {
//          os << ", marks start of a compressed frame\n";
//       } else {
//          os << "\n";
//       }
//    }
//    return os;
// }

// std::ostream& printer::operator()(const std::vector<unsigned char>& data) const
// {
//    return os << data;
// }

// std::ostream& operator<<(std::ostream& os, const ob_type& data)
// {
//    printer pr{os};
//    return boost::apply_visitor(pr, data);
// }

}

}

}
