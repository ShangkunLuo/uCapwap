#ifndef SHM_UTIL
#define SHM_UTIL
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>

#define DEBUG

#ifdef DEBUG
#define DBG(x...) printf(x)
#else
#define DBG(x...)
#endif

#define SYSKEY 8192

#ifdef CW_MAX_WTP
#define AP_SIZE CW_MAX_WTP
#else
#ifdef RTLSUPPORT
#define AP_SIZE 30
#else
#define AP_SIZE 1024
#endif
#endif

#ifdef WTP_RADIO_MAX
#define RADIOCNT WTP_RADIO_MAX
#else
#define RADIOCNT 5
#endif

#ifdef WTP_MAX_INTERFACES
#define WLANCNT WTP_MAX_INTERFACES
#else
#define WLANCNT 4
#endif

#ifdef WTP_MAX_STA
#define STACNT WTP_MAX_STA
#else
#define STACNT 128
#endif

#define ACLCNT STACNT
#define MAX_NEIGHBOR 64
#define ADDRSTRLEN 16
#define HVLEN 64
#define SVLEN 256
#define GRPLEN 32
#define LATLEN 32
#define DEVNAMELEN 32
#define MANUFACTURELEN 32
#define MAX_WEB_LOGIN 2
#define PATHLEN 256
#define HW_MODEL_MAX_NUM 32
/*******************************************************************************
 * use to setting config option: 
 * 0 is default;
 * 1 only config system, 
 * 2 only config wireless binding
*******************************************************************************/
enum {
        CO_DEFAULT = 0,
        CO_SYSTEM = 1,
        CO_WIRELESS_BIND = 2
};

enum {
        DFTCFG2G = 0,
        DFTCFG5G = 1
};

typedef struct {
        char type[2];//0:disable, 1:root, 2:normal
        char usrname[32];
        char passwd[32];
} weblogin;

typedef struct {
        char autocheck[2];//0:don't check, 1:check in join
        char protocol[2];//0:disable, 1:tftp,2:ftp,3:wget,4:capwap
        char name[SVLEN];//upgrade firmware
        char hwmodel[HVLEN];
        char swversion[SVLEN];
} sysupgrade;

/*******************************************************************************
 * rbtype:
 * 	0-disable 
 * 	1-hour 
 * 	2-days 
 * 	3-month 
 * 	4-week
 * rbtime index:
 * 	0:hours
 * 	1:days
 * 	2:month
 * 	3:week
 * reset:
 * 	0:keep config
 *  1:reset config
 * ****************************************************************************/
typedef struct {
        char hw_version[HVLEN];
        char sw_version[SVLEN];
        char groupid[4];
        char group[GRPLEN];
        char location[LATLEN];
        char devname[DEVNAMELEN];
        char manufacture[MANUFACTURELEN];
        char rbtype[2];
        char rbtime[4];
        weblogin login[MAX_WEB_LOGIN];
        char cfgopt[2];
        sysupgrade upgrade;
        char reset[4];       
        char rsvd[4][4];//reserved
} AP_TYPE;

typedef struct {
        char         id[4];
        char         bssid[32];
        char         mode[8];
        char         authmode[4];
        char         ciphertype[4];
		char         epport[8];//802.1x
        char         eapserver[20];//802.1x
        char         codeidx[4];//wep:0,1,2,3,other:0
        char         codefmt[4];//wep:0,1,other:0
        char         key[128];
        char         ssid[64];
        char         network[8];
        char         wmm[2];
        char         hidden[2];
        char         bintval[8];
        char         shortgi[2];
        char         frag[8];
        char         rts[8];
        char         maxsta[4];
        char         isolate[2];
        char         wds[2];
        char         extap[2];
        char         disabled[2];
        char         scantype[2];// 0:disable,1:nonperiodic,2:periodic
        char         period[8];//unit: second
        char         vlantag[8];
        //char       disablecoext[2];
        //char       uprate[8];//unit kbps
        //char       downrate[8];//unit kbps
} wIface;

typedef struct macacl {
        char config[2];//0:no, 1:yes
        char flag[2];//0:disable, 1:enable white acl, 2:enable black acl
        char cnt[4];
        char haddr[ACLCNT][32];
} macacl;

/*
config wifi-device 'wifi1'
        option type 'qcawifi'
        option country 'CN'
        option channel '153'
        option hwmode '11ac'
        option htmode 'HT80'
        option txpower '30'
        option rssi_thres '-90'
        option AMPDU '1'
        option acktimeout '64'
*/
typedef struct {
        char         cn[8];
        char         channel[4];
        char         hwmode[8];
        char         htmode[8];
        char         txpower[4];
        char         rssi_thres[8];
        char         AMPDU[2];
        char         acktimeout[4];
        char         disabled[2];
        char         devexist[4];
        macacl       maclist;
        char         lbd[4];//5g first
        char         rsvd[4][4];//reserved for future
} wHw;

typedef struct {
        struct {
                wHw     hw;
                wIface  wlan[WLANCNT];
        } w[RADIOCNT];
} wConfig;

typedef struct {
        int disabled;
        int mode;
} apQos;

typedef struct {
        uint8_t         haddr[ETHER_ADDR_LEN];
        uint8_t         flag;
        uint8_t         bssid[ETHER_ADDR_LEN];
        uint8_t         ipaddr[ADDRSTRLEN];
        time_t          runtime;
        short           rssi;
        short           channel;
        short           txrate;
        short           rxrate;
        uint64_t        inbytes;
        uint64_t        outbytes;
        uint64_t        inpkts;
        uint64_t        outpkts;
} staInfo;

typedef struct {
        char iptype[2];//0:disable, 1:static,2:dhcp
        uint8_t ipaddr[ADDRSTRLEN];
        uint8_t netmask[ADDRSTRLEN];
        uint8_t gateway[ADDRSTRLEN];
        time_t runtime;
} apinfo;

typedef struct {
        short channel;
        uint64_t txpkts;/* tx data pkts */
        uint64_t rxpkts;/* rx data pkts */
        uint64_t txbytes;/* uplink */
        uint64_t rxbytes;/* downlink */
} radioinfo;

typedef struct {
        char band[4];
        char ssid[64];
        uint8_t bssid[ETH_ALEN];
        short rssi;
        char quality;
        char quality_max;
        char encryto[32];
        uint16_t channel;
} neighbor_info;

typedef struct {
        apinfo ainfo;
        radioinfo rinfo[RADIOCNT];
        staInfo radio_sta[RADIOCNT][STACNT];
        neighbor_info nbrap[RADIOCNT][MAX_NEIGHBOR];
} apstats;

typedef struct {
        uint8_t         haddr[ETHER_ADDR_LEN];
        uint8_t         flag;
        pthread_mutex_t mutex;
        time_t          uptime;
        AP_TYPE         ap_type;
        apQos           ap_qos;
        wConfig         wconfig;
        apstats         stats;
} APInfo;

typedef struct interval {
        char enable;
        int discover;
        int echo;
        int beat;
} interval;

typedef enum {
        PRTC_TFTP_INDEX = 0,
        PRTC_FTP_INDEX = 1,
        PRTC_WGET_INDEX = 2,
        PRTC_CAPWAP_INDEX = 3,
        PRTC_NONE = 4,
} path_index;

typedef struct sconf {
        pthread_mutex_t mutex;
        //APInfo *head, *tail, *end;
        interval intval;
        int ap_num;
        char image_path[PRTC_NONE][PATHLEN];
        wConfig wcfg;
        sysupgrade upgrade[HW_MODEL_MAX_NUM];
} sconf;

#define USED  (1<<0)
#define ONLINE (1<<1)
#define OWNERDEAD (1<<2)

#define IS_OWNERDEAD(ap) ((ap)->flag&OWNERDEAD)
#define SET_OWNERDEAD(ap) ((ap)->flag |= OWNERDEAD)
#define CLR_OWNERDEAD(ap) ((ap)->flag &= (~OWNERDEAD))

#define IS_USED(ap) ((ap)->flag&USED)
#define SET_USED(ap) ((ap)->flag |= USED)
#define CLR_USED(ap) ((ap)->flag = 0)

#define IS_ONLINE(ap) ((ap)->flag&ONLINE)
#define SET_ONLINE(ap) ((ap)->flag |= ONLINE)
#define CLR_ONLINE(ap) ((ap)->flag &= (~ONLINE))

#define SC ap->ap_type
#define RC(ridx) ap->wconfig.w[ridx].hw
#define WC(ridx, widx) ap->wconfig.w[ridx].wlan[widx]
#define APINFO ap->stats.ainfo
#define RADIOINFO(ridx) ap->stats.rinfo[ridx]
#define STAINFO(ridx, staidx) ap->stats.radio_sta[ridx][staidx]
#define NBRAPINFO(ridx, apidx) ap->stats.nbrap[ridx][apidx]

int init_apinfo(void);
int get_ap_num();
APInfo* get_by_mac_lock(const uint8_t *mac, int sec);
APInfo* find_by_mac(const uint8_t *mac, int sec);
APInfo* get_by_mac(const uint8_t *mac);
void free_ap(APInfo *ap);
int clear_ap_used(APInfo *ap);
/*intval->enable,0:get the interval, 1:set the interval*/
int do_interval(struct interval *intval);
int opt_image_path(char *path, int idxpath, int flag);
int opt_auto_upgrade(sysupgrade *upgrade, int flag, int size);
extern APInfo *gAPList;
extern sconf *gconf;
//#ifdef RTLSUPPORT
#if 0
#define list_for_each(ap, ret) \
            for(ap=gAPList; \
               (ap - gAPList)<AP_SIZE; \
               pthread_mutex_unlock(&ap->mutex), ap+=1) \
            if(IS_USED(ap)&&(pthread_mutex_lock(&ap->mutex)==0))
#else
#define list_for_each(ap, ret) \
            for(ap=gAPList; \
               (ap - gAPList)<AP_SIZE; \
               pthread_mutex_unlock(&ap->mutex), ap+=1) \
            if(IS_USED(ap)&&( ret=pthread_mutex_lock(&ap->mutex), ret==0 ? true : ret==EOWNERDEAD ? pthread_mutex_consistent(&ap->mutex)==0 : false))
#endif
#endif
