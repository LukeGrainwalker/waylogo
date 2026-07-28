void report(char* fmt, ...);
void ireport(char* fmt, ...);
void wreport(char* msg);
void report_prio(int priority, char* fmt, ...);
void report_start(int log_level);
void report_end();

// flags in waylogo_config.flags:
#define CONFIG_QUIET 0x01
#define CONFIG_RENDER 0x02
#define CONFIG_SHARP 0x04
#define CONFIG_SHAPE 0x08
#define CONFIG_HELP 0x10
#define CONFIG_DEBUG 0x20

struct waylogo_config {
	int flags;
	int log_level;
};

struct waylogo_config *waylogo_configure(int argc, char** argv);
