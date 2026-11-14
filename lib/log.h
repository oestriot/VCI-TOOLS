#ifndef LOG_H
#define LOG_H 1

void enable_logging();
void disable_logging();
int is_logging_enabled();

#define LOG(...) if(is_logging_enabled()) { printf(__VA_ARGS__); }
#define LOG_BOOL(label, b) LOG("%s: %s\n", label, b ? "True" : "False");
#define LOG_BUFFER(label, buf, sz) if(is_logging_enabled()) { LOG("%s:", label); for(size_t i = 0; i < sz; i++) { LOG(" %02X", ((uint8_t*)(buf))[i]); } LOG("\n"); }
#define LOG_LINE(...) if(is_logging_enabled()) { LOG(__VA_ARGS__); LOG("\n"); }


#endif