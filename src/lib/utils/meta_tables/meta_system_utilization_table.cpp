#ifdef __linux__

#include <numa.h>
#include <sys/sysinfo.h>
#include <sys/times.h>
#include <fstream>
#include <sstream>

#endif

#ifdef __APPLE__

#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <mach/mach_time.h>
#include <mach/vm_map.h>
#include <mach/vm_statistics.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <time.h>

#endif

#include <sys/types.h>

#include <chrono>

#include "meta_system_utilization_table.hpp"

#include "hyrise.hpp"

namespace opossum {

MetaSystemUtilizationTable::MetaSystemUtilizationTable()
    : AbstractMetaSystemTable(TableColumnDefinitions{{"cpu_system_usage", DataType::Float, false},
                                                     {"cpu_process_usage", DataType::Float, false},
                                                     {"load_average_1_min", DataType::Float, false},
                                                     {"load_average_5_min", DataType::Float, false},
                                                     {"load_average_15_min", DataType::Float, false},
                                                     {"system_memory_free", DataType::Long, false},
                                                     {"process_virtual_memory", DataType::Long, false},
                                                     {"process_physical_memory", DataType::Long, false}}) {}

const std::string& MetaSystemUtilizationTable::name() const {
  static const auto name = std::string{"system_utilization"};
  return name;
}

void MetaSystemUtilizationTable::init() {
  _get_system_cpu_usage();
  _get_process_cpu_usage();
}

std::shared_ptr<Table> MetaSystemUtilizationTable::_on_generate() {
  auto output_table = std::make_shared<Table>(_column_definitions, TableType::Data, std::nullopt, UseMvcc::Yes);

  const auto system_cpu_usage = _get_system_cpu_usage();
  const auto process_cpu_usage = _get_process_cpu_usage();
  const auto load_avg = _get_load_avg();
  const auto system_memory_usage = _get_system_memory_usage();
  const auto process_memory_usage = _get_process_memory_usage();

  output_table->append({system_cpu_usage, process_cpu_usage, load_avg.load_1_min, load_avg.load_5_min,
                        load_avg.load_15_min, system_memory_usage.free_ram, process_memory_usage.virtual_memory,
                        process_memory_usage.physical_memory});

  return output_table;
}

MetaSystemUtilizationTable::LoadAvg MetaSystemUtilizationTable::_get_load_avg() {
#ifdef __linux__

  std::ifstream load_avg_file;
  load_avg_file.open("/proc/loadavg", std::ifstream::in);

  std::string load_avg_value;
  std::vector<float> load_avg_values;
  for (int value_index = 0; value_index < 3; ++value_index) {
    std::getline(load_avg_file, load_avg_value, ' ');
    load_avg_values.push_back(std::stof(load_avg_value));
  }
  load_avg_file.close();

  return {load_avg_values[0], load_avg_values[1], load_avg_values[2]};
#endif

#ifdef __APPLE__

  loadavg load_avg;
  size_t size = sizeof(load_avg);
  if (sysctlbyname("vm.loadavg", &load_avg, &size, nullptr, 0) != 0) {
    Fail("Unable to call sysctl vm.loadavg");
  }

  return {static_cast<float>(load_avg.ldavg[0]) / static_cast<float>(load_avg.fscale),
          static_cast<float>(load_avg.ldavg[1]) / static_cast<float>(load_avg.fscale),
          static_cast<float>(load_avg.ldavg[2]) / static_cast<float>(load_avg.fscale)};

#endif

  Fail("Method not implemented for this platform");
}

float MetaSystemUtilizationTable::_get_system_cpu_usage() {
#ifdef __linux__
  std::ifstream stat_file;
  stat_file.open("/proc/stat", std::ifstream::in);
  std::string cpu_line;
  std::getline(stat_file, cpu_line);
  stat_file.close();

  const auto cpu_times = _get_values(cpu_line);
  SystemCPUTime system_cpu_time{};
  system_cpu_time.user_time = cpu_times.at(0);
  system_cpu_time.user_nice_time = cpu_times.at(1);
  system_cpu_time.kernel_time = cpu_times.at(2);
  system_cpu_time.idle_time = cpu_times.at(3);

  const auto used = (system_cpu_time.user_time - _last_system_cpu_time.user_time) +
                    (system_cpu_time.user_nice_time - _last_system_cpu_time.user_nice_time) +
                    (system_cpu_time.kernel_time - _last_system_cpu_time.kernel_time);
  const auto total = used + (system_cpu_time.idle_time - _last_system_cpu_time.idle_time);

  _last_system_cpu_time = system_cpu_time;

  const auto cpus = _get_cpu_count();

  return static_cast<float>(100.0 * used) / static_cast<float>(total * cpus);
#endif

#ifdef __APPLE__

  host_cpu_load_info_data_t cpu_info;
  mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
  if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, reinterpret_cast<host_info_t>(&cpu_info), &count) !=
      KERN_SUCCESS) {
    Fail("Unable to access host_statistics");
  }

  SystemCPUTicks system_cpu_ticks{};
  system_cpu_ticks.total_ticks = 0;
  for (int cpu_state = 0; cpu_state <= CPU_STATE_MAX; ++cpu_state) {
    system_cpu_ticks.total_ticks += cpu_info.cpu_ticks[cpu_state];
  }
  system_cpu_ticks.idle_ticks = cpu_info.cpu_ticks[CPU_STATE_IDLE];

  const auto total = system_cpu_ticks.total_ticks - _last_system_cpu_ticks.total_ticks;
  const auto idle = system_cpu_ticks.idle_ticks - _last_system_cpu_ticks.idle_ticks;

  _last_system_cpu_ticks = system_cpu_ticks;

  return 100.0f * (1.0f - (static_cast<float>(idle) / static_cast<float>(total)));

#endif

  Fail("Method not implemented for this platform");
}

float MetaSystemUtilizationTable::_get_process_cpu_usage() {
#ifdef __linux__
  struct tms time_sample {};

  ProcessCPUTime process_cpu_time{};
  process_cpu_time.clock_time = times(&time_sample);
  process_cpu_time.kernel_time = time_sample.tms_stime;
  process_cpu_time.user_time = time_sample.tms_utime;

  const auto used = (process_cpu_time.user_time - _last_process_cpu_time.user_time) +
                    (process_cpu_time.kernel_time - _last_process_cpu_time.kernel_time);
  const auto total = process_cpu_time.clock_time - _last_process_cpu_time.clock_time;

  _last_process_cpu_time = process_cpu_time;

  int cpus;
  if (numa_available() != -1) {
    cpus = numa_num_task_cpus();
  } else {
    cpus = _get_cpu_count();
  }

  return static_cast<float>(100.0 * used) / static_cast<float>(total * cpus);
#endif

#ifdef __APPLE__
  ProcessCPUTime process_cpu_time;
  process_cpu_time.system_clock = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
  process_cpu_time.process_clock = clock_gettime_nsec_np(CLOCK_PROCESS_CPUTIME_ID);

  const auto used = (process_cpu_time.process_clock - _last_process_cpu_time.process_clock);
  const auto total = (process_cpu_time.system_clock - _last_process_cpu_time.system_clock);

  const auto cpus = _get_cpu_count();

  _last_process_cpu_time = process_cpu_time;

  return static_cast<float>(100.0f * used) / static_cast<float>(total * cpus);

#endif

  Fail("Method not implemented for this platform");
}

MetaSystemUtilizationTable::SystemMemoryUsage MetaSystemUtilizationTable::_get_system_memory_usage() {
#ifdef __linux__

  struct sysinfo memory_info{};
  sysinfo(&memory_info);

  MetaSystemUtilizationTable::SystemMemoryUsage memory_usage{};
  memory_usage.total_ram = memory_info.totalram * memory_info.mem_unit;
  memory_usage.total_swap = memory_info.totalswap * memory_info.mem_unit;
  memory_usage.free_ram = memory_info.freeram * memory_info.mem_unit;
  memory_usage.free_swap = memory_info.freeswap * memory_info.mem_unit;
  memory_usage.total_memory = memory_usage.total_ram + memory_usage.total_swap;
  memory_usage.free_memory = memory_usage.free_ram + memory_usage.free_swap;

  return memory_usage;
#endif

#ifdef __APPLE__

  int64_t physical_memory;
  size_t size = sizeof(physical_memory);
  if (sysctlbyname("hw.memsize", &physical_memory, &size, nullptr, 0) != 0) {
    Fail("Unable to call sysctl hw.memsize");
  }

  // Attention: total swap might change if more swap is needed
  xsw_usage swap_usage;
  size = sizeof(swap_usage);
  if (sysctlbyname("vm.swapusage", &swap_usage, &size, nullptr, 0) != 0) {
    Fail("Unable to call sysctl vm.swapusage");
  }

  vm_size_t page_size;
  vm_statistics64_data_t vm_statistics;
  mach_msg_type_number_t count = sizeof(vm_statistics) / sizeof(natural_t);

  if (host_page_size(mach_host_self(), &page_size) != KERN_SUCCESS ||
      host_statistics64(mach_host_self(), HOST_VM_INFO, reinterpret_cast<host_info64_t>(&vm_statistics), &count) !=
          KERN_SUCCESS) {
    Fail("Unable to access host_page_size or host_statistics64");
  }

  MetaSystemUtilizationTable::SystemMemoryUsage memory_usage;
  memory_usage.total_ram = physical_memory;
  memory_usage.total_swap = swap_usage.xsu_total;
  memory_usage.free_swap = swap_usage.xsu_avail;
  memory_usage.free_ram = vm_statistics.free_count * page_size;

  // auto used = (vm_statistics.active_couunt + vm_statistice.inactive_count + vm_statistics.wire_count) * page_size;

  return memory_usage;

#endif

  Fail("Method not implemented for this platform");
}

#ifdef __linux__
std::vector<int64_t> MetaSystemUtilizationTable::_get_values(std::string &input_string) {
  std::stringstream input_stream;
  input_stream << input_string;
  std::vector<int64_t> output_values;

  std::string token;
  int64_t value;
  while (!input_stream.eof()) {
    input_stream >> token;
    if (std::stringstream(token) >> value) {
      output_values.push_back(value);
    }
  }

  return output_values;
}
#endif

MetaSystemUtilizationTable::ProcessMemoryUsage MetaSystemUtilizationTable::_get_process_memory_usage() {
#ifdef __linux__

  std::ifstream self_status_file;
  self_status_file.open("/proc/self/status", std::ifstream::in);

  MetaSystemUtilizationTable::ProcessMemoryUsage memory_usage{};
  std::string self_status_line;
  while (std::getline(self_status_file, self_status_line)) {
    if (self_status_line.rfind("VmSize", 0) == 0) {
      memory_usage.virtual_memory = _get_values(self_status_line)[0] * 1000;
    } else if (self_status_line.rfind("VmRSS", 0) == 0) {
      memory_usage.physical_memory = _get_values(self_status_line)[0] * 1000;
    }
  }

  self_status_file.close();

  return memory_usage;
#endif

#ifdef __APPLE__

  struct task_basic_info info;
  mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS) {
    Fail("Unable to access task_info");
  }

  return {static_cast<int64_t>(info.virtual_size), static_cast<int64_t>(info.resident_size)};

#endif

  Fail("Method not implemented for this platform");
}

}  // namespace opossum
