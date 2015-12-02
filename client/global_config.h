#define BUFFER_SIZE               1024  
#define PORT_NO                  20001
#define DELAY                        1 // ms

static pthread_t transmitThread;
static pthread_t resultThread;
static pthread_t mflistenThread;

pthread_mutex_t sendLock;         // mutex lock to make sure transmit order
unordered_set<int> id_set;        // set to store the sock id used  
int global_dst_GUID;              // used for close command
queue<struct timeval> *timeQueue; // queue for timestamps

int global_stop = 0;
int orbit = 0;
int neworbit = 0;
int sb = 0;
bool test = false;
bool consume = false;
int size_per_time = 1;
int fake_id = 0;
char *userID;
int debug = 0;
MsgDistributor MsgD;
MFPackager* mfpack;
int drawResult = 0;
string resultShown = "";
string result_title = "";
string result_artist = "";
string result_date = "";
string matchIndex = "";
float coord[8];
int delay_time = 0;

struct arg_transmit {
    int sock;
    char file_name[100];
};
