#include "property-services-monitor.h"

class BlueIrisAlert {
  public:
    std::string camera, clip, filesize, resolution, path, res;
    unsigned int color, flags, newalerts, offset, zones;
    time_t date;

    BlueIrisAlert(nlohmann::json);
    std::string dateAsString(std::string);
    std::string pathThumb();
    std::string pathClip();
};
