#ifndef CHANNELS_H
#define CHANNELS_H

enum class Channels {
    SVT1,
    SVT2,
    SVT24,
    
};

// Utilities to classify channels and map to owning app
namespace ChannelUtil {
    enum class AppId { SVT, EON, Unknown };

    inline constexpr bool isSVT(Channels ch) {
        switch (ch) {
            case Channels::SVT1:
            case Channels::SVT2:
            case Channels::SVT24:
                return true;
            default:
                return false;
        }
    }

    inline constexpr bool isEON(Channels /*ch*/) {
        // No EON channels yet; update when added
        return false;
    }

    inline constexpr AppId appFor(Channels ch) {
        return isSVT(ch) ? AppId::SVT : (isEON(ch) ? AppId::EON : AppId::Unknown);
    }
}

#endif