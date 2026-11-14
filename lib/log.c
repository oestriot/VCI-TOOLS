#include "log.h"

static int G_LOGGING_ENABLED = 0;

void enable_logging() {
	G_LOGGING_ENABLED = 1;
}

void disable_logging() {
	G_LOGGING_ENABLED = 0;
}

int is_logging_enabled() {
	return G_LOGGING_ENABLED;
}