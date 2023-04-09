
#pragma once

#include <cstdint>
#include "utils/thirdparty/quiche/quic_clock.h"
#include "utils/thirdparty/quiche/quic_time.h"
#include "demo/congestioncontrol.hpp"
#include "thirdparty/quiche/cubic_bytes.h"

struct CubicCongestionCtlConfig {
};

class CubicCongestionContrl : public CongestionCtlAlgo {
public:

    explicit CubicCongestionContrl(const CubicCongestionCtlConfig &ccConfig) : cubic_(DefaultClock::GetClock()) {
        cubic_.ResetCubicState();
        cubic_.SetNumConnections(quic::kDefaultNumConnections);
    }

    ~CubicCongestionContrl() override {
        SPDLOG_DEBUG("");
    }

    CongestionCtlType GetCCtype() override {
        return CongestionCtlType::cubic;
    }

    void OnDataSent(const InflightPacket &sentpkt) override {
        SPDLOG_TRACE("");
        // do nothing
    }

    void OnDataAckOrLoss(const AckEvent &ackEvent, const LossEvent &lossEvent, RttStats &rttstats) override {
        SPDLOG_TRACE("ackevent:{}, lossevent:{}", ackEvent.DebugInfo(), lossEvent.DebugInfo());
        if (lossEvent.valid) {
            SPDLOG_TRACE("lossEvent");
            m_cwnd = cubic_.CongestionWindowAfterPacketLoss(m_cwnd);
        }

        if (ackEvent.valid) {
            SPDLOG_TRACE("ackEvent");
            auto _now = DefaultClock::GetClock()->ApproximateNow();
            m_cwnd = cubic_.CongestionWindowAfterAck(1000, m_cwnd,
                                            _now - ackEvent.sendtic, _now);
        }
    }

    uint32_t GetCWND() override {
        SPDLOG_DEBUG(" {}", m_cwnd);
        return m_cwnd/1000;
    }

private:

    quic::CubicBytes cubic_;

    uint32_t m_cwnd{1000};
    uint32_t m_cwndCnt{0}; /** in congestion avoid phase, used for counting ack packets*/
//    Timepoint lastLagestLossPktSentTic{ Timepoint::Zero() };

    uint32_t m_minCwnd{1};
    uint32_t m_maxCwnd{64};
    uint32_t m_ssThresh{32};/** slow start threshold*/

};