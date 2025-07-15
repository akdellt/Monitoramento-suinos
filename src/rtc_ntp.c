#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

#include "inc/controle.h"
#include "configura_geral.h"

typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    bool dns_request_sent;
    struct udp_pcb *ntp_pcb;
    absolute_time_t ntp_test_time;
    alarm_id_t ntp_resend_alarm;
} NTP_T;

// 
static void ntp_result(NTP_T* state, int status, time_t *result) {
    if (status == 0 && result) {
        struct tm *utc = gmtime(result);
        printf("NTP resposta: %02d/%02d/%04d %02d:%02d:%02d \n",
               utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
               utc->tm_hour, utc->tm_min, utc->tm_sec);

        // SEMPRE INICIALIZA RTC CASO NÃO ESTEJA RODANDO
        if (!rtc_running()) {
            printf("RTC não estava rodando, iniciando... \n");
            rtc_init();
        }

        // COMEÇA NA SEXTA FEIRA, 5 DE JUNHO DE 2020 ÀS 15:45:00
        datetime_t t = {
            .year  = utc->tm_year + 1900,
            .month = utc->tm_mon + 1,
            .day   = utc->tm_mday, 
            .dotw  = utc->tm_wday,
            .hour  = utc->tm_hour,
            .min   = utc->tm_min,
            .sec   = utc->tm_sec
        };
        rtc_set_datetime(&t);
        printf("RTC sincronizado com NTP \n");
    } else {
        printf("Erro ao processar resposta do NTP \n");
    }

    // CANCELAR ALARME SE NECESSÁRIO
    if (state->ntp_resend_alarm > 0) {
        cancel_alarm(state->ntp_resend_alarm);
        state->ntp_resend_alarm = 0;
    }

    // ATUALIZA TIMESTAMP PARA PRÓXIMA SICRONIZAÇÃO
    state->ntp_test_time = make_timeout_time_ms(NTP_TEST_TIME);
    state->dns_request_sent = false;
}


static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);

// REALIZA SOLICITAÇÃO NTP
static void ntp_request(NTP_T *state) {
    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *) p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data)
{
    NTP_T* state = (NTP_T*)user_data;
    printf("Falha na solicitação ntp \n");
    ntp_result(state, -1, NULL);
    return 0;
}

// CALLBACK COM RESULTADO DO DNS
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    NTP_T *state = (NTP_T*)arg;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        printf("Endereço ntp %s\n", ip4addr_ntoa(ipaddr));
        ntp_request(state);
    } else {
        printf("Falha na solicitação do dns ntp \n");
        ntp_result(state, -1, NULL);
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    NTP_T *state = (NTP_T*)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
        mode == 0x4 && stratum != 0) {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970 - (3 * 3600);
        ntp_result(state, 0, &epoch) ;
    } else {
        printf("Resposta ntp inválida \n");
        ntp_result(state, -1, NULL);
    }
    pbuf_free(p);
}

// INICIALIZAÇÃO DO NTP
NTP_T* ntp_init(void) {
    NTP_T *state = calloc(1, sizeof(NTP_T));
    if (!state) {
        printf("failed to allocate state\n");
        return NULL;
    }
    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state->ntp_pcb) {
        printf("failed to create pcb\n");
        free(state);
        return NULL;
    }
    udp_recv(state->ntp_pcb, ntp_recv, state);
    return state;
}


void start_ntp_request(NTP_T *state) {
    if (state->dns_request_sent) return; // Requisição já em progresso

    // Agenda um alarme para o caso da requisição falhar (timeout)
    state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, state, true);

    // Inicia a busca pelo DNS do servidor NTP
    cyw43_arch_lwip_begin();
    int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_found, state);
    cyw43_arch_lwip_end();

    state->dns_request_sent = true;
    if (err == ERR_OK) {
        ntp_request(state); // Se o IP já estiver em cache, faz a requisição NTP
    } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS significa que o callback será chamado
        printf("Falha na requisição DNS\n");
        ntp_result(state, -1, NULL);
    }
}

// CHAMADA PERIÓDICA PARA VERIFICAR TEMPO
void rtc_loop_tick() { // mudar de nome talvez
    if (rtc_running()) {
        datetime_t t;
        rtc_get_datetime(&t);

        // MOSTRA HORA ATUAL -- TIRAR
        //char datetime_buf[64];
        //datetime_to_str(datetime_buf, sizeof(datetime_buf), &t);
        //printf("RTC: %s\n", datetime_buf);

        // CHAMA FUNÇÕES DE VERIFICAR TEMPOS DE ACIONAMENTO
        verificar_acionamento_racao(&t);
        verificar_acionamento_limpeza(&t);
    } else {
        printf("RTC não está rodando! \n");
    }
}
