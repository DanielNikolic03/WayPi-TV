#ifndef CHANNELS_H
#define CHANNELS_H

enum class Channels {
    SVT1,
    SVT2,
    KUNSKAPSKANALEN,
    SVT24,
    EON_RTS_1,
    EON_PINK,
    EON_PRVA,
    EON_HAPPY,
    EON_BN,
    EON_BN_MUZIKA,
    EON_NATURE
};

// Utilities to classify channels and map to owning app
namespace ChannelUtil {
    enum class AppId { SVT, EON, Unknown };

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

    inline constexpr bool isEON(Channels ch) {
        switch (ch) {
            case Channels::EON_RTS_1:
            case Channels::EON_PINK:
            case Channels::EON_PRVA:
            case Channels::EON_HAPPY:
            case Channels::EON_BN:
            case Channels::EON_BN_MUZIKA:
            case Channels::EON_NATURE:
                return true;
            default:
                return false;
        }
    }

    inline constexpr AppId appFor(Channels ch) {
        return isSVT(ch) ? AppId::SVT : (isEON(ch) ? AppId::EON : AppId::Unknown);
    }
}

#endif
