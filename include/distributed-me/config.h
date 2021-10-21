#ifndef DISTRIBUTED_ME_CONFIG_H_
#define DISTRIBUTED_ME_CONFIG_H_

#define MAX_SERVERS                 3
#define MAX_CLIENTS                 5
#define MAX_BUFFER_SIZE             8192
#define MAX_PATH_LENGTH             4096
#define MAX_FILENAME_LENGTH         256
#define MESSAGE_QUEUE_MAX_SIZE      100

#define MAX_MESSAGE_QUEUE_TIME_OUT  10
#define MAX_SERVING_TIME            100
#define MESSAGE_QUEUE_SLEEP_TIME    10


#define CONFIG_FILE                     "../config/config_servers.txt"
#define GROUP_SERVER_HOSTED_DIRECTORY   "../test_data/servers"
#define GROUP_CLIENT_INPUT_DIRECTORY    "../test_data/clients"
#define SERVER_LOG_DIR                  "../logs"
#define CLIENT_LOG_DIR                  "../logs"

#endif // DISTRIBUTED_ME_CONFIG_H_
