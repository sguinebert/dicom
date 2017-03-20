#ifndef QUERYRETRIEVE_SCP_HPP
#define QUERYRETRIEVE_SCP_HPP

#include <string>
#include <functional>

#include "network/upperlayer/upperlayer.hpp"
#include "network/dimse/dimse_pm.hpp"
#include "network/dimse/sop_class.hpp"
#include "network/dimse/association_definition.hpp"
#include "storage_scu.hpp"

class queryretrieve_scp
{
   public:
      queryretrieve_scp(std::string calling_ae, std::string called_ae,
                        std::size_t max_message_len, dicom::data::dictionary::dictionary& dict,
                        std::function<void(queryretrieve_scp*, dicom::data::dataset::commandset_data, std::unique_ptr<dicom::data::dataset::iod>)> handler);

      void start_listen();

      dicom::network::upperlayer::scp& get_scp();

      void send_image(dicom::data::dataset::iod data);

   private:
      void cstore_callback(storage_scu* st, dicom::data::dataset::commandset_data cmd, std::unique_ptr<dicom::data::dataset::iod> data);

      void handle_cfind(dicom::network::dimse::dimse_pm* pm,
                        dicom::data::dataset::commandset_data command,
                        std::unique_ptr<dicom::data::dataset::iod> data);

      //storage_scu cstore;

      dicom::data::dictionary::dictionary& dict;
      dicom::network::dimse::SOP_class sop_class;
      dicom::network::dimse::association_definition assoc_def;
      dicom::network::upperlayer::scp scp;
      dicom::network::dimse::dimse_pm_manager dimse_pm;

      std::function<void(queryretrieve_scp*, dicom::data::dataset::commandset_data, std::unique_ptr<dicom::data::dataset::iod>)> handler;


};

#endif // QUERYRETRIEVE_SCP_HPP
