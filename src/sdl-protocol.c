#include <libwebsockets.h>
#include <pthread.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdbool.h>

#include "parson.h"

#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

// Can受信.
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <stdint.h>
#include <unistd.h>

#include <signal.h>
#include <libgen.h>
#include <wayland-util.h>

#define MAX_DATA_SIZE  8192/* 一度にwebsocketに送信できる最大サイズ */
#define STACK_SIZE 10  /* スタックデータ数の最大値 */

#define DEBUG_LOG_ENABLE (1)

struct lws_context* g_lws_context;
pthread_t g_pthread_t;

void lws_touch_handle_down(uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w);
void lws_touch_handle_up(uint32_t time, int32_t id);
void lws_touch_handle_motion(uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w);

// 前方宣言(Websocket)
static int getRPCType(char*);
static char* getMethodStr(char*);
static void persistence(char*);

static void receive_rpc_vr(struct lws*, unsigned int, char*, int);
static void receive_rpc_tts(struct lws*, unsigned int, char*, int);
static void receive_rpc_ui(char* string, struct lws*, unsigned int, char*, int);
static void receive_rpc_navigation(struct lws*, unsigned int, char*, int);
static void receive_rpc_vehicleInfo(struct lws*, unsigned int, char*, int);
static void receive_rpc_rc(struct lws*, unsigned int, char*, int);
static void receive_rpc_buttons(struct lws*, unsigned int, char*, int);
static void receive_rpc_basiccommunication(struct lws*, unsigned int, char*, int, char*);
static void receive_rpc_sdl(struct lws*, unsigned int, char*, int, char*);
static void receive_persistence(char*, struct lws*);
static void send_registerComponent(struct lws*, unsigned int, unsigned int);
static void send_registerComponent_VR(struct lws*);
static void send_registerComponent_Navigation(struct lws*);
static void send_registerComponent_TTS(struct lws*);
static void send_registerComponent_UI(struct lws*);
static void send_registerComponent_Buttons(struct lws*);
static void send_registerComponent_VehicleInfo(struct lws*);
static void send_registerComponent_RC(struct lws*);
static void send_registerComponent_BasicCommunication(struct lws*);

static void send_subscribeTo_Navigation_OnAudioDataStreaming(struct lws*);
static void send_subscribeTo_Navigation_OnVideoDataStreaming(struct lws*);
static void send_subscribeTo_UI_OnRecordStart(struct lws*);
static void send_subscribeTo_Buttons_OnButtonSubscription(struct lws*);
static void send_subscribeTo_BasicCommunication_OnPutFile(struct lws*);
static void send_subscribeTo_BasicCommunication_OnSDLPersistenceComplete(struct lws*);
static void send_subscribeTo_BasicCommunication_OnFileRemoved(struct lws*);
static void send_subscribeTo_BasicCommunication_OnAppRegistered(struct lws*);
static void send_subscribeTo_BasicCommunication_OnAppUnregistered(struct lws*);
static void send_subscribeTo_BasicCommunication_OnSDLClose(struct lws*);
static void send_subscribeTo_BasicCommunication_OnResumeAudioSource(struct lws*);
static void send_subscribeTo_SDL_OnSDLConsentNeeded(struct lws*);
static void send_subscribeTo_SDL_OnStatusUpdate(struct lws*);
static void send_subscribeTo_SDL_OnAppPermissionChanged(struct lws*);
static void send_subscribeTo_Navigation(struct lws*, unsigned int, char*);
static void send_subscribeTo_UI(struct lws*, unsigned int, char*);
static void send_subscribeTo_Buttons(struct lws*, unsigned int, char*);
static void send_subscribeTo_BasicCommunication(struct lws*, unsigned int, char*);

static void send_BasicCommunication_GetSystemInfo(struct lws* wsi, unsigned int id);
static void send_BasicCommunication_OnReady(struct lws*);
static void send_rpc_vr_GetCapabilities(struct lws* , unsigned int, int);
static void send_rpc_ui_GetCapabilities(struct lws* , unsigned int, int);
static void send_rpc_tts_GetCapabilities(struct lws* , unsigned int, int);
static void send_rpc_rc_GetCapabilities(struct lws* , unsigned int, int);
static void send_rpc_buttons_GetCapabilities(struct lws* , unsigned int, int);
static void do_lws_write(struct lws*, char* , JSON_Value*);

static void send_rpc_isReady(struct lws*, unsigned int, int);
static void send_rpc_GetLanguage(struct lws* , unsigned int, int );
static void send_rpc_GetSupportedLanguages(struct lws* , unsigned int, int );
static void send_BasicCommunication_MixingAudioSupported(struct lws*, unsigned int);
static void renketu_write(struct lws*, JSON_Value *, JSON_Object *, char* );

static void send_rpc_VehicleInfo_GetVehicleData(wsi, id, rpctype);
static void send_rpc_VehicleInfo_GetVehicleType(wsi, id, rpctype);
static void send_rpc_rc_OnRemoteControlSettings(struct lws*, unsigned int, int );

static void send_rpc_VehicleInfo_SubscribeVehicleData(struct lws*, unsigned int, int);
static void send_rpc_VehicleInfo_UnsubscribeVehicleData(struct lws*, unsigned int, int);
static void do_lws_write_can(struct lws*, char*);
static void parse_VehicleData_string(const char *);

static void send_BasicCommunication_OnFindApplications(struct lws*);
// アプリ起動関連
static void send_rpc_ChangeRegistration(struct lws*, unsigned int, int);
static void send_rpc_AddCommand(struct lws*, unsigned int, int );
static void send_BasicCommunication_UpdateDeviceList(struct lws*, unsigned int, char*);
static void do_send_BasicCommunication_UpdateDeviceList(struct lws*, unsigned int);
static void send_rpc_tts_SetGlobalProperties(struct lws*, unsigned int);
static void send_rpc_ui_SetAppIcon(struct lws*, unsigned int);
static void saveparam_BasicCommunication_OnAppRegistered(struct lws*, unsigned int, char*);
static void releaseparam_BasicCommunication_OnAppUnregistered(struct lws*, unsigned int, char*);
static void send_BasicCommunication_UpdateAppList(struct lws*, unsigned int, char*);
static void send_BasicCommunication_OnAppDeactivated(struct lws*);
static void send_BasicCommunication_PolicyUpdate(struct lws*, unsigned int);
static void send_BasicCommunication_ActivateApp(struct lws*, unsigned int);
static void send_sdl_OnStatusUpdate(struct lws*, unsigned int, char*);
static void send_sdl_GetUserFriendlyMessage(struct lws*);
static void send_sdl_GetURLS(struct lws*);
static void send_sdl_ActivateApp(struct lws*);
static void send_BasicCommunication_OnSystemRequest(struct lws*, unsigned int);
static void send_navigation_SetVideoConfig(struct lws*, unsigned int);
static void send_navigation_StartStream(struct lws*, unsigned int);
static void send_navigation_StartAudioStream(struct lws*, unsigned int);
static void send_navigation_StopAudioStream(struct lws*, unsigned int);
// タッチイベント関連UI.OnTouchEvent
static void send_ui_OnTouchEvent(struct lws*, unsigned int, unsigned int, unsigned int, unsigned int, char*);

//PAT
static void send_rpc_ui_EndAudioPassThru(struct lws*, unsigned int);
static void send_rpc_ui_EndAudioPassThru_error(struct lws*, unsigned int);
static void send_rpc_ui_PerformAudioPassThru(struct lws*);
static void send_rpc_ui_PerformAudioPassThru_error(struct lws*, unsigned int);
void *PerformAudioPassThru_timerThread(void *wsi);

//CAN
static void send_OnVehicleData(struct canfd_frame);
static void send_BasicCommunication_illumi_OnSystemRequest(struct canfd_frame);

//Bluetooth
static void send_BasicCommunication_OnStartDeviceDiscovery(struct lws*);
static void start_OnStartDeviceDiscovery_thread();
//sysmteRequest RequestType
#define HTTP            0
#define FILE_RESUME     1
#define AUTH_REQUEST    2
#define AUTH_CHALLENGE  3
#define AUTH_ACK        4
#define PROPRIETARY     5

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif /* _CRT_SECURE_NO_WARNINGS */
#endif /* _MSC_VER */

/* Apparently sscanf is not implemented in some "standard" libraries, so don't use it, if you
 * don't have to. */
#define sscanf THINK_TWICE_ABOUT_USING_SSCANF

#define STARTING_CAPACITY 16
#define MAX_NESTING       8192
#define FLOAT_FORMAT      "%1.17g"

#define SIZEOF_TOKEN(a)       (sizeof(a) - 1)
#define SKIP_CHAR(str)        ((*str)++)
#define SKIP_WHITESPACES(str) while (isspace((unsigned char)(**str))) { SKIP_CHAR(str); }
#define MAX(a, b)             ((a) > (b) ? (a) : (b))

#undef malloc
#undef free

static JSON_Malloc_Function parson_malloc = malloc;
static JSON_Free_Function parson_free = free;

#define IS_CONT(b) (((unsigned char)(b) & 0xC0) == 0x80) /* is utf-8 continuation byte */

/* Type definitions */
typedef union json_value_value {
    char        *string;
    double       number;
    JSON_Object *object;
    JSON_Array  *array;
    int          boolean;
    int          null;
} JSON_Value_Value;

struct json_value_t {
    JSON_Value      *parent;
    JSON_Value_Type  type;
    JSON_Value_Value value;
};

struct json_object_t {
    JSON_Value  *wrapping_value;
    char       **names;
    JSON_Value **values;
    size_t       count;
    size_t       capacity;
};

struct json_array_t {
    JSON_Value  *wrapping_value;
    JSON_Value **items;
    size_t       count;
    size_t       capacity;
};

/* Various */
static char * read_file(const char *filename);
static void   remove_comments(char *string, const char *start_token, const char *end_token);
static char * parson_strndup(const char *string, size_t n);
static char * parson_strdup(const char *string);
static int    hex_char_to_int(char c);
static int    parse_utf16_hex(const char *string, unsigned int *result);
static int    num_bytes_in_utf8_sequence(unsigned char c);
static int    verify_utf8_sequence(const unsigned char *string, int *len);
static int    is_valid_utf8(const char *string, size_t string_len);
static int    is_decimal(const char *string, size_t length);

/* JSON Object */
static JSON_Object * json_object_init(JSON_Value *wrapping_value);
static JSON_Status   json_object_add(JSON_Object *object, const char *name, JSON_Value *value);
static JSON_Status   json_object_resize(JSON_Object *object, size_t new_capacity);
static JSON_Value  * json_object_nget_value(const JSON_Object *object, const char *name, size_t n);
static void          json_object_free(JSON_Object *object);

/* JSON Array */
static JSON_Array * json_array_init(JSON_Value *wrapping_value);
static JSON_Status  json_array_add(JSON_Array *array, JSON_Value *value);
static JSON_Status  json_array_resize(JSON_Array *array, size_t new_capacity);
static void         json_array_free(JSON_Array *array);

/* JSON Value */
static JSON_Value * json_value_init_string_no_copy(char *string);

/* Parser */
static JSON_Status  skip_quotes(const char **string);
static int          parse_utf16(const char **unprocessed, char **processed);
static char *       process_string(const char *input, size_t len);
static char *       get_quoted_string(const char **string);
static JSON_Value * parse_object_value(const char **string, size_t nesting);
static JSON_Value * parse_array_value(const char **string, size_t nesting);
static JSON_Value * parse_string_value(const char **string);
static JSON_Value * parse_boolean_value(const char **string);
static JSON_Value * parse_number_value(const char **string);
static JSON_Value * parse_null_value(const char **string);
static JSON_Value * parse_value(const char **string, size_t nesting);

/* Serialization */
static int    json_serialize_to_buffer_r(const JSON_Value *value, char *buf, int level, int is_pretty, char *num_buf);
static int    json_serialize_string(const char *string, char *buf);
static int    append_indent(char *buf, int level);
static int    append_string(char *buf, const char *string);

// touch_event
int lws_touch_info[10][2] = {{0, 0},   // id:0 (x, y)
                             {0, 0},   // id:1 (x, y)
                             {0, 0},   // id:2 (x, y)
                             {0, 0},   // id:3 (x, y)
                             {0, 0},   // id:4 (x, y)
                             {0, 0},   // id:5 (x, y)
                             {0, 0},   // id:6 (x, y)
                             {0, 0},   // id:7 (x, y)
                             {0, 0},   // id:8 (x, y)
                             {0, 0}};  // id:9 (x, y)

// 車両情報(ダミーデータ)読み取り. 
FILE *fp_can = NULL;
pthread_mutex_t mutex;

// 車両情報送信スレッド生成状態. 
static int vehicledata_thread = -1;

// 定期通知スレッド未生成時に、GetVehicleDataをコールされると. 
// 次のダミーデータを読みにいくので、抑止するフラグを設置. 
int getvehicledata_init = 0;

// 車両情報 保持変数.
// 車両情報(GPS). 
double vehicledata_gps_longitudedegrees = 0;
double vehicledata_gps_latitudedegrees = 0;

int vehicledata_gps_utcyear = 0;
int vehicledata_gps_utcmonth = 0;
int vehicledata_gps_utcday = 0;
int vehicledata_gps_utchours = 0;
int vehicledata_gps_utcminutes = 0;
int vehicledata_gps_utcseconds = 0;

char* vehicledata_gps_compassdirection;

double vehicledata_gps_pdop = 0;
double vehicledata_gps_hdop = 0;
double vehicledata_gps_vdop = 0;

int vehicledata_gps_actual = 0;
int vehicledata_gps_satellites = 0;
char* vehicledata_gps_dimension;
double vehicledata_gps_altitude = 0;
double vehicledata_gps_heading = 0;
double vehicledata_gps_speed = 0;

// 車両情報(Speed). 
double vehicledata_speed = 0;


// Can関連
#define ILLUMI_REQ_ID 1001

// CANID
#define CANID_VEHICLE_SPPED 0x610
#define CANID_ILLUMI 0x123

static int running = 1;
const int canfd_on = 1;

extern int optind, opterr, optopt;
// Can受信用スレッド
void sigterm(int signo)
{
    // Signalを受け取ったらスレッド停止.
    running = 0;
}

void *can_thread(void *p) {
    fd_set rdfs;
    int socketid;
    int rcvbuf_size = 0;
    int ret;
    char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(__u32))];
    struct iovec iov;
    struct msghdr msg;
    struct cmsghdr *cmsg;

    //signal(SIGTERM, sigterm);
    //signal(SIGHUP, sigterm);
    //signal(SIGINT, sigterm);

    // CANソケット生成.
    socketid = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if( socketid < 0 ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN]\n", __func__, __LINE__);
        exit(1);
    }

    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN] socketid=%d\n", __func__, __LINE__, socketid);

    const char *ifname = "can0";
    struct ifreq ifr;
    memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
    strcpy(ifr.ifr_name, ifname);

    if (ioctl(socketid, SIOCGIFINDEX, &ifr) < 0) {
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN]\n", __func__, __LINE__);
        exit(1);
    }

    setsockopt(socketid, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_on, sizeof(canfd_on));

    struct can_filter rfilter[2];
    rfilter[0].can_id   = 0x610;
    rfilter[0].can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_SFF_MASK);
    rfilter[1].can_id   = 0x123;
    rfilter[1].can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_SFF_MASK);
    setsockopt(socketid, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

    struct sockaddr_can addr;
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socketid, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN]\n", __func__, __LINE__);
        exit(1);
    }

    /* these settings are static and can be held out of the hot path */
    struct canfd_frame frame;
    iov.iov_base = &frame;
    msg.msg_name = &addr;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &ctrlmsg;

    /* 待ち合わせ時間3秒 */
    struct timeval tv;
    tv.tv_sec  = 3;
    tv.tv_usec = 0;

    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN][Start] frame.can_id=%d\n", __func__, __LINE__, frame.can_id);

    while (running) {
        FD_ZERO(&rdfs);
        FD_SET(socketid, &rdfs);

        ret = select(socketid+1, &rdfs, NULL, NULL, NULL);
        if (ret < 0) {
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN][Start] can socket wait. socketid=%d errno=%d\n", __func__, __LINE__, socketid, errno);
            return;
        } else if (ret == 0) {
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN][Start] can socket wait. socketid=%d\n", __func__, __LINE__, socketid);
            continue;
        } else {
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN][Start] can socket running. socketid=%d\n", __func__, __LINE__, socketid);
        }

        if (FD_ISSET(socketid, &rdfs)) {
            int maxdlen = 0;
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN][Start] can socket running.\n", __func__, __LINE__);
            /* these settings may be modified by recvmsg() */
            iov.iov_len = sizeof(frame);
            msg.msg_namelen = sizeof(addr);
            msg.msg_controllen = sizeof(ctrlmsg);  
            msg.msg_flags = 0;

            int recvbytes = recvmsg(socketid, &msg, 0);

            if (recvbytes < 0) {
                if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN] errno=%d\n", __func__, __LINE__, errno);
                continue;
            }
            
            // コンピュータネットワークでは、Maximum Transmission Unit（MTU;最大伝送ユニット）は
            // 単一のネットワークレイヤでのトランザクションで通信できる最大のプロトコルデータユニット（PDU）のサイズです
            if ((size_t)recvbytes == CAN_MTU)
                maxdlen = CAN_MAX_DLEN;
            else if ((size_t)recvbytes == CANFD_MTU)
                // CAN FD（CAN with Flexible Data-Rate）通信とは、従来のCAN通信仕様を拡張した通信仕様です。
                // CAN FD通信は、CAN通信に比べて、大量のデータを高速で送受信することが可能になります。
                // データフィールドは最大64バイトに拡張され、通信ボーレートを1Mbps以上に高速化することが可能です。
                // CAN FDのデータフレームは、従来CANと同等のフィールド構成となっています。
                maxdlen = CANFD_MAX_DLEN;
            else {
                if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN]\n", __func__, __LINE__);
                //return 1;
            }
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN] maxdlen=%d\n", __func__, __LINE__, maxdlen);
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN][Modify] frame.can_id=%d\n", __func__, __LINE__, frame.can_id);
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN][Modify] frame.len=%d\n", __func__, __LINE__, frame.len);

            if( frame.can_id == CANID_VEHICLE_SPPED ) {
                // 本来はlengthを見て、CAN仕様に沿った形でデータを翻訳する必要があるが
                // Dia2ではテスト用に1バイト固定としているため直で設定.
                send_OnVehicleData(frame);
            } else if( frame.can_id == CANID_ILLUMI ){
                send_BasicCommunication_illumi_OnSystemRequest(frame);
            }
            usleep(500000);
        }
    }

    close(socketid);
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[CAN] can_thread End.\n", __func__, __LINE__);
    return 0;
}




/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/

/* Various */
static char * parson_strndup(const char *string, size_t n) {
    char *output_string = (char*)parson_malloc(n + 1);
    if (!output_string) {
        return NULL;
    }
    output_string[n] = '\0';
    strncpy(output_string, string, n);
    return output_string;
}

static char * parson_strdup(const char *string) {
    return parson_strndup(string, strlen(string));
}

static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static int parse_utf16_hex(const char *s, unsigned int *result) {
    int x1, x2, x3, x4;
    if (s[0] == '\0' || s[1] == '\0' || s[2] == '\0' || s[3] == '\0') {
        return 0;
    }
    x1 = hex_char_to_int(s[0]);
    x2 = hex_char_to_int(s[1]);
    x3 = hex_char_to_int(s[2]);
    x4 = hex_char_to_int(s[3]);
    if (x1 == -1 || x2 == -1 || x3 == -1 || x4 == -1) {
        return 0;
    }
    *result = (unsigned int)((x1 << 12) | (x2 << 8) | (x3 << 4) | x4);
    return 1;
}

static int num_bytes_in_utf8_sequence(unsigned char c) {
    if (c == 0xC0 || c == 0xC1 || c > 0xF4 || IS_CONT(c)) {
        return 0;
    } else if ((c & 0x80) == 0) {    /* 0xxxxxxx */
        return 1;
    } else if ((c & 0xE0) == 0xC0) { /* 110xxxxx */
        return 2;
    } else if ((c & 0xF0) == 0xE0) { /* 1110xxxx */
        return 3;
    } else if ((c & 0xF8) == 0xF0) { /* 11110xxx */
        return 4;
    }
    return 0; /* won't happen */
}

static int verify_utf8_sequence(const unsigned char *string, int *len) {
    unsigned int cp = 0;
    *len = num_bytes_in_utf8_sequence(string[0]);

    if (*len == 1) {
        cp = string[0];
    } else if (*len == 2 && IS_CONT(string[1])) {
        cp = string[0] & 0x1F;
        cp = (cp << 6) | (string[1] & 0x3F);
    } else if (*len == 3 && IS_CONT(string[1]) && IS_CONT(string[2])) {
        cp = ((unsigned char)string[0]) & 0xF;
        cp = (cp << 6) | (string[1] & 0x3F);
        cp = (cp << 6) | (string[2] & 0x3F);
    } else if (*len == 4 && IS_CONT(string[1]) && IS_CONT(string[2]) && IS_CONT(string[3])) {
        cp = string[0] & 0x7;
        cp = (cp << 6) | (string[1] & 0x3F);
        cp = (cp << 6) | (string[2] & 0x3F);
        cp = (cp << 6) | (string[3] & 0x3F);
    } else {
        return 0;
    }

    /* overlong encodings */
    if ((cp < 0x80    && *len > 1) ||
        (cp < 0x800   && *len > 2) ||
        (cp < 0x10000 && *len > 3)) {
        return 0;
    }

    /* invalid unicode */
    if (cp > 0x10FFFF) {
        return 0;
    }

    /* surrogate halves */
    if (cp >= 0xD800 && cp <= 0xDFFF) {
        return 0;
    }

    return 1;
}

static int is_valid_utf8(const char *string, size_t string_len) {
    int len = 0;
    const char *string_end =  string + string_len;
    while (string < string_end) {
        if (!verify_utf8_sequence((const unsigned char*)string, &len)) {
            return 0;
        }
        string += len;
    }
    return 1;
}

static int is_decimal(const char *string, size_t length) {
    if (length > 1 && string[0] == '0' && string[1] != '.') {
        return 0;
    }
    if (length > 2 && !strncmp(string, "-0", 2) && string[2] != '.') {
        return 0;
    }
    while (length--) {
        if (strchr("xX", string[length])) {
            return 0;
        }
    }
    return 1;
}

static char * read_file(const char * filename) {
    FILE *fp = fopen(filename, "r");
    size_t size_to_read = 0;
    size_t size_read = 0;
    long pos;
    char *file_contents;
    if (!fp) {
        return NULL;
    }
    fseek(fp, 0L, SEEK_END);
    pos = ftell(fp);
    if (pos < 0) {
        fclose(fp);
        return NULL;
    }
    size_to_read = pos;
    rewind(fp);
    file_contents = (char*)parson_malloc(sizeof(char) * (size_to_read + 1));
    if (!file_contents) {
        fclose(fp);
        return NULL;
    }
    size_read = fread(file_contents, 1, size_to_read, fp);
    if (size_read == 0 || ferror(fp)) {
        fclose(fp);
        parson_free(file_contents);
        return NULL;
    }
    fclose(fp);
    file_contents[size_read] = '\0';
    return file_contents;
}

static void remove_comments(char *string, const char *start_token, const char *end_token) {
    int in_string = 0, escaped = 0;
    size_t i;
    char *ptr = NULL, current_char;
    size_t start_token_len = strlen(start_token);
    size_t end_token_len = strlen(end_token);
    if (start_token_len == 0 || end_token_len == 0) {
        return;
    }
    while ((current_char = *string) != '\0') {
        if (current_char == '\\' && !escaped) {
            escaped = 1;
            string++;
            continue;
        } else if (current_char == '\"' && !escaped) {
            in_string = !in_string;
        } else if (!in_string && strncmp(string, start_token, start_token_len) == 0) {
            for(i = 0; i < start_token_len; i++) {
                string[i] = ' ';
            }
            string = string + start_token_len;
            ptr = strstr(string, end_token);
            if (!ptr) {
                return;
            }
            for (i = 0; i < (ptr - string) + end_token_len; i++) {
                string[i] = ' ';
            }
            string = ptr + end_token_len - 1;
        }
        escaped = 0;
        string++;
    }
}

/* JSON Object */
static JSON_Object * json_object_init(JSON_Value *wrapping_value) {
    JSON_Object *new_obj = (JSON_Object*)parson_malloc(sizeof(JSON_Object));
    if (new_obj == NULL) {
        return NULL;
    }
    new_obj->wrapping_value = wrapping_value;
    new_obj->names = (char**)NULL;
    new_obj->values = (JSON_Value**)NULL;
    new_obj->capacity = 0;
    new_obj->count = 0;
    return new_obj;
}

static JSON_Status json_object_add(JSON_Object *object, const char *name, JSON_Value *value) {
    size_t index = 0;
    if (object == NULL || name == NULL || value == NULL) {
        return JSONFailure;
    }
    if (json_object_get_value(object, name) != NULL) {
        return JSONFailure;
    }
    if (object->count >= object->capacity) {
        size_t new_capacity = MAX(object->capacity * 2, STARTING_CAPACITY);
        if (json_object_resize(object, new_capacity) == JSONFailure) {
            return JSONFailure;
        }
    }
    index = object->count;
    object->names[index] = parson_strdup(name);
    if (object->names[index] == NULL) {
        return JSONFailure;
    }
    value->parent = json_object_get_wrapping_value(object);
    object->values[index] = value;
    object->count++;
    return JSONSuccess;
}

static JSON_Status json_object_resize(JSON_Object *object, size_t new_capacity) {
    char **temp_names = NULL;
    JSON_Value **temp_values = NULL;

    if ((object->names == NULL && object->values != NULL) ||
        (object->names != NULL && object->values == NULL) ||
        new_capacity == 0) {
            return JSONFailure; /* Shouldn't happen */
    }
    temp_names = (char**)parson_malloc(new_capacity * sizeof(char*));
    if (temp_names == NULL) {
        return JSONFailure;
    }
    temp_values = (JSON_Value**)parson_malloc(new_capacity * sizeof(JSON_Value*));
    if (temp_values == NULL) {
        parson_free(temp_names);
        return JSONFailure;
    }
    if (object->names != NULL && object->values != NULL && object->count > 0) {
        memcpy(temp_names, object->names, object->count * sizeof(char*));
        memcpy(temp_values, object->values, object->count * sizeof(JSON_Value*));
    }
    parson_free(object->names);
    parson_free(object->values);
    object->names = temp_names;
    object->values = temp_values;
    object->capacity = new_capacity;
    return JSONSuccess;
}

static JSON_Value * json_object_nget_value(const JSON_Object *object, const char *name, size_t n) {
    size_t i, name_length;
    for (i = 0; i < json_object_get_count(object); i++) {
        name_length = strlen(object->names[i]);
        if (name_length != n) {
            continue;
        }
        if (strncmp(object->names[i], name, n) == 0) {
            return object->values[i];
        }
    }
    return NULL;
}

static void json_object_free(JSON_Object *object) {
    size_t i;
    for (i = 0; i < object->count; i++) {
        parson_free(object->names[i]);
        json_value_free(object->values[i]);
    }
    parson_free(object->names);
    parson_free(object->values);
    parson_free(object);
}

/* JSON Array */
static JSON_Array * json_array_init(JSON_Value *wrapping_value) {
    JSON_Array *new_array = (JSON_Array*)parson_malloc(sizeof(JSON_Array));
    if (new_array == NULL) {
        return NULL;
    }
    new_array->wrapping_value = wrapping_value;
    new_array->items = (JSON_Value**)NULL;
    new_array->capacity = 0;
    new_array->count = 0;
    return new_array;
}

static JSON_Status json_array_add(JSON_Array *array, JSON_Value *value) {
    if (array->count >= array->capacity) {
        size_t new_capacity = MAX(array->capacity * 2, STARTING_CAPACITY);
        if (json_array_resize(array, new_capacity) == JSONFailure) {
            return JSONFailure;
        }
    }
    value->parent = json_array_get_wrapping_value(array);
    array->items[array->count] = value;
    array->count++;
    return JSONSuccess;
}

static JSON_Status json_array_resize(JSON_Array *array, size_t new_capacity) {
    JSON_Value **new_items = NULL;
    if (new_capacity == 0) {
        return JSONFailure;
    }
    new_items = (JSON_Value**)parson_malloc(new_capacity * sizeof(JSON_Value*));
    if (new_items == NULL) {
        return JSONFailure;
    }
    if (array->items != NULL && array->count > 0) {
        memcpy(new_items, array->items, array->count * sizeof(JSON_Value*));
    }
    parson_free(array->items);
    array->items = new_items;
    array->capacity = new_capacity;
    return JSONSuccess;
}

static void json_array_free(JSON_Array *array) {
    size_t i;
    for (i = 0; i < array->count; i++) {
        json_value_free(array->items[i]);
    }
    parson_free(array->items);
    parson_free(array);
}

/* JSON Value */
static JSON_Value * json_value_init_string_no_copy(char *string) {
    JSON_Value *new_value = (JSON_Value*)parson_malloc(sizeof(JSON_Value));
    if (!new_value) {
        return NULL;
    }
    new_value->parent = NULL;
    new_value->type = JSONString;
    new_value->value.string = string;
    return new_value;
}

/* Parser */
static JSON_Status skip_quotes(const char **string) {
    if (**string != '\"') {
        return JSONFailure;
    }
    SKIP_CHAR(string);
    while (**string != '\"') {
        if (**string == '\0') {
            return JSONFailure;
        } else if (**string == '\\') {
            SKIP_CHAR(string);
            if (**string == '\0') {
                return JSONFailure;
            }
        }
        SKIP_CHAR(string);
    }
    SKIP_CHAR(string);
    return JSONSuccess;
}

static int parse_utf16(const char **unprocessed, char **processed) {
    unsigned int cp, lead, trail;
    int parse_succeeded = 0;
    char *processed_ptr = *processed;
    const char *unprocessed_ptr = *unprocessed;
    unprocessed_ptr++; /* skips u */
    parse_succeeded = parse_utf16_hex(unprocessed_ptr, &cp);
    if (!parse_succeeded) {
        return JSONFailure;
    }
    if (cp < 0x80) {
        processed_ptr[0] = (char)cp; /* 0xxxxxxx */
    } else if (cp < 0x800) {
        processed_ptr[0] = ((cp >> 6) & 0x1F) | 0xC0; /* 110xxxxx */
        processed_ptr[1] = ((cp)      & 0x3F) | 0x80; /* 10xxxxxx */
        processed_ptr += 1;
    } else if (cp < 0xD800 || cp > 0xDFFF) {
        processed_ptr[0] = ((cp >> 12) & 0x0F) | 0xE0; /* 1110xxxx */
        processed_ptr[1] = ((cp >> 6)  & 0x3F) | 0x80; /* 10xxxxxx */
        processed_ptr[2] = ((cp)       & 0x3F) | 0x80; /* 10xxxxxx */
        processed_ptr += 2;
    } else if (cp >= 0xD800 && cp <= 0xDBFF) { /* lead surrogate (0xD800..0xDBFF) */
        lead = cp;
        unprocessed_ptr += 4; /* should always be within the buffer, otherwise previous sscanf would fail */
        if (*unprocessed_ptr++ != '\\' || *unprocessed_ptr++ != 'u') {
            return JSONFailure;
        }
        parse_succeeded = parse_utf16_hex(unprocessed_ptr, &trail);
        if (!parse_succeeded || trail < 0xDC00 || trail > 0xDFFF) { /* valid trail surrogate? (0xDC00..0xDFFF) */
            return JSONFailure;
        }
        cp = ((((lead - 0xD800) & 0x3FF) << 10) | ((trail - 0xDC00) & 0x3FF)) + 0x010000;
        processed_ptr[0] = (((cp >> 18) & 0x07) | 0xF0); /* 11110xxx */
        processed_ptr[1] = (((cp >> 12) & 0x3F) | 0x80); /* 10xxxxxx */
        processed_ptr[2] = (((cp >> 6)  & 0x3F) | 0x80); /* 10xxxxxx */
        processed_ptr[3] = (((cp)       & 0x3F) | 0x80); /* 10xxxxxx */
        processed_ptr += 3;
    } else { /* trail surrogate before lead surrogate */
        return JSONFailure;
    }
    unprocessed_ptr += 3;
    *processed = processed_ptr;
    *unprocessed = unprocessed_ptr;
    return JSONSuccess;
}


/* Copies and processes passed string up to supplied length.
Example: "\u006Corem ipsum" -> lorem ipsum */
static char* process_string(const char *input, size_t len) {
    const char *input_ptr = input;
    size_t initial_size = (len + 1) * sizeof(char);
    size_t final_size = 0;
    char *output = NULL, *output_ptr = NULL, *resized_output = NULL;
    output = (char*)parson_malloc(initial_size);
    if (output == NULL) {
        goto error;
    }
    output_ptr = output;
    while ((*input_ptr != '\0') && (size_t)(input_ptr - input) < len) {
        if (*input_ptr == '\\') {
            input_ptr++;
            switch (*input_ptr) {
                case '\"': *output_ptr = '\"'; break;
                case '\\': *output_ptr = '\\'; break;
                case '/':  *output_ptr = '/';  break;
                case 'b':  *output_ptr = '\b'; break;
                case 'f':  *output_ptr = '\f'; break;
                case 'n':  *output_ptr = '\n'; break;
                case 'r':  *output_ptr = '\r'; break;
                case 't':  *output_ptr = '\t'; break;
                case 'u':
                    if (parse_utf16(&input_ptr, &output_ptr) == JSONFailure) {
                        goto error;
                    }
                    break;
                default:
                    goto error;
            }
        } else if ((unsigned char)*input_ptr < 0x20) {
            goto error; /* 0x00-0x19 are invalid characters for json string (http://www.ietf.org/rfc/rfc4627.txt) */
        } else {
            *output_ptr = *input_ptr;
        }
        output_ptr++;
        input_ptr++;
    }
    *output_ptr = '\0';
    /* resize to new length */
    final_size = (size_t)(output_ptr-output) + 1;
    /* todo: don't resize if final_size == initial_size */
    resized_output = (char*)parson_malloc(final_size);
    if (resized_output == NULL) {
        goto error;
    }
    memcpy(resized_output, output, final_size);
    parson_free(output);
    return resized_output;
error:
    parson_free(output);
    return NULL;
}

/* Return processed contents of a string between quotes and
   skips passed argument to a matching quote. */
static char * get_quoted_string(const char **string) {
    const char *string_start = *string;
    size_t string_len = 0;
    JSON_Status status = skip_quotes(string);
    if (status != JSONSuccess) {
        return NULL;
    }
    string_len = *string - string_start - 2; /* length without quotes */
    return process_string(string_start + 1, string_len);
}

static JSON_Value * parse_value(const char **string, size_t nesting) {
    if (nesting > MAX_NESTING) {
        return NULL;
    }
    SKIP_WHITESPACES(string);
    switch (**string) {
        case '{':
            return parse_object_value(string, nesting + 1);
        case '[':
            return parse_array_value(string, nesting + 1);
        case '\"':
            return parse_string_value(string);
        case 'f': case 't':
            return parse_boolean_value(string);
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return parse_number_value(string);
        case 'n':
            return parse_null_value(string);
        default:
            return NULL;
    }
}

static JSON_Value * parse_object_value(const char **string, size_t nesting) {
    JSON_Value *output_value = json_value_init_object(), *new_value = NULL;
    JSON_Object *output_object = json_value_get_object(output_value);
    char *new_key = NULL;
    if (output_value == NULL || **string != '{') {
        return NULL;
    }
    SKIP_CHAR(string);
    SKIP_WHITESPACES(string);
    if (**string == '}') { /* empty object */
        SKIP_CHAR(string);
        return output_value;
    }
    while (**string != '\0') {
        new_key = get_quoted_string(string);
        if (new_key == NULL) {
            json_value_free(output_value);
            return NULL;
        }
        SKIP_WHITESPACES(string);
        if (**string != ':') {
            parson_free(new_key);
            json_value_free(output_value);
            return NULL;
        }
        SKIP_CHAR(string);
        new_value = parse_value(string, nesting);
        if (new_value == NULL) {
            parson_free(new_key);
            json_value_free(output_value);
            return NULL;
        }
        if (json_object_add(output_object, new_key, new_value) == JSONFailure) {
            parson_free(new_key);
            json_value_free(new_value);
            json_value_free(output_value);
            return NULL;
        }
        parson_free(new_key);
        SKIP_WHITESPACES(string);
        if (**string != ',') {
            break;
        }
        SKIP_CHAR(string);
        SKIP_WHITESPACES(string);
    }
    SKIP_WHITESPACES(string);
    if (**string != '}' || /* Trim object after parsing is over */
        json_object_resize(output_object, json_object_get_count(output_object)) == JSONFailure) {
            json_value_free(output_value);
            return NULL;
    }
    SKIP_CHAR(string);
    return output_value;
}

static JSON_Value * parse_array_value(const char **string, size_t nesting) {
    JSON_Value *output_value = json_value_init_array(), *new_array_value = NULL;
    JSON_Array *output_array = json_value_get_array(output_value);
    if (!output_value || **string != '[') {
        return NULL;
    }
    SKIP_CHAR(string);
    SKIP_WHITESPACES(string);
    if (**string == ']') { /* empty array */
        SKIP_CHAR(string);
        return output_value;
    }
    while (**string != '\0') {
        new_array_value = parse_value(string, nesting);
        if (new_array_value == NULL) {
            json_value_free(output_value);
            return NULL;
        }
        if (json_array_add(output_array, new_array_value) == JSONFailure) {
            json_value_free(new_array_value);
            json_value_free(output_value);
            return NULL;
        }
        SKIP_WHITESPACES(string);
        if (**string != ',') {
            break;
        }
        SKIP_CHAR(string);
        SKIP_WHITESPACES(string);
    }
    SKIP_WHITESPACES(string);
    if (**string != ']' || /* Trim array after parsing is over */
        json_array_resize(output_array, json_array_get_count(output_array)) == JSONFailure) {
            json_value_free(output_value);
            return NULL;
    }
    SKIP_CHAR(string);
    return output_value;
}

static JSON_Value * parse_string_value(const char **string) {
    JSON_Value *value = NULL;
    char *new_string = get_quoted_string(string);
    if (new_string == NULL) {
        return NULL;
    }
    value = json_value_init_string_no_copy(new_string);
    if (value == NULL) {
        parson_free(new_string);
        return NULL;
    }
    return value;
}

static JSON_Value * parse_boolean_value(const char **string) {
    size_t true_token_size = SIZEOF_TOKEN("true");
    size_t false_token_size = SIZEOF_TOKEN("false");
    if (strncmp("true", *string, true_token_size) == 0) {
        *string += true_token_size;
        return json_value_init_boolean(1);
    } else if (strncmp("false", *string, false_token_size) == 0) {
        *string += false_token_size;
        return json_value_init_boolean(0);
    }
    return NULL;
}

static JSON_Value * parse_number_value(const char **string) {
    char *end;
    double number = 0;
    errno = 0;
    number = strtod(*string, &end);
    if (errno || !is_decimal(*string, end - *string)) {
        return NULL;
    }
    *string = end;
    return json_value_init_number(number);
}

static JSON_Value * parse_null_value(const char **string) {
    size_t token_size = SIZEOF_TOKEN("null");
    if (strncmp("null", *string, token_size) == 0) {
        *string += token_size;
        return json_value_init_null();
    }
    return NULL;
}

/* Serialization */
#define APPEND_STRING(str) do { written = append_string(buf, (str));\
                                if (written < 0) { return -1; }\
                                if (buf != NULL) { buf += written; }\
                                written_total += written; } while(0)

#define APPEND_INDENT(level) do { written = append_indent(buf, (level));\
                                  if (written < 0) { return -1; }\
                                  if (buf != NULL) { buf += written; }\
                                  written_total += written; } while(0)

static int json_serialize_to_buffer_r(const JSON_Value *value, char *buf, int level, int is_pretty, char *num_buf)
{
    const char *key = NULL, *string = NULL;
    JSON_Value *temp_value = NULL;
    JSON_Array *array = NULL;
    JSON_Object *object = NULL;
    size_t i = 0, count = 0;
    double num = 0.0;
    int written = -1, written_total = 0;

    switch (json_value_get_type(value)) {
        case JSONArray:
            array = json_value_get_array(value);
            count = json_array_get_count(array);
            APPEND_STRING("[");
            if (count > 0 && is_pretty) {
                APPEND_STRING("\n");
            }
            for (i = 0; i < count; i++) {
                if (is_pretty) {
                    APPEND_INDENT(level+1);
                }
                temp_value = json_array_get_value(array, i);
                written = json_serialize_to_buffer_r(temp_value, buf, level+1, is_pretty, num_buf);
                if (written < 0) {
                    return -1;
                }
                if (buf != NULL) {
                    buf += written;
                }
                written_total += written;
                if (i < (count - 1)) {
                    APPEND_STRING(",");
                }
                if (is_pretty) {
                    APPEND_STRING("\n");
                }
            }
            if (count > 0 && is_pretty) {
                APPEND_INDENT(level);
            }
            APPEND_STRING("]");
            return written_total;
        case JSONObject:
            object = json_value_get_object(value);
            count  = json_object_get_count(object);
            APPEND_STRING("{");
            if (count > 0 && is_pretty) {
                APPEND_STRING("\n");
            }
            for (i = 0; i < count; i++) {
                key = json_object_get_name(object, i);
                if (key == NULL) {
                    return -1;
                }
                if (is_pretty) {
                    APPEND_INDENT(level+1);
                }
                written = json_serialize_string(key, buf);
                if (written < 0) {
                    return -1;
                }
                if (buf != NULL) {
                    buf += written;
                }
                written_total += written;
                APPEND_STRING(":");
                if (is_pretty) {
                    APPEND_STRING(" ");
                }
                temp_value = json_object_get_value(object, key);
                written = json_serialize_to_buffer_r(temp_value, buf, level+1, is_pretty, num_buf);
                if (written < 0) {
                    return -1;
                }
                if (buf != NULL) {
                    buf += written;
                }
                written_total += written;
                if (i < (count - 1)) {
                    APPEND_STRING(",");
                }
                if (is_pretty) {
                    APPEND_STRING("\n");
                }
            }
            if (count > 0 && is_pretty) {
                APPEND_INDENT(level);
            }
            APPEND_STRING("}");
            return written_total;
        case JSONString:
            string = json_value_get_string(value);
            if (string == NULL) {
                return -1;
            }
            written = json_serialize_string(string, buf);
            if (written < 0) {
                return -1;
            }
            if (buf != NULL) {
                buf += written;
            }
            written_total += written;
            return written_total;
        case JSONBoolean:
            if (json_value_get_boolean(value)) {
                APPEND_STRING("true");
            } else {
                APPEND_STRING("false");
            }
            return written_total;
        case JSONNumber:
            num = json_value_get_number(value);
            if (buf != NULL) {
                num_buf = buf;
            }
            written = sprintf(num_buf, FLOAT_FORMAT, num);
            if (written < 0) {
                return -1;
            }
            if (buf != NULL) {
                buf += written;
            }
            written_total += written;
            return written_total;
        case JSONNull:
            APPEND_STRING("null");
            return written_total;
        case JSONError:
            return -1;
        default:
            return -1;
    }
}

static int json_serialize_string(const char *string, char *buf) {
    size_t i = 0, len = strlen(string);
    char c = '\0';
    int written = -1, written_total = 0;
    APPEND_STRING("\"");
    for (i = 0; i < len; i++) {
        c = string[i];
        switch (c) {
            case '\"': APPEND_STRING("\\\""); break;
            case '\\': APPEND_STRING("\\\\"); break;
            case '/':  APPEND_STRING("\\/"); break; /* to make json embeddable in xml\/html */
            case '\b': APPEND_STRING("\\b"); break;
            case '\f': APPEND_STRING("\\f"); break;
            case '\n': APPEND_STRING("\\n"); break;
            case '\r': APPEND_STRING("\\r"); break;
            case '\t': APPEND_STRING("\\t"); break;
            case '\x00': APPEND_STRING("\\u0000"); break;
            case '\x01': APPEND_STRING("\\u0001"); break;
            case '\x02': APPEND_STRING("\\u0002"); break;
            case '\x03': APPEND_STRING("\\u0003"); break;
            case '\x04': APPEND_STRING("\\u0004"); break;
            case '\x05': APPEND_STRING("\\u0005"); break;
            case '\x06': APPEND_STRING("\\u0006"); break;
            case '\x07': APPEND_STRING("\\u0007"); break;
            /* '\x08' duplicate: '\b' */
            /* '\x09' duplicate: '\t' */
            /* '\x0a' duplicate: '\n' */
            case '\x0b': APPEND_STRING("\\u000b"); break;
            /* '\x0c' duplicate: '\f' */
            /* '\x0d' duplicate: '\r' */
            case '\x0e': APPEND_STRING("\\u000e"); break;
            case '\x0f': APPEND_STRING("\\u000f"); break;
            case '\x10': APPEND_STRING("\\u0010"); break;
            case '\x11': APPEND_STRING("\\u0011"); break;
            case '\x12': APPEND_STRING("\\u0012"); break;
            case '\x13': APPEND_STRING("\\u0013"); break;
            case '\x14': APPEND_STRING("\\u0014"); break;
            case '\x15': APPEND_STRING("\\u0015"); break;
            case '\x16': APPEND_STRING("\\u0016"); break;
            case '\x17': APPEND_STRING("\\u0017"); break;
            case '\x18': APPEND_STRING("\\u0018"); break;
            case '\x19': APPEND_STRING("\\u0019"); break;
            case '\x1a': APPEND_STRING("\\u001a"); break;
            case '\x1b': APPEND_STRING("\\u001b"); break;
            case '\x1c': APPEND_STRING("\\u001c"); break;
            case '\x1d': APPEND_STRING("\\u001d"); break;
            case '\x1e': APPEND_STRING("\\u001e"); break;
            case '\x1f': APPEND_STRING("\\u001f"); break;
            default:
                if (buf != NULL) {
                    buf[0] = c;
                    buf += 1;
                }
                written_total += 1;
                break;
        }
    }
    APPEND_STRING("\"");
    return written_total;
}

static int append_indent(char *buf, int level) {
    int i;
    int written = -1, written_total = 0;
    for (i = 0; i < level; i++) {
        APPEND_STRING("    ");
    }
    return written_total;
}

static int append_string(char *buf, const char *string) {
    if (buf == NULL) {
        return (int)strlen(string);
    }
    return sprintf(buf, "%s", string);
}

#undef APPEND_STRING
#undef APPEND_INDENT

/* Parser API */
JSON_Value * json_parse_file(const char *filename) {
    char *file_contents = read_file(filename);
    JSON_Value *output_value = NULL;
    if (file_contents == NULL) {
        return NULL;
    }
    output_value = json_parse_string(file_contents);
    parson_free(file_contents);
    return output_value;
}

JSON_Value * json_parse_file_with_comments(const char *filename) {
    char *file_contents = read_file(filename);
    JSON_Value *output_value = NULL;
    if (file_contents == NULL) {
        return NULL;
    }
    output_value = json_parse_string_with_comments(file_contents);
    parson_free(file_contents);
    return output_value;
}

JSON_Value * json_parse_string(const char *string) {
    if (string == NULL) {
        return NULL;
    }
    if (string[0] == '\xEF' && string[1] == '\xBB' && string[2] == '\xBF') {
        string = string + 3; /* Support for UTF-8 BOM */
    }
    return parse_value((const char**)&string, 0);
}

JSON_Value * json_parse_string_with_comments(const char *string) {
    JSON_Value *result = NULL;
    char *string_mutable_copy = NULL, *string_mutable_copy_ptr = NULL;
    string_mutable_copy = parson_strdup(string);
    if (string_mutable_copy == NULL) {
        return NULL;
    }
    remove_comments(string_mutable_copy, "/*", "*/");
    remove_comments(string_mutable_copy, "//", "\n");
    string_mutable_copy_ptr = string_mutable_copy;
    result = parse_value((const char**)&string_mutable_copy_ptr, 0);
    parson_free(string_mutable_copy);
    return result;
}

/* JSON Object API */

JSON_Value * json_object_get_value(const JSON_Object *object, const char *name) {
    if (object == NULL || name == NULL) {
        return NULL;
    }
    return json_object_nget_value(object, name, strlen(name));
}

const char * sdl_json_object_get_string(const JSON_Object *object, const char *name) {
    return json_value_get_string(json_object_get_value(object, name));
}

double json_object_get_number(const JSON_Object *object, const char *name) {
    return json_value_get_number(json_object_get_value(object, name));
}

JSON_Object * sdl_json_object_get_object(const JSON_Object *object, const char *name) {
    return json_value_get_object(json_object_get_value(object, name));
}

JSON_Array * sdl_json_object_get_array(const JSON_Object *object, const char *name) {
    return json_value_get_array(json_object_get_value(object, name));
}

int sdl_json_object_get_boolean(const JSON_Object *object, const char *name) {
    return json_value_get_boolean(json_object_get_value(object, name));
}

JSON_Value * json_object_dotget_value(const JSON_Object *object, const char *name) {
    const char *dot_position = strchr(name, '.');
    if (!dot_position) {
        return json_object_get_value(object, name);
    }
    object = json_value_get_object(json_object_nget_value(object, name, dot_position - name));
    return json_object_dotget_value(object, dot_position + 1);
}

const char * json_object_dotget_string(const JSON_Object *object, const char *name) {
    return json_value_get_string(json_object_dotget_value(object, name));
}

double json_object_dotget_number(const JSON_Object *object, const char *name) {
    return json_value_get_number(json_object_dotget_value(object, name));
}

JSON_Object * json_object_dotget_object(const JSON_Object *object, const char *name) {
    return json_value_get_object(json_object_dotget_value(object, name));
}

JSON_Array * json_object_dotget_array(const JSON_Object *object, const char *name) {
    return json_value_get_array(json_object_dotget_value(object, name));
}

int json_object_dotget_boolean(const JSON_Object *object, const char *name) {
    return json_value_get_boolean(json_object_dotget_value(object, name));
}

size_t json_object_get_count(const JSON_Object *object) {
    return object ? object->count : 0;
}

const char * json_object_get_name(const JSON_Object *object, size_t index) {
    if (object == NULL || index >= json_object_get_count(object)) {
        return NULL;
    }
    return object->names[index];
}

JSON_Value * json_object_get_value_at(const JSON_Object *object, size_t index) {
    if (object == NULL || index >= json_object_get_count(object)) {
        return NULL;
    }
    return object->values[index];
}

JSON_Value *json_object_get_wrapping_value(const JSON_Object *object) {
    return object->wrapping_value;
}

int json_object_has_value (const JSON_Object *object, const char *name) {
    return json_object_get_value(object, name) != NULL;
}

int json_object_has_value_of_type(const JSON_Object *object, const char *name, JSON_Value_Type type) {
    JSON_Value *val = json_object_get_value(object, name);
    return val != NULL && json_value_get_type(val) == type;
}

int json_object_dothas_value (const JSON_Object *object, const char *name) {
    return json_object_dotget_value(object, name) != NULL;
}

int json_object_dothas_value_of_type(const JSON_Object *object, const char *name, JSON_Value_Type type) {
    JSON_Value *val = json_object_dotget_value(object, name);
    return val != NULL && json_value_get_type(val) == type;
}

/* JSON Array API */
JSON_Value * json_array_get_value(const JSON_Array *array, size_t index) {
    if (array == NULL || index >= json_array_get_count(array)) {
        return NULL;
    }
    return array->items[index];
}

const char * json_array_get_string(const JSON_Array *array, size_t index) {
    return json_value_get_string(json_array_get_value(array, index));
}

double json_array_get_number(const JSON_Array *array, size_t index) {
    return json_value_get_number(json_array_get_value(array, index));
}

JSON_Object * json_array_get_object(const JSON_Array *array, size_t index) {
    return json_value_get_object(json_array_get_value(array, index));
}

JSON_Array * json_array_get_array(const JSON_Array *array, size_t index) {
    return json_value_get_array(json_array_get_value(array, index));
}

int json_array_get_boolean(const JSON_Array *array, size_t index) {
    return json_value_get_boolean(json_array_get_value(array, index));
}

size_t json_array_get_count(const JSON_Array *array) {
    return array ? array->count : 0;
}

JSON_Value * json_array_get_wrapping_value(const JSON_Array *array) {
    return array->wrapping_value;
}

/* JSON Value API */
JSON_Value_Type json_value_get_type(const JSON_Value *value) {
    return value ? value->type : JSONError;
}

JSON_Object * json_value_get_object(const JSON_Value *value) {
    return json_value_get_type(value) == JSONObject ? value->value.object : NULL;
}

JSON_Array * json_value_get_array(const JSON_Value *value) {
    return json_value_get_type(value) == JSONArray ? value->value.array : NULL;
}

const char * json_value_get_string(const JSON_Value *value) {
    return json_value_get_type(value) == JSONString ? value->value.string : NULL;
}

double json_value_get_number(const JSON_Value *value) {
    return json_value_get_type(value) == JSONNumber ? value->value.number : 0;
}

int json_value_get_boolean(const JSON_Value *value) {
    return json_value_get_type(value) == JSONBoolean ? value->value.boolean : -1;
}

JSON_Value * json_value_get_parent (const JSON_Value *value) {
    return value ? value->parent : NULL;
}

void json_value_free(JSON_Value *value) {
    switch (json_value_get_type(value)) {
        case JSONObject:
            json_object_free(value->value.object);
            break;
        case JSONString:
            parson_free(value->value.string);
            break;
        case JSONArray:
            json_array_free(value->value.array);
            break;
        default:
            break;
    }
    parson_free(value);
}

JSON_Value * json_value_init_object(void) {
    JSON_Value *new_value = (JSON_Value*)parson_malloc(sizeof(JSON_Value));
    if (!new_value) {
        return NULL;
    }
    new_value->parent = NULL;
    new_value->type = JSONObject;
    new_value->value.object = json_object_init(new_value);
    if (!new_value->value.object) {
        parson_free(new_value);
        return NULL;
    }
    return new_value;
}

JSON_Value * json_value_init_array(void) {
    JSON_Value *new_value = (JSON_Value*)parson_malloc(sizeof(JSON_Value));
    if (!new_value) {
        return NULL;
    }
    new_value->parent = NULL;
    new_value->type = JSONArray;
    new_value->value.array = json_array_init(new_value);
    if (!new_value->value.array) {
        parson_free(new_value);
        return NULL;
    }
    return new_value;
}

JSON_Value * json_value_init_string(const char *string) {
    char *copy = NULL;
    JSON_Value *value;
    size_t string_len = 0;
    if (string == NULL) {
        return NULL;
    }
    string_len = strlen(string);
    if (!is_valid_utf8(string, string_len)) {
        return NULL;
    }
    copy = parson_strndup(string, string_len);
    if (copy == NULL) {
        return NULL;
    }
    value = json_value_init_string_no_copy(copy);
    if (value == NULL) {
        parson_free(copy);
    }
    return value;
}

JSON_Value * json_value_init_number(double number) {
    JSON_Value *new_value = NULL;
    if ((number * 0.0) != 0.0) { /* nan and inf test */
        return NULL;
    }
    new_value = (JSON_Value*)parson_malloc(sizeof(JSON_Value));
    if (new_value == NULL) {
        return NULL;
    }
    new_value->parent = NULL;
    new_value->type = JSONNumber;
    new_value->value.number = number;
    return new_value;
}

JSON_Value * json_value_init_boolean(int boolean) {
    JSON_Value *new_value = (JSON_Value*)parson_malloc(sizeof(JSON_Value));
    if (!new_value) {
        return NULL;
    }
    new_value->parent = NULL;
    new_value->type = JSONBoolean;
    new_value->value.boolean = boolean ? 1 : 0;
    return new_value;
}

JSON_Value * json_value_init_null(void) {
    JSON_Value *new_value = (JSON_Value*)parson_malloc(sizeof(JSON_Value));
    if (!new_value) {
        return NULL;
    }
    new_value->parent = NULL;
    new_value->type = JSONNull;
    return new_value;
}

JSON_Value * json_value_deep_copy(const JSON_Value *value) {
    size_t i = 0;
    JSON_Value *return_value = NULL, *temp_value_copy = NULL, *temp_value = NULL;
    const char *temp_string = NULL, *temp_key = NULL;
    char *temp_string_copy = NULL;
    JSON_Array *temp_array = NULL, *temp_array_copy = NULL;
    JSON_Object *temp_object = NULL, *temp_object_copy = NULL;

    switch (json_value_get_type(value)) {
        case JSONArray:
            temp_array = json_value_get_array(value);
            return_value = json_value_init_array();
            if (return_value == NULL) {
                return NULL;
            }
            temp_array_copy = json_value_get_array(return_value);
            for (i = 0; i < json_array_get_count(temp_array); i++) {
                temp_value = json_array_get_value(temp_array, i);
                temp_value_copy = json_value_deep_copy(temp_value);
                if (temp_value_copy == NULL) {
                    json_value_free(return_value);
                    return NULL;
                }
                if (json_array_add(temp_array_copy, temp_value_copy) == JSONFailure) {
                    json_value_free(return_value);
                    json_value_free(temp_value_copy);
                    return NULL;
                }
            }
            return return_value;
        case JSONObject:
            temp_object = json_value_get_object(value);
            return_value = json_value_init_object();
            if (return_value == NULL) {
                return NULL;
            }
            temp_object_copy = json_value_get_object(return_value);
            for (i = 0; i < json_object_get_count(temp_object); i++) {
                temp_key = json_object_get_name(temp_object, i);
                temp_value = json_object_get_value(temp_object, temp_key);
                temp_value_copy = json_value_deep_copy(temp_value);
                if (temp_value_copy == NULL) {
                    json_value_free(return_value);
                    return NULL;
                }
                if (json_object_add(temp_object_copy, temp_key, temp_value_copy) == JSONFailure) {
                    json_value_free(return_value);
                    json_value_free(temp_value_copy);
                    return NULL;
                }
            }
            return return_value;
        case JSONBoolean:
            return json_value_init_boolean(json_value_get_boolean(value));
        case JSONNumber:
            return json_value_init_number(json_value_get_number(value));
        case JSONString:
            temp_string = json_value_get_string(value);
            if (temp_string == NULL) {
                return NULL;
            }
            temp_string_copy = parson_strdup(temp_string);
            if (temp_string_copy == NULL) {
                return NULL;
            }
            return_value = json_value_init_string_no_copy(temp_string_copy);
            if (return_value == NULL) {
                parson_free(temp_string_copy);
            }
            return return_value;
        case JSONNull:
            return json_value_init_null();
        case JSONError:
            return NULL;
        default:
            return NULL;
    }
}

size_t json_serialization_size(const JSON_Value *value) {
    char num_buf[1100]; /* recursively allocating buffer on stack is a bad idea, so let's do it only once */
    int res = json_serialize_to_buffer_r(value, NULL, 0, 0, num_buf);
    return res < 0 ? 0 : (size_t)(res + 1);
}

JSON_Status json_serialize_to_buffer(const JSON_Value *value, char *buf, size_t buf_size_in_bytes) {
    int written = -1;
    size_t needed_size_in_bytes = json_serialization_size(value);
    if (needed_size_in_bytes == 0 || buf_size_in_bytes < needed_size_in_bytes) {
        return JSONFailure;
    }
    written = json_serialize_to_buffer_r(value, buf, 0, 0, NULL);
    if (written < 0) {
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_serialize_to_file(const JSON_Value *value, const char *filename) {
    JSON_Status return_code = JSONSuccess;
    FILE *fp = NULL;
    char *serialized_string = json_serialize_to_string(value);
    if (serialized_string == NULL) {
        return JSONFailure;
    }
    fp = fopen(filename, "w");
    if (fp == NULL) {
        json_free_serialized_string(serialized_string);
        return JSONFailure;
    }
    if (fputs(serialized_string, fp) == EOF) {
        return_code = JSONFailure;
    }
    if (fclose(fp) == EOF) {
        return_code = JSONFailure;
    }
    json_free_serialized_string(serialized_string);
    return return_code;
}

char * json_serialize_to_string(const JSON_Value *value) {
    JSON_Status serialization_result = JSONFailure;
    size_t buf_size_bytes = json_serialization_size(value);
    char *buf = NULL;
    if (buf_size_bytes == 0) {
        return NULL;
    }
    buf = (char*)parson_malloc(buf_size_bytes);
    if (buf == NULL) {
        return NULL;
    }
    serialization_result = json_serialize_to_buffer(value, buf, buf_size_bytes);
    if (serialization_result == JSONFailure) {
        json_free_serialized_string(buf);
        return NULL;
    }
    return buf;
}

size_t json_serialization_size_pretty(const JSON_Value *value) {
    char num_buf[1100]; /* recursively allocating buffer on stack is a bad idea, so let's do it only once */
    int res = json_serialize_to_buffer_r(value, NULL, 0, 1, num_buf);
    return res < 0 ? 0 : (size_t)(res + 1);
}

JSON_Status json_serialize_to_buffer_pretty(const JSON_Value *value, char *buf, size_t buf_size_in_bytes) {
    int written = -1;
    size_t needed_size_in_bytes = json_serialization_size_pretty(value);
    if (needed_size_in_bytes == 0 || buf_size_in_bytes < needed_size_in_bytes) {
        return JSONFailure;
    }
    written = json_serialize_to_buffer_r(value, buf, 0, 1, NULL);
    if (written < 0) {
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_serialize_to_file_pretty(const JSON_Value *value, const char *filename) {
    JSON_Status return_code = JSONSuccess;
    FILE *fp = NULL;
    char *serialized_string = json_serialize_to_string_pretty(value);
    if (serialized_string == NULL) {
        return JSONFailure;
    }
    fp = fopen(filename, "w");
    if (fp == NULL) {
        json_free_serialized_string(serialized_string);
        return JSONFailure;
    }
    if (fputs(serialized_string, fp) == EOF) {
        return_code = JSONFailure;
    }
    if (fclose(fp) == EOF) {
        return_code = JSONFailure;
    }
    json_free_serialized_string(serialized_string);
    return return_code;
}

char * json_serialize_to_string_pretty(const JSON_Value *value) {
    JSON_Status serialization_result = JSONFailure;
    size_t buf_size_bytes = json_serialization_size_pretty(value);
    char *buf = NULL;
    if (buf_size_bytes == 0) {
        return NULL;
    }
    buf = (char*)parson_malloc(buf_size_bytes);
    if (buf == NULL) {
        return NULL;
    }
    serialization_result = json_serialize_to_buffer_pretty(value, buf, buf_size_bytes);
    if (serialization_result == JSONFailure) {
        json_free_serialized_string(buf);
        return NULL;
    }
    return buf;
}

void json_free_serialized_string(char *string) {
    parson_free(string);
}

JSON_Status json_array_remove(JSON_Array *array, size_t ix) {
    size_t to_move_bytes = 0;
    if (array == NULL || ix >= json_array_get_count(array)) {
        return JSONFailure;
    }
    json_value_free(json_array_get_value(array, ix));
    to_move_bytes = (json_array_get_count(array) - 1 - ix) * sizeof(JSON_Value*);
    memmove(array->items + ix, array->items + ix + 1, to_move_bytes);
    array->count -= 1;
    return JSONSuccess;
}

JSON_Status json_array_replace_value(JSON_Array *array, size_t ix, JSON_Value *value) {
    if (array == NULL || value == NULL || value->parent != NULL || ix >= json_array_get_count(array)) {
        return JSONFailure;
    }
    json_value_free(json_array_get_value(array, ix));
    value->parent = json_array_get_wrapping_value(array);
    array->items[ix] = value;
    return JSONSuccess;
}

JSON_Status json_array_replace_string(JSON_Array *array, size_t i, const char* string) {
    JSON_Value *value = json_value_init_string(string);
    if (value == NULL) {
        return JSONFailure;
    }
    if (json_array_replace_value(array, i, value) == JSONFailure) {
        json_value_free(value);
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_array_replace_number(JSON_Array *array, size_t i, double number) {
    JSON_Value *value = json_value_init_number(number);
    if (value == NULL) {
        return JSONFailure;
    }
    if (json_array_replace_value(array, i, value) == JSONFailure) {
        json_value_free(value);
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_array_replace_boolean(JSON_Array *array, size_t i, int boolean) {
    JSON_Value *value = json_value_init_boolean(boolean);
    if (value == NULL) {
        return JSONFailure;
    }
    if (json_array_replace_value(array, i, value) == JSONFailure) {
        json_value_free(value);
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_array_replace_null(JSON_Array *array, size_t i) {
    JSON_Value *value = json_value_init_null();
    if (value == NULL) {
        return JSONFailure;
    }
    if (json_array_replace_value(array, i, value) == JSONFailure) {
        json_value_free(value);
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_array_clear(JSON_Array *array) {
    size_t i = 0;
    if (array == NULL) {
        return JSONFailure;
    }
    for (i = 0; i < json_array_get_count(array); i++) {
        json_value_free(json_array_get_value(array, i));
    }
    array->count = 0;
    return JSONSuccess;
}

JSON_Status json_array_append_value(JSON_Array *array, JSON_Value *value) {
    if (array == NULL || value == NULL || value->parent != NULL) {
        return JSONFailure;
    }
    return json_array_add(array, value);
}

JSON_Status json_array_append_string(JSON_Array *array, const char *string) {
    JSON_Value *value = json_value_init_string(string);
    if (value == NULL) {
        return JSONFailure;
    }
    if (json_array_append_value(array, value) == JSONFailure) {
        json_value_free(value);
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_array_append_number(JSON_Array *array, double number) {
    JSON_Value *value = json_value_init_number(number);
    if (value == NULL) {
        return JSONFailure;
    }
    if (json_array_append_value(array, value) == JSONFailure) {
        json_value_free(value);
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_array_append_boolean(JSON_Array *array, int boolean) {
    JSON_Value *value = json_value_init_boolean(boolean);
    if (value == NULL) {
        return JSONFailure;
    }
    if (json_array_append_value(array, value) == JSONFailure) {
        json_value_free(value);
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_array_append_null(JSON_Array *array) {
    JSON_Value *value = json_value_init_null();
    if (value == NULL) {
        return JSONFailure;
    }
    if (json_array_append_value(array, value) == JSONFailure) {
        json_value_free(value);
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_object_set_value(JSON_Object *object, const char *name, JSON_Value *value) {
    size_t i = 0;
    JSON_Value *old_value;
    if (object == NULL || name == NULL || value == NULL || value->parent != NULL) {
        return JSONFailure;
    }
    old_value = json_object_get_value(object, name);
    if (old_value != NULL) { /* free and overwrite old value */
        json_value_free(old_value);
        for (i = 0; i < json_object_get_count(object); i++) {
            if (strcmp(object->names[i], name) == 0) {
                value->parent = json_object_get_wrapping_value(object);
                object->values[i] = value;
                return JSONSuccess;
            }
        }
    }
    /* add new key value pair */
    return json_object_add(object, name, value);
}

JSON_Status json_object_set_string(JSON_Object *object, const char *name, const char *string) {
    return json_object_set_value(object, name, json_value_init_string(string));
}

JSON_Status json_object_set_number(JSON_Object *object, const char *name, double number) {
    return json_object_set_value(object, name, json_value_init_number(number));
}

JSON_Status json_object_set_boolean(JSON_Object *object, const char *name, int boolean) {
    return json_object_set_value(object, name, json_value_init_boolean(boolean));
}

JSON_Status json_object_set_null(JSON_Object *object, const char *name) {
    return json_object_set_value(object, name, json_value_init_null());
}

JSON_Status json_object_dotset_value(JSON_Object *object, const char *name, JSON_Value *value) {
    const char *dot_pos = NULL;
    char *current_name = NULL;
    JSON_Object *temp_obj = NULL;
    JSON_Value *new_value = NULL;
    if (object == NULL || name == NULL || value == NULL) {
        return JSONFailure;
    }
    dot_pos = strchr(name, '.');
    if (dot_pos == NULL) {
        return json_object_set_value(object, name, value);
    } else {
        current_name = parson_strndup(name, dot_pos - name);
        temp_obj = sdl_json_object_get_object(object, current_name);
        if (temp_obj == NULL) {
            new_value = json_value_init_object();
            if (new_value == NULL) {
                parson_free(current_name);
                return JSONFailure;
            }
            if (json_object_add(object, current_name, new_value) == JSONFailure) {
                json_value_free(new_value);
                parson_free(current_name);
                return JSONFailure;
            }
            temp_obj = sdl_json_object_get_object(object, current_name);
        }
        parson_free(current_name);
        return json_object_dotset_value(temp_obj, dot_pos + 1, value);
    }
}

JSON_Status json_object_dotset_string(JSON_Object *object, const char *name, const char *string) {
    JSON_Value *value = json_value_init_string(string);
    if (value == NULL) {
        return JSONFailure;
    }
    if (json_object_dotset_value(object, name, value) == JSONFailure) {
        json_value_free(value);
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_object_dotset_number(JSON_Object *object, const char *name, double number) {
    JSON_Value *value = json_value_init_number(number);
    if (value == NULL) {
        return JSONFailure;
    }
    if (json_object_dotset_value(object, name, value) == JSONFailure) {
        json_value_free(value);
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_object_dotset_boolean(JSON_Object *object, const char *name, int boolean) {
    JSON_Value *value = json_value_init_boolean(boolean);
    if (value == NULL) {
        return JSONFailure;
    }
    if (json_object_dotset_value(object, name, value) == JSONFailure) {
        json_value_free(value);
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_object_dotset_null(JSON_Object *object, const char *name) {
    JSON_Value *value = json_value_init_null();
    if (value == NULL) {
        return JSONFailure;
    }
    if (json_object_dotset_value(object, name, value) == JSONFailure) {
        json_value_free(value);
        return JSONFailure;
    }
    return JSONSuccess;
}

JSON_Status json_object_remove(JSON_Object *object, const char *name) {
    size_t i = 0, last_item_index = 0;
    if (object == NULL || json_object_get_value(object, name) == NULL) {
        return JSONFailure;
    }
    last_item_index = json_object_get_count(object) - 1;
    for (i = 0; i < json_object_get_count(object); i++) {
        if (strcmp(object->names[i], name) == 0) {
            parson_free(object->names[i]);
            json_value_free(object->values[i]);
            if (i != last_item_index) { /* Replace key value pair with one from the end */
                object->names[i] = object->names[last_item_index];
                object->values[i] = object->values[last_item_index];
            }
            object->count -= 1;
            return JSONSuccess;
        }
    }
    return JSONFailure; /* No execution path should end here */
}

JSON_Status json_object_dotremove(JSON_Object *object, const char *name) {
    const char *dot_pos = strchr(name, '.');
    char *current_name = NULL;
    JSON_Object *temp_obj = NULL;
    if (dot_pos == NULL) {
        return json_object_remove(object, name);
    } else {
        current_name = parson_strndup(name, dot_pos - name);
        temp_obj = sdl_json_object_get_object(object, current_name);
        parson_free(current_name);
        if (temp_obj == NULL) {
            return JSONFailure;
        }
        return json_object_dotremove(temp_obj, dot_pos + 1);
    }
}

JSON_Status json_object_clear(JSON_Object *object) {
    size_t i = 0;
    if (object == NULL) {
        return JSONFailure;
    }
    for (i = 0; i < json_object_get_count(object); i++) {
        parson_free(object->names[i]);
        json_value_free(object->values[i]);
    }
    object->count = 0;
    return JSONSuccess;
}

JSON_Status json_validate(const JSON_Value *schema, const JSON_Value *value) {
    JSON_Value *temp_schema_value = NULL, *temp_value = NULL;
    JSON_Array *schema_array = NULL, *value_array = NULL;
    JSON_Object *schema_object = NULL, *value_object = NULL;
    JSON_Value_Type schema_type = JSONError, value_type = JSONError;
    const char *key = NULL;
    size_t i = 0, count = 0;
    if (schema == NULL || value == NULL) {
        return JSONFailure;
    }
    schema_type = json_value_get_type(schema);
    value_type = json_value_get_type(value);
    if (schema_type != value_type && schema_type != JSONNull) { /* null represents all values */
        return JSONFailure;
    }
    switch (schema_type) {
        case JSONArray:
            schema_array = json_value_get_array(schema);
            value_array = json_value_get_array(value);
            count = json_array_get_count(schema_array);
            if (count == 0) {
                return JSONSuccess; /* Empty array allows all types */
            }
            /* Get first value from array, rest is ignored */
            temp_schema_value = json_array_get_value(schema_array, 0);
            for (i = 0; i < json_array_get_count(value_array); i++) {
                temp_value = json_array_get_value(value_array, i);
                if (json_validate(temp_schema_value, temp_value) == JSONFailure) {
                    return JSONFailure;
                }
            }
            return JSONSuccess;
        case JSONObject:
            schema_object = json_value_get_object(schema);
            value_object = json_value_get_object(value);
            count = json_object_get_count(schema_object);
            if (count == 0) {
                return JSONSuccess; /* Empty object allows all objects */
            } else if (json_object_get_count(value_object) < count) {
                return JSONFailure; /* Tested object mustn't have less name-value pairs than schema */
            }
            for (i = 0; i < count; i++) {
                key = json_object_get_name(schema_object, i);
                temp_schema_value = json_object_get_value(schema_object, key);
                temp_value = json_object_get_value(value_object, key);
                if (temp_value == NULL) {
                    return JSONFailure;
                }
                if (json_validate(temp_schema_value, temp_value) == JSONFailure) {
                    return JSONFailure;
                }
            }
            return JSONSuccess;
        case JSONString: case JSONNumber: case JSONBoolean: case JSONNull:
            return JSONSuccess; /* equality already tested before switch */
        case JSONError: default:
            return JSONFailure;
    }
}

int json_value_equals(const JSON_Value *a, const JSON_Value *b) {
    JSON_Object *a_object = NULL, *b_object = NULL;
    JSON_Array *a_array = NULL, *b_array = NULL;
    const char *a_string = NULL, *b_string = NULL;
    const char *key = NULL;
    size_t a_count = 0, b_count = 0, i = 0;
    JSON_Value_Type a_type, b_type;
    a_type = json_value_get_type(a);
    b_type = json_value_get_type(b);
    if (a_type != b_type) {
        return 0;
    }
    switch (a_type) {
        case JSONArray:
            a_array = json_value_get_array(a);
            b_array = json_value_get_array(b);
            a_count = json_array_get_count(a_array);
            b_count = json_array_get_count(b_array);
            if (a_count != b_count) {
                return 0;
            }
            for (i = 0; i < a_count; i++) {
                if (!json_value_equals(json_array_get_value(a_array, i),
                                       json_array_get_value(b_array, i))) {
                    return 0;
                }
            }
            return 1;
        case JSONObject:
            a_object = json_value_get_object(a);
            b_object = json_value_get_object(b);
            a_count = json_object_get_count(a_object);
            b_count = json_object_get_count(b_object);
            if (a_count != b_count) {
                return 0;
            }
            for (i = 0; i < a_count; i++) {
                key = json_object_get_name(a_object, i);
                if (!json_value_equals(json_object_get_value(a_object, key),
                                       json_object_get_value(b_object, key))) {
                    return 0;
                }
            }
            return 1;
        case JSONString:
            a_string = json_value_get_string(a);
            b_string = json_value_get_string(b);
            if (a_string == NULL || b_string == NULL) {
                return 0; /* shouldn't happen */
            }
            return strcmp(a_string, b_string) == 0;
        case JSONBoolean:
            return json_value_get_boolean(a) == json_value_get_boolean(b);
        case JSONNumber:
            return fabs(json_value_get_number(a) - json_value_get_number(b)) < 0.000001; /* EPSILON */
        case JSONError:
            return 1;
        case JSONNull:
            return 1;
        default:
            return 1;
    }
}

JSON_Value_Type sdl_json_type(const JSON_Value *value) {
    return json_value_get_type(value);
}

JSON_Object * sdl_json_object (const JSON_Value *value) {
    return json_value_get_object(value);
}

JSON_Array * json_array  (const JSON_Value *value) {
    return json_value_get_array(value);
}

const char * json_string (const JSON_Value *value) {
    return json_value_get_string(value);
}

double json_number (const JSON_Value *value) {
    return json_value_get_number(value);
}

int json_boolean(const JSON_Value *value) {
    return json_value_get_boolean(value);
}

void json_set_allocation_functions(JSON_Malloc_Function malloc_fun, JSON_Free_Function free_fun) {
    parson_malloc = malloc_fun;
    parson_free = free_fun;
}

// 前方宣言
static void stack_print(void);

/* スタックデータの定義 */
struct stackdata{
    char data[MAX_DATA_SIZE]; /* 要素の格納先 */
};
typedef struct stackdata stackdata_t;

static int current_stacksize = 0;
static stackdata_t stack_list[10]; // 10個までしか持たないという意味

/*
 * @brief         スタックにデータを挿入する
 * @param[in/out] stk        スタック
 * @param[in]     push_data  挿入するデータ
 * @return        0          success
 * @return        -1         failure
 */
int stack_push(char* push_data)
{
    if( current_stacksize >= 10 ){
        printf("stack is max\n");
        return 0;
    }
    memset( stack_list[current_stacksize].data, 0x00, MAX_DATA_SIZE);
    memcpy( stack_list[current_stacksize].data, push_data, strlen(push_data) );
    current_stacksize++;
    
    //stack_print();
    return(0);
}

/*
 * @brief         スタックからデータを取得する
 * @param[in/out] stk        スタック
 * @param[out]    pop_data   挿入するデータ
 * @return        0          success
 * @return        -1         failure
 */
int stack_pop(char** pop_data)
{
    /* スタックが空でないかチェックする */
    if(current_stacksize < 1) {
        printf("stack is empty\n");
        return(-1);
    }

    current_stacksize--;
    *pop_data = stack_list[current_stacksize].data;
    return(0);
}

/* 
 * @brief     スタック内にある要素を一覧表示する
 * @param[in] stk        スタック
 */
static void stack_print()
{
    int i;
    for(i = 0; i < current_stacksize; i++){
        printf("stack_list[%d] : %s\n", i, stack_list[i].data);
    }
    
}

void stack_init()
{
    memset(stack_list, 0x00, sizeof(stack_list));
    current_stacksize = 0;
}

int get_stack_size()
{
    return current_stacksize;
}

/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/


static struct lws *web_socket = NULL;

//

// 単体デバッグ用.
// WebSocketに必要の無いJson文字列を加えたり、閲覧したりするので
// 単体デバッグ用以外の用途では利用しないこと.
// #define DEBUG_MODE
//#define DEBUG_BUILD_MACHINE

#ifdef DEBUG_MODE
static FILE *fp_sended_datafile = NULL;
static FILE *fp_received_datafile = NULL;

#ifdef DEBUG_BUILD_MACHINE
#define SENDED_FILEPATH_LOGFILE "./log_sended.txt"
#define RECEIVED_FILEPATH_LOGFILE "./log_received.txt"
#else
#define SENDED_FILEPATH_LOGFILE "/storage/log_sended.txt"
#define RECEIVED_FILEPATH_LOGFILE "/storage/log_received.txt"
#endif // DEBUG_BUILD_MACHINE

#endif // DEBUG_MODE

// 受信用RPC種別
#define RPC_VR                  0
#define RPC_TTS                 1
#define RPC_UI                  2
#define RPC_Navigation          3
#define RPC_VehicleInfo         4
#define RPC_RC                  5
#define RPC_Buttons             6
#define RPC_BasicCommunication  7
#define RPC_SDL                 8
#define RPC_TERMINATE           9 // これより下に追加してはならない

// 受信用RPC種別(method文字列)
#define RPC_VR_METHOD                   "VR."
#define RPC_TTS_METHOD                  "TTS."
#define RPC_UI_METHOD                   "UI."
#define RPC_Navigation_METHOD           "Navigation."
#define RPC_VehicleInfo_METHOD          "VehicleInfo."
#define RPC_RC_METHOD                   "RC."
#define RPC_Buttons_METHOD              "Buttons."
#define RPC_BasicCommunication_METHOD   "BasicCommunication."
#define RPC_SDL_METHOD                  "SDL."
#define RPC_TERMINATE_METHOD            ""// これより下に追加してはならない

/* 受信用Method名定義(共通)[Start] */
#define RPC_RECEIVE_IsReady                 "IsReady"
#define RPC_RECEIVE_GetCapabilities         "GetCapabilities"
#define RPC_RECEIVE_GetLanguage             "GetLanguage"
#define RPC_RECEIVE_GetSupportedLanguages   "GetSupportedLanguages"
#define RPC_RECEIVE_ChangeRegistration      "ChangeRegistration"
#define RPC_RECEIVE_AddCommand              "AddCommand"
/* 受信用Method名定義(共通)[End] */

/* 受信用Method名定義(BasicCommunication)[Start] */
#define RPC_RECEIVE_BasicCommunication_GetSystemInfo            "GetSystemInfo"
#define RPC_RECEIVE_BasicCommunication_MixingAudioSupported     "MixingAudioSupported"
#define RPC_RECEIVE_BasicCommunication_UpdateDeviceList         "UpdateDeviceList"
#define RPC_RECEIVE_BasicCommunication_OnAppRegistered          "OnAppRegistered"
#define RPC_RECEIVE_BasicCommunication_UpdateAppList            "UpdateAppList"
#define RPC_RECEIVE_BasicCommunication_PolicyUpdate             "PolicyUpdate"
#define RPC_RECEIVE_BasicCommunication_OnAppUnregistered        "OnAppUnregistered"
#define RPC_RECEIVE_BasicCommunication_ActivateApp              "ActivateApp"
/* 受信用Method名定義(BasicCommunication)[End] */

/* 受信用Method名定義(VehicleInfo)[Start] */
#define RPC_RECEIVE_VehicleInfo_GetVehicleData  "GetVehicleData"
#define RPC_RECEIVE_VehicleInfo_GetVehicleType  "GetVehicleType"
#define RPC_RECEIVE_VehicleInfo_SubscribeVehicleData  "SubscribeVehicleData"
#define RPC_RECEIVE_VehicleInfo_UnsubscribeVehicleData  "UnsubscribeVehicleData"
/* 受信用Method名定義(VehicleInfo)[End] */

/* 受信用Method名定義(TTS)[Start] */
#define RPC_RECEIVE_TTS_SetGlobalProperties  "SetGlobalProperties"
/* 受信用Method名定義(TTS)[End] */

/* 受信用Method名定義(Navigation)[Start] */
#define RPC_RECEIVE_Navigation_SetVideoConfig  "SetVideoConfig"
#define RPC_RECEIVE_Navigation_StartStream  "StartStream"
#define RPC_RECEIVE_Navigation_StartAudioStream  "StartAudioStream"
#define RPC_RECEIVE_Navigation_StopAudioStream  "StopAudioStream"
#define RPC_RECEIVE_Navigation_OnVideoDataStreaming  "OnVideoDataStreaming"
#define RPC_RECEIVE_Navigation_OnAudioDataStreaming  "OnAudioDataStreaming"
/* 受信用Method名定義(Navigation)[End] */

/* 受信用Method名定義(SDL)[Start] */
#define RPC_RECEIVE_SDL_OnStatusUpdate          "OnStatusUpdate"
#define RPC_RECEIVE_SDL_GetUserFriendlyMessage  "GetUserFriendlyMessage"
#define RPC_RECEIVE_SDL_GetURLS                 "GetURLS"
#define RPC_RECEIVE_SDL_ActivateApp             "ActivateApp"
/* 受信用Method名定義(SDL)[End] */

/* 受信用Method名定義(Buttons)[Start] */
#define RPC_RECEIVE_Buttons_OnButtonSubscription    "OnButtonSubscription"
/* 受信用Method名定義(Buttons)[End] */

/* 受信用Method名定義(UI)[Start] */
#define RPC_RECEIVE_UI_SetAppIcon    "SetAppIcon"
/* 受信用Method名定義(UI)[End] */

#define RPC_RECEIVE_UI_PerformAudioPassThru "PerformAudioPassThru"
#define RPC_RECEIVE_UI_EndAudioPassThru "EndAudioPassThru"
static unsigned int g_PerformAudioPassThru_request_id = 0;
static unsigned int g_PerformAudioPassThru_maxDuration = 0; //msec
static bool g_PerformAudioPassThru_running = false;

/* 受信用RPC種別と受信用RPC種別(method文字列)のペアテーブル[Start] */
typedef struct _RPC_PAIR {
    int rpc_type;
    char* rpc_method_str;
} RPC_PAIR;
const RPC_PAIR rpc_table[RPC_TERMINATE] = 
{
    { RPC_VR,                   RPC_VR_METHOD},
    { RPC_TTS,                  RPC_TTS_METHOD},
    { RPC_UI,                   RPC_UI_METHOD},
    { RPC_Navigation,           RPC_Navigation_METHOD},
    { RPC_VehicleInfo,          RPC_VehicleInfo_METHOD},
    { RPC_RC,                   RPC_RC_METHOD},
    { RPC_Buttons,              RPC_Buttons_METHOD},
    { RPC_BasicCommunication,   RPC_BasicCommunication_METHOD},
    { RPC_SDL,                  RPC_SDL_METHOD},
    { RPC_TERMINATE,            RPC_TERMINATE_METHOD}
};
/* RPC種別とMethod文字列のペアテーブル[End] */

// 送信用ID定義
#define SEND_registerComponent_VR_ID                    500
#define SEND_registerComponent_Navigation_ID            800
#define SEND_registerComponent_TTS_ID                   300
#define SEND_registerComponent_UI_ID                    400
#define SEND_registerComponent_Buttons_ID               200
#define SEND_registerComponent_VehicleInfo_ID           100
#define SEND_registerComponent_RC_ID                    900
#define SEND_registerComponent_BasicCommunication_ID    600

// 送信用ファイルパス
#ifdef DEBUG_BUILD_MACHINE
#define SEND_PATH_BUTTONS_GetCapabilities "./Buttons.GetCapabilities.txt"
#define SEND_PATH_UI_GetCapabilities "./UI.GetCapabilities.txt"
#define SEND_PATH_RC_GetCapabilities "./RC.GetCapabilities.txt"
#else
#define SEND_PATH_BUTTONS_GetCapabilities "/storage/Buttons.GetCapabilities.txt"
#define SEND_PATH_UI_GetCapabilities "/storage//UI.GetCapabilities.txt"
#define SEND_PATH_RC_GetCapabilities "./RC.GetCapabilities.txt"
#endif

// ***********************************************
// グローバル変数定義[Start]
// ***********************************************
static unsigned int g_Navigation_result_ID = 0;
static unsigned int g_Navigation_result_increment = 0;

static unsigned int g_UI_result_ID = 0;
static unsigned int g_UI_result_increment = 0;

static unsigned int g_Buttons_result_ID = 0;
static unsigned int g_Buttons_result_increment = 0;

static unsigned int g_BasicCommunication_result_ID = 0;
static unsigned int g_BasicCommunication_result_increment = 0;

static unsigned char g_buf[MAX_DATA_SIZE];
static unsigned int g_len = 0;
static enum lws_write_protocol g_protocol = 0;

/* アプリ起動関連[Start] */
static char g_deviceid[128];
static char g_name[24];
static char g_transportType[12];
static bool g_isSDLAllowed = true;

static int g_appID = 0;
static char g_status[64];
/* アプリ起動関連[End] */

// ***********************************************
// グローバル変数定義[End]
// ***********************************************


// ***********************************************
// Receive関数郡[Start]
// ***********************************************

// method文字列 (ex."VR.IsReady") からmethod名 (ex."IsReady") 部分だけを返却する
static char* getMethodStr(char* method){
    char* result_strstr = strstr(method, ".");
    result_strstr++;
    return result_strstr;
}

// method文字列からRPC種別を取得する
static int getRPCType(char* method){
    for(int i=0; i < RPC_TERMINATE; i++){
        char* result_strstr = strstr(method, rpc_table[i].rpc_method_str);
        if( result_strstr != NULL ){
            return rpc_table[i].rpc_type;
        }
    }
    return RPC_TERMINATE;
}

static void receive_rpc_vr(struct lws* wsi, unsigned int id, char* method, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
    char* method_str = getMethodStr(method);
    if( strncmp(method_str, RPC_RECEIVE_IsReady, sizeof(RPC_RECEIVE_IsReady)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_isReady(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_GetCapabilities, sizeof(RPC_RECEIVE_GetCapabilities)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_vr_GetCapabilities(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_GetLanguage, sizeof(RPC_RECEIVE_GetLanguage)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_GetLanguage(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_GetSupportedLanguages, sizeof(RPC_RECEIVE_GetSupportedLanguages)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_GetSupportedLanguages(wsi, id, rpctype);
    }
    // アプリ起動関連
    else if( strncmp(method_str, RPC_RECEIVE_ChangeRegistration, sizeof(RPC_RECEIVE_ChangeRegistration)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_ChangeRegistration(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_AddCommand, sizeof(RPC_RECEIVE_AddCommand)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_AddCommand(wsi, id, rpctype);
    }
}

static void receive_rpc_tts(struct lws* wsi, unsigned int id, char* method, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
    char* method_str = getMethodStr(method);
    if( strncmp(method_str, RPC_RECEIVE_IsReady, sizeof(RPC_RECEIVE_IsReady)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_isReady(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_GetLanguage, sizeof(RPC_RECEIVE_GetLanguage)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_GetLanguage(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_GetSupportedLanguages, sizeof(RPC_RECEIVE_GetSupportedLanguages)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_GetSupportedLanguages(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_GetCapabilities, sizeof(RPC_RECEIVE_GetCapabilities)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_tts_GetCapabilities(wsi, id, rpctype);
    }
    // アプリ起動関連
    else if( strncmp(method_str, RPC_RECEIVE_ChangeRegistration, sizeof(RPC_RECEIVE_ChangeRegistration)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_ChangeRegistration(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_TTS_SetGlobalProperties, sizeof(RPC_RECEIVE_TTS_SetGlobalProperties)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_tts_SetGlobalProperties(wsi, id);
    }
}

static void receive_rpc_ui(char* string, struct lws* wsi, unsigned int id, char* method, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
    char* method_str = getMethodStr(method);
    if( strncmp(method_str, RPC_RECEIVE_IsReady, sizeof(RPC_RECEIVE_IsReady)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_isReady(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_GetLanguage, sizeof(RPC_RECEIVE_GetLanguage)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_GetLanguage(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_GetSupportedLanguages, sizeof(RPC_RECEIVE_GetSupportedLanguages)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_GetSupportedLanguages(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_GetCapabilities, sizeof(RPC_RECEIVE_GetCapabilities)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_ui_GetCapabilities(wsi, id, rpctype);
    }
    // アプリ起動関連
    else if( strncmp(method_str, RPC_RECEIVE_ChangeRegistration, sizeof(RPC_RECEIVE_ChangeRegistration)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_ChangeRegistration(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_AddCommand, sizeof(RPC_RECEIVE_AddCommand)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_AddCommand(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_UI_SetAppIcon, sizeof(RPC_RECEIVE_UI_SetAppIcon)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_ui_SetAppIcon(wsi, id);
    }
    else if( strncmp(method_str, RPC_RECEIVE_UI_PerformAudioPassThru, sizeof(RPC_RECEIVE_UI_PerformAudioPassThru)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        //エラーケース以外では、PerformAudioPassThruはRequestに対するResponseをここで返してはいけない。
        //EndAudioPassThruに応答した後、またはタイムアウト時にResponseを返す。
        if (g_PerformAudioPassThru_running){
            //既に処理中のエラーレスポンスを用意する必要がある。
            send_rpc_ui_PerformAudioPassThru_error(wsi, id);
            return;
        }
        g_PerformAudioPassThru_request_id = id;
        g_PerformAudioPassThru_running = true;

        // 録音時間タイマ
        JSON_Value *schema = json_parse_string(string);
        g_PerformAudioPassThru_maxDuration = (unsigned int)json_object_dotget_number(sdl_json_object(schema), "params.maxDuration");
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s,duration:%d.\n", __func__, __LINE__, id, method,g_PerformAudioPassThru_maxDuration);
        json_value_free(schema);
        pthread_t pthread;
        pthread_create( &pthread, NULL, &PerformAudioPassThru_timerThread, wsi);
    }
    else if( strncmp(method_str, RPC_RECEIVE_UI_EndAudioPassThru, sizeof(RPC_RECEIVE_UI_EndAudioPassThru)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        //先に終了要求に応答し、続いてPerformAudioPassThruの結果を返す。
        if (g_PerformAudioPassThru_running){
            send_rpc_ui_EndAudioPassThru(wsi, id);
            sleep(1);
            send_rpc_ui_PerformAudioPassThru(wsi);
        } else {
            //PATが起動していない場合はエラー応答
            send_rpc_ui_EndAudioPassThru_error(wsi, id);
        }
    }
}

static void receive_rpc_navigation(struct lws* wsi, unsigned int id, char* method, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
    char* method_str = getMethodStr(method);
    if( strncmp(method_str, RPC_RECEIVE_IsReady, sizeof(RPC_RECEIVE_IsReady)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_isReady(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_GetLanguage, sizeof(RPC_RECEIVE_GetLanguage)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_GetLanguage(wsi, id, rpctype);
    }
    // 映像転送関連
    else if( strncmp(method_str, RPC_RECEIVE_Navigation_SetVideoConfig, sizeof(RPC_RECEIVE_Navigation_SetVideoConfig)) == 0){
        send_navigation_SetVideoConfig(wsi, id);
    }
    else if( strncmp(method_str, RPC_RECEIVE_Navigation_StartStream, sizeof(RPC_RECEIVE_Navigation_StartStream)) == 0){
        send_navigation_StartStream(wsi, id);
    }
    else if( strncmp(method_str, RPC_RECEIVE_Navigation_OnVideoDataStreaming, sizeof(RPC_RECEIVE_Navigation_OnVideoDataStreaming)) == 0){
        // 応答を受けて何かをするタイプのものではないため、なにもしない.
    }
    // 音声転送関連
    else if( strncmp(method_str, RPC_RECEIVE_Navigation_StartAudioStream, sizeof(RPC_RECEIVE_Navigation_StartAudioStream)) == 0){
        send_navigation_StartAudioStream(wsi, id);
    }
    else if( strncmp(method_str, RPC_RECEIVE_Navigation_StopAudioStream, sizeof(RPC_RECEIVE_Navigation_StopAudioStream)) == 0){
        send_navigation_StopAudioStream(wsi, id);
    }
    else if( strncmp(method_str, RPC_RECEIVE_Navigation_OnAudioDataStreaming, sizeof(RPC_RECEIVE_Navigation_OnAudioDataStreaming)) == 0){
        // 応答を受けて何かをするタイプのものではないため、なにもしない.
    }
}

static void receive_rpc_vehicleInfo(struct lws* wsi, unsigned int id, char* method, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
    char* method_str = getMethodStr(method);
    if( strncmp(method_str, RPC_RECEIVE_IsReady, sizeof(RPC_RECEIVE_IsReady)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_isReady(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_GetLanguage, sizeof(RPC_RECEIVE_GetLanguage)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_GetLanguage(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_VehicleInfo_GetVehicleData, sizeof(RPC_RECEIVE_VehicleInfo_GetVehicleData)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_VehicleInfo_GetVehicleData(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_VehicleInfo_GetVehicleType, sizeof(RPC_RECEIVE_VehicleInfo_GetVehicleType)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_VehicleInfo_GetVehicleType(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_VehicleInfo_SubscribeVehicleData, sizeof(RPC_RECEIVE_VehicleInfo_SubscribeVehicleData)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_VehicleInfo_SubscribeVehicleData(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_VehicleInfo_UnsubscribeVehicleData, sizeof(RPC_RECEIVE_VehicleInfo_UnsubscribeVehicleData)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_VehicleInfo_UnsubscribeVehicleData(wsi, id, rpctype);
    }
}

static void receive_rpc_rc(struct lws* wsi, unsigned int id, char* method, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
    char* method_str = getMethodStr(method);
    if( strncmp(method_str, RPC_RECEIVE_IsReady, sizeof(RPC_RECEIVE_IsReady)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_isReady(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_GetLanguage, sizeof(RPC_RECEIVE_GetLanguage)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_GetLanguage(wsi, id, rpctype);
    }
    else if( strncmp(method_str, RPC_RECEIVE_GetCapabilities, sizeof(RPC_RECEIVE_GetCapabilities)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_rc_GetCapabilities(wsi, id, rpctype);
        // システム起動後ならいつでもよいようなので、このタイミングでOnRemoteControlSettingsを送る.
        send_rpc_rc_OnRemoteControlSettings(wsi, id, rpctype);
    }
}

static void receive_rpc_buttons(struct lws* wsi, unsigned int id, char* method, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
    char* method_str = getMethodStr(method);
    if( strncmp(method_str, RPC_RECEIVE_GetCapabilities, strlen(RPC_RECEIVE_GetCapabilities)) == 0 ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_rpc_buttons_GetCapabilities(wsi, id, rpctype);
    }
}

static void receive_rpc_basiccommunication(struct lws* wsi, unsigned int id, char* method, int rpctype, char* string){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
    char* method_str = getMethodStr(method);
    if( strncmp(method_str, RPC_RECEIVE_BasicCommunication_GetSystemInfo, sizeof(RPC_RECEIVE_BasicCommunication_GetSystemInfo)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_BasicCommunication_GetSystemInfo(wsi, id);
    }
    else if( strncmp(method_str, RPC_RECEIVE_BasicCommunication_MixingAudioSupported, sizeof(RPC_RECEIVE_BasicCommunication_MixingAudioSupported)) == 0 ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_BasicCommunication_MixingAudioSupported(wsi, id);
        send_BasicCommunication_OnFindApplications(wsi);
    }
    // アプリ起動関連
    else if( strncmp(method_str, RPC_RECEIVE_BasicCommunication_UpdateDeviceList, sizeof(RPC_RECEIVE_BasicCommunication_UpdateDeviceList)) == 0 ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_BasicCommunication_UpdateDeviceList(wsi, id, string);
    }
    else if( strncmp(method_str, RPC_RECEIVE_BasicCommunication_OnAppRegistered, sizeof(RPC_RECEIVE_BasicCommunication_OnAppRegistered)) == 0 ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        // OnAppRegisteredに伴う応答はないが、パラメータを保持する必要がある.
        saveparam_BasicCommunication_OnAppRegistered(wsi, id, string);
    }
    else if( strncmp(method_str, RPC_RECEIVE_BasicCommunication_UpdateAppList, sizeof(RPC_RECEIVE_BasicCommunication_UpdateAppList)) == 0 ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_BasicCommunication_UpdateAppList(wsi, id, string);
    }
    else if( strncmp(method_str, RPC_RECEIVE_BasicCommunication_PolicyUpdate, sizeof(RPC_RECEIVE_BasicCommunication_PolicyUpdate)) == 0 ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_BasicCommunication_PolicyUpdate(wsi, id);
    }
    else if( strncmp(method_str, RPC_RECEIVE_BasicCommunication_OnAppUnregistered, sizeof(RPC_RECEIVE_BasicCommunication_OnAppUnregistered)) == 0 ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        // OnAppRegisteredに伴う応答はないが、パラメータを保持する必要がある.
        releaseparam_BasicCommunication_OnAppUnregistered(wsi, id, string);
    }
    else if( strncmp(method_str, RPC_RECEIVE_BasicCommunication_ActivateApp, sizeof(RPC_RECEIVE_BasicCommunication_ActivateApp)) == 0 ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        // OnAppRegisteredに伴う応答はないが、パラメータを保持する必要がある.
        send_BasicCommunication_ActivateApp(wsi, id);
    }
}

static void receive_rpc_sdl(struct lws* wsi, unsigned int id, char* method, int rpctype, char* string){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
    char* method_str = getMethodStr(method);
    if( strncmp(method_str, RPC_RECEIVE_SDL_OnStatusUpdate, sizeof(RPC_RECEIVE_SDL_OnStatusUpdate)) == 0){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        send_sdl_OnStatusUpdate(wsi, id, string);
    }
    else if( strncmp(method_str, RPC_RECEIVE_SDL_GetUserFriendlyMessage, sizeof(RPC_RECEIVE_SDL_GetUserFriendlyMessage)) == 0 ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        //応答を受けて何かをするタイプのものではないため、なにもしない.
    }
    else if( strncmp(method_str, RPC_RECEIVE_SDL_GetURLS, sizeof(RPC_RECEIVE_SDL_GetURLS)) == 0 ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        //応答を受けて何かをするタイプのものではないため、なにもしない.
    }
    else if( strncmp(method_str, RPC_RECEIVE_SDL_ActivateApp, sizeof(RPC_RECEIVE_SDL_ActivateApp)) == 0 ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d id:%d method:%s\n", __func__, __LINE__, id, method);
        //応答を受けて何かをするタイプのものではないため、なにもしない.
    }
}

// receive処理.
// personを使ってjson文字列を解読する関数.
static void receive_persistence(char* string, struct lws* wsi) {
    JSON_Value *schema = json_parse_string(string);
    char buf[256];

    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d schema: %s.\n", __func__, __LINE__, string);

    char* jsonrpc = sdl_json_object_get_string(sdl_json_object(schema), "jsonrpc");
    if( jsonrpc != NULL ) {
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d %s\n", __func__, __LINE__, jsonrpc);
    }
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    unsigned int id = (unsigned int)json_object_get_number(sdl_json_object(schema), "id");
    if( id != 0 ) {
        // json_object_get_number は存在しなければ0を返すので、上記のエラーチェックは必要
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d %d\n", __func__, __LINE__, id);
    }

    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    unsigned int result_numbar = (unsigned int)json_object_get_number(sdl_json_object(schema), "result");
    if( result_numbar != 0 ) { // "result":3000 などのパターン
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d %d\n", __func__, __LINE__, result_numbar);
    }

    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    char* result_ret = sdl_json_object_get_string(sdl_json_object(schema), "result");
    if( result_ret != NULL ) { // "result":"OK" などのパターン
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d %s\n", __func__, __LINE__, result_ret);
    }

    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    char* method = sdl_json_object_get_string(sdl_json_object(schema), "method");
    if( method != NULL ) { // "method":"VR.IsReady" などのパターン
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d %s\n", __func__, __LINE__, result_ret);
        int rpctype = getRPCType(method);
        switch(rpctype){
        case RPC_VR:
            receive_rpc_vr(wsi, id, method, rpctype);
            break;
        case RPC_TTS:
            receive_rpc_tts(wsi, id, method, rpctype);
            break;
        case RPC_UI:
            receive_rpc_ui(string, wsi, id, method, rpctype);
            break;
        case RPC_Navigation:
            receive_rpc_navigation(wsi, id, method, rpctype);
            break;
        case RPC_VehicleInfo:
            receive_rpc_vehicleInfo(wsi, id, method, rpctype);
            break;
        case RPC_RC:
            receive_rpc_rc(wsi, id, method, rpctype);
            break;
        case RPC_Buttons:
            receive_rpc_buttons(wsi, id, method, rpctype);
            break;
        case RPC_BasicCommunication:
            receive_rpc_basiccommunication(wsi, id, method, rpctype, string);
            break;
        case RPC_SDL:
            receive_rpc_sdl(wsi, id, method, rpctype, string);
            break;
        case RPC_TERMINATE:
        default:
            exit(0);
            break;
        }
    } 
    else if(result_numbar != 0) {
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
        /* registerComponentの応答[Start] */
        // IDの値を見て、なんの応答か判断する.
        // その後、次の要求を投げる.
        send_registerComponent(wsi,id, result_numbar); 
        /* registerComponentの応答[End] */
    }
    else if(result_ret != NULL) {
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
        /* subscribeToの応答[Start] */
        if( g_Navigation_result_increment != 0 ){
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d g_Navigation_result_ID=%d, g_Navigation_result_increment=%d\n", 
                __func__, __LINE__, g_Navigation_result_ID, g_Navigation_result_increment);
            send_subscribeTo_Navigation(wsi, id, result_ret);
        } 
        else if( g_UI_result_increment != 0 ){
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d g_Navigation_result_ID=%d, g_Navigation_result_increment=%d\n", 
                __func__, __LINE__, g_Navigation_result_ID, g_Navigation_result_increment);
            send_subscribeTo_UI(wsi, id, result_ret);
        }
        else if( g_Buttons_result_increment != 0 ){
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d g_UI_result_ID=%d, g_UI_result_increment=%d\n", 
                __func__, __LINE__, g_UI_result_ID, g_UI_result_increment);
            send_subscribeTo_Buttons(wsi, id, result_ret);
        }
        else if( g_BasicCommunication_result_increment != 0 ){
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d g_Buttons_result_ID=%d, g_Buttons_result_increment=%d\n", 
                __func__, __LINE__, g_UI_result_ID, g_UI_result_increment);
            send_subscribeTo_BasicCommunication(wsi, id, result_ret);
        }
        /* subscribeToの応答[End] */
    }
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);

    json_value_free(schema);
    return;
}

static void send_registerComponent(struct lws* wsi, unsigned int id, unsigned int result_numbar){
    if( id == SEND_registerComponent_VR_ID ){
        send_registerComponent_Navigation(wsi);
    }
    else if(id == SEND_registerComponent_TTS_ID){
        send_registerComponent_UI(wsi);
    }
    if(id == SEND_registerComponent_Navigation_ID){
        send_registerComponent_TTS(wsi);
        // subscribeTo用に保持.
        g_Navigation_result_ID = result_numbar;
    }
    else if(id == SEND_registerComponent_UI_ID){
        send_registerComponent_Buttons(wsi);
        // subscribeTo用に保持.
        g_UI_result_ID = result_numbar;
    }
    else if(id == SEND_registerComponent_Buttons_ID){
        send_registerComponent_VehicleInfo(wsi);
        g_Buttons_result_ID = result_numbar;
    }
    else if(id == SEND_registerComponent_VehicleInfo_ID){
        send_registerComponent_RC(wsi);
    }
    else if(id == SEND_registerComponent_RC_ID){
        send_registerComponent_BasicCommunication(wsi);
    }
    else if(id == SEND_registerComponent_BasicCommunication_ID){
        g_BasicCommunication_result_ID = result_numbar;
        
        // registerCompnentが終わったのでsubscribeToを流す.
        // インクリメントして送信.
        g_Navigation_result_increment++; 
        send_subscribeTo_Navigation(wsi, 0, NULL);
    }
}

static void send_subscribeTo_Navigation(struct lws* wsi, unsigned int id, char* result_ret){
    void (*func[2])(struct lws*) = {
        send_subscribeTo_Navigation_OnAudioDataStreaming,
        send_subscribeTo_Navigation_OnVideoDataStreaming,
    };
    
    // 関数呼び出し
    func[g_Navigation_result_increment-1](wsi);
    g_Navigation_result_increment++;
    if( 2 <= g_Navigation_result_increment ){
        g_Navigation_result_increment = 0;
        // 次のシーケンスを進めるためにインクリメントしておく.
        g_UI_result_increment++;
    }
}

static void send_subscribeTo_UI(struct lws* wsi, unsigned int id, char* result_ret){
    void (*func[1])(struct lws*) = {
        send_subscribeTo_UI_OnRecordStart,
    };
    
    // 関数呼び出し
    func[g_UI_result_increment-1](wsi);
    g_UI_result_increment++;
    if( 1 <= g_UI_result_increment ){
        g_UI_result_increment = 0;
        // 次のシーケンスを進めるためにインクリメントしておく.
        g_Buttons_result_increment++;
    }
}

static void send_subscribeTo_Buttons(struct lws* wsi, unsigned int id, char* result_ret){
    void (*func[1])(struct lws*) = {
        send_subscribeTo_Buttons_OnButtonSubscription,
    };
    
    // 関数呼び出し
    func[g_Buttons_result_increment-1](wsi);
    g_Buttons_result_increment++;
    if( 1 <= g_Buttons_result_increment ){
        g_Buttons_result_increment = 0;
        // 次のシーケンスを進めるためにインクリメントしておく.
        g_BasicCommunication_result_increment++;
    }
}


static void send_subscribeTo_BasicCommunication(struct lws* wsi, unsigned int id, char* result_ret){
    if( 11 <= g_BasicCommunication_result_increment ){
        return;
    }
    
    // 関数ポインタの配列
    void (*func[10])(struct lws*) = {
        send_subscribeTo_BasicCommunication_OnPutFile,
        send_subscribeTo_SDL_OnStatusUpdate,
        send_subscribeTo_SDL_OnAppPermissionChanged,
        send_subscribeTo_BasicCommunication_OnSDLPersistenceComplete,
        send_subscribeTo_BasicCommunication_OnFileRemoved,
        send_subscribeTo_BasicCommunication_OnAppRegistered,
        send_subscribeTo_BasicCommunication_OnAppUnregistered,
        send_subscribeTo_BasicCommunication_OnSDLClose,
        send_subscribeTo_SDL_OnSDLConsentNeeded,
        send_subscribeTo_BasicCommunication_OnResumeAudioSource
    };
    
    // 関数呼び出し
    func[g_BasicCommunication_result_increment-1](wsi);
    g_BasicCommunication_result_increment++;
    if( 11 <= g_BasicCommunication_result_increment ){
        
        // registerComponentが終わったのでOnReadyを通知するが
        // OnReadyはSDLコアから応答が無いので、応答を待ってシーケンスを流すことが出来ない。
        // libWebSocketsは同期で連続してwrite出来ないようなので、usleep(1000000)を行う.
        send_BasicCommunication_OnReady(wsi);
        //usleep(1000000);
    }
}

// ***********************************************
// Receive関数郡[End]
// ***********************************************

// ***********************************************
// Send関数郡[Start]
// ***********************************************

// @todo 適切な名前が浮かばなかった...
// write処理をまとめた関数.
static void do_lws_write(struct lws* wsi, char* serialized_string, JSON_Value *root_value){
    unsigned char buf[MAX_DATA_SIZE];
    memset(buf,'\0',MAX_DATA_SIZE);
    strncpy(buf, serialized_string, strlen(serialized_string));
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
    
    g_len = strlen(buf);
    g_protocol = LWS_WRITE_TEXT;
    memset(g_buf, '\0', MAX_DATA_SIZE);
    memcpy(g_buf, buf, g_len);
    
    stack_push(g_buf);
    
    lws_callback_on_writable( wsi );
}

/* 共通利用[Start] */
static void send_rpc_isReady(struct lws* wsi, unsigned int id, int rpctype) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_number(root_object, "id", id);
    json_object_set_string(root_object, "jsonrpc", "2.0");

    json_object_dotset_boolean(root_object, "result.available", 1);
    json_object_dotset_number(root_object, "result.code", 0); 
    
    switch(rpctype){
    case RPC_VR:
        json_object_dotset_string(root_object, "result.method", "VR.IsReady");
        break;
    case RPC_TTS:
        json_object_dotset_string(root_object, "result.method", "TTS.IsReady");
        break;
    case RPC_UI:
        json_object_dotset_string(root_object, "result.method", "UI.IsReady");
        break;
    case RPC_Navigation:
        json_object_dotset_string(root_object, "result.method", "Navigation.IsReady");
        break;
    case RPC_VehicleInfo:
        json_object_dotset_string(root_object, "result.method", "VehicleInfo.IsReady");
        break;
    case RPC_RC:
        json_object_dotset_string(root_object, "result.method", "RC.IsReady");
        break;
    default:
        exit(0);
        break;
    }
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
    
    const char *isBluetooth;
    isBluetooth = getenv("IS_BLUETOOTH");
    if( isBluetooth != NULL ){
        // BT対応
        if(rpctype == RPC_Navigation){
            start_OnStartDeviceDiscovery_thread();
        }
    }
}
static void send_rpc_GetSupportedLanguages(struct lws* wsi, unsigned int id, int rpctype) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);

    json_object_dotset_number(root_object, "result.code", 0); 
    
    switch(rpctype){
    case RPC_VR:
        json_object_dotset_string(root_object, "result.method", "VR.GetSupportedLanguages");
        break;
    case RPC_TTS:
        json_object_dotset_string(root_object, "result.method", "TTS.GetSupportedLanguages");
        break;
    case RPC_UI:
        json_object_dotset_string(root_object, "result.method", "UI.GetSupportedLanguages");
        break;
    default:
        exit(0);
        break;
    }
    json_object_dotset_string(root_object, "result.language", "EN-US");

    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_rpc_GetLanguage(struct lws* wsi, unsigned int id, int rpctype) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);

    json_object_dotset_number(root_object, "result.code", 0); 
    
    switch(rpctype){
    case RPC_VR:
        json_object_dotset_string(root_object, "result.method", "VR.GetLanguage");
        break;
    case RPC_TTS:
        json_object_dotset_string(root_object, "result.method", "TTS.GetLanguage");
        break;
    case RPC_UI:
        json_object_dotset_string(root_object, "result.method", "UI.GetLanguage");
        break;
    case RPC_Navigation:
        json_object_dotset_string(root_object, "result.method", "Navigation.GetLanguage");
        break;
    case RPC_VehicleInfo:
        json_object_dotset_string(root_object, "result.method", "VehicleInfo.GetLanguage");
        break;
    case RPC_RC:
        json_object_dotset_string(root_object, "result.method", "RC.GetLanguage");
        break;
    default:
        exit(0);
        break;
    }
    json_object_dotset_string(root_object, "result.language", "EN-US");

    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_rpc_ChangeRegistration(struct lws* wsi, unsigned int id, int rpctype) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);

    json_object_dotset_number(root_object, "result.code", 0); 
    
    switch(rpctype){
    case RPC_VR:
        json_object_dotset_string(root_object, "result.method", "VR.ChangeRegistration");
        break;
    case RPC_TTS:
        json_object_dotset_string(root_object, "result.method", "TTS.ChangeRegistration");
        break;
    case RPC_UI:
        json_object_dotset_string(root_object, "result.method", "UI.ChangeRegistration");
        break;
    default:
        exit(0);
        break;
    }
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_rpc_AddCommand(struct lws* wsi, unsigned int id, int rpctype) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);

    json_object_dotset_number(root_object, "result.code", 0); 
    
    switch(rpctype){
    case RPC_VR:
        json_object_dotset_string(root_object, "result.method", "VR.AddCommand");
        break;
    case RPC_UI:
        json_object_dotset_string(root_object, "result.method", "UI.AddCommand");
        break;
    default:
        exit(0);
        break;
    }
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}


/* 共通利用[End] */
static void send_ui_OnTouchEvent(struct lws* wsi, unsigned int val_x, unsigned int val_y, unsigned int id, unsigned int ts, char* type){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld wsi=%p\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), wsi);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_string(root_object, "method", "UI.OnTouchEvent");
    json_object_dotset_value(root_object, "params.event", "Navigation.SetVideoConfig");

    
#if 0
    /* devicelistのidを取得[Start] */
    JSON_Value *schema = json_parse_string(string);
    JSON_Array* devicelist = json_object_dotget_array(sdl_json_object(schema), "params.deviceList");
    for (int l = 0; l < json_array_get_count(devicelist); l++) {
        JSON_Object *link = json_array_get_object(devicelist, l);
        // deviceidの文字列を保持.
        char* ideviceid = sdl_json_object_get_string(link, "id");
        memset(g_deviceid, 0x00, sizeof(g_deviceid));
        memcpy(g_deviceid, ideviceid, strlen(ideviceid));

        char* iname = sdl_json_object_get_string(link, "name");
        memset(g_name, 0x00, sizeof(g_name));
        memcpy(g_name, iname, strlen(iname));

        char* itransportType = sdl_json_object_get_string(link, "transportType");
        memset(g_transportType, 0x00, sizeof(g_transportType));
        memcpy(g_transportType, itransportType, strlen(itransportType));

        g_isSDLAllowed = sdl_json_object_get_boolean(link, "isSDLAllowed");
    }
#endif
}

static void send_navigation_SetVideoConfig(struct lws* wsi, unsigned int id){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld wsi=%p\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), wsi);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "Navigation.SetVideoConfig");
    serialized_string = json_serialize_to_string_pretty(root_value);
    
    do_lws_write(web_socket, serialized_string, root_value);
}

static void send_navigation_StartAudioStream(struct lws* wsi, unsigned int id){
    lwsl_notice("%s:%d pid=%d, tid=%ld wsi=%p\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), wsi);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "Navigation.StartAudioStream");
    serialized_string = json_serialize_to_string_pretty(root_value);
    
    do_lws_write(web_socket, serialized_string, root_value);
}
static void send_navigation_StopAudioStream(struct lws* wsi, unsigned int id){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld wsi=%p\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), wsi);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "Navigation.StopAudioStream");
    serialized_string = json_serialize_to_string_pretty(root_value);
    
    do_lws_write(web_socket, serialized_string, root_value);
}

static void send_navigation_StartStream(struct lws* wsi, unsigned int id){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld wsi=%p\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), wsi);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "Navigation.StartStream");
    serialized_string = json_serialize_to_string_pretty(root_value);
    
    do_lws_write(web_socket, serialized_string, root_value);
}

void *OnFindApplications_thread(void *p) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld web_socket=%p\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), web_socket);

    while( 1 ) {
        JSON_Value *root_value = json_value_init_object();
        JSON_Object *root_object = json_value_get_object(root_value);
        char *serialized_string = NULL;
        json_object_set_string(root_object, "jsonrpc", "2.0");
        json_object_set_string(root_object, "method", "BasicCommunication.OnFindApplications");
        serialized_string = json_serialize_to_string_pretty(root_value);

        do_lws_write(web_socket, serialized_string, root_value);
        
        sleep(30);
    }
}

void *OnStartDeviceDiscovery_thread(void *p) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld web_socket=%p\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), web_socket);

    while( 1 ) {
        // export
        const char *bt_found_execute;
        bt_found_execute = getenv("BT_FOUND_EXECUTE");
        if( bt_found_execute != NULL ){
            send_BasicCommunication_OnStartDeviceDiscovery(web_socket);
            sleep(30);
        } else {
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d BTfound is oneshot", __func__, __LINE__, getpid(), syscall(SYS_gettid), web_socket);
            send_BasicCommunication_OnStartDeviceDiscovery(web_socket);
            return;
        }
    }
}

static void start_OnStartDeviceDiscovery_thread(){
    pthread_t pthread;
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld \n", __func__, __LINE__, getpid(), syscall(SYS_gettid));
    pthread_create( &pthread, NULL, &OnStartDeviceDiscovery_thread, NULL);
}

static void send_BasicCommunication_OnFindApplications(struct lws* wsi){
    pthread_t pthread;
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld wsi=%p\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), wsi);
    pthread_create( &pthread, NULL, &OnFindApplications_thread, NULL);
}

static void send_BasicCommunication_UpdateDeviceList(struct lws* wsi, unsigned int id, char* string){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld wsi=%p\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), wsi);
    // {"id":43,"jsonrpc":"2.0","method":"BasicCommunication.UpdateDeviceList",
    // "params":{"deviceList":[{"id":"c8624d25341699e297408b608797e42c342a62a97db8c7eb8bed2dd21468dd07","isSDLAllowed":true,"name":"192.168.1.53","transportType":"WIFI"}]}}
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    // UpdateDeviceList がきたら、OnDeviceRankChangedをまずは呼ぶ
    json_object_set_string(root_object, "method", "BasicCommunication.OnDeviceRankChanged");

    /* devicelistのidを取得[Start] */
    JSON_Value *schema = json_parse_string(string);
    JSON_Array* devicelist = json_object_dotget_array(sdl_json_object(schema), "params.deviceList");
    for (int l = 0; l < json_array_get_count(devicelist); l++) {
        JSON_Object *link = json_array_get_object(devicelist, l);
        // deviceidの文字列を保持.
        char* ideviceid = sdl_json_object_get_string(link, "id");
        memset(g_deviceid, 0x00, sizeof(g_deviceid));
        memcpy(g_deviceid, ideviceid, strlen(ideviceid));

        char* iname = sdl_json_object_get_string(link, "name");
        memset(g_name, 0x00, sizeof(g_name));
        memcpy(g_name, iname, strlen(iname));

        char* itransportType = sdl_json_object_get_string(link, "transportType");
        memset(g_transportType, 0x00, sizeof(g_transportType));
        memcpy(g_transportType, itransportType, strlen(itransportType));

        g_isSDLAllowed = sdl_json_object_get_boolean(link, "isSDLAllowed");
    }

    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d g_deviceid:%s\n", __func__, __LINE__, g_deviceid);
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d iname:%s\n", __func__, __LINE__, g_name);
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d g_transportType:%s\n", __func__, __LINE__, g_transportType);
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d g_isSDLAllowed:%d\n", __func__, __LINE__, g_isSDLAllowed);
    json_value_free(schema);
    /* devicelistのidを取得[End] */

    json_object_dotset_string(root_object, "params.device.id", g_deviceid);
    json_object_dotset_boolean(root_object, "params.device.isSDLAllowed", g_isSDLAllowed);
    json_object_dotset_string(root_object, "params.device.name", g_name);
    json_object_dotset_string(root_object, "params.device.transportType", g_transportType);
    json_object_dotset_string(root_object, "params.deviceRank", "DRIVER");
    serialized_string = json_serialize_to_string_pretty(root_value);
    do_lws_write(wsi, serialized_string, root_value);

    // 続けてupdateDeviceListをsendする.
    do_send_BasicCommunication_UpdateDeviceList(wsi, id);
}

static void do_send_BasicCommunication_UpdateDeviceList(struct lws* wsi, unsigned int id){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld wsi=%p\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), wsi);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_string(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "BasicCommunication.UpdateDeviceList");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_rpc_rc_OnRemoteControlSettings(struct lws*wsi, unsigned int id, int rpctype){
    
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_string(root_object, "method", "RC.OnRemoteControlSettings");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_rpc_VehicleInfo_GetVehicleData(struct lws*wsi, unsigned int id, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    
    // 定期通知用のスレッドが動いていない場合は、初期値を取得. 
    // getvehicledata_init == 1 の場合、初期値取得済み. 
    if ((vehicledata_thread != 0) && (getvehicledata_init == 0)) {
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d GetVehicleData : vehicledata_thread1 = %d \n", __func__, __LINE__, vehicledata_thread);
        // GetVehicleData向け初期値取得.
        char line_string[MAX_DATA_SIZE];
        memset(line_string,'\0',MAX_DATA_SIZE);
        // JSON形式 : sendstrings. 
        char sendstrings[MAX_DATA_SIZE];
        memset(sendstrings,'\0',MAX_DATA_SIZE);
        
        // 車両情報(ダミーデータ)を読み込み. 
        if( fp_can == NULL ) {
            fp_can = fopen("/storage/vehicledata.txt", "r");
            if (fp_can == NULL) {
                if(DEBUG_LOG_ENABLE)lwsl_notice("fp_can == NULL : vehicledata.txt open error.");
                return;
            }
        }
        
        while (true) {
            // fp_canから1行文字列を読み込み、line_stringに格納('\n'有). 
            if (fgets(line_string, MAX_DATA_SIZE, fp_can) == EOF) {
                parse_VehicleData_string(sendstrings);
                memset(sendstrings,'\0',MAX_DATA_SIZE);
                
                if(DEBUG_LOG_ENABLE)lwsl_notice("GetVehicleData : vehicledata.txt : read end.");
                break;
            } else {
                if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d test line_string : %s\n", __func__, __LINE__, line_string);
                if( strncmp("<End>", line_string, 5) == 0 ){
                    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d sendstrings===>%s\n", __func__, __LINE__, sendstrings);
                    // 初期値を保持する. 
                    parse_VehicleData_string(sendstrings);
                    memset(sendstrings,'\0',MAX_DATA_SIZE);
                    
                    // 定期通知のスレッドが動いていない場合. 
                    // ダミーデータを読み進めないため、フラグ管理する. 
                    getvehicledata_init = 1;
                    
                    if(DEBUG_LOG_ENABLE)lwsl_notice("GetVehicleData : vehicledata.txt : send read.");
                    break;
                } else if ( strncmp("FINISH_VEHICLE_DATA", line_string, 19) == 0 ){
                    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d sendstrings===>%s\n", __func__, __LINE__, sendstrings);
                    // 初期値を保持する. 
                    parse_VehicleData_string(sendstrings);
                    memset(sendstrings,'\0',MAX_DATA_SIZE);
                    
                    // 定期通知のスレッドが動いていない場合. 
                    // ダミーデータを読み進めないため、フラグ管理する. 
                    getvehicledata_init = 1;
                    
                    // 最終行のため、閉じる. 
                    fclose(fp_can);
                    fp_can = NULL;
                    
                    if(DEBUG_LOG_ENABLE)lwsl_notice("GetVehicleData : vehicledata.txt : finish read.");
                    break;
                }else if ( strncmp("<Start>", line_string, 7) == 0 ){
                    // なにもしない.
                    if(DEBUG_LOG_ENABLE)lwsl_notice("start tag read.\n");
                } else {
                    //if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d %s\n", __func__, __LINE__, line_string);
                    // JSON形式に文字列を変更するため、sendstringsに文字列連結. 
                    strncat(sendstrings, line_string, strlen(line_string));
                }
            }
        }
    }
    
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "VehicleInfo.GetVehicleData");
    
    // 車両情報(GSP). 
    json_object_dotset_number(root_object, "result.gps.longitudeDegrees", vehicledata_gps_longitudedegrees);
    json_object_dotset_number(root_object, "result.gps.latitudeDegrees", vehicledata_gps_latitudedegrees);
    
    json_object_dotset_number(root_object, "result.gps.utcYear", vehicledata_gps_utcyear);
    json_object_dotset_number(root_object, "result.gps.utcMonth", vehicledata_gps_utcmonth);
    json_object_dotset_number(root_object, "result.gps.utcDay", vehicledata_gps_utcday);
    json_object_dotset_number(root_object, "result.gps.utcHours", vehicledata_gps_utchours);
    json_object_dotset_number(root_object, "result.gps.utcMinutes", vehicledata_gps_utcminutes);
    json_object_dotset_number(root_object, "result.gps.utcSeconds", vehicledata_gps_utcseconds);
    
    json_object_dotset_string(root_object, "result.gps.compassDirection", vehicledata_gps_compassdirection);
    json_object_dotset_number(root_object, "result.gps.pdop", vehicledata_gps_pdop);
    json_object_dotset_number(root_object, "result.gps.hdop", vehicledata_gps_hdop);
    json_object_dotset_number(root_object, "result.gps.vdop", vehicledata_gps_vdop);
    
    json_object_dotset_boolean(root_object, "result.gps.actual", vehicledata_gps_actual);
    json_object_dotset_number(root_object, "result.gps.satellites", vehicledata_gps_satellites);
    json_object_dotset_string(root_object, "result.gps.dimension", vehicledata_gps_dimension);
    
    json_object_dotset_number(root_object, "result.gps.altitude", vehicledata_gps_altitude);
    json_object_dotset_number(root_object, "result.gps.heading", vehicledata_gps_heading);
    json_object_dotset_number(root_object, "result.gps.speed", vehicledata_gps_speed);
    
    // 車両情報(Speed). 
    json_object_dotset_number(root_object, "result.speed", vehicledata_speed);
    
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_rpc_VehicleInfo_GetVehicleType(struct lws* wsi, unsigned int id, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "VehicleInfo.GetVehicleType");
    json_object_dotset_string(root_object, "result.vehicleType.make", "Ford");
    json_object_dotset_string(root_object, "result.vehicleType.model", "Fiesta");
    json_object_dotset_string(root_object, "result.vehicleType.modelYear", "2013");
    json_object_dotset_string(root_object, "result.vehicleType.trim", "SE");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

void *OnVehicleData_thread(void *p) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld web_socket=%p\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), web_socket);
    
    if( web_socket == NULL ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("web_socket == NULL");
        return;
    }
    
    // 指定文字数分メモリ確保. 
    // 1行読み込み : line_string. 
    char line_string[MAX_DATA_SIZE];
    memset(line_string,'\0',MAX_DATA_SIZE);
    // JSON形式 : sendstrings. 
    char sendstrings[MAX_DATA_SIZE];
    memset(sendstrings,'\0',MAX_DATA_SIZE);
    
    // 車両情報(ダミーデータ)を読み込み. 
    if( fp_can == NULL ) {
        fp_can = fopen("/storage/vehicledata.txt", "r");
        if (fp_can == NULL) {
            if(DEBUG_LOG_ENABLE)lwsl_notice("fp_can == NULL : vehicledata.txt open error.");
            return;
        }
    }
    
    while (true) {
        if (vehicledata_thread != 0) {
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d vehicledata_thread cancel = %d \n", __func__, __LINE__, vehicledata_thread);
            break;
        }
        // fp_canから1行文字列を読み込み、line_stringに格納('\n'有). 
        if (fgets(line_string, MAX_DATA_SIZE, fp_can) == NULL) {
            if(DEBUG_LOG_ENABLE)lwsl_notice("vehicledata.txt : fgets == NULL.");
            // 読み取り終わったので、再度読み直し. 
            fclose(fp_can);
            fp_can = NULL;
            fp_can = fopen("/storage/vehicledata.txt", "r");
            continue;
        } else {
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d fgets != NULL : %s\n", __func__, __LINE__, line_string);
            if( strncmp("<End>", line_string, 5) == 0 ){
                if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d sendstrings===>%s\n", __func__, __LINE__, sendstrings);
                // SDLコアに送信. 
                do_lws_write_can(web_socket, sendstrings);
                usleep(100000);
                memset(sendstrings,'\0',MAX_DATA_SIZE);
                continue;
            } else if ( strncmp("<Start>", line_string, 7) == 0 ){
               // なにもしない.
                if(DEBUG_LOG_ENABLE)lwsl_notice("start tag read.\n");
            } else {
                //if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d %s\n", __func__, __LINE__, line_string);
                // JSON形式に文字列を変更するため、sendstringsに文字列連結. 
                strncat(sendstrings, line_string, strlen(line_string));
            }
        }
    }
}

static void send_rpc_VehicleInfo_SubscribeVehicleData(struct lws* wsi, unsigned int id, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    
    // FFW.VehicleInfo.SubscribeVehicleDataResponse -- From line 263 of file:///android_asset/sdl_hmi/ffw/VehicleInfoRPC.js
    // {"jsonrpc":"2.0","id":49,"result":{"accPedalPosition":{"dataType":"VEHICLEDATA_ACCPEDAL","resultCode":"SUCCESS"},"airbagStatus":{"dataType":"VEHICLEDATA_AIRBAGSTATUS","resultCode":"SUCCESS"},"beltStatus":{"dataType":"VEHICLEDATA_BELTSTATUS","resultCode":"SUCCESS"},"bodyInformation":{"dataType":"VEHICLEDATA_BODYINFO","resultCode":"SUCCESS"},"clusterModes":{"dataType":"VEHICLEDATA_CLUSTERMODESTATUS","resultCode":"SUCCESS"},"deviceStatus":{"dataType":"VEHICLEDATA_DEVICESTATUS","resultCode":"SUCCESS"},"driverBraking":{"dataType":"VEHICLEDATA_BRAKING","resultCode":"SUCCESS"},"eCallInfo":{"dataType":"VEHICLEDATA_ECALLINFO","resultCode":"SUCCESS"},"emergencyEvent":{"dataType":"VEHICLEDATA_EMERGENCYEVENT","resultCode":"SUCCESS"},"engineTorque":{"dataType":"VEHICLEDATA_ENGINETORQUE","resultCode":"SUCCESS"},"extendData1":{"dataType":"VEHICLEDATA_EXTENDDATA1","resultCode":"SUCCESS"},"extendData2":{"dataType":"VEHICLEDATA_EXTENDDATA2","resultCode":"SUCCESS"},"extendData3":{"dataType":"VEHICLEDATA_EXTENDDATA3","resultCode":"SUCCESS"},"extendData4":{"dataType":"VEHICLEDATA_EXTENDDATA4","resultCode":"SUCCESS"},"extendData5":{"dataType":"VEHICLEDATA_EXTENDDATA5","resultCode":"SUCCESS"},"externalTemperature":{"dataType":"VEHICLEDATA_EXTERNTEMP","resultCode":"VEHICLE_DATA_NOT_AVAILABLE"},"fuelLevel":{"dataType":"VEHICLEDATA_FUELLEVEL","resultCode":"SUCCESS"},"fuelLevel_State":{"dataType":"VEHICLEDATA_FUELLEVEL_STATE","resultCode":"SUCCESS"},"gps":{"dataType":"VEHICLEDATA_GPS","resultCode":"SUCCESS"},"headLampStatus":{"dataType":"VEHICLEDATA_HEADLAMPSTATUS","resultCode":"SUCCESS"},"instantFuelConsumption":{"dataType":"VEHICLEDATA_FUELCONSUMPTION","resultCode":"SUCCESS"},"myKey":{"dataType":"VEHICLEDATA_MYKEY","resultCode":"SUCCESS"},"odometer":{"dataType":"VEHICLEDATA_ODOMETER","resultCode":"SUCCESS"},"prndl":{"dataType":"VEHICLEDATA_PRNDL","resultCode":"SUCCESS"},"rpm":{"dataType":"VEHICLEDATA_RPM","resultCode":"SUCCESS"},"speed":{"dataType":"VEHICLEDATA_SPEED","resultCode":"SUCCESS"},"steeringWheelAngle":{"dataType":"VEHICLEDATA_STEERINGWHEEL","resultCode":"SUCCESS"},"tirePressure":{"dataType":"VEHICLEDATA_TIREPRESSURE","resultCode":"SUCCESS"},"wiperStatus":{"dataType":"VEHICLEDATA_WIPERSTATUS","resultCode":"SUCCESS"},"code":0,"method":"VehicleInfo.SubscribeVehicleData"}} -- From line 247 of file:///android_asset/sdl_hmi/ffw/RPCClient.js
    
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "VehicleInfo.SubscribeVehicleData");
    json_object_dotset_string(root_object, "result.gps.dataType", "VEHICLEDATA_GPS");
    json_object_dotset_string(root_object, "result.gps.resultCode", "SUCCESS");
    json_object_dotset_string(root_object, "result.speed.dataType", "VEHICLEDATA_SPEED");
    json_object_dotset_string(root_object, "result.speed.resultCode", "SUCCESS");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
    
    // 車両情報(ダミーデータ)送信スレッド生成. 
    if (vehicledata_thread != 0) {
        pthread_t pthread;
        //if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld wsi=%p\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), wsi);
        vehicledata_thread = pthread_create( &pthread, NULL, &OnVehicleData_thread, NULL);
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d vehicledata_thread = %d \n", __func__, __LINE__, vehicledata_thread);
    }
}

static void send_rpc_VehicleInfo_UnsubscribeVehicleData(struct lws* wsi, unsigned int id, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    
    // FFW.VehicleInfo.SubscribeVehicleDataResponse -- From line 263 of file:///android_asset/sdl_hmi/ffw/VehicleInfoRPC.js
    // {"jsonrpc":"2.0","id":49,"result":{"accPedalPosition":{"dataType":"VEHICLEDATA_ACCPEDAL","resultCode":"SUCCESS"},"airbagStatus":{"dataType":"VEHICLEDATA_AIRBAGSTATUS","resultCode":"SUCCESS"},"beltStatus":{"dataType":"VEHICLEDATA_BELTSTATUS","resultCode":"SUCCESS"},"bodyInformation":{"dataType":"VEHICLEDATA_BODYINFO","resultCode":"SUCCESS"},"clusterModes":{"dataType":"VEHICLEDATA_CLUSTERMODESTATUS","resultCode":"SUCCESS"},"deviceStatus":{"dataType":"VEHICLEDATA_DEVICESTATUS","resultCode":"SUCCESS"},"driverBraking":{"dataType":"VEHICLEDATA_BRAKING","resultCode":"SUCCESS"},"eCallInfo":{"dataType":"VEHICLEDATA_ECALLINFO","resultCode":"SUCCESS"},"emergencyEvent":{"dataType":"VEHICLEDATA_EMERGENCYEVENT","resultCode":"SUCCESS"},"engineTorque":{"dataType":"VEHICLEDATA_ENGINETORQUE","resultCode":"SUCCESS"},"extendData1":{"dataType":"VEHICLEDATA_EXTENDDATA1","resultCode":"SUCCESS"},"extendData2":{"dataType":"VEHICLEDATA_EXTENDDATA2","resultCode":"SUCCESS"},"extendData3":{"dataType":"VEHICLEDATA_EXTENDDATA3","resultCode":"SUCCESS"},"extendData4":{"dataType":"VEHICLEDATA_EXTENDDATA4","resultCode":"SUCCESS"},"extendData5":{"dataType":"VEHICLEDATA_EXTENDDATA5","resultCode":"SUCCESS"},"externalTemperature":{"dataType":"VEHICLEDATA_EXTERNTEMP","resultCode":"VEHICLE_DATA_NOT_AVAILABLE"},"fuelLevel":{"dataType":"VEHICLEDATA_FUELLEVEL","resultCode":"SUCCESS"},"fuelLevel_State":{"dataType":"VEHICLEDATA_FUELLEVEL_STATE","resultCode":"SUCCESS"},"gps":{"dataType":"VEHICLEDATA_GPS","resultCode":"SUCCESS"},"headLampStatus":{"dataType":"VEHICLEDATA_HEADLAMPSTATUS","resultCode":"SUCCESS"},"instantFuelConsumption":{"dataType":"VEHICLEDATA_FUELCONSUMPTION","resultCode":"SUCCESS"},"myKey":{"dataType":"VEHICLEDATA_MYKEY","resultCode":"SUCCESS"},"odometer":{"dataType":"VEHICLEDATA_ODOMETER","resultCode":"SUCCESS"},"prndl":{"dataType":"VEHICLEDATA_PRNDL","resultCode":"SUCCESS"},"rpm":{"dataType":"VEHICLEDATA_RPM","resultCode":"SUCCESS"},"speed":{"dataType":"VEHICLEDATA_SPEED","resultCode":"SUCCESS"},"steeringWheelAngle":{"dataType":"VEHICLEDATA_STEERINGWHEEL","resultCode":"SUCCESS"},"tirePressure":{"dataType":"VEHICLEDATA_TIREPRESSURE","resultCode":"SUCCESS"},"wiperStatus":{"dataType":"VEHICLEDATA_WIPERSTATUS","resultCode":"SUCCESS"},"code":0,"method":"VehicleInfo.SubscribeVehicleData"}} -- From line 247 of file:///android_asset/sdl_hmi/ffw/RPCClient.js
    
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "VehicleInfo.UnsubscribeVehicleData");
    json_object_dotset_string(root_object, "result.gps.dataType", "VEHICLEDATA_GPS");
    json_object_dotset_string(root_object, "result.gps.resultCode", "SUCCESS");
    json_object_dotset_string(root_object, "result.speed.dataType", "VEHICLEDATA_SPEED");
    json_object_dotset_string(root_object, "result.speed.resultCode", "SUCCESS");
    serialized_string = json_serialize_to_string_pretty(root_value);
    
    if (vehicledata_thread == 0) {
        vehicledata_thread = -1;
        pthread_cancel(&OnVehicleData_thread);
        if (fp_can != NULL) {
            fclose(fp_can);
            fp_can = NULL;
            getvehicledata_init = 0;
        }
        if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pthread_cancel \n", __func__, __LINE__);
    }
    
    do_lws_write(wsi, serialized_string, root_value);
}

static void parse_VehicleData_string(const char *string) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    //if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d test string : %s\n", __func__, __LINE__, string);
    
    // JSONオブジェクト生成. 
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    JSON_Value *schema = json_parse_string(string);
    
    // 車両情報(GPS). 
    vehicledata_gps_longitudedegrees = json_object_dotget_number(sdl_json_object(schema), "params.gps.longitudeDegrees");
    vehicledata_gps_latitudedegrees = json_object_dotget_number(sdl_json_object(schema), "params.gps.latitudeDegrees");
    
    vehicledata_gps_utcyear = json_object_dotget_number(sdl_json_object(schema), "params.gps.utcYear");
    vehicledata_gps_utcmonth = json_object_dotget_number(sdl_json_object(schema), "params.gps.utcMonth");
    vehicledata_gps_utcday = json_object_dotget_number(sdl_json_object(schema), "params.gps.utcDay");
    vehicledata_gps_utchours = json_object_dotget_number(sdl_json_object(schema), "params.gps.utcHours");
    vehicledata_gps_utcminutes = json_object_dotget_number(sdl_json_object(schema), "params.gps.utcMinutes");
    vehicledata_gps_utcseconds = json_object_dotget_number(sdl_json_object(schema), "params.gps.utcSeconds");
    
    vehicledata_gps_compassdirection = json_object_dotget_string(sdl_json_object(schema), "params.gps.compassDirection");
    
    vehicledata_gps_pdop = json_object_dotget_number(sdl_json_object(schema), "params.gps.pdop");
    vehicledata_gps_hdop = json_object_dotget_number(sdl_json_object(schema), "params.gps.hdop");
    vehicledata_gps_vdop = json_object_dotget_number(sdl_json_object(schema), "params.gps.vdop");
    
    vehicledata_gps_actual = json_object_dotget_boolean(sdl_json_object(schema), "params.gps.actual");
    vehicledata_gps_satellites = json_object_dotget_number(sdl_json_object(schema), "params.gps.satellites");
    vehicledata_gps_dimension  = json_object_dotget_string(sdl_json_object(schema), "params.gps.dimension");
    
    vehicledata_gps_altitude = json_object_dotget_number(sdl_json_object(schema), "params.gps.altitude");
    vehicledata_gps_heading = json_object_dotget_number(sdl_json_object(schema), "params.gps.heading");
    vehicledata_gps_speed = json_object_dotget_number(sdl_json_object(schema), "params.gps.speed");
    
    // 車両情報(Speed). 
    vehicledata_speed = json_object_dotget_number(sdl_json_object(schema), "params.speed");
}

static void do_lws_write_can(struct lws* wsi, char* serialized_string){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d : start \n", __func__, __LINE__);
    unsigned char buf[MAX_DATA_SIZE];
    memset(buf,'\0',MAX_DATA_SIZE);
    strncpy(buf, serialized_string, strlen(serialized_string));
    
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_string(root_object, "method", "BasicCommunication.OnSystemRequest");
    // 設定必須項目.
    json_object_dotset_string(root_object, "params.requestType", "NAVIGATION");
    json_object_dotset_string(root_object, "params.fileName", "/storage/NAVIGATION");
    // 以下は任意.
    json_object_dotset_string(root_object, "params.fileType", "JSON");
    json_object_dotset_number(root_object, "params.appID", g_appID);
    json_object_dotset_string(root_object, "params.url", (const char*)buf);

    char *iSerialized_string = NULL;
    iSerialized_string = json_serialize_to_string_pretty(root_value);

    char buf2[MAX_DATA_SIZE];
    memset(buf2,'\0',MAX_DATA_SIZE);
    strncpy(buf2, iSerialized_string, strlen(iSerialized_string));
    stack_push(buf2);
    
    lws_callback_on_writable( wsi );
}

static void send_rpc_vr_GetCapabilities(struct lws* wsi, unsigned int id, int rpctype) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    // {"jsonrpc":"2.0","id":21,"result":{"code":0,"method":"VR.GetCapabilities","vrCapabilities":["TEXT"]}}
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "VR.GetCapabilities");
    json_object_dotset_value(root_object, "result.vrCapabilities", json_parse_string("[\"TEXT\"]"));
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_rpc_buttons_GetCapabilities(struct lws* wsi, unsigned int id, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_set_number(root_object, "code", 0);
    json_object_set_string(root_object, "method", "Buttons.GetCapabilities");

    renketu_write(wsi, root_value, root_object, SEND_PATH_BUTTONS_GetCapabilities);

}

static void send_rpc_ui_GetCapabilities(struct lws* wsi, unsigned int id, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_set_number(root_object, "code", 0);
    json_object_set_string(root_object, "method", "UI.GetCapabilities");
    
    renketu_write(wsi, root_value, root_object, SEND_PATH_UI_GetCapabilities);
}

static void send_rpc_rc_GetCapabilities(struct lws* wsi, unsigned int id, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_set_number(root_object, "code", 0);
    json_object_set_string(root_object, "method", "RC.GetCapabilities");
    
    renketu_write(wsi, root_value, root_object, SEND_PATH_RC_GetCapabilities);

}

static void renketu_write(struct lws* wsi, JSON_Value *root_value, JSON_Object *root_object, char* filepath){
    char *serialized_string = NULL;
    serialized_string = json_serialize_to_string_pretty(root_value);

    // ここは力技で文字列を作成する.
    unsigned char tmpbuf[MAX_DATA_SIZE];
    memset(tmpbuf, '\0', MAX_DATA_SIZE);
    //ファイルを読み込みモードで開く
    FILE* fp_send = fopen(filepath,"r");
    if( fp_send == NULL ){
        if(DEBUG_LOG_ENABLE)lwsl_notice("fp_send open error");
        return 0;
    }
    fgets(tmpbuf, MAX_DATA_SIZE, fp_send);
    fclose(fp_send);

    char renketu[MAX_DATA_SIZE];
    memset(renketu, '\0', MAX_DATA_SIZE);

    // 末尾の文字"}"を終端文字に変更したバッファを別に容易.
    // そのままserialized_stringに余計なことすると、メモリ解放時に落ちる.
    char tmpbuf2[MAX_DATA_SIZE];
    memset(tmpbuf2, '\0', MAX_DATA_SIZE);
    memcpy(tmpbuf2, serialized_string, strlen(serialized_string) - 1);

    // Parsonで組み立てた文字列とファイルから読み出した文字列を連結して"}"で閉じる.
    snprintf(renketu, MAX_DATA_SIZE, "%s%s", tmpbuf2, tmpbuf);

    json_free_serialized_string(serialized_string);
    json_value_free(root_value);

    // write処理
    // 書き出し対象とメモリ解放処理が異なることからdo_lws_writeを利用できないため.
    g_len = strlen(renketu);
    g_protocol = LWS_WRITE_TEXT;
    memset(g_buf, '\0', MAX_DATA_SIZE);
    memcpy(g_buf, renketu, g_len);

    stack_push(g_buf);
    lws_callback_on_writable( wsi );
}

static void send_rpc_tts_GetCapabilities(struct lws* wsi, unsigned int id, int rpctype){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "TTS.GetCapabilities");
    json_object_dotset_value(root_object, "result.speechCapabilities", 
        json_parse_string("[\"TEXT\",\"PRE_RECORDED\"]"));
    json_object_dotset_value(root_object, "result.prerecordedSpeechCapabilities",
        json_parse_string("[\"HELP_JINGLE\",\"INITIAL_JINGLE\",\"LISTEN_JINGLE\",\"POSITIVE_JINGLE\",\"NEGATIVE_JINGLE\"]"));
    
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_BasicCommunication_MixingAudioSupported(struct lws* wsi, unsigned int id) {
    // {"id":18,"jsonrpc":"2.0","result":{"code":0,"attenuatedSupported":true,"method":"BasicCommunication.MixingAudioSupported"}}
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_boolean(root_object, "result.attenuatedSupported", 1);
    json_object_dotset_string(root_object, "result.method", "BasicCommunication.MixingAudioSupported");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_BasicCommunication_GetSystemInfo(struct lws* wsi, unsigned int id) {
    // {"jsonrpc":"2.0","id":1,"result":{"code":0,"method":"BasicCommunication.GetSystemInfo","ccpu_version":"ccpu_version","language":"EN-US","wersCountryCode":"wersCountryCode"}}
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "BasicCommunication.GetSystemInfo");
    json_object_dotset_string(root_object, "result.ccpu_version", "ccpu_version");
    json_object_dotset_string(root_object, "result.language", "EN-US");
    json_object_dotset_string(root_object, "result.wersCountryCode", "wersCountryCode");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);

}

/* BasicCommunication.OnReady の送信関数定義[Start] */
static void send_BasicCommunication_OnReady(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_string(root_object, "method", "BasicCommunication.OnReady");
    serialized_string = json_serialize_to_string_pretty(root_value);
    
    do_lws_write(wsi, serialized_string, root_value);

}
/* BasicCommunication.OnReady の送信関数定義[End] */

/* subscribeTo Navigation関連の定義[Start] */
static void send_subscribeTo_Navigation_OnAudioDataStreaming(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_Navigation_result_ID + g_Navigation_result_increment);
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "Navigation.OnAudioDataStreaming");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_subscribeTo_Navigation_OnVideoDataStreaming(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_Navigation_result_ID + g_Navigation_result_increment); 
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "Navigation.OnVideoDataStreaming");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}
/* subscribeTo Navigation関連の定義[End] */

/* subscribeTo UI関連の定義[Start] */
static void send_subscribeTo_UI_OnRecordStart(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_UI_result_ID + g_UI_result_increment); 
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "UI.OnRecordStart");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}
/* subscribeTo UI関連の定義[End] */


/* subscribeTo Buttons関連の定義[Start] */
static void send_subscribeTo_Buttons_OnButtonSubscription(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_Buttons_result_ID + g_Buttons_result_increment); 
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "Buttons.OnButtonSubscription");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}
/* subscribeTo Buttons関連の定義[Start] */

/* subscribeTo BasicCommunication関連の定義[Start] */
static void send_subscribeTo_BasicCommunication_OnPutFile(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_BasicCommunication_result_ID + g_BasicCommunication_result_increment);
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "BasicCommunication.OnPutFile");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_subscribeTo_BasicCommunication_OnSDLPersistenceComplete(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_BasicCommunication_result_ID + g_BasicCommunication_result_increment); 
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "BasicCommunication.OnSDLPersistenceComplete");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_subscribeTo_BasicCommunication_OnFileRemoved(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_BasicCommunication_result_ID + g_BasicCommunication_result_increment); 
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "BasicCommunication.OnFileRemoved");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_subscribeTo_BasicCommunication_OnAppRegistered(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_BasicCommunication_result_ID + g_BasicCommunication_result_increment); 
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "BasicCommunication.OnAppRegistered");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_subscribeTo_BasicCommunication_OnAppUnregistered(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_BasicCommunication_result_ID + g_BasicCommunication_result_increment); 
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "BasicCommunication.OnAppUnregistered");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_subscribeTo_BasicCommunication_OnSDLClose(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_BasicCommunication_result_ID + g_BasicCommunication_result_increment); 
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "BasicCommunication.OnSDLClose");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_subscribeTo_BasicCommunication_OnResumeAudioSource(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_BasicCommunication_result_ID + g_BasicCommunication_result_increment); 
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "BasicCommunication.OnResumeAudioSource");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}
/* subscribeTo BasicCommunication関連の定義[End] */

/* subscribeTo SDL関連の定義[Start] */
static void send_subscribeTo_SDL_OnSDLConsentNeeded(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    // SDLの場合もBasicCommunicationと同様のID定義で行う。インクリメントして送信.
    json_object_set_number(root_object, "id", g_BasicCommunication_result_ID + g_BasicCommunication_result_increment); 
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "SDL.OnSDLConsentNeeded");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_subscribeTo_SDL_OnStatusUpdate(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    // SDLの場合もBasicCommunicationと同様のID定義で行う。インクリメントして送信.
    json_object_set_number(root_object, "id", g_BasicCommunication_result_ID + g_BasicCommunication_result_increment); 
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "SDL.OnStatusUpdate");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_subscribeTo_SDL_OnAppPermissionChanged(struct lws* wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    // SDLの場合もBasicCommunicationと同様のID定義で行う。インクリメントして送信.
    json_object_set_number(root_object, "id", g_BasicCommunication_result_ID + g_BasicCommunication_result_increment); 
    json_object_set_string(root_object, "method", "MB.subscribeTo");
    json_object_dotset_string(root_object, "params.propertyName", "SDL.OnAppPermissionChanged");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}
/* subscribeTo SDL関連の定義[End] */

/* registerComponent の送信関数定義[Start] */
static void send_registerComponent_VR(struct lws *wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", SEND_registerComponent_VR_ID);
    json_object_set_string(root_object, "method", "MB.registerComponent");
    json_object_dotset_string(root_object, "params.componentName", "VR");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_registerComponent_Navigation(struct lws *wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", SEND_registerComponent_Navigation_ID);
    json_object_set_string(root_object, "method", "MB.registerComponent");
    json_object_dotset_string(root_object, "params.componentName", "Navigation");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_registerComponent_TTS(struct lws *wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", SEND_registerComponent_TTS_ID);
    json_object_set_string(root_object, "method", "MB.registerComponent");
    json_object_dotset_string(root_object, "params.componentName", "TTS");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_registerComponent_UI(struct lws *wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", SEND_registerComponent_UI_ID);
    json_object_set_string(root_object, "method", "MB.registerComponent");
    json_object_dotset_string(root_object, "params.componentName", "UI");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_registerComponent_Buttons(struct lws *wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", SEND_registerComponent_Buttons_ID);
    json_object_set_string(root_object, "method", "MB.registerComponent");
    json_object_dotset_string(root_object, "params.componentName", "Buttons");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_registerComponent_VehicleInfo(struct lws *wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", SEND_registerComponent_VehicleInfo_ID);
    json_object_set_string(root_object, "method", "MB.registerComponent");
    json_object_dotset_string(root_object, "params.componentName", "VehicleInfo");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_registerComponent_RC(struct lws *wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", SEND_registerComponent_RC_ID);
    json_object_set_string(root_object, "method", "MB.registerComponent");
    json_object_dotset_string(root_object, "params.componentName", "RC");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_registerComponent_BasicCommunication(struct lws *wsi) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", SEND_registerComponent_BasicCommunication_ID);
    json_object_set_string(root_object, "method", "MB.registerComponent");
    json_object_dotset_string(root_object, "params.componentName", "BasicCommunication");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

//@tobi
static void send_rpc_tts_SetGlobalProperties(struct lws* wsi, unsigned int id) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "TTS.SetGlobalProperties");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_rpc_ui_SetAppIcon(struct lws* wsi, unsigned int id){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "UI.SetAppIcon");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_rpc_ui_EndAudioPassThru(struct lws* wsi, unsigned int id){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "UI.EndAudioPassThru");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}
void *PerformAudioPassThru_timerThread(void *wsi) {
    //1000ms 未満は切り上げとする
    int sleepSec = (g_PerformAudioPassThru_maxDuration + 999) / 1000;
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld web_socket=%p,id=%d, sleepSec=%d, duration=%d.\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), web_socket, g_PerformAudioPassThru_request_id, sleepSec,g_PerformAudioPassThru_maxDuration);

    while( g_PerformAudioPassThru_running && sleepSec > 0 ) {
        sleep(1);
        sleepSec -= 1;
        if (sleepSec == 0){
            if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d pid=%d, tid=%ld web_socket=%p, %s\n", __func__, __LINE__, getpid(), syscall(SYS_gettid), web_socket, "PAT is Timeout.");
            //sleep中にEndを受けている可能性があるため再チェックして通知する。
            if (g_PerformAudioPassThru_running) {
                send_rpc_ui_PerformAudioPassThru((struct lws*)wsi);
            }
        }
    }
}

static void send_rpc_ui_EndAudioPassThru_error(struct lws* wsi, unsigned int id){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "error.code", 4);
    json_object_dotset_string(root_object, "error.message", "Rejected: no PerformAudioPassThru is now active");
    json_object_dotset_string(root_object, "error.data.method", "UI.EndAudioPassThru");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}
static void send_rpc_ui_PerformAudioPassThru(struct lws* wsi){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_PerformAudioPassThru_request_id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "UI.PerformAudioPassThru");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
    //応答を返したら初期化する。
    g_PerformAudioPassThru_request_id = 0;
    g_PerformAudioPassThru_running = false;
}
static void send_rpc_ui_PerformAudioPassThru_error(struct lws* wsi, unsigned int id){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "error.code", 4);
    json_object_dotset_string(root_object, "error.message", "already running PAT.");
    json_object_dotset_string(root_object, "error.data.method", "UI.PerformAudioPassThru");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void saveparam_BasicCommunication_OnAppRegistered(struct lws* wsi, unsigned int id, char* string){
    lwsl_notice("%s:%d \n", __func__, __LINE__);
    /*
    {
     "jsonrpc":"2.0",
     "method":"BasicCommunication.OnAppRegistered",
     "params":{
        "application":{
            "appID":545095898,
            "appName":"Hello Sdl",
            "appType":["NAVIGATION"],
            "deviceInfo":{
                "id":"c8624d25341699e297408b608797e42c342a62a97db8c7eb8bed2dd21468dd07",
                "isSDLAllowed":true,
                "name":"192.168.1.53",
                "transportType":"WIFI"
            },
            "hmiDisplayLanguageDesired":"EN-US",
            "icon":"",
            "isMediaApplication":false,
            "ngnMediaScreenAppName":"Hello Sdl",
            "policyAppID":"8675309",
            "requestType":[]
        },
     "priority":"NONE",
     "resumeVrGrammars":false,
     "vrSynonyms":["Hello Sdl"]
     }
    }
    */
    JSON_Value *schema = json_parse_string(string);
    // @todo 一つのアプリしかないからこのやり方でいいが、複数アプリへの対応となる場合、以下の管理では問題となる
    g_appID = json_object_dotget_number(sdl_json_object(schema), "params.application.appID");
    json_value_free(schema);
    
    sleep(1);
    send_sdl_ActivateApp(wsi);
}

static void releaseparam_BasicCommunication_OnAppUnregistered(struct lws* wsi, unsigned int id, char* string){
    lwsl_notice("%s:%d \n", __func__, __LINE__);
    // @todo 一つのアプリしかないからこのやり方でいいが、複数アプリへの対応となる場合、以下の管理では問題となる
    g_appID = 0;
}

static void send_BasicCommunication_UpdateAppList(struct lws* wsi, unsigned int id, char* string){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "BasicCommunication.UpdateAppList");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
    // アプリ終了時関連
    JSON_Value *schema = json_parse_string(string);
    JSON_Array* devicelist = json_object_dotget_array(sdl_json_object(schema), "params.applications");
    int count = json_array_get_count(devicelist);
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d count=%d\n", __func__, __LINE__, count);
    if( count == 0 ){
        send_BasicCommunication_OnAppDeactivated(wsi);
    }
}

static void send_BasicCommunication_OnAppDeactivated(struct lws* wsi){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    // "jsonrpc":"2.0","method":"BasicCommunication.OnAppDeactivated","params":{"appID":545095898}}
    if( g_appID != 0 ){
        JSON_Value *root_value = json_value_init_object();
        JSON_Object *root_object = json_value_get_object(root_value);
        char *serialized_string = NULL;
        json_object_set_string(root_object, "jsonrpc", "2.0");
        json_object_set_string(root_object, "method", "BasicCommunication.OnAppDeactivated");
        json_object_dotset_number(root_object, "params.appID", g_appID);
        serialized_string = json_serialize_to_string_pretty(root_value);

        do_lws_write(wsi, serialized_string, root_value);
    }
}


static void send_BasicCommunication_PolicyUpdate(struct lws* wsi, unsigned int id){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "BasicCommunication.PolicyUpdate");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
    
    // PolicyUpdateのタイミングで送信する.
    send_sdl_GetURLS(wsi);
}

static void send_BasicCommunication_ActivateApp(struct lws* wsi, unsigned int id){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", id);
    json_object_dotset_number(root_object, "result.code", 0);
    json_object_dotset_string(root_object, "result.method", "BasicCommunication.ActivateApp");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
    
    // PolicyUpdateのタイミングで送信する.
    send_sdl_GetURLS(wsi);
}

static void send_sdl_OnStatusUpdate(struct lws* wsi, unsigned int id, char* string){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);

    JSON_Value *schema = json_parse_string(string);
    // OnStatusUpdate そのものを返すわけではないが、パラメータを見て
    // {"jsonrpc":"2.0","method":"SDL.OnStatusUpdate","params":{"status":"UPDATE_NEEDED"}}
    
    char* istatus = json_object_dotget_string(sdl_json_object(schema), "params.status");
    memset(g_status, 0x00, sizeof(g_status));
    memcpy(g_status, istatus, strlen(istatus));
    
    send_sdl_GetUserFriendlyMessage(wsi);
    json_value_free(schema);
}

static void send_sdl_GetUserFriendlyMessage(struct lws* wsi){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_BasicCommunication_result_ID  + g_BasicCommunication_result_increment);
    g_BasicCommunication_result_increment++;
    json_object_set_string(root_object, "method", "SDL.GetUserFriendlyMessage");
    json_object_dotset_string(root_object, "params.language", "EN-US");
    if(strncmp(g_status, "UPDATE_NEEDED", sizeof("UPDATE_NEEDED")) == 0 ) {
        json_object_dotset_value(root_object, "params.messageCodes", json_parse_string("[\"StatusNeeded\"]"));
    }
    else if(strncmp(g_status, "UPDATING", sizeof("UPDATING")) == 0){
        json_object_dotset_value(root_object, "params.messageCodes", json_parse_string("[\"StatusPending\"]"));
        //send_sdl_ActivateApp(wsi);
    }
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

static void send_sdl_GetURLS(struct lws* wsi){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_BasicCommunication_result_ID  + g_BasicCommunication_result_increment);
    g_BasicCommunication_result_increment++;
    json_object_set_string(root_object, "method", "SDL.GetURLS");
    json_object_dotset_number(root_object, "params.service", 7);
    serialized_string = json_serialize_to_string_pretty(root_value);
}

static void send_sdl_ActivateApp(struct lws* wsi){
    //{"jsonrpc":"2.0","id":6014,"method":"SDL.ActivateApp","params":{"appID":545095898}}
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_number(root_object, "id", g_BasicCommunication_result_ID  + g_BasicCommunication_result_increment);
    g_BasicCommunication_result_increment++;
    json_object_set_string(root_object, "method", "SDL.ActivateApp");
    json_object_dotset_number(root_object, "params.appID", g_appID);
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

// CAN受信対応(HMIからSDLコアへの通知)
static void send_OnVehicleData(struct canfd_frame frame){
    if( web_socket == NULL ){
        return;
    }

    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_string(root_object, "method", "VehicleInfo.OnVehicleData");
    json_object_dotset_number(root_object, "params.speed", frame.data[0]);

    serialized_string = json_serialize_to_string_pretty(root_value);
    do_lws_write(web_socket, serialized_string, root_value);
}

static void send_BasicCommunication_illumi_OnSystemRequest(struct canfd_frame frame){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    if( web_socket == NULL ){
        return;
    }
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
/*
            "requestType":"HTTP",
            "fileType":"JSON",
            "offset":1000,
            "length":10000,
            "timeout":500,
            "fileName":"/home/tobinai/DIA/sdl_hmi/IVSU/PROPRIETARY_REQUEST",
            "url":"/home/tobinai/DIA/sdl_hmi/IVSU/PROPRIETARY_REQUEST",
            "appID":545095898
*/
    
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_string(root_object, "method", "BasicCommunication.OnSystemRequest");
    // 設定必須項目.
    json_object_dotset_string(root_object, "params.requestType", "NAVIGATION");
    json_object_dotset_string(root_object, "params.fileName", "/storage/NAVIGATION");
    // 以下は任意.
    json_object_dotset_string(root_object, "params.fileType", "JSON");
    json_object_dotset_number(root_object, "params.appID", g_appID);
    if( frame.data[0] == 0x00 ){
        json_object_dotset_string(root_object, "params.url", "Day");
    } else {
        json_object_dotset_string(root_object, "params.url", "Night");
    }

    serialized_string = json_serialize_to_string_pretty(root_value);
    do_lws_write(web_socket, serialized_string, root_value);
}

/* registerComponent の送信関数定義[End] */

static void send_BasicCommunication_OnStartDeviceDiscovery(struct lws* wsi){
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d \n", __func__, __LINE__);
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "jsonrpc", "2.0");
    json_object_set_string(root_object, "method", "BasicCommunication.OnStartDeviceDiscovery");
    serialized_string = json_serialize_to_string_pretty(root_value);

    do_lws_write(wsi, serialized_string, root_value);
}

/********************************************************************************************************************/
/********************************************************************************************************************/
/********************************************************************************************************************/
/********************************************************************************************************************/
// @tobi Websocket実装
#define EXAMPLE_RX_BUFFER_BYTES (MAX_DATA_SIZE)

static int callback_example( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
    //if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d reason=%d\n", __func__, __LINE__, reason);
	switch( reason )
	{
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
		    if(DEBUG_LOG_ENABLE)lwsl_notice("LWS_CALLBACK_CLIENT_ESTABLISHED wsi=%p user=%p in=%p len=%ld\n", wsi, user, in, len);
			send_registerComponent_VR(wsi);
			break;

		case LWS_CALLBACK_CLIENT_RECEIVE:
			/* Handle incomming messages here. */
			if(DEBUG_LOG_ENABLE)lwsl_notice("LWS_CALLBACK_CLIENT_RECEIVE wsi=%p user=%p in=%p len=%ld \n", wsi, user, in, len);
			if(DEBUG_LOG_ENABLE)lwsl_notice("LWS_CALLBACK_CLIENT_RECEIVE in=%s \n", in);
#ifdef DEBUG_MODE
            fp_received_datafile = fopen(RECEIVED_FILEPATH_LOGFILE,"a");
            if( fp_received_datafile == NULL ){
                if(DEBUG_LOG_ENABLE)lwsl_notice("fp_received_datafile open error");
                return 0;
            }
            fwrite(in, len, 1, fp_received_datafile);
            fclose(fp_received_datafile);
#endif
			receive_persistence(in, wsi);
			break;

		case LWS_CALLBACK_CLIENT_RECEIVE_PONG:
			/* Handle incomming messages here. */
			if(DEBUG_LOG_ENABLE)lwsl_notice("LWS_CALLBACK_CLIENT_RECEIVE wsi=%p user=%p in=%p len=%ld\n", wsi, user, in, len);
			if(DEBUG_LOG_ENABLE)lwsl_notice("LWS_CALLBACK_CLIENT_RECEIVE_PONG  in=%s \n", in);
			break;

		case LWS_CALLBACK_RECEIVE:
			/* Handle incomming messages here. */
			if(DEBUG_LOG_ENABLE)lwsl_notice("LWS_CALLBACK_RECEIVE wsi=%p user=%p in=%p len=%ld\n", wsi, user, in, len);
			if(DEBUG_LOG_ENABLE)lwsl_notice("LWS_CALLBACK_RECEIVE in=%s \n", (char*)in);
			break;

		case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
			/* Handle incomming messages here. */
			break;

		case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
			/* Handle incomming messages here. */
			break;

		case LWS_CALLBACK_CLIENT_WRITEABLE:
            // @tobi
            if(DEBUG_LOG_ENABLE)lwsl_notice("LWS_CALLBACK_CLIENT_WRITEABLE wsi=%p user=%p in=%p len=%ld\n", wsi, user, in, len);
            char* buf = NULL;
            stack_pop(&buf);
            if( buf == NULL ){
                break;
            }
            if(DEBUG_LOG_ENABLE)lwsl_notice("LWS_CALLBACK_CLIENT_WRITEABLE buf=%s \n", buf);
#ifdef DEBUG_MODE
            //ファイルを読み込みモードで開く
            fp_sended_datafile = fopen(SENDED_FILEPATH_LOGFILE,"a");
            if( fp_sended_datafile == NULL ){
                if(DEBUG_LOG_ENABLE)lwsl_notice("fp_sended_datafile open error");
                return 0;
            }
            fwrite(buf, strlen(buf), 1, fp_sended_datafile);
            fputs("\n", fp_sended_datafile);
            fclose(fp_sended_datafile);
#endif
			lws_write(wsi, buf, strlen(buf), g_protocol);
			if( get_stack_size() != 0 ){
                lws_callback_on_writable( wsi );
            }

            break;

		case LWS_CALLBACK_CLOSED:
            if(DEBUG_LOG_ENABLE)lwsl_notice("LWS_CALLBACK_CLOSED wsi=%p user=%p in=%p len=%ld\n", wsi, user, in, len);
            web_socket = NULL;
            g_Navigation_result_ID = 0;
            g_Navigation_result_increment = 0;
            g_UI_result_ID = 0;
            g_UI_result_increment = 0;
            g_Buttons_result_ID = 0;
            g_Buttons_result_increment = 0;
            g_BasicCommunication_result_ID = 0;
            g_BasicCommunication_result_increment = 0;
            break;

		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		    if(DEBUG_LOG_ENABLE)lwsl_notice("LWS_CALLBACK_CLIENT_CONNECTION_ERROR wsi=%p user=%p in=%p len=%ld\n", wsi, user, in, len);
			web_socket = NULL;
            g_Navigation_result_ID = 0;
            g_Navigation_result_increment = 0;
            g_UI_result_ID = 0;
            g_UI_result_increment = 0;
            g_Buttons_result_ID = 0;
            g_Buttons_result_increment = 0;
            g_BasicCommunication_result_ID = 0;
            g_BasicCommunication_result_increment = 0;
			break;

		default:
			break;
	}

	return 0;
}

enum protocols
{
	PROTOCOL_EXAMPLE = 0,
	PROTOCOL_COUNT
};

static struct lws_protocols protocols[] =
{
	{
		"example-protocol",
		callback_example,
		0,
		EXAMPLE_RX_BUFFER_BYTES,
	},
	{ NULL, NULL, 0, 0 } /* terminator */
};

void *func_thread(void *p) {
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[Websocket][Start] pid=%d, tid=%ld [end] \n", __func__, __LINE__, getpid(), syscall(SYS_gettid));
	struct lws_context_creation_info info;
	memset( &info, 0, sizeof(info) );

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	struct lws_context *context = lws_create_context( &info );

	while (running)
	{
		/* Connect if we are not connected to the server. */
		if( !web_socket )
		{
			struct lws_client_connect_info ccinfo = {0};
			ccinfo.context = context;
			ccinfo.address = "localhost";
			ccinfo.port = 8087;
			ccinfo.path = "/";
			ccinfo.host = lws_canonical_hostname( context );
			ccinfo.origin = "origin";
			ccinfo.protocol = protocols[PROTOCOL_EXAMPLE].name;
			web_socket = lws_client_connect_via_info(&ccinfo);
		}

		/* Send a random number to the server every second. */
		//lws_callback_on_writable( web_socket );
        usleep(50000);
		lws_service( context, /* timeout_ms = */ 1 );
	}
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d[Websocket][End] pid=%d, tid=%ld [end] \n", __func__, __LINE__, getpid(), syscall(SYS_gettid));
	lws_context_destroy( context );
}

void websocket_init()
{
    pthread_t pthread;
    pthread_t pthread_can;
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d\n",__func__, __LINE__);

    stack_init();

    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d\n",__func__, __LINE__);
#ifdef DEBUG_MODE
    if( remove( SENDED_FILEPATH_LOGFILE ) == 0 ){
        printf( "%sファイルを削除しました\n", SENDED_FILEPATH_LOGFILE );
    }
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d\n",__func__, __LINE__);
    if( remove( RECEIVED_FILEPATH_LOGFILE ) == 0 ){
        printf( "%sファイルを削除しました\n", RECEIVED_FILEPATH_LOGFILE );
    }
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d\n",__func__, __LINE__);
#endif
// 車両情報(ダミーデータ)には、不必要であるため. 
//    pthread_create( &pthread_can, NULL, &can_thread, NULL);
//    printf("%s:%d\n",__func__, __LINE__);

#if 1
    pthread_create( &pthread, NULL, &func_thread, NULL);
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s:%d\n",__func__, __LINE__);
#endif
    if(DEBUG_LOG_ENABLE)lwsl_notice("%s main pid=%d, tid=%ld [end] \n", __func__, getpid(), syscall(SYS_gettid));


    //while( 1 ){}
}

// touch_event
void lws_touch_handle_down(uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
    if (id != 0) {
        return;
    }
    //if(DEBUG_LOG_ENABLE)lwsl_notice("%s main pid=%d, tid=%ld [start] \n", __func__, getpid(), syscall(SYS_gettid));
    
    // Message to convert: protocol 0; json {"jsonrpc":"2.0","method":"UI.OnTouchEvent","params":{"event":[{"c":[{"x":208,"y":139}],"id":0,"ts":[24022]}],"type":"BEGIN"}}
    // {"jsonrpc":"2.0","method":"UI.OnTouchEvent","params":{"event":[{"c":[{"x":208,"y":139}],"id":0,"ts":[24022]}],"type":"BEGIN"}}
    double cx = wl_fixed_to_double(x_w);
    double cy = wl_fixed_to_double(y_w);
//    GST_ERROR ("tobi: %s: cx=%f cy=%f", __func__, cx, cy);
    
    char buf[MAX_DATA_SIZE];
    memset(buf, 0x00, sizeof(buf));
    
    char* json_word_00 = "{\"jsonrpc\":\"2.0\",\"method\":\"UI.OnTouchEvent\",\"params\":{\"event\":[{\"c\":[{\"x\":";
    char* json_word_01 = ",\"y\":";
    char* json_word_02 = "}],\"id\":";
    char* json_word_03 = ",\"ts\":[";
    char* json_word_04 = "]}],\"type\":\"BEGIN\"";
    snprintf(buf,MAX_DATA_SIZE,"%s%d%s%d%s%d%s%d%s}}\n", json_word_00, (int)cx, json_word_01, (int)cy, json_word_02, id, json_word_03, time, json_word_04);
    
    lws_touch_info[id][0] = (int)cx;
    lws_touch_info[id][1] = (int)cy;
    
    if(DEBUG_LOG_ENABLE)lwsl_notice("lws_touch_handle_down buf=%s \n", buf);
    lws_write( web_socket, buf, strlen(buf), LWS_WRITE_TEXT );
    
//    if(DEBUG_LOG_ENABLE)lwsl_notice("lws_touch_handle_down Complete \n");
}

void lws_touch_handle_up(uint32_t time, int32_t id)
{
    if (id != 0) {
        return;
    }
    //if(DEBUG_LOG_ENABLE)lwsl_notice("%s main pid=%d, tid=%ld [start] \n", __func__, getpid(), syscall(SYS_gettid));
    
    // Message to convert: protocol 0; json {"jsonrpc":"2.0","method":"UI.OnTouchEvent","params":{"event":[{"c":[{"x":208,"y":139}],"id":0,"ts":[24093]}],"type":"END"}}
    // {"jsonrpc":"2.0","method":"UI.OnTouchEvent","params":{"event":[{"c":[{"x":208,"y":139}],"id":0,"ts":[24093]}],"type":"END"}}
    char buf[MAX_DATA_SIZE];
    memset(buf, 0x00, sizeof(buf));
    
    char* json_word_00 = "{\"jsonrpc\":\"2.0\",\"method\":\"UI.OnTouchEvent\",\"params\":{\"event\":[{\"c\":[{\"x\":";
    char* json_word_01 = ",\"y\":";
    char* json_word_02 = "}],\"id\":";
    char* json_word_03 = ",\"ts\":[";
    char* json_word_04 = "]}],\"type\":\"END\"";
    snprintf(buf,MAX_DATA_SIZE,"%s%d%s%d%s%d%s%d%s}}\n", json_word_00, lws_touch_info[id][0], json_word_01, lws_touch_info[id][1], json_word_02, id, json_word_03, time, json_word_04);
    
    lws_touch_info[id][0] = 0;
    lws_touch_info[id][1] = 0;
    
    if(DEBUG_LOG_ENABLE)lwsl_notice("lws_touch_handle_up buf=%s \n", buf);
    lws_write( web_socket, buf, strlen(buf), LWS_WRITE_TEXT );
    
//    if(DEBUG_LOG_ENABLE)lwsl_notice("lws_touch_handle_up Complete \n");
}

void lws_touch_handle_motion(uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
    if (id != 0) {
        return;
    }
    //if(DEBUG_LOG_ENABLE)lwsl_notice("%s main pid=%d, tid=%ld [start] \n", __func__, getpid(), syscall(SYS_gettid));
    
    // Message to convert: protocol 0; json {"jsonrpc":"2.0","method":"UI.OnTouchEvent","params":{"event":[{"c":[{"x":208,"y":139}],"id":0,"ts":[24053]}],"type":"MOVE"}}
    // {"jsonrpc":"2.0","method":"UI.OnTouchEvent","params":{"event":[{"c":[{"x":208,"y":139}],"id":0,"ts":[24053]}],"type":"MOVE"}}
    double cx = wl_fixed_to_double(x_w);
    double cy = wl_fixed_to_double(y_w);
//    GST_ERROR ("tobi: %s: cx=%f cy=%f", __func__, cx, cy);
    
    char buf[MAX_DATA_SIZE];
    memset(buf, 0x00, sizeof(buf));
    
    char* json_word_00 = "{\"jsonrpc\":\"2.0\",\"method\":\"UI.OnTouchEvent\",\"params\":{\"event\":[{\"c\":[{\"x\":";
    char* json_word_01 = ",\"y\":";
    char* json_word_02 = "}],\"id\":";
    char* json_word_03 = ",\"ts\":[";
    char* json_word_04 = "]}],\"type\":\"MOVE\"";
    snprintf(buf,MAX_DATA_SIZE,"%s%d%s%d%s%d%s%d%s}}\n", json_word_00, (int)cx, json_word_01, (int)cy, json_word_02, id, json_word_03, time, json_word_04);
    
    lws_touch_info[id][0] = (int)cx;
    lws_touch_info[id][1] = (int)cy;
    
    if(DEBUG_LOG_ENABLE)lwsl_notice("lws_touch_handle_motion buf=%s \n", buf);
    lws_write( web_socket, buf, strlen(buf), LWS_WRITE_TEXT );
    
//    if(DEBUG_LOG_ENABLE)lwsl_notice("lws_touch_handle_motion Complete \n");
}
