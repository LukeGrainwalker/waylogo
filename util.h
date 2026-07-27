enum waylogo_error {
	WLERR_GENERIC,
	WLERR_EXIST,
	WLERR_LOAD,
	WLERR_WDEBUG, // wayland protocol debug
};

void report(char* msg);
void wreport(char* msg);
void report_code(enum waylogo_error err, char* info);
void report_start();
void report_end();
// flags in waylogo_config.flags:
#define CONFIG_QUIET 0x01
#define CONFIG_RENDER 0x02
#define CONFIG_SHARP 0x04
#define CONFIG_SHAPE 0x08
#define CONFIG_HELP 0x10

struct waylogo_config {
	int flags;
};

struct waylogo_config *waylogo_configure(int argc, char** argv);
