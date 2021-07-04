#include "Logging.h"

namespace LOGGING
{

std::recursive_timed_mutex Logging::mutx{}; ///< API access mutex

} // namespace LOGGING