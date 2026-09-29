#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

#include "sea_port/crane_optimizer.h"
#include "sea_port/port_simulation.h"
#include "sea_port/weather.h"

namespace {

using sea_port::CargoType;
using sea_port::CraneOptimizer;
using sea_port::DailyReport;
using sea_port::OptimizationOptions;
using sea_port::PortSimulation;
using sea_port::Season;
using sea_port::ShipSpec;
using sea_port::SimulationConfig;
using sea_port::Statistics;

int ReadInt(const std::string& prompt, int minimum, int maximum, int default_value) {
  while (true) {
    std::cout << prompt << " [" << default_value << "]: ";
    std::string line;
    if (!std::getline(std::cin, line)) {
      throw std::runtime_error("Ввод завершён");
    }
    if (line.empty()) {
      return default_value;
    }
    std::istringstream input(line);
    int value = 0;
    char remainder = '\0';
    if ((input >> value) && !(input >> remainder) && value >= minimum && value <= maximum) {
      return value;
    }
    std::cout << "Введите целое число от " << minimum << " до " << maximum << ".\n";
  }
}

double ReadDouble(const std::string& prompt, double minimum,
                  double maximum,  // NOLINT(bugprone-easily-swappable-parameters)
                  double default_value) {
  while (true) {
    std::cout << prompt << " [" << default_value << "]: ";
    std::string line;
    if (!std::getline(std::cin, line)) {
      throw std::runtime_error("Ввод завершён");
    }
    if (line.empty()) {
      return default_value;
    }
    std::istringstream input(line);
    double value = 0.0;
    char remainder = '\0';
    if ((input >> value) && !(input >> remainder) && value >= minimum && value <= maximum) {
      return value;
    }
    std::cout << "Введите число в допустимом диапазоне.\n";
  }
}

std::string ReadString(const std::string& prompt, const std::string& default_value) {
  std::cout << prompt << " [" << default_value << "]: ";
  std::string value;
  if (!std::getline(std::cin, value)) {
    throw std::runtime_error("Ввод завершён");
  }
  return value.empty() ? default_value : value;
}

Season ReadSeason() {
  const int value = ReadInt("Время года: 1 — зима, 2 — весна, 3 — лето, 4 — осень", 1, 4, 3);
  return static_cast<Season>(value - 1);
}

CargoType ReadCargoType() {
  const int value = ReadInt("Тип: 1 — сухогруз, 2 — танкер, 3 — контейнеровоз", 1, 3, 1);
  return static_cast<CargoType>(value - 1);
}

void PrintDailyReport(const DailyReport& report, int step_days) {
  std::cout << "\nДень " << report.day << ": волны " << report.weather.wave_level
            << "/5, работоспособность " << std::fixed << std::setprecision(0)
            << report.weather.operability * 100.0 << "%\n";
  for (const std::string& event : report.events) {
    std::cout << "  - " << event << '\n';
  }
  std::cout << "  Прибыло: " << report.arrived << ", начало разгрузки: " << report.started_unloading
            << ", завершено: " << report.completed << ", ушло: " << report.abandoned
            << ", отказано: " << report.rejected << '\n'
            << "  В очередях: " << report.waiting << ", разгружается: " << report.unloading
            << ", результат дня: " << std::setprecision(2) << report.daily_net_revenue
            << " у.е., рейтинг: " << report.rating << '\n';
  for (std::size_t index = 0; index < sea_port::kCargoTypeCount; ++index) {
    std::cout << "  Очередь «" << sea_port::ToString(static_cast<CargoType>(index)) << "»: ";
    if (report.queues[index].empty()) {
      std::cout << "пусто";
    } else {
      for (std::size_t ship_index = 0; ship_index < report.queues[index].size(); ++ship_index) {
        std::cout << (ship_index == 0 ? "" : ", ") << report.queues[index][ship_index];
      }
    }
    std::cout << '\n';
  }
  if (!report.active_operations.empty()) {
    std::cout << "  Активные операции: ";
    for (std::size_t index = 0; index < report.active_operations.size(); ++index) {
      std::cout << (index == 0 ? "" : ", ") << report.active_operations[index];
    }
    std::cout << '\n';
  }
  if (report.day % step_days == 0) {
    std::cout << "  --- контрольная точка шага моделирования ---\n";
  }
}

void PrintSummary(const PortSimulation& simulation) {
  const Statistics& stats = simulation.statistics();
  const double purchase_cost = simulation.CranePurchaseCost();
  std::cout << "\n========== ИТОГИ ==========\n"
            << "Прибыло: " << stats.arrived_ships() << '\n'
            << "Разгружено: " << stats.completed_ships() << '\n'
            << "Ушло без разгрузки: " << stats.abandoned_ships() << '\n'
            << "Не принято: " << stats.rejected_ships() << '\n'
            << "Осталось в порту: " << stats.unfinished_ships() << '\n'
            << "Средняя / максимальная очередь: " << std::fixed << std::setprecision(2)
            << stats.average_queue_length() << " / " << stats.maximum_queue_length() << '\n'
            << "Среднее / максимальное ожидание: " << stats.average_waiting_days() << " / "
            << stats.maximum_waiting_days() << " дн.\n"
            << "Средняя / максимальная дополнительная задержка: "
            << stats.average_unloading_delay_days() << " / " << stats.maximum_unloading_delay_days()
            << " дн.\n"
            << "Начисленный доход: " << stats.gross_revenue() << " у.е.\n"
            << "Штрафы: " << stats.total_penalties() << " у.е.\n"
            << "Операционная прибыль: " << stats.net_revenue() << " у.е.\n"
            << "Стоимость кранов: " << purchase_cost << " у.е.\n"
            << "Результат с учётом покупки: " << stats.net_revenue() - purchase_cost << " у.е.\n"
            << "Рейтинг: " << simulation.rating() << "/100\n"
            << "Доля обслуженных: " << stats.service_rate() * 100.0 << "%\n";

  std::cout << "\nЗавершённые разгрузки:\n";
  for (const auto& record : stats.completions()) {
    std::cout << "  " << record.ship_name << ": приход " << record.actual_arrival_day
              << ", ожидание " << record.waiting_days << ", начало " << record.unloading_start_day
              << ", длительность " << record.unloading_duration_days << ", результат "
              << record.net_revenue << " у.е.\n";
  }
}

SimulationConfig ReadConfiguration() {
  SimulationConfig config;
  std::cout << "Моделирование работы морского порта\n";
  config.season = ReadSeason();
  config.crane_counts[0] = ReadInt("Краны для сыпучих грузов", 0, 20, 2);
  config.crane_counts[1] = ReadInt("Краны для жидких грузов", 0, 20, 1);
  config.crane_counts[2] = ReadInt("Контейнерные краны", 0, 20, 1);
  config.waiting_capacity = static_cast<std::size_t>(ReadInt("Мест ожидания", 1, 1000, 20));
  config.unloading_places[0] = ReadInt("Мест разгрузки сухогрузов", 0, 20, 4);
  config.unloading_places[1] = ReadInt("Мест разгрузки танкеров", 0, 20, 3);
  config.unloading_places[2] = ReadInt("Мест разгрузки контейнеровозов", 0, 20, 3);
  config.step_days = ReadInt("Шаг вывода модели в днях", 1, 3, 1);
  return config;
}

void AddManualShips(PortSimulation& simulation, int day) {
  const int count = ReadInt("Количество ручных заявок", 1, 30, 1);
  for (int number = 1; number <= count; ++number) {
    ShipSpec spec;
    spec.cargo_type = ReadCargoType();
    spec.name = ReadString("Название корабля",
                           "MANUAL-" + std::to_string(day) + "-" + std::to_string(number));
    spec.booking_day = day;
    const int maximum_offset = std::min(6, simulation.config().simulation_days - day);
    const int offset = ReadInt("Через сколько дней ожидается прибытие", 0, maximum_offset, 0);
    spec.planned_arrival_day = day + offset;
    spec.cargo_weight_tons = ReadDouble("Масса груза, тонн", 1.0, 100000.0, 2000.0);
    spec.planned_stay_days = ReadInt("Плановый срок стоянки", 1, 30, 4);
    spec.max_wait_days = ReadInt("Максимальное ожидание", 0, 30, 6);
    simulation.ScheduleShip(spec);
  }
}

void RunInteractive() {
  SimulationConfig config = ReadConfiguration();
  PortSimulation simulation(config);
  while (simulation.current_day() <= config.simulation_days) {
    const int day = simulation.current_day();
    std::cout << "\nНастройка дня " << day << '\n';
    const int weather_mode = ReadInt("Погода: 1 — случайная, 2 — вручную", 1, 2, 1);
    if (weather_mode == 2) {
      simulation.SetWeather(
          day, sea_port::WeatherFromWaveLevel(ReadInt("Уровень волн от 0 до 5", 0, 5, 1)));
    }

    const int plan_mode =
        ReadInt("Заявки: 1 — случайно на неделю, 2 — вручную, 3 — не добавлять", 1, 3, 1);
    if (plan_mode == 1) {
      simulation.GenerateRandomParametersForDay(day, 7);
    } else if (plan_mode == 2) {
      AddManualShips(simulation, day);
    }
    PrintDailyReport(simulation.SimulateDay(), config.step_days);
  }
  simulation.Finalize();
  PrintSummary(simulation);
}

void RunAutomatic(SimulationConfig config) {
  PortSimulation simulation(config);
  while (simulation.current_day() <= config.simulation_days) {
    simulation.GenerateRandomParametersForDay(simulation.current_day(), 7);
    PrintDailyReport(simulation.SimulateDay(), config.step_days);
  }
  simulation.Finalize();
  PrintSummary(simulation);
}

void RunOptimizer(const SimulationConfig& config) {
  std::cout << "Запуск экономической оптимизации кранов...\n";
  const auto report = CraneOptimizer::Optimize(config, OptimizationOptions{});
  std::cout << "\nЛучшие варианты (Б — сыпучие, Ж — жидкие, К — контейнерные):\n"
            << " Б  Ж  К | Покупка | Опер. прибыль | После покупки | Рейтинг | Обслужено | "
               "Окупаемость\n";
  const std::size_t shown = std::min<std::size_t>(10, report.results.size());
  for (std::size_t index = 0; index < shown; ++index) {
    const auto& item = report.results[index];
    std::cout << std::setw(2) << item.crane_counts[0] << ' ' << std::setw(2) << item.crane_counts[1]
              << ' ' << std::setw(2) << item.crane_counts[2] << " | " << std::fixed
              << std::setprecision(0) << std::setw(8) << item.purchase_cost << " | "
              << std::setw(13) << item.average_operating_profit << " | " << std::setw(13)
              << item.average_investment_profit << " | " << std::setw(7) << std::setprecision(1)
              << item.average_rating << " | " << std::setw(9) << item.average_service_rate * 100.0
              << "% | " << std::setw(10) << std::setprecision(2) << item.payback_months << " мес."
              << (item.meets_quality_limits ? "" : "  [не прошёл ограничения]") << '\n';
  }

  if (report.best_result.has_value()) {
    const auto& best = *report.best_result;
    std::cout << "\n"
              << (best.meets_quality_limits ? "Оптимальный допустимый набор: "
                                            : "Лучший набор без ограничений: ")
              << best.crane_counts[0] << " / " << best.crane_counts[1] << " / "
              << best.crane_counts[2] << "; ожидаемый результат после покупки: " << std::fixed
              << std::setprecision(2) << best.average_investment_profit << " у.е.\n";
    if (!best.meets_quality_limits) {
      std::cout << "Внимание: ни один проверенный набор не прошёл ограничения рейтинга и "
                   "обслуживания; показан лучший экономический результат без ограничений.\n";
    }
  }
}

void PrintHelp(const char* executable) {
  std::cout << "Использование:\n"
            << "  " << executable << "             интерактивная симуляция\n"
            << "  " << executable << " --auto      автоматическая симуляция\n"
            << "  " << executable << " --optimize  подбор кранов\n"
            << "Дополнительно: --seed N задаёт генератор случайных чисел.\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  try {
    bool automatic = false;
    bool optimize = false;
    SimulationConfig config;
    for (int index = 1; index < argc; ++index) {
      const std::string argument = argv[index];
      if (argument == "--auto") {
        automatic = true;
      } else if (argument == "--optimize") {
        optimize = true;
      } else if (argument == "--seed" && index + 1 < argc) {
        config.random_seed = std::stoull(argv[++index]);
      } else if (argument == "--help" || argument == "-h") {
        PrintHelp(argv[0]);
        return 0;
      } else {
        throw std::invalid_argument("Неизвестный аргумент: " + argument);
      }
    }

    if (optimize) {
      RunOptimizer(config);
    } else if (automatic) {
      RunAutomatic(config);
    } else {
      RunInteractive();
    }
  } catch (const std::exception& error) {
    std::cerr << "Ошибка: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
