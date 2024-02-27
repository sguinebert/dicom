#include "upperlayer_statemachine.hpp"

#include <map>
#include <functional>

#include <boost/log/trivial.hpp>

#include "upperlayer_properties.hpp"
#include "upperlayer_connection.hpp"

namespace dicom
{

namespace network
{

namespace upperlayer
{

using namespace dicom::util::log;




// statemachine::CONN_STATE statemachine::get_state()
// {
//    return state;
// }

// statemachine::CONN_STATE statemachine::transition(EVENT e)
// {
//    if (transition_table.find({e, state}) != transition_table.end()) {
//       transition_table[{e, state}](this);
//       return state;
//    } else {
//       return CONN_STATE::INV;
//    }
// }
//using namespace statemachine::EVENT;


/**
 * @brief contains the target state as a function of pair of the current state and a primitive received from the user
 *
 */
std::map<std::pair<StateMachine::EVENT, StateMachine::CONN_STATE>, std::function<void(StateMachine*)>> StateMachine::transition_table {
    {{StateMachine::EVENT::A_ASSOCIATE_RQ, StateMachine::CONN_STATE::STA1}, std::mem_fn(&StateMachine::ae1)},

    {{StateMachine::EVENT::TRANS_CONN_CONF, StateMachine::CONN_STATE::STA4}, std::mem_fn(&StateMachine::ae2)},

    {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::ae3)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa6)},

    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::ae4)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa6)},

    {{StateMachine::EVENT::TRANS_CONN_INDIC, StateMachine::CONN_STATE::STA1}, std::mem_fn(&StateMachine::ae5)},

    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::ae6)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_ASSOCIATE_RQ_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa7)},

    {{StateMachine::EVENT::LOCL_A_ASSOCIATE_AC_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::ae7)},

    {{StateMachine::EVENT::LOCL_A_ASSOCIATE_RJ_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::ae8)},

    {{StateMachine::EVENT::LOCL_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::dt1)},
    {{StateMachine::EVENT::LOCL_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::ar7)},

    {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::dt2)},
    {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::ar6)},
    {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_P_DATA_TF_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa6)},

    {{StateMachine::EVENT::LOCL_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::ar1)},

    {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::ar2)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::ar8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RQ_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa6)},

    {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::ar3)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::ar10)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::ar3)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::RECV_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa6)},

    {{StateMachine::EVENT::LOCL_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::ar4)},
    {{StateMachine::EVENT::LOCL_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::ar9)},
    {{StateMachine::EVENT::LOCL_A_RELEASE_RP_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::ar4)},

    {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA4}, std::mem_fn(&StateMachine::aa2)},
    {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::LOCL_A_ABORT_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa1)},

    {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa2)},
    {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa3)},
    {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa3)},
    {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa3)},
    {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa3)},
    {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa3)},
    {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa3)},
    {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa3)},
    {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa3)},
    {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa3)},
    {{StateMachine::EVENT::RECV_A_ABORT_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa2)},

    {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa5)},
    {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa4)},
    {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA4}, std::mem_fn(&StateMachine::aa4)},
    {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa4)},
    {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa4)},
    {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa4)},
    {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa4)},
    {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa4)},
    {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa4)},
    {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa4)},
    {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa4)},
    {{StateMachine::EVENT::TRANS_CONN_CLOSED, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::ar5)},

    {{StateMachine::EVENT::ARTIM_EXPIRED, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa2)},
    {{StateMachine::EVENT::ARTIM_EXPIRED, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa2)},

    {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA2}, std::mem_fn(&StateMachine::aa1)},
    {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA3}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA5}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA6}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA7}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA8}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA9}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA10}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA11}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA12}, std::mem_fn(&StateMachine::aa8)},
    {{StateMachine::EVENT::UNRECOG_PDU, StateMachine::CONN_STATE::STA13}, std::mem_fn(&StateMachine::aa7)},
};

}

}

}

