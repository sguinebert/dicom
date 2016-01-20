#include "sop_class.hpp"

SOP_class::SOP_class(std::string SOP_class_UID, std::map<DIMSE_SERVICE_GROUP
                     , std::function<void(std::unique_ptr<iod> data)>> handler):
   sop_uid {SOP_class_UID}, operations {handler}
{
}

void SOP_class::operator()(DIMSE_SERVICE_GROUP op, std::unique_ptr<iod> data) const
{
   operations.at(op)(std::move(data));
}

const char*SOP_class::get_SOP_class_UID() const
{
      return sop_uid.c_str();
}
