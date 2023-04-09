namespace quic {
    using QuicPacketCount = uint64_t;
    using QuicByteCount = uint64_t;

    // Default maximum packet size used in the Linux TCP implementation.
// Used in QUIC for congestion window computations in bytes.
//    inline constexpr QuicByteCount kDefaultTCPMSS = 1460;
    constexpr QuicByteCount kDefaultTCPMSS = 1000;
//    inline constexpr QuicByteCount kDefaultTCPMSS = 1000;
    constexpr QuicByteCount kMaxSegmentSize = kDefaultTCPMSS;

    // Default number of connections for N-connection emulation.
    constexpr uint32_t kDefaultNumConnections = 2;

    constexpr uint64_t kNumMillisPerSecond = 1000;
    constexpr uint64_t kNumMicrosPerMilli = 1000;
    constexpr uint64_t kNumMicrosPerSecond =
            kNumMicrosPerMilli * kNumMillisPerSecond;

}