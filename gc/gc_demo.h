
/*
 * gc_demo.h - one language-agnostic garbage-collection demo body, shared by all
 * four tours (py / csharp / rust / js).
 *
 * It is written entirely against the common gc_* interface, so the SAME code
 * exercises whichever collector was selected with -DGC_OPTION.  Each language
 * main() builds three message strings with its own string helper (they all share
 * the {offset,length} + print-proc shape) and hands them here, keeping the tours
 * free of duplicated collector-driving logic.
 *
 * What it proves at run time:
 *   1. allocate two managed Nodes a and b, link a.next = b (a real write barrier
 *      under refcount) and print their heap addresses;
 *   2. drop the roots and run the collector;
 *   3. allocate a third Node and print ITS address - which equals a freed one,
 *      demonstrating the space was actually reclaimed and reused.
 *   4. gc_fuzz (gc_fuzz.h) builds a three-node graph, collects, and exits
 *      nonzero if a rooted value or link did not survive. That check is
 *      part of this body, so it does not add an assemble.
 *
 * gc.h includes this header after gc_fuzz.h.
 */

/* A console message: its data offset and byte length, tagged for the print proc
 * (lpBuffer / nBytes) - exactly what every wrapper's string helper produces. */
typedef struct { AsmDataField off; AsmDataField len; } GcMsg;

/* Build a console message from buffer off and length len. */
static inline GcMsg gc_msg(AsmDataField off, AsmDataField len) {
    GcMsg m = { off, len };
    return m;
}

/*
 * gc_msg_of - build a GcMsg directly from any wrapper string handle (CppStr,
 * CsStr, PyStr, JsStr, RsStr - all `{off, len}`). A macro so it accepts every
 * handle type; keeps generators from reaching into a handle's `.off` / `.len`.
 */
#define gc_msg_of(s) gc_msg((s).off, (s).len)

/* Run the shared collector tour: allocate two nodes, link them, collect,
 * allocate again, then gc_fuzz. print writes hdr, alloc, and reuse.
 * heap is the collector and node is the managed type. */
static inline void gc_demo(AsmExtern* print, GcHeap* heap, const GcType* node,
                           GcMsg hdr, GcMsg alloc, GcMsg reuse) {
    write_call(print, hdr.off, hdr.len);

    gc_start(heap);
    {
        GcScope sc = gc_scope_begin();

        CValue a = gc_new(heap, node, "a");     /* first managed object          */
        gc_root(&sc, &a);
        CValue b = gc_new(heap, node, "b");     /* second managed object         */
        gc_root(&sc, &b);

        gc_set(heap, a, GC_HDR, b);             /* a.next = b (write barrier)     */

        write_call(print, alloc.off, alloc.len);
        gc_hex(heap, a);                        /* print a's address             */
        gc_hex(heap, b);                        /* print b's address             */

        gc_scope_end(heap, &sc);                /* refcount reclaims here         */
        gc_collect(heap);                       /* tracing/moving reclaim here    */

        write_call(print, reuse.off, reuse.len);
        CValue c = gc_new(heap, node, "c");     /* reuses freed space            */
        gc_hex(heap, c);                        /* address matches a freed one   */
        gc_fuzz(heap, node, &a, &b, &c);
    }
}

