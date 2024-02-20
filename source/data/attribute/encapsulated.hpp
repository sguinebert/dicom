#ifndef ENCAPSULATED_HPP
#define ENCAPSULATED_HPP

#include <vector>
#include <ostream>

#include "boost/variant.hpp"

#include "base_types.hpp"


namespace dicom
{

namespace data
{

namespace attribute
{


/**
 * @brief The encapsulated class manages encapsulated data consisting of
 *        fragments and possibly information about compressed frames
 */
class encapsulated
{
   public:
      enum class OFFSET_TABLE_INFO
      {
         COMPRESSED_FRAMES,
         FRAGMENTS
      };

   private:
      OFFSET_TABLE_INFO offset_table;
      std::vector<std::vector<unsigned char>> fragments;
      std::vector<std::size_t> compressed_frame_indices;

      void throw_if_no_compressed_frame_info() const
      {
          if (!have_compressed_frame_info()) {
              throw std::runtime_error("Encapsulated data object has no frame information!");
          }
      }

   public:
      encapsulated(OFFSET_TABLE_INFO offset_table = OFFSET_TABLE_INFO::FRAGMENTS):
          offset_table {offset_table}
      {
      }

      /**
       * @brief have_compressed_frame_info checks whether there is information
       *        of compressed frames
       * @return true if compressed frame info exists, false otherwise
       */
      bool have_compressed_frame_info() const
      {
          return offset_table == OFFSET_TABLE_INFO::COMPRESSED_FRAMES;
      }

      /**
       * @brief fragment_count returns the number of fragments
       * @return number of fragments
       */
      std::size_t fragment_count() const
      {
          return fragments.size();
      }

      /**
       * @brief push_fragment adds another fragment to the encapsulated data
       * @param data byte array of fragment data
       */
      void push_fragment(std::vector<unsigned char> data)
      {
          fragments.push_back(data);
      }

      /**
       * @brief get_fragment retrieves the fragment at index
       * @param index fragment index to retrieve
       * @return raw fragment byte data
       */
      std::vector<unsigned char> get_fragment(std::size_t index)
      {
          return fragments[index];
      }

      /**
       * @brief get_fragment retrieves the fragment at index
       * @param index fragment index to retrieve
       * @return raw fragment byte data
       */
      const std::vector<unsigned char>& get_fragment(std::size_t index) const
      {
          return fragments[index];
      }

      /**
       * @brief marks_frame_start checks whether the fragment at index is the
       *        beginning of a compressed frame
       * @param index fragment index
       * @return true if fragment is start of a frame, false otherwise
       */
      bool marks_frame_start(std::size_t index) const
      {
          throw_if_no_compressed_frame_info();
          return std::find(compressed_frame_indices.begin(), compressed_frame_indices.end(), index)
                 != compressed_frame_indices.end();
      }


      /**
       * @brief mark_compressed_frame_start marks the beginning of a compressed
       *        frame consisting of one or more fragments
       */
      void mark_compressed_frame_start()
      {
          throw_if_no_compressed_frame_info();
          compressed_frame_indices.push_back(fragments.size());
      }

};


using ob_type = boost::variant<std::vector<unsigned char>, encapsulated>;


class byte_size : public boost::static_visitor<std::size_t>
{
   public:
       std::size_t operator()(const encapsulated& /*encapsulated_data*/) const
       {
           return 0xffffffff;
       }
       std::size_t operator()(const std::vector<unsigned char>& data) const
       {
           return byte_length(data);
       }
};

std::size_t byte_length(const ob_type& data)
{
    byte_size sz;
    return boost::apply_visitor(sz, data);
}

class printer : public boost::static_visitor<std::ostream&>
{
    private:
        std::ostream& os;

   public:
        printer(std::ostream& os):
            os {os}
        {

        }

        std::ostream& operator()(const encapsulated& encapsulated_data) const
        {
            if (encapsulated_data.have_compressed_frame_info()) {
                os << "encapsulated data (with compressed frame info)\n";
            } else {
                os << "encapsulated data (no compressed frame info)\n";
            }
            os << encapsulated_data.fragment_count() << " fragments\n";

            for (std::size_t i = 0; i < encapsulated_data.fragment_count(); ++i) {
                const auto& fragment = encapsulated_data.get_fragment(i);
                os << "fragment " << i << ": " << fragment.size() << " bytes";
                if (encapsulated_data.have_compressed_frame_info() &&
                    encapsulated_data.marks_frame_start(i)) {
                    os << ", marks start of a compressed frame\n";
                } else {
                    os << "\n";
                }
            }
            return os;
        }
        std::ostream& operator()(const std::vector<unsigned char>& data) const
        {
            return os << data;
        }
};

std::ostream& operator<<(std::ostream& os, const ob_type& data)
{
    printer pr{os};
    return boost::apply_visitor(pr, data);
}

}

}

}

#endif // ENCAPSULATED_HPP
