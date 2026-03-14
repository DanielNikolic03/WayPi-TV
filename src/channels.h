#ifndef CHANNELS_H
#define CHANNELS_H

enum class Channels {
    SVT1,
    SVT2,
    KUNSKAPSKANALEN,
    SVT24,
    MOVE_RTS_1,
    MOVE_PINK,
    MOVE_PRVA,
    MOVE_HAPPY,
    MOVE_BN,
    MOVE_BN_MUZIKA
};

// Utilities to classify channels and map to owning app
namespace ChannelUtil {
    enum class AppId { SVT, MOVE, Unknown };

    inline constexpr bool isSVT(Channels ch) {
        switch (ch) {
            case Channels::SVT1:
            case Channels::SVT2:
            case Channels::KUNSKAPSKANALEN:
            case Channels::SVT24:
                return true;
            default:
                return false;
        }
    }

    inline constexpr bool isMOVE(Channels ch) {
        switch (ch) {
            case Channels::MOVE_RTS_1:
            case Channels::MOVE_PINK:
            case Channels::MOVE_PRVA:
            case Channels::MOVE_HAPPY:
            case Channels::MOVE_BN:
            case Channels::MOVE_BN_MUZIKA:
                return true;
            default:
                return false;
        }
    }

    inline constexpr AppId appFor(Channels ch) {
        return isSVT(ch) ? AppId::SVT : (isMOVE(ch) ? AppId::MOVE : AppId::Unknown);
    }
}

#endif
