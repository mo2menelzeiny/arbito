
#include "MarketDataClient.h"


/*
#include <Aeron.h>
#include <concurrent/BusySpinIdleStrategy.h>
#include <Configuration.h>
#include <FragmentAssembler.h>
#include <aeronmd/aeronmd.h>
#include <aeronmd/concurrent/aeron_atomic64_gcc_x86_64.h>
*/




/*
volatile bool running_md = true;

inline bool aeron_is_running() {
    bool result;
    AERON_GET_VOLATILE(result, running_md);
    return result;
}

int aeron_driver() {

    int status = EXIT_FAILURE;
    aeron_driver_context_t *context = NULL;
    aeron_driver_t *driver = NULL;

    if (aeron_driver_context_init(&context) < 0) {
        fprintf(stderr, "ERROR: context init (%d) %s\n", aeron_errcode(), aeron_errmsg());
        goto cleanup;
    }

    if (aeron_driver_init(&driver, context) < 0) {
        fprintf(stderr, "ERROR: driver init (%d) %s\n", aeron_errcode(), aeron_errmsg());
        goto cleanup;
    }

    if (aeron_driver_start(driver, true) < 0) {
        fprintf(stderr, "ERROR: driver start (%d) %s\n", aeron_errcode(), aeron_errmsg());
        goto cleanup;
    }

    while (aeron_is_running()) {
        aeron_driver_main_idle_strategy(driver, aeron_driver_main_do_work(driver));
    }

    printf("Shutting down driver...\n");

    cleanup:

    aeron_driver_close(driver);
    aeron_driver_context_close(context);

    return status;
}

*/


/*struct TestEvent {
    long value;
};

struct EngineEventHandler : Disruptor::IEventHandler<TestEvent> {
    void onEvent(TestEvent &event, int64_t, bool) override {
        std::cout << "Engine: " << event.value << "\n";
    }
};

struct AeronEventHandler : Disruptor::IEventHandler<TestEvent>, Disruptor::ILifecycleAware {
    void onEvent(TestEvent &event, int64_t, bool) override {
        std::cout << "Aeron: " << event.value << "\n";
    }

    void onStart() override {
        m_aeron = aeron::Aeron::connect(m_context);
    }

    void onShutdown() override {

    }

private:
    aeron::Context m_context;
    std::shared_ptr<aeron::Aeron> m_aeron;
};

using namespace aeron;
using namespace std::chrono;

*/

/*int sub() {

    try {
        float sum = 0;
        long count = 0;

        std::cout << "Subscribing to channel " << getenv("SUB_CHANNEL") << " on Stream ID " << getenv("SUB_STREAM_ID")
                  << std::endl;

        aeron::Context context;

        context.newSubscriptionHandler(
                [](const std::string &channel, std::int32_t streamId, std::int64_t correlationId) {
                    std::cout << "Subscription: " << channel << " " << correlationId << ":" << streamId << std::endl;
                });

        context.availableImageHandler([](Image &image) {
            std::cout << "Available image correlationId=" << image.correlationId() << " sessionId="
                      << image.sessionId();
            std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
        });

        context.unavailableImageHandler([](Image &image) {
            std::cout << "Unavailable image on correlationId=" << image.correlationId() << " sessionId="
                      << image.sessionId();
            std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
        });

        std::shared_ptr<Aeron> aeron = Aeron::connect(context);

        // add the subscription to start the process
        std::int64_t id = aeron->addSubscription(getenv("SUB_CHANNEL"), atoi(getenv("SUB_STREAM_ID")));

        std::shared_ptr<Subscription> subscription = aeron->findSubscription(id);
        // wait for the subscription to be valid
        while (!subscription) {
            std::this_thread::yield();
            subscription = aeron->findSubscription(id);
        }

        fragment_handler_t handler = [&](const AtomicBuffer &buffer, util::index_t offset, util::index_t length,
                                         const Header &header) {
            steady_clock::time_point rec_time = steady_clock::now();
            steady_clock::time_point send_time;
            buffer.getBytes(offset, (uint8_t *) &send_time, sizeof(steady_clock::time_point));
            ++count;
            sum += duration_cast<nanoseconds>(rec_time - send_time).count();

            if (count > 1000) {
                const float result = sum / count / 1000000;
                std::cout << std::fixed;
                std::cout << std::setprecision(6);
                std::cout << "average duration: " << result << " ms/msg " << "Batch: " << count << "\n";
                count = 0;
                sum = 0;
            }
        };

        BusySpinIdleStrategy idleStrategy;

        while (running) {
            idleStrategy.idle(subscription->poll(handler, 10));
        }

    }
    catch (const SourcedException &e) {
        std::cerr << "FAILED: " << e.what() << " : " << e.where() << std::endl;
        return -1;
    }
    catch (const std::exception &e) {
        std::cerr << "FAILED: " << e.what() << " : " << std::endl;
        return -1;
    }

    return 0;
}

int pub() {
    try {
        std::cout << "Publishing to channel " << getenv("PUB_CHANNEL") << " on Stream ID " << getenv("PUB_STREAM_ID")
                  << std::endl;

        aeron::Context context;

        context.newPublicationHandler(
                [](const std::string &channel, std::int32_t streamId, std::int32_t sessionId,
                   std::int64_t correlationId) {
                    std::cout << "Publication: " << channel << " " << correlationId << ":" << streamId << ":"
                              << sessionId << std::endl;
                });

        context.availableImageHandler([](Image &image) {
            std::cout << "Available image correlationId=" << image.correlationId() << " sessionId="
                      << image.sessionId();
            std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
        });

        context.unavailableImageHandler([](Image &image) {
            std::cout << "Unavailable image on correlationId=" << image.correlationId() << " sessionId="
                      << image.sessionId();
            std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
        });

        std::shared_ptr<Aeron> aeron = Aeron::connect(context);

        // add the publication to start the process
        std::int64_t id = aeron->addPublication(getenv("PUB_CHANNEL"), atoi(getenv("PUB_STREAM_ID")));

        std::shared_ptr<Publication> publication = aeron->findPublication(id);
        // wait for the publication to be valid
        while (!publication) {
            std::this_thread::yield();
            publication = aeron->findPublication(id);
        }

        std::unique_ptr<std::uint8_t[]> buffer(new std::uint8_t[256]);
        concurrent::AtomicBuffer srcBuffer(buffer.get(), 256);

        do {
            std::chrono::steady_clock::time_point send_time = std::chrono::steady_clock::now();
            srcBuffer.putBytes(0, (uint8_t *) &send_time, sizeof(std::chrono::steady_clock::time_point));
            const std::int64_t result = publication->offer(srcBuffer, 0, sizeof(srcBuffer));

            if (result < 0) {
                if (BACK_PRESSURED == result) {
                    std::cout << "Offer failed due to back pressure" << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                } else if (NOT_CONNECTED == result) {
                    std::cout << "Offer failed because publisher is not connected to subscriber" << std::endl;
                } else if (ADMIN_ACTION == result) {
                    std::cout << "Offer failed because of an administration action in the system" << std::endl;
                } else if (PUBLICATION_CLOSED == result) {
                    std::cout << "Offer failed publication is closed" << std::endl;
                } else {
                    std::cout << "Offer failed due to unknown reason" << result << std::endl;
                }
            }

            if (!publication->isConnected()) {
                std::cout << "No active subscribers detected" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(atoi(getenv("RATE_MS"))));
        } while (running);
    }
    catch (const SourcedException &e) {
        std::cerr << "FAILED: " << e.what() << " : " << e.where() << std::endl;
        return -1;
    }
    catch (const std::exception &e) {
        std::cerr << "FAILED: " << e.what() << " : " << std::endl;
        return -1;
    }
}*/

int main() {

    try {
        std::shared_ptr<LMAX::MarketDataClient> lmax_md_client = std::make_shared<LMAX::MarketDataClient>(
                "fix-marketdata.london-demo.lmax.com", 443, "AhmedDEMO", "password1", "AhmedDEMO", "LMXBDM", 5);
        lmax_md_client->start();
        lmax_md_client->handle();

    } catch (const std::exception &e) {
        std::cerr << "FAILED: " << e.what() << " : " << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}