#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <net/ethernet.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <syslog.h>
#include <unistd.h>
#include "shmutil.h"

#define DEFGROUP "default"

wConfig gWconfig = {
        {
                {
                        {"CN", "1", "13", "HT40", "6", "-90", "1", "64", "1", "0", {"0", "0", "0", {"0"}}, "0", {"0"}},
                        {
                                {"00", "00:00:00:00:00:00", "ap", "6", "3", "0", "0", "0", "0", "88888888", "wl 2.4g", "lan", "1", "0", "100", "1", "2346", "2346", "64", "0", "1", "1", "0", "0", "0", "0"},
                                {"01", "00:00:00:00:00:00", "ap", "6", "3", "0", "0", "0", "0", "88888888", "wl 2.4g", "lan", "1", "0", "100", "1", "2346", "2346", "64", "0", "1", "1", "1", "0", "0", "0"},
                                {"02", "00:00:00:00:00:00", "ap", "6", "3", "0", "0", "0", "0", "88888888", "wl 2.4g", "lan", "1", "0", "100", "1", "2346", "2346", "64", "0", "1", "1", "1", "0", "0", "0"},
                                {"03", "00:00:00:00:00:00", "ap", "6", "3", "0", "0", "0", "0", "88888888", "wl 2.4g", "lan", "1", "0", "100", "1", "2346", "2346", "64", "0", "1", "1", "1", "0", "0", "0"},
                        }
                },
                {
                        {"CN", "149", "14", "HT80", "6", "-90", "1", "64", "1", "0", {"0", "0", "0", {"0"}}, "0", {"0"}},
                        {
                                {"00", "00:00:00:00:00:00", "ap", "6", "3", "0", "0", "0", "0", "88888888", "wl 5g", "lan", "1", "0", "100", "1", "2346", "2346", "64", "0", "1", "1", "0", "0", "0", "0"},
                                {"01", "00:00:00:00:00:00", "ap", "6", "3", "0", "0", "0", "0", "88888888", "wl 5g", "lan", "1", "0", "100", "1", "2346", "2346", "64", "0", "1", "1", "1", "0", "0", "0"},
                                {"02", "00:00:00:00:00:00", "ap", "6", "3", "0", "0", "0", "0", "88888888", "wl 5g", "lan", "1", "0", "100", "1", "2346", "2346", "64", "0", "1", "1", "1", "0", "0", "0"},
                                {"03", "00:00:00:00:00:00", "ap", "6", "3", "0", "0", "0", "0", "88888888", "wl 5g", "lan", "1", "0", "100", "1", "2346", "2346", "64", "0", "1", "1", "1", "0", "0", "0"},
                        }
                },
        }
};

char gimage_path[PRTC_NONE][PATHLEN] = {
        {"192.168.111.69"},
        {"ftp://192.168.111.69/"},
        {"http://192.168.111.69/"},
        {"192.168.111.69"}
};

APInfo *gAPList;
sconf *gconf;

bool ether_addr_equal(const uint8_t *mac1, const uint8_t *mac2)
{
        uint16_t *a=(uint16_t*)mac1;
        uint16_t *b=(uint16_t*)mac2;

        return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) == 0;
}

void mutex_ownerdead_check(pthread_mutex_t *mutex)
{
        if(mutex) {
                pthread_mutex_consistent(mutex);
        }
}

APInfo* get_free_node(const uint8_t *mac, wConfig *wcfg)
{
        int i;
        wConfig cfg;
        if(pthread_mutex_lock(&gconf->mutex) == EOWNERDEAD) {
                mutex_ownerdead_check(&gconf->mutex);
        }
        if(gconf->ap_num == AP_SIZE) {
                return NULL;
        }

        for(i=0; IS_USED(gAPList+i); i++);//break untill gAPList+i UNUSED

        APInfo *ap = gAPList + i;
        gconf->ap_num++;
        if(wcfg) {
                memcpy(&cfg, wcfg, sizeof(cfg));
        } else {
                memcpy(&cfg, &gconf->wcfg, sizeof(cfg));
        }
        pthread_mutex_unlock(&gconf->mutex);

        SET_USED(ap);
        if(mac) {
                memcpy(ap->haddr, mac, ETHER_ADDR_LEN);
                //strncpy(ap->ap_type.group, DEFGROUP, GRPLEN);
                memcpy(&ap->wconfig, &cfg, sizeof(ap->wconfig));
        }
        if(!wcfg) {
                if(pthread_mutex_lock(&ap->mutex) == EOWNERDEAD) {
                        DBG("Ower dir\n");
                        SET_OWNERDEAD(ap);
                        mutex_ownerdead_check(&ap->mutex);
                }
        }
        return ap;
}


APInfo* find_by_mac(const uint8_t *mac, int sec)
{
        int i=0, ret=0;
        struct timespec tout;

        if(!mac) {
                return NULL;
        }

        if(sec) {
                clock_gettime(CLOCK_REALTIME, &tout);
                tout.tv_sec += sec;
        }
#if 0
        for(i=0; IS_USED(gAPList+i); i++) {
#else
        for(i=0; i<AP_SIZE; i++) {
#endif
                if(IS_USED(gAPList+i) && ether_addr_equal(mac, gAPList[i].haddr)) {
                        if(sec) {
                                ret = pthread_mutex_timedlock(&(gAPList+i)->mutex, &tout);
                                if(ret == EOWNERDEAD) {
                                        DBG("Ower dir\n");
                                        SET_OWNERDEAD(gAPList+i);
                                        mutex_ownerdead_check(&(gAPList+i)->mutex);
                                } else if(ret == ETIMEDOUT) {
                                        DBG("Time out\n");
                                        return NULL;
                                }
                        } else {
                                if(pthread_mutex_trylock(&gAPList[i].mutex) == EOWNERDEAD) {
                                        DBG("Ower dir\n");
                                        SET_OWNERDEAD(gAPList+i);
                                        mutex_ownerdead_check(&(gAPList+i)->mutex);
                                }
                        }

                        return gAPList+i;
                }
        }
        return NULL;
}

APInfo* get_by_mac(const uint8_t *mac)
{
        int i=0;

        if(!mac) {
                return NULL;
        }
#if 0
        for(i=0; IS_USED(gAPList+i); i++) {
#else
        for(i=0; i<AP_SIZE; i++) {
#endif
                if(IS_USED(gAPList+i) && ether_addr_equal(mac, gAPList[i].haddr)) {
                        return gAPList+i;
                }
        }

        return NULL;
}

APInfo* get_by_mac_lock(const uint8_t *mac, int sec)
{
        APInfo *ap;
        if((ap = find_by_mac(mac, sec)) != NULL) {
                return ap;
        }
        if ((ap=get_by_mac(mac)) != NULL) {
                return ap;
        }
#ifdef HAVE_AC
        return get_free_node(mac, NULL);
#else
        return NULL;
#endif
}

void free_ap(APInfo *ap)
{
        pthread_mutex_unlock(&ap->mutex);
}

int get_ap_num()
{
        int num;
        if(pthread_mutex_lock(&gconf->mutex) == EOWNERDEAD) {
                mutex_ownerdead_check(&gconf->mutex);
        }
        num = gconf->ap_num;
        pthread_mutex_unlock(&gconf->mutex);
        return num;
}

int clear_ap_used(APInfo *ap)
{
        if(!ap) {
                perror("ap is null");
                return -1;
        } else {
                if(pthread_mutex_lock(&gconf->mutex) == EOWNERDEAD) {
                        mutex_ownerdead_check(&gconf->mutex);
                }
                if(gconf->ap_num > 0) {
                        gconf->ap_num--;
                }
                pthread_mutex_unlock(&gconf->mutex);
                CLR_USED(ap);
                bzero(ap->haddr, ETHER_ADDR_LEN);
                //free_ap(ap);
        }
        return 0;
}

int do_interval(struct interval *intval)
{
        if(intval) {
                if(pthread_mutex_lock(&gconf->mutex) == EOWNERDEAD) {
                        mutex_ownerdead_check(&gconf->mutex);
                }

                if(intval->enable == 1) { //set interval
                        memcpy(&(gconf->intval), intval, sizeof(gconf->intval));
                } else { //get interval
                        memcpy(intval, &(gconf->intval), sizeof(gconf->intval));
                }
                pthread_mutex_unlock(&gconf->mutex);
        }
        return -1;
}

int opt_image_path(char *path, int idxpath, int flag)
{
        if(pthread_mutex_lock(&gconf->mutex) == EOWNERDEAD) {
                mutex_ownerdead_check(&gconf->mutex);
        }

        if((path) && (idxpath >= 0) && (idxpath < PRTC_NONE)) {
                if(flag == 1) { //set path
                        memcpy(&(gconf->image_path[idxpath]), path, sizeof(gconf->image_path[idxpath]));
                } else { //get path
                        memcpy(path, &(gconf->image_path[idxpath]), sizeof(gconf->image_path[idxpath]));
                }
        }
        pthread_mutex_unlock(&gconf->mutex);
        return 0;
}

/*****************************************************
 * flag:
 * 	0 get all the configure;
 * 	1 set the configure;
 * 	2 delete the configure.
 *****************************************************/
int opt_auto_upgrade(sysupgrade *upgrade, int flag, int size)
{
        int i, j = -1, index;
        int cnt = 0;
        if(pthread_mutex_lock(&gconf->mutex) == EOWNERDEAD) {
                mutex_ownerdead_check(&gconf->mutex);
        }

        if(upgrade) {
                if(flag != 0) {
                        for(i=0; i<HW_MODEL_MAX_NUM; i++) {
                                if(strlen(gconf->upgrade[i].hwmodel) == 0) {
                                        j = i;
                                }
                                if(!strcmp(gconf->upgrade[i].hwmodel, upgrade->hwmodel)) {
                                        break;
                                }
                        }

                        if(i < HW_MODEL_MAX_NUM) {
                                index = i;
                        } else if((j >= 0) && (j < HW_MODEL_MAX_NUM)) {
                                index = j;
                        } else {
                                pthread_mutex_unlock(&gconf->mutex);
                                return -1;
                        }
                }

                if(flag == 1) { //set configure
                        memcpy(&gconf->upgrade[index], upgrade, sizeof(gconf->upgrade[index]));
                } else if(flag == 2) {//delete configure
                        memset(&gconf->upgrade[index], 0, sizeof(gconf->upgrade[index]));
                } else { //get all the configure
                        if(size  < HW_MODEL_MAX_NUM * sizeof(sysupgrade)) {
                                perror("Not enough memory for getting auto upgrade");
                        } else {
                                for(i=0; i<HW_MODEL_MAX_NUM; i++) {
                                        if(strlen(gconf->upgrade[i].hwmodel) > 0) {
                                                memcpy(&upgrade[cnt++], &gconf->upgrade[i], sizeof(gconf->upgrade[i]));
                                        }
                                }
                        }
                }
        }
        pthread_mutex_unlock(&gconf->mutex);
        return cnt;
}

int init_apinfo(void)
{
        key_t key=SYSKEY;
        int shmid, i=0;
        int exist = 0;
        char *shm = NULL;
        pthread_mutexattr_t mattr;

        if ((shmid=shmget(key, sizeof(APInfo)*AP_SIZE+sizeof(*gconf), IPC_CREAT | IPC_EXCL | 0666)) < 0) {
                if(errno == EEXIST) {
                        if((shmid=shmget(key, sizeof(APInfo)*AP_SIZE+sizeof(*gconf), 0666)) < 0) {
                                exit(1);
                        }
                        printf("have shared\n");
                        exist = 1;
                } else {
                        exit(1);
                }
        }

        if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
                exit(1);
        }

        if(exist) {
                gconf = (void*)shm;
                gAPList = (APInfo*)(gconf+1);
#ifdef HAVE_AC
                if(pthread_mutex_lock(&gconf->mutex) == EOWNERDEAD) {
                        mutex_ownerdead_check(&gconf->mutex);
                }
                memset(&(gconf->intval), 0, sizeof(gconf->intval));
                gconf->ap_num = 0;
                //memcpy(&gconf->wcfg, &gWconfig, sizeof(gconf->wcfg));
                for(i=0; i<RADIOCNT; i++) {
                        if(i == 0)
                                memcpy(&gconf->wcfg.w[i], &gWconfig.w[DFTCFG2G], sizeof(gWconfig.w[DFTCFG2G]));
                        else
                                memcpy(&gconf->wcfg.w[i], &gWconfig.w[DFTCFG5G], sizeof(gWconfig.w[DFTCFG5G]));
                        memset(&(gconf->wcfg.w[i].hw.maclist), 0, sizeof(gconf->wcfg.w[i].hw.maclist));
                }
                pthread_mutex_unlock(&gconf->mutex);

                for(i=0; i<AP_SIZE; i++) {
                        APInfo *ap;
                        ap = gAPList+i;

                        if(pthread_mutex_lock(&ap->mutex) == EOWNERDEAD) {
                                mutex_ownerdead_check(&ap->mutex);
                        }
                        ap->flag = 0;
                        ap->uptime = 0;
                        bzero(ap->haddr, ETHER_ADDR_LEN);
                        bzero(&ap->ap_type, sizeof(AP_TYPE));
                        bzero(&ap->ap_qos, sizeof(apQos));
                        bzero(&ap->wconfig, sizeof(wConfig));
                        bzero(&ap->stats, sizeof(apstats));

                        pthread_mutex_unlock(&ap->mutex);
                }
#endif
                return 0;
        }

        memset(shm, 0x0, sizeof(APInfo)*AP_SIZE+sizeof(*gconf));

        pthread_mutexattr_init(&mattr);
#if 0
        if(pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK) < 0) {
                perror("HEllo");
                exit(1);
        }
#else
        if(pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL) < 0) {
                perror("HEllo");
                exit(1);
        }
#endif
        if(pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED) < 0) {
                perror("HEllo");
                exit(1);
        }

        if(pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST) < 0) {
                perror("HEllo");
                exit(1);
        }

        gconf = (void*)shm;
        pthread_mutex_init(&gconf->mutex, &mattr);

        gAPList = (APInfo*)(gconf+1);
        //gconf->head = gAPList;
        //gconf->tail = gAPList;
        //gconf->end = gAPList + AP_SIZE;
        gconf->ap_num = 0;
        memcpy(gconf->image_path, gimage_path, sizeof(gconf->image_path));
        //memcpy(&gconf->wcfg, &gWconfig, sizeof(gconf->wcfg));
        memset(gconf->upgrade, 0, sizeof(gconf->upgrade));
        for(i=0; i<RADIOCNT; i++) {
                if(i == 0)
                        memcpy(&gconf->wcfg.w[i], &gWconfig.w[DFTCFG2G], sizeof(gWconfig.w[DFTCFG2G]));
                else
                        memcpy(&gconf->wcfg.w[i], &gWconfig.w[DFTCFG5G], sizeof(gWconfig.w[DFTCFG5G]));
                memset(&(gconf->wcfg.w[i].hw.maclist), 0, sizeof(gconf->wcfg.w[i].hw.maclist));
        }

        for(i=0; i<AP_SIZE; i++) {
                pthread_mutex_init(&(gAPList+i)->mutex, &mattr);
        }
        memcpy(&gconf->wcfg, &gWconfig, sizeof(gconf->wcfg));
        return 0;
}

