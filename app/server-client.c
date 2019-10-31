#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <net/if.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>

#include "pthread.h"
#include <unistd.h>
#include <arpa/inet.h>

#include "config.h"

#include "encode/decode.h"
#include "encode/encode.h"
#include "logging/logging.h"

#include "iota/send-msg.h"

#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "proto_compiled/DataResponse.pb.h"
#include "proto_compiled/DataRequest.pb.h"
#include "proto_compiled/FeatureResponse.pb.h"

//tmp
#include <errno.h>

char iota_seed[81];

sensor_node_t sensor_nodes[SENSOR_NODES_LENGTH];

int sock;
bool client_is_running = true;

sensor_command_t get_rpc_sensor_command(uint8_t byte) {
    switch(byte){
        case 33:
            return FEATURE_REQUEST_CMD;
        case 34:
            return DATA_REQUEST_CMD;
        case 35:
            return DATA_RESPONSE_CMD;
        case 36:
            return FEATURE_RESPONSE_CMD;
        case 88:
            return SETUP_TEST_CMD;
        default:
            return NO_CMD;
    }
}

uint8_t get_rpc_command_byte(sensor_command_t command) {
    switch(command){
        case FEATURE_REQUEST_CMD:
            return 33;
        case DATA_REQUEST_CMD:
            return 34;
        case DATA_RESPONSE_CMD:
            return 35;
        case FEATURE_RESPONSE_CMD:
            return 36;
        case SETUP_TEST_CMD:
            return 88;
        default:
            return 0;
    }
}


void init_sensor_config(int argc, char *argv[]) {
    //iota-seed client-address sensor-address sensor-port interface-name
    sensor_node_t *node = &sensor_nodes[0];

    if (inet_pton(AF_INET6, argv[3], &node->config.address.sin6_addr) == 1) // success!
    {
        log_string("DEBUG", "sensor_config", "sensor_ipv6", argv[3]);
    }
    else
    {
        log_string("ERROR", "client_init", "sensor_ipv6", "FAILED to parse sensor IPv6!");
    }

    node->config.address.sin6_family = AF_INET6;
    node->config.address.sin6_scope_id = 0x00;
    node->config.address.sin6_port = htons(51037);
}

struct sockaddr_in6 client_addr;
void init_client(int argc, char *argv[]) {
    //iota-seed client-address sensor-address sensor-port interface-name

    unsigned int client_addr_len = sizeof(client_addr);
    memset(&client_addr, 0, client_addr_len);
    client_addr.sin6_family = AF_INET6;
    client_addr.sin6_port = htons(argv[2]);
    client_addr.sin6_scope_id = if_nametoindex(argv[5]);

    if (inet_pton(AF_INET6, argv[2], &client_addr.sin6_addr) == 1) // success!
    {
        log_string("DEBUG", "client_init", "client_ipv6", argv[2]);
    }
    else
    {
        log_string("ERROR", "client_init", "client_ipv6", "FAILED to parse Client IPv6!");
    }

    if (DEBUG_SERVER) {
        log_addr("DEBUG", "client_init", "client_addr", &client_addr);
        log_int("DEBUG", "client_init", "client_port", ntohs(client_addr.sin6_port));
        log_hex("DEBUG", "client_init", "client_scope_id", client_addr.sin6_scope_id);
    }

    if ((sock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        log_int("ERROR", "client_init", "server_socket", sock);
    }

    if (bind(sock, (struct sockaddr *) &client_addr, client_addr_len) < 0) {
        log_string("ERROR", "client_init", "bind_client_socket", "bind failed");
        fprintf(stderr, "socket() failed: %s\n", strerror(errno));
        exit(1);
    }

    init_sensor_config(argc, argv);
}

void client_stop(void) {
    client_is_running = false;
}

void clear_socket_buffer(uint8_t *socket_buffer){
    memset(socket_buffer, 0, SOCKET_BUFFER_SIZE);
}

void send_buffer(struct sockaddr_in6 *server_addr_ptr, uint8_t *encode_buffer, int encode_buffer_length) {
    if (DEBUG_SERVER) {
        log_int("DEBUG", "send_command", "command", encode_buffer[0]);
        log_int("DEBUG", "send_command", "buffer_length", encode_buffer_length);
        log_addr("DEBUG", "send_command", "server_addr", server_addr_ptr);
        log_int("DEBUG", "send_command", "server_port", ntohs(server_addr_ptr->sin6_port));
    }

    sendto(
            sock, encode_buffer, encode_buffer_length, 0,
            (struct sockaddr *) server_addr_ptr, sizeof(struct sockaddr_in6));
}

int get_env_sensor_rpc_command_name(char *result, sensor_command_t command) {
    switch (command) {
        case FEATURE_REQUEST_CMD:
            strcat(result, "FEATURE_REQUEST_CMD\0");
            break;
        case DATA_REQUEST_CMD:
            strcat(result, "DATA_REQUEST_CMD\0");
            break;
        case DATA_RESPONSE_CMD:
            strcat(result, "DATA_RESPONSE_CMD\0");
            break;
        case FEATURE_RESPONSE_CMD:
            strcat(result, "DATA_RESPONSE_CMD\0");
            break;
        default:
            strcat(result, "NONE_CMD\0");
            break;
    }

    return 0;
}

bool check_ip_address(uint8_t *ip_address, uint8_t *ip_address_to_match) {
    for(int i = 0; i < 16; i++){
        if(ip_address[i] != ip_address_to_match[i]){
            return false;
        }
    }
    return true;
}

float get_scaled_value(environmentSensors_SingleDataPoint *data_point) {
    if(data_point->value == 0){
        return 0;
    }else{
        return data_point->value / pow(10, -data_point->scale);
    }
}

int get_data_ring_position(env_sensor_data_ring_t *data_ring) {
    int current_index = data_ring->position;

    if(current_index == DATA_SIZE){
        data_ring->position = 0;
        return 0;
    }else{
        data_ring->position = current_index + 1;
        return current_index;
    }
}

void add_to_data_ring(env_sensor_data_ring_t *data_ring, env_sensor_data_t *sensor_data) {
    int position = get_data_ring_position(data_ring);
    if(DEBUG_SERVER){
        log_int("DEBUG", "add_to_data_ring", "position", position);
        log_sensor_data("DEBUG", "add_to_data_ring", "sensor_data", sensor_data);
    }
    //log_int("DEBUG", "add_", "temperature", get_scaled_value(&data_ring->data[0]));
    memcpy(&data_ring->data[position], sensor_data, sizeof(env_sensor_data_t));
}

char json_buffer[JSON_BUFFER_SIZE];
void clear_json_buffer(){
    memset(json_buffer, 0, JSON_BUFFER_SIZE);
}

void write_temperature_to_buffer(char * buffer, environmentSensors_SingleDataPoint *data) {

    char first[] = ", \"temperature\":{ \"scale\": ";
    strcat(buffer, first);

    char scale[10];
    sprintf(scale, "%d", data->scale);
    strncat(buffer, &scale, sizeof(int32_t));

    char second[] = ", \"value\": ";
    strcat(buffer, second);

    char value[10];
    sprintf(value, "%d", data->value);
    strncat(buffer, &value, sizeof(int32_t));

    strcat(buffer, "}");

}

void write_humidity_to_buffer(char * buffer, environmentSensors_SingleDataPoint *data) {

    char first[] = ", \"humidity\":{ \"scale\": ";
    strcat(buffer, first);

    char scale[10];
    sprintf(scale, "%d", data->scale);
    strncat(buffer, &scale, sizeof(int32_t));

    char second[] = ", \"value\": ";
    strcat(buffer, second);

    char value[10];
    sprintf(value, "%d", data->value);
    strncat(buffer, &value, sizeof(int32_t));

    strcat(buffer, "}");
}

void write_atmosphericPressure_to_buffer(char * buffer, environmentSensors_SingleDataPoint *data) {

    char first[] = ", \"atmosphericPressure\":{ \"scale\": ";
    strcat(buffer, first);

    char scale[10];
    sprintf(scale, "%d", data->scale);
    strncat(buffer, &scale, sizeof(int32_t));

    char second[] = ", \"value\": ";
    strcat(buffer, second);

    char value[10];
    sprintf(value, "%d", data->value);
    strncat(buffer, &value, sizeof(int32_t));

    strcat(buffer, "}");

}

void write_pm2_5_to_buffer(char * buffer, environmentSensors_SingleDataPoint *data) {

    char first[] = ", \"pm2_5\":{ \"scale\": ";
    strcat(buffer, first);

    char scale[10];
    sprintf(scale, "%d", data->scale);
    strncat(buffer, &scale, sizeof(int32_t));

    char second[] = ", \"value\": ";
    strcat(buffer, second);

    char value[10];
    sprintf(value, "%d", data->value);
    strncat(buffer, &value, sizeof(int32_t));

    strcat(buffer, "}");

}

void write_data_response_to_buffer(char * buffer, environmentSensors_DataResponse *data_response) {

    if(data_response->has_temperature){
        char first[] = "{\"hasTemperature\": \"true\" ";
        strcpy(buffer, first);

        write_temperature_to_buffer(buffer, &data_response->temperature);
    }else{
        char first[] = "{\"hasTemperature\": \"false\" ";
        strcpy(json_buffer, first);
        strcat(buffer, first);
    }

    if(data_response->has_humidity){
        char second[] = ", \"hasHumidity\": \"true\" ";
        strcat(buffer, second);

        write_humidity_to_buffer(buffer, &data_response->temperature);
    }else{
        char second[] = ", \"hasHumidity\": \"false\" ";
        strcat(buffer, second);
    }

    if(data_response->has_atmosphericPressure){
        char third[] = ", \"hasAtmosphericPressure\": \"true\" ";
        strcat(buffer, third);

        write_atmosphericPressure_to_buffer(buffer, &data_response->temperature);
    }else{
        char third[] = ", \"hasAtmosphericPressure\": \"false\" ";
        strcat(buffer, third);
    }

    if(data_response->has_pm2_5){
        char fourth[] = ", \"hasPm2_5\": \"true\" ";
        strcat(buffer, fourth);

        write_pm2_5_to_buffer(buffer, &data_response->temperature);
    }else{
        char fourth[] = ", \"hasPm2_5\": \"false\" ";
        strcat(buffer, fourth);
    }

    strcat(buffer, "}");

}

void send_to_tangle(environmentSensors_DataResponse *data_response) {
    clear_json_buffer();

    write_data_response_to_buffer(json_buffer, data_response);

    int json_size = strlen(json_buffer);
    log_int("DEBUG", "send_to_tangle", "json_buffer_size", json_size);

    log_string("DEBUG", "send_to_tangle", "json_buffer", json_buffer);
    json_buffer[json_size] = '\0';

    mam_send_message(IOTA_HOST, IOTA_PORT, iota_seed, json_buffer, json_size, true);
}

void handle_data_response(struct sockaddr_in6 *server_addr_ptr, uint8_t *socket_buffer_ptr, int buffer_length) {
    environmentSensors_DataResponse data_response;

    env_sensor_data_response_decode(&data_response, socket_buffer_ptr, buffer_length);

    for(int i = 0; i < SENSOR_NODES_LENGTH; i++) {
        if(check_ip_address(sensor_nodes[i].config.address.sin6_addr.s6_addr, server_addr_ptr->sin6_addr.s6_addr)) {
            if(DEBUG_SERVER){
                log_float("DEBUG", "handle_data_response", "temperature", get_scaled_value(&data_response.temperature));
            }

            env_sensor_data_t sensor_data = {
                    .humidity = get_scaled_value(&data_response.humidity),
                    .temperature = get_scaled_value(&data_response.temperature),
                    .pm2_5 = get_scaled_value(&data_response.pm2_5),
                    .atmosphericPressure = get_scaled_value(&data_response.atmosphericPressure),
                    };
            send_to_tangle(&data_response);
            add_to_data_ring(&sensor_nodes[i].data_ring, &sensor_data);
        }
    }
}

void handle_feature_response(struct sockaddr_in6 *server_addr_ptr, uint8_t *socket_buffer_ptr, int buffer_length) {
    environmentSensors_FeatureResponse feature_response;

    env_sensor_feature_response_decode(&feature_response, socket_buffer_ptr, buffer_length);

    for(int i = 0; i < SENSOR_NODES_LENGTH; i++) {
        if(check_ip_address(sensor_nodes[i].config.address.sin6_addr.s6_addr, server_addr_ptr->sin6_addr.s6_addr)) {
            sensor_nodes[i].features.hasAtmosphericPressure = feature_response.hasAtmosphericPressure;
            sensor_nodes[i].features.hasHumidity = feature_response.hasHumidity;
            sensor_nodes[i].features.hasTemperature = feature_response.hasTemperature;
            sensor_nodes[i].features.has2_5 = feature_response.hasPm2_5;
        }
    }
}

void handle_incoming_message(
        int sock, struct sockaddr_in6 *server_addr_ptr,
        uint8_t *socket_buffer_ptr, int socket_buffer_length) {
    char func_name[] = "handle_incoming_message";

    sensor_command_t command = get_rpc_sensor_command(socket_buffer_ptr[0]);

    if (DEBUG_SERVER) {
        log_hex("DEBUG", func_name, "hex_command", (uint8_t) socket_buffer_ptr[0]);
        char command_name[20];
        memset(command_name, 0, 20);
        get_env_sensor_rpc_command_name(command_name, command);
        log_string("DEBUG", func_name, "command_name", command_name);
        log_int("DEBUG", func_name, "server_port", ntohs(server_addr_ptr->sin6_port));
        log_addr("DEBUG", func_name, "server_addr", server_addr_ptr);
    }

    switch(command) {
        case FEATURE_RESPONSE_CMD:
            handle_feature_response(server_addr_ptr, &socket_buffer_ptr[1], socket_buffer_length - 1);
            break;
        case DATA_RESPONSE_CMD:
            handle_data_response(server_addr_ptr, &socket_buffer_ptr[1], socket_buffer_length - 1);
            break;
        default:
            break;
    }

}

void start_listening(void) {
    unsigned int client_addr_len = sizeof(struct sockaddr_in6);
    int socket_buffer_length;

    uint8_t socket_buffer[SOCKET_BUFFER_SIZE];

    while (client_is_running) {
        int length = recvfrom(sock, socket_buffer, SOCKET_BUFFER_SIZE - 1, 0,
                              (struct sockaddr *) &client_addr,
                              &client_addr_len);

        if (length < 0) {
            log_int("ERROR", "server_start_listening", "recvfrom", length);
            break;
        }
        socket_buffer[length] = '\0';
        socket_buffer_length = length;

        if (client_is_running) {
            handle_incoming_message(sock, &client_addr, socket_buffer, socket_buffer_length);
        } else {
            clear_socket_buffer(socket_buffer);
        }
    }
}

void *run_receiver_thread(void *args) {
    (void) args;

    start_listening();
    int value = 0;
    close(sock);
    pthread_exit(&value);
}

void start_sending(void) {
    uint8_t command = get_rpc_command_byte(FEATURE_REQUEST_CMD);
    for(int i = 0; i < SENSOR_NODES_LENGTH; i++) {
        struct sockaddr_in6 * address = &sensor_nodes[i].config.address;
        send_buffer(address, &command, 1);
    }

    command = get_rpc_command_byte(DATA_REQUEST_CMD);
    while(client_is_running){
        sleep(5);
        for(int i = 0; i < SENSOR_NODES_LENGTH; i++) {
            struct sockaddr_in6 *address = &sensor_nodes[i].config.address;
            send_buffer(address, &command, 1);
        }
    }
}

void *run_send_thread(void *args) {
    (void) args;

    start_sending();
    int value = 0;
    close(sock);
    pthread_exit(&value);
}

void node_status_report(sensor_node_t *node){
    char func_name[] = "node_status_report";
    char level[] = "DEBUG";
    if(DEBUG_SERVER){
        log_addr(level, func_name, "sensor_address", &node->config.address);
    }

    for(int i = 0; i < DATA_SIZE;i++){
        log_sensor_data(level, func_name, "sensor_data", &node->data_ring.data[i]);
    }
}

void start_status_report(void) {
    while(client_is_running){
        sleep(10);
        for(int i = 0; i < SENSOR_NODES_LENGTH; i++) {
            node_status_report(&sensor_nodes[i]);
        }
    }
}

void *run_status_thread(void *args) {
    (void) args;

    start_status_report();
    int value = 0;
    close(sock);
    pthread_exit(&value);
}

pthread_t status_thread;
pthread_t send_thread;
pthread_t listing_thread;
int main(int argc, char *argv[]){
    //iota-seed client-address sensor-address sensor-port interface-name

    memcpy(iota_seed, argv[1], 81);

    init_client(argc, argv);
    if (listing_thread > 0) {
        puts("Server is already running.");
    } else {
        listing_thread = pthread_create(&listing_thread, NULL, &run_receiver_thread, NULL);
        send_thread = pthread_create(&send_thread, NULL, &run_send_thread, NULL);
        status_thread = pthread_create(&status_thread, NULL, &run_status_thread, NULL);
    }
    while(client_is_running){}
    return 0;
}