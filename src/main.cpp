#include <iostream>
#include <ctime>
#include <csignal>
#include <fix8/f8includes.hpp>
#include <fix8/multisession.hpp>
#include "LMAX_FIXM_types.hpp"
#include "LMAX_FIXM_router.hpp"
#include "LMAX_FIXM_classes.hpp"
#include "LMAX_FIXM_session.hpp"

FIX8::LMAX_FIXM::MarketDataRequest *generate_mdr();

int main() {
    try {
        FIX8::GlobalLogger::set_global_filename("global-" + std::to_string(std::time(nullptr)) + ".log");
        const std::string conf_file("./resources/LMAX_FIXM_session_config.xml");
        XmlElement *root(XmlElement::Factory(conf_file));
        if (!root) {
            std::cerr << "Failed to parse " << conf_file << std::endl;
            return EXIT_FAILURE;
        }
        std::unique_ptr<FIX8::ClientSessionBase> mc(new FIX8::ReliableClientSession<LMAX_FIXM_session_client>
                (FIX8::LMAX_FIXM::ctx(), conf_file, "LMAX_FIXM_DEMO"));
        mc->start(false);

        while (!mc->has_given_up()) {
            if(mc->session_ptr()->get_session_state() == FIX8::States::SessionStates::st_continuous) {
                mc->session_ptr()->send(generate_mdr());
                std::cout << "Market data subscription sent" << std::endl;
                break;
            }
        };

        while(!mc->has_given_up()){};

    } catch (FIX8::f8Exception &e) {
        std::cerr << e.what() << std::endl;
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION CAUGHT" << std::endl;
    }
    return EXIT_SUCCESS;
}

bool LMAX_FIXM_session_client::handle_application(const unsigned seqnum, const FIX8::Message *&msg) {
    return enforce(seqnum, msg) || msg->process(_router);
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
    *nomd_bid << new FIX8::LMAX_FIXM::MDEntryType(FIX8::LMAX_FIXM::MDEntryType_BID); // bids
    *nomd << nomd_bid;
    FIX8::MessageBase *nomd_ask(nomd->create_group());
    *nomd_ask << new FIX8::LMAX_FIXM::MDEntryType(FIX8::LMAX_FIXM::MDEntryType_OFFER); // offers
    *nomd << nomd_ask;
    *mdr << new FIX8::LMAX_FIXM::NoRelatedSym(1);
    FIX8::GroupBase *noin(mdr->find_group<FIX8::LMAX_FIXM::MarketDataRequest::NoRelatedSym>());
    FIX8::MessageBase *noin_sym(noin->create_group());
    *noin_sym << new FIX8::LMAX_FIXM::SecurityID("4001");
    *noin_sym << new FIX8::LMAX_FIXM::SecurityIDSource(FIX8::LMAX_FIXM::SecurityIDSource_EXCHSYMB);
    *noin << noin_sym;
    return mdr;
}

FIX8::Message *LMAX_FIXM_session_client::generate_logon(const unsigned heartbeat_interval,
                                                        const FIX8::f8String davi) {
    FIX8::Message *msg(Session::generate_logon(heartbeat_interval, davi));
    *msg += new FIX8::LMAX_FIXM::Username("AhmedDEMO");
    *msg += new FIX8::LMAX_FIXM::Password("password1");
    return msg;
}
bool LMAX_FIXM_router_client::operator()(const FIX8::LMAX_FIXM::MarketDataSnapshotFullRefresh *msg) const {
    FIX8::Message::report_codec_timings("benchmarks");
    return true;
}