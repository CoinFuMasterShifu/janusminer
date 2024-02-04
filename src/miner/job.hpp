#pragma once
#include "mine_job.hpp"
#include "stratum/job.hpp"
#include <variant>

namespace job {
struct Job : public std::variant<MineJob, StratumJob> {
    template <typename T>
    Job(T&& t)
        : variant(std::forward<T>(t))
    {
    }
    auto target() const{ 
        return std::visit([&](auto& j){ return j.target();}, *this);
    }
    auto header() const{
        return std::visit([](auto& job) { return job.header(); }, *this);
    }
    size_t clean_index() const
    {
        return std::visit([](auto& job) { return job.clean_index(); }, *this);
    }
};
using OptJob = std::optional<Job>;

struct NoneGenerator;
struct NoneGeneratorArg {
    using generator_t = NoneGenerator;
};
struct NoneGenerator {
    NoneGenerator(const NoneGeneratorArg&){}
    NoneGenerator(){}
    OptJob generate_job() const
    {
        return {};
    }
};

using generator_variant_t = std::variant<NoneGenerator, NodeJobGenerator, StratumJobGenerator>;
using GeneratorArg = std::variant<NoneGeneratorArg, MineJob, StratumGeneratorArgs>;

class Generator {
    generator_variant_t variant;

public:
    Generator() {};
    template <typename T>
    Generator(T&& arg)
        : variant(std::in_place_type<typename T::generator_t>, std::forward<T>(arg)) {}
    template <typename T>
    void from_arg(T&& arg)
    {
        variant.emplace<typename std::remove_cv_t<T>::generator_t>(std::forward<T>(arg));
    }
    OptJob generate()
    {
        return std::visit([&](auto& generator) -> OptJob {
            return generator.generate_job();
        },
            variant);
    }
};
}
