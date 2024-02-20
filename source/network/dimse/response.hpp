#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "boost/optional.hpp"

#include "data/dataset/datasets.hpp"

namespace dicom
{

namespace network
{

namespace dimse
{

/**
 * @brief The response class is used by the user-specified handlers for each
 *        Service Group used in the SOP to prompt the dimse protocol machine
 *        to send a response.
 */
class response
{
   public:
      /**
       * @brief response constructor for the response
       * @param dsg service group of the response
       * @param data optional; pointer owning response iod data
       * @param status optional; status indication, SUCCESS by default
       * @param prio optional; priority of the response, MEDIUM by default
       */
       response(data::dataset::DIMSE_SERVICE_GROUP dsg,
                data::dataset::commandset_data last_command,
                boost::optional<data::dataset::iod> data = boost::none,
                data::dataset::STATUS status = data::dataset::STATUS::SUCCESS,
                data::dataset::DIMSE_PRIORITY prio = data::dataset::DIMSE_PRIORITY::MEDIUM
                ):
           response_type {dsg},
           last_command {last_command},
           data {data},
           status {status},
           prio {prio}

       {
       }

       data::dataset::DIMSE_SERVICE_GROUP get_response_type() const
       {
           return response_type;
       }
      const data::dataset::commandset_data& get_command() const
      {
          return last_command;
      }
      const boost::optional<data::dataset::iod>& get_data() const
      {
          return data;
      }
      data::dataset::STATUS get_status() const
      {
          return status;
      }
      data::dataset::DIMSE_PRIORITY get_priority() const
      {
          return prio;
      }

   private:
      const data::dataset::DIMSE_SERVICE_GROUP response_type;
      const data::dataset::commandset_data last_command;
      const boost::optional<data::dataset::iod> data;
      const data::dataset::STATUS status;
      const data::dataset::DIMSE_PRIORITY prio;
};

std::ostream& operator<<(std::ostream& os, const response& r)
{
    auto rt = r.get_response_type();
    auto pr = r.get_priority();
    auto st = r.get_status();
    return os << "Service Group: \t" << rt << std::hex << "\t(" << static_cast<unsigned>(rt) << ")\n"
              << "Priority: \t\t" << pr << "\t(" << static_cast<unsigned>(pr) << ")\n"
              << "Status: \t\t" << st << "\t(" << static_cast<unsigned>(st) << ")\n";
}


}

}

}

#endif // RESPONSE_HPP
