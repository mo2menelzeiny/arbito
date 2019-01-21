
#ifndef ARBITO_BUSINESSOFFICE_H
#define ARBITO_BUSINESSOFFICE_H

// Disruptor
#include <Disruptor/Disruptor.h>

// SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

// Domain
#include "BusinessEvent.h"
#include "MarketEvent.h"
#include "TriggerDifference.h"
#include "MongoDBDriver.h"

using namespace std;
using namespace std::chrono;

class BusinessOffice {
public:
	BusinessOffice(
			std::shared_ptr<Disruptor::RingBuffer<MarketEvent>> &inRingBuffer,
			std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &outRingBuffer,
			int orderDelaySec,
			int maxOrders,
			double diffOpen,
			double diffClose,
			const char *dbUri,
			const char *dbName
	);

	inline void doWork() {
		m_marketEventPoller->poll(m_marketEventHandler);

		if (m_isOrderDelayed && ((time(nullptr) - m_lastOrderTime) < m_orderDelaySec)) {
			return;
		}

		m_isOrderDelayed = false;

		bool canOpen = m_ordersCount < m_maxOrders;

		double diffA = m_marketDataA.bid - m_marketDataBTrunc.offer;
		double diffB = m_marketDataBTrunc.bid - m_marketDataA.offer;

		switch (m_currentDiff) {
			case DIFF_A:
				if (canOpen && diffA >= m_diffOpen) {
					m_currentOrder = DIFF_A;
					++m_ordersCount;
					break;
				}

				if (diffB >= m_diffClose) {
					m_currentOrder = DIFF_B;
					--m_ordersCount;
					break;
				}

				break;

			case DIFF_B:
				if (canOpen && diffB >= m_diffOpen) {
					m_currentOrder = DIFF_B;
					++m_ordersCount;
					break;
				}

				if (diffA >= m_diffClose) {
					m_currentOrder = DIFF_A;
					--m_ordersCount;
					break;
				}

				break;

			case DIFF_NONE:
				if (diffA >= m_diffOpen) {
					m_currentDiff = DIFF_A;
					m_currentOrder = DIFF_A;
					++m_ordersCount;
					break;
				}

				if (diffB >= m_diffOpen) {
					m_currentDiff = DIFF_B;
					m_currentOrder = DIFF_B;
					++m_ordersCount;
					break;
				}

				break;

			default:
				break;

		}

		if (m_currentOrder == DIFF_NONE) {
			return;
		}

		const char *orderType = m_currentOrder == m_currentDiff ? "OPEN" : "CLOSE";

		if (m_ordersCount == 0) {
			m_currentDiff = DIFF_NONE;
		}

		auto randomId = m_idGenerator();
		sprintf(m_randomIdStrBuff, "%lu", randomId);

		auto nextSequence = m_outRingBuffer->next();
		(*m_outRingBuffer)[nextSequence].id = randomId;

		switch (m_currentOrder) {
			case DIFF_A:

				(*m_outRingBuffer)[nextSequence].sell = m_marketDataA.broker;
				(*m_outRingBuffer)[nextSequence].buy = m_marketDataB.broker;
				m_outRingBuffer->publish(nextSequence);

				m_systemLogger->info(
						"{} bid A: {} sequence A: {} offer B: {} sequence B: {}",
						orderType,
						m_marketDataA.bid,
						m_marketDataA.sequence,
						m_marketDataB.offer,
						m_marketDataB.sequence
				);

				std::thread([mongoDriver = &m_mongoDriver,
						            id = m_randomIdStrBuff,
						            a = m_marketDataA,
						            b = m_marketDataB,
						            orderType] {
					mongoDriver->record(id, a.bid, b.offer, orderType);
				}).detach();

				break;

			case DIFF_B:
				(*m_outRingBuffer)[nextSequence].sell = m_marketDataB.broker;
				(*m_outRingBuffer)[nextSequence].buy = m_marketDataA.broker;
				m_outRingBuffer->publish(nextSequence);

				m_systemLogger->info(
						"{} bid B: {} sequence B: {} offer A: {} sequence A: {}",
						orderType,
						m_marketDataB.bid,
						m_marketDataB.sequence,
						m_marketDataA.offer,
						m_marketDataA.sequence
				);

				std::thread([mongoDriver = &m_mongoDriver,
						            id = m_randomIdStrBuff,
						            a = m_marketDataA,
						            b = m_marketDataB,
						            orderType] {
					mongoDriver->record(id, b.bid, a.offer, orderType);
				}).detach();

				break;

			default:
				break;
		}

		m_currentOrder = DIFF_NONE;

		m_marketDataA.bid = -99;
		m_marketDataA.offer = 99;

		m_marketDataB.bid = -99;
		m_marketDataB.offer = 99;

		m_marketDataBTrunc.bid = -99;
		m_marketDataBTrunc.offer = 99;

		m_lastOrderTime = time(nullptr);
		m_isOrderDelayed = true;

		m_consoleLogger->info("{}", orderType);
	}

	void initiate();

	void terminate();

private:
	std::shared_ptr<Disruptor::RingBuffer<MarketEvent>> m_inRingBuffer;
	std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> m_outRingBuffer;
	int m_orderDelaySec;
	int m_maxOrders;
	double m_diffOpen;
	double m_diffClose;
	MongoDBDriver m_mongoDriver;
	std::shared_ptr<spdlog::logger> m_consoleLogger;
	std::shared_ptr<spdlog::logger> m_systemLogger;
	std::shared_ptr<Disruptor::EventPoller<MarketEvent>> m_marketEventPoller;
	std::function<bool(MarketEvent &, long, bool)> m_marketEventHandler;
	MarketEvent m_marketDataA;
	MarketEvent m_marketDataB;
	MarketEvent m_marketDataBTrunc;
	int m_ordersCount;
	TriggerDifference m_currentDiff;
	TriggerDifference m_currentOrder;
	std::mt19937_64 m_idGenerator;
	char m_randomIdStrBuff[64];
	char m_truncStrBuff[9];
	bool m_isOrderDelayed;
	time_t m_lastOrderTime;
};


#endif //ARBITO_BUSINESSOFFICE_H
