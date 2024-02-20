#include "association_definition.hpp"

#include <algorithm>

namespace dicom
{

namespace network
{

namespace dimse
{


// std::vector<association_definition::presentation_context>
//    association_definition::get_SOP_class(std::string abstract_syntax) const
// {
//    std::vector<presentation_context> result_set;
//    for (auto pc : supported_sops) {
//       std::string s = pc.sop_class.get_SOP_class_UID();
//       if (s == std::string(abstract_syntax.c_str())) {
//          result_set.push_back(pc);
//       }
//    }
//    return result_set;
// }

// std::vector<association_definition::presentation_context>
//    association_definition::get_all_SOP() const
// {
//    return supported_sops;
// }

// std::vector<association_definition::presentation_context>&
//    association_definition::get_all_SOP()
// {
//    return supported_sops;
// }



// std::vector<association_definition::presentation_context> make_presentation_contexts(std::vector<SOP_class> sop_classes,
//       std::initializer_list<std::string> transfer_syntaxes,
//       association_definition::DIMSE_MSG_TYPE msg_type)
// {
//    std::vector<association_definition::presentation_context> pcs;
//    pcs.reserve(sop_classes.size());
//    for (auto sop : sop_classes) {
//       pcs.emplace_back(sop, transfer_syntaxes, msg_type);
//    }
//    return pcs;
// }

// std::vector<association_definition::presentation_context> operator+(std::vector<association_definition::presentation_context> a,
//                                                                     std::vector<association_definition::presentation_context> b)
// {
//    for (const auto rs : b) {
//       a.emplace_back(rs);
//    }
//    return a;
// }


}

}

}

