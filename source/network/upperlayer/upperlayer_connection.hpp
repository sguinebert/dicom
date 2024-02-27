#ifndef DICOM_UPPERLAYER_HPP
#define DICOM_UPPERLAYER_HPP

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <initializer_list>
#include <utility>
#include <deque>

#include <boost/optional.hpp>
#include <boost/log/trivial.hpp>

#include "infrastructure/asio_tcp_connection.hpp"
#include "upperlayer_properties.hpp"
#include "upperlayer_statemachine.hpp"
#include "infrastructure/asio_tcp_connection.hpp"

#include "data/dataset/transfer_processor.hpp"

#include "util/channel_sev_logger.hpp"

using namespace dicom::util::log;
using namespace std::literals::chrono_literals;
using std::chrono::steady_clock;

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
/**
 * The dicom namespace is the global namespace of the project, containing all
 * DICOM related implementations.
 */
namespace dicom
{

/**
 * The network namespace contains all DICOM network-related functionality, like
 * the implementation of the upperlayer or the dimse protocol machine.
 */
namespace network
{

/**
 * The upperlayer namespace contains classes and functions which implement the upperlayer as
 * described in the DICOM standard chapter 8.
 * @link http://medical.nema.org/Dicom/2011/11_08pu.pdf
 */
namespace upperlayer
{

class Connection;

/**
 * @brief The Istate_trans_ops struct is an interface which the statemachine class uses to
 *        perform certain operations, like resetting the ARTIM timer or sending an a_abort
 */
struct Istate_trans_ops
{
      virtual void reset_artim() = 0;
      virtual void stop_artim() = 0;
      virtual void start_artim() = 0;
      virtual void ignore_next() = 0;
      virtual void queue_for_write_w_prio(std::unique_ptr<property> p) = 0;
      virtual void close_connection() = 0;
      virtual ~Istate_trans_ops() = 0;
};

/**
 * @brief The Iupperlayer_comm_ops struct is an interface used for writing properties and
 *        injecting handlers for received properties
 */
struct Iupperlayer_comm_ops
{
      virtual void queue_for_write(std::unique_ptr<property> p) = 0;
      virtual void inject(TYPE t, std::function<void(Iupperlayer_comm_ops*, std::unique_ptr<property>)> f) = 0;
      virtual void inject_conf(TYPE t, std::function<void(Iupperlayer_comm_ops*, property* f)>) = 0;
      virtual ~Iupperlayer_comm_ops() = 0;
};


/**
 * @brief The scx class implements basic functionality used both by the specialed scp and scu
 *        subclasses, like reading and writing to the connected peer. It also manages the
 *        state machine
 * @todo  write reconnect() method using unique_ptr<socket>
 *        inherit from a service class interface
 *
 * upperlayer::scx provides send() and read() functions independetly if the
 * subclass for clients (scu) or servers (scp) is used.
 * It must be noted that the sole responsibility of the upperlayer::scx is to
 * keep the state machine valid. It is not involved in the association
 * negotiation. This has to be done by the user of this class (either a facade
 * or the DIMSE_PM).
 */
class Connection: public Istate_trans_ops, public Iupperlayer_comm_ops, public connection
{
   public:
    // explicit Connection(data::dictionary::dictionaries& dict,
    //                     std::function<void(Iupperlayer_comm_ops*, std::exception_ptr)> on_error,
    //                     std::vector<std::pair<TYPE, std::function<void(Connection*, std::unique_ptr<property>)>>> l):
    //     statem {this, send_queue},
    //     error_handler {on_error},
    //     logger {"upperlayer"},
    //     proc {data::dataset::commandset_processor {dict}},
    //     received_pdu {boost::none},
    //     handlers {},
    //     shutdown_requested {false}
    // {
    //     for (const auto p : l) {
    //         handlers[p.first] = p.second;
    //     }
    // }


    explicit Connection(asio::io_context ctx, data::dictionary::dictionaries& dict,
                        std::function<void(Iupperlayer_comm_ops*, std::exception_ptr)> on_error,
                        std::vector<std::pair<TYPE, std::function<void(Connection*, std::unique_ptr<property>)>>> l):
        connection(ctx),
        statem {this, send_queue},
        error_handler {on_error},
        logger {"upperlayer"},
        proc {data::dataset::commandset_processor {dict}},
        received_pdu {boost::none},
        handlers {},
        shutdown_requested {false}
    {
        for (const auto& p : l) {
            handlers[p.first] = p.second;
        }
    }


    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    virtual ~Connection()
    {
        // closing of the connection may only be done when there are no
        // outstanding operations
    }

      /**
       * @brief run blocks until asynchronous operations are completed
       */
      void run();

      /**
       * @brief get_state returns the current state
       * @return
       * @callgraph
       */
      StateMachine::CONN_STATE get_state()
      {
          return statem.get_state();
      }


      /**
       * @brief inject sets a handler for a received property type t.
       * @param[in] t
       * @param[in] f
       */
      void inject(TYPE t, std::function<void(Iupperlayer_comm_ops*, std::unique_ptr<property>)> f) override
      {
          handlers[t] = f;
      }

      /**
       * @brief inject_conf sets a handler for a sent property type t.
       * @param[in] t
       * @param[in] f
       */
      void inject_conf(TYPE t, std::function<void(Iupperlayer_comm_ops*, property*)> f) override
      {
          handlers_conf[t] = f;
      }

      /**
       * @brief queue_for_write takes ownership of a property and queues it for
       *        writing
       * @param[in] p
       *
       * A write queue is necessary to prevent multiple writes to the socket, which
       * may result in interleaving
       */
      void queue_for_write(std::unique_ptr<property> p) override
      {
          // when send_queue.size() is greater than 1, there are still properties being
          // written by scx::send(). To prevent interleaving, we do not call send here
          // and just leave the property in the queue
          send_queue.emplace_back(std::move(p));
          if (send_queue.size() > 1) {
              return;
          }
          send(send_queue.front().get());
      }

      /**
       * @brief queue_for_write_w_prio queues a property for writing, but moves it
       *        in the front of the queue so it will be retrieved first
       * @param[in] p
       * @see queue_for_write();
       */
      void queue_for_write_w_prio(std::unique_ptr<property> p) override
      {
          // see scx::queue_for_write for explanation
          send_queue.emplace_front(std::move(p));
          if (send_queue.size() > 1) {
              return;
          }
          send(send_queue.front().get());
      }

      /**
       * @brief reset_artim resets the artim timer
       */
      void reset_artim() override
      {
          stop_artim();
          start_artim();
      }

      /**
       * @brief stop_artim stops the artim timer
       */
      void stop_artim() override
      {
          artim_timer()->cancel();
      }

      /**
       * @brief start_artim starts the artim timer
       */
      void start_artim() override
      {
          using namespace std::placeholders;
          artim_timer()->wait_async();
              //member function artim_expired has implicit scx* as first parameter
      }

      /**
       * @brief ignore_next indicates whether a received pdu shall be ignored by
       *        not invoking the respective handler
       */
      void ignore_next() override
      {
          received_pdu = boost::none;
          assert(!received_pdu.is_initialized());
      }

      /**
       * @brief close_connection aborts all async operations on the io_service object,
       *        making io_service::run() terminate
       */
      void close_connection() override
      {
          statem.transition(StateMachine::EVENT::TRANS_CONN_CLOSED);

          shutdown_requested = true;

          // closing of the connection may only be done when there are no
          // outstanding operations

          connection()->close();

          // TODO: update shutdown_requested

          //   io_s().post([this]() {
          //      sock().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
          //      sock().close();
          //      handler_end_connection(this);
          //      shutdown_requested = false;
          //   });
      }


   protected:
      StateMachine statem;

      /**
       * @brief Connection::do_read reads a pdu asynchronously from the peer
       *
       * The pdu is received in two steps:
       * The first async_read is passed a handler which is called when
       * exactly six bytes are received. These first six bytes contain
       * the size of the pdu _len_.
       * Then, in this first read-handler, another async_read is
       * dispatched which calls its read-handler when exactly _len_
       * bytes (ie the whole pdu) are received. This second read-handler
       * contains the code which actually processes the pdu (calls the
       * appropriate handler, manages state transitions, ...)
       */
      asio::awaitable<void> do_read()
      {
          // because the async operations terminate immediately the containers would go out of scope
          // and async_write would write into "unallocated" memory. To prevent this, reference-counting
          // shared_ptrs are used, which are captured, copied and therefore held alive by the lambda
          // passed to the read handler.
          // There may only be one async_read at a time. This is ensured by calling async_reads _only_
          // in the read handlers, i.e. when the previous read has completed.
          auto size = std::make_shared<std::vector<unsigned char>>(6);
          auto rem_data = std::make_shared<std::vector<unsigned char>>();
          auto compl_data = std::make_shared<std::vector<unsigned char>>();

          for(;;) {
              //auto rsize = co_await asio::async_read(socket_, size, 6, asio::use_awaitable);
              std::size_t len = be_char_to_32b({size->begin() + 2, size->begin() + 6 });
              rem_data->resize(len);

              //auto vrsize = co_await asio::async_read(socket_, rem_data, len, asio::use_awaitable);
              compl_data->reserve(size->size() + rem_data->size());
              compl_data->insert(compl_data->end(), size->begin(), size->end());
              compl_data->insert(compl_data->end(), rem_data->begin(), rem_data->end());
              received_pdu = compl_data.get();

              auto ptype = get_type(*compl_data);
              BOOST_LOG_SEV(logger, info) << "Received property of type " << ptype;
              StateMachine::EVENT e;
              switch (ptype) {
              case TYPE::A_ABORT:
                  e = StateMachine::EVENT::RECV_A_ABORT_PDU;
                  break;
              case TYPE::A_ASSOCIATE_AC:
                  e = StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU;
                  break;
              case TYPE::A_ASSOCIATE_RJ:
                  e = StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU;
                  break;
              case TYPE::A_ASSOCIATE_RQ:
                  e = StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU;
                  break;
              case TYPE::A_RELEASE_RQ:
                  e = StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU;
                  break;
              case TYPE::A_RELEASE_RP:
                  e = StateMachine::EVENT::RECV_A_RELEASE_RP_PDU;
                  break;
              case TYPE::P_DATA_TF:
                  e = StateMachine::EVENT::RECV_P_DATA_TF_PDU;
                  break;
              default:
                  e = StateMachine::EVENT::UNRECOG_PDU;
              }

              statem.transition(e); // side effects of the statemachine's transition function

              if (received_pdu != boost::none) {
                  auto property = make_property(*compl_data);
                  BOOST_LOG_SEV(logger, debug) << "Property info: \n" << *property;

                  // PDUs of type p_data_tf may come in fragments and with a
                  // data set, which needs to be received.
                  if (ptype == TYPE::P_DATA_TF) {
                      BOOST_LOG_SEV(logger, trace) << "Read data fragment of size " << bytes;
                      using namespace data::attribute;
                      auto pdataprop = dynamic_cast<p_data_tf*>(property.get());
                      assert(pdataprop);
                      if (!pdataprop->command_set.empty()) {
                          // check if a dataset is present in the message
                          auto commandset = proc.deserialize(pdataprop->command_set);
                          unsigned short datasetpresent;
                          get_value_field<VR::US>(commandset.at({0x0000, 0x0800}), datasetpresent);
                          if (datasetpresent != 0x0101) {
                              BOOST_LOG_SEV(logger, trace) << "Dataset present in the PDU ("
                                                           << "(0000,0800) = " << datasetpresent << ")";
                              get_complete_dataset(compl_data);
                          } else {
                              handle_pdu(std::move(property), TYPE::P_DATA_TF);
                          }
                      } else {
                          BOOST_LOG_SEV(logger, warning) << "No commandset present in the received data PDU. " <<
                              "Ignoring Message";
                      }
                  } else {
                      BOOST_LOG_SEV(logger, trace) << "Read PDU of size " << bytes;
                      handle_pdu(std::move(property), ptype);
                  }
              }
          }


          connection()->read_data(size, 6, [=](const boost::system::error_code& err, std::size_t bytes)  {
              try
              {
                  if (err) {
                      throw boost::system::system_error(err);
                  }
                  assert(bytes == 6);

                  std::size_t len = be_char_to_32b({size->begin()+2, size->begin()+6});
                  rem_data->resize(len);

                  BOOST_LOG_SEV(logger, trace) << "Size of incoming data unit: " << len;

                  connection()->read_data(rem_data, len,
                                          [=](const boost::system::error_code& err, std::size_t bytes) {
                                              try
                                              {
                                                  if (err) {
                                                      throw boost::system::system_error(err);
                                                  }

                                                  compl_data->reserve(size->size() + rem_data->size());
                                                  compl_data->insert(compl_data->end(), size->begin(), size->end());
                                                  compl_data->insert(compl_data->end(), rem_data->begin(), rem_data->end());
                                                  received_pdu = compl_data.get();

                                                  auto ptype = get_type(*compl_data);
                                                  BOOST_LOG_SEV(logger, info) << "Received property of type " << ptype;
                                                  StateMachine::EVENT e;
                                                  switch (ptype) {
                                                  case TYPE::A_ABORT:
                                                      e = StateMachine::EVENT::RECV_A_ABORT_PDU;
                                                      break;
                                                  case TYPE::A_ASSOCIATE_AC:
                                                      e = StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU;
                                                      break;
                                                  case TYPE::A_ASSOCIATE_RJ:
                                                      e = StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU;
                                                      break;
                                                  case TYPE::A_ASSOCIATE_RQ:
                                                      e = StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU;
                                                      break;
                                                  case TYPE::A_RELEASE_RQ:
                                                      e = StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU;
                                                      break;
                                                  case TYPE::A_RELEASE_RP:
                                                      e = StateMachine::EVENT::RECV_A_RELEASE_RP_PDU;
                                                      break;
                                                  case TYPE::P_DATA_TF:
                                                      e = StateMachine::EVENT::RECV_P_DATA_TF_PDU;
                                                      break;
                                                  default:
                                                      e = StateMachine::EVENT::UNRECOG_PDU;
                                                  }

                                                  statem.transition(e); // side effects of the statemachine's transition function

                                                  if (received_pdu != boost::none) {
                                                      auto property = make_property(*compl_data);
                                                      BOOST_LOG_SEV(logger, debug) << "Property info: \n" << *property;

                                                      // PDUs of type p_data_tf may come in fragments and with a
                                                      // data set, which needs to be received.
                                                      if (ptype == TYPE::P_DATA_TF) {
                                                          BOOST_LOG_SEV(logger, trace) << "Read data fragment of size " << bytes;
                                                          using namespace data::attribute;
                                                          auto pdataprop = dynamic_cast<p_data_tf*>(property.get());
                                                          assert(pdataprop);
                                                          if (!pdataprop->command_set.empty()) {
                                                              // check if a dataset is present in the message
                                                              auto commandset = proc.deserialize(pdataprop->command_set);
                                                              unsigned short datasetpresent;
                                                              get_value_field<VR::US>(commandset.at({0x0000, 0x0800}), datasetpresent);
                                                              if (datasetpresent != 0x0101) {
                                                                  BOOST_LOG_SEV(logger, trace) << "Dataset present in the PDU ("
                                                                                               << "(0000,0800) = " << datasetpresent << ")";
                                                                  get_complete_dataset(compl_data);
                                                              } else {
                                                                  handle_pdu(std::move(property), TYPE::P_DATA_TF);
                                                              }
                                                          } else {
                                                              BOOST_LOG_SEV(logger, warning) << "No commandset present in the received data PDU. " <<
                                                                  "Ignoring Message";
                                                          }
                                                      } else {
                                                          BOOST_LOG_SEV(logger, trace) << "Read PDU of size " << bytes;
                                                          handle_pdu(std::move(property), ptype);
                                                      }
                                                  }
                                              } catch (std::exception& excep) {
                                                  BOOST_LOG_SEV(logger, error) << "Error occured reading from connection\n"
                                                                               << "For the rest of the data\n"
                                                                               << excep.what();
                                                  error_handler(this, std::current_exception());
                                              }
                                          });
              } catch (std::exception& excep) {
                  BOOST_LOG_SEV(logger, error) << "Error occured reading from connection\n"
                                               << "During read of first 6 bytes (length)\n"
                                               << excep.what();
                  error_handler(this, std::current_exception());
              }
          });
      }

      /**
       * @brief artim_expired is called when the artim timer expires
       * @param[in] error
       */
      void artim_expired()
      {
          BOOST_LOG_SEV(this->logger, info) << "ARTIM timer expired";
          statem.transition(StateMachine::EVENT::ARTIM_EXPIRED);
      }

      // asio::awaitable<void> wathdog() {
      //     auto executor = co_await asio::this_coro::executor;
      //     asio::steady_timer timer(executor);
      //     for(;;) {
      //         co_await timer.async_wait(asio::use_awaitable);
      //         if(1)
      //             break;
      //     }
      //     artim_expired();
      //     co_return;
      // }

      asio::awaitable<void> watchdog(steady_clock::time_point& deadline)
      {
          asio::steady_timer timer(co_await boost::asio::this_coro::executor);
          deadline = std::max(deadline, steady_clock::now() + 10s);

          auto now = steady_clock::now();
          while (deadline > now)
          {
              timer.expires_at(deadline);
              //co_await timer.async_wait(asio::bind_cancellation_slot(timer_cancel_.slot(), use_nothrow_awaitable));
              now = steady_clock::now();
          }
          //kill
          artim_expired();
          //close();
          co_return;
      }

      std::map<TYPE, std::function<void(Connection*, property*)>> handlers_conf;

      std::function<void(Iupperlayer_comm_ops*, std::exception_ptr)> error_handler;

      dicom::util::log::channel_sev_logger logger;

   private:
      /**
       * @brief send takes a property and uses its property::make_pdu() function
       *        for serialization. The serialized data is sent to the peer via
       *        the socket.
       * @param[in] p
       */
       void send(property* p)
       {
           auto pdu = std::make_shared<std::vector<unsigned char>>(p->make_pdu());
           auto ptype = get_type(*pdu);

           StateMachine::EVENT e;
           switch (ptype) {
           case TYPE::A_ABORT:
               e = StateMachine::EVENT::LOCL_A_ABORT_PDU;
               break;
           case TYPE::A_ASSOCIATE_AC:
               e = StateMachine::EVENT::LOCL_A_ASSOCIATE_AC_PDU;
               break;
           case TYPE::A_ASSOCIATE_RJ:
               e = StateMachine::EVENT::LOCL_A_ASSOCIATE_RJ_PDU;
               break;
           case TYPE::A_ASSOCIATE_RQ:
               assert(false);
               break;
           case TYPE::A_RELEASE_RQ:
               e = StateMachine::EVENT::LOCL_A_RELEASE_RQ_PDU;
               break;
           case TYPE::A_RELEASE_RP:
               e = StateMachine::EVENT::LOCL_A_RELEASE_RP_PDU;
               break;
           case TYPE::P_DATA_TF:
               e = StateMachine::EVENT::LOCL_P_DATA_TF_PDU;
               break;
           default:
               e = StateMachine::EVENT::UNRECOG_PDU;
           }

           if (statem.transition(e) != StateMachine::CONN_STATE::INV) {
               // call async_write after each sent property until the queue is
               // empty
               if (ptype != TYPE::P_DATA_TF) {
                   connection()->write_data(pdu,
                                            [this, p, ptype](const boost::system::error_code& err, std::size_t bytes) {
                                                try
                                                {
                                                    if (err) {
                                                        throw boost::system::system_error(err);
                                                    }

                                                    BOOST_LOG_SEV(logger, trace) << "Sent PDU of size " << bytes;
                                                    BOOST_LOG_SEV(logger, info) << "Sent property of type " << ptype;
                                                    BOOST_LOG_SEV(logger, debug) << "Property info: \n" << *p;
                                                    handle_pdu_conf(p, ptype);
                                                } catch (std::exception& excep) {
                                                    BOOST_LOG_SEV(logger, error) << "Error occured writing to connection\n"
                                                                                 << excep.what();
                                                    error_handler(this, std::current_exception());
                                                }
                                            });
               } else {
                   auto pdataprop = dynamic_cast<p_data_tf*>(p);
                   assert(pdataprop);

                   std::size_t len = be_char_to_32b({pdu->begin()+6, pdu->begin()+10});
                   auto commandset = std::make_shared<std::vector<unsigned char>>(pdu->begin(), pdu->begin()+10+len);
                   connection()->write_data(commandset,
                                            [this, commandset, len, pdu, p, ptype, pdataprop]
                                            (const boost::system::error_code& err, std::size_t bytes) {
                                                try
                                                {
                                                    if (err) {
                                                        throw boost::system::system_error(err);
                                                    }

                                                    BOOST_LOG_SEV(logger, trace) << "Sent data fragment of size " << bytes;
                                                    BOOST_LOG_SEV(logger, info) << "Sent property of type " << ptype;
                                                    BOOST_LOG_SEV(logger, debug) << "Property info: \n" << *p;

                                                    using namespace data::attribute;
                                                    if (!pdataprop->command_set.empty()) {
                                                        // check if a dataset is present in the message
                                                        auto commandset = proc.deserialize(pdataprop->command_set);
                                                        unsigned short datasetpresent;
                                                        get_value_field<VR::US>(commandset.at({0x0000, 0x0800}), datasetpresent);
                                                        if (datasetpresent != 0x0101) {
                                                            auto dataset = std::make_shared<std::vector<unsigned char>>(pdu->begin() + 10 + len, pdu->end());
                                                            write_complete_dataset(p, dataset);
                                                        } else {
                                                            handle_pdu_conf(p, TYPE::P_DATA_TF);
                                                        }
                                                    } else {
                                                        assert(false);
                                                    }
                                                } catch (std::exception& excep) {
                                                    BOOST_LOG_SEV(logger, error) << "Error occured writing command set"
                                                                                 << "of p_data_tf to connection"
                                                                                 << excep.what();
                                                    error_handler(this, std::current_exception());
                                                }
                                            });
               }
           }
       }

      /**
       * @brief handle_pdu calls the pdu-type specific handler
       * @param p property containing data
       * @param ptype type of the pdu / property
       */
      void handle_pdu(std::unique_ptr<property> p, TYPE ptype)
      {
          if (handlers[ptype])
          {
              handlers[ptype](this, std::move(p));
          }

          if (get_state() == StateMachine::CONN_STATE::STA13) {
              close_connection();
          } else {
              // be ready for new data
              if (!connection()->is_stopped() && !shutdown_requested) {
                  do_read();
              }
          }

      }

      /**
       * @brief handle_pdu_conf
       * @param p
       * @param ptype
       */
      void handle_pdu_conf(property* p, TYPE ptype)
      {
          if (handlers_conf.find(ptype) != handlers_conf.end()) {
              handlers_conf[ptype](this, p);
          }
          send_queue.pop_front();
          if (!send_queue.empty()) {
              send(send_queue.front().get());
          }
      }

      /**
       * @brief get_complete_dataset reads the complete dataset and stores it
       *        in the property's dataset member.
       * @param[in] data
       */
      void get_complete_dataset(std::shared_ptr<std::vector<unsigned char> > data)
      {
          auto nextbuflen = std::make_shared<std::vector<unsigned char>>(6);
          auto nextbufdata = std::make_shared<std::vector<unsigned char>>();
          auto nextbufcompl = std::make_shared<std::vector<unsigned char>>();


          connection()->read_data(nextbuflen, 6, [=] (const boost::system::error_code& err, std::size_t bytes) mutable {
              try
              {
                  if (err) {
                      throw boost::system::system_error(err);
                  }
                  std::size_t len = be_char_to_32b({nextbuflen->begin()+2, nextbuflen->begin()+6});
                  nextbufdata->resize(len);

                  BOOST_LOG_SEV(logger, trace) << "Size of incoming data unit: " << len;

                  connection()->read_data(nextbufdata, [=] (const boost::system::error_code& err, std::size_t bytes) mutable {
                      try
                      {
                          if (err) {
                              throw boost::system::system_error(err);
                          }

                          BOOST_LOG_SEV(logger, trace) << "Read data fragment of size " << bytes;

                          nextbufcompl->reserve(len+6);
                          nextbufcompl->insert(nextbufcompl->end(), std::make_move_iterator(nextbuflen->begin()), std::make_move_iterator(nextbuflen->end()));
                          nextbufcompl->insert(nextbufcompl->end(), std::make_move_iterator(nextbufdata->begin()), std::make_move_iterator(nextbufdata->end()));
                          data->insert(data->end(), std::make_move_iterator(nextbufcompl->begin()), std::make_move_iterator(nextbufcompl->end()));

                          bool lastsegment = ((*nextbufcompl)[11] & 0x02);
                          if (lastsegment) {
                              BOOST_LOG_SEV(logger, trace) << "Last data fragment";
                              auto pdu = make_property(*data);
                              handle_pdu(std::move(pdu), TYPE::P_DATA_TF);
                          } else {
                              BOOST_LOG_SEV(logger, trace) << "More data fragments expected";
                              get_complete_dataset(data);
                          }
                      } catch (std::exception& excep) {
                          BOOST_LOG_SEV(logger, error) << "Error occured reading from connection\n"
                                                       << "For the rest of the data\n"
                                                       << excep.what();
                          error_handler(this, std::current_exception());
                      }
                  });
              } catch (std::exception& excep) {
                  BOOST_LOG_SEV(logger, error) << "Error occured reading from connection\n"
                                               << "During read of first 6 bytes (length)\n"
                                               << excep.what();
                  error_handler(this, std::current_exception());
              }
          });
      }


      /**
       * @brief write_complete_dataset write the complete dataset to the peer
       * @param p property being written (stored in the write queue)
       * @param data serialized property being written
       */
      void write_complete_dataset(property* p, std::shared_ptr<std::vector<unsigned char>> data, std::size_t begin = 0)
      {
          std::size_t len = be_char_to_32b({data->begin()+begin+2, data->begin()+begin+6}) + 6;
              // 6 bytes header + length

          void* data_offset = (static_cast<void*>(data->data()+begin));

          connection()->write_data(data_offset, len,
                                   [this, p, data, len, begin, data_offset](const boost::system::error_code& err, std::size_t bytes) {
                                       try
                                       {
                                           if (err) {
                                               throw boost::system::system_error(err);
                                           }

                                           BOOST_LOG_SEV(logger, trace) << "Sent data fragment of size " << bytes;

                                           bool lastsegment = ((static_cast<unsigned char*>(data_offset))[11] & 0x02);
                                           if (lastsegment) {
                                               BOOST_LOG_SEV(logger, trace) << "Last data fragment";
                                               handle_pdu_conf(p, TYPE::P_DATA_TF);
                                           } else {
                                               BOOST_LOG_SEV(logger, trace) << "More data fragments to be sent";
                                               std::size_t newbegin = begin + len;
                                               write_complete_dataset(p, data, newbegin);
                                           }
                                       } catch (std::exception& excep) {
                                           BOOST_LOG_SEV(logger, error) << "Error writing fragment of p_data_tf\n"
                                                                        << excep.what();
                                           error_handler(this, std::current_exception());
                                       }
                                   });

      }


      /**
       * @brief artim_timer returns a pointer to the artim timer
       * @return referencing pointer to the artim timer
       */
      //virtual Iinfrastructure_timeout_connection* artim_timer() = 0;

      data::dataset::commandset_processor proc;

      std::deque<std::unique_ptr<property>> send_queue;
      boost::optional<std::vector<unsigned char>*> received_pdu;
      std::map<TYPE, std::function<void(Connection*, std::unique_ptr<property>)>> handlers;

      bool shutdown_requested;


   protected:
      //virtual Iinfrastructure_upperlayer_connection* connection() = 0;

      // std::function<void(Iupperlayer_comm_ops*)> handler_new_connection;
      // std::function<void(Iupperlayer_comm_ops*)> handler_end_connection;
};

/**
 * @brief The scp_connection class represents a single association between the
 *        scp and a remote scu.
 */
// class scp_connection: public Connection
// {
//    public:
//     scp_connection(Iinfrastructure_upperlayer_connection* tcp_conn,
//                    data::dictionary::dictionaries& dict,
//                    std::function<void(Iupperlayer_comm_ops*)> handler_new_conn,
//                    std::function<void(Iupperlayer_comm_ops*)> handler_end_conn,
//                    std::function<void(Iupperlayer_comm_ops*, std::exception_ptr)> on_error,
//                    std::vector<std::pair<TYPE, std::function<void(Connection*, std::unique_ptr<property>)>>> l = {{}}):
//         Connection {dict, on_error, l},
//         conn {tcp_conn},
//         artim {conn->timeout_timer(std::chrono::seconds(10), [this](){ artim_expired(); })}
//     {
//         // handler_new_connection = handler_new_conn;
//         // handler_end_connection = handler_end_conn;
//         artim->cancel();
//         statem.transition(StateMachine::EVENT::TRANS_CONN_INDIC);
//         // handler_new_connection(this);
//         do_read();
//     }

//       scp_connection(const scp_connection&) = delete;
//       scp_connection& operator=(const scp_connection&) = delete;

//    private:
//       Iinfrastructure_upperlayer_connection* conn;

//        Iinfrastructure_timeout_connection* artim_timer() override
//        {
//            return artim.get();
//        }

//       std::unique_ptr<Iinfrastructure_timeout_connection> artim;

//    protected:
//       virtual Iinfrastructure_upperlayer_connection* connection() override { return conn; }
// };

// /**
//  * @brief The scu_connection class represents a single associaton between the
//  *        local scu and the remote scp.
//  */
// class scu_connection: public Connection
// {
//    public:
//     scu_connection(Iinfrastructure_upperlayer_connection* conn,
//                    data::dictionary::dictionaries& dict,
//                    a_associate_rq& rq,
//                    std::function<void(Iupperlayer_comm_ops*)> handler_new_conn,
//                    std::function<void(Iupperlayer_comm_ops*)> handler_end_conn,
//                    std::function<void(Iupperlayer_comm_ops*, std::exception_ptr)> on_error,
//                    std::vector<std::pair<TYPE, std::function<void(Connection*, std::unique_ptr<property>)>>> l = {{}}):
//         Connection {dict, on_error, l},
//         conn {conn},
//         artim {conn->timeout_timer(std::chrono::seconds(10), [this](){ artim_expired(); })}
//     {
//         handler_new_connection = handler_new_conn;
//         handler_end_connection = handler_end_conn;
//         statem.transition(StateMachine::EVENT::A_ASSOCIATE_RQ);

//         statem.transition(StateMachine::EVENT::TRANS_CONN_CONF);

//         handler_new_connection(this);

//         auto pdu = std::make_shared<std::vector<unsigned char>>(rq.make_pdu());
//         //this->queue_for_write(std::unique_ptr<property>(new a_associate_rq {rq}));/*
//         conn->write_data(pdu, [this, pdu, &rq](const boost::system::error_code& err, std::size_t) mutable {
//             try
//             {
//                 if (!err) {
//                     auto type = TYPE::A_ASSOCIATE_RQ;
//                     BOOST_LOG_SEV(logger, info) << "Sent property of type " << type;
//                     BOOST_LOG_SEV(logger, debug) << "Property info: \n" << rq;
//                     if (handlers_conf.find(type) != handlers_conf.end()) {
//                         handlers_conf[type](this, &rq);
//                     }
//                     do_read();
//                 } else {
//                     throw boost::system::system_error(err);
//                 }
//             } catch (std::exception& excep) {
//                 // BOOST_LOG_SEV(logger, error) << "Error occured writing initial a_associate_rq\n"
//                 //                              << excep.what();
//                 error_handler(this, std::current_exception());
//             }
//         });
//     }

//       scu_connection(const scu_connection&) = delete;
//       scu_connection& operator=(const scu_connection&) = delete;

//    private:
//       Iinfrastructure_upperlayer_connection* conn;

//        Iinfrastructure_timeout_connection* artim_timer() override
//        {
//            return artim.get();
//        }

//       std::unique_ptr<Iinfrastructure_timeout_connection> artim;

//    protected:
//       virtual Iinfrastructure_upperlayer_connection* connection() override { return conn; }
// };

}

}

}

#endif // DICOMUPPERLAYER_HPP
