/*
 * gc.h - the single header a generator includes to get a garbage collector.
 *
 *   cl /DGC_OPTION=refcount     ... -> Option A: reference counting
 *   cl /DGC_OPTION=marksweep    ... -> Option B: mark-sweep (+ coalescing)
 *   cl /DGC_OPTION=compact      ... -> Option C: compacting / moving
 *   cl /DGC_OPTION=conservative ... -> Option D: conservative stack scanning
 *
 * Include once, after c_funcs.h. This is the include switch: gc_common.h,
 * the one option header selected by GC_SELECTED, gc_defaults.h, gc_fuzz.h,
 * and gc_demo.h. Each option header defines GC_HAS_<name> for the
 * interface functions it overrides. gc_defaults.h fills in the rest, so
 * one tour body compiles against any collector. See GC_imp.md and GC.md.
 * GC_SWITCH is set so a later header can offer the calls that need this
 * collector. It is not an include guard.
 */
#define GC_SWITCH 1
#include "gc_common.h"
#if GC_SELECTED == GCV_refcount
#include "gc_refcount.h"
#elif GC_SELECTED == GCV_marksweep
#include "gc_marksweep.h"
#elif GC_SELECTED == GCV_compact
#include "gc_compact.h"
#elif GC_SELECTED == GCV_conservative
#include "gc_conservative.h"
#else
#error "GC_OPTION must be one of: refcount marksweep compact conservative"
#endif
#include "gc_defaults.h"
#include "gc_fuzz.h"
#include "gc_demo.h"
