#ifndef UPPERLAYER_CONNECTION_PROPERTIES_HPP
#define UPPERLAYER_CONNECTION_PROPERTIES_HPP

#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <ostream>
#include <cassert>

#include "data/dataset/tools.h"

namespace dicom
{

namespace network
{

namespace upperlayer
{

enum class TYPE : unsigned char
{
   A_ASSOCIATE_RQ = 0x01,
   A_ASSOCIATE_AC = 0x02,
   A_ASSOCIATE_RJ = 0x03,
   P_DATA_TF = 0x04,
   A_RELEASE_RQ = 0x05,
   A_RELEASE_RP = 0x06,
   A_ABORT = 0x07
};

TYPE get_type(const std::vector<unsigned char>& pdu)
{
    return static_cast<TYPE>(pdu[0]);
}

TYPE get_type(const std::vector<unsigned char>& pdu);

//std::ostream& operator<<(std::ostream& out, TYPE t);


/**
 * @brief The property struct or rather its subclasses represent the serial pdu data
 *        in a structured form, so information contained in these pdus can be accessed
 *        easily.
 */
struct property
{
      virtual void from_pdu(std::vector<unsigned char> pdu) = 0;
      virtual std::vector<unsigned char> make_pdu() const = 0;
      virtual TYPE type() const = 0;
      virtual std::ostream& print(std::ostream& os) const = 0;
      virtual ~property() = 0;
};

std::ostream& operator<<(std::ostream& os, const property& p);

/**
 * @brief make_property is a factory function which creates structured data
 *        depending on the type of pdu passed to it
 * @param[in] pdu
 * @return unique_ptr to the structured data
 */
std::unique_ptr<property> make_property(const std::vector<unsigned char>& pdu);



struct p_data_tf: property
{
      p_data_tf() = default;
    void from_pdu(std::vector<unsigned char> pdu) override
    {
        std::size_t pos = 0;
        bool pdvs_left = true;
        while (pdvs_left || pos < pdu.size()-1) {
            pos += 6;
            std::size_t pdv_len = be_char_to_32b({pdu.begin() + pos, pdu.begin() + pos + 4});
            pos += 4;
            pres_context_id = pdu[pos];

            unsigned char msg_control = pdu[pos+1];

            pos += 2;
            if (msg_control & 0x01) {
                command_set.insert(command_set.end(), pdu.begin()+pos, pdu.begin()+(pos+pdv_len-2));
            } else {
                data_set.insert(data_set.end(), pdu.begin()+pos, pdu.begin()+(pos+pdv_len-2));
            }

            pdvs_left = !(msg_control & 0x02);
            pos += pdv_len-2; // message control id and presentation context id are
                // included in the length
        }
    }
      std::vector<unsigned char> make_pdu() const override
    {
        const std::size_t preamble_length = 12;
        const std::size_t m_length = msg_length-preamble_length;
        std::vector<uchar> pack;

        {
            // insert command part
            // one extra byte each for message id and message header
            std::vector<uchar> pdv_len;

            if (!command_set.empty()) {
                auto begin = pack.end()-pack.begin();
                pack.push_back(static_cast<uchar>(TYPE::P_DATA_TF));
                pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x00});
                std::size_t len_pos = begin + 2;

                pdv_len = ui_to_32b_be(command_set.size()+2);
                pack.insert(pack.end(), pdv_len.begin(), pdv_len.end());
                pack.push_back(pres_context_id);
                //         if (data_set.empty()) {
                //            pack.push_back(0x03);
                //         } else {
                //            pack.push_back(0x01);
                //         }
                pack.push_back(0x03);
                pack.insert(pack.end(), command_set.begin(), command_set.end());

                std::size_t pdu_len = pack.size()-6;
                std::vector<uchar> plen = ui_to_32b_be(pdu_len);
                std::copy(plen.begin(), plen.end(), pack.begin()+len_pos);
            }

            // insert pack header
            if (!data_set.empty()) {
                for (std::size_t pos = 0; pos < data_set.size(); pos += m_length) {
                    auto begin = pack.end()-pack.begin();
                    pack.push_back(static_cast<uchar>(TYPE::P_DATA_TF));
                    pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x00});
                    std::size_t len_pos = begin + 2;

                    auto remaining = std::min(m_length, data_set.size()-pos);
                    pdv_len = ui_to_32b_be(remaining+2);
                    pack.insert(pack.end(), pdv_len.begin(), pdv_len.end());
                    pack.push_back(pres_context_id);

                    if (remaining < m_length) {
                        pack.push_back(0x02);
                    } else {
                        pack.push_back(0x00);
                    }
                    pack.insert(pack.end(), data_set.begin()+pos, data_set.begin()+pos+remaining);

                    std::size_t pdu_len = remaining+6;
                    std::vector<uchar> plen = ui_to_32b_be(pdu_len);
                    std::copy(plen.begin(), plen.end(), pack.begin()+len_pos);
                }
            }
        }

        return pack;
    }

    TYPE type() const override
    {
        return TYPE::P_DATA_TF;
    }
    std::ostream& print(std::ostream& os) const override
    {
        return os << "Presentation Context ID: "
                  << std::to_string(pres_context_id) << "\n";
    }

      std::size_t msg_length;
      unsigned char pres_context_id;
      std::vector<unsigned char> command_set;
      std::vector<unsigned char> data_set;
};


/**
 * @brief The upperlayer_connection_property struct contains information about
 *        the offered / negotiated association with the remote application
 *        entity.
 *
 * @see DICOM3 standard, 3.8 .9.3
 */
struct a_associate_rq: property
{
      a_associate_rq() = default;

/**
 * @brief This parse function "deserializes" an a-associate-rq
 *        contained in the parameter pdu and returns a struct
 *        describing properties
 * @param[in] pdu
 *
 * The scope structure of this function resembles the structure
 * of the a-associate-rq pdu.
 * At the beginning of a scope the pos index always points to the
 * first byte of the item, ie the item type.
 */
    void from_pdu(std::vector<unsigned char> pdu) override
    {
        called_ae = std::string({pdu.begin()+10, pdu.begin()+26});
        calling_ae = std::string({pdu.begin()+26, pdu.begin()+42});

        std::size_t pos = 74;
        {
            assert(pdu[pos] == 0x10);
            std::size_t appl_cont_len = be_char_to_16b({pdu.begin()+pos+2, pdu.begin()+pos+4});
            application_context = std::string({pdu.begin()+pos+4, pdu.begin()+pos+4+appl_cont_len});

            pos += (4+appl_cont_len);

            // read presentation contexts
            while (pdu[pos] == 0x20) {
                unsigned char id;
                std::string abst_synt;
                std::vector<std::string> trans_synt;

                id = pdu[pos + 4];

                pos += 8;
                // read abstract syntax item (does not exist in a-associate-ac)
                if (pdu[pos] == 0x30)
                {
                    std::size_t abst_synt_len = be_char_to_16b({pdu.begin()+pos+2, pdu.begin()+pos+4});
                    abst_synt = std::string({pdu.begin()+pos+4, pdu.begin()+pos+4+abst_synt_len});
                    pos += abst_synt_len+4;
                }

                assert(pdu[pos] == 0x40);
                // read transfer syntaxes
                while (pdu[pos] == 0x40) {
                    assert(pdu[pos] == 0x40);
                    std::size_t trans_synt_len = be_char_to_16b({pdu.begin()+pos+2, pdu.begin()+pos+4});
                    trans_synt.push_back(std::string({pdu.begin()+pos+4, pdu.begin()+pos+4+trans_synt_len}));
                    pos += trans_synt_len+4;
                }

                presentation_context q {id, abst_synt, trans_synt};
                pres_contexts.push_back(q);
            }

            // read user info item
            {
                assert(pdu[pos] == 0x50);
                pos += 4;
                {
                    assert(pdu[pos] == 0x51);
                    max_message_length = be_char_to_32b({pdu.begin()+pos+4, pdu.begin()+pos+8});
                }
            }

        }
    }
      ;
    /**
 * @brief this function operates on the structured connection properties and
 *        deserializes it into a ready-to-transmit a-associate-rq or an
 *        a-associate-ac pdu
 * @param[in] t
 * @return pdu representing structured data
 */
    std::vector<unsigned char> make_pdu() const override
    {
        std::vector<uchar> pack;
        pack.push_back(static_cast<uchar>(TYPE::A_ASSOCIATE_RQ));
        pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}); // preamble
        std::size_t len_pos = 2;

        // insert called and calling ae
        assert(called_ae.size() == 16 && calling_ae.size() == 16);
        pack.insert(pack.end(), called_ae.begin(), called_ae.end());
        pack.insert(pack.end(), calling_ae.begin(), calling_ae.end());
        pack.insert(pack.end(), 32, 0x00);

        {
            // insert application context item
            pack.insert(pack.end(), {0x10, 0x00});
            std::vector<uchar> ac_len = ui_to_16b_be(application_context.size());
            pack.insert(pack.end(), ac_len.begin(), ac_len.end());
            pack.insert(pack.end(), application_context.begin(), application_context.end());

            // insert presentation context items
            for (const auto pc : pres_contexts) {
                pack.insert(pack.end(), {0x20, 0x00});

                // Calculating size of presentation context
                std::size_t pcl = 0;
                pcl += pc.abstract_syntax.size()+4; //+4 <=> preamble of abstract syntax item
                for (const auto ts : pc.transfer_syntaxes) {
                    pcl += ts.size()+4; //+4 <=> preamble of transfer syntax item
                }
                pcl += 4; //size of presentation context id info

                std::vector<uchar> pc_len = ui_to_16b_be(pcl);
                pack.insert(pack.end(), pc_len.begin(), pc_len.end());
                pack.insert(pack.end(), {pc.id, 0x00, 0x00, 0x00});

                {
                    // insert abstract syntax
                    pack.insert(pack.end(), {0x30, 0x00});
                    std::vector<uchar> as_len = ui_to_16b_be(pc.abstract_syntax.size());
                    pack.insert(pack.end(), as_len.begin(), as_len.end());
                    pack.insert(pack.end(), pc.abstract_syntax.begin(), pc.abstract_syntax.end());


                    for (const auto& ts : pc.transfer_syntaxes) {
                        pack.insert(pack.end(), {0x40, 0x00});
                        std::vector<uchar> ts_len = ui_to_16b_be(ts.size() ); //+4 for padding
                        pack.insert(pack.end(), ts_len.begin(), ts_len.end());
                        pack.insert(pack.end(), ts.begin(), ts.end());
                    }
                }
            }

            // insert user info item
            pack.insert(pack.end(), {0x50, 0x00});
            std::vector<uchar> ui_len = ui_to_16b_be(0x08);
            pack.insert(pack.end(), ui_len.begin(), ui_len.end());

            {
                // insert maximum length item
                pack.insert(pack.end(), {0x51, 0x00, 0x00, 0x04});
                std::vector<uchar> max_len = ui_to_32b_be(max_message_length);
                pack.insert(pack.end(), max_len.begin(), max_len.end());
            }
        }

        std::size_t pdu_len = pack.size()-6;
        std::vector<uchar> plen = ui_to_32b_be(pdu_len);
        std::copy(plen.begin(), plen.end(), pack.begin()+len_pos);

        return pack;
    }
    TYPE type() const override
    {
        return TYPE::A_ASSOCIATE_RQ;
    }
    std::ostream& print(std::ostream& os) const override
    {
        os << "Local Application Entity:\t" << called_ae << "\n"
           << "Remote Application Entity:\t" << calling_ae << "\n"
           << "Maximum Message Length:\t\t" << max_message_length << "\n"
           << "Application Context:\t" << application_context << "\n"
           << "Proposed presentation contexts:\n";
        for (const auto pc : pres_contexts) {
            os << "\tContext id: " << static_cast<unsigned>(pc.id) << "\n";
            os << "\tAbstract Syntax: " << pc.abstract_syntax << "\n";
            for (const auto ts : pc.transfer_syntaxes) {
                os << "\t\tTransfer Syntax: " << ts << "\n";
            }
        }
        return os;
    }


      std::string called_ae;
      std::string calling_ae;
      std::string application_context;

      struct presentation_context
      {
            presentation_context() = default;
            unsigned char id;
            std::string abstract_syntax;
            std::vector<std::string> transfer_syntaxes;
      };

      std::vector<presentation_context> pres_contexts;
      std::size_t max_message_length;
};

struct a_associate_ac: property
{
      a_associate_ac() = default;
    void from_pdu(std::vector<unsigned char> pdu) override
    {
        called_ae = std::string({pdu.begin()+10, pdu.begin()+26});
        calling_ae = std::string({pdu.begin()+26, pdu.begin()+42});

        std::size_t pos = 74;
        {
            assert(pdu[pos] == 0x10);
            std::size_t appl_cont_len = be_char_to_16b({pdu.begin()+pos+2, pdu.begin()+pos+4});
            application_context = std::string({pdu.begin()+pos+4, pdu.begin()+pos+4+appl_cont_len});

            pos += (4+appl_cont_len);

            // read presentation contexts
            while (pdu[pos] == 0x21) {
                unsigned char id;
                presentation_context::RESULT res;
                std::string trans_synt;

                id = pdu[pos + 4];
                res = static_cast<presentation_context::RESULT>(pdu[pos+6]);

                pos += 8;

                assert(pdu[pos] == 0x40);
                // read transfer syntax
                std::size_t trans_synt_len = be_char_to_16b({pdu.begin()+pos+2, pdu.begin()+pos+4});
                trans_synt = std::string({pdu.begin()+pos+4, pdu.begin()+pos+4+trans_synt_len});
                pos += trans_synt_len+4;

                presentation_context q {id, res, trans_synt};
                pres_contexts.push_back(q);
            }

            // read user info item
            {
                assert(pdu[pos] == 0x50);
                pos += 4;
                {
                    assert(pdu[pos] == 0x51);
                    max_message_length = be_char_to_32b({pdu.begin()+pos+4, pdu.begin()+pos+8});
                }
            }

        }
    }
      std::vector<unsigned char> make_pdu() const override
    {
        std::vector<uchar> pack;
        pack.push_back(static_cast<uchar>(TYPE::A_ASSOCIATE_AC));
        pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}); // preamble
        std::size_t len_pos = 2;

        // insert called and calling ae
        assert(called_ae.size() == 16 && calling_ae.size() == 16);
        pack.insert(pack.end(), called_ae.begin(), called_ae.end());
        pack.insert(pack.end(), calling_ae.begin(), calling_ae.end());
        pack.insert(pack.end(), 32, 0x00);

        {
            // insert application context item
            pack.insert(pack.end(), {0x10, 0x00});
            std::vector<uchar> ac_len = ui_to_16b_be(application_context.size());
            pack.insert(pack.end(), ac_len.begin(), ac_len.end());
            pack.insert(pack.end(), application_context.begin(), application_context.end());

            // insert presentation context items
            for (const auto pc : pres_contexts) {
                pack.insert(pack.end(), {0x21, 0x00});

                // Calculating size of presentation context
                std::size_t pcl = pc.transfer_syntax.size();

                std::vector<uchar> pc_len = ui_to_16b_be(pcl +4 +4); // 4 bytes padding (pc and ts)
                pack.insert(pack.end(), pc_len.begin(), pc_len.end());
                pack.insert(pack.end(), {pc.id, 0x00, static_cast<uchar>(pc.result_), 0x00});

                {
                    pack.insert(pack.end(), {0x40, 0x00});
                    std::vector<uchar> ts_len = ui_to_16b_be(pc.transfer_syntax.size() );
                    pack.insert(pack.end(), ts_len.begin(), ts_len.end());
                    pack.insert(pack.end(), pc.transfer_syntax.begin(), pc.transfer_syntax.end());
                }
            }

            // insert user info item
            pack.insert(pack.end(), {0x50, 0x00});
            std::vector<uchar> ui_len = ui_to_16b_be(0x08);
            pack.insert(pack.end(), ui_len.begin(), ui_len.end());

            {
                // insert maximum length item
                pack.insert(pack.end(), {0x51, 0x00, 0x00, 0x04});
                std::vector<uchar> max_len = ui_to_32b_be(max_message_length);
                pack.insert(pack.end(), max_len.begin(), max_len.end());
            }
        }

        std::size_t pdu_len = pack.size()-6;
        std::vector<uchar> plen = ui_to_32b_be(pdu_len);
        std::copy(plen.begin(), plen.end(), pack.begin()+len_pos);

        return pack;
    }
    TYPE type() const override
    {
        return TYPE::A_ASSOCIATE_AC;
    }
    std::ostream& print(std::ostream& os) const override
    {
        os << "Local Application Entity:\t" << called_ae << "\n"
           << "Remote Application Entity:\t" << calling_ae << "\n"
           << "Maximum Message Length:\t\t" << max_message_length << "\n"
           << "Application Context:\t" << application_context << "\n"
           << "Status of proposed presentation contexts:\n";
        for (const auto pc : pres_contexts) {
            os << "\tContext id: " << static_cast<unsigned>(pc.id);
            os << "\tResult: " << pc.result_ << "\n";
            os << "\t\tTransfer Syntax: " << pc.transfer_syntax << "\n";
        }
        return os;
    }

      std::string called_ae;
      std::string calling_ae;
      std::string application_context;

      struct presentation_context
      {
            enum class RESULT
            {
               ACCEPTANCE = 0x00, USER_REJEC = 0x01,
               PROV_REJEC_NO_REASON = 0x02,
               ABSTR_CONT_NOT_SUPP = 0x03,
               TRANSF_SYNT_NOT_SUPP = 0x04
            };

            presentation_context() = default;
            presentation_context(unsigned char id, RESULT res, std::string ts) : id(id), result_(res), transfer_syntax(ts) {}
            unsigned char id;
            RESULT result_;
            std::string transfer_syntax;
      };

      std::vector<presentation_context> pres_contexts;
      std::size_t max_message_length;
};

//std::ostream& operator<<(std::ostream& os, a_associate_ac::presentation_context::RESULT r);

struct a_associate_rj: property
{
      a_associate_rj() = default;
      void from_pdu(std::vector<unsigned char> pdu) override
      {
          source_ = static_cast<SOURCE>(pdu[8]);
          reason_ = static_cast<REASON>(pdu[9]);
      }
      std::vector<unsigned char> make_pdu() const override
      {
          std::vector<uchar> pack;
          pack.push_back(static_cast<uchar>(TYPE::A_ASSOCIATE_RJ));
          pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01});
          pack.push_back(static_cast<uchar>(source_));
          pack.push_back(static_cast<uchar>(reason_));
          return pack;
      }
      TYPE type() const override
      {
          return TYPE::A_ASSOCIATE_RJ;
      }
      std::ostream& print(std::ostream& os) const override
      {
          auto sr = std::pair<SOURCE, REASON> {source_, reason_};
          os << "Source: " << source_ << "\n"
             << "Reason: " << sr << "\n";
          return os;
      }

      enum class SOURCE : unsigned char
      {
         UL_SERVICE_USER = 0x01, UL_SERVICE_PROV_ACSE = 0x02,
         UL_SERVICE_PROV_PRESREL = 0x03
      };
      enum class REASON : unsigned char
      {
         NOT_SPECIFIED = 0x01, APPL_CONT_NOT_SUPP = 0x02, CALLING_AE_NOT_RECOG = 0x03,
         CALLED_AE_NOT_RECOG = 0x07
      };

      REASON reason_;
      SOURCE source_;
};

std::ostream& operator<<(std::ostream& os, a_associate_rj::SOURCE s);
std::ostream& operator<<(std::ostream& os,
                         std::pair<a_associate_rj::SOURCE, a_associate_rj::REASON> sr);

struct a_release_rq: property
{
      a_release_rq() = default;
    void from_pdu(std::vector<unsigned char> pdu) override
    {
    }
      std::vector<unsigned char> make_pdu() const override
    {
        std::vector<uchar> pack;
        pack.push_back(static_cast<uchar>(TYPE::A_RELEASE_RQ));
        pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00});
        return pack;
    }

    TYPE type() const override
    {
        return TYPE::A_RELEASE_RQ;
    }
    std::ostream& print(std::ostream& os) const override
    {
        return os;
    }
};

struct a_release_rp: property
{
      a_release_rp() = default;
    void from_pdu(std::vector<unsigned char> pdu) override
    {
    }
      std::vector<unsigned char> make_pdu() const override
    {
        std::vector<uchar> pack;
        pack.push_back(static_cast<uchar>(TYPE::A_RELEASE_RP));
        pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00});
        return pack;
    }
    TYPE type() const override
    {
        return TYPE::A_RELEASE_RP;
    }
    std::ostream& print(std::ostream& os) const override
    {
        return os;
    }
};

struct a_abort: property
{
      a_abort() = default;
    void from_pdu(std::vector<unsigned char> pdu) override
    {
        source_ = static_cast<SOURCE>(pdu[8]);
        reason_ = static_cast<REASON>(pdu[9]);
    }
      std::vector<unsigned char> make_pdu() const override
    {
        std::vector<uchar> pack;
        pack.push_back(static_cast<uchar>(TYPE::A_ABORT));
        pack.insert(pack.end(), {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x01});
        pack.push_back(static_cast<uchar>(source_));
        pack.push_back(static_cast<uchar>(reason_));
        return pack;
    }

    TYPE type() const override
    {
        return TYPE::A_ABORT;
    }
    std::ostream& print(std::ostream& os) const override
    {
        os << "Source: " << static_cast<unsigned>(source_) << "\n"
           << "Reason: " << static_cast<unsigned>(reason_) << "\n";
        return os;
    }

      enum class SOURCE : unsigned char
      {
         UL_SERVICE_USER = 0x01,
         UL_SERVICE_PROV = 0x03
      };
      enum class REASON : unsigned char
      {
         NOT_SPECIFIED = 0x00, UNRECOG_PDU = 0x01,
         UNEXPEC_PDU = 0x02,
         UNRECOG_PDU_PARAM = 0x04,
         UNEXPEC_PDU_PARAM = 0x05,
         INVALID_PDU_PARAM = 0x06
      };

      SOURCE source_;
      REASON reason_;
};


std::unique_ptr<property> make_property(const std::vector<unsigned char>& pdu)
{
    auto ptype = get_type(pdu);
    switch (ptype) {
    case TYPE::A_ABORT: {
        auto a = new a_abort();
        a->from_pdu(pdu);
        return std::unique_ptr<a_abort>(a);
    }
    case TYPE::A_RELEASE_RQ: {
        auto a = new a_release_rq();
        a->from_pdu(pdu);
        return std::unique_ptr<a_release_rq>(a);
    }
    case TYPE::A_RELEASE_RP: {
        auto a = new a_release_rp();
        a->from_pdu(pdu);
        return std::unique_ptr<a_release_rp>(a);
    }
    case TYPE::A_ASSOCIATE_RQ: {
        auto a = new a_associate_rq();
        a->from_pdu(pdu);
        return std::unique_ptr<a_associate_rq>(a);
    }
    case TYPE::A_ASSOCIATE_AC: {
        auto a = new a_associate_ac();
        a->from_pdu(pdu);
        return std::unique_ptr<a_associate_ac>(a);
    }
    case TYPE::A_ASSOCIATE_RJ: {
        auto a = new a_associate_rj();
        a->from_pdu(pdu);
        return std::unique_ptr<a_associate_rj>(a);
    }
    case TYPE::P_DATA_TF: {
        auto p = new p_data_tf();
        p->from_pdu(pdu);
        return std::unique_ptr<p_data_tf>(p);
    }
    }
    return nullptr;
}

std::ostream& operator<<(std::ostream& os, const property& p)
{
    return p.print(os);
}

std::ostream& operator<<(std::ostream& os, TYPE t)
{
    switch (t) {
    case TYPE::A_ABORT:
        return os << "a_abort";
    case TYPE::A_ASSOCIATE_AC:
        return os << "a_associate_ac";
    case TYPE::A_ASSOCIATE_RQ:
        return os << "a_associate_rq";
    case TYPE::A_ASSOCIATE_RJ:
        return os << "a_associate_rj";
    case TYPE::A_RELEASE_RQ:
        return os << "a_release_rq";
    case TYPE::A_RELEASE_RP:
        return os << "a_release_rp";
    case TYPE::P_DATA_TF:
        return os << "p_data_tf";
    default:
        assert(false);
    }
}

std::ostream& operator<<(std::ostream& os, a_associate_ac::presentation_context::RESULT r)
{
    using pc = a_associate_ac::presentation_context;
    switch (r) {
    case pc::RESULT::ACCEPTANCE:
        return os << "Acceptance";
    case pc::RESULT::ABSTR_CONT_NOT_SUPP:
        return os << "Abstract Syntax not supported";
    case pc::RESULT::PROV_REJEC_NO_REASON:
        return os << "Provider rejection, no reason";
    case pc::RESULT::TRANSF_SYNT_NOT_SUPP:
        return os << "Transfer Syntax not supported";
    case pc::RESULT::USER_REJEC:
        return os << "User rejection";
    default:
        assert(false);
    }
}

std::ostream& operator<<(std::ostream& os, a_associate_rj::SOURCE s)
{
    using rj = a_associate_rj;
    switch (s) {
    case rj::SOURCE::UL_SERVICE_USER:
        return os << "DICOM UL service-user";
    case rj::SOURCE::UL_SERVICE_PROV_ACSE:
        return os << "DICOM UL service-provider (ACSE related function)";
    case rj::SOURCE::UL_SERVICE_PROV_PRESREL:
        return os << "DICOM UL service-provider (Presentation related function)";
    default:
        assert(false);
    }
}

std::ostream& operator<<(std::ostream& os, std::pair<a_associate_rj::SOURCE, a_associate_rj::REASON> sr)
{
    using rj = a_associate_rj;
    if (sr.first == rj::SOURCE::UL_SERVICE_USER) {
        if (static_cast<int>(sr.second) == 1) {
            return os << "no-reason-given";
        } else if (static_cast<int>(sr.second) == 2) {
            return os << "application-context-name-not-supported";
        } else if (static_cast<int>(sr.second) == 3) {
            return os << "calling-AE-title-not-recognized";
        } else if (static_cast<int>(sr.second) == 7) {
            return os << "called-AE-title-not-recognized";
        } else {
            return os << ""; //reserved
        }
    } else if (sr.first == rj::SOURCE::UL_SERVICE_PROV_ACSE) {
        if (static_cast<int>(sr.second) == 1) {
            return os << "no-reason-given";
        } else if (static_cast<int>(sr.second) == 2) {
            return os << "protocol-version-not-supported";
        } else {
            return os << "";
        }
    } else if (sr.first == rj::SOURCE::UL_SERVICE_PROV_PRESREL) {
        if (static_cast<int>(sr.second) == 1) {
            return os << "temporary-congestion";
        } else if (static_cast<int>(sr.second) == 2) {
            return os << "local-limit-exceeded";
        } else {
            return os << "";
        }
    } else {
        assert(false);
    }
}


}

}

}

#endif // DICOM_UPPERLAYER_CONNECTION_PROPERTIES_HPP
