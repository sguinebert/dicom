#include "upperlayer_properties.hpp"

#include <algorithm>
#include <ostream>
#include <vector>
#include <string>
#include <cassert>

namespace dicom
{

namespace network
{

namespace upperlayer
{

namespace
{
using uchar = unsigned char;

// std::size_t be_char_to_16b(std::vector<uchar> bs)
// {
//    assert(bs.size() == 2);
//    std::size_t sz = 0;
//    sz |= (static_cast<std::size_t>(bs[0]) << 8);
//    sz |= (static_cast<std::size_t>(bs[1]));
//    return sz;
// }
// std::size_t be_char_to_32b(std::vector<uchar> bs)
// {
//    assert(bs.size() == 4);
//    std::size_t sz = 0;
//    sz |= (static_cast<std::size_t>(bs[0]) << 24);
//    sz |= (static_cast<std::size_t>(bs[1]) << 16);
//    sz |= (static_cast<std::size_t>(bs[2]) << 8);
//    sz |= (static_cast<std::size_t>(bs[3]));
//    return sz;
// }
// std::vector<uchar> ui_to_16b_be(unsigned val)
// {
//    std::vector<unsigned char> be_val(2);
//    be_val[0] = ((val & 0xFF00) >> 8);
//    be_val[1] = (val & 0xFF);
//    return be_val;
// }
// std::vector<uchar> ui_to_32b_be(unsigned val)
// {
//    std::vector<unsigned char> be_val(4);
//    be_val[0] = ((val & 0xFF000000) >> 24);
//    be_val[1] = ((val & 0xFF0000)) >> 16;
//    be_val[2] = ((val & 0xFF00) >> 8);
//    be_val[3] = (val & 0xFF);
//    return be_val;
// }
// }



// TYPE get_type(const std::vector<unsigned char>& pdu)
// {
//    return static_cast<TYPE>(pdu[0]);
// }


// property::~property()
// {
// }




// std::vector<uchar> p_data_tf::make_pdu() const
// {
//    const std::size_t preamble_length = 12;
//    const std::size_t m_length = msg_length-preamble_length;
//    std::vector<uchar> pack;

//    {
//       // insert command part
//       // one extra byte each for message id and message header
//       std::vector<uchar> pdv_len;

//       if (!command_set.empty()) {
//          auto begin = pack.end()-pack.begin();
//          pack.push_back(static_cast<uchar>(TYPE::P_DATA_TF));
//          pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x00});
//          std::size_t len_pos = begin + 2;

//          pdv_len = ui_to_32b_be(command_set.size()+2);
//          pack.insert(pack.end(), pdv_len.begin(), pdv_len.end());
//          pack.push_back(pres_context_id);
// //         if (data_set.empty()) {
// //            pack.push_back(0x03);
// //         } else {
// //            pack.push_back(0x01);
// //         }
//          pack.push_back(0x03);
//          pack.insert(pack.end(), command_set.begin(), command_set.end());

//          std::size_t pdu_len = pack.size()-6;
//          std::vector<uchar> plen = ui_to_32b_be(pdu_len);
//          std::copy(plen.begin(), plen.end(), pack.begin()+len_pos);
//       }

//       // insert pack header
//       if (!data_set.empty()) {
//          for (std::size_t pos = 0; pos < data_set.size(); pos += m_length) {
//             auto begin = pack.end()-pack.begin();
//             pack.push_back(static_cast<uchar>(TYPE::P_DATA_TF));
//             pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x00});
//             std::size_t len_pos = begin + 2;

//             auto remaining = std::min(m_length, data_set.size()-pos);
//             pdv_len = ui_to_32b_be(remaining+2);
//             pack.insert(pack.end(), pdv_len.begin(), pdv_len.end());
//             pack.push_back(pres_context_id);

//             if (remaining < m_length) {
//                pack.push_back(0x02);
//             } else {
//                pack.push_back(0x00);
//             }
//             pack.insert(pack.end(), data_set.begin()+pos, data_set.begin()+pos+remaining);

//             std::size_t pdu_len = remaining+6;
//             std::vector<uchar> plen = ui_to_32b_be(pdu_len);
//             std::copy(plen.begin(), plen.end(), pack.begin()+len_pos);
//          }
//       }
//    }

//    return pack;
// }









/**
 * @brief this function operates on the structured connection properties and
 *        deserializes it into a ready-to-transmit a-associate-rq or an
 *        a-associate-ac pdu
 * @param[in] t
 * @return pdu representing structured data
 */
// std::vector<uchar> a_associate_rq::make_pdu() const
// {
//    std::vector<uchar> pack;
//    pack.push_back(static_cast<uchar>(TYPE::A_ASSOCIATE_RQ));
//    pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}); // preamble
//    std::size_t len_pos = 2;

//    // insert called and calling ae
//    assert(called_ae.size() == 16 && calling_ae.size() == 16);
//    pack.insert(pack.end(), called_ae.begin(), called_ae.end());
//    pack.insert(pack.end(), calling_ae.begin(), calling_ae.end());
//    pack.insert(pack.end(), 32, 0x00);

//    {
//       // insert application context item
//       pack.insert(pack.end(), {0x10, 0x00});
//       std::vector<uchar> ac_len = ui_to_16b_be(application_context.size());
//       pack.insert(pack.end(), ac_len.begin(), ac_len.end());
//       pack.insert(pack.end(), application_context.begin(), application_context.end());

//       // insert presentation context items
//       for (const auto pc : pres_contexts) {
//          pack.insert(pack.end(), {0x20, 0x00});

//          // Calculating size of presentation context
//          std::size_t pcl = 0;
//          pcl += pc.abstract_syntax.size()+4; //+4 <=> preamble of abstract syntax item
//          for (const auto ts : pc.transfer_syntaxes) {
//             pcl += ts.size()+4; //+4 <=> preamble of transfer syntax item
//          }
//          pcl += 4; //size of presentation context id info

//          std::vector<uchar> pc_len = ui_to_16b_be(pcl);
//          pack.insert(pack.end(), pc_len.begin(), pc_len.end());
//          pack.insert(pack.end(), {pc.id, 0x00, 0x00, 0x00});

//          {
//             // insert abstract syntax
//             pack.insert(pack.end(), {0x30, 0x00});
//             std::vector<uchar> as_len = ui_to_16b_be(pc.abstract_syntax.size());
//             pack.insert(pack.end(), as_len.begin(), as_len.end());
//             pack.insert(pack.end(), pc.abstract_syntax.begin(), pc.abstract_syntax.end());


//             for (const auto ts : pc.transfer_syntaxes) {
//                pack.insert(pack.end(), {0x40, 0x00});
//                std::vector<uchar> ts_len = ui_to_16b_be(ts.size() ); //+4 for padding
//                pack.insert(pack.end(), ts_len.begin(), ts_len.end());
//                pack.insert(pack.end(), ts.begin(), ts.end());
//             }
//          }
//       }

//       // insert user info item
//       pack.insert(pack.end(), {0x50, 0x00});
//       std::vector<uchar> ui_len = ui_to_16b_be(0x08);
//       pack.insert(pack.end(), ui_len.begin(), ui_len.end());

//       {
//          // insert maximum length item
//          pack.insert(pack.end(), {0x51, 0x00, 0x00, 0x04});
//          std::vector<uchar> max_len = ui_to_32b_be(max_message_length);
//          pack.insert(pack.end(), max_len.begin(), max_len.end());
//       }
//    }

//    std::size_t pdu_len = pack.size()-6;
//    std::vector<uchar> plen = ui_to_32b_be(pdu_len);
//    std::copy(plen.begin(), plen.end(), pack.begin()+len_pos);

//    return pack;
// }









// std::vector<uchar> a_associate_ac::make_pdu() const
// {
//    std::vector<uchar> pack;
//    pack.push_back(static_cast<uchar>(TYPE::A_ASSOCIATE_AC));
//    pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}); // preamble
//    std::size_t len_pos = 2;

//    // insert called and calling ae
//    assert(called_ae.size() == 16 && calling_ae.size() == 16);
//    pack.insert(pack.end(), called_ae.begin(), called_ae.end());
//    pack.insert(pack.end(), calling_ae.begin(), calling_ae.end());
//    pack.insert(pack.end(), 32, 0x00);

//    {
//       // insert application context item
//       pack.insert(pack.end(), {0x10, 0x00});
//       std::vector<uchar> ac_len = ui_to_16b_be(application_context.size());
//       pack.insert(pack.end(), ac_len.begin(), ac_len.end());
//       pack.insert(pack.end(), application_context.begin(), application_context.end());

//       // insert presentation context items
//       for (const auto pc : pres_contexts) {
//          pack.insert(pack.end(), {0x21, 0x00});

//          // Calculating size of presentation context
//          std::size_t pcl = pc.transfer_syntax.size();

//          std::vector<uchar> pc_len = ui_to_16b_be(pcl +4 +4); // 4 bytes padding (pc and ts)
//          pack.insert(pack.end(), pc_len.begin(), pc_len.end());
//          pack.insert(pack.end(), {pc.id, 0x00, static_cast<uchar>(pc.result_), 0x00});

//          {
//             pack.insert(pack.end(), {0x40, 0x00});
//             std::vector<uchar> ts_len = ui_to_16b_be(pc.transfer_syntax.size() );
//             pack.insert(pack.end(), ts_len.begin(), ts_len.end());
//             pack.insert(pack.end(), pc.transfer_syntax.begin(), pc.transfer_syntax.end());
//          }
//       }

//       // insert user info item
//       pack.insert(pack.end(), {0x50, 0x00});
//       std::vector<uchar> ui_len = ui_to_16b_be(0x08);
//       pack.insert(pack.end(), ui_len.begin(), ui_len.end());

//       {
//          // insert maximum length item
//          pack.insert(pack.end(), {0x51, 0x00, 0x00, 0x04});
//          std::vector<uchar> max_len = ui_to_32b_be(max_message_length);
//          pack.insert(pack.end(), max_len.begin(), max_len.end());
//       }
//    }

//    std::size_t pdu_len = pack.size()-6;
//    std::vector<uchar> plen = ui_to_32b_be(pdu_len);
//    std::copy(plen.begin(), plen.end(), pack.begin()+len_pos);

//    return pack;
// }






// void a_associate_rj::from_pdu(std::vector<uchar> pdu)
// {
//    source_ = static_cast<SOURCE>(pdu[8]);
//    reason_ = static_cast<REASON>(pdu[9]);
// }

// std::vector<uchar> a_associate_rj::make_pdu() const
// {
//    std::vector<uchar> pack;
//    pack.push_back(static_cast<uchar>(TYPE::A_ASSOCIATE_RJ));
//    pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01});
//    pack.push_back(static_cast<uchar>(source_));
//    pack.push_back(static_cast<uchar>(reason_));
//    return pack;
// }








// std::vector<uchar> a_release_rq::make_pdu() const
// {
//    std::vector<uchar> pack;
//    pack.push_back(static_cast<uchar>(TYPE::A_RELEASE_RQ));
//    pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00});
//    return pack;
// }








// std::vector<uchar> a_release_rp::make_pdu() const
// {
//    std::vector<uchar> pack;
//    pack.push_back(static_cast<uchar>(TYPE::A_RELEASE_RP));
//    pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00});
//    return pack;
// }








// std::vector<uchar> a_abort::make_pdu() const
// {
//    std::vector<uchar> pack;
//    pack.push_back(static_cast<uchar>(TYPE::A_ABORT));
//    pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x01});
//    pack.push_back(static_cast<uchar>(source_));
//    pack.push_back(static_cast<uchar>(reason_));
//    return pack;
// }







}

}

}
