
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
//            m_cwnd = cubic_.CongestionWindowAfterPacketLoss(m_cwnd * 1000)/1000;
            m_kcwnd = cubic_.CongestionWindowAfterPacketLoss(m_kcwnd);
            m_ssThresh = m_cwnd;
        }

        if (ackEvent.valid) {
            SPDLOG_TRACE("ackEvent");
//            auto rtt = rttstats.smoothed_rtt();
//            if(m_dmin.IsZero()){
//                m_dmin = rtt;
//            }else{
//                if( m_dmin > rtt) {
//                    m_dmin = rtt;
//                }
//            }

            if(InSlowStart()){
//                m_cwnd += 1 ;
                m_kcwnd += 1000 ;
            }else{
                m_kcwnd = cubic_.CongestionWindowAfterAck(1000, m_kcwnd,
                                                          rttstats.MinOrInitialRtt(),ackEvent.recvstic);
//                if(m_cwndCnt > cnt){
//                    m_cwndCnt = 0;
//                    m_cwnd +=1;
//                }else{
//                    m_cwndCnt += 1;
//                }

            }

        }
    }

    uint32_t GetCWND() override {
//        SPDLOG_DEBUG(" {}", m_cwnd);
//        return m_cwnd;
        SPDLOG_DEBUG(" {}", m_kcwnd);
        return BoundCwnd(m_kcwnd/1000);

    }

private:

    bool InSlowStart()
    {
        return m_kcwnd < m_kssThresh;
//        return m_cwnd < m_ssThresh;
    }

    uint32_t BoundCwnd(uint32_t trySetCwnd)
    {
        return std::max(m_minCwnd, std::min(trySetCwnd, m_maxCwnd));
    }

    quic::CubicBytes cubic_;

    uint32_t m_kcwnd{1000};
    uint32_t m_cwnd{1};
    uint32_t m_cwndCnt{0}; /** in congestion avoid phase, used for counting ack packets*/
    Duration m_dmin{Duration::Zero()};

    uint32_t m_minCwnd{1};
    uint32_t m_maxCwnd{128};
    uint32_t m_ssThresh{32};/** slow start threshold*/
    uint32_t m_kssThresh{32000};/** slow start threshold*/

};