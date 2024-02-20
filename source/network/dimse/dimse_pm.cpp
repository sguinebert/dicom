#include "dimse_pm.hpp"

#include <memory>
#include <string>
#include <initializer_list>
#include <functional>
#include <iostream>
#include <numeric>

#include "network/upperlayer/upperlayer_properties.hpp"
#include "network/upperlayer/upperlayer.hpp"

#include "data/dataset/datasets.hpp"
#include "data/dataset/dataset_iterator.hpp"

#include "data/attribute/constants.hpp"


namespace dicom
{

namespace network
{

namespace dimse
{

using namespace data::attribute;
using namespace data::dictionary;
using namespace data::dataset;
using namespace util::log;
























































// transfer_processor& dimse_pm::find_transfer_processor(unsigned char presentation_context_id)
// {
//    using kvpair = std::pair<const std::string, std::unique_ptr<data::dataset::transfer_processor>>;


//    // retrieve the negotiated transfer syntax of the presentation context
//    auto pres_contexts = connection_properties.get().pres_contexts;
//    std::string ts_of_presentation_context = (std::find_if(pres_contexts.begin(), pres_contexts.end(),
//       [presentation_context_id](upperlayer::a_associate_ac::presentation_context pc) { return pc.id == presentation_context_id; })
//          )->transfer_syntax;

//    // and return a reference to our corresponding transfer processor instance
// //   return *((std::find_if(transfer_processors.begin(), transfer_processors.end(),
// //      [this, ts_of_presentation_context](kvpair& kv) { return ts_of_presentation_context == kv.first; }))->second);

//    // we might as well throw if there is no such transfer processor for the
//    // syntax, association should not have been accepted in the first place
//    return *transfer_processors[ts_of_presentation_context];
// }















































}

}

}
