#pragma once
#include <iostream>
#include "SystemRegistry.h"

#include "ExecutionGraphCreationContext.h"
#include "SystemProfileReport.h"
#include "SystemScheduler.h"
#include <tbb/task_group.h>

SystemRegistry::SystemRegistry(): systemKindRegistry(ComponentKind::of<SystemComponentType>()), stageKindRegistry(ComponentKind::of<StageComponentType>()) {
    auto updateStage = updateStageAllocator.allocate<UpdateStage>(1);

    auto& nullField = stageKindRegistry.getNullField();

    new (updateStage) UpdateStage(*nullField.descriptor, nullptr);

    stages.emplace_back(updateStage);
}

void SystemRegistry::addStage(const ComponentField<StageField> &field, TypeUUID type) {
    if (type.id() < stages.size()) return; // already added

    auto updateStage = updateStageAllocator.allocate<UpdateStage>(1);
    new (updateStage) UpdateStage(*field.descriptor, field.instance);

    stages.emplace_back(updateStage);

    switch (field.descriptor->scheduleModel) {
        case StageScheduleModel::MANUAL:
            manualStages.emplace_back(updateStage);
            break;
        case StageScheduleModel::PER_FRAME:
        case StageScheduleModel::FIXED_HZ:
            perTickStages.emplace_back(updateStage);
            break;
        case StageScheduleModel::PASSIVE:
            break;
    }
}

SystemProfileReport SystemRegistry::getProfileReport(const RuntimeSystemDescriptor& desc) {
    mem::vector<SystemProfileReport::StageReport> stageReports;
    stageReports.reserve(desc.stages().size());

    for (auto& [stage, system] : desc.stages()) {
        if (stage->executionModel == StageExecutionModel::PASSIVE) continue;

        auto& updateStage = *stages[stage->selfType.id()];

        auto result = updateStage.getReadSystemResults()[system.localID];

        stageReports.emplace_back(stage->type->name, result.slowestExecution,
            result.averageExecution, result.fastestExecution,
            result.totalExecutions
        );
    }
    return SystemProfileReport{&desc, std::move(stageReports)};
}

bool SystemRegistry::doSystemDependenciesExist(UpdateSystemDescriptor& descriptor) {
    auto ensureExists = [&](auto dependencies) {
        for (auto dependency : dependencies) {
            if (!systemKindRegistry.has(dependency)) {
                std::cout << "Dependency Not Found: " << systemKindRegistry.getFieldOf(dependency).type->name << std::endl;
                return false;
            }
        }
        return true;
    };
    bool exists = ensureExists(descriptor.sysReads());

    if (!exists) return false;
    exists = ensureExists(descriptor.sysWrites());
    if (!exists) return false;
    exists = ensureExists(descriptor.dependencies());
    return exists;
}

SystemErrorStack SystemRegistry::createExecutionGraph(StageEntry& stageEntry) {
    auto& updStage = *stageEntry.stagePtr;

    SystemErrorStack errors;
    if (!updStage.isOutdated()) return errors;

    const auto& systems = updStage.systems();

    for (size_t i = 0; i < systems.size();) {
        bool exists = doSystemDependenciesExist(updStage.graph[i]);

        if (!exists) {
            updStage.discard(i);
            i = 0;
        } else {
            ++i;
        }
    }

    std::cout << "Stage: " << updStage.stage->type->name << "\n";
    for (auto& system : updStage.systems()) {
        std::cout << "> System: " << systemKindRegistry.getFieldOf(system.descriptor->selfType).type->name << "\n";
    }
    stageEntry.initialize();

    if (updStage.stage->executionModel == StageExecutionModel::PASSIVE) return errors;

    ExecutionGraphCreationContext ctx(systemKindRegistry, stageKindRegistry);
    updStage.createExecutionGraph(ctx);

    return errors;
}

void SystemRegistry::createExecutionGraphs() {
    tbb::task_group group;

    std::vector<SystemErrorStack> errors;
    errors.assign(stages.size(), SystemErrorStack{});

    int idx = 0;

    for (auto& stage : stages) {
    //    group.run([&, j = idx]{
        int j = idx;
            errors[j] = createExecutionGraph(stage);
    //    });
        ++idx;
    }
  //  group.wait();

    // for (auto& [stage, sysHash] : errors) {
    //     std::cout << "System '" << TypeHashMap::get(sysHash) << "' was discarded in " << stage << " because: \n[\n";
    //     for (auto& dep : errors) {
    //         std::cout << "  " << dep.first << ": '" << TypeHashMap::get(dep.second) << "' does not exist,\n";
    //     }
    //     std::cout << "]\n";
    // }

    std::cout << "All Stages: \n";

    for (auto& stage : stages) {
        std::cout << stage->stage->type->name << "\n";
    }
    std::cout << "\n------------------\n\n";
    std::cout << "graph TD\n";
    for (auto& stage : stages) {
        stage->dumpMermaid(systemKindRegistry);
    }
    std::cout << "\n------------------\n\n";
}

std::ostream& operator << (std::ostream& stream, StageExecutionModel model) {
    switch (model) {
        case StageExecutionModel::SERIAL:
            stream << "Serial";
            break;
        case StageExecutionModel::PARALLEL:
            stream << "Parallel";
            break;
        case StageExecutionModel::DETERMINISTIC:
            stream << "Deterministic";
    }
    return stream;
}

std::ostream& operator << (std::ostream& stream, StageScheduleModel model) {
    switch (model) {
        case StageScheduleModel::PER_FRAME:
            stream << "Per Frame";
            break;
        case StageScheduleModel::FIXED_HZ:
            stream << "Fixed Tick Rate";
            break;
        case StageScheduleModel::PASSIVE:
            stream << "Passive";
        case StageScheduleModel::MANUAL:
            stream << "Manual";
            break;
    }
    return stream;
}
std::ostream& operator<<(std::ostream& stream, const SystemProfileReport::StageReport& report) {
    stream << "  |- Stage: " << report.name << "\n"
           << "  |   Slowest: " << report.slowestExecution << "\n"
           << "  |   Average: " << (report.totalExecutions ? report.averageExecution / report.totalExecutions : Time(0)) << "\n"
           << "  |   Fastest: " << report.fastestExecution << "\n"
           << "  |   Total Executions: " << report.totalExecutions << "\n"
           << "  |--------------------------------\n";
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const SystemProfileReport& report) {
    stream << "|--------------------------------------\n"
           << "| System Profile: " << report.system->name() << "\n"
           << "|--------------------------------------\n";
    for (const auto& stage : report.stageReports) {
        stream << stage;
    }
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const StageProfileReport& report) {
    if (!report.stage) return stream;

    stream << "|--------------------------------------\n"
           << "| Stage Profile: " << report.stage->stage->type->name << "\n"
           << "|--------------------------------------\n";
    stream << "  |- Systems Registered: " << report.stage->systems().size() << "\n";
    stream << "  |- Execution Model: "   << report.stage->stage->executionModel << "\n";
    stream << "  |- Execution Order: "   << report.stage->stage->scheduleModel << "\n";

    if (report.stage->stage->scheduleModel == StageScheduleModel::FIXED_HZ) {
        stream << "  |- Tick Rate: " << report.stage->stage->hz << "\n";
    } else {
        stream << "  |- Tick Rate: N/A\n";
    }
    stream << "  |---------------------------------\n";

    if (report.stage->stage->onStageBegin) {
        stream << "  |- onStageBegin: avg. " << report.stage->metrics.onStageBeginCounter / report.metrics.totalExecutions << "\n";
    } else {
        stream << "  |- onStageBegin: N/A\n";
    }
    if (report.stage->stage->onStageEnd) {
        stream << "  |- onStageEnd:   avg. " << report.stage->metrics.onStageEndCounter / report.metrics.totalExecutions << "\n";
    } else {
        stream << "  |- onStageEnd: N/A\n";
    }

    stream << "  |---------------------------------\n";

    if (report.metrics.totalExecutions > 0) {
        stream << "  |- Total Executions:  " << report.metrics.totalExecutions << "\n"
               << "  |- Fastest Execution: " << report.metrics.fastestExecutionTime << "\n"
               << "  |- Slowest Execution: " << report.metrics.slowestExecutionTime << "\n"
               << "  |- Average Execution: " << report.metrics.totalExecutionTime / report.metrics.totalExecutions << "\n"
               << "  |---------------------------------\n";
    } else {
        stream   << "  |- Total Executions:  " << 0 << "\n"
                 << "  |- Fastest Execution: " << "N/A" << "\n"
                 << "  |- Slowest Execution: " << "N/A" << "\n"
                 << "  |- Average Execution: " << "N/A" << "\n"
                 << "  |---------------------------------\n";
    }

    return stream;
}