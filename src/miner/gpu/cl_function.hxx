#pragma once
#include "cl_helper.hpp"
#include "log/trace.hpp"
#include "spdlog/spdlog.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>

template <typename T, size_t N>
struct Vec {
    std::vector<T> vdata;
    auto begin() const { return vdata.begin(); }
    auto end() const { return vdata.end(); }
    size_t size() const { return vdata.size(); }
    size_t bytesize() const { return size() * sizeof(T); }
    auto data() const { return vdata.data(); }
    auto data() { return vdata.data(); }
    auto& operator[](size_t i) const { return vdata[i]; }
    Vec(const Vec&) = delete;
    Vec(Vec&&) = default;
    Vec()
        : vdata(N)
    {
        assert(vdata.size() == N);
    }
};
template <typename... IN>
struct CLFunction {
    template <typename... OUT>
    struct Returning {

        template <typename T>
        struct run_arg_type { };

        template <typename T, size_t N>
        requires std::is_trivial_v<T>
        struct run_arg_type<std::array<T, N>> {
            using type = std::span<T, N>;
        };
        template <typename T>
        requires std::is_trivial_v<T>
        struct run_arg_type<T> {
            using type = T;
        };
        template <typename T>
        using run_arg_t = run_arg_type<T>::type;

        template <typename T>
        struct prepare_type { };
        template <typename T, size_t N>
        requires std::is_trivial_v<T>
        struct prepare_type<std::array<T, N>> {
            using type = CL::Buffer;
        };
        template <typename T>
        requires std::is_trivial_v<T>
        struct prepare_type<T> {
            using type = T;
        };
        template <typename T>
        using in_prepare_t = prepare_type<T>::type;

        template <typename T, size_t N>
        requires std::is_trivial_v<T>
        using svj = size_t;

        template <typename T>
        using sv = size_t;
        template <typename T>
        using clb = CL::Buffer;

        template <typename T>
        struct ArgInfo {
            static constexpr size_t size = 0;
        };
        template <size_t N>
        using IN_TYPE = std::tuple_element_t<N, std::tuple<IN...>>;
        template <size_t N>
        using OUT_TYPE = std::tuple_element_t<N, std::tuple<OUT...>>;

        template <size_t N, bool in>
        struct typestruct { };
        template <size_t N>
        struct typestruct<N, false> {
            using type = OUT_TYPE<N>;
        };
        template <size_t N>
        struct typestruct<N, true> {
            using type = IN_TYPE<N>;
        };
        template <size_t N, bool in>
        using TYPE = typestruct<N, in>::type;

        template <typename T>
        requires std::is_trivial_v<T>
        struct ArgInfo<T> {
            static constexpr size_t size = sizeof(T);
        };
        template <typename T, size_t N>
        requires std::is_trivial_v<T>
        struct ArgInfo<std::array<T, N>> {
            static constexpr size_t size = sizeof(T) * N;
        };
        template <typename T, size_t N>
        struct ArgInfo<Vec<T, N>> {
            static constexpr size_t size = sizeof(T) * N;
        };
        cl::KernelFunctor<in_prepare_t<IN>..., clb<OUT>...> functor;
        std::tuple<CL::RetvalGenerator<OUT>...> generators;
        Returning(const cl::Program& program, const std::string name)

            : functor(program, name) {};
        [[nodiscard]] cl::Context context()
        {
            cl::Context ctx(
                functor.getKernel().template getInfo<CL_KERNEL_CONTEXT>());
            return ctx;
        }

        void throw_missing(size_t index)
        {
            throw std::runtime_error(std::string("Missing value at index ") + std::to_string(index) + ".");
        }

        template <size_t N, bool in>
        auto& get_optional()
        {
            if constexpr (in) {
                return std::get<N>(inBuffers);
            } else {
                return std::get<N>(outBuffers);
            }
        }
        template <size_t N>
        [[nodiscard]] size_t override_reserve_outvector(size_t elements)
        {
            using V=OUT_TYPE<N>;
            static_assert(CL::VectorChecker<V>::isVector, "This method is only for POD std::vector types");
            constexpr size_t elemSize{sizeof(V{}[0])};
            std::optional<clb<V>>& b { std::get<N>(outBuffers) };
            size_t newSize{elements*elemSize};
            if (b.has_value() && b->size() == newSize) 
                return elements;
            b.reset();
            while (true) {
                size_t bufsize=elements*elemSize ;
                try {
                    b = CL::Buffer { context(), CL_MEM_READ_WRITE, bufsize};
                    break;
                }catch(cl::Error& e) {
                    trace_log().error("[CLBUFFER/FAILED] {}, {}", bufsize, e.err());
                    if (e.err() != -61 || elements <= 1){
                        spdlog::error("Cannot create buffer of size {}, error {}", bufsize, e.err());
                        throw;
                    }
                    elements/=2;
                }
            }
            return elements;
        }


        template<size_t N> 
        auto& generator(){return std::get<N>(generators);}

    private:
        template <size_t N, bool in>
        void allocate_missing_at(const cl::Context& ctx)
        {
            auto& r = get_optional<N, in>();
            if (!r.has_value()) {
                using VT = TYPE<N, in>;
                if constexpr (in == true && std::is_integral_v<VT>) { // special treatment for trivial
                    // input parameters: they don't need
                    // a CL::Buffer
                    r = VT {};
                } else {
                    constexpr auto s = ArgInfo<VT>::size;
                    if constexpr (s == 0)
                        throw_missing(N);
                    r = CL::Buffer { ctx, (in ? CL_MEM_READ_ONLY : CL_MEM_READ_WRITE), s };
                }
            }
        }
        template <size_t N, bool in>
        void allocate_missing(const cl::Context& ctx)
        {
            allocate_missing_at<N, in>(ctx);
            if constexpr (N > 0) {
                allocate_missing<N - 1, in>(ctx);
            }
        }
        template <typename T>
        void copy_arg(CL::CommandQueue& queue, T&& v, CL::Buffer& buf)
        {
            cl::copy(queue, begin(v), end(v), buf);
        }

        template <typename T>
        requires std::is_trivial_v<std::remove_reference_t<T>>
        void copy_arg(CL::CommandQueue&, T&& v,
            std::remove_const_t<std::remove_reference_t<T>>& out)
        {
            out = v;
        }

        template <size_t pos, typename arg, typename... REST>
        void copy_first_arg_recursive(CL::CommandQueue& queue, arg&& a,
            REST&&... rest)
        {
            auto& buf = std::get<pos>(inBuffers).value();
            copy_arg(queue, std::forward<arg>(a), buf);
            copy_first_arg_recursive<pos + 1>(queue, std::forward<REST>(rest)...);
        }
        template <size_t pos, typename arg>
        void copy_first_arg_recursive(CL::CommandQueue& queue, arg&& a)
        {
            auto& buf = std::get<pos>(inBuffers).value();
            copy_arg(queue, std::forward<arg>(a), buf);
        }

    public:
        void allocate_missing()
        {
            auto ctx { context() };
            if constexpr (inSize > 0)
                allocate_missing<inSize - 1, true>(ctx);
            if constexpr (outSize > 0)
                allocate_missing<outSize - 1, false>(ctx);
        }

        std::tuple<std::optional<in_prepare_t<IN>>...> inBuffers;
        std::tuple<std::optional<clb<OUT>>...> outBuffers;
        static constexpr size_t inSize = std::tuple_size_v<decltype(inBuffers)>;
        static constexpr size_t outSize = std::tuple_size_v<decltype(outBuffers)>;
        static constexpr bool hasArguments = inSize != 0;
        static constexpr bool hasNoArguments = inSize == 0;

        template <typename T, typename>
        struct index_runner { };

        template <size_t... is, size_t... os>
        struct index_runner<std::index_sequence<is...>, std::index_sequence<os...>> {
            template <typename Functor, typename TupleIn, typename TupleOut>
            static void apply(Functor& functor, cl::EnqueueArgs& ea, TupleIn&& tin,
                TupleOut& tout)
            {
                functor(ea, (std::get<is>(tin).value())...,
                    (std::get<os>(tout).value())...);
            }
            template <typename T,typename G>
            static auto return_tuple(CL::CommandQueue& queue, T&& outBuffers, G&& generators)
            {
                return std::tuple<OUT...>(queue.read<OUT>(
                    std::get<os>(std::forward<T>(outBuffers)).value(),std::get<os>(std::forward<G>(generators)))...);
            }
        };

        [[nodiscard]] auto run(CL::CommandQueue& queue, cl::EnqueueArgs& ea,
            const run_arg_t<IN>&... in)
        requires(outSize != 0)
        {
            allocate_missing();
            if constexpr (hasArguments) {
                copy_first_arg_recursive<0>(queue, in...);
            }
            using ir = index_runner<std::make_index_sequence<inSize>,
                std::make_index_sequence<outSize>>;
            ir::apply(functor, ea, inBuffers, outBuffers);
            return ir::return_tuple(queue, outBuffers, generators);
        }
        void run(CL::CommandQueue& queue, cl::EnqueueArgs& ea,
            const run_arg_t<IN>&... in)
        requires(outSize == 0)
        {
            allocate_missing();
            if constexpr (hasArguments) {
                copy_first_arg_recursive<0>(queue, in...);
            }
            using ir = index_runner<std::make_index_sequence<inSize>,
                std::make_index_sequence<outSize>>;
            ir::apply(functor, ea, inBuffers, outBuffers);
        }
    };
};
