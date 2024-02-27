#ifndef UPPERLAYER_TRANSITIONS_HPP
#define UPPERLAYER_TRANSITIONS_HPP

#include <map>
#include <functional>
#include <queue>
#include <memory>

#include "Connection.hpp"
#include "upperlayer_properties.hpp"
//#include "upperlayer_connection.hpp"

#include "util/channel_sev_logger.hpp"
using namespace dicom::util::log;

namespace dicom
{

namespace network
{

namespace upperlayer
{

//struct Istate_trans_ops;

class StateMachine
{
   public:
      /**
       * @brief The CONN_STATE enum contains the states for the state machine
       * @see DICOM3 standard table 9-10
       */
      enum class CONN_STATE
      {
         INV = 0x00,
         STA1,
         STA2,
         STA3,
         STA4,
         STA5,
         STA6,
         STA7,
         STA8,
         STA9,
         STA10,
         STA11,
         STA12,
         STA13
      };

      enum class EVENT
      {
         A_ASSOCIATE_RQ,
         TRANS_CONN_CONF,
         RECV_A_ASSOCIATE_AC_PDU,
         RECV_A_ASSOCIATE_RJ_PDU,
         TRANS_CONN_INDIC,
         RECV_A_ASSOCIATE_RQ_PDU,
         LOCL_A_ASSOCIATE_AC_PDU,
         LOCL_A_ASSOCIATE_RJ_PDU,
         LOCL_P_DATA_TF_PDU,
         RECV_P_DATA_TF_PDU,
         LOCL_A_RELEASE_RQ_PDU,
         RECV_A_RELEASE_RQ_PDU,
         RECV_A_RELEASE_RP_PDU,
         LOCL_A_RELEASE_RP_PDU,
         LOCL_A_ABORT_PDU,
         RECV_A_ABORT_PDU,
         TRANS_CONN_CLOSED,
         ARTIM_EXPIRED,
         UNRECOG_PDU
      };

      StateMachine(connection *connection, std::deque<std::unique_ptr<property>> &send_queue):
         // ul {ul},
          connection_(connection),
          state_ {CONN_STATE::STA1},
          send_queue_(send_queue),
          logger {"upperlayer sm"}
      {
      }

      CONN_STATE get_state()
      {
          return state_;
      }
      CONN_STATE transition(EVENT e)
      {
          if (transition_table.find({e, state_}) != transition_table.end()) {
              transition_table[{e, state_}](this);
              return state_;
          } else {
              return CONN_STATE::INV;
          }
      }

   private:
      //Istate_trans_ops* ul;
       connection *connection_;
       CONN_STATE state_;
       std::deque<std::unique_ptr<property>> &send_queue_;
      //bool request_shutdown = false;

   private:

      void aa1()
      {
          BOOST_LOG_SEV(logger, trace) << "AA-1";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta13";
          send_queue_.emplace_back(new a_abort {});
          //ul->queue_for_write_w_prio(std::unique_ptr<property>(new a_abort {}));
          state_ = CONN_STATE::STA13;
      }
      void aa2()
      {
          BOOST_LOG_SEV(logger, trace) << "AA-2";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta1";
          //ul->stop_artim();
          connection_->stop_artim();
          connection_->stop_artim();
          state_ = CONN_STATE::STA1;
          //ul->close_connection();
          connection_->close();
      }
      void aa3()
      {
          BOOST_LOG_SEV(logger, trace) << "AA-3";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta1";
          // call a_abort handler
          state_ = CONN_STATE::STA1;
          //ul->close_connection();
          connection_->close();
      }
      void aa4()
      {
          BOOST_LOG_SEV(logger, trace) << "AA-4";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta1";
          //ul->stop_artim();
          connection_->stop_artim();
          state_ = CONN_STATE::STA1;
      }

      void aa5()
      {
          BOOST_LOG_SEV(logger, trace) << "AA-5";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta1";
          //ul->stop_artim();
          connection_->stop_artim();
          state_ = CONN_STATE::STA1;
      }
      void aa6()
      {
          BOOST_LOG_SEV(logger, trace) << "AA-6";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta13";
          //ul->ignore_next();
          connection_->ignorenext_ = true;
          state_ = CONN_STATE::STA13;
          connection_->close();
      }
      void aa7()
      {
          BOOST_LOG_SEV(logger, trace) << "AA-7";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta13";
          send_queue_.emplace_back(new a_abort {});
          //ul->queue_for_write_w_prio(std::unique_ptr<property>(new a_abort {}));
          state_ = CONN_STATE::STA13;
      }
      void aa8()
      {
          BOOST_LOG_SEV(logger, trace) << "AA-8";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta13";
          send_queue_.emplace_back(new a_abort {});
          //ul->queue_for_write_w_prio(std::unique_ptr<property>(new a_abort {}));
          ////ul->start_artim();
          connection_->start_artim();
          // A-P-Abort indic
          state_ = CONN_STATE::STA13;
      }

      void ae1()
      {
          BOOST_LOG_SEV(logger, trace) << "AE-1";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta4";
          state_ = CONN_STATE::STA4;
      }
      void ae2()
      {
          BOOST_LOG_SEV(logger, trace) << "AE-2";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta5";
          state_ = CONN_STATE::STA5;
      }
      void ae3()
      {
          BOOST_LOG_SEV(logger, trace) << "AE-3";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta6";
          state_ = CONN_STATE::STA6;
      }
      void ae4()
      {
          BOOST_LOG_SEV(logger, trace) << "AE-4";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta1";
          state_ = CONN_STATE::STA1;
          //ul->close_connection();
          connection_->close();
      }

      void ae5()
      {
          BOOST_LOG_SEV(logger, trace) << "AE-5";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta2";
          //ul->start_artim();
          connection_->start_artim();
          state_ = CONN_STATE::STA2;
      }
      void ae6()
      {
          BOOST_LOG_SEV(logger, trace) << "AE-6";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta3";
          //ul->stop_artim();
          connection_->stop_artim();
          state_ = CONN_STATE::STA3;
      }
      void ae7()
      {
          BOOST_LOG_SEV(logger, trace) << "AE-7";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta6";
          state_ = CONN_STATE::STA6;
      }
      void ae8()
      {
          BOOST_LOG_SEV(logger, trace) << "AE-8";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta13";
          //ul->start_artim();
          connection_->start_artim();
          state_ = CONN_STATE::STA13;
      }

      void ar1()
      {
          BOOST_LOG_SEV(logger, trace) << "AR-1";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta7";
          state_ = CONN_STATE::STA7;
      }
      void ar2()
      {
          BOOST_LOG_SEV(logger, trace) << "AR-2";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta8";
          state_ = CONN_STATE::STA8;
      }
      void ar3()
      {
          BOOST_LOG_SEV(logger, trace) << "AR-3";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta1";
          state_ = CONN_STATE::STA1;
          // release conf handler
          //ul->close_connection();
          connection_->close();
      }
      void ar4()
      {
          BOOST_LOG_SEV(logger, trace) << "AR-4";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta13";
          //ul->start_artim();
          connection_->start_artim();
          state_ = CONN_STATE::STA13;
      }
      void ar5()
      {
          BOOST_LOG_SEV(logger, trace) << "AR-5";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta1";
          //ul->stop_artim();
          connection_->stop_artim();
          state_ = CONN_STATE::STA1;
      }
      void ar6()
      {
          BOOST_LOG_SEV(logger, trace) << "AR-6";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta7";
          state_ = CONN_STATE::STA7;
      }
      void ar7()
      {
          BOOST_LOG_SEV(logger, trace) << "AR-7";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta8";
          state_ = CONN_STATE::STA8;
      }
      void ar8()
      {
          BOOST_LOG_SEV(logger, trace) << "AR-8";
          if (connection_->isSCU_ /*dynamic_cast<upperlayer::scu_connection*>(ul)*/) {
              BOOST_LOG_SEV(logger, trace) << "Next state is Sta9";
              state_ = CONN_STATE::STA9;
          } else {
              BOOST_LOG_SEV(logger, trace) << "Next state is Sta10";
              state_ = CONN_STATE::STA10;
          }
          // a release indication collision
      }
      void ar9()
      {
          BOOST_LOG_SEV(logger, trace) << "AR-9";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta11";
          state_ = CONN_STATE::STA11;
      }
      void ar10()
      {
          BOOST_LOG_SEV(logger, trace) << "AR-10";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta12";
          state_ = CONN_STATE::STA12;
      }

      void dt1()
      {
          BOOST_LOG_SEV(logger, trace) << "DT-1";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta6";
          state_ = CONN_STATE::STA6;
      }
      void dt2()
      {
          BOOST_LOG_SEV(logger, trace) << "DT-2";
          BOOST_LOG_SEV(logger, trace) << "Next state is Sta6";
          state_ = CONN_STATE::STA6;
      }

      static std::map<std::pair<EVENT, CONN_STATE>, std::function<void(StateMachine*)>> transition_table;

      dicom::util::log::channel_sev_logger logger;

      CONN_STATE handleEvent(EVENT event)
      {
          //using namespace StateMachine::EVENT;
          switch (event)
          {
          case EVENT::A_ASSOCIATE_RQ:
              //{StateMachine::EVENT::A_ASSOCIATE_RQ, StateMachine::CONN_STATE::STA1}, std::mem_fn(&StateMachine::ae1)},
              if (state_ == CONN_STATE::STA1) {
                  ae1();
              }
              else {
               return CONN_STATE::INV;
              }
                  break;
          case EVENT::TRANS_CONN_CONF:
              //{{StateMachine::EVENT::TRANS_CONN_CONF, StateMachine::CONN_STATE::STA4}, std::mem_fn(&StateMachine::ae2)},
              if (state_ == CONN_STATE::STA4) {
                  ae2(); // Call the function ae2 if the current state is STA4
              }  else {
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::RECV_A_ASSOCIATE_AC_PDU:
              // {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::ae3)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa6)},
              switch (state_) {
              case CONN_STATE::STA2:
                  aa1();
                  break;
              case CONN_STATE::STA3:
              case CONN_STATE::STA6:
              case CONN_STATE::STA7:
              case CONN_STATE::STA8:
              case CONN_STATE::STA9:
              case CONN_STATE::STA10:
              case CONN_STATE::STA11:
              case CONN_STATE::STA12:
                  aa8();
                  break;
              case CONN_STATE::STA5:
                  ae3();
                  break;
              case CONN_STATE::STA13:
                  aa6();
                  break;
              default:
                  // Handle invalid state
                  return CONN_STATE::INV;
                  break;
              }

              break;
          case EVENT::RECV_A_ASSOCIATE_RJ_PDU:
              // {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::ae4)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa6)},
              break;
          case EVENT::TRANS_CONN_INDIC:
              //{{StateMachine::EVENT::TRANS_CONN_INDIC, StateMachine::CONN_STATE::STA1}, std::mem_fn(&StateMachine::ae5)},
              switch (state_)
              {
                  case CONN_STATE::STA1:
                  ae5();
                  break;
                  default:
                      return CONN_STATE::INV;
              }
              break;
          case EVENT::RECV_A_ASSOCIATE_RQ_PDU:
              // {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::ae6)},
              // {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
              // {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa8)},
              // {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa8)},
              // {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa8)},
              // {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
              // {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
              // {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa8)},
              // {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa8)},
              // {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
              // {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa7)},
              switch (state_)
              {
              case CONN_STATE::STA2:
                  ae6();
                  break;
              case CONN_STATE::STA3:
              case CONN_STATE::STA5:
              case CONN_STATE::STA6:
              case CONN_STATE::STA7:
              case CONN_STATE::STA8:
              case CONN_STATE::STA9:
              case CONN_STATE::STA10:
              case CONN_STATE::STA11:
              case CONN_STATE::STA12:
                  aa8();
                  break;
              case CONN_STATE::STA13:
                  aa7();
                  break;
              default:
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::LOCL_A_ASSOCIATE_AC_PDU:
              //{{StateMachine::EVENT::LOCL_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::ae7)},
              if (state_ == CONN_STATE::STA3) {
                  ae7(); // Placeholder for action, no code inside
              }
              else {
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::LOCL_A_ASSOCIATE_RJ_PDU:
              //{{StateMachine::EVENT::LOCL_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::ae8)},
              if (state_ == CONN_STATE::STA3) {
                  ae8(); // Placeholder for action, no code inside
              }
              else {
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::LOCL_P_DATA_TF_PDU:
              // {{StateMachine::EVENT::LOCL_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::dt1)},
              // {{StateMachine::EVENT::LOCL_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::ar7)},
              switch (state_) {
              case CONN_STATE::STA6:
                  dt1(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA8:
                  ar7(); // Placeholder for action, no code inside
                  break;
              default:
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::RECV_P_DATA_TF_PDU:
              // {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::dt2)},
              //         {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::ar6)},
              //         {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa6)},
              switch (state_) {
              case CONN_STATE::STA2:
                  aa1(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA3:
              case CONN_STATE::STA5:
              case CONN_STATE::STA8:
              case CONN_STATE::STA9:
              case CONN_STATE::STA10:
              case CONN_STATE::STA11:
              case CONN_STATE::STA12:
                  aa8(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA6:
                  dt2(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA7:
                  ar6(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA13:
                  aa6(); // Placeholder for action, no code inside
                  break;
              default:
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::LOCL_A_RELEASE_RQ_PDU:
              //{{StateMachine::EVENT::LOCL_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::ar1)},
              if (state_ == CONN_STATE::STA6) {
                  ar1(); // Placeholder for action, no code inside
              }
              else {
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::RECV_A_RELEASE_RQ_PDU:
              // {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::ar2)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::ar8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa6)},
              switch (state_) {
              case CONN_STATE::STA2:
                  aa1(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA3:
              case CONN_STATE::STA5:
              case CONN_STATE::STA8:
              case CONN_STATE::STA9:
              case CONN_STATE::STA10:
              case CONN_STATE::STA11:
              case CONN_STATE::STA12:
                  aa8(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA6:
                  ar2(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA7:
                  ar8(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA13:
                  aa6(); // Placeholder for action, no code inside
                  break;
              default:
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::RECV_A_RELEASE_RP_PDU:
              // {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::ar3)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::ar10)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::ar3)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa6)},
              switch (state_) {
              case CONN_STATE::STA2:
                  aa1(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA3:
              case CONN_STATE::STA5:
              case CONN_STATE::STA6:
              case CONN_STATE::STA8:
              case CONN_STATE::STA9:
              case CONN_STATE::STA12:
                  aa8(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA7:
              case CONN_STATE::STA11:
                  ar3(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA10:
                  ar10(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA13:
                  aa6(); // Placeholder for action, no code inside
                  break;
              default:
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::LOCL_A_RELEASE_RP_PDU:
              // {{StateMachine::EVENT::LOCL_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::ar4)},
              //         {{StateMachine::EVENT::LOCL_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::ar9)},
              //         {{StateMachine::EVENT::LOCL_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::ar4)},
              switch (state_) {
              case CONN_STATE::STA8:
              case CONN_STATE::STA12:
                  ar4(); // Placeholder for action, no code inside
                  break;
              case CONN_STATE::STA9:
                  ar9(); // Placeholder for action, no code inside
                  break;
              default:
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::LOCL_A_ABORT_PDU:
              // {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA4}, std::mem_fn(&StateMachine::aa2)},
              //         {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa1)},
              switch (state_) {
              case CONN_STATE::STA2:
                  aa1(); // Action for state STA2
                  break;
              case CONN_STATE::STA3:
              case CONN_STATE::STA5:
              case CONN_STATE::STA6:
              case CONN_STATE::STA8:
              case CONN_STATE::STA9:
              case CONN_STATE::STA12:
                  aa8(); // Shared action for multiple states
                  break;
              case CONN_STATE::STA7:
              case CONN_STATE::STA11:
                  ar3(); // Shared action for STA7 and STA11
                  break;
              case CONN_STATE::STA10:
                  ar10(); // Action for state STA10
                  break;
              case CONN_STATE::STA13:
                  aa6(); // Action for state STA13
                  break;
              default:
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::RECV_A_ABORT_PDU:
              // {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa2)},
              //         {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa3)},
              //         {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa3)},
              //         {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa3)},
              //         {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa3)},
              //         {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa3)},
              //         {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa3)},
              //         {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa3)},
              //         {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa3)},
              //         {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa3)},
              //         {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa2)},
              switch (state_) {
              case CONN_STATE::STA2:
                  aa2(); // Specific action for STA2
                  break;
              case CONN_STATE::STA3:
              case CONN_STATE::STA4:
              case CONN_STATE::STA5:
              case CONN_STATE::STA6:
              case CONN_STATE::STA7:
              case CONN_STATE::STA8:
              case CONN_STATE::STA9:
              case CONN_STATE::STA10:
              case CONN_STATE::STA11:
              case CONN_STATE::STA12:
                  aa3(); // Shared action for multiple states
                  break;
              case CONN_STATE::STA13:
                  ar2(); // Specific action for STA13
                  break;
              default:
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::TRANS_CONN_CLOSED:
              // {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa5)},
              //         {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa4)},
              //         {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA4}, std::mem_fn(&StateMachine::aa4)},
              //         {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa4)},
              //         {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa4)},
              //         {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa4)},
              //         {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa4)},
              //         {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa4)},
              //         {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa4)},
              //         {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa4)},
              //         {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa4)},
              //         {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::ar5)},
              switch (state_) {
              case CONN_STATE::STA2:
                  aa5(); // Specific action for STA2
                  break;
              case CONN_STATE::STA3:
              case CONN_STATE::STA4:
              case CONN_STATE::STA5:
              case CONN_STATE::STA6:
              case CONN_STATE::STA7:
              case CONN_STATE::STA8:
              case CONN_STATE::STA9:
              case CONN_STATE::STA10:
              case CONN_STATE::STA11:
              case CONN_STATE::STA12:
                  aa4(); // Shared action for multiple states
                  break;
              case CONN_STATE::STA13:
                  ar5(); // Specific action for STA13
                  break;
              default:
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::ARTIM_EXPIRED:
              // {{StateMachine::EVENT::ARTIM_EXPIRED, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa2)},
              //         {{StateMachine::EVENT::ARTIM_EXPIRED, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa2)},
              switch (state_) {
              case CONN_STATE::STA2:
              case CONN_STATE::STA13:
                  aa2(); // Shared action for STA2 and STA13
                  break;
              default:
                  return CONN_STATE::INV;
              }
              break;
          case EVENT::UNRECOG_PDU:
              // {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa1)},
              //         {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
              //         {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa7)},
              switch (state_) {
              case CONN_STATE::STA2:
                  aa1(); // Specific action for STA2
                  break;
              case CONN_STATE::STA3:
              case CONN_STATE::STA5:
              case CONN_STATE::STA6:
              case CONN_STATE::STA7:
              case CONN_STATE::STA8:
              case CONN_STATE::STA9:
              case CONN_STATE::STA10:
              case CONN_STATE::STA11:
              case CONN_STATE::STA12:
                  aa8(); // Shared action for these states
                  break;
              case CONN_STATE::STA13:
                  aa7(); // Specific action for STA13
                  break;
              default:
                  return CONN_STATE::INV;
              }
              break;
          default:
              return CONN_STATE::INV;
          }
          return state_;
      }
};

}

}

}

#endif // UPPERLAYER_TRANSITIONS_HPP
