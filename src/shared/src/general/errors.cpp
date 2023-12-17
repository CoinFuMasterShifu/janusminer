#include "errors.hpp"
#include "block/chain/height.hpp"

namespace errors {
#define ERR_NAME_GEN(code, name, _) \
    case code:                      \
        return #name;
const char* err_name(int err)
{

    switch (err) {
        ADDITIONAL_ERRNO_MAP(ERR_NAME_GEN)
    }
    return "UNKNOWN";
}
#undef ERR_NAME_GEN

#define STRERROR_GEN(code, name, str) \
    case code:                        \
        return str;
const char* strerror(int err)
{
    switch (err) {
        ADDITIONAL_ERRNO_MAP(STRERROR_GEN)
    }
    return "UNKNOWN";
}
#undef STRERROR_GEN

} // namespace errors

ChainError::ChainError(int32_t e, NonzeroHeight height)
    : Error(e)
    , h(height.value()) {};

NonzeroHeight ChainError::height() const
{
    return NonzeroHeight(h);
};
