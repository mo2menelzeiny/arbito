#include <iostream>
#include <fix8/f8includes.hpp>
#include "LMAX_FIXM_types.hpp"
#include "LMAX_FIXM_router.hpp"
#include "LMAX_FIXM_classes.hpp"
#include "LMAX_FIXM_session.hpp"
#include "LMAX_FIX_types.hpp"
#include "LMAX_FIX_router.hpp"
#include "LMAX_FIX_classes.hpp"
#include "LMAX_FIX_session.hpp"
#include <boost/algorithm/string/find.hpp>
#include <libtrading/proto/fix_session.h>
#include <libtrading/proto/mbt_fix.h>


FIX8::LMAX_FIXM::MarketDataRequest *generate_mdr();

void benchmark();

int main() {

    try {
        FIX8::GlobalLogger::set_global_filename("global-" + std::to_string(std::time(nullptr)) + ".log");
        FIX8::ClientManager cm;

        const std::string LMAX_FIXM_conf_file("./resources/LMAX_FIXM_session_config.xml");
        const std::string LMAX_FIX_conf_file("./resources/LMAX_FIX_session_config.xml");

        XmlElement *root1(XmlElement::Factory(LMAX_FIXM_conf_file));
        XmlElement *root2(XmlElement::Factory(LMAX_FIX_conf_file));

        if (!root1) {
            std::cerr << "Failed to parse " << LMAX_FIXM_conf_file << std::endl;
            return EXIT_FAILURE;
        } else if (!root2) {
            std::cerr << "Failed to parse " << LMAX_FIX_conf_file << std::endl;
            return EXIT_FAILURE;
        }


        cm.add("LMAX_FIXM_DEMO", new FIX8::ReliableClientSession<LMAX_FIXM_session_client>
                (FIX8::LMAX_FIXM::ctx(), LMAX_FIXM_conf_file, "LMAX_FIXM_DEMO"));
        cm.add("LMAX_FIX_DEMO", new FIX8::ReliableClientSession<LMAX_FIX_session_client>
                (FIX8::LMAX_FIX::ctx(), LMAX_FIX_conf_file, "LMAX_FIX_DEMO"));

        std::function<bool(FIX8::ClientSessionBase *pp)> lambda = [&](FIX8::ClientSessionBase *pp) {
            pp->start(false);
            return true;
        };

        FIX8::ClientSessionBase *csb(cm.for_each_if(lambda));

        cm["LMAX_FXM_DEMO"]->session_ptr()->get_connection()->send("asdasd");

        // exit(0);

    } catch (FIX8::f8Exception &e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
    } catch (std::exception &e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION CAUGHT" << std::endl;
    }
    return EXIT_SUCCESS;
}

void benchmark() {
    std::string str("8=FIX.4.4 9=75 35=A 49=LMXBD 56=AhmedDEMO 34=1 52=20180531-04:52:25.328 98=0 108=30 141=Y 10=173");
    long count = 0, sum = 0;
    while (count <= 1000000) {
        count++;
        auto t1 = std::chrono::steady_clock::now();
        auto t2 = std::chrono::steady_clock::now();
        sum += std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    }
    std::cout << "wasted: " << sum / count << std::endl;
    count = 0;
    sum = 0;
    while (count <= 1000000) {
        count++;
        auto t1 = std::chrono::steady_clock::now();
        auto pos = boost::algorithm::find_first(str, "34=").begin() - str.begin();
        auto t2 = std::chrono::steady_clock::now();
        sum += std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
        if (count == 1000000) {
            auto x = pos;
            std::cout << x;
        }
    }

    std::cout << "total: " << sum / count << std::endl;

}

FIX8::LMAX_FIXM::MarketDataRequest *generate_mdr() {
    auto *mdr(new FIX8::LMAX_FIXM::MarketDataRequest);
    *mdr << new FIX8::LMAX_FIXM::MDReqID(std::to_string(std::time(0)))
         << new FIX8::LMAX_FIXM::SubscriptionRequestType(FIX8::LMAX_FIXM::SubscriptionRequestType_SNAPSHOTUPDATE)
         << new FIX8::LMAX_FIXM::MarketDepth(1)
         << new FIX8::LMAX_FIXM::MDUpdateType(FIX8::LMAX_FIXM::MDUpdateType_FULL)
         << new FIX8::LMAX_FIXM::NoMDEntryTypes(2);
    FIX8::GroupBase *nomd(mdr->find_group<FIX8::LMAX_FIXM::MarketDataRequest::NoMDEntryTypes>());
    FIX8::MessageBase *nomd_bid(nomd->create_group());
    *nomd_bid << new FIX8::LMAX_FIXM::MDEntryType(FIX8::LMAX_FIXM::MDEntryType_BID);
    *nomd << nomd_bid;
    FIX8::MessageBase *nomd_ask(nomd->create_group());
    *nomd_ask << new FIX8::LMAX_FIXM::MDEntryType(FIX8::LMAX_FIXM::MDEntryType_OFFER);
    *nomd << nomd_ask;
    *mdr << new FIX8::LMAX_FIXM::NoRelatedSym(1);
    FIX8::GroupBase *noin(mdr->find_group<FIX8::LMAX_FIXM::MarketDataRequest::NoRelatedSym>());
    FIX8::MessageBase *noin_sym(noin->create_group());
    *noin_sym << new FIX8::LMAX_FIXM::SecurityID("4001");
    *noin_sym << new FIX8::LMAX_FIXM::SecurityIDSource(FIX8::LMAX_FIXM::SecurityIDSource_EXCHSYMB);
    *noin << noin_sym;
    return mdr;
}

bool LMAX_FIXM_session_client::handle_application(const unsigned seqnum, const FIX8::Message *&msg) {
    return enforce(seqnum, msg) || msg->process(_router);
}

bool LMAX_FIXM_router_client::operator()(const FIX8::LMAX_FIXM::MarketDataSnapshotFullRefresh *msg) const {
    FIX8::Message::report_codec_timings("benchmarks");
    return true;
}

bool LMAX_FIX_session_client::handle_application(const unsigned seqnum, const FIX8::Message *&msg) {
    return enforce(seqnum, msg) || msg->process(_router);
}

FIX8::Message *LMAX_FIXM_session_client::generate_logon(const unsigned heartbeat_interval, const FIX8::f8String davi) {
    FIX8::Message *msg(Session::generate_logon(heartbeat_interval, davi));
    *msg += new FIX8::LMAX_FIXM::Username("AhmedDEMO");
    *msg += new FIX8::LMAX_FIXM::Password("password1");
    return msg;
}

bool LMAX_FIXM_session_client::process(const FIX8::f8String &from) {
    using namespace FIX8;
    unsigned seqnum(0);
    bool remote_logged_out{};
    const Message *msg = nullptr;
    std::cout << _sid << "\n";
    try {
        const f8String::size_type fpos(from.find("34="));
        if (fpos == f8String::npos) {
            slout_debug << "Session::process throwing for " << from;
            throw InvalidMessage(from, FILE_LINE);
        }

        seqnum = fast_atoi<unsigned>(from.data() + fpos + 3, default_field_separator);

        bool retry_plog(false);
        if (_plogger && _plogger->has_flag(Logger::inbound)) {
            if (_state != States::st_wait_for_logon)
                plog(from, Logger::Info, 1);
            else
                retry_plog = true;
        }

        if (!(msg = Message::factory(_ctx, from, _loginParameters._no_chksum_flag,
                                     _loginParameters._permissive_mode_flag))) {
            glout_fatal << "Fatal: factory failed to generate a valid message";
            return false;
        }

        if ((_control & printnohb) && msg->get_msgtype() != Common_MsgType_HEARTBEAT)
            std::cout << *msg << std::endl;
        else if (_control & print)
            std::cout << *msg << std::endl;

        bool result(false), admin_result(msg->is_admin() ? handle_admin(seqnum, msg) : true);
        if (msg->get_msgtype().size() > 1)
            goto application_call;
        else
            switch (msg->get_msgtype()[0]) {
                default:
                application_call:
                    if (activation_check(seqnum, msg))
                        result = handle_application(seqnum, msg);
                    break;
                case Common_MsgByte_HEARTBEAT:
                    result = handle_heartbeat(seqnum, msg);
                    break;
                case Common_MsgByte_TEST_REQUEST:
                    result = handle_test_request(seqnum, msg);
                    break;
                case Common_MsgByte_RESEND_REQUEST:
                    result = handle_resend_request(seqnum, msg);
                    break;
                case Common_MsgByte_REJECT:
                    result = handle_reject(seqnum, msg);
                    break;
                case Common_MsgByte_SEQUENCE_RESET:
                    result = handle_sequence_reset(seqnum, msg);
                    break;
                case Common_MsgByte_LOGOUT:
                    result = handle_logout(seqnum, msg);
                    remote_logged_out = true;
                    break;
                case Common_MsgByte_LOGON:
                    result = handle_logon(seqnum, msg);
                    break;
            }

        ++_next_receive_seq;
        if (retry_plog)
            plog(from, Logger::Info, 1);

        update_persist_seqnums();

        if (remote_logged_out) // FX-615; permit logout seqnum to be persisted
        {
            slout_debug << "logout received from remote";
            stop();
        }

        delete msg;
        return result && admin_result;
    }
    catch (LogfileException &e) {
        std::cerr << e.what() << std::endl;
    }
    catch (f8Exception &e) {
        slout_debug << "process: f8exception" << ' ' << seqnum << ' ' << e.what();

        if (e.force_logoff()) {
            if (_plogger && _plogger->has_flag(Logger::inbound))
                plog(from, Logger::Info, 1);
            slout_fatal << e.what() << " - will logoff";
            if (_state == States::st_logon_received && !_loginParameters._silent_disconnect) {
                do_state_change(States::st_session_terminated);
                send(generate_logout(e.what()), true, 0, true); // so it won't increment
                do_state_change(States::st_logoff_sent);
            }
            if (_loginParameters._reliable) {
                slout_debug << "rethrowing ...";
                throw;
            }
            stop();
        } else {
            slout_error << e.what() << " - inbound message rejected";
            handle_outbound_reject(seqnum, msg, e.what());
            ++_next_receive_seq;
            if (_plogger && _plogger->has_flag(Logger::inbound))
                plog(from, Logger::Info, 1);
            delete msg;
            return true; // message is handled but has errors
        }
    }
    catch (Poco::Net::NetException &e) {
        slout_debug << "process:: Poco::Net::NetException";
        slout_error << e.what();
    }
    catch (std::exception &e) {
        slout_debug << "process:: std::exception";
        slout_error << e.what();
    }

    return false;
}

FIX8::Message *LMAX_FIX_session_client::generate_logon(const unsigned heartbeat_interval, const FIX8::f8String davi) {
    FIX8::Message *msg(Session::generate_logon(heartbeat_interval, davi));
    *msg += new FIX8::LMAX_FIX::Username("AhmedDEMO");
    *msg += new FIX8::LMAX_FIX::Password("password1");
    return msg;
}
