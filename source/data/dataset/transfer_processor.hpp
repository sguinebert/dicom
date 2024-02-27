#ifndef TRANSFER_PROCESSOR_HPP
#define TRANSFER_PROCESSOR_HPP

#include <vector>
#include <string>
#include <stack>
#include <numeric>
#include <vector>
#include <string>
#include <algorithm>

#include <boost/log/trivial.hpp>

#include "data/attribute/attribute_field_coder.hpp"
#include "data/attribute/constants.hpp"
#include "data/dictionary/dictionary_entry.hpp" // vr of string
#include "dataset_iterator.hpp"



#include "datasets.hpp"
#include "dataset_iterator.hpp"
#include "data/dictionary/dictionary.hpp"
#include "data/dictionary/datadictionary.hpp"
#include "data/dictionary/dictionary_dyn.hpp"
#include "data/attribute/attribute.hpp"
#include "util/channel_sev_logger.hpp"

namespace dicom
{

namespace data
{

/**
 * The dataset namespace contains all functionality related to sets of
 * attributes comprising a DICOM dataset (iod or command header data).
 */
namespace dataset
{
using namespace boost::log::trivial;
using namespace attribute;


static const std::vector<VR> specialVRs {VR::OB, VR::OW, VR::OF, VR::SQ, VR::UR, VR::UT, VR::UN, VR::NI, VR::NN};

static const std::vector<tag_type> item_attributes {Item, ItemDelimitationItem, SequenceDelimitationItem};

class transfer_processor;

std::vector<std::string> supported_transfer_syntaxes()
{
    return std::vector<std::string>
        {
            "1.2.840.10008.1.2",
            "1.2.840.10008.1.2.1",
            "1.2.840.10008.1.2.2",
            "1.2.840.10008.1.2.4.70",
            "1.2.840.10008.1.2.4.50",
            "1.2.840.10008.1.2.4.57"
        };
}





/**
 * @brief is_specialVR checks if the given VR needs special handling in explicit
 *        transfer syntaxes
 * @param vr VR to be checked
 * @return true if VR is "special", false otherwise
 * @see DICOM standard part 3.5, chapter 7.1.2
 */
static bool is_special_VR(VR vr)
{
    return std::find(specialVRs.begin(), specialVRs.end(), vr)
           != specialVRs.end();
}


/**
 * @brief is_item_attribute checks if the passed tag is of an item type, ie.
 *        Item, ItemDelimitationItem or SequenceDelimitationItem
 * @param tag tag to be checked
 * @return true if tag corresponds to an "item type", false otherwise
 */
static bool is_item_attribute(tag_type tag)
{
    return std::find(item_attributes.begin(), item_attributes.end(), tag)
           != item_attributes.end();
}



/**
 * @brief The transfer_processor class defines the interface for serializing
 *        and deserializing attribute sets (IODs), honoring the transfer
 *        syntax
 * This abstraction shall be mapped by the DIMSE layer to each transfer syntax
 * it wants to offer for each presentation context.
 */
class transfer_processor
{
   protected:
      /**
       * @brief The VR_TYPE enum defines if an explicit VR is used in the
       *        transfer syntax.
       */
      enum class VR_TYPE
      {
         IMPLICIT, EXPLICIT
      };

      /**
       * @brief The vr_of_tag struct defines a transfer-syntax specific mapping
       *        of a tag (range) to a VR.
       */
      struct vr_of_tag
      {
            const attribute::tag_type tag;
            const unsigned eid_mask;
            const unsigned gid_mask;

            const attribute::VR vr;

            vr_of_tag(attribute::tag_type tag,
                      attribute::VR vr,
                      unsigned eid_mask = 0xffff,
                      unsigned gid_mask = 0xffff):
                tag {tag},
                eid_mask {eid_mask},
                gid_mask {gid_mask},
                vr {vr}
            {
            }
      };

      /**
       * @brief transfer_processor constructor
       * @param dict optional dictionary, only necessary for implicit VR TSs
       * @param tfs transfer syntax UID
       * @param vrtype enum instance defining if the ts is [ex|im]plicit VR.
       * @param tstags list of transfer-syntax-specific tag-to-VR-mapping
       */
      transfer_processor(boost::optional<dictionary::dictionaries&> dict,
                         std::string tfs,
                         VR_TYPE vrtype,
                         attribute::ENDIANNESS endianness,
                         std::initializer_list<vr_of_tag> tstags = {})
          :
          tstags {tstags},
          dict(dict),
          transfer_syntax {tfs},
          vrtype {vrtype},
          endianness {endianness},
          logger {"transfer processor"}
      {
          if (vrtype == VR_TYPE::IMPLICIT && !dict.is_initialized()) {
              throw std::runtime_error("Uninitialized dictionary with "
                                       "implicit VR type specificed!");
          }
      }

      transfer_processor(const transfer_processor& other):
          dict {other.dict},
          transfer_syntax {other.transfer_syntax},
          vrtype {other.vrtype},
          logger {"transfer processor"}
      {
      }

      /**
       * @brief get_vr returns the VR of a given tag, honoring transfer-syntax
       *        specific settings specified by a subclass.
       * @param tag tag of the VR to be looked for
       * @return VR associated with the tag
       */
      attribute::VR get_vr(attribute::tag_type tag) const
      {
          auto spectag = std::find_if(tstags.begin(), tstags.end(),
                                      [tag](vr_of_tag vrt) {
                                          return (tag.group_id & vrt.gid_mask) == vrt.tag.group_id
                                                 && (tag.element_id & vrt.eid_mask) == vrt.tag.element_id;
                                      });
          if (spectag == tstags.end()) {
              return dict.get().lookup(tag).vr[0];
          } else {
              return spectag->vr;
          }
      }

      dicom::data::dictionary::dictionaries& get_dictionary() const
      {
          return *dict;
      }

   public:

      /**
       * @brief serialize is used to serialize an iod / command set
       * @param data structured dataset to be serialized
       * @return bytestream of serialized data
       */
      std::vector<unsigned char> serialize(iod dataset) const
       {
           calculate_item_lengths(dataset);

           std::vector<unsigned char> stream;
           bool explicit_length_item = true;
           bool explicit_length_sequence = true;
           for (auto attr : dataset_iterator_adaptor(dataset)) {
               if (attr.first == SequenceDelimitationItem) {
                   if (!explicit_length_sequence) {
                       auto tag = convhelper::encode_tag(attr.first, endianness);
                       auto len = encode_len(4, attr.second.value_len, endianness);
                       stream.insert(stream.end(), tag.begin(), tag.end());
                       stream.insert(stream.end(), len.begin(), len.end());
                   }
                   continue;
               } else if (attr.first == ItemDelimitationItem) {
                   if (!explicit_length_item)  {
                       auto tag = convhelper::encode_tag(attr.first, endianness);
                       auto len = encode_len(4, attr.second.value_len, endianness);
                       stream.insert(stream.end(), tag.begin(), tag.end());
                       stream.insert(stream.end(), len.begin(), len.end());
                   }
                   continue;
               } else if (attr.first == Item) {
                   auto tag = convhelper::encode_tag(attr.first, endianness);
                   auto len = encode_len(4, attr.second.value_len, endianness);
                   stream.insert(stream.end(), tag.begin(), tag.end());
                   stream.insert(stream.end(), len.begin(), len.end());
                   explicit_length_item = (attr.second.value_len != 0xffffffff);
                   continue;
               }

               VR repr;
               if (vrtype == VR_TYPE::EXPLICIT || attr.second.value_rep.is_initialized()) {
                   repr = attr.second.value_rep.get();
               } else {
                   repr = get_vr(attr.first);
               }

               auto& value_field = attr.second;
               auto data = serialize_attribute(value_field, endianness, repr);
               std::size_t value_length = value_field.value_len;
               if (data.size() != value_field.value_len
                   && value_field.value_len != 0xffffffff) {

                   if (value_field.value_rep.is_initialized() &&
                       *value_field.value_rep != VR::SQ &&
                       *value_field.value_rep != VR::NN &&
                       *value_field.value_rep != VR::NI) {
                       BOOST_LOG_SEV(logger, warning) << "Mismatched value lengths for tag "
                                                      << attr.first << ": Expected "
                                                      << value_field.value_len << ", actual "
                                                      << data.size();
                       value_length = data.size();
                   }
               }

               auto tag = convhelper::encode_tag(attr.first, endianness);
               std::vector<unsigned char> len;
               stream.insert(stream.end(), tag.begin(), tag.end());
               if (vrtype == VR_TYPE::EXPLICIT) {
                   auto vr = dictionary::dictionary_entry::vr_of_string.right.at(repr);
                   stream.insert(stream.end(), vr.begin(), vr.begin()+2);
                   if (is_special_VR(repr)) {
                       stream.push_back(0x00); stream.push_back(0x00);
                       len = encode_len(4, value_length, endianness);
                   } else {
                       len = encode_len(2, value_length, endianness);
                   }
               } else {
                   len = encode_len(4, value_length, endianness);
               }

               if (repr == VR::SQ)
               {
                   explicit_length_sequence = (attr.second.value_len != 0xffffffff);
               }

               stream.insert(stream.end(), len.begin(), len.end());
               stream.insert(stream.end(), data.begin(), data.end());
           }
           return stream;
       }

      /**
       * @brief deserialize shall be called to deserialize a datastream into
       *        a structured attribute
       * @param data datastream
       * @return structured attribute
       */
      dataset_type deserialize(std::vector<unsigned char> data) const
      {
          dataset_type dataset;
          std::vector<dataset_type> outerset {dataset};

          // current_data defines the serialized nested set which is processed at
          // a time as half-open interval [start,start+nested_set_size).
          std::stack<std::pair<std::size_t, std::size_t>> current_data;

          std::stack<std::vector<dataset_type>> current_sequence;
          std::stack<std::size_t> positions;
          std::stack<std::pair<tag_type, std::size_t>> lasttag;

          positions.push(0);
          current_sequence.push(outerset);
          current_data.push({0, data.size()});
          while (current_data.size() > 0) {

              bool undefined_length_item = false;
              bool undefined_length_sequence = false;
              // if we are coming right out of the deserialization of a nested sequence
              // of items, add it to the last item deserialized before the nested sq.
              if (lasttag.size() > 0) {
                  assert(current_sequence.size() > 1);
                  auto tag = lasttag.top().first;
                  auto size = lasttag.top().second;
                  lasttag.pop();
                  auto nested_seq = std::move(current_sequence.top());
                  current_sequence.pop();
                  current_sequence.top().back().emplace(tag, std::move(make_elementfield<VR::SQ>(size, std::move(nested_seq))));
                  positions.pop();

              }

              while (positions.top() < current_data.top().second) {
                  auto& pos = positions.top();
                  assert(!current_sequence.top().empty());

                  tag_type tag = convhelper::decode_tag(data, pos, endianness);
                  pos += 4;

                  VR repr = deserialize_VR(data, tag, pos);
                  std::size_t value_len = deserialize_length(data, tag, repr, pos);


                  // Items and DelimitationItems do not have a VR or length field and are
                  // to be treated separately.
                  if (!is_item_attribute(tag)) {

                      // if the current item is a sequence, push the nested serialized
                      // data on the stack so it will be processed next. Store the current
                      // state of the deserialization on appropriate stacks.
                      if (repr == VR::SQ) {
                          undefined_length_sequence = ((value_len & 0xffffffff) == 0xffffffff);
                          value_len = ((value_len & 0xffffffff) == 0xffffffff)
                                          ? find_enclosing(data, pos)
                                          : value_len;
                          current_data.push({pos, pos+value_len});
                          current_sequence.push({dataset_type {}});
                          positions.push(current_data.top().first);
                          lasttag.push({tag, undefined_length_sequence ? 0xffffffff : value_len});
                      } else {
                          //auto multiplicity = get_dictionary().lookup(tag).vm;
                          current_sequence.top().back().emplace(tag, std::move(deserialize_attribute(data, endianness, value_len, repr, "*", pos)));

                          if (repr == VR::OB && value_len == 0xffffffff) {
                              value_len = current_sequence.top().back()[tag].value_len;
                          }
                      }

                      pos += value_len;

                  } else {
                      // Put Items and respective DelimitationItems into the deserialized
                      // set.
                      if (tag == Item) {
                          // detect if an ItemDelimitationItem was missed
                          if (positions.top()-current_data.top().first > 8) {
                              current_sequence.top().back()[ItemDelimitationItem]
                                  = make_elementfield<VR::NN>();
                              current_sequence.top().push_back(dataset_type {});
                          }
                          undefined_length_item = ((value_len & 0xffffffff) == 0xffffffff);
                          if (!undefined_length_item) {
                              current_sequence.top().back()[tag]
                                  = make_elementfield<VR::NI>(value_len, VR::NI);
                          } else {
                              current_sequence.top().back()[tag]
                                  = make_elementfield<VR::NI>(0xffffffff, VR::NI);
                          }

                      } else if (tag == ItemDelimitationItem) {
                          // ItemDelimitationItem for this item is set when the next Item
                          // tag is encountered to handle possibly missing
                          // ItemDelimitationItems
                      } else if (tag == SequenceDelimitationItem) {
                          // sequence delimitation is in a separate, the last, vector
                          if (current_sequence.top().size() > 0) {
                              current_sequence.top().push_back(dataset_type {});
                          }
                          current_sequence.top().back()[tag] = make_elementfield<VR::NN>();
                      }
                  }
              }

              // add item and sequence delimination on the last item of a nested
              // sequence, leaving the outermost without.
              if (current_sequence.size() > 1) {
                  auto& itemset = current_sequence.top().back();
                  auto& prevset = *(current_sequence.top().end()-2);
                  if (current_sequence.top().size() < 2) {
                      current_sequence.top().back()[ItemDelimitationItem] = make_elementfield<VR::NN>();
                  } else {
                      bool have_item_delimitation = false;
                      if (prevset.find(Item) != prevset.end() && prevset.find(ItemDelimitationItem) == prevset.end()) {
                          prevset[ItemDelimitationItem] = make_elementfield<VR::NN>();
                          have_item_delimitation = true;
                      }
                      if (!have_item_delimitation && itemset.find(ItemDelimitationItem) == itemset.end() &&
                          !(itemset.size() == 1 && itemset.find(SequenceDelimitationItem) != itemset.end())) {
                          itemset[ItemDelimitationItem] = make_elementfield<VR::NN>();
                          have_item_delimitation = true;
                      }
                  }
                  current_sequence.top().back()[SequenceDelimitationItem] = make_elementfield<VR::NN>();
              }
              current_data.pop();
          }

          // The size of the outermost set should be exactly one
          assert(current_sequence.top().size() == 1);
          return std::move(current_sequence.top()[0]); // current_sequence not used after this, move necessary
      }

      /**
       * @brief get_transfer_syntax is used to acquire the UID of the classe's
       *        implemented transfer syntax.
       * @return UID of the transfer syntax
       */
      std::string get_transfer_syntax() const
      {
          return transfer_syntax;
      }

      std::size_t dataelement_length(const std::pair<attribute::tag_type, const attribute::elementfield&>& attribute) const
      {
          const elementfield& ef = attribute.second;
          if (vrtype == VR_TYPE::IMPLICIT) {
              return ef.value_len + 4 + 2 + 2;
          } else {
              //std::size_t length = ef.value_len + 2 + 2;
              //std::size_t length = ef.value_field->byte_size();
              std::size_t length = ef.value_len;
              if (ef.value_field->byte_size() != ef.value_len
                  && ef.value_len != 0xffffffff) {

                  if (ef.value_rep.is_initialized() &&
                      *ef.value_rep != VR::SQ &&
                      *ef.value_rep != VR::NN &&
                      *ef.value_rep != VR::NI) {
                      length = ef.value_field->byte_size();
                      BOOST_LOG_SEV(logger, warning) << "Mismatched value lengths for tag " << ef.value_len
                                                     << attribute.first << ": Expected "
                                                     << ef.value_len << ", actual "
                                                     << length;
                  }
              }
              length += 2 + 2;
              if (is_special_VR(*ef.value_rep)) {
                  length += 2 + 2 + 4;
              } else {
                  length += 2 + 2;
              }
              return length;
          }
      }

      virtual ~transfer_processor() {}

   protected:
      /**
       * @brief serialize_attribute is overriden by a subclass to implement
       *        transfer-syntax specific serialization of data.
       * @param e attribute
       * @param vr VR of the attribute
       * @return vector of bytes with serialized data
       */
      virtual std::vector<unsigned char>
      serialize_attribute(attribute::elementfield& e, attribute::ENDIANNESS end, attribute::VR vr) const = 0;

      /**
       * @brief deserialize_attribute is overriden by a subclass to implement
       *        transfer-syntax specific deserialization of data.
       * @param data array containing serialized bytes
       * @param len length of the attribute's data field
       * @param vr VR of the attribute
       * @param pos starting position of the data field in the serialized array
       * @return elementfield with structured data
       */
      virtual attribute::elementfield
      deserialize_attribute(std::vector<unsigned char>& data,
                            attribute::ENDIANNESS end,
                            std::size_t len, attribute::VR vr, std::string vm,
                            std::size_t pos) const = 0;

      /**
       * @brief deserialize_VR deserializes and returns the VR, which may be
       *        explicit or implicit.
       * @param dataset serialized data
       * @param tag tag of the current attribute
       * @param[in, out] pos
       * @return deserialized VR
       */
      attribute::VR deserialize_VR(const std::vector<unsigned char>& dataset,
                                   attribute::tag_type tag,
                                   std::size_t& pos)  const;

      /**
       * @brief deserialize_length deserializes and returns the length of the
       *        value field of the current attribute
       * @param dataset serialized data
       * @param tag tag of the current attribute
       * @param[in, out] pos
       * @return length of the value field
       */
      std::size_t deserialize_length(const std::vector<unsigned char>& dataset,
                                     attribute::tag_type tag,
                                     attribute::VR repr,
                                     std::size_t& pos) const
      {
          const std::size_t lengthfield_size
              = (vrtype == VR_TYPE::IMPLICIT || is_special_VR(repr))
                    ? 4 : 2;

          std::size_t value_len;
          if (!is_item_attribute(tag)) {
              value_len = decode_len(dataset, endianness, lengthfield_size, pos);
          } else if (tag == Item) {
              value_len = decode_len(dataset, endianness, 4, pos);
          } else {
              value_len = 0;
          }
          pos += lengthfield_size;

          return value_len;
      }

      /**
       * @brief find_enclosing is used to determine the size of the nested set if it
       *        is not explicitly specified.
       * @todo convert this recursive function into an iterative one
       * @param data serialized dataset
       * @param explicitvr true if vr is explicit, false otherwise
       * @param endianness endianness of the data stream
       * @param beg first element
       * @param dict dictionary for the tag lookup
       * @return size of the nested set
       */
      std::size_t find_enclosing(const std::vector<unsigned char>& data, std::size_t beg) const
      {
          std::size_t pos = beg;
          int nested_sets = 0;

          std::stack<std::size_t> beginnings;
          beginnings.push(pos);

          while (!beginnings.empty() && pos < data.size()) {
              //pos = beginnings.top();
              tag_type tag = convhelper::decode_tag(data, pos, endianness);
              if (tag == SequenceDelimitationItem || tag == ItemDelimitationItem) {
                  pos += 8;
                  beginnings.pop();
                  //         std::size_t oldpos = beginnings.top();
                  //         beginnings.top() += pos-oldpos;
                  continue;
              }
              pos += 4;

              VR repr = deserialize_VR(data, tag, pos);
              std::size_t value_len = deserialize_length(data, tag, repr, pos);

              if (repr == VR::SQ) {
                  nested_sets++;
              }

              if ((value_len & 0xffffffff) == 0xffffffff) {
                  beginnings.push(pos);
              } else {
                  pos += value_len;
              }

          }
          return pos-beg;
      }

      /**
       * @brief calculate_item_lengths calculates the correct sequence and item lengths
       *        given the specified transfer syntax.
       * @param dataset dataset which will receive updated length values
       * @return byte length of the set given in the parameter minus any item and / or
       *         sequence sizes which are undefined length.
       */
      std::size_t calculate_item_lengths(data::dataset::dataset_type& dataset) const
      {
          std::size_t accu = 0;

          // skip undefined length items. The accumulator for this sequence will stay at 0.
          // This is ok since it is not used for the sequence length, since when one item
          // length is undefined, the sequence length is necessarily undefined as well.
          if (dataset.count(Item) > 0 && dataset[Item].value_len == 0xffffffff) {
              return accu;
          }

          // item only contains "item type" tags, so the length is essentially zero.
          if (dataset.size() == 3 &&
              std::all_of(dataset.begin(), dataset.end(),
                          [](std::pair<const tag_type, elementfield> attr)
                          { return is_item_attribute(attr.first); })) {
              return 0;
          }


          for (auto& attr : dataset) {
              VR repr;
              if (vrtype == VR_TYPE::EXPLICIT) {
                  repr = attr.second.value_rep.get();
              } else {
                  repr = get_vr(attr.first);
              }

              if (repr != VR::SQ && !is_item_attribute(attr.first)) {
                  accu += dataelement_length(attr);
              } else if (repr == VR::SQ) {
                  // now handle all sequence items
                  // sequences with undefined length need to be checked as well
                  // because they may contain items with defined length
                  typename type_of<VR::SQ>::type data;
                  get_value_field<VR::SQ>(attr.second, data);
                  std::size_t sequencesize = 0;
                  for (dataset_type& itemset : data) {
                      auto itemsize = calculate_item_lengths(itemset);
                      if (itemsize > 0) {
                          itemset[Item].value_len = itemsize;
                          sequencesize += itemsize + 8;
                      } else {
                          // subset only contains delimitation items. When we write with
                          // explicit lengths, this amounts to a sequence of size zero.
                          if (std::all_of(itemset.begin(), itemset.end(),
                                          [](std::pair<const tag_type, elementfield> attr) {
                                              return attr.first == ItemDelimitationItem
                                                     || attr.first == SequenceDelimitationItem; }))
                          {
                              sequencesize += 0;
                          } else {
                              sequencesize += 8;
                          }
                      }
                  }

                  if (attr.second.value_len != 0xffffffff) {
                      attr.second.value_len = sequencesize;
                      set_value_field<VR::SQ>(attr.second, data);
                      accu += dataelement_length(attr); // update the sequence length
                  }
              }
          }
          return accu;
      }

      std::vector<vr_of_tag> tstags;

      boost::optional<dictionary::dictionaries&> dict;
      const std::string transfer_syntax;
      VR_TYPE vrtype;
      attribute::ENDIANNESS endianness;

   protected:
      mutable dicom::util::log::channel_sev_logger logger;
};

/**
 * @brief The commandset_processor class is used to (de)serialize the command
 *        set of the DICOM message, which is always encoded in little endian
 *        with implicit vr.
 */
class commandset_processor: public transfer_processor
{
   public:
      explicit commandset_processor(dictionary::dictionaries& dict)
           :
           transfer_processor {boost::optional<dictionary::dictionaries&> {dict}, "", VR_TYPE::IMPLICIT, ENDIANNESS::LITTLE}
       {
       }

   private:
      virtual std::vector<unsigned char>
      serialize_attribute(attribute::elementfield& e, attribute::ENDIANNESS end, attribute::VR vr) const override
       {
           return encode_value_field(e, end, vr);
       }

      virtual attribute::elementfield
      deserialize_attribute(std::vector<unsigned char>& data,
                            attribute::ENDIANNESS end,
                            std::size_t len, attribute::VR vr, std::string vm,
                            std::size_t pos) const override
       {
           return decode_value_field(data, end, len, vr, vm, pos);
       }
};

/**
 * @brief The little_endian_implicit class implements the little endian implicit
 *        transfer syntax.
 */
class little_endian_implicit: public transfer_processor
{
   public:
      explicit little_endian_implicit(dictionary::dictionaries& dict,
                                      std::string transfer_syntax = "1.2.840.10008.1.2"):
           transfer_processor {boost::optional<dictionary::dictionaries&> {dict},
                              transfer_syntax,
                              VR_TYPE::IMPLICIT,
                              ENDIANNESS::LITTLE,
                              {
                                  {{0x7fe0, 0x0010}, VR::OW},
                                  {{0x6000, 0x3000}, VR::OW, 0xff00, 0xffff},
                                  {{0x5400, 0x1010}, VR::OW}, {{0x0028, 0x1201}, VR::OW},
                                  {{0x0028, 0x1202}, VR::OW}, {{0x0028, 0x1203}, VR::OW},
                                  {{0x0028, 0x1204}, VR::OW}, {{0x0028, 0x1101}, VR::SS},
                                  {{0x0028, 0x1102}, VR::SS}, {{0x0028, 0x1103}, VR::SS},
                                  {{0x0028, 0x1221}, VR::OW}, {{0x0028, 0x1222}, VR::OW},
                                  {{0x0028, 0x1223}, VR::OW}, {{0x0028, 0x3006}, VR::US},
                                  {{0x0028, 0x3002}, VR::SS}, {{0x0028, 0x1408}, VR::OW},
                                  {{0x0066, 0x0025}, VR::OW}, {{0x0066, 0x0024}, VR::OW},
                                  {{0x0066, 0x0023}, VR::OW}, {{0x0066, 0x0029}, VR::OW}
                              } }
       {
       }

      little_endian_implicit(const little_endian_implicit& other):
           transfer_processor {other}
       {

       }

   private:
      virtual std::vector<unsigned char>
      serialize_attribute(attribute::elementfield& e, attribute::ENDIANNESS end, attribute::VR vr) const
       {
           return encode_value_field(e, end, vr);
       }

      virtual attribute::elementfield
      deserialize_attribute(std::vector<unsigned char>& data, attribute::ENDIANNESS end,
                            std::size_t len, attribute::VR vr, std::string vm,
                            std::size_t pos) const;
};

class little_endian_explicit: public transfer_processor
{
   public:
      explicit little_endian_explicit(dictionary::dictionaries& dict,
                                      std::string transfer_syntax = "1.2.840.10008.1.2.1"):
           transfer_processor {boost::optional<dictionary::dictionaries&> {dict},
                              transfer_syntax,
                              VR_TYPE::EXPLICIT, ENDIANNESS::LITTLE,
                              {
                                  {{0x7fe0, 0x0010}, VR::OW},
                                  {{0x6000, 0x3000}, VR::OW, 0xff00, 0xffff},
                                  {{0x5400, 0x1010}, VR::OW}, {{0x0028, 0x1201}, VR::OW},
                                  {{0x0028, 0x1202}, VR::OW}, {{0x0028, 0x1203}, VR::OW},
                                  {{0x0028, 0x1204}, VR::OW}, {{0x0028, 0x1101}, VR::SS},
                                  {{0x0028, 0x1102}, VR::SS}, {{0x0028, 0x1103}, VR::SS},
                                  {{0x0028, 0x1221}, VR::OW}, {{0x0028, 0x1222}, VR::OW},
                                  {{0x0028, 0x1223}, VR::OW}, {{0x0028, 0x3006}, VR::US},
                                  {{0x0028, 0x3002}, VR::SS}, {{0x0028, 0x1408}, VR::OW},
                                  {{0x0066, 0x0025}, VR::OW}, {{0x0066, 0x0024}, VR::OW},
                                  {{0x0066, 0x0023}, VR::OW}, {{0x0066, 0x0029}, VR::OW}
                              } }
       {

       }

   private:
      virtual std::vector<unsigned char>
      serialize_attribute(attribute::elementfield& e, attribute::ENDIANNESS end, attribute::VR vr) const;

      virtual attribute::elementfield
      deserialize_attribute(std::vector<unsigned char>& data,
                            attribute::ENDIANNESS end,
                            std::size_t len, attribute::VR vr, std::string vm,
                            std::size_t pos) const;
};

class big_endian_explicit: public transfer_processor
{
   public:
      explicit big_endian_explicit(dictionary::dictionaries& dict,
                                   std::string transfer_syntax = "1.2.840.10008.1.2.2"):
           transfer_processor {boost::optional<dictionary::dictionaries&> {dict},
                              transfer_syntax,
                              VR_TYPE::EXPLICIT, ENDIANNESS::BIG,
                              {
                                  {{0x7fe0, 0x0010}, VR::OW},
                                  {{0x6000, 0x3000}, VR::OW, 0xff00, 0xffff},
                                  {{0x5400, 0x1010}, VR::OW}, {{0x0028, 0x1201}, VR::OW},
                                  {{0x0028, 0x1202}, VR::OW}, {{0x0028, 0x1203}, VR::OW},
                                  {{0x0028, 0x1204}, VR::OW}, {{0x0028, 0x1101}, VR::SS},
                                  {{0x0028, 0x1102}, VR::SS}, {{0x0028, 0x1103}, VR::SS},
                                  {{0x0028, 0x1221}, VR::OW}, {{0x0028, 0x1222}, VR::OW},
                                  {{0x0028, 0x1223}, VR::OW}, {{0x0028, 0x3006}, VR::US},
                                  {{0x0028, 0x3002}, VR::SS}, {{0x0028, 0x1408}, VR::OW},
                                  {{0x0066, 0x0025}, VR::OW}, {{0x0066, 0x0024}, VR::OW},
                                  {{0x0066, 0x0023}, VR::OW}, {{0x0066, 0x0029}, VR::OW}
                              } }
       {

       }

   private:
      virtual std::vector<unsigned char>
      serialize_attribute(attribute::elementfield& e, attribute::ENDIANNESS end, attribute::VR vr) const;

      virtual attribute::elementfield
      deserialize_attribute(std::vector<unsigned char>& data,
                            attribute::ENDIANNESS end,
                            std::size_t len, attribute::VR vr, std::string vm,
                            std::size_t pos) const;
};


class encapsulated: public little_endian_explicit
{
   public:
      explicit encapsulated(dictionary::dictionaries& dict, std::string transfer_syntax):
           little_endian_explicit {dict, transfer_syntax}
       {

       }

   private:
      virtual std::vector<unsigned char>
      serialize_attribute(attribute::elementfield& e, attribute::ENDIANNESS end, attribute::VR vr) const
      {
          if (vr == VR::OB) {
              boost::variant<std::vector<unsigned char>, attribute::encapsulated> data;
              get_value_field<VR::OB>(e, data);
              if (data.type() == typeid(attribute::encapsulated)) {
                  e.value_len = 0xffffffff;
                  auto encapsulated = boost::get<attribute::encapsulated>(data);
                  return serialize_fragments(encapsulated);
              } else {
                  return encode_value_field(e, end, vr);
              }
          } else {
              return encode_value_field(e, end, vr);
          }
      }

      virtual attribute::elementfield
      deserialize_attribute(std::vector<unsigned char>& data,
                            attribute::ENDIANNESS end,
                            std::size_t len, attribute::VR vr, std::string vm,
                            std::size_t pos) const
      {
          if (vr == VR::OB && len == 0xffffffff) {
              //::encapsulated encapsulated_data;
              std::size_t fragment_size = 0;
              auto encapsulated_data = deserialize_fragments(data, pos, fragment_size);
              typename type_of<VR::OB>::type ob_data {encapsulated_data};
              return make_elementfield<VR::OB>(fragment_size, ob_data);
          } else {
              return decode_value_field(data, end, len, vr, vm, pos);
          }
      }

      attribute::encapsulated deserialize_fragments(std::vector<unsigned char>& data, std::size_t pos, std::size_t& outsize) const
      {
          attribute::encapsulated encapsulated_data;
          tag_type tag = convhelper::decode_tag(data, pos, ENDIANNESS::LITTLE);
          std::size_t beg = pos;

          pos += 4;
          if (tag == Item) {
              // read basic offset table
              auto length = deserialize_length(data, tag, VR::NI, pos);
              if (length == 0) {
                  // implicit length
                  //pos += 4; // the length zero
                  while (tag != SequenceDelimitationItem) {
                      pos += 4; // skip item tag
                      // shall be Item
                      auto item_length = deserialize_length(data, tag, VR::NI, pos);
                      //pos += 4; // skip length
                      encapsulated_data.push_fragment(std::vector<unsigned char>(data.begin()+pos, data.begin()+pos+item_length));
                      pos += item_length;

                      // read the next tag
                      tag = convhelper::decode_tag(data, pos, endianness);
                  }
                  pos += 4;
                  pos += 4;
              } else {
                  // explicit length
                  attribute::encapsulated encapsulated_data(attribute::encapsulated::OFFSET_TABLE_INFO::COMPRESSED_FRAMES);

                  std::size_t offset_table_length = length + 4 + 4;

                  std::size_t next = 0;
                  for (std::size_t i=0; i<length; i += 4) {
                      auto offset = deserialize_length(data, tag, VR::NI, pos);

                      encapsulated_data.mark_compressed_frame_start();

                      std::size_t itempos = beg + offset + offset_table_length;
                      if (i < length-4) {
                          std::size_t nextpos = pos; // pos was already increment by the last call to deserialize_length
                          next = beg + deserialize_length(data, tag, VR::NI, nextpos) + offset_table_length;
                      }
                      while (itempos < next || i == length-4) {
                          tag = convhelper::decode_tag(data, itempos, endianness);
                          if (tag == SequenceDelimitationItem) {
                              pos = itempos;
                              break;
                          }
                          itempos += 4;
                          auto item_length = deserialize_length(data, tag, VR::NI, itempos);

                          encapsulated_data.push_fragment(std::vector<unsigned char>(data.begin()+itempos, data.begin()+itempos+item_length));
                          itempos += item_length;

                          if (i == length-4 && tag != Item) {
                              pos = itempos;
                              break;
                          }
                      }
                  }

                  // attempt reading sequence delimitation item
                  tag = convhelper::decode_tag(data, pos, endianness);
                  pos += 4;
                  if (tag != SequenceDelimitationItem) {
                      // print warning or error
                  }
                  deserialize_length(data, tag, VR::NI, pos);

                  //pos += 4;
                  outsize = pos - beg;
                  assert(outsize > 0);
                  return encapsulated_data;
              }
          }

          outsize = pos - beg;
          assert(outsize > 0);
          return encapsulated_data;
      }

      std::vector<unsigned char> serialize_fragments(attribute::encapsulated data) const
      {
          std::vector<unsigned char> encapsulated_data;
          if (data.have_compressed_frame_info()) {
              // write basic offset table
              std::vector<unsigned char> offset_table;
              std::size_t accu = 0;
              for (std::size_t i=0; i<data.fragment_count(); ++i) {
                  if (data.marks_frame_start(i)) {
                      auto frag_len = encode_len(4, accu, endianness);
                      offset_table.insert(offset_table.end(), std::begin(frag_len), std::end(frag_len));
                  }
                  accu += 4; // Item tag
                  accu += 4; // Length field
                  const auto& fragment = data.get_fragment(i);
                  accu += fragment.size();
              }

              auto offset_table_entries = encode_len(4, offset_table.size(), endianness);
              auto item_tag = convhelper::encode_tag(Item, endianness);
              encapsulated_data.insert(encapsulated_data.end(), std::begin(item_tag), std::end(item_tag));
              encapsulated_data.insert(encapsulated_data.end(), std::begin(offset_table_entries), std::end(offset_table_entries));

              std::copy(std::begin(offset_table), std::end(offset_table), std::back_inserter(encapsulated_data));

              // now the actual values
              for (std::size_t i=0; i<data.fragment_count(); ++i) {
                  const auto& fragment = data.get_fragment(i);

                  auto item_tag = convhelper::encode_tag(Item, endianness);
                  encapsulated_data.insert(encapsulated_data.end(), std::begin(item_tag), std::end(item_tag));
                  auto frag_len = encode_len(4, fragment.size(), endianness);
                  encapsulated_data.insert(encapsulated_data.end(), std::begin(frag_len), std::end(frag_len));
                  std::copy(std::begin(fragment), std::end(fragment), std::back_inserter(encapsulated_data));
              }

              // sequence delimitation item
              auto seq_del = convhelper::encode_tag(SequenceDelimitationItem, endianness);
              auto zero_len = encode_len(4, 0, endianness);
              encapsulated_data.insert(encapsulated_data.end(), std::begin(seq_del), std::end(seq_del));
              encapsulated_data.insert(encapsulated_data.end(), std::begin(zero_len), std::end(zero_len));
          } else {
              // no compressed frame data

              // write empty basic offset table
              auto offset_table_entries = encode_len(4, 0, endianness);
              auto item_tag = convhelper::encode_tag(Item, endianness);
              encapsulated_data.insert(encapsulated_data.end(), std::begin(item_tag), std::end(item_tag));
              encapsulated_data.insert(encapsulated_data.end(), std::begin(offset_table_entries), std::end(offset_table_entries));

              for (std::size_t i=0; i<data.fragment_count(); ++i) {
                  const auto& fragment = data.get_fragment(i);

                  auto item_tag = convhelper::encode_tag(Item, endianness);
                  encapsulated_data.insert(encapsulated_data.end(), std::begin(item_tag), std::end(item_tag));
                  auto frag_len = encode_len(4, fragment.size(), endianness);
                  encapsulated_data.insert(encapsulated_data.end(), std::begin(frag_len), std::end(frag_len));
                  std::copy(std::begin(fragment), std::end(fragment), std::back_inserter(encapsulated_data));
              }

              // sequence delimitation item
              auto seq_del = convhelper::encode_tag(SequenceDelimitationItem, endianness);
              auto zero_len = encode_len(4, 0, endianness);
              encapsulated_data.insert(encapsulated_data.end(), std::begin(seq_del), std::end(seq_del));
              encapsulated_data.insert(encapsulated_data.end(), std::begin(zero_len), std::end(zero_len));
          }

          return encapsulated_data;
      }
};


/**
 * @brief dataset_bytesize calculates the size in bytes of a dataset given a
 *        certain transfer syntax.
 * @param data dataset
 * @param transfer_proc represents the underlying transfer syntax for the
 *        calculation
 * @return size of the dataset in bytes
 */
std::size_t dataset_bytesize(dicom::data::dataset::dataset_type data, const transfer_processor& transfer_proc)
{
    return std::accumulate(data.begin(), data.end(), 0,
                           [&transfer_proc](int acc, const std::pair<const tag_type, elementfield>& attr)
                           {
                               return acc += transfer_proc.dataelement_length(attr);
                           });
}


std::unique_ptr<transfer_processor> make_transfer_processor(std::string transfer_syntax_uid, dictionary::dictionaries& dict)
{
    auto supported = supported_transfer_syntaxes();
    transfer_syntax_uid.erase(std::remove_if(std::begin(transfer_syntax_uid), std::end(transfer_syntax_uid), [](char c) {return c==0;}), std::end(transfer_syntax_uid));
    if (std::find(std::begin(supported), std::end(supported), transfer_syntax_uid) != supported.end()) {
        if (transfer_syntax_uid == "1.2.840.10008.1.2") {
            return std::unique_ptr<transfer_processor>(new little_endian_implicit {dict});
        }
        if (transfer_syntax_uid == "1.2.840.10008.1.2.1") {
            return std::unique_ptr<transfer_processor>(new little_endian_explicit {dict});
        }
        if (transfer_syntax_uid == "1.2.840.10008.1.2.2") {
            return std::unique_ptr<transfer_processor>(new big_endian_explicit {dict});
        }
        if (transfer_syntax_uid == "1.2.840.10008.1.2.4.70" ||
            transfer_syntax_uid == "1.2.840.10008.1.2.4.50" ||
            transfer_syntax_uid == "1.2.840.10008.1.2.4.57") {
            return std::unique_ptr<transfer_processor>(new encapsulated {dict, transfer_syntax_uid});
        }
    } else {
        throw std::runtime_error("unsupported transfer syntax " + transfer_syntax_uid);
    }
    return nullptr;
}


}

}

}


#endif // TRANSFER_PROCESSOR_HPP
