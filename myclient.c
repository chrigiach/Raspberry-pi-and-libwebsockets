/*
 * A project for Real Time and Embedded Systems, an ECE AUTH course of 2022.
 * A client that connects to Finnhub web sockets, receives incoming data about trading, parses them and stores them into an output txt file.
 * A project that runs on a raspberry pi zero 2w, with the raspberryOS software.
 * For compiling and running it, a makefile is used.
 * More info can be found on the readme file and the report.
*/


// The libraries that will be used
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "libwebsockets.h"


// Global parameters
int end_program_signal = 0; // The variable that signals the termination of the program (0: no termination, 1: termination)
int connection_status = 0; // The variable that signals the connection status (0: no connection, 1: connection)
int communication_status = 0; // The variable that states if the client has to send a message to the server (0: ready to send message, 1: not ready to send message)
char p_buffer[25]; // temporary buffer
char s_buffer[25]; // temporary buffer
char t_buffer[25]; // temporary buffer
char v_buffer[25]; // temporary buffer
char str1[] = "AMZN"; // comparison string
char str2[] = "APPL"; // comparison string
char str3[] = "BINANCE:BTCUSDT"; // comparison string
char str4[] = "IC MARKETS:1"; // comparison string
int down_counter = 4; // global counter for cb function
int cs_signal = 1; // 1: keep running, 0: loop termination
// mutexes for the pthreads
pthread_mutex_t *amzn;
pthread_mutex_t *appl;
pthread_mutex_t *binance;
pthread_mutex_t *icm;
// Temporary storage variables
float apple_first_value;
float apple_last_value;
float apple_min_value;
float apple_max_value;
int apple_volume;
unsigned long long apple_time;

float amazon_first_value;
float amazon_last_value;
float amazon_min_value;
float amazon_max_value;
int amazon_volume;
unsigned long long amazon_time;

float bitcoin_first_value;
float bitcoin_last_value;
float bitcoin_min_value;
float bitcoin_max_value;
int bitcoin_volume;
unsigned long long bitcoin_time;

float icmarket_first_value;
float icmarket_last_value;
float icmarket_min_value;
float icmarket_max_value;
float icmarket_volume;
unsigned long long icmarket_time;

// First time info arrival checker variables
int amzn_first = 0; // 0: no data for stock has arrived, 1: client received at least 1 set of data for the stock
int appl_first = 0; // 0: no data for stock has arrived, 1: client received at least 1 set of data for the stock
int binance_first = 0; // 0: no data for stock has arrived, 1: client received at least 1 set of data for the stock
int icm_first = 0; // 0: no data for stock has arrived, 1: client received at least 1 set of data for the stock

// CPU usage parameters
clock_t start_waiting;
clock_t end_waiting;
double cpu_time_not_used = 0; // seconds

// End program signal changer
static void INT_HANDLER(int sig){
    end_program_signal = 1;
}


// The paths JSON follows, that contain usefull information for us
// Change the strings here, if you need other info
// A list of other usefull info can be found here: https://finnhub.io/docs/api/websocket-trades
// s: stands for symbol
// p: stands for last price
// t: stands for UNIX milliseconds timestamp
// v: stands for Volume
static const char * const info_paths[] = {

    "data[].s",
    "data[].p",
    "data[].t",
    "data[].v",

};


// The output place pointer of the parsed data
FILE *amzn_output_file;
FILE *appl_output_file;
FILE *binance_output_file;
FILE *ICMarket_output_file;

// The output for the candlesticks
FILE *amzn_candlestick;
FILE *appl_candlestick;
FILE *binance_candlestick;
FILE *ICMarket_candlestick;

// The output for time delays
FILE *lws_times;
FILE *storing_times;

// Callback function that handles parsing events from LEJP
// Writing into the output file
// lejp_ctx: the structure with the stored info
// reason: the reason for calling this function (parsing or end of parsing)
static signed char cb(struct lejp_ctx *ctx, char reason){
	
	// Let's write to the output files (1st task of the project)
	if (reason & LEJP_FLAG_CB_IS_VALUE) {
		if(ctx->path_match > 0){
			// The values client receives are 4 each time. So we store each value to a temporary buffer. When we store the last of the 4 values, we store all 4 of them into the corresponding txt file. We also do some analyzing for the second task(candlestick).
			switch(down_counter){
				case 4: // store the last price temporarily
					sprintf(p_buffer, ctx->buf);
					down_counter = down_counter - 1;
					break;
				case 3: // store the symbol temporarily
					sprintf(s_buffer, ctx->buf);
					down_counter = down_counter - 1;
					break;
				case 2: // store the timastamp temporarily
					sprintf(t_buffer, ctx->buf);
					down_counter = down_counter - 1;
					break;
				case 1: // store the volume temporarily
					sprintf(v_buffer, ctx->buf);
					down_counter = 4;
					// store all 4 values into the corresponding file
					if(strcmp(str1, s_buffer) == 0){
						// Save the output
						fprintf(amzn_output_file, "Symbol: %s\n", s_buffer);
						fprintf(amzn_output_file, "Last price: %s\n", p_buffer);
						fprintf(amzn_output_file, "Volume: %s\n", v_buffer);
						fprintf(amzn_output_file, "Timestamp: %s\n", t_buffer);
						fprintf(lws_times, "%s\n", t_buffer);
						// Find the current UNIX timestamp
						struct timeval tv;
						gettimeofday(&tv, NULL);
						unsigned long long millisecondsSinceEpoch =
						(unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
						fprintf(amzn_output_file, "Current timestamp: %llu\n\n\n ___New entry___\n", millisecondsSinceEpoch);
						fprintf(storing_times, "%llu\n", millisecondsSinceEpoch);
						pthread_mutex_lock (amzn);
						if(amzn_first == 0){
							amzn_first = 1;
							float temp_p = atof(p_buffer);
							float temp_v = atof(v_buffer);
							amazon_first_value = temp_p;
							amazon_max_value = temp_p;
							amazon_min_value = temp_p;
							amazon_last_value = temp_p;
							amazon_volume = temp_v;
							amazon_time = millisecondsSinceEpoch;
						} else {
							float temp_p = atof(p_buffer);
							float temp_v = atof(v_buffer);
							if(temp_p > amazon_max_value){
								amazon_max_value = temp_p;
							}
							if(temp_p < amazon_min_value){
								amazon_min_value = temp_p;
							}
							amazon_last_value = temp_p;
							amazon_volume = temp_v;
							amazon_time = millisecondsSinceEpoch;
						}
						pthread_mutex_unlock (amzn);
					} else if(strcmp(str2, s_buffer) == 0){
						// Save the output
						fprintf(appl_output_file, "Symbol: %s\n", s_buffer);
						fprintf(appl_output_file, "Last price: %s\n", p_buffer);
						fprintf(appl_output_file, "Volume: %s\n", v_buffer);
						fprintf(appl_output_file, "Timestamp: %s\n", t_buffer);
						fprintf(lws_times, "%s\n", t_buffer);
						// Find the current UNIX timestamp
						struct timeval tv;
						gettimeofday(&tv, NULL);
						unsigned long long millisecondsSinceEpoch =
						(unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
						fprintf(appl_output_file, "Current timestamp: %llu\n\n\n ___New entry___\n", millisecondsSinceEpoch);
						fprintf(storing_times, "%llu\n", millisecondsSinceEpoch);
						pthread_mutex_lock (appl);
						if(appl_first == 0){
							appl_first = 1;
							float temp_p = atof(p_buffer);
							float temp_v = atof(v_buffer);
							apple_first_value = temp_p;
							apple_max_value = temp_p;
							apple_min_value = temp_p;
							apple_last_value = temp_p;
							apple_volume = temp_v;
							apple_time = millisecondsSinceEpoch;
						} else {
							float temp_p = atof(p_buffer);
							float temp_v = atof(v_buffer);
							if(temp_p > apple_max_value){
								apple_max_value = temp_p;
							}
							if(temp_p < apple_min_value){
								apple_min_value = temp_p;
							}
							apple_last_value = temp_p;
							apple_volume = temp_v;
							apple_time = millisecondsSinceEpoch;
						}
						pthread_mutex_unlock (appl);
					} else if(strcmp(str3, s_buffer) == 0){
						// Save the output
						fprintf(binance_output_file, "Symbol: %s\n", s_buffer);
						fprintf(binance_output_file, "Last price: %s\n", p_buffer);
						fprintf(binance_output_file, "Volume: %s\n", v_buffer);
						fprintf(binance_output_file, "Timestamp: %s\n", t_buffer);
						fprintf(lws_times, "%s\n", t_buffer);
						// Find the current UNIX timestamp
						struct timeval tv;
						gettimeofday(&tv, NULL);
						unsigned long long millisecondsSinceEpoch =
						(unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
						fprintf(binance_output_file, "Current timestamp: %llu\n\n\n ___New entry___\n", millisecondsSinceEpoch);
						fprintf(storing_times, "%llu\n", millisecondsSinceEpoch);
						pthread_mutex_lock (binance);
						if(binance_first == 0){
							binance_first = 1;
							float temp_p = atof(p_buffer);
							float temp_v = atof(v_buffer);
							bitcoin_first_value = temp_p;
							bitcoin_max_value = temp_p;
							bitcoin_min_value = temp_p;
							bitcoin_last_value = temp_p;
							bitcoin_volume = temp_v;
							bitcoin_time = millisecondsSinceEpoch;
						} else {
							float temp_p = atof(p_buffer);
							float temp_v = atof(v_buffer);
							if(temp_p > bitcoin_max_value){
								bitcoin_max_value = temp_p;
							}
							if(temp_p < bitcoin_min_value){
								bitcoin_min_value = temp_p;
							}
							bitcoin_last_value = temp_p;
							bitcoin_volume = temp_v;
							bitcoin_time = millisecondsSinceEpoch;
						}
						pthread_mutex_unlock (binance);
					} else if(strcmp(str4, s_buffer) == 0){
						// Save the output
						fprintf(ICMarket_output_file, "Symbol: %s\n", s_buffer);
						fprintf(ICMarket_output_file, "Last price: %s\n", p_buffer);
						fprintf(ICMarket_output_file, "Volume: %s\n", v_buffer);
						fprintf(ICMarket_output_file, "Timestamp: %s\n", t_buffer);
						fprintf(lws_times, "%s\n", t_buffer);
						// Find the current UNIX timestamp
						struct timeval tv;
						gettimeofday(&tv, NULL);
						unsigned long long millisecondsSinceEpoch =
						(unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
						fprintf(ICMarket_output_file, "Current timestamp: %llu\n\n\n ___New entry___\n", millisecondsSinceEpoch);
						fprintf(storing_times, "%llu\n", millisecondsSinceEpoch);
						pthread_mutex_lock (icm);
						if(icm_first == 0){
							icm_first = 1;
							float temp_p = atof(p_buffer);
							float temp_v = atof(v_buffer);
							icmarket_first_value = temp_p;
							icmarket_max_value = temp_p;
							icmarket_min_value = temp_p;
							icmarket_last_value = temp_p;
							icmarket_volume = temp_v;
							icmarket_time = millisecondsSinceEpoch;
						} else {
							float temp_p = atof(p_buffer);
							float temp_v = atof(v_buffer);
							if(temp_p > icmarket_max_value){
								icmarket_max_value = temp_p;
							}
							if(temp_p < icmarket_min_value){
								icmarket_min_value = temp_p;
							}
							icmarket_last_value = temp_p;
							icmarket_volume = temp_v;
							icmarket_time = millisecondsSinceEpoch;
						}
						pthread_mutex_unlock (icm);
					}
					break;
			}
		}
		return 0;
	}
	
	// If the parsing procedure finished
	if (reason == LEJPCB_COMPLETE){
		printf("Message received and stored!!!\n\n");
	}
	
    return 0;
}



// ***The basic websocket function***
// The function that manages the actions of the program, sends messages to the server, parses the received data and signals the status of the program
// wsi: the web socket struct that will be sent to the server
// reason: the case the function is called to handle
// user: ...
// incoming_data: The data we have received from the server
// len: size of the data
static int ws_service_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *incoming_data, size_t len){

    switch(reason){
		
		// When the connection is established, send message to the server
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("***Successfull connection!!!***\n");
            connection_status = 1; // connected
			lws_callback_on_writable(wsi); // Request a callback when this socket becomes able to be written to without blocking
            break;
		
		// Problem!!! Unsuccessfull connection
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("***Unsuccessful connection!!! :( ***.\n");
            connection_status = 0; // disconnected
            break;
		
		// Problem!!! Connection lost
        case LWS_CALLBACK_CLOSED:
            printf("***Connection was lost. Trying to reconnect...***\n");
            connection_status = 0; // disconnected
            break;
		
		// Subscribe to the trade symbols you want to monitor and ask for trade data from the server
        case LWS_CALLBACK_CLIENT_WRITEABLE :
            printf("***Server ready to receive message***.\n");
			// if server is ready to receive a message from the client
			if(communication_status == 0){
				char symb_arr[4][50] = {"APPL\0", "AMZN\0", "BINANCE:BTCUSDT\0", "IC MARKETS:1\0"};
				char str[100];
				for(int i = 0; i < 4; i++){
					sprintf(str, "{\"type\":\"subscribe\",\"symbol\":\"%s\"}", symb_arr[i]);
					int len = strlen(str);
					if (str == NULL || wsi == NULL)
						return -1;
					char *out = NULL; // the buffer
					out = (char *)malloc(sizeof(char) * (LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
					memcpy(out + LWS_SEND_BUFFER_PRE_PADDING, str, len);
					int n = lws_write(wsi, out + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT); // send the message to the server
					start_waiting = clock();
					free(out); // free the buffer
				}
			}
			communication_status = 1; // declare that the server is not ready now for message receives
			break;
		
		// Receiving and parsing a message from the server
        case LWS_CALLBACK_CLIENT_RECEIVE:
			end_waiting = clock();
			cpu_time_not_used = cpu_time_not_used + ((double) (end_waiting - start_waiting)) / CLOCKS_PER_SEC;
            printf("<<<Client received message. Parsing procedure is on!!!>>>\n");
			//*****parsing the data*****
			//Input of lejp parser constructor initialization
			struct lejp_ctx ctx;
			char *data;
			data = (char *)incoming_data;
			int length = strlen(data);
			//parser constructor
			lejp_construct(&ctx, cb, NULL, info_paths, LWS_ARRAY_SIZE(info_paths));
			// let's parse the data
			int m = lejp_parse(&ctx, (uint8_t *)data, length);
			if (m < 0 && m != LEJP_CONTINUE) {
				lwsl_err("Parse failed!!!!:( %d\n", m);
			}
			printf("<Parsing is over>\n");
			//*****End of parsing*****
            break;
		
		// Problem!!! Connection lost.
		case LWS_CALLBACK_CLIENT_CLOSED:
			connection_status = 0; // disconnected
			communication_status = 0; // server ready for the client's message
			
		default:
			break;
    }

    return 0;
}

// The protocol websocket client needs to know, in order to follow it
// (Just use the ws_service_callback function)
static struct lws_protocols protocols[] = {
	{	"simple-client-protocol",
		ws_service_callback, },
	{ NULL, NULL, 0, 0 }
};

//Candlestick pthread procedure
void *candlestick(void *param){
	printf("*********Hi from the candlestick thread**************!!!!");
	sleep(30); // Sleep for half a minute so that there are enough data to analyze
	int delay = *((int*)param);
	int size = 0;

	// loop every 1 minute(or how much the delay is set to)
	while(cs_signal == 1){
		printf("New loop. Let's compute the candlestick.");
		// check if the general files are emptry
		if(amzn_first == 1){
			pthread_mutex_lock (amzn);
			// store candlestick
			fprintf(amzn_candlestick, "New entry\n");
			fprintf(amzn_candlestick, "First price: %f\n", amazon_first_value);
			fprintf(amzn_candlestick, "Last price: %f\n", amazon_last_value);
			fprintf(amzn_candlestick, "Max price: %f\n", amazon_max_value);
			fprintf(amzn_candlestick, "Min price: %f\n", amazon_min_value);
			fprintf(amzn_candlestick, "Total volume: %f\n\n\n", amazon_volume);
			pthread_mutex_unlock (amzn);
		}
		
		if(appl_first == 1){
			pthread_mutex_lock (appl);
			// store candlestick
			fprintf(appl_candlestick, "New entry\n");
			fprintf(appl_candlestick, "First price: %f\n", apple_first_value);
			fprintf(appl_candlestick, "Last price: %f\n", apple_last_value);
			fprintf(appl_candlestick, "Max price: %f\n", apple_max_value);
			fprintf(appl_candlestick, "Min price: %f\n", apple_min_value);
			fprintf(appl_candlestick, "Total volume: %f\n\n\n", apple_volume);
			pthread_mutex_unlock (appl);
		}
		
		if(binance_first == 1){
			pthread_mutex_lock (binance);
			// store candlestick
			fprintf(binance_candlestick, "New entry\n");
			fprintf(binance_candlestick, "First price: %f\n", bitcoin_first_value);
			fprintf(binance_candlestick, "Last price: %f\n", bitcoin_last_value);
			fprintf(binance_candlestick, "Max price: %f\n", bitcoin_max_value);
			fprintf(binance_candlestick, "Min price: %f\n", bitcoin_min_value);
			fprintf(binance_candlestick, "Total volume: %f\n\n\n", bitcoin_volume);
			pthread_mutex_unlock (binance);
		}
		
		if(icm_first == 1){
			pthread_mutex_lock (icm);
			// store candlestick
			fprintf(ICMarket_candlestick, "New entry\n");
			fprintf(ICMarket_candlestick, "First price: %f\n", icmarket_first_value);
			fprintf(ICMarket_candlestick, "Last price: %f\n", icmarket_last_value);
			fprintf(ICMarket_candlestick, "Max price: %f\n", icmarket_max_value);
			fprintf(ICMarket_candlestick, "Min price: %f\n", icmarket_min_value);
			fprintf(ICMarket_candlestick, "Total volume: %f\n\n\n", icmarket_volume);
			pthread_mutex_unlock (icm);
		}
		printf("Time to sleep.");
		sleep(delay); // wait for 1 minute and store the new candlestick
	}
}


// The main function...where all fun exists...
int main(void) {
	
	// Time staff
	time_t start = time(NULL);
	
    // First open the files, to where you will export the data
    appl_output_file = fopen("Apple_general.txt", "w+");
	
    amzn_output_file = fopen("Amazon_general.txt", "w+");
	
    binance_output_file = fopen("Binance_general.txt", "w+");
	
    ICMarket_output_file = fopen("ICMarket_general.txt", "w+");
	
	appl_candlestick = fopen("Apple_candlestick.txt", "w");
	
    amzn_candlestick = fopen("Amazon_candlestick.txt", "w");
	
    binance_candlestick = fopen("Binance_candlestick.txt", "w");
	
    ICMarket_candlestick = fopen("ICMarket_candlestick.txt", "w");
	
	lws_times = fopen("lws_times.txt", "w");
	
	storing_times = fopen("storing_times.txt", "w");
	
    // register the signal SIGINT handler
    signal(SIGINT, INT_HANDLER);

    // Prepare the socket
    struct lws_context *context = NULL;
    struct lws_context_creation_info info;
    struct lws *wsi = NULL;
    memset(&info, 0, sizeof info);
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	
	//Pthread initialization
	amzn = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init (amzn, NULL);
	appl = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init (appl, NULL);
	binance = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init (binance, NULL);
	icm = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init (icm, NULL);
	pthread_t analyzer; // analyzer: second task analyzer (for candlestick)
	
    // The Finnhub url for the connection and requests
    // Log up to Finnhub.io and get an API key for the connection
    // Put the key inside the quotes. "...?token=...your key must be placed here..."
    char *api_key = "cbpog8aad3ieg7fatna0"; // PLACE YOUR API KEY HERE!
    char *inputURL[300];
    sprintf(inputURL, "wss://ws.finnhub.io/?token=%s", api_key);
    char urlPath[300];
    const char *urlProtocol, *urlTempPath;
    
    // create the socket context
    context = lws_create_context(&info);
    printf("Socket is created\n");
	if(!context){
        printf("Socket problem. Never actually created.\n");
        return 1;
    }

	// clientConnectionInfo: parameters to connect with when using lws_client_connect_via_info()
    struct lws_client_connect_info clientConnectionInfo;
    memset(&clientConnectionInfo, 0, sizeof(clientConnectionInfo));
    clientConnectionInfo.context = context;

	// Parse url**************************************
    if (lws_parse_uri(inputURL, &urlProtocol, &clientConnectionInfo.address, &clientConnectionInfo.port, &urlTempPath))
    {
        printf("Couldn't parse URL\n");
    }

    urlPath[0] = '/';
    strncpy(urlPath + 1, urlTempPath, sizeof(urlPath) - 2);
    urlPath[sizeof(urlPath) - 1] = '\0';
    clientConnectionInfo.port = 443;
    clientConnectionInfo.path = urlPath;
    clientConnectionInfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    clientConnectionInfo.host = clientConnectionInfo.address;
    clientConnectionInfo.origin = clientConnectionInfo.address;
    clientConnectionInfo.ietf_version_or_minus_one = -1;
    clientConnectionInfo.protocol = protocols[0].name;
    printf(">>>Testing %s\n\n", clientConnectionInfo.address);
    printf(">>>Connecting to %s://%s:%d%s \n\n", urlProtocol, 
    clientConnectionInfo.address, clientConnectionInfo.port, urlPath);

	// make the connection
    wsi = lws_client_connect_via_info(&clientConnectionInfo);
    if (wsi == NULL) {
        printf("wsi not created. Error!!!\n");
        return -1;
    }
    printf("wsi created successfully.\n");
	
	// pthread startpoint
	int *arg = malloc(sizeof(*arg));
	*arg = 60; // 60 seconds delay
	pthread_create(&analyzer, NULL, candlestick, arg);
	
	// Keep serving until an outsider terminates the procedure (Ctrl + C)
    while(!end_program_signal){		
		// Service any pending websocket activity before termination
        lws_service(context, 0);
    }
	printf("Ending process initiated. Have a nice day. Terminating...\n");
	cs_signal = 0; // tell the pthread loop to stop iterations
	clock_t before = clock();
	pthread_join(analyzer, NULL);
	clock_t after = clock();
	cpu_time_not_used = cpu_time_not_used + ((double) (after - before)) / CLOCKS_PER_SEC;
	// destroy the websocket context: This function closes any active connections and then frees the context.
    lws_context_destroy(context);
	// Close all the opened files that where used for exportin purposes
	fclose(appl_output_file);
	fclose(amzn_output_file);
	fclose(binance_output_file);
	fclose(ICMarket_output_file);
	fclose(appl_candlestick);
	fclose(amzn_candlestick);
	fclose(binance_candlestick);
	fclose(ICMarket_candlestick);
	fclose(lws_times);
	fclose(storing_times);
	pthread_mutex_destroy (amzn);
	pthread_mutex_destroy (appl);
	pthread_mutex_destroy (binance);
	pthread_mutex_destroy (icm);
	cpu_time_not_used = cpu_time_not_used / CLOCKS_PER_SEC;
	time_t end = time(NULL);
	time_t overall_time = end - start;
	double time_ratio = ((double) cpu_time_not_used / (double)overall_time);
	double time_perc = time_ratio * 100;
	printf("The CPU was not in use for %f seconds.\n", cpu_time_not_used);
	printf("The overall time of the program until termination was %ld seconds.\n", overall_time);
	printf("The inactivity percentage for the cpu is about %f %% of the overall time.\n", time_perc);
    return 0;
}
