void report(char* msg);

// flags in waylogo_config.flags:
#define CONFIG_QUIET 0x01
#define CONFIG_RENDER 0x02
#define CONFIG_SHARP 0x04
#define CONFIG_SHAPE 0x08

struct waylogo_config {
	int flags;
};

struct waylogo_config *waylogo_configure(int argc, char** argv);
