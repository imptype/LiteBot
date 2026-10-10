/* json.h - read a few fields out of a JSON message in place: no tree, no allocation, no JSON library.
 *
 * A "value" is a pointer to its first character inside the NUL-terminated text of one message; the helpers walk the text where
 * it is. Enough for the handful of fields this bot reads from Discord and from config.json.
 *
 *   j_get(obj, key)      value of key in the object at obj (NULL if missing; also NULL when obj is NULL, so calls chain)
 *   j_first / j_next     walk the elements of an array
 *   j_int / j_copy / j_eq / j_is_str    read a value as a number / copy a string out / compare it with a string
 *   j_skip               step over a value (also tells whether the text is well formed)
 *
 * Limits on purpose: keys are matched as written (a key containing an escape sequence never matches), and j_copy refuses strings
 * with escapes. Every id, token and name this bot reads is plain.
 */
#ifndef JSON_H
#define JSON_H
#include <string.h>

static const char *j_ws(const char *p) { while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++; return p; }
static const char *j_str_end(const char *p) {                  /* p just after an opening quote: the closing quote, or NULL */
    for (; *p; p++) { if (*p == '\\' && p[1]) p++; else if (*p == '"') return p; }
    return NULL;
}
static const char *j_skip(const char *p) {                     /* p at a value: just past it, or NULL if malformed */
    p = j_ws(p);
    if (*p == '"') { p = j_str_end(p + 1); return p ? p + 1 : NULL; }
    if (*p == '{' || *p == '[') {
        for (int depth = 0; *p; p++) {
            if (*p == '"') { p = j_str_end(p + 1); if (!p) return NULL; }
            else if (*p == '{' || *p == '[') depth++;
            else if (*p == '}' || *p == ']') { if (--depth == 0) return p + 1; }
        }
        return NULL;
    }
    if (!*p || *p == ',' || *p == '}' || *p == ']' || *p == ':') return NULL;
    while (*p && *p != ',' && *p != '}' && *p != ']' && *p != ' ' && *p != '\n' && *p != '\r' && *p != '\t') p++;   /* number, true, false, null */
    return p;
}
static const char *j_get(const char *obj, const char *key) {   /* value of `key` in the object at obj, or NULL (also for obj == NULL) */
    if (!obj) return NULL;
    size_t kl = strlen(key);
    const char *p = j_ws(obj);
    if (*p != '{') return NULL;
    p = j_ws(p + 1);
    while (*p == '"') {
        const char *ks = p + 1, *ke = j_str_end(ks);
        if (!ke) return NULL;
        p = j_ws(ke + 1);
        if (*p++ != ':') return NULL;
        p = j_ws(p);
        if ((size_t)(ke - ks) == kl && !memcmp(ks, key, kl)) return p;
        if (!(p = j_skip(p))) return NULL;
        p = j_ws(p);
        if (*p != ',') return NULL;
        p = j_ws(p + 1);
    }
    return NULL;
}
static const char *j_first(const char *arr) {                  /* first element of the array at arr, or NULL */
    if (!arr) return NULL;
    const char *p = j_ws(arr);
    if (*p != '[') return NULL;
    p = j_ws(p + 1);
    return *p == ']' ? NULL : p;
}
static const char *j_next(const char *e) {                     /* element after e, or NULL at the end */
    const char *p = j_skip(e);
    if (!p) return NULL;
    p = j_ws(p);
    return *p == ',' ? j_ws(p + 1) : NULL;
}
static int j_is_str(const char *v) { return v && *v == '"'; }
static long long j_int(const char *v, long long dflt) {        /* integer value (a fraction is cut off), or dflt if v is not a number */
    if (!v) return dflt;
    int neg = *v == '-';
    if (neg) v++;
    if (*v < '0' || *v > '9') return dflt;
    long long x = 0;
    while (*v >= '0' && *v <= '9') x = x * 10 + (*v++ - '0');
    return neg ? -x : x;
}
static int j_copy(const char *v, char *out, size_t cap) {      /* string value -> out. 1 = ok (it fits and has no escapes) */
    if (!j_is_str(v)) return 0;
    const char *e = j_str_end(v + 1);
    if (!e || (size_t)(e - v - 1) >= cap || memchr(v + 1, '\\', (size_t)(e - v - 1))) return 0;
    memcpy(out, v + 1, (size_t)(e - v - 1));
    out[e - v - 1] = 0;
    return 1;
}
static int j_eq(const char *v, const char *s) {                /* v is the string s */
    size_t n = strlen(s);
    return j_is_str(v) && !strncmp(v + 1, s, n) && v[1 + n] == '"';
}
#endif