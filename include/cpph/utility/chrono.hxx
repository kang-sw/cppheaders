#pragma once
#include <chrono>

namespace cpph {
using namespace std::chrono_literals;

using std::chrono::duration_cast;
using std::chrono::time_point_cast;

using std::chrono::hours;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::minutes;
using std::chrono::nanoseconds;
using std::chrono::seconds;

using std::chrono::high_resolution_clock;
using std::chrono::steady_clock;
using std::chrono::system_clock;

using std::chrono::duration;
using std::chrono::time_point;

template <typename Rep, typename Ratio>
double to_seconds(duration<Rep, Ratio> const& dur)
{
    return duration_cast<duration<double>>(dur).count();
}

//! @see https://stackoverflow.com/a/44063597
inline duration<double> timezone_offset()
{
    time_t gmt, rawtime = time(NULL);
    struct tm* ptm;

#if !defined(WIN32)
    struct tm gbuf;
    ptm = gmtime_r(&rawtime, &gbuf);
#else
    struct tm gbuf;
    gmtime_s(ptm = &gbuf, &rawtime);
#endif
    // Request that mktime() looksup dst in timezone database
    ptm->tm_isdst = -1;
    gmt = mktime(ptm);

    return 1.s * difftime(rawtime, gmt);
}
}  // namespace cpph
