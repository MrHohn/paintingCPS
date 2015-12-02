#define BUFFER_SIZE               1024  
#define PORT_NO                  20001
#define MAX_CONNECTION              10

static pthread_t mflistenThread;

// semaphore to limit the number of threads would be launched
sem_t process_sem;
// global flag for quit
int global_stop = 0; 
bool orbit = false;
bool mf = false;
bool tcp = true;
bool storm = false;
bool train = false;
string spoutIP;
int debug = 0;

// MFPackager
MFPackager* mfpack;

MsgDistributor MsgD;

// Kafka Producer
bool kafka = false;
KafkaProducer* producer;

// Metrics recorder
Metrics* metrics;

// map for result thread to search the queue address
unordered_map<string, queue<string>*> queue_map;
// mutex lock for queue_map operation
pthread_mutex_t queue_map_lock;
// map for transmit thread to search the semaphore address
unordered_map<string, sem_t*> sem_map;
// mutex lock for sem_map operation
pthread_mutex_t sem_map_lock;
// map to store logged in userID
unordered_map<String, uint> user_map;
// mutex lock for user_map operation
pthread_mutex_t user_map_lock;

struct arg_result {
    int sock;
    char file_name[100];
};
