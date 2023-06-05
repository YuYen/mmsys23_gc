
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
        cubic_.SetNumConnections(basefw::quic::kDefaultNumConnections);
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
            if(InSlowStart()){
                if(m_state == 0){
                    if(m_kcwnd < 2 * m_initkcwnd){
                        m_kcwnd -= basefw::quic::kDefaultTCPMSS;
                    }else{
                        m_kcwnd /= 2;
                    }
                }else{
                    m_kcwnd = std::max(m_kcwnd/2,  m_prekCwnd);
                    m_state = 0;
                }

            }else{
                m_kcwnd = cubic_.CongestionWindowAfterPacketLoss(m_kcwnd);
            }
            m_kssThresh =m_kcwnd;

            if(m_kcwnd < m_kminCwnd){
                m_kcwnd = m_kminCwnd;
            }
        }

        if (ackEvent.valid) {
            SPDLOG_TRACE("ackEvent");
            if(InSlowStart()) {
                m_kcwnd += basefw::quic::kDefaultTCPMSS;
            }else{
                m_kcwnd = cubic_.CongestionWindowAfterAck(basefw::quic::kDefaultTCPMSS, m_kcwnd,
                                                          rttstats.MinOrInitialRtt(),ackEvent.recvstic);
                if( m_kcwnd > m_kmaxCwnd){
                    m_kcwnd = m_kmaxCwnd;
                }
            }
        }
    }

    uint32_t GetCWND() override {
        return BoundCwnd(m_kcwnd)/basefw::quic::kDefaultTCPMSS;
    }

    void UpdateState() override{
        if(!InSlowStart()) {
            m_prekCwnd = m_kcwnd;
            m_kssThresh = 4 * m_prekCwnd;
            m_state = 1;
        }

//        if(!InSlowStart()){
//            m_prekCwnd = m_kcwnd;
////            m_kssThresh *= 2;
//            m_kssThresh = 4 * m_prekCwnd;
//            m_state = 1;
//            return true;
//        }
//        return false;
    }

    bool InSlowStart() override{
        return m_kcwnd < m_kssThresh;
//        return m_cwnd < m_ssThresh;
    }

private:

    uint32_t BoundCwnd(uint32_t trySetCwnd)
    {
        return std::max(m_kminCwnd, std::min(trySetCwnd, m_kmaxCwnd));
    }

    basefw::quic::CubicBytes cubic_;

    uint32_t m_kcwnd{3000};
    uint32_t m_initkcwnd{3000};
//    uint32_t m_cwnd{1};
//    uint32_t m_cwndCnt{0}; /** in congestion avoid phase, used for counting ack packets*/
//    Duration m_dmin{Duration::Zero()};

//    uint32_t m_minCwnd{1};
//    uint32_t m_maxCwnd{128};

    uint32_t m_kminCwnd = 1 * basefw::quic::kDefaultTCPMSS;
    uint32_t m_kmaxCwnd = 128 * basefw::quic::kDefaultTCPMSS;
//    uint32_t m_ssThresh{32};/** slow start threshold*/
    uint32_t m_kssThresh{32000};/** slow start threshold*/

    uint32_t m_prekCwnd{1000};
    uint8_t m_state = 0;
};