#pragma once
#include <ECS/Time.h>
#include <ECS/System/RuntimeSystemDescriptor.h>
#include <ECS/System/Stages/UpdateStage.h>

struct SystemProfileReport {
    struct StageReport {
        const char* name = "Unknown Stage";
        Time slowestExecution = Time(0);
        Time averageExecution = Time(0);
        Time fastestExecution = Time(0);
        size_t totalExecutions = 0;

        StageReport() = default;

        StageReport(const char* name) : name(name) {}

        StageReport(const char* name, const Time slowest, const Time average, const Time fastest, const size_t total)
            : name(name), slowestExecution(slowest), averageExecution(average), fastestExecution(fastest), totalExecutions(total)
        {}

        friend std::ostream& operator<<(std::ostream& stream, const StageReport& report);
    };
private:
    const RuntimeSystemDescriptor* system{};
    mem::vector<StageReport> stageReports;
public:
    SystemProfileReport() = default;
    SystemProfileReport(const RuntimeSystemDescriptor* system, mem::vector<StageReport>&& stageReports) : system(system), stageReports(std::move(stageReports)) {}

    template <IsStage Stage>
    StageReport* inStage() {
        for (auto& stage : stageReports) {
            if (stage.name == Stage::name) {
                return &stage;
            }
        }
        return nullptr;
    }

    auto reports() {
        return mem::make_range(stageReports.data(), stageReports.size());
    }

    friend std::ostream& operator<<(std::ostream& stream, const SystemProfileReport& report);
};

class StageProfileReport {
    const UpdateStage* stage = nullptr;
    UpdateStageMetrics metrics{};
public:
    StageProfileReport() = default;
    explicit StageProfileReport(const UpdateStage* stage) : stage(stage) {
        if (stage) {
            metrics = stage->metrics;
        }
    }

    friend std::ostream& operator<<(std::ostream& stream, const StageProfileReport& report);
};

std::ostream& operator << (std::ostream& stream, StageExecutionModel model);
std::ostream& operator << (std::ostream& stream, StageScheduleModel model);

std::ostream& operator<<(std::ostream& stream, const SystemProfileReport::StageReport& report);
std::ostream& operator<<(std::ostream& stream, const SystemProfileReport& report);
std::ostream& operator<<(std::ostream& stream, const StageProfileReport& report);