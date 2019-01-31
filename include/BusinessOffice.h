
#ifndef ARBITO_BUSINESSOFFICE_H
#define ARBITO_BUSINESSOFFICE_H

#define CORRECTION_ID 99699

// Disruptor
#include <Disruptor/Disruptor.h>

// SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

// Domain
#include "OrderEvent.h"
#include "PriceEvent.h"
#include "ExecutionEvent.h"
#include "Difference.h"
#include "MongoDBDriver.h"

using namespace std;
using namespace std::chrono;

class BusinessOffice {
public:
	BusinessOffice(
			std::shared_ptr<Disruptor::RingBuffer<PriceEvent>> &priceRingBuffer,
			std::shared_ptr<Disruptor::RingBuffer<ExecutionEvent>> &executionRingBuffer,
			std::shared_ptr<Disruptor::RingBuffer<OrderEvent>> &orderRingBuffer,
			int orderDelaySec,
			int maxOrders,
			double diffOpen,
			double diffClose,
			const char *dbUri,
			const char *dbName
	);

	inline void doWork() {
		m_timestampNow = system_clock::now();

		m_priceEventPoller->poll(m_priceEventHandler);
		m_executionEventPoller->poll(m_executionEventHandler);

		/*if(m_isExpiredB) {
			return;
		}

		if (!m_isExpiredB && duration_cast<milliseconds>(m_timestampNow - m_timestampB).count() >= m_windowMs) {
			m_isExpiredB = true;
			m_priceB.bid = -99;
			m_priceB.offer = 99;
			m_priceBTrunc.bid = -99;
			m_priceBTrunc.offer = 99;
			return;
		}*/

		if (m_canLogPrices) {
			++m_sequence;

			m_systemLogger->info(
					"[{}] Bid/Ask A:{}/{} SeqA:{} Bid/Ask B:{}/{} SeqB:{} DiffA/DiffB:{}/{}",
					m_sequence,
					m_priceA.bid, m_priceA.offer, m_priceA.sequence,
					m_priceB.bid, m_priceB.offer, m_priceB.sequence,
					(m_priceA.bid - m_priceB.offer) * 100000,
					(m_priceB.bid - m_priceA.offer) * 100000
			);

			m_canLogPrices = false;
		}

		if (m_isOrderDelayed && ((time(nullptr) - m_lastOrderTime) < m_orderDelaySec)) {
			return;
		}

		m_isOrderDelayed = false;

		if (m_ordersCount % 2 != 0) {
			return;
		}

		bool canOpen = m_ordersCount < m_maxOrders;

		double diffA = m_priceA.bid - m_priceB.offer;
		double diffB = m_priceB.bid - m_priceA.offer;

		char orderType[16];

		switch (m_currentDiff) {
			case Difference::A:
				if (canOpen && diffA >= m_diffOpen) {
					m_currentOrder = Difference::A;
					sprintf(orderType, "%s", "OPEN");
					break;
				}

				if (diffB >= m_diffClose) {
					m_currentOrder = Difference::B;
					sprintf(orderType, "%s", "CLOSE");
					break;
				}

				break;

			case Difference::B:
				if (canOpen && diffB >= m_diffOpen) {
					m_currentOrder = Difference::B;
					sprintf(orderType, "%s", "OPEN");
					break;
				}

				if (diffA >= m_diffClose) {
					m_currentOrder = Difference::A;
					sprintf(orderType, "%s", "CLOSE");
					break;
				}

				break;

			case Difference::NONE:
				if (diffA >= m_diffOpen) {
					m_currentDiff = Difference::A;
					m_currentOrder = Difference::A;
					sprintf(orderType, "%s", "OPEN");
					break;
				}

				if (diffB >= m_diffOpen) {
					m_currentDiff = Difference::B;
					m_currentOrder = Difference::B;
					sprintf(orderType, "%s", "OPEN");
					break;
				}

				break;

			default:
				break;

		}

		if (m_ordersCount == 0) {
			m_currentDiff = Difference::NONE;
		}

		if (m_currentOrder == Difference::NONE) {
			return;
		}

		auto randomId = m_idGenerator();
		sprintf(m_randomIdStrBuff, "%lu", randomId);

		switch (m_currentOrder) {
			case Difference::A: {
				auto nextSeqA = m_orderRingBuffer->next();
				(*m_orderRingBuffer)[nextSeqA] = {
						m_priceA.broker,
						OrderType::MARKET,
						OrderSide::SELL,
						m_priceA.bid,
						randomId
				};
				m_orderRingBuffer->publish(nextSeqA);

				auto nextSeqB = m_orderRingBuffer->next();
				(*m_orderRingBuffer)[nextSeqB] = {
						m_priceB.broker,
						OrderType::LIMIT,
						OrderSide::BUY,
						m_priceB.offer,
						randomId
				};
				m_orderRingBuffer->publish(nextSeqB);

				m_systemLogger->info(
						"{} bid A: {} sequence A: {} offer B: {} sequence B: {}",
						orderType,
						m_priceA.bid,
						m_priceA.sequence,
						m_priceB.offer,
						m_priceB.sequence
				);

				std::thread([mongoDriver = &m_mongoDriver,
						            id = m_randomIdStrBuff,
						            bid = m_priceA.bid,
						            offer = m_priceB.offer,
						            type = orderType] {
					mongoDriver->record(id, bid, offer, type);
				}).detach();
			}
				break;

			case Difference::B: {
				auto nextSeqA = m_orderRingBuffer->next();
				(*m_orderRingBuffer)[nextSeqA] = {
						m_priceA.broker,
						OrderType::MARKET,
						OrderSide::BUY,
						m_priceA.offer,
						randomId
				};
				m_orderRingBuffer->publish(nextSeqA);

				auto nextSeqB = m_orderRingBuffer->next();
				(*m_orderRingBuffer)[nextSeqB] = {
						m_priceB.broker,
						OrderType::LIMIT,
						OrderSide::SELL,
						m_priceB.bid,
						randomId
				};
				m_orderRingBuffer->publish(nextSeqB);

				m_systemLogger->info(
						"{} bid B: {} sequence B: {} offer A: {} sequence A: {}",
						orderType,
						m_priceB.bid,
						m_priceB.sequence,
						m_priceA.offer,
						m_priceA.sequence
				);

				std::thread([mongoDriver = &m_mongoDriver,
						            id = m_randomIdStrBuff,
						            bid = m_priceB.bid,
						            offer = m_priceA.offer,
						            type = orderType] {
					mongoDriver->record(id, bid, offer, type);
				}).detach();
			}
				break;

			default:
				break;
		}

		m_currentOrder = Difference::NONE;

		m_priceA.bid = -99;
		m_priceA.offer = 99;

		m_priceB.bid = -99;
		m_priceB.offer = 99;

		m_priceBTrunc.bid = -99;
		m_priceBTrunc.offer = 99;

		m_lastOrderTime = time(nullptr);
		m_isOrderDelayed = true;

		m_consoleLogger->info("{}", orderType);
	}

	void initiate();

	void terminate();

private:
	std::shared_ptr<Disruptor::RingBuffer<PriceEvent>> m_priceRingBuffer;
	std::shared_ptr<Disruptor::RingBuffer<ExecutionEvent>> m_executionRingBuffer;
	std::shared_ptr<Disruptor::RingBuffer<OrderEvent>> m_orderRingBuffer;
	int m_orderDelaySec;
	int m_windowMs;
	int m_maxOrders;
	double m_diffOpen;
	double m_diffClose;
	MongoDBDriver m_mongoDriver;
	std::shared_ptr<spdlog::logger> m_consoleLogger;
	std::shared_ptr<spdlog::logger> m_systemLogger;
	std::shared_ptr<Disruptor::EventPoller<PriceEvent>> m_priceEventPoller;
	std::function<bool(PriceEvent &, long, bool)> m_priceEventHandler;
	std::shared_ptr<Disruptor::EventPoller<ExecutionEvent>> m_executionEventPoller;
	std::function<bool(ExecutionEvent &, long, bool)> m_executionEventHandler;
	PriceEvent m_priceA;
	PriceEvent m_priceB;
	PriceEvent m_priceBTrunc;
	int m_ordersCount;
	Difference m_currentDiff;
	Difference m_currentOrder;
	std::mt19937_64 m_idGenerator;
	char m_randomIdStrBuff[64];
	char m_truncStrBuff[16];
	bool m_isOrderDelayed;
	time_t m_lastOrderTime;
	bool m_isExpiredB;
	time_point<system_clock> m_timestampB;
	time_point<system_clock> m_timestampNow;
	bool m_canLogPrices;
	long m_sequence;
};


#endif //ARBITO_BUSINESSOFFICE_H
