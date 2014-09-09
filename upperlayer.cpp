#include "upperlayer.hpp"

#include <map>
#include <utility>
#include <vector>
#include <algorithm>
#include <ostream>
#include <cassert>
#include <functional>
#include <initializer_list>

#include <boost/asio.hpp>

#include "upperlayer_properties.hpp"
#include "upperlayer_statemachine.hpp"


namespace upperlayer
{
namespace
{
using uchar = unsigned char;

std::size_t be_char_to_16b(std::vector<uchar> bs)
{
   assert(bs.size() == 2);
   std::size_t sz = 0;
   sz |= (static_cast<std::size_t>(bs[0]) << 8);
   sz |= (static_cast<std::size_t>(bs[1]));
   return sz;
}
std::size_t be_char_to_32b(std::vector<uchar> bs)
{
   assert(bs.size() == 4);
   std::size_t sz = 0;
   sz |= (static_cast<std::size_t>(bs[0]) << 24);
   sz |= (static_cast<std::size_t>(bs[1]) << 16);
   sz |= (static_cast<std::size_t>(bs[2]) << 8);
   sz |= (static_cast<std::size_t>(bs[3]));
   return sz;
}

}



scx::scx(std::initializer_list<std::pair<TYPE, std::function<void(scx*, std::unique_ptr<property>)>>> l):
   statem {},
   handlers {}
{
   for (const auto p : l) {
      handlers[p.first] = p.second;
   }
}

scx::~scx()
{
}


void scx::send(property* p)
{
   auto pdu = p->make_pdu();
   auto ptype = get_type(pdu);

   statemachine::EVENT e;
   switch (ptype) {
      case TYPE::A_ABORT:
         e = statemachine::EVENT::LOCL_A_ABORT_PDU;
         break;
      case TYPE::A_ASSOCIATE_AC:
         e = statemachine::EVENT::LOCL_A_ASSOCIATE_AC_PDU;
         break;
      case TYPE::A_ASSOCIATE_RJ:
         e = statemachine::EVENT::LOCL_A_ASSOCIATE_RJ_PDU;
         break;
      case TYPE::A_ASSOCIATE_RQ:
         e = statemachine::EVENT::LOCL_A_ASSOCIATE_RJ_PDU;
         break;
      case TYPE::A_RELEASE_RQ:
         e = statemachine::EVENT::LOCL_A_RELEASE_RQ_PDU;
         break;
      case TYPE::A_RELEASE_RP:
         e = statemachine::EVENT::LOCL_A_RELEASE_RP_PDU;
         break;
      case TYPE::P_DATA_TF:
         e = statemachine::EVENT::LOCL_P_DATA_TF_PDU;
         break;
      default:
         assert(false);
   }

   if (statem.transition(e) != statemachine::CONN_STATE::INV) {
      boost::asio::async_write(sock(), boost::asio::buffer(pdu),
         [=](const boost::system::error_code& error, std::size_t bytes) { }
      );
   }
}

void scx::do_read()
{
   auto size = std::make_shared<std::vector<unsigned char>>(6);
   auto rem_data = std::make_shared<std::vector<unsigned char>>();
   auto compl_data = std::make_shared<std::vector<unsigned char>>();
   boost::asio::async_read(sock(), boost::asio::buffer(*size), boost::asio::transfer_exactly(6),
      [=](const boost::system::error_code& error, std::size_t bytes)  {
         assert(bytes == 6);

         std::size_t len = be_char_to_32b({size->begin()+2, size->begin()+6});
         rem_data->resize(len);
         boost::asio::async_read(sock(), boost::asio::buffer(*rem_data), boost::asio::transfer_exactly(len),
            [=](const boost::system::error_code& error, std::size_t bytes) {

               compl_data->reserve(size->size() + rem_data->size());
               compl_data->insert(compl_data->end(), size->begin(), size->end());
               compl_data->insert(compl_data->end(), rem_data->begin(), rem_data->end());
               auto ptype = get_type(*compl_data);

               statemachine::EVENT e;
               switch (ptype) {
                  case TYPE::A_ABORT:
                     e = statemachine::EVENT::RECV_A_ABORT_PDU;
                     break;
                  case TYPE::A_ASSOCIATE_AC:
                     e = statemachine::EVENT::RECV_A_ASSOCIATE_AC_PDU;
                     break;
                  case TYPE::A_ASSOCIATE_RJ:
                     e = statemachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU;
                     break;
                  case TYPE::A_ASSOCIATE_RQ:
                     e = statemachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU;
                     break;
                  case TYPE::A_RELEASE_RQ:
                     e = statemachine::EVENT::RECV_A_RELEASE_RQ_PDU;
                     break;
                  case TYPE::A_RELEASE_RP:
                     e = statemachine::EVENT::RECV_A_RELEASE_RP_PDU;
                     break;
                  case TYPE::P_DATA_TF:
                     e = statemachine::EVENT::RECV_P_DATA_TF_PDU;
                     break;
                  default:
                     assert(false);
               }
               statem.transition(e);

               // call appropriate handler
               if (statem.process_next) {
                  handlers[ptype](this, make_property(*compl_data));
               } else {
                  statem.process_next = true; //reset
               }

               // ? revisit
               if (get_state() == statemachine::CONN_STATE::STA13) {
                  io_s().stop();
                  return;
               }

               // be ready for new incoming data
               if (get_state() != statemachine::CONN_STATE::STA2) {
                  do_read();
               }
            }
         );
      }
   );
}

void scx::run()
{
   io_s().run();
}



void scx::inject(TYPE t, std::function<void (scx*, std::unique_ptr<property>)> f)
{
   handlers[t] = f;
}

statemachine::CONN_STATE scx::get_state()
{
   return statem.get_state();
}


scp::scp(short port, std::initializer_list<std::pair<TYPE, std::function<void(scx*, std::unique_ptr<property>)>>> l):
   scx {l},
   io_service(),
   socket(io_service),
   acptr(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
   acptr.async_accept(socket, [=](boost::system::error_code ec) {
         if (!ec) {
            statem.transition(statemachine::EVENT::TRANS_CONN_INDIC);
            do_read();
         }
      } );
}

scu::scu(std::string host, std::string port, std::initializer_list<std::pair<TYPE, std::function<void(scx*, std::unique_ptr<property>)>>> l):
   scx{l},
   io_service(),
   resolver(io_service),
   query(host, port),
   endpoint_iterator(resolver.resolve(query)),
   socket(io_service)
{
   statem.transition(statemachine::EVENT::A_ASSOCIATE_RQ);
   boost::asio::ip::tcp::resolver::iterator end;
   boost::system::error_code error = boost::asio::error::host_not_found;
   while(error && endpoint_iterator != end)
   {
     socket.close();
     socket.connect(*endpoint_iterator++, error);
   }
   statem.transition(statemachine::EVENT::TRANS_CONN_CONF);
   assert(!error);
}

boost::asio::ip::tcp::socket&scp::sock()
{
   return socket;
}

boost::asio::io_service&scp::io_s()
{
   return io_service;
}

scp::~scp()
{
   switch (get_state()) {
      case statemachine::CONN_STATE::STA2:
         break;
      default: {
         a_abort ab;
         boost::asio::write(sock(), boost::asio::buffer(ab.make_pdu()));
      }
   }
}

boost::asio::ip::tcp::socket& scu::sock()
{
   return socket;
}

boost::asio::io_service& scu::io_s()
{
   return io_service;
}

scu::~scu()
{
   switch (get_state()) {
      case statemachine::CONN_STATE::STA2:
         break;
      default: {
         a_abort ab;
         boost::asio::write(sock(), boost::asio::buffer(ab.make_pdu()));
      }
   }
}

}
