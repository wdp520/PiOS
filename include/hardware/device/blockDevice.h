typedef enum {
    OpRead,
    OpWrite,
    OpGetStatus //BANANA
} BlockDevOp;

typedef struct {
    char name[11];
    unsigned char* buffer;
    unsigned int name_length;
    int(*init)(void);
    unsigned int(*operation)(BlockDevOp opCode, void* arg, void *arg2);
    void(*cleanup)(void);
} BlockDevice;