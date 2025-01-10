#pragma once
#ifndef COMPLETE_HPP
#define COMPLETE_HPP

#include "engine/ebus.hpp"

class ModuleCompleteEvent : public BaseEvent {
public:
    explicit ModuleCompleteEvent(const std::string& moduleName) {
        setData("module_name", moduleName);
    }
    
    std::type_index getType() const override {
        return std::type_index(typeid(ModuleCompleteEvent));
    }
    
    std::string getName() const override {
        return "ModuleComplete";
    }
};

class AllModulesCompleteEvent : public BaseEvent {
public:
    std::type_index getType() const override {
        return std::type_index(typeid(AllModulesCompleteEvent));
    }
    
    std::string getName() const override {
        return "AllModulesComplete";
    }
};

#endif // COMPLETE_HPP