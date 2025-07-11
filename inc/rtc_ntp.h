#ifndef RCT_NTP_H
#define RCT_NTP_H

#include <stdbool.h>

typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    bool dns_request_sent;
    struct udp_pcb *ntp_pcb;
    absolute_time_t ntp_test_time;
    alarm_id_t ntp_resend_alarm;
} NTP_T;

void rtc_loop_tick();

NTP_T* ntp_init(void);

void start_ntp_request(NTP_T *state);

#endif 