#include "sysmetrics.h"

perf::SystemMetrics& perf::GetSystemMetrics()
{
     static SystemMetrics metrics;
     return metrics;
}
