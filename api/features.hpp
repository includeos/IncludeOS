#ifndef INCLUDEOS_FEATURES_HPP
#define INCLUDEOS_FEATURES_HPP

/**
 * Toggles for experimental features.
 *
 * Guidelines:
 *
 * 1. Prefer if constexpr over #ifdef when checking for these as it will allow
 * the compiler instrumentation to process the contained code preventing rot.
 *
 * 2. If the feature provides some value as is it can be stored as as a
 * std::optional data member, initialized with
 * if constexpr (toggle_enabled) { ... }
 *
 * 3. Use static constexpr bool enable_feature = INCLUDEOS_EXPERIMENTAL_FEATURE;
 * for compile time toggles, and expose these to user code to allow for runtime
 * checks by code compiled without visibility of these macros.
 *
 * 4. Use extern const bool enable_feature; for runtime toggles. The
 * if constexpr (toggle_enabled) { ... } will work for both runtime and
 * compile time toggles.
 * TODO: We might need to standardise how to define and toggle runtime toggles.
 *
 * 5. Add CMake options for toggles as the features mature.
 *
 * 6. Put experimental / incomplete features in an 'experimental' namespace to
 * clearly communicate this to users.
 */

#ifndef INCLUDEOS_EXPERIMENTAL_MLD2
#define INCLUDEOS_EXPERIMENTAL_MLD2 false
#endif

#endif
