#include "datasets.hpp"

#include <iostream>
#include <iomanip>

#include "dataset_iterator.hpp"

namespace dicom
{

namespace data
{

namespace dataset
{


using namespace attribute;

std::ostream& operator<<(std::ostream& os, elementfield::tag_type tag)
{
   std::ios state(nullptr);
   state.copyfmt(os);

   os << "(" << std::hex << std::setw(4) << std::setfill('0') << tag.group_id
      << "," << std::hex << std::setw(4) << std::setfill('0') << tag.element_id
      << ")";

   os.copyfmt(state);
   return os;
}

std::ostream& operator<<(std::ostream& os, const dataset_type& data)
{

   int depth = 0;
   for (const auto attr : dataset_iterator_adaptor(data)) {
      if (attr.first == elementfield::tag_type {0xfffe, 0xe0dd}) {
         depth--;
      }
      if (attr.second.value_rep == VR::SQ) {
         depth++;
      }

      std::fill_n(std::ostream_iterator<char>(os), depth, '\t');
      os << attr.first << "\n";
   }

   return os;
}

std::size_t dataset_size(dataset_type data, bool explicitvr)
{
   return std::accumulate(data.begin(), data.end(), 0,
      [explicitvr](int acc, const std::pair<const elementfield::tag_type, elementfield>& attr) {
      return acc += attr.second.value_len + 4 + 4
            + (explicitvr ? 2 : 0);
   });
}

}

}

}
