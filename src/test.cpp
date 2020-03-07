#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "property-services-monitor.h"
#include "monitor_service_blueiris.h"

using namespace std;

std::shared_ptr<MonitorServiceFactory::map_type> MonitorServiceFactory::map = nullptr;


TEST_CASE("testing function duration_to_seconds()") {
  unsigned int minute = 60;
  unsigned int hour = minute * 60;
  unsigned int day = hour * 24;
  unsigned int week = day * 7;

  // It gets rough with these measures:
  unsigned int month = 30 * day;
  unsigned int year = 365 * day;

  CHECK(duration_to_seconds("1 second") == 1);
  CHECK(duration_to_seconds("1 minute") == minute);
  CHECK(duration_to_seconds("1 hour")   == hour);
  CHECK(duration_to_seconds("1 day")    == day);
  CHECK(duration_to_seconds("1 week")   == week);

  CHECK(duration_to_seconds("1 minutes") == minute);
  CHECK(duration_to_seconds("1 hours")   == hour);
  CHECK(duration_to_seconds("1 days")    == day);
  CHECK(duration_to_seconds("1 weeks")   == week);

  CHECK(duration_to_seconds("1 month") == month);
  CHECK(duration_to_seconds("1 year")  == year);

  CHECK(duration_to_seconds("24 hours") == day);
  CHECK(duration_to_seconds("60 minutes") == hour);
  CHECK(duration_to_seconds("300 minutes") == 300*minute);
  CHECK(duration_to_seconds("48 hours") == 2*day);

  CHECK(duration_to_seconds("2 years, 2 days") == 2*year+2*day);
  CHECK(duration_to_seconds("30 minutes and 1 hour") == 1*hour+30*minute);
  CHECK(duration_to_seconds("3 weeks, 2 hours, 30 minutes, 10 seconds") == 
    3*week+2*hour+30*minute+10);
  CHECK(duration_to_seconds("67") == 67);
  CHECK(duration_to_seconds("nonsense") == 0);
}

TEST_CASE("testing function percent_string_to_float()") {
  CHECK(percent_string_to_float("1%") == 1.0f);
  CHECK(percent_string_to_float("8%") == 8.0f);
  CHECK(percent_string_to_float("1 percent") == 1.0f);
  CHECK(percent_string_to_float("1.5 percent") == 1.5f);
  CHECK(percent_string_to_float("2.5 percent") == 2.5f);
  CHECK(percent_string_to_float("95 %") == 95.0f);
  CHECK(percent_string_to_float("110%") == 110.0f);
  CHECK(percent_string_to_float("nonsense") == 0.0f);
}

TEST_CASE("testing function relative_time_from()") {
  time_t e, expected;
  auto str_to_time = [](string time) -> time_t {
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    strptime(time.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
    return mktime(&tm);
  };

  e = str_to_time("2020-01-01 09:00:00");
  expected = str_to_time("2019-12-31 11:00:00");

  CHECK(relative_time_from(e, "day prior,      11:00 A.") == expected);
  CHECK(relative_time_from(e, "yesterday.      11:00a")   == expected);
  CHECK(relative_time_from(e, "Yesterday at 11:00a")      == expected);
  CHECK(relative_time_from(e, "Day before,     11:00")    == expected);
  CHECK(relative_time_from(e, "the day before, 11:00 A")  == expected);
  CHECK(relative_time_from(e, "11:00 A the day before.")  == expected);
  CHECK(relative_time_from(e, "1 day prior.    11:00 a")  == expected);
  CHECK(relative_time_from(e, "1 day ago,      11:00 A")  == expected);
  CHECK(relative_time_from(e, "1 day before.   11:00 a")  == expected);
  CHECK(relative_time_from(e, "1 day before, 11:00 A")    == expected);
  CHECK(relative_time_from(e, "11:00 A, 1 day before")    == expected);

  e = str_to_time("2020-03-01 10:00:00");
  expected = str_to_time("2020-02-28 23:00:00");
  CHECK(relative_time_from(e, "2 days prior,  11:00 P")  == expected);
  CHECK(relative_time_from(e, "11:00 P, 2 days prior")   == expected);
  CHECK(relative_time_from(e, "2 days ago.    11:00 p")  == expected);
  CHECK(relative_time_from(e, "2 days before, 11:00 P")  == expected);
  CHECK(relative_time_from(e, "2 day before,  11:00p")   == expected);
  CHECK(relative_time_from(e, "11:00p, 2 days before")   == expected);

  e = str_to_time("2020-02-01 18:00:00");
  expected = str_to_time("2020-02-01 13:05:00");
  CHECK(relative_time_from(e, "today, 1:05 P")   == expected);
  CHECK(relative_time_from(e, "Today at 1:05 P")   == expected);
  CHECK(relative_time_from(e, "day of. 13:05 A") == expected);
  CHECK(relative_time_from(e, "13:05")           == expected);
  CHECK(relative_time_from(e, "1:05 P today")    == expected);

  e = str_to_time("2020-04-15 12:00:00");
  expected = str_to_time("2020-04-14 12:00:00");
  CHECK(relative_time_from(e, "Yesterday")   == expected);
  CHECK(relative_time_from(e, "1 day ago")   == expected);
  CHECK(relative_time_from(e, "1 day ago")   == expected);

  e = str_to_time("2020-04-15 12:00:00");
  CHECK(relative_time_from(e, "1 hour ago") == str_to_time("2020-4-15 11:00:00"));
  CHECK(relative_time_from(e, "2 hours ago") == str_to_time("2020-4-15 10:00:00"));
  CHECK(relative_time_from(e, "10 minutes ago") == str_to_time("2020-4-15 11:50:00"));
  CHECK(relative_time_from(e, "10 mins before") == str_to_time("2020-4-15 11:50:00"));
  CHECK(relative_time_from(e, "2 days, 1 hour, 10 minutes back...") == str_to_time("2020-4-13 10:50:00"));

  CHECK(relative_time_from(e, "unparseable adjustment") == e);
}

TEST_CASE("testing class BlueIrisAlert()") {
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
