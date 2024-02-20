#ifndef ASIO_TCP_CONNECTION_MANAGER_HPP
#define ASIO_TCP_CONNECTION_MANAGER_HPP

#include "asio_tcp_connection.hpp"


/**
 * @brief The Iinfrastructure_server_acceptor interface represents the functionality
 *        used by an scp to accept new connections and be notified when one ends.
 * TODO: To facilitate DIP this should be moved to where it is actually used
 */
struct Iinfrastructure_server_acceptor
{
      /**
       * @brief start starts the process, for a TCP socket this would start
       *        listening.
       */
    virtual void start() = 0;

      /**
       * @brief set_handler_new sets the callback to be invoked for a new
       *        connection
       * @param handler callback to be invoked
       */
      virtual void set_handler_new(std::function<void(Iinfrastructure_upperlayer_connection*)> handler) = 0;

      /**
       * @brief set_handler_end sets the callback to be invoked when a
       *        connection terminates
       * @param handler callback to be invoked
       */
      virtual void set_handler_end(std::function<void(Iinfrastructure_upperlayer_connection*)> handler) = 0;

      virtual ~Iinfrastructure_server_acceptor() = 0;
};

/**
 * @brief The Iinfrastructure_client_acceptor interface abstracts functionality
 *        used by an scu to start a new connection and be notified when one
 *        ends.
 * TODO: To facilitate DIP this should be moved to where it is actually used
 */
struct Iinfrastructure_client_acceptor
{
      /**
       * @brief run starts the connection
       */
      virtual void run() = 0;

      /**
       * @brief set_handler_new sets the callback to be invoked for a new
       *        connection
       * @param handler callback to be invoked
       */
      virtual void set_handler_new(std::function<void(Iinfrastructure_upperlayer_connection*)> handler) = 0;

      /**
       * @brief set_handler_end sets the callback to be invoked when a
       *        connection terminates
       * @param handler callback to be invoked
       */
      virtual void set_handler_end(std::function<void(Iinfrastructure_upperlayer_connection*)> handler) = 0;

      /**
       * @brief accept_new_conn invokes a new connection as a client
       */
      virtual void accept_new_conn() = 0;

      virtual ~Iinfrastructure_client_acceptor() = 0;
};

/**
 * @brief The Server class encapsulates functionality to
 *        accept tcp connection as a server using boost asio.
 */
class Server /*: public Iinfrastructure_server_acceptor*/
{
    public:
    Server(short port, std::string AETname = ""
           /*std::function<void(Iinfrastructure_upperlayer_connection*)> new_connection = nullptr,
           std::function<void(Iinfrastructure_upperlayer_connection*)> end_connection = nullptr*/)
            :
            // handler_new {new_connection},
            // handler_end {end_connection},
            ctx_ {},
            acceptor_ {ctx_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)}
        {
            // using namespace std::placeholders;
            // auto socket = std::make_shared<boost::asio::ip::tcp::socket>(acceptor_.get_executor());
            // acceptor_.async_accept(*socket, [socket, this](boost::system::error_code ec) { accept_new(socket, ec); });
        }

        void do_accept() {
            auto connection = std::make_shared<tcpconnection>(ctx_);
            acceptor_.async_accept(connection->socket(), [this, connection](const boost::system::error_code& code) {
                if (!code) {
                    connection->socket().set_option(asio::ip::tcp::no_delay{true});
                    connection->start(connection->shared_from_this());
                    /*
                     *     co_spawn(this->socket_.get_executor(), this->coro_http(this->shared_from_this()), detached);
                     *     co_spawn(this->socket_.get_executor(), do_handshake(this->shared_from_this()), detached); -> coro_http(this->shared_from_this())
                     */
                }

                this->do_accept();
            });
        }

        void run() {
            //auto work = std::make_shared<asio::io_context::work>(ctx_);
            worker_guards_.emplace_back(ctx_);
            do_accept();
            ctx_.run();
        }

        void start()
        {
            ctx_.run();
        }

        // void set_handler_new(std::function<void(Iinfrastructure_upperlayer_connection*)> handler) override;

        // void set_handler_end(std::function<void(Iinfrastructure_upperlayer_connection*)> handler) override;


    private:
        // void accept_new(std::shared_ptr<boost::asio::ip::tcp::socket> sock, boost::system::error_code ec)
        // {
        //     using namespace std::placeholders;
        //     if (!ec) {
        //         connections.push_back(std::unique_ptr<tcpconnection>
        //                               {
        //                                   new tcpconnection {ctx_, sock, handler_end}
        //                               });
        //     } else {
        //         throw boost::system::system_error(ec);
        //     }

        //     handler_new(connections.back().get());

        //     auto newsock = std::make_shared<boost::asio::ip::tcp::socket>(acceptor_.get_executor());
        //     acceptor_.async_accept(*newsock, [newsock, this](boost::system::error_code ec) { accept_new(newsock, ec); });
        // }

        std::vector<std::unique_ptr<Iinfrastructure_upperlayer_connection>> connections;

        std::function<void(Iinfrastructure_upperlayer_connection*)> handler_new;
        std::function<void(Iinfrastructure_upperlayer_connection*)> handler_end;

        asio::io_context ctx_;
        asio::ip::tcp::acceptor acceptor_;
        std::vector<asio::io_context::work> worker_guards_;
        std::vector<std::shared_ptr<tcpconnection>> connections_;
};

/**
 * @brief The asio_tcp_client_acceptor class encapsulates functionality to
 *        start new tcp connections to a remote peer as a client, using boost
 *        asio.
 */
class asio_tcp_client_acceptor : public Iinfrastructure_client_acceptor
{
    public:
        asio_tcp_client_acceptor(std::string host, std::string port,
                                 std::function<void(Iinfrastructure_upperlayer_connection*)> new_connection = nullptr,
                                 std::function<void(Iinfrastructure_upperlayer_connection*)> end_connection = nullptr);

        void run() override;

        void set_handler_new(std::function<void(Iinfrastructure_upperlayer_connection*)> handler) override;

        void set_handler_end(std::function<void(Iinfrastructure_upperlayer_connection*)> handler) override;

        void accept_new_conn() override;

    private:
        void accept_new();

        std::vector<std::unique_ptr<Iinfrastructure_upperlayer_connection>> connections;

        std::function<void(Iinfrastructure_upperlayer_connection*)> handler_new;
        std::function<void(Iinfrastructure_upperlayer_connection*)> handler_end;

        asio::io_context io_s;
        asio::ip::tcp::resolver resolver;
        asio::ip::tcp::resolver::query query;
        asio::ip::tcp::resolver::iterator endpoint_iterator;
};

#endif // ASIO_TCP_CONNECTION_MANAGER_HPP
