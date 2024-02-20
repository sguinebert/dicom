#include "transfer_processor.hpp"

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

namespace dicom
{

namespace data
{

namespace dataset
{




// std::size_t transfer_processor::dataelement_length(const std::pair<tag_type, const elementfield&>& attribute) const
// {
//    const elementfield& ef = attribute.second;
//    if (vrtype == VR_TYPE::IMPLICIT) {
//       return ef.value_len + 4 + 2 + 2;
//    } else {
//       //std::size_t length = ef.value_len + 2 + 2;
//       //std::size_t length = ef.value_field->byte_size();
//       std::size_t length = ef.value_len;
//       if (ef.value_field->byte_size() != ef.value_len
//           && ef.value_len != 0xffffffff) {

//          if (ef.value_rep.is_initialized() &&
//              *ef.value_rep != VR::SQ &&
//              *ef.value_rep != VR::NN &&
//              *ef.value_rep != VR::NI) {
//             length = ef.value_field->byte_size();
//             BOOST_LOG_SEV(logger, warning) << "Mismatched value lengths for tag " << ef.value_len
//                                            << attribute.first << ": Expected "
//                                            << ef.value_len << ", actual "
//                                            << length;
//          }
//       }
//       length += 2 + 2;
//       if (is_special_VR(*ef.value_rep)) {
//          length += 2 + 2 + 4;
//       } else {
//          length += 2 + 2;
//       }
//       return length;
//    }
// }


// std::size_t transfer_processor::find_enclosing(const std::vector<unsigned char>& data, std::size_t beg) const
// {
//    std::size_t pos = beg;
//    int nested_sets = 0;

//    std::stack<std::size_t> beginnings;
//    beginnings.push(pos);

//    while (!beginnings.empty() && pos < data.size()) {
//       //pos = beginnings.top();
//       tag_type tag = decode_tag(data, pos, endianness);
//       if (tag == SequenceDelimitationItem || tag == ItemDelimitationItem) {
//          pos += 8;
//          beginnings.pop();
// //         std::size_t oldpos = beginnings.top();
// //         beginnings.top() += pos-oldpos;
//          continue;
//       }
//       pos += 4;

//       VR repr = deserialize_VR(data, tag, pos);
//       std::size_t value_len = deserialize_length(data, tag, repr, pos);

//       if (repr == VR::SQ) {
//          nested_sets++;
//       }

//       if ((value_len & 0xffffffff) == 0xffffffff) {
//          beginnings.push(pos);
//       } else {
//          pos += value_len;
//       }

//    }
//    return pos-beg;
// }


// dataset_type transfer_processor::deserialize(std::vector<unsigned char> data) const
// {
//    dataset_type dataset;
//    std::vector<dataset_type> outerset {dataset};

//    // current_data defines the serialized nested set which is processed at
//    // a time as half-open interval [start,start+nested_set_size).
//    std::stack<std::pair<std::size_t, std::size_t>> current_data;

//    std::stack<std::vector<dataset_type>> current_sequence;
//    std::stack<std::size_t> positions;
//    std::stack<std::pair<tag_type, std::size_t>> lasttag;

//    positions.push(0);
//    current_sequence.push(outerset);
//    current_data.push({0, data.size()});
//    while (current_data.size() > 0) {

//       bool undefined_length_item = false;
//       bool undefined_length_sequence = false;
//       // if we are coming right out of the deserialization of a nested sequence
//       // of items, add it to the last item deserialized before the nested sq.
//       if (lasttag.size() > 0) {
//          assert(current_sequence.size() > 1);
//          auto tag = lasttag.top().first;
//          auto size = lasttag.top().second;
//          lasttag.pop();
//          auto nested_seq = std::move(current_sequence.top());
//          current_sequence.pop();
//          current_sequence.top().back().emplace(tag, std::move(make_elementfield<VR::SQ>(size, std::move(nested_seq))));
//          positions.pop();

//       }

//       while (positions.top() < current_data.top().second) {
//          auto& pos = positions.top();
//          assert(!current_sequence.top().empty());

//          tag_type tag = decode_tag(data, pos, endianness);
//          pos += 4;

//          VR repr = deserialize_VR(data, tag, pos);
//          std::size_t value_len = deserialize_length(data, tag, repr, pos);


//          // Items and DelimitationItems do not have a VR or length field and are
//          // to be treated separately.
//          if (!is_item_attribute(tag)) {

//             // if the current item is a sequence, push the nested serialized
//             // data on the stack so it will be processed next. Store the current
//             // state of the deserialization on appropriate stacks.
//             if (repr == VR::SQ) {
//                undefined_length_sequence = ((value_len & 0xffffffff) == 0xffffffff);
//                value_len = ((value_len & 0xffffffff) == 0xffffffff)
//                      ? find_enclosing(data, pos)
//                      : value_len;
//                current_data.push({pos, pos+value_len});
//                current_sequence.push({dataset_type {}});
//                positions.push(current_data.top().first);
//                lasttag.push({tag, undefined_length_sequence ? 0xffffffff : value_len});
//             } else {
//                //auto multiplicity = get_dictionary().lookup(tag).vm;
//                current_sequence.top().back().emplace(tag, std::move(deserialize_attribute(data, endianness, value_len, repr, "*", pos)));

//                if (repr == VR::OB && value_len == 0xffffffff) {
//                   value_len = current_sequence.top().back()[tag].value_len;
//                }
//             }

//             pos += value_len;

//          } else {
//             // Put Items and respective DelimitationItems into the deserialized
//             // set.
//             if (tag == Item) {
//                // detect if an ItemDelimitationItem was missed
//                if (positions.top()-current_data.top().first > 8) {
//                   current_sequence.top().back()[ItemDelimitationItem]
//                         = make_elementfield<VR::NN>();
//                   current_sequence.top().push_back(dataset_type {});
//                }
//                undefined_length_item = ((value_len & 0xffffffff) == 0xffffffff);
//                if (!undefined_length_item) {
//                   current_sequence.top().back()[tag]
//                         = make_elementfield<VR::NI>(value_len, VR::NI);
//                } else {
//                   current_sequence.top().back()[tag]
//                         = make_elementfield<VR::NI>(0xffffffff, VR::NI);
//                }

//             } else if (tag == ItemDelimitationItem) {
//                // ItemDelimitationItem for this item is set when the next Item
//                // tag is encountered to handle possibly missing
//                // ItemDelimitationItems
//             } else if (tag == SequenceDelimitationItem) {
//                // sequence delimitation is in a separate, the last, vector
//                if (current_sequence.top().size() > 0) {
//                   current_sequence.top().push_back(dataset_type {});
//                }
//                current_sequence.top().back()[tag] = make_elementfield<VR::NN>();
//             }
//          }
//       }

//       // add item and sequence delimination on the last item of a nested
//       // sequence, leaving the outermost without.
//       if (current_sequence.size() > 1) {
//          auto& itemset = current_sequence.top().back();
//          auto& prevset = *(current_sequence.top().end()-2);
//          if (current_sequence.top().size() < 2) {
//             current_sequence.top().back()[ItemDelimitationItem] = make_elementfield<VR::NN>();
//          } else {
//             bool have_item_delimitation = false;
//             if (prevset.find(Item) != prevset.end() && prevset.find(ItemDelimitationItem) == prevset.end()) {
//                prevset[ItemDelimitationItem] = make_elementfield<VR::NN>();
//                have_item_delimitation = true;
//             }
//             if (!have_item_delimitation && itemset.find(ItemDelimitationItem) == itemset.end() &&
//                 !(itemset.size() == 1 && itemset.find(SequenceDelimitationItem) != itemset.end())) {
//                itemset[ItemDelimitationItem] = make_elementfield<VR::NN>();
//                have_item_delimitation = true;
//             }
//          }
//          current_sequence.top().back()[SequenceDelimitationItem] = make_elementfield<VR::NN>();
//       }
//       current_data.pop();
//    }

//    // The size of the outermost set should be exactly one
//    assert(current_sequence.top().size() == 1);
//    return std::move(current_sequence.top()[0]); // current_sequence not used after this, move necessary
// }


// VR transfer_processor::deserialize_VR(const std::vector<unsigned char>& dataset, tag_type tag, std::size_t& pos) const
// {
//    if (!is_item_attribute(tag)) {
//       if (vrtype != VR_TYPE::IMPLICIT) {
//          VR repr = dictionary::dictionary_entry::vr_of_string
//                .left.at(std::string {dataset.begin()+pos, dataset.begin()+pos+2});
//          if (is_special_VR(repr)) {
//             pos += 4;
//          } else {
//             pos += 2;
//          }
//          return repr;
//       } else {
//          return get_vr(tag);
//       }
//    }
//    return VR::NN;
// }




// std::vector<unsigned char> commandset_processor::serialize_attribute(elementfield& e, ENDIANNESS end, VR vr) const
// {
//    return encode_value_field(e, end, vr);
// }

// std::vector<unsigned char> transfer_processor::serialize(iod dataset) const
// {
//    calculate_item_lengths(dataset);

//    std::vector<unsigned char> stream;
//    bool explicit_length_item = true;
//    bool explicit_length_sequence = true;
//    for (auto attr : dataset_iterator_adaptor(dataset)) {
//       if (attr.first == SequenceDelimitationItem) {
//          if (!explicit_length_sequence) {
//             auto tag = encode_tag(attr.first, endianness);
//             auto len = encode_len(4, attr.second.value_len, endianness);
//             stream.insert(stream.end(), tag.begin(), tag.end());
//             stream.insert(stream.end(), len.begin(), len.end());
//          }
//          continue;
//       } else if (attr.first == ItemDelimitationItem) {
//          if (!explicit_length_item)  {
//             auto tag = encode_tag(attr.first, endianness);
//             auto len = encode_len(4, attr.second.value_len, endianness);
//             stream.insert(stream.end(), tag.begin(), tag.end());
//             stream.insert(stream.end(), len.begin(), len.end());
//          }
//          continue;
//       } else if (attr.first == Item) {
//          auto tag = encode_tag(attr.first, endianness);
//          auto len = encode_len(4, attr.second.value_len, endianness);
//          stream.insert(stream.end(), tag.begin(), tag.end());
//          stream.insert(stream.end(), len.begin(), len.end());
//          explicit_length_item = (attr.second.value_len != 0xffffffff);
//          continue;
//       }

//       VR repr;
//       if (vrtype == VR_TYPE::EXPLICIT || attr.second.value_rep.is_initialized()) {
//          repr = attr.second.value_rep.get();
//       } else {
//          repr = get_vr(attr.first);
//       }

//       auto& value_field = attr.second;
//       auto data = serialize_attribute(value_field, endianness, repr);
//       std::size_t value_length = value_field.value_len;
//       if (data.size() != value_field.value_len
//           && value_field.value_len != 0xffffffff) {

//          if (value_field.value_rep.is_initialized() &&
//              *value_field.value_rep != VR::SQ &&
//              *value_field.value_rep != VR::NN &&
//              *value_field.value_rep != VR::NI) {
//             BOOST_LOG_SEV(logger, warning) << "Mismatched value lengths for tag "
//                                            << attr.first << ": Expected "
//                                            << value_field.value_len << ", actual "
//                                            << data.size();
//             value_length = data.size();
//          }
//       }

//       auto tag = encode_tag(attr.first, endianness);
//       std::vector<unsigned char> len;
//       stream.insert(stream.end(), tag.begin(), tag.end());
//       if (vrtype == VR_TYPE::EXPLICIT) {
//          auto vr = dictionary::dictionary_entry::vr_of_string.right.at(repr);
//          stream.insert(stream.end(), vr.begin(), vr.begin()+2);
//          if (is_special_VR(repr)) {
//             stream.push_back(0x00); stream.push_back(0x00);
//             len = encode_len(4, value_length, endianness);
//          } else {
//             len = encode_len(2, value_length, endianness);
//          }
//       } else {
//          len = encode_len(4, value_length, endianness);
//       }

//       if (repr == VR::SQ)
//       {
//          explicit_length_sequence = (attr.second.value_len != 0xffffffff);
//       }

//       stream.insert(stream.end(), len.begin(), len.end());
//       stream.insert(stream.end(), data.begin(), data.end());
//    }
//    return stream;
// }

// std::string transfer_processor::get_transfer_syntax() const
// {
//    return transfer_syntax;
// }

// elementfield commandset_processor::deserialize_attribute(std::vector<unsigned char>& data,
//                                                          attribute::ENDIANNESS end,
//                                                        std::size_t len,
//                                                        VR vr, std::string vm,
//                                                        std::size_t pos) const
// {
//    return decode_value_field(data, end, len, vr, vm, pos);
// }


// transfer_processor::transfer_processor(boost::optional<dictionary::dictionaries&> dict,
//                                        std::string tfs, VR_TYPE vrtype,
//                                        ENDIANNESS endianness,
//                                        std::initializer_list<vr_of_tag> tstags):
//    tstags {tstags},
//    dict(dict),
//    transfer_syntax {tfs},
//    vrtype {vrtype},
//    endianness {endianness},
//    logger {"transfer processor"}
// {
//    if (vrtype == VR_TYPE::IMPLICIT && !dict.is_initialized()) {
//       throw std::runtime_error("Uninitialized dictionary with "
//                                "implicit VR type specificed!");
//    }
// }

// data::dictionary::dictionaries& transfer_processor::get_dictionary() const
// {
//    return *dict;
// }

// transfer_processor::transfer_processor(const transfer_processor& other):
//    dict {other.dict},
//    transfer_syntax {other.transfer_syntax},
//    vrtype {other.vrtype},
//    logger {"transfer processor"}
// {
// }

// VR transfer_processor::get_vr(tag_type tag) const
// {
//    auto spectag = std::find_if(tstags.begin(), tstags.end(),
//                                [tag](vr_of_tag vrt) {
//          return (tag.group_id & vrt.gid_mask) == vrt.tag.group_id
//              && (tag.element_id & vrt.eid_mask) == vrt.tag.element_id;
//    });
//    if (spectag == tstags.end()) {
//       return dict.get().lookup(tag).vr[0];
//    } else {
//       return spectag->vr;
//    }
// }

// little_endian_implicit::little_endian_implicit(dictionary::dictionaries& dict, std::string transfer_syntax):
//    transfer_processor {boost::optional<dictionary::dictionaries&> {dict},
//                        transfer_syntax,
//                        VR_TYPE::IMPLICIT,
//                        ENDIANNESS::LITTLE,
//                        {
//                            {{0x7fe0, 0x0010}, VR::OW},
//                            {{0x6000, 0x3000}, VR::OW, 0xff00, 0xffff},
//                            {{0x5400, 0x1010}, VR::OW}, {{0x0028, 0x1201}, VR::OW},
//                            {{0x0028, 0x1202}, VR::OW}, {{0x0028, 0x1203}, VR::OW},
//                            {{0x0028, 0x1204}, VR::OW}, {{0x0028, 0x1101}, VR::SS},
//                            {{0x0028, 0x1102}, VR::SS}, {{0x0028, 0x1103}, VR::SS},
//                            {{0x0028, 0x1221}, VR::OW}, {{0x0028, 0x1222}, VR::OW},
//                            {{0x0028, 0x1223}, VR::OW}, {{0x0028, 0x3006}, VR::US},
//                            {{0x0028, 0x3002}, VR::SS}, {{0x0028, 0x1408}, VR::OW},
//                            {{0x0066, 0x0025}, VR::OW}, {{0x0066, 0x0024}, VR::OW},
//                            {{0x0066, 0x0023}, VR::OW}, {{0x0066, 0x0029}, VR::OW}
//                        } }
// {
// }

// little_endian_implicit::little_endian_implicit(const little_endian_implicit& other):
//    transfer_processor {other}
// {

// }

// std::vector<unsigned char> little_endian_implicit::serialize_attribute(elementfield& e, ENDIANNESS end, attribute::VR vr) const
// {
//    return encode_value_field(e, end, vr);
// }

// elementfield little_endian_implicit::deserialize_attribute(std::vector<unsigned char>& data,
//                                                            attribute::ENDIANNESS end,
//                                                            std::size_t len, attribute::VR vr,
//                                                            std::string vm,
//                                                            std::size_t pos) const
// {
//    return decode_value_field(data, end, len, vr, vm, pos);
// }

// transfer_processor::vr_of_tag::vr_of_tag(tag_type tag,
//                                          VR vr,
//                                          unsigned eid_mask,
//                                          unsigned gid_mask):
//    tag {tag},
//    eid_mask {eid_mask},
//    gid_mask {gid_mask},
//    vr {vr}
// {
// }

// little_endian_explicit::little_endian_explicit(dictionary::dictionaries& dict, std::string transfer_syntax):
//    transfer_processor {boost::optional<dictionary::dictionaries&> {dict},
//                        transfer_syntax,
//                        VR_TYPE::EXPLICIT, ENDIANNESS::LITTLE,
//                        {
//                            {{0x7fe0, 0x0010}, VR::OW},
//                            {{0x6000, 0x3000}, VR::OW, 0xff00, 0xffff},
//                            {{0x5400, 0x1010}, VR::OW}, {{0x0028, 0x1201}, VR::OW},
//                            {{0x0028, 0x1202}, VR::OW}, {{0x0028, 0x1203}, VR::OW},
//                            {{0x0028, 0x1204}, VR::OW}, {{0x0028, 0x1101}, VR::SS},
//                            {{0x0028, 0x1102}, VR::SS}, {{0x0028, 0x1103}, VR::SS},
//                            {{0x0028, 0x1221}, VR::OW}, {{0x0028, 0x1222}, VR::OW},
//                            {{0x0028, 0x1223}, VR::OW}, {{0x0028, 0x3006}, VR::US},
//                            {{0x0028, 0x3002}, VR::SS}, {{0x0028, 0x1408}, VR::OW},
//                            {{0x0066, 0x0025}, VR::OW}, {{0x0066, 0x0024}, VR::OW},
//                            {{0x0066, 0x0023}, VR::OW}, {{0x0066, 0x0029}, VR::OW}
//                        } }
// {

// }

// std::vector<unsigned char> little_endian_explicit::serialize_attribute(elementfield& e, attribute::ENDIANNESS end, VR vr) const
// {
//    return encode_value_field(e, end, vr);
// }

// elementfield little_endian_explicit::deserialize_attribute(std::vector<unsigned char>& data, ENDIANNESS end,
//                                                            std::size_t len, VR vr, std::string vm,
//                                                            std::size_t pos) const
// {
//    return decode_value_field(data, end, len, vr, vm, pos);
// }

// big_endian_explicit::big_endian_explicit(dictionary::dictionaries& dict, std::string transfer_syntax):
//    transfer_processor {boost::optional<dictionary::dictionaries&> {dict},
//                        transfer_syntax,
//                        VR_TYPE::EXPLICIT, ENDIANNESS::BIG,
//                        {
//                            {{0x7fe0, 0x0010}, VR::OW},
//                            {{0x6000, 0x3000}, VR::OW, 0xff00, 0xffff},
//                            {{0x5400, 0x1010}, VR::OW}, {{0x0028, 0x1201}, VR::OW},
//                            {{0x0028, 0x1202}, VR::OW}, {{0x0028, 0x1203}, VR::OW},
//                            {{0x0028, 0x1204}, VR::OW}, {{0x0028, 0x1101}, VR::SS},
//                            {{0x0028, 0x1102}, VR::SS}, {{0x0028, 0x1103}, VR::SS},
//                            {{0x0028, 0x1221}, VR::OW}, {{0x0028, 0x1222}, VR::OW},
//                            {{0x0028, 0x1223}, VR::OW}, {{0x0028, 0x3006}, VR::US},
//                            {{0x0028, 0x3002}, VR::SS}, {{0x0028, 0x1408}, VR::OW},
//                            {{0x0066, 0x0025}, VR::OW}, {{0x0066, 0x0024}, VR::OW},
//                            {{0x0066, 0x0023}, VR::OW}, {{0x0066, 0x0029}, VR::OW}
//                        } }
// {

// }

// std::vector<unsigned char> big_endian_explicit::serialize_attribute(elementfield& e, attribute::ENDIANNESS end, VR vr) const
// {
//    return encode_value_field(e, end, vr);
// }

// elementfield big_endian_explicit::deserialize_attribute(std::vector<unsigned char>& data, ENDIANNESS end,
//                                                            std::size_t len, VR vr, std::string vm,
//                                                            std::size_t pos) const
// {
//    return decode_value_field(data, end, len, vr, vm, pos);
// }


// encapsulated::encapsulated(dictionary::dictionaries& dict, std::string transfer_syntax):
//    little_endian_explicit {dict, transfer_syntax}
// {

// }



// elementfield encapsulated::deserialize_attribute(std::vector<unsigned char>& data, ENDIANNESS end,
//                                                            std::size_t len, VR vr, std::string vm,
//                                                            std::size_t pos) const
// {
//    if (vr == VR::OB && len == 0xffffffff) {
//       //::encapsulated encapsulated_data;
//       std::size_t fragment_size = 0;
//       auto encapsulated_data = deserialize_fragments(data, pos, fragment_size);
//       typename type_of<VR::OB>::type ob_data {encapsulated_data};
//       return make_elementfield<VR::OB>(fragment_size, ob_data);
//    } else {
//       return decode_value_field(data, end, len, vr, vm, pos);
//    }
// }



// std::vector<unsigned char> encapsulated::serialize_fragments(attribute::encapsulated data) const
// {
//    std::vector<unsigned char> encapsulated_data;
//    if (data.have_compressed_frame_info()) {
//       // write basic offset table
//       std::vector<unsigned char> offset_table;
//       std::size_t accu = 0;
//       for (std::size_t i=0; i<data.fragment_count(); ++i) {
//          if (data.marks_frame_start(i)) {
//             auto frag_len = encode_len(4, accu, endianness);
//             offset_table.insert(offset_table.end(), std::begin(frag_len), std::end(frag_len));
//          }
//          accu += 4; // Item tag
//          accu += 4; // Length field
//          const auto& fragment = data.get_fragment(i);
//          accu += fragment.size();
//       }

//       auto offset_table_entries = encode_len(4, offset_table.size(), endianness);
//       auto item_tag = encode_tag(Item, endianness);
//       encapsulated_data.insert(encapsulated_data.end(), std::begin(item_tag), std::end(item_tag));
//       encapsulated_data.insert(encapsulated_data.end(), std::begin(offset_table_entries), std::end(offset_table_entries));

//       std::copy(std::begin(offset_table), std::end(offset_table), std::back_inserter(encapsulated_data));

//       // now the actual values
//       for (std::size_t i=0; i<data.fragment_count(); ++i) {
//          const auto& fragment = data.get_fragment(i);

//          auto item_tag = encode_tag(Item, endianness);
//          encapsulated_data.insert(encapsulated_data.end(), std::begin(item_tag), std::end(item_tag));
//          auto frag_len = encode_len(4, fragment.size(), endianness);
//          encapsulated_data.insert(encapsulated_data.end(), std::begin(frag_len), std::end(frag_len));
//          std::copy(std::begin(fragment), std::end(fragment), std::back_inserter(encapsulated_data));
//       }

//       // sequence delimitation item
//       auto seq_del = encode_tag(SequenceDelimitationItem, endianness);
//       auto zero_len = encode_len(4, 0, endianness);
//       encapsulated_data.insert(encapsulated_data.end(), std::begin(seq_del), std::end(seq_del));
//       encapsulated_data.insert(encapsulated_data.end(), std::begin(zero_len), std::end(zero_len));
//    } else {
//       // no compressed frame data

//       // write empty basic offset table
//       auto offset_table_entries = encode_len(4, 0, endianness);
//       auto item_tag = encode_tag(Item, endianness);
//       encapsulated_data.insert(encapsulated_data.end(), std::begin(item_tag), std::end(item_tag));
//       encapsulated_data.insert(encapsulated_data.end(), std::begin(offset_table_entries), std::end(offset_table_entries));

//       for (std::size_t i=0; i<data.fragment_count(); ++i) {
//          const auto& fragment = data.get_fragment(i);

//          auto item_tag = encode_tag(Item, endianness);
//          encapsulated_data.insert(encapsulated_data.end(), std::begin(item_tag), std::end(item_tag));
//          auto frag_len = encode_len(4, fragment.size(), endianness);
//          encapsulated_data.insert(encapsulated_data.end(), std::begin(frag_len), std::end(frag_len));
//          std::copy(std::begin(fragment), std::end(fragment), std::back_inserter(encapsulated_data));
//       }

//       // sequence delimitation item
//       auto seq_del = encode_tag(SequenceDelimitationItem, endianness);
//       auto zero_len = encode_len(4, 0, endianness);
//       encapsulated_data.insert(encapsulated_data.end(), std::begin(seq_del), std::end(seq_del));
//       encapsulated_data.insert(encapsulated_data.end(), std::begin(zero_len), std::end(zero_len));
//    }

//    return encapsulated_data;
// }


// std::vector<std::string> supported_transfer_syntaxes()
// {
//    return std::vector<std::string>
//    {
//       "1.2.840.10008.1.2",
//       "1.2.840.10008.1.2.1",
//       "1.2.840.10008.1.2.2",
//       "1.2.840.10008.1.2.4.70",
//       "1.2.840.10008.1.2.4.50",
//       "1.2.840.10008.1.2.4.57"
//    };
// }

// std::unique_ptr<transfer_processor> make_transfer_processor(std::string transfer_syntax_uid, dictionary::dictionaries& dict)
// {
//    auto supported = supported_transfer_syntaxes();
//    transfer_syntax_uid.erase(std::remove_if(std::begin(transfer_syntax_uid), std::end(transfer_syntax_uid), [](char c) {return c==0;}), std::end(transfer_syntax_uid));
//    if (std::find(std::begin(supported), std::end(supported), transfer_syntax_uid) != supported.end()) {
//       if (transfer_syntax_uid == "1.2.840.10008.1.2") {
//          return std::unique_ptr<transfer_processor>(new little_endian_implicit {dict});
//       }
//       if (transfer_syntax_uid == "1.2.840.10008.1.2.1") {
//          return std::unique_ptr<transfer_processor>(new little_endian_explicit {dict});
//       }
//       if (transfer_syntax_uid == "1.2.840.10008.1.2.2") {
//          return std::unique_ptr<transfer_processor>(new big_endian_explicit {dict});
//       }
//       if (transfer_syntax_uid == "1.2.840.10008.1.2.4.70" ||
//           transfer_syntax_uid == "1.2.840.10008.1.2.4.50" ||
//           transfer_syntax_uid == "1.2.840.10008.1.2.4.57") {
//          return std::unique_ptr<transfer_processor>(new encapsulated {dict, transfer_syntax_uid});
//       }
//    } else {
//       throw std::runtime_error("unsupported transfer syntax " + transfer_syntax_uid);
//    }
// }


}

}

}
