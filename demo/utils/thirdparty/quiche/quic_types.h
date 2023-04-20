
namespace basefw {
    namespace quic {

        using QuicPacketCount = uint64_t;
        using QuicByteCount = uint64_t;

        // Default maximum packet size used in the Linux TCP implementation.
// Used in QUIC for congestion window computations in bytes.
//    inline constexpr QuicByteCount kDefaultTCPMSS = 1460;
        constexpr QuicByteCount kDefaultTCPMSS = 1000;
        constexpr QuicByteCount kMaxSegmentSize = kDefaultTCPMSS;

        // Default number of connections for N-connection emulation.
        constexpr uint32_t kDefaultNumConnections = 1;

        constexpr uint64_t kNumMillisPerSecond = 1000;
        constexpr uint64_t kNumMicrosPerMilli = 1000;
        constexpr uint64_t kNumMicrosPerSecond = kNumMicrosPerMilli * kNumMillisPerSecond;

        constexpr uint32_t MaxOneTimeSentCount = 16;

//    using QuicRandom = quiche::QuicheRandom;
//
//    enum CongestionControlType {
//        kCubicBytes,
//        kRenoBytes,
//        kBBR,
//        kBBRv2,
//    };
//    constexpr QuicPacketCount quic_max_congestion_window = 64;
//    enum class Perspective : uint8_t { IS_SERVER, IS_CLIENT };
//
//    enum HasRetransmittableData : uint8_t {
//        NO_RETRANSMITTABLE_DATA,
//        HAS_RETRANSMITTABLE_DATA,
//    };
//    using QuicPacketLength = uint16_t;
    }
}