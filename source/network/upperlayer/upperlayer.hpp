#ifndef UPPERLAYER_HPP
#define UPPERLAYER_HPP

#include <functional>
#include <memory>

#include <boost/log/trivial.hpp>

#include "upperlayer_connection.hpp"
#include "infrastructure/asio_tcp_connection_manager.hpp"
#include "data/dictionary/dictionary.hpp"
#include "util/channel_sev_logger.hpp"


namespace dicom
{


namespace network
{


namespace upperlayer
{

/**
 * @brief The Iupperlayer_connection_handlers struct defines an interface to inject callbacks
 *        for the start / termination of a connection.
 */
struct Iupperlayer_connection_handlers
{
      /**
       * @brief new_connection is the handler that will be invoked when a new
       *        connection is established.
       * @param f function which receives a pointer to the communication object
       *        as a parameter
       */
      virtual void new_connection(std::function<void(Iupperlayer_comm_ops*)> f) = 0;

      /**
       * @brief end_connection is the handler when an existing association is
       *        terminated.
       * @param f function with the communication instance as parameter which
       *        has ended.
       */
      virtual void end_connection(std::function<void(Iupperlayer_comm_ops*)> f) = 0;


      virtual void connection_error(std::function<void(Iupperlayer_comm_ops*, std::exception_ptr)> f) = 0;

      /**
       * @brief run starts the handling of the connections
       */
      virtual void run() = 0;

      virtual ~Iupperlayer_connection_handlers() = 0;
};


/**
 * @brief The scp class acts as a service class provider which can have
 *        and manages multiple connections simultaneously as specified by the
 *        constructor's parameters.
 */
class scp: public Iupperlayer_connection_handlers
{
   public:
    scp(Iinfrastructure_server_acceptor& infrstr_scp,
        data::dictionary::dictionaries& dict):
        acceptor {infrstr_scp},
        dict {dict},
        logger {"scp"}
    {
        acceptor.set_handler_new([this](Iinfrastructure_upperlayer_connection* conn) { accept_new(conn);});
        acceptor.set_handler_end([this](Iinfrastructure_upperlayer_connection* conn) { connection_end(conn);});
    }
      scp(const scp&) = delete;
      scp& operator=(const scp&) = delete;

      ~scp()
      {
      }

      /**
       * @brief run() instructs the server to start listening for associations
       */
      virtual void run() override
      {
          acceptor.start();
      }

      virtual void new_connection(std::function<void(Iupperlayer_comm_ops*)> handler) override
      {
          handler_new_connection = handler;
      }
      virtual void end_connection(std::function<void(Iupperlayer_comm_ops*)> handler) override
      {
          handler_end_connection = handler;
      }
      virtual void connection_error(std::function<void(Iupperlayer_comm_ops*, std::exception_ptr)> handler) override
      {
          handler_error = handler;
      }

   private:

      std::map<Iinfrastructure_upperlayer_connection*, std::unique_ptr<scp_connection>> scps;

       void accept_new(Iinfrastructure_upperlayer_connection* conn)
       {
           scps[conn] = std::unique_ptr<scp_connection>
               {
                   new scp_connection {conn, dict, handler_new_connection, handler_end_connection, [&](Iupperlayer_comm_ops* conn, std::exception_ptr exception) { this->error_handler(conn, exception); }}
               };
           BOOST_LOG_SEV(logger, info) << "New connection" << conn;
       }

      void error_handler(Iupperlayer_comm_ops* conn, std::exception_ptr exception)
      {
          try {
              if (exception) {
                  std::rethrow_exception(exception);
              }
          } catch (std::exception& exc) {
              BOOST_LOG_SEV(logger, error) << "Error occured on connection " << conn
                                           << "Error message: " << exc.what();
          }
          if (handler_error)
          {
              handler_error(conn, exception);
          }
      }

      void connection_end(Iinfrastructure_upperlayer_connection* conn)
      {
          auto& sc = scps.at(conn);
          handler_end_connection(sc.get());
          //sc->reset();
          scps.erase(conn);
          BOOST_LOG_SEV(logger, info) << "Connection ended" << conn;
      }

      std::function<void(Iupperlayer_comm_ops*)> handler_new_connection;
      std::function<void(Iupperlayer_comm_ops*)> handler_end_connection;
      std::function<void(Iupperlayer_comm_ops*, std::exception_ptr)> handler_error;

      Iinfrastructure_server_acceptor& acceptor;

      data::dictionary::dictionaries& dict;
      util::log::channel_sev_logger logger;
};

/**
 * @brief The scu class handles all connections to a remote application entity.
 */
class scu: public Iupperlayer_connection_handlers
{
   public:
    scu(Iinfrastructure_client_acceptor& infr_scu,
        data::dictionary::dictionaries& dict,
        a_associate_rq& rq):
        acceptor {infr_scu},
        request {rq},
        dict {dict},
        logger {"scu"}
    {
        using namespace std::placeholders;
        acceptor.set_handler_new([this](Iinfrastructure_upperlayer_connection* conn) { accept_new(conn);});
        acceptor.set_handler_end([this](Iinfrastructure_upperlayer_connection* conn) { connection_end(conn);});
    }
      scu(const scu&) = delete;
      scu& operator=(const scu&) = delete;

      ~scu()
      {
      }

      /**
       * @brief run() instructs to client to start transmitting
       */
      virtual void run() override
      {
          acceptor.run();
      }

      /**
       * @brief accept_new starts a new association with the parameters
       *        specified in the constructor.
       */
      void accept_new()
      {
          acceptor.accept_new_conn();
      }

      virtual void new_connection(std::function<void(Iupperlayer_comm_ops*)> handler) override
      {
          handler_new_connection = handler;
      }
      virtual void end_connection(std::function<void(Iupperlayer_comm_ops*)> handler) override
      {
          handler_end_connection = handler;
      }
      virtual void connection_error(std::function<void(Iupperlayer_comm_ops*, std::exception_ptr)> handler) override
      {
          handler_error = handler;
      }

   private:
      void accept_new(Iinfrastructure_upperlayer_connection* conn)
      {
          scus[conn] = std::unique_ptr<scu_connection> {new scu_connection {conn, dict, request, handler_new_connection, handler_end_connection, [&](Iupperlayer_comm_ops* conn, std::exception_ptr exception) { this->error_handler(conn, exception); }}};
          BOOST_LOG_SEV(logger, info) << "New connection " << conn;
      }

       void error_handler(Iupperlayer_comm_ops* conn, std::exception_ptr exception)
       {
           try {
               if (exception) {
                   std::rethrow_exception(exception);
               }
           } catch (std::exception& exc) {
               BOOST_LOG_SEV(logger, error) << "Error occured on connection " << conn
                                            << "Error message: " << exc.what();
           }

           if (handler_error)
           {
               handler_error(conn, exception);
           }
       }

       void connection_end(Iinfrastructure_upperlayer_connection* conn)
       {
           auto& sc = scus.at(conn);
           handler_end_connection(sc.get());
           //sc->reset();
           scus.erase(conn);
           BOOST_LOG_SEV(logger, info) << "Connection ended" << conn;
       }

      std::function<void(Iupperlayer_comm_ops*)> handler_new_connection;
      std::function<void(Iupperlayer_comm_ops*)> handler_end_connection;
      std::function<void(Iupperlayer_comm_ops*, std::exception_ptr)> handler_error;

      Iinfrastructure_client_acceptor& acceptor;

      std::map<Iinfrastructure_upperlayer_connection*, std::unique_ptr<scu_connection>> scus;
      a_associate_rq& request;

      data::dictionary::dictionaries& dict;
      util::log::channel_sev_logger logger;
};

}

}

}


#endif // UPPERLAYER_HPP
