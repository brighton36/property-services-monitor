#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "property-services-monitor.h"
#include "monitor_service_blueiris.h"

using namespace std;

std::shared_ptr<MonitorServiceFactory::map_type> MonitorServiceFactory::map = nullptr;

TEST_CASE("testing BlueIrisAlert() class") {
  BlueIrisAlert alert1 = BlueIrisAlert(
    "{\"camera\":\"LotNorth\",\"clip\":\"@299856758.bvr\",\"color\":8151097,\"d"
    "ate\":1583249201,\"filesize\":\"14sec\",\"flags\":196608,\"newalerts\":199"
    ",\"offset\":445450,\"path\":\"@300598614.bvr\",\"res\":\"2688x1520\",\"zon"
    "es\":1}"_json);

  CHECK(alert1.camera    == "LotNorth");
  CHECK(alert1.clip      == "@299856758.bvr");
  CHECK(alert1.color     == 8151097);
  CHECK(alert1.date      == (long int)1583249201);
  CHECK(alert1.filesize  == "14sec");
  CHECK(alert1.flags     == 196608);
  CHECK(alert1.newalerts == 199);
  CHECK(alert1.offset    == 445450);
  CHECK(alert1.path      == "@300598614.bvr");
  CHECK(alert1.res       == "2688x1520");
  CHECK(alert1.zones     == 1);

  CHECK(alert1.dateAsString("%F %r") == "2020-03-03 10:26:41 AM");
  CHECK(alert1.pathThumb() == "/thumbs/@300598614.bvr");
  CHECK(alert1.pathClip() == "/file/clips/@299856758.bvr?time=0&cache=1&h=240");

  BlueIrisAlert alert2 = BlueIrisAlert(
    "{\"camera\":\"LotSouth\",\"clip\":\"@225022208.bvr\",\"color\":8151097,\"d"
    "ate\":1583101071,\"filesize\":\"12sec\",\"flags\":196608,\"offset\":467003"
    "0,\"path\":\"@226540688.bvr\",\"res\":\"2688x1520\",\"zones\":1}"_json);

  CHECK(alert2.camera    == "LotSouth");
  CHECK(alert2.clip      == "@225022208.bvr");
  CHECK(alert2.color     == 8151097);
  CHECK(alert2.date      == (long int)1583101071);
  CHECK(alert2.filesize  == "12sec");
  CHECK(alert2.flags     == 196608);
  CHECK(alert2.newalerts == 0);
  CHECK(alert2.offset    == 4670030);
  CHECK(alert2.path      == "@226540688.bvr");
  CHECK(alert2.res       == "2688x1520");
  CHECK(alert2.zones     == 1);

  CHECK(alert2.dateAsString("%F %r") == "2020-03-01 05:17:51 PM");
  CHECK(alert2.pathThumb() == "/thumbs/@226540688.bvr");
  CHECK(alert2.pathClip() == "/file/clips/@225022208.bvr?time=0&cache=1&h=240");
}
